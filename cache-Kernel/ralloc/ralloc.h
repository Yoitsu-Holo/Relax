#ifndef RALLOC_H
#define RALLOC_H

#include <cstddef>
#include <cstdint>

// Ralloc - Unified memory allocator
// Uses slab for [16B, 4KiB), buddy for [4KiB, 256KiB], malloc for >256KiB

class RAlloc
{
public:
    RAlloc();
    ~RAlloc();

    // Disable copy and assignment
    RAlloc(const RAlloc &) = delete;
    RAlloc &operator=(const RAlloc &) = delete;

    // Initialize the allocator with maximum number of slab and buddy allocators
    bool init(size_t max_slabs = 256, size_t max_buddies = 64);

    // Allocate memory
    void *allocate(size_t size);

    // Free memory
    void deallocate(void *ptr);

    // Statistics
    size_t get_total_free_memory() const;
    size_t get_slab_count() const;
    size_t get_buddy_count() const;

private:
    // Forward declarations
    class SlabManager;

    // Helper functions
    enum AllocType
    {
        ALLOC_UNKNOWN = 0,
        ALLOC_SLAB = 1,
        ALLOC_BUDDY = 2,
        ALLOC_MALLOC = 3
    };

    AllocType identify_allocation(void *ptr) const;

private:
    SlabManager *slab_manager_;   // Single SlabManager (manages multiple SlabAllocators internally)
    void *buddy_manager_;         // Single BuddyManager (manages multiple BuddyAllocators internally)
    bool initialized_;
};

// Global interface
RAlloc *ralloc_get_instance();
void *ralloc_malloc(size_t size);
void ralloc_free(void *ptr);

#endif // RALLOC_H
