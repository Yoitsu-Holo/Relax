#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <iomanip>

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
    // 测试配置
    constexpr int TOTAL_OPS = 10000000; // 总共 1000万 次操作

    std::cout << "=== Buddy System Performance Benchmark ===" << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << std::endl;

    // 测试不同大小的分配
    size_t test_sizes[] = {4096, 8192, 16384, 32768, 65536, 131072, 262144};
    const char *size_names[] = {"4KiB", "8KiB", "16KiB", "32KiB", "64KiB", "128KiB", "256KiB"};
    const size_t batch_size[] = {2000, 2000, 1000, 500, 250, 100, 50};
    for (size_t test_idx = 0; test_idx < sizeof(test_sizes) / sizeof(test_sizes[0]); ++test_idx)
    {
        size_t block_size = test_sizes[test_idx];
        int order = size_to_order(block_size);

        // 计算该块大小下的最大可分配数量
        size_t max_blocks = (16 * MiB) / block_size;
        int BATCH_SIZE = batch_size[test_idx];
        int NUM_BATCHES = TOTAL_OPS / BATCH_SIZE;

        // 动态分配指针数组
        void **batch_ptrs = new void *[BATCH_SIZE];

        std::cout << "\n========================================" << std::endl;
        std::cout << "Testing with block size: " << size_names[test_idx] << " (" << block_size << " bytes, order " << order << ")" << std::endl;
        std::cout << "Max allocatable blocks: " << max_blocks << std::endl;
        std::cout << "Batch size: " << BATCH_SIZE << " (" << (BATCH_SIZE * 100.0 / max_blocks) << "% of max)" << std::endl;
        std::cout << "Number of batches: " << NUM_BATCHES << std::endl;
        std::cout << "========================================" << std::endl;

        // ========== Buddy System测试 ==========
        std::cout << "\n=== Buddy Allocator ===" << std::endl;

        // BuddyAllocator 包含 16MiB 内置数据，需要堆分配
        // 使用 16MiB 对齐确保 data 成员也对齐
        BuddyAllocator *buddy = BuddyAllocator::get_new();
        if (!buddy)
        {
            std::cerr << "Failed to allocate memory for Buddy Allocator" << std::endl;
            return 1;
        }

        std::cout << "Initialized Buddy Allocator (16 MiB)" << std::endl;

        auto buddy_start = std::chrono::high_resolution_clock::now();

        // 执行批次操作
        for (int batch = 0; batch < NUM_BATCHES; batch++)
        {
            // 分配对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = buddy->allocate(order);
                if (!batch_ptrs[i])
                {
                    // 内存耗尽，跳过剩余分配
                    for (int j = 0; j < i; j++)
                    {
                        buddy->deallocate(batch_ptrs[j]);
                    }
                    goto buddy_end;
                }
            }

            // 释放对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                buddy->deallocate(batch_ptrs[i]);
            }
        }

    buddy_end:
        auto buddy_end = std::chrono::high_resolution_clock::now();
        auto buddy_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(buddy_end - buddy_start).count();
        auto buddy_time_us = buddy_time_ns / 1000;

        std::cout << "Buddy Allocator completed!" << std::endl;

        // 清理
        buddy->~BuddyAllocator();
        std::free(buddy);

        // ========== 标准库malloc/free测试 ==========
        std::cout << "\n=== Standard Library (malloc/free) ===" << std::endl;

        auto std_start = std::chrono::high_resolution_clock::now();

        // 执行批次操作
        for (int batch = 0; batch < NUM_BATCHES; batch++)
        {
            // 分配对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = malloc(block_size);
            }

            // 释放对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                free(batch_ptrs[i]);
            }
        }

        auto std_end = std::chrono::high_resolution_clock::now();
        auto std_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std_end - std_start).count();
        auto std_time_us = std_time_ns / 1000;

        // ========== 标准库new/delete测试 ==========
        std::cout << "\n=== Standard Library (new/delete) ===" << std::endl;

        auto new_start = std::chrono::high_resolution_clock::now();

        // 执行批次操作
        for (int batch = 0; batch < NUM_BATCHES; batch++)
        {
            // 分配对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = ::operator new(block_size);
            }

            // 释放对象
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                ::operator delete(batch_ptrs[i]);
            }
        }

        auto new_end = std::chrono::high_resolution_clock::now();
        auto new_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(new_end - new_start).count();
        auto new_time_us = new_time_ns / 1000;

        // ========== 结果汇总 ==========
        std::cout << "\n=== Performance Summary ===" << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "\nBuddy Allocator:" << std::endl;
        std::cout << "  Total time: " << buddy_time_us << " μs (" << buddy_time_ns << " ns)" << std::endl;
        std::cout << "  Average per operation: " << static_cast<double>(buddy_time_ns) / (TOTAL_OPS * 2) << " ns" << std::endl;
        std::cout << "  Average per alloc: " << static_cast<double>(buddy_time_ns) / TOTAL_OPS << " ns" << std::endl;
        std::cout << "  Average per dealloc: " << static_cast<double>(buddy_time_ns) / TOTAL_OPS << " ns" << std::endl;
        std::cout << "  Operations per second: " << (TOTAL_OPS * 2 * 1000000000.0) / buddy_time_ns << std::endl;

        std::cout << "\nStandard malloc/free:" << std::endl;
        std::cout << "  Total time: " << std_time_us << " μs (" << std_time_ns << " ns)" << std::endl;
        std::cout << "  Average per operation: " << static_cast<double>(std_time_ns) / (TOTAL_OPS * 2) << " ns" << std::endl;
        std::cout << "  Average per alloc: " << static_cast<double>(std_time_ns) / TOTAL_OPS << " ns" << std::endl;
        std::cout << "  Average per dealloc: " << static_cast<double>(std_time_ns) / TOTAL_OPS << " ns" << std::endl;
        std::cout << "  Operations per second: " << (TOTAL_OPS * 2 * 1000000000.0) / std_time_ns << std::endl;

        std::cout << "\nStandard new/delete:" << std::endl;
        std::cout << "  Total time: " << new_time_us << " μs (" << new_time_ns << " ns)" << std::endl;
        std::cout << "  Average per operation: " << static_cast<double>(new_time_ns) / (TOTAL_OPS * 2) << " ns" << std::endl;
        std::cout << "  Average per alloc: " << static_cast<double>(new_time_ns) / TOTAL_OPS << " ns" << std::endl;
        std::cout << "  Average per dealloc: " << static_cast<double>(new_time_ns) / TOTAL_OPS << " ns" << std::endl;
        std::cout << "  Operations per second: " << (TOTAL_OPS * 2 * 1000000000.0) / new_time_ns << std::endl;

        std::cout << "\n=== Performance Comparison ===" << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        double speedup_vs_malloc = static_cast<double>(std_time_ns) / buddy_time_ns;
        double speedup_vs_new = static_cast<double>(new_time_ns) / buddy_time_ns;

        std::cout << "Buddy Allocator vs malloc/free: " << speedup_vs_malloc << "x ";
        if (speedup_vs_malloc > 1.0)
        {
            std::cout << "faster (" << (speedup_vs_malloc - 1.0) * 100 << "% improvement)" << std::endl;
        }
        else
        {
            std::cout << "slower (" << (1.0 - speedup_vs_malloc) * 100 << "% slower)" << std::endl;
        }

        std::cout << "Buddy Allocator vs new/delete: " << speedup_vs_new << "x ";
        if (speedup_vs_new > 1.0)
        {
            std::cout << "faster (" << (speedup_vs_new - 1.0) * 100 << "% improvement)" << std::endl;
        }
        else
        {
            std::cout << "slower (" << (1.0 - speedup_vs_new) * 100 << "% slower)" << std::endl;
        }

        // 释放动态分配的指针数组
        delete[] batch_ptrs;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "All benchmarks completed!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
