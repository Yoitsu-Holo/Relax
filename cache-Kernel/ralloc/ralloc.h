#ifndef RALLOC_H
#define RALLOC_H

#include <cstddef>
#include <cstdint>
#include "../../lib/ankerl_unordered_dense/unordered_dense.h"
#include "../slab/slab_manager.h"
#include "../buddySystem/buddy_manager.h"

// Ralloc - Unified memory allocator
// Uses slab for [16B, 4KiB], buddy for (4KiB, 256KiB], malloc for >256KiB
// Optimized with fixed slab allocators and compact O(1) lookup table

class RAlloc
{
public:
    RAlloc();
    ~RAlloc();

    // Disable copy and assignment
    RAlloc(const RAlloc &) = delete;
    RAlloc &operator=(const RAlloc &) = delete;

    // Initialize the allocator with maximum number of buddy allocators
    bool init(size_t max_buddies = 64);

    // Allocate memory
    void *allocate(size_t size);

    // Free memory
    void deallocate(void *ptr);

    // Statistics
    size_t get_total_free_memory() const;
    size_t get_slab_count() const;
    size_t get_buddy_count() const;

private:
    // Helper functions
    enum AllocType
    {
        ALLOC_UNKNOWN = 0,
        ALLOC_SLAB = 1,
        ALLOC_BUDDY = 2,
        ALLOC_MALLOC = 3
    };

    // Build the compact lookup table during initialization
    void build_lookup_table();

    // Fast slab lookup: O(1) using compact table
    inline uint8_t get_slab_index(size_t size) const;

private:
    // Fixed slab allocators for predefined block sizes (from slab_allocator.h)
    static constexpr size_t SLAB_COUNT = 14;
    static constexpr size_t SLAB_SIZES[SLAB_COUNT] = {
        16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096};

    // Compact lookup table: each uint8 stores two slab indices (high 4 bits, low 4 bits)
    // Lookup: lookup_idx = (size - 1) >> 4, then extract from table[lookup_idx / 2]
    uint8_t slab_lookup_table_[128];

    SlabManager *slabs_[SLAB_COUNT]; // Fixed slab managers, one per block size
    BuddyManager *buddy_manager_;    // BuddyManager for (4KiB, 256KiB]
    bool initialized_;

    // Address tracking map: aligned_address -> AllocType
    // For slab: key = ptr & ~(64KiB-1), value = ALLOC_SLAB
    // For buddy: key = ptr & ~(16MiB-1), value = ALLOC_BUDDY
    ankerl::unordered_dense::map<uint64_t, uint8_t> alloc_map_;
};

#endif // RALLOC_H
