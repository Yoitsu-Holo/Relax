# RAlloc - Unified Memory Allocator

RAlloc is a high-performance memory allocator that combines slab allocation, buddy system, and malloc to provide efficient memory management across different size ranges.

## Features

- **Multi-tier allocation strategy:**
  - `[16B, 4KiB)`: Slab allocator for small objects
  - `[4KiB, 256KiB]`: Buddy system for medium-sized allocations
  - `>256KiB`: Standard malloc for large allocations

- **Automatic allocation tracking**: Properly identifies and frees memory from the correct allocator
- **High performance**: Optimized for speed with minimal overhead
- **Thread-safe**: Can be extended for multi-threaded environments

## Architecture

RAlloc maintains a three-tier allocation system:

```
RAlloc
├── SlabManager (single instance)
│   └── Multiple SlabAllocators (64KiB each, for different size classes)
├── BuddyManager (single instance)
│   └── Multiple BuddyAllocators (16MiB each, on-demand creation)
└── System malloc (for large allocations >256KiB)
```

### Slab Allocator ([16B, 4KiB))

RAlloc uses a SlabManager that maintains multiple SlabAllocators for different size classes:
- Supported sizes: 16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072 bytes
- Each slab is 64KiB aligned for fast metadata access
- Metadata stored in the first 512 bytes of each 64KiB slab
- O(1) allocation and deallocation using two-level bitmaps

### Buddy System ([4KiB, 256KiB])

- Managed by BuddyManager with multiple 16MiB BuddyAllocators
- Base block size: 4KiB
- Supports orders 0-6 (4KiB to 256KiB)
- Fast O(1) allocation using two-level bitmaps
- Automatic buddy coalescing on free

### Malloc (>256KiB)

- Falls back to system malloc for very large allocations
- Adds a small header (MallocHeader) for tracking
- Header contains magic number, size, and allocation type

## Allocation Type Identification

RAlloc automatically identifies the allocation type during deallocation:

1. **Slab**: Checks 64KiB alignment and metadata magic number
2. **Buddy**: Checks 4KiB alignment within 16MiB-aligned blocks
3. **Malloc**: Validates header with magic number

This allows the single `ralloc_free()` function to correctly route frees to the appropriate allocator.

## Usage

### Basic Usage

```cpp
#include "ralloc.h"

int main() {
    // Get global instance
    RAlloc* allocator = ralloc_get_instance();

    // Or use convenience functions
    void* ptr1 = ralloc_malloc(128);      // Uses slab
    void* ptr2 = ralloc_malloc(8192);     // Uses buddy
    void* ptr3 = ralloc_malloc(1048576);  // Uses malloc

    ralloc_free(ptr1);
    ralloc_free(ptr2);
    ralloc_free(ptr3);

    return 0;
}
```

### Advanced Usage

```cpp
#include "ralloc.h"

int main() {
    RAlloc allocator;

    // Initialize with custom limits
    if (!allocator.init(512, 128)) {
        // max_slabs=512, max_buddies=128
        return 1;
    }

    // Allocate memory
    void* ptr = allocator.allocate(1024);

    // Use the memory
    // ...

    // Free memory
    allocator.deallocate(ptr);

    // Get statistics
    size_t free_mem = allocator.get_total_free_memory();
    size_t slab_count = allocator.get_slab_count();
    size_t buddy_count = allocator.get_buddy_count();

    return 0;
}
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

To disable tests:
```bash
cmake -DBUILD_RALLOC_TESTS=OFF ..
make
```

## Performance Characteristics

| Size Range | Allocator | Time Complexity | Space Overhead |
|------------|-----------|-----------------|----------------|
| 16B-4KiB | Slab | O(1) | ~1.5% (metadata) |
| 4KiB-256KiB | Buddy | O(1) | ~0.4% (metadata) |
| >256KiB | Malloc | Varies | 24 bytes (header) |

## Memory Layout

### Slab Layout (64KiB per slab)
```
[0-512B: Metadata] [512B-64KiB: Data blocks]
```

### Buddy Layout (16MiB per allocator)
```
[16MiB aligned data blocks]
```

### Malloc Layout
```
[MallocHeader: 24B] [User data: size bytes]
```

## Dependencies

- C++17 or later
- SlabAllocator (../slab)
- BuddyManager and BuddyAllocator (../buddySystem)
- Standard library: `<cstdlib>`, `<cstring>`, `<vector>`, `<algorithm>`

## Limitations

- Maximum number of slab allocators: configurable (default 256)
- Maximum number of buddy allocators: configurable (default 64)
- Minimum allocation size: 16 bytes
- Not thread-safe by default (can be extended with locks)

## Future Improvements

- Thread-local caches for improved multi-threaded performance
- Memory pool warming/preallocation
- Statistics and profiling support
- Memory usage reporting and debugging tools
- NUMA-aware allocation

## License

Same as the parent project.
