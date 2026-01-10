/*
 * RAlloc - Unified Memory Allocator
 *
 * Architecture:
 *   RAlloc (single instance)
 *   ├── SlabManager (single instance managing multiple SlabAllocators)
 *   │   └── SlabAllocator[] (64KiB each, one per size class as needed)
 *   ├── BuddyManager (single instance managing multiple BuddyAllocators)
 *   │   └── BuddyAllocator[] (16MiB each, created on demand)
 *   └── System malloc (for allocations > 256KiB)
 *
 * Allocation Strategy:
 *   [16B, 4KiB)    -> SlabManager
 *   [4KiB, 256KiB] -> BuddyManager
 *   >256KiB        -> malloc with tracking header
 */

#include "ralloc.h"
#include "../slab/slab_allocator.h"
#include "../buddySystem/buddy_manager.h"
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>

// Forward declaration for alignment check
#ifndef KiB
#define KiB 1024
#define MiB (1024 * KiB)
#endif

// Allocation type constants (matching RAlloc::AllocType)
constexpr uint8_t ALLOC_UNKNOWN = 0;
constexpr uint8_t ALLOC_SLAB = 1;
constexpr uint8_t ALLOC_BUDDY = 2;
constexpr uint8_t ALLOC_MALLOC = 3;

// Malloc header for large allocations
struct MallocHeader
{
    uint64_t magic;     // Magic number to identify malloc allocations
    size_t size;        // Allocation size
    uint8_t alloc_type; // ALLOC_MALLOC

    static constexpr uint64_t MAGIC_NUMBER = 0xDEADBEEFCAFEBABEULL;
};

// SlabManager manages multiple SlabAllocators for different sizes
class RAlloc::SlabManager
{
public:
    SlabManager() : initialized_(false), max_slabs_(0) {}

    ~SlabManager()
    {
        // Cleanup all slab lists
        for (auto &list : slab_lists_)
        {
            for (auto *slab : list)
            {
                free(slab); // SlabAllocator was allocated with aligned_alloc
            }
        }
    }

    bool init(size_t max_slabs)
    {
        if (initialized_)
            return false;

        max_slabs_ = max_slabs;

        // Initialize slab lists for each supported size
        slab_lists_.resize(SLAB_SIZE_COUNT);
        initialized_ = true;
        return true;
    }

    void *allocate(size_t size)
    {
        if (!initialized_ || size < 16 || size >= 4 * KiB)
            return nullptr;

        // Find appropriate slab size index
        int size_idx = get_size_index(size);
        if (size_idx < 0)
            return nullptr;

        size_t actual_size = SLAB_SIZES[size_idx];

        // Try to allocate from existing slabs
        for (auto *slab : slab_lists_[size_idx])
        {
            void *ptr = slab->allocate();
            if (ptr != nullptr)
                return ptr;
        }

        // Need to create a new slab
        if (get_total_slab_count() >= max_slabs_)
            return nullptr; // Reached limit

        // Create new slab allocator (64KiB aligned)
        void *mem = aligned_alloc(64 * KiB, sizeof(SlabAllocator));
        if (!mem)
            return nullptr;

        // Construct SlabAllocator in-place with manager index
        uint64_t manager_info = (static_cast<uint64_t>(ALLOC_SLAB) << 56) | size_idx;
        SlabAllocator *new_slab = new (mem) SlabAllocator(actual_size, manager_info);

        slab_lists_[size_idx].push_back(new_slab);

        // Try to allocate from the new slab
        return new_slab->allocate();
    }

    void deallocate(void *ptr)
    {
        if (!ptr)
            return;

        // Get metadata to find the size index
        uint64_t metadata = SlabAllocator::get_metadata(ptr);
        int size_idx = static_cast<int>(metadata & 0xFF);

        if (size_idx < 0 || size_idx >= SLAB_SIZE_COUNT)
            return;

        // Find the slab and deallocate
        // The pointer is 64KiB aligned, so we can get the slab directly
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t slab_addr = addr & ~(64 * KiB - 1);
        SlabAllocator *slab = reinterpret_cast<SlabAllocator *>(slab_addr);

        slab->deallocate(ptr);
    }

    size_t get_total_slab_count() const
    {
        size_t count = 0;
        for (const auto &list : slab_lists_)
        {
            count += list.size();
        }
        return count;
    }

    size_t get_total_free_memory() const
    {
        size_t total = 0;
        for (size_t i = 0; i < slab_lists_.size(); ++i)
        {
            for (const auto *slab : slab_lists_[i])
            {
                total += slab->get_free_blocks() * slab->get_block_size();
            }
        }
        return total;
    }

private:
    static constexpr size_t SLAB_SIZE_COUNT = 13;
    static constexpr size_t SLAB_SIZES[SLAB_SIZE_COUNT] = {
        16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072};

    int get_size_index(size_t size) const
    {
        // Find the smallest slab size that can fit the requested size
        for (int i = 0; i < SLAB_SIZE_COUNT; ++i)
        {
            if (size <= SLAB_SIZES[i])
                return i;
        }
        return -1; // Size too large for slab
    }

    bool initialized_;
    size_t max_slabs_;
    std::vector<std::vector<SlabAllocator *>> slab_lists_;
};

// RAlloc implementation

RAlloc::RAlloc()
    : slab_manager_(nullptr),
      buddy_manager_(nullptr),
      initialized_(false)
{
}

RAlloc::~RAlloc()
{
    if (slab_manager_)
    {
        delete slab_manager_;
        slab_manager_ = nullptr;
    }

    if (buddy_manager_)
    {
        delete static_cast<BuddyManager *>(buddy_manager_);
        buddy_manager_ = nullptr;
    }
}

bool RAlloc::init(size_t max_slabs, size_t max_buddies)
{
    if (initialized_)
        return false;

    // Initialize slab manager
    slab_manager_ = new SlabManager();
    if (!slab_manager_ || !slab_manager_->init(max_slabs))
    {
        delete slab_manager_;
        slab_manager_ = nullptr;
        return false;
    }

    // Initialize buddy manager
    BuddyManager *buddy = new BuddyManager();
    if (!buddy || !buddy->init(max_buddies))
    {
        delete buddy;
        delete slab_manager_;
        slab_manager_ = nullptr;
        return false;
    }
    buddy_manager_ = buddy;

    initialized_ = true;
    return true;
}

void *RAlloc::allocate(size_t size)
{
    if (!initialized_ || size == 0)
        return nullptr;

    // [16B, 4KiB) - try slab first
    if (size >= 16 && size < 4 * KiB)
    {
        void *ptr = slab_manager_->allocate(size);
        if (ptr != nullptr)
            return ptr;

        // Slab allocation failed (might be too large for slab sizes)
        // Fall through to try other allocators
    }

    // [4KiB, 256KiB] - use buddy
    if (size >= 4 * KiB && size <= 256 * KiB)
    {
        BuddyManager *buddy = static_cast<BuddyManager *>(buddy_manager_);
        return buddy->allocate(size);
    }

    // For sizes between largest slab and 4KiB, or >256KiB - use malloc
    // Allocate extra space for header
    size_t total_size = sizeof(MallocHeader) + size;
    void *raw_mem = malloc(total_size);
    if (!raw_mem)
        return nullptr;

    // Write header
    MallocHeader *header = static_cast<MallocHeader *>(raw_mem);
    header->magic = MallocHeader::MAGIC_NUMBER;
    header->size = size;
    header->alloc_type = ALLOC_MALLOC;

    // Return pointer after header
    return static_cast<uint8_t *>(raw_mem) + sizeof(MallocHeader);
}

void RAlloc::deallocate(void *ptr)
{
    if (!ptr || !initialized_)
        return;

    AllocType type = identify_allocation(ptr);

    switch (type)
    {
    case ALLOC_SLAB:
        slab_manager_->deallocate(ptr);
        break;

    case ALLOC_BUDDY:
    {
        BuddyManager *buddy = static_cast<BuddyManager *>(buddy_manager_);
        buddy->deallocate(ptr);
        break;
    }

    case ALLOC_MALLOC:
    {
        // Get header and free
        uint8_t *raw_mem = static_cast<uint8_t *>(ptr) - sizeof(MallocHeader);
        free(raw_mem);
        break;
    }

    default:
        // Unknown allocation type, do nothing to avoid corruption
        break;
    }
}

RAlloc::AllocType RAlloc::identify_allocation(void *ptr) const
{
    if (!ptr)
        return ALLOC_UNKNOWN;

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    // Strategy: Use alignment and metadata to identify allocation type
    // 1. Check for malloc: validate header with magic number (most reliable)
    // 2. Check for slab: validate metadata (64KiB aligned blocks)
    // 3. Assume buddy: 4KiB aligned allocations

    // First, try to identify malloc allocations by checking the header
    // Malloc allocations are typically 16-byte aligned but not necessarily 4KiB aligned
    if ((addr & (4 * KiB - 1)) != 0)
    {
        // Not 4KiB aligned, likely malloc (slab and buddy are at least 16B and 4KiB aligned)
        // However, small slab allocations might not be 4KiB aligned either
        // Let's check the malloc header
        if (addr >= sizeof(MallocHeader))
        {
            uint8_t *raw_mem = static_cast<uint8_t *>(ptr) - sizeof(MallocHeader);
            MallocHeader *header = reinterpret_cast<MallocHeader *>(raw_mem);

            if (header->magic == MallocHeader::MAGIC_NUMBER &&
                header->alloc_type == ALLOC_MALLOC)
            {
                return ALLOC_MALLOC;
            }
        }
    }

    // Check if it's a slab allocation by examining metadata
    // Slab allocations are in 64KiB-aligned blocks, but the returned pointers
    // are not necessarily at the block boundary
    uint64_t metadata = SlabAllocator::get_metadata(ptr);
    uint8_t alloc_type = static_cast<uint8_t>((metadata >> 56) & 0xFF);

    if (alloc_type == ALLOC_SLAB)
    {
        return ALLOC_SLAB;
    }

    // Check if it's 4KiB aligned, which suggests buddy allocation
    // Buddy allocations are always 4KiB aligned
    if ((addr & (4 * KiB - 1)) == 0)
    {
        // Very likely a buddy allocation
        return ALLOC_BUDDY;
    }

    // Final fallback: check malloc header even for aligned pointers
    if (addr >= sizeof(MallocHeader))
    {
        uint8_t *raw_mem = static_cast<uint8_t *>(ptr) - sizeof(MallocHeader);
        MallocHeader *header = reinterpret_cast<MallocHeader *>(raw_mem);

        if (header->magic == MallocHeader::MAGIC_NUMBER &&
            header->alloc_type == ALLOC_MALLOC)
        {
            return ALLOC_MALLOC;
        }
    }

    return ALLOC_UNKNOWN;
}

size_t RAlloc::get_total_free_memory() const
{
    if (!initialized_)
        return 0;

    size_t total = 0;

    // Slab free memory
    if (slab_manager_)
    {
        total += slab_manager_->get_total_free_memory();
    }

    // Buddy free memory
    if (buddy_manager_)
    {
        BuddyManager *buddy = static_cast<BuddyManager *>(buddy_manager_);
        total += buddy->get_total_free_memory();
    }

    return total;
}

size_t RAlloc::get_slab_count() const
{
    if (!initialized_ || !slab_manager_)
        return 0;

    return slab_manager_->get_total_slab_count();
}

size_t RAlloc::get_buddy_count() const
{
    if (!initialized_ || !buddy_manager_)
        return 0;

    BuddyManager *buddy = static_cast<BuddyManager *>(buddy_manager_);
    return buddy->get_allocator_count();
}

// Global instance
static RAlloc *g_ralloc_instance = nullptr;

RAlloc *ralloc_get_instance()
{
    if (!g_ralloc_instance)
    {
        g_ralloc_instance = new RAlloc();
        g_ralloc_instance->init();
    }
    return g_ralloc_instance;
}

void *ralloc_malloc(size_t size)
{
    return ralloc_get_instance()->allocate(size);
}

void ralloc_free(void *ptr)
{
    ralloc_get_instance()->deallocate(ptr);
}
