#include "ralloc.h"
#include <iostream>
#include <cstring>

int main()
{
    std::cout << "RAlloc Example - Unified Memory Allocator\n" << std::endl;

    // Create allocator instance
    RAlloc allocator;
    if (!allocator.init())
    {
        std::cerr << "Failed to initialize RAlloc" << std::endl;
        return 1;
    }

    // Example 1: Small allocation (uses slab)
    std::cout << "1. Allocating 128 bytes (slab)..." << std::endl;
    void *small_ptr = allocator.allocate(128);
    if (small_ptr)
    {
        strcpy(static_cast<char *>(small_ptr), "Hello from slab allocator!");
        std::cout << "   Data: " << static_cast<char *>(small_ptr) << std::endl;
        allocator.deallocate(small_ptr);
    }

    // Example 2: Medium allocation (uses buddy system)
    std::cout << "\n2. Allocating 16 KB (buddy)..." << std::endl;
    void *medium_ptr = allocator.allocate(16 * 1024);
    if (medium_ptr)
    {
        strcpy(static_cast<char *>(medium_ptr), "Hello from buddy system!");
        std::cout << "   Data: " << static_cast<char *>(medium_ptr) << std::endl;
        allocator.deallocate(medium_ptr);
    }

    // Example 3: Large allocation (uses malloc)
    std::cout << "\n3. Allocating 512 KB (malloc)..." << std::endl;
    void *large_ptr = allocator.allocate(512 * 1024);
    if (large_ptr)
    {
        strcpy(static_cast<char *>(large_ptr), "Hello from malloc!");
        std::cout << "   Data: " << static_cast<char *>(large_ptr) << std::endl;
        allocator.deallocate(large_ptr);
    }

    // Example 4: Multiple allocations
    std::cout << "\n4. Multiple allocations of different sizes..." << std::endl;
    void *ptrs[5];
    size_t sizes[] = {64, 4096, 100000, 1024, 65536};

    for (int i = 0; i < 5; i++)
    {
        ptrs[i] = allocator.allocate(sizes[i]);
        if (ptrs[i])
        {
            std::cout << "   Allocated " << sizes[i] << " bytes at " << ptrs[i] << std::endl;
        }
    }

    // Free all allocations (in any order)
    std::cout << "\n5. Freeing allocations..." << std::endl;
    for (int i = 0; i < 5; i++)
    {
        allocator.deallocate(ptrs[i]);
    }

    // Statistics
    std::cout << "\n6. Statistics:" << std::endl;
    std::cout << "   Slab allocators: " << allocator.get_slab_count() << std::endl;
    std::cout << "   Buddy allocators: " << allocator.get_buddy_count() << std::endl;
    std::cout << "   Total free memory: " << allocator.get_total_free_memory() << " bytes" << std::endl;

    std::cout << "\nDone!" << std::endl;
    return 0;
}
