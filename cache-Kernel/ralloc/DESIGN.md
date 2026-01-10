# RAlloc Design Document

## Overview

RAlloc is a unified memory allocator that combines three allocation strategies to efficiently handle allocations of different sizes.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                          RAlloc                             │
│                    (Unified Interface)                      │
└────────────┬──────────────┬─────────────┬──────────────────┘
             │              │             │
   ┌─────────▼──────┐  ┌────▼─────┐  ┌───▼────────┐
   │  SlabManager   │  │  Buddy   │  │   malloc   │
   │   (1 instance) │  │  Manager │  │  (system)  │
   │                │  │(1 instance│  │            │
   └────────┬───────┘  └────┬─────┘  └────────────┘
            │               │
   ┌────────▼────────┐  ┌───▼──────────┐
   │ SlabAllocator[] │  │BuddyAllocator│
   │  (on-demand)    │  │  [] (on-    │
   │                 │  │   demand)    │
   └─────────────────┘  └──────────────┘
```

## Components

### 1. RAlloc (Top Level)
- **Responsibility**: Route allocation requests to appropriate sub-allocator
- **Instance count**: Typically 1 (singleton pattern via `ralloc_get_instance()`)
- **Members**:
  - `SlabManager* slab_manager_` - single instance
  - `BuddyManager* buddy_manager_` - single instance

### 2. SlabManager
- **Responsibility**: Manage multiple SlabAllocators for different size classes
- **Instance count**: 1 per RAlloc instance
- **Size classes**: 16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072 bytes
- **Creates**: SlabAllocator instances on-demand (one per size class)
- **Max SlabAllocators**: Configurable (default: 256)

### 3. SlabAllocator
- **Responsibility**: Fast O(1) allocation for fixed-size blocks
- **Instance count**: Multiple (managed by SlabManager)
- **Size**: 64 KiB each (aligned)
- **Structure**:
  - Metadata: 512 bytes (bitmap)
  - Data: ~63.5 KiB (actual allocations)
- **Identification**: 64 KiB alignment + metadata type field

### 4. BuddyManager
- **Responsibility**: Manage multiple BuddyAllocators
- **Instance count**: 1 per RAlloc instance
- **Size range**: 4 KiB to 256 KiB (orders 0-6)
- **Creates**: BuddyAllocator instances on-demand
- **Max BuddyAllocators**: Configurable (default: 64)

### 5. BuddyAllocator
- **Responsibility**: Handle medium-sized allocations with buddy system
- **Instance count**: Multiple (managed by BuddyManager)
- **Size**: 16 MiB each (aligned)
- **Base block**: 4 KiB
- **Orders**: 0-6 (4KiB, 8KiB, 16KiB, 32KiB, 64KiB, 128KiB, 256KiB)
- **Identification**: 4 KiB alignment within 16 MiB boundaries

### 6. System malloc
- **Responsibility**: Handle large allocations
- **Size range**: > 256 KiB
- **Header**: 24 bytes (magic number, size, type)
- **Identification**: Magic number in header

## Allocation Flow

```
User calls: ralloc_malloc(size)
              │
              ▼
         RAlloc::allocate(size)
              │
              ├─[16B ≤ size < 4KiB]──────► SlabManager::allocate()
              │                                    │
              │                                    ├─Find/create SlabAllocator
              │                                    │  for size class
              │                                    │
              │                                    └─Return block
              │
              ├─[4KiB ≤ size ≤ 256KiB]───► BuddyManager::allocate()
              │                                    │
              │                                    ├─Find/create BuddyAllocator
              │                                    │  with available space
              │                                    │
              │                                    └─Return block
              │
              └─[size > 256KiB or other]─► malloc() + header
                                                   │
                                                   ├─Add MallocHeader
                                                   │
                                                   └─Return ptr after header
```

## Deallocation Flow

```
User calls: ralloc_free(ptr)
              │
              ▼
         RAlloc::deallocate(ptr)
              │
              ▼
         identify_allocation(ptr)
              │
              ├─Check metadata (64KiB aligned)──► ALLOC_SLAB
              │                                         │
              │                                         └─SlabAllocator::deallocate()
              │
              ├─Check alignment (4KiB aligned)──► ALLOC_BUDDY
              │                                         │
              │                                         └─BuddyManager::deallocate()
              │
              └─Check header magic──────────────► ALLOC_MALLOC
                                                        │
                                                        └─free(ptr - header_size)
```

## Memory Layout

### SlabAllocator (64 KiB)
```
┌──────────────────┬────────────────────────────────────┐
│   Metadata       │         Allocation Blocks          │
│   512 bytes      │          ~63.5 KiB                 │
│                  │                                    │
│  - manager info  │  [block 0][block 1]...[block N]   │
│  - idx0 (L1)     │                                    │
│  - idx1[] (L2)   │                                    │
└──────────────────┴────────────────────────────────────┘
0                512                                 64KiB
```

### BuddyAllocator (16 MiB)
```
┌────────────────────────────────────────────────────────┐
│              Allocation Blocks (4KiB base)             │
│                                                        │
│  [4KiB blocks with buddy system management]           │
│                                                        │
│  Metadata stored separately in bitmaps                │
└────────────────────────────────────────────────────────┘
0                                                     16MiB
```

### Malloc Layout
```
┌─────────────┬──────────────────────────────┐
│ MallocHeader│     User Data                │
│  24 bytes   │     size bytes               │
│             │                              │
│ - magic     │                              │
│ - size      │                              │
│ - type      │                              │
└─────────────┴──────────────────────────────┘
              ▲
              └─ Returned pointer
```

## Design Decisions

### Why Three Tiers?

1. **Slab [16B-4KiB)**:
   - Extremely fast O(1) allocation/deallocation
   - Minimal overhead (~1.5%)
   - Perfect for small, frequently allocated objects
   - No external fragmentation within a size class

2. **Buddy [4KiB-256KiB]**:
   - Efficient for medium-sized allocations
   - Good balance of speed and flexibility
   - Automatic coalescing reduces fragmentation
   - Power-of-2 sizes work well with system page size

3. **Malloc [>256KiB]**:
   - System malloc is already optimized for large allocations
   - Avoids duplicating complex large allocation logic
   - Small header overhead is negligible for large sizes

### Why Single Manager Instances?

- **SlabManager**: Centralizes size class management, prevents duplication
- **BuddyManager**: Coordinates buddy allocator selection and lifecycle
- **Simplicity**: Clear ownership and lifecycle management
- **Statistics**: Easy to track overall memory usage

### Size Boundaries

- **16B minimum**: Smaller than typical pointer size, not worth specialized handling
- **4KiB boundary**: Matches system page size, natural boundary for buddy system
- **256KiB maximum**: Beyond this, malloc's mmap-based allocation is efficient

## Performance Characteristics

| Size Range  | Allocator | Time      | Space Overhead | Fragmentation |
|-------------|-----------|-----------|----------------|---------------|
| 16B-4KiB    | Slab      | O(1)      | ~1.5%          | None (internal)|
| 4KiB-256KiB | Buddy     | O(1)      | ~0.4%          | Low (power-2) |
| >256KiB     | Malloc    | O(log n)* | 24B fixed      | Varies        |

*Depends on system malloc implementation

## Example Usage Patterns

### Pattern 1: Small object cache
```cpp
// Many small allocations (e.g., cache entries)
for (int i = 0; i < 10000; i++) {
    void* entry = ralloc_malloc(128);  // Uses Slab
    // Use entry...
    ralloc_free(entry);
}
```

### Pattern 2: Medium buffers
```cpp
// Network buffers, file I/O
void* buffer = ralloc_malloc(64 * 1024);  // Uses Buddy
// Use buffer...
ralloc_free(buffer);
```

### Pattern 3: Large data
```cpp
// Large datasets, images
void* data = ralloc_malloc(4 * 1024 * 1024);  // Uses malloc
// Use data...
ralloc_free(data);
```

## Thread Safety

**Current Status**: Not thread-safe

**Future Enhancement**: Can add:
- Per-thread caches for SlabManager
- Lock-free operations for single allocator access
- Mutex protection for manager-level operations

## Limitations

1. **Slab size classes**: Fixed at 13 sizes, may not be optimal for all workloads
2. **Max allocators**: Hard limits (default 256 slabs, 64 buddies)
3. **Not thread-safe**: Requires external synchronization
4. **Alignment**: Buddy allocations always 4KiB aligned (may waste space)

## Future Improvements

1. **Configurable size classes**: Allow runtime size class configuration
2. **Thread-local caching**: Reduce contention in multi-threaded scenarios
3. **Memory pressure handling**: Return empty allocators to system
4. **Statistics**: Detailed allocation tracking and profiling
5. **NUMA awareness**: Allocate from local NUMA nodes
