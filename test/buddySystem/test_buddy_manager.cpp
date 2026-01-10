#include "buddy_manager.h"
#include <iostream>
#include <cassert>
#include <iomanip>

void print_stats(const BuddyManager &manager)
{
    std::cout << "Total free memory: "
              << manager.get_total_free_memory() / 1024 << " KiB" << std::endl;
    std::cout << "Allocator count: "
              << manager.get_allocator_count() << std::endl;
}

int main()
{
    std::cout << "=== Buddy Manager Test ===" << std::endl
              << std::endl;

    // 初始化manager
    BuddyManager manager;
    assert(manager.init(64));
    std::cout << "✓ Manager initialized (max 64 allocators)" << std::endl;

    // 测试1：基本分配和释放
    std::cout << "\n--- Test 1: Basic Allocation ---" << std::endl;
    void *ptr1 = manager.allocate(4096);
    assert(ptr1 != nullptr);
    std::cout << "✓ Allocated 4KiB block at: " << ptr1 << std::endl;
    print_stats(manager);

    manager.deallocate(ptr1);
    std::cout << "✓ Deallocated 4KiB block" << std::endl;
    print_stats(manager);

    // 测试2：多个不同大小的分配
    std::cout << "\n--- Test 2: Multiple Allocations ---" << std::endl;
    void *ptr2 = manager.allocate(4096);
    void *ptr3 = manager.allocate(8192);
    void *ptr4 = manager.allocate(65536);
    void *ptr5 = manager.allocate(262144);

    assert(ptr2 && ptr3 && ptr4 && ptr5);
    std::cout << "✓ Allocated 4KiB, 8KiB, 64KiB, 256KiB" << std::endl;
    print_stats(manager);

    manager.deallocate(ptr2);
    manager.deallocate(ptr3);
    manager.deallocate(ptr4);
    manager.deallocate(ptr5);
    std::cout << "✓ Deallocated all blocks" << std::endl;
    print_stats(manager);

    // 测试3：大量小块分配（触发多个allocator创建）
    std::cout << "\n--- Test 3: Large Scale Allocation ---" << std::endl;
    const int num_allocs = 5000;
    void *ptrs[num_allocs];

    for (int i = 0; i < num_allocs; ++i)
    {
        ptrs[i] = manager.allocate(4096);
        if (!ptrs[i])
        {
            std::cout << "✓ Allocated " << i << " blocks before exhaustion" << std::endl;
            break;
        }
    }
    print_stats(manager);

    // 释放所有
    for (int i = 0; i < num_allocs; ++i)
    {
        if (ptrs[i])
        {
            manager.deallocate(ptrs[i]);
        }
    }
    std::cout << "✓ Deallocated all blocks" << std::endl;
    print_stats(manager);

    // 测试4：交错分配不同大小
    std::cout << "\n--- Test 4: Mixed Sizes ---" << std::endl;
    void *p1 = manager.allocate(16384);
    void *p2 = manager.allocate(4096);
    void *p3 = manager.allocate(131072);
    void *p4 = manager.allocate(8192);

    assert(p1 && p2 && p3 && p4);
    std::cout << "✓ Allocated 16KiB, 4KiB, 128KiB, 8KiB" << std::endl;
    print_stats(manager);

    manager.deallocate(p1);
    manager.deallocate(p3);
    std::cout << "✓ Deallocated 16KiB and 128KiB" << std::endl;
    print_stats(manager);

    manager.deallocate(p2);
    manager.deallocate(p4);
    std::cout << "✓ Deallocated remaining blocks" << std::endl;
    print_stats(manager);

    std::cout << "\n=== All tests passed! ===" << std::endl;

    return 0;
}
