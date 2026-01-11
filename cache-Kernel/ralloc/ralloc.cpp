/*
 * RAlloc - Unified Memory Allocator (Optimized)
 *
 * Architecture:
 *   RAlloc (single instance)
 *   ├── Fixed SlabManager[14] (auto-expanding, one per predefined size)
 *   ├── BuddyManager (for allocations > 4KiB)
 *   └── System malloc (for allocations > 256KiB)
 *
 * Allocation Strategy:
 *   [1B, 4KiB]     -> Fixed SlabManager (with O(1) lookup table)
 *   (4KiB, 256KiB] -> BuddyManager
 *   >256KiB        -> malloc with tracking header
 *
 * Optimization:
 *   - Uses SlabManager for dynamic expansion of slab allocators
 *   - Compact 128-byte lookup table for O(1) slab index resolution
 *   - Each uint8 in lookup table stores two indices (4 bits each)
 */

#include "ralloc.h"
#include "../buddySystem/buddy_manager.h"
#include <cstdlib>
#include <cstring>

// Forward declaration for alignment check
#ifndef KiB
#define KiB 1024
#define MiB (1024 * KiB)
#endif

// Allocation type constants (matching RAlloc::AllocType)
// Using bit encoding: low 4 bits for type, high 4 bits for slab_idx
// - Buddy: 0x01 (1)
// - Malloc: 0x02 (2)
// - Slab: (slab_idx << 4), results in 0x00, 0x10, 0x20, ..., 0xD0
constexpr uint8_t ALLOC_UNKNOWN = 0;
constexpr uint8_t ALLOC_SLAB = 1;   // Not used directly, kept for compatibility
constexpr uint8_t ALLOC_BUDDY = 1;  // Low 4 bits = 1
constexpr uint8_t ALLOC_MALLOC = 2; // Low 4 bits = 2

// Malloc header for large allocations
struct MallocHeader
{
    uint64_t magic;     // Magic number to identify malloc allocations
    size_t size;        // Allocation size
    uint8_t alloc_type; // ALLOC_MALLOC

    static constexpr uint64_t MAGIC_NUMBER = 0xDEADBEEFCAFEBABEULL;
};

// RAlloc implementation

RAlloc::RAlloc()
    : buddy_manager_(nullptr),
      initialized_(false)
{
    // Initialize slab array to nullptr
    for (size_t i = 0; i < SLAB_COUNT; ++i)
    {
        slabs_[i] = nullptr;
    }

    // Initialize lookup table to 0
    memset(slab_lookup_table_, 0, sizeof(slab_lookup_table_));
}

RAlloc::~RAlloc()
{
    // Cleanup slab managers
    for (size_t i = 0; i < SLAB_COUNT; ++i)
    {
        if (slabs_[i])
        {
            delete slabs_[i]; // SlabManager was allocated with new
            slabs_[i] = nullptr;
        }
    }

    if (buddy_manager_)
    {
        delete buddy_manager_;
        buddy_manager_ = nullptr;
    }
}

void RAlloc::build_lookup_table()
{
    // Build compact lookup table for sizes [1, 4096]
    // lookup_idx ranges from 0 to 255 (for sizes 1-4096, divided by 16)
    uint8_t lookup_raw[256];

    // For each lookup_idx, find the appropriate slab index
    for (size_t lookup_idx = 0; lookup_idx < 256; ++lookup_idx)
    {
        // Size range: [(lookup_idx * 16 + 1), (lookup_idx + 1) * 16]
        size_t max_size = (lookup_idx + 1) * 16;

        // Find the smallest slab that can fit max_size
        uint8_t slab_idx = 0;
        for (uint8_t i = 0; i < SLAB_COUNT; ++i)
        {
            if (max_size <= SLAB_SIZES[i])
            {
                slab_idx = i;
                break;
            }
        }
        lookup_raw[lookup_idx] = slab_idx;
    }

    // Pack into compact format: each uint8 stores two indices
    // Low 4 bits = even lookup_idx, High 4 bits = odd lookup_idx
    for (size_t i = 0; i < 128; ++i)
    {
        uint8_t even_idx = lookup_raw[i * 2];    // Low 4 bits
        uint8_t odd_idx = lookup_raw[i * 2 + 1]; // High 4 bits
        slab_lookup_table_[i] = (odd_idx << 4) | even_idx;
    }
}

bool RAlloc::init(size_t max_buddies)
{
    if (initialized_)
        return false;

    // Build the compact lookup table
    build_lookup_table();

    // Initialize all fixed slab managers
    for (size_t i = 0; i < SLAB_COUNT; ++i)
    {
        // Create new SlabManager
        slabs_[i] = new SlabManager();
        if (!slabs_[i])
        {
            // Cleanup previously allocated slabs
            for (size_t j = 0; j < i; ++j)
            {
                delete slabs_[j];
                slabs_[j] = nullptr;
            }
            return false;
        }

        // Initialize SlabManager with the corresponding block size
        if (slabs_[i]->init(SLAB_SIZES[i]) != 0)
        {
            // Cleanup on failure
            for (size_t j = 0; j <= i; ++j)
            {
                delete slabs_[j];
                slabs_[j] = nullptr;
            }
            return false;
        }
    }

    // Initialize buddy manager
    buddy_manager_ = new BuddyManager();
    if (!buddy_manager_ || !buddy_manager_->init(max_buddies))
    {
        delete buddy_manager_;

        // Cleanup slabs
        for (size_t i = 0; i < SLAB_COUNT; ++i)
        {
            delete slabs_[i];
            slabs_[i] = nullptr;
        }
        return false;
    }

    initialized_ = true;
    return true;
}

inline uint8_t RAlloc::get_slab_index(size_t size) const
{
    // Fast O(1) lookup using compact table
    // Calculate lookup index: (size - 1) / 16
    size_t lookup_idx = (size - 1) >> 4;

    // Bounds check (should not happen if size <= 4096)
    if (__builtin_expect(lookup_idx >= 256, 0))
        return SLAB_COUNT - 1; // Use largest slab (4096)

    // Extract slab index from packed table
    uint8_t packed = slab_lookup_table_[lookup_idx >> 1];
    uint8_t slab_idx = (lookup_idx & 1) ? (packed >> 4) : (packed & 0x0F);

    return slab_idx;
}

void *RAlloc::allocate(size_t size)
{
    if (!initialized_ || size == 0)
        return nullptr;

    // [1B, 4KiB） - use fixed slab managers
    if (size < 4 * KiB)
    {
        uint8_t slab_idx = get_slab_index(size);
        void *ptr = slabs_[slab_idx]->allocate();

        if (ptr != nullptr)
        {
            // Record slab allocation with encoded value (64KiB aligned)
            // Encode: slab_idx << 4 to avoid conflict with ALLOC_BUDDY(1) and ALLOC_MALLOC(2)
            uint64_t aligned_addr = reinterpret_cast<uint64_t>(ptr) & ~(64 * KiB - 1);
            alloc_map_[aligned_addr] = slab_idx << 4;
            return ptr;
        }
        // Slab is full, fall through to buddy/malloc
        exit(-1);
        return nullptr;
    }

    // [4KiB, 256KiB] - use buddy
    if (size >= 4 * KiB && size <= 256 * KiB)
    {
        void *ptr = buddy_manager_->allocate(size);

        if (ptr != nullptr)
        {
            // Record buddy allocation (16MiB aligned)
            uint64_t aligned_addr = reinterpret_cast<uint64_t>(ptr) & ~(16 * MiB - 1);
            alloc_map_[aligned_addr] = ALLOC_BUDDY;
            return ptr;
        }
        exit(-1);
        return nullptr;
    }

    // For >256KiB or slab overflow - use malloc with header
    size_t total_size = sizeof(MallocHeader) + size;
    void *raw_mem = malloc(total_size);
    if (!raw_mem)
        return nullptr;

    // Write header
    MallocHeader *header = static_cast<MallocHeader *>(raw_mem);
    header->magic = MallocHeader::MAGIC_NUMBER;
    header->size = size;
    header->alloc_type = ALLOC_MALLOC;

    // Return pointer after header (no need to record in map)
    return static_cast<uint8_t *>(raw_mem) + sizeof(MallocHeader);
}

void RAlloc::deallocate(void *ptr)
{
    if (!ptr || !initialized_)
        return;

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    // Try 64KiB alignment (slab)
    uint64_t aligned_64k = addr & ~(64 * KiB - 1);
    auto it = alloc_map_.find(aligned_64k);

    if (it != alloc_map_.end())
    {
        uint8_t value = it->second;

        // Decode allocation type using bit pattern
        if (value == ALLOC_BUDDY)
        {
            // This shouldn't happen for 64KiB aligned address
            // Fall through to check 16MiB alignment
        }
        else if (value == ALLOC_MALLOC)
        {
            // This shouldn't happen for 64KiB aligned address
            // Fall through to malloc handling
        }
        else if ((value & 0x0F) == 0)
        {
            // Slab allocation: decode slab_idx from high 4 bits
            uint8_t slab_idx = value >> 4;
            if (slab_idx < SLAB_COUNT)
            {
                slabs_[slab_idx]->deallocate(ptr);
                return;
            }
        }
    }

    // Try 16MiB alignment (buddy)
    uint64_t aligned_16m = addr & ~(16 * MiB - 1);
    it = alloc_map_.find(aligned_16m);

    if (it != alloc_map_.end() && it->second == ALLOC_BUDDY)
    {
        // Buddy allocation
        buddy_manager_->deallocate(ptr);
        return;
    }

    // Must be malloc allocation
    uint8_t *raw_mem = static_cast<uint8_t *>(ptr) - sizeof(MallocHeader);
    free(raw_mem);
}

size_t RAlloc::get_total_free_memory() const
{
    if (!initialized_)
        return 0;

    size_t total = 0;

    // Note: SlabManager doesn't expose free memory information
    // Only counting buddy free memory

    // Buddy free memory
    if (buddy_manager_)
    {
        total += buddy_manager_->get_total_free_memory();
    }

    return total;
}

size_t RAlloc::get_slab_count() const
{
    if (!initialized_)
        return 0;

    // Count total initialized SlabAllocators across all SlabManagers
    size_t count = 0;
    for (size_t i = 0; i < SLAB_COUNT; ++i)
    {
        if (slabs_[i])
            count += slabs_[i]->get_initialized_count();
    }
    return count;
}

size_t RAlloc::get_buddy_count() const
{
    if (!initialized_ || !buddy_manager_)
        return 0;

    return buddy_manager_->get_allocator_count();
}
