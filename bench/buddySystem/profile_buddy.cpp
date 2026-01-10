#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <iostream>
#include <vector>

// 辅助函数：将size转换为order
inline int size_to_order(size_t size)
{
    if (size <= BUDDY_ALLOC_BLOCK_SIZE)
        return 0;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 2)
        return 1;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 4)
        return 2;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 8)
        return 3;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 16)
        return 4;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 32)
        return 5;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 64)
        return 6;
    return -1;
}

int main()
{
    constexpr int BATCH_SIZE = 1000;     // 每批次分配数量
    constexpr int TOTAL_OPS = 100000000; // 1亿次操作（足够采集火焰图数据）
    constexpr int NUM_BATCHES = TOTAL_OPS / BATCH_SIZE;

    void *batch_ptrs[BATCH_SIZE];

    std::cout << "=== Buddy Allocator Profiling Test ===" << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << "Batch size: " << BATCH_SIZE << std::endl;
    std::cout << "Number of batches: " << NUM_BATCHES << std::endl;
    std::cout << std::endl;

    // 测试不同大小
    size_t test_sizes[] = {8192};
    const char *size_names[] = {"8KiB"};

    for (size_t test_idx = 0; test_idx < sizeof(test_sizes) / sizeof(test_sizes[0]); ++test_idx)
    {
        size_t block_size = test_sizes[test_idx];
        int order = size_to_order(block_size);

        std::cout << "\n=== Testing " << size_names[test_idx] << " allocations (order " << order << ") ===" << std::endl;

        // 初始化 Buddy Allocator
        // BuddyAllocator 包含 16MiB 内置数据，需要堆分配并使用 16MiB 对齐
        BuddyAllocator *buddy = BuddyAllocator::get_new();
        if (!buddy)
        {
            std::cerr << "Failed to allocate memory for Buddy Allocator" << std::endl;
            return 1;
        }

        std::cout << "Starting allocation/deallocation loop..." << std::endl;

        // 执行批次操作 - 这是主要的性能热点，用于火焰图分析
        for (int batch = 0; batch < NUM_BATCHES; batch++)
        {
            // 分配对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = buddy->allocate(order);
                if (!batch_ptrs[i])
                {
                    // 内存耗尽，释放并重新开始
                    for (int j = 0; j < i; j++)
                    {
                        buddy->deallocate(batch_ptrs[j]);
                    }
                    i = -1; // 重新开始
                    continue;
                }
            }

            // 释放对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                buddy->deallocate(batch_ptrs[i]);
            }
        }

        std::cout << "Completed " << size_names[test_idx] << " test!" << std::endl;

        // 清理
        buddy->~BuddyAllocator();
        std::free(buddy);
    }

    std::cout << "\nAll profiling tests completed!" << std::endl;
    return 0;
}
