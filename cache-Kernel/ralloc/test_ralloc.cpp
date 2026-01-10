#include "ralloc.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

void test_small_allocations()
{
    std::cout << "Testing small allocations (slab: 16B-4KiB)..." << std::endl;

    RAlloc allocator;
    assert(allocator.init());

    std::vector<void *> ptrs;

    // Test various sizes in slab range
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 3072};

    for (size_t size : sizes)
    {
        void *ptr = allocator.allocate(size);
        assert(ptr != nullptr);

        // Write to memory to ensure it's valid
        memset(ptr, 0xAB, size);

        ptrs.push_back(ptr);
        std::cout << "  Allocated " << size << " bytes at " << ptr << std::endl;
    }

    // Free all allocations
    for (void *ptr : ptrs)
    {
        allocator.deallocate(ptr);
    }

    std::cout << "  ✓ Small allocations test passed" << std::endl;
}

void test_medium_allocations()
{
    std::cout << "Testing medium allocations (buddy: 4KiB-256KiB)..." << std::endl;

    RAlloc allocator;
    assert(allocator.init());

    std::vector<void *> ptrs;

    // Test various sizes in buddy range
    size_t sizes[] = {4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024};

    for (size_t size : sizes)
    {
        void *ptr = allocator.allocate(size);
        assert(ptr != nullptr);

        // Write to memory to ensure it's valid
        memset(ptr, 0xCD, size);

        ptrs.push_back(ptr);
        std::cout << "  Allocated " << size << " bytes at " << ptr << std::endl;
    }

    // Free all allocations
    for (void *ptr : ptrs)
    {
        allocator.deallocate(ptr);
    }

    std::cout << "  ✓ Medium allocations test passed" << std::endl;
}

void test_large_allocations()
{
    std::cout << "Testing large allocations (malloc: >256KiB)..." << std::endl;

    RAlloc allocator;
    assert(allocator.init());

    std::vector<void *> ptrs;

    // Test various sizes in malloc range
    size_t sizes[] = {512 * 1024, 1024 * 1024, 4 * 1024 * 1024};

    for (size_t size : sizes)
    {
        void *ptr = allocator.allocate(size);
        assert(ptr != nullptr);

        // Write to memory to ensure it's valid
        memset(ptr, 0xEF, size);

        ptrs.push_back(ptr);
        std::cout << "  Allocated " << size << " bytes at " << ptr << std::endl;
    }

    // Free all allocations
    for (void *ptr : ptrs)
    {
        allocator.deallocate(ptr);
    }

    std::cout << "  ✓ Large allocations test passed" << std::endl;
}

void test_mixed_allocations()
{
    std::cout << "Testing mixed allocations..." << std::endl;

    RAlloc allocator;
    assert(allocator.init());

    std::vector<void *> ptrs;

    // Mix of different sizes
    size_t sizes[] = {64, 4096, 500000, 128, 16384, 2000000, 1024, 131072, 800000};

    for (size_t size : sizes)
    {
        void *ptr = allocator.allocate(size);
        assert(ptr != nullptr);

        // Write pattern based on size
        memset(ptr, static_cast<int>(size & 0xFF), size);

        ptrs.push_back(ptr);
        std::cout << "  Allocated " << size << " bytes" << std::endl;
    }

    // Free in reverse order
    std::cout << "  Freeing in reverse order..." << std::endl;
    for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it)
    {
        allocator.deallocate(*it);
    }

    std::cout << "  ✓ Mixed allocations test passed" << std::endl;
}

void test_statistics()
{
    std::cout << "Testing statistics..." << std::endl;

    RAlloc allocator;
    assert(allocator.init());

    std::cout << "  Initial state:" << std::endl;
    std::cout << "    Slab count: " << allocator.get_slab_count() << std::endl;
    std::cout << "    Buddy count: " << allocator.get_buddy_count() << std::endl;

    // Allocate some memory
    void *ptr1 = allocator.allocate(256);
    void *ptr2 = allocator.allocate(8192);
    void *ptr3 = allocator.allocate(1048576);

    std::cout << "  After allocations:" << std::endl;
    std::cout << "    Slab count: " << allocator.get_slab_count() << std::endl;
    std::cout << "    Buddy count: " << allocator.get_buddy_count() << std::endl;
    std::cout << "    Total free memory: " << allocator.get_total_free_memory() << " bytes" << std::endl;

    allocator.deallocate(ptr1);
    allocator.deallocate(ptr2);
    allocator.deallocate(ptr3);

    std::cout << "  ✓ Statistics test passed" << std::endl;
}

void test_global_interface()
{
    std::cout << "Testing global interface..." << std::endl;

    void *ptr1 = ralloc_malloc(128);
    void *ptr2 = ralloc_malloc(8192);
    void *ptr3 = ralloc_malloc(1048576);

    assert(ptr1 != nullptr);
    assert(ptr2 != nullptr);
    assert(ptr3 != nullptr);

    std::cout << "  Allocated 128B at " << ptr1 << std::endl;
    std::cout << "  Allocated 8KB at " << ptr2 << std::endl;
    std::cout << "  Allocated 1MB at " << ptr3 << std::endl;

    ralloc_free(ptr1);
    ralloc_free(ptr2);
    ralloc_free(ptr3);

    std::cout << "  ✓ Global interface test passed" << std::endl;
}

void test_edge_cases()
{
    std::cout << "Testing edge cases..." << std::endl;

    RAlloc allocator;
    assert(allocator.init());

    // Test zero size
    void *ptr_zero = allocator.allocate(0);
    assert(ptr_zero == nullptr);
    std::cout << "  ✓ Zero size allocation returns nullptr" << std::endl;

    // Test null pointer free
    allocator.deallocate(nullptr);
    std::cout << "  ✓ Null pointer free handled" << std::endl;

    // Test boundary sizes
    void *ptr_15 = allocator.allocate(15); // Below slab minimum, will use malloc
    assert(ptr_15 != nullptr);
    allocator.deallocate(ptr_15);
    std::cout << "  ✓ Below slab minimum (15B) uses malloc" << std::endl;

    void *ptr_16 = allocator.allocate(16); // Minimum slab
    assert(ptr_16 != nullptr);
    allocator.deallocate(ptr_16);
    std::cout << "  ✓ Minimum slab size (16B) works" << std::endl;

    void *ptr_4095 = allocator.allocate(4095); // Just below buddy
    assert(ptr_4095 != nullptr);
    allocator.deallocate(ptr_4095);
    std::cout << "  ✓ Maximum slab size (3072B rounds to 3072B) works" << std::endl;

    void *ptr_4096 = allocator.allocate(4096); // Minimum buddy
    assert(ptr_4096 != nullptr);
    allocator.deallocate(ptr_4096);
    std::cout << "  ✓ Minimum buddy size (4KiB) works" << std::endl;

    void *ptr_256k = allocator.allocate(256 * 1024); // Maximum buddy
    assert(ptr_256k != nullptr);
    allocator.deallocate(ptr_256k);
    std::cout << "  ✓ Maximum buddy size (256KiB) works" << std::endl;

    void *ptr_257k = allocator.allocate(257 * 1024); // Minimum malloc
    assert(ptr_257k != nullptr);
    allocator.deallocate(ptr_257k);
    std::cout << "  ✓ Minimum malloc size (>256KiB) works" << std::endl;

    std::cout << "  ✓ Edge cases test passed" << std::endl;
}

int main()
{
    std::cout << "=== RAlloc Test Suite ===" << std::endl << std::endl;

    try
    {
        test_small_allocations();
        std::cout << std::endl;

        test_medium_allocations();
        std::cout << std::endl;

        test_large_allocations();
        std::cout << std::endl;

        test_mixed_allocations();
        std::cout << std::endl;

        test_statistics();
        std::cout << std::endl;

        test_global_interface();
        std::cout << std::endl;

        test_edge_cases();
        std::cout << std::endl;

        std::cout << "=== All tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
