#include "../../cache-Kernel/slab/slab.h"
#include "../../cache-Kernel/slab/slab_allocator.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cstdlib>

int main()
{
    constexpr int WORD_LEN = 120;                       // 分配字长
    constexpr int BATCH_SIZE = 100000;                  // 每批次 BATCH_SIZE 个对象
    constexpr int TOTAL_OPS = 100000000;                // 总共 1亿 次操作
    constexpr int NUM_BATCHES = TOTAL_OPS / BATCH_SIZE; // 批次数

    void *batch_ptrs[BATCH_SIZE]; // 用于保存每批次的指针

    std::cout << "=== Slab Manager Performance Benchmark ===" << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << "Batch size: " << BATCH_SIZE << std::endl;
    std::cout << "Number of batches: " << NUM_BATCHES << std::endl;
    std::cout << "Block size: " << WORD_LEN << " bytes" << std::endl;
    std::cout << std::endl;

    // ========== Slab Manager测试 ==========
    std::cout << "=== Testing Slab Manager (with auto-expansion) ===" << std::endl;

    Slab slab_mgr;
    if (slab_mgr.init(128) != 0)
    {
        std::cerr << "Failed to initialize Slab manager" << std::endl;
        return 1;
    }

    std::cout << "Initialized Slab manager with block size: " << WORD_LEN << " bytes " << std::endl;
    std::cout << "Auto-expansion enabled" << std::endl;
    std::cout << std::endl;

    auto slab_start = std::chrono::high_resolution_clock::now();

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++)
    {
        // 分配对象
        for (int i = 0; i < BATCH_SIZE; i++)
            batch_ptrs[i] = slab_mgr.allocate();

        // 释放对象
        for (int i = 0; i < BATCH_SIZE; i++)
            slab_mgr.deallocate(batch_ptrs[i]);
    }

    auto slab_end = std::chrono::high_resolution_clock::now();
    auto slab_time = std::chrono::duration_cast<std::chrono::microseconds>(slab_end - slab_start).count();

    std::cout << "Slab Manager completed!" << std::endl;
    std::cout << "Total allocators created: " << slab_mgr.get_initialized_count() << std::endl;
    std::cout << std::endl;

    // ========== 标准库malloc/free测试 ==========
    std::cout << "=== Testing Standard Library (malloc/free) ===" << std::endl;

    auto std_start = std::chrono::high_resolution_clock::now();

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++)
    {
        // 分配对象
        for (int i = 0; i < BATCH_SIZE; i++)
            batch_ptrs[i] = malloc(WORD_LEN);

        // 释放对象
        for (int i = 0; i < BATCH_SIZE; i++)
            free(batch_ptrs[i]);
    }

    auto std_end = std::chrono::high_resolution_clock::now();
    auto std_time = std::chrono::duration_cast<std::chrono::microseconds>(std_end - std_start).count();

    // ========== 标准库new/delete测试 ==========
    std::cout << "=== Testing Standard Library (new/delete) ===" << std::endl;

    auto new_start = std::chrono::high_resolution_clock::now();

    // 定义32字节的对象类型
    struct TestBlock
    {
        char data[WORD_LEN];
    };
    TestBlock *new_batch_ptrs[BATCH_SIZE];

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++)
    {
        // 分配对象
        for (int i = 0; i < BATCH_SIZE; i++)
            new_batch_ptrs[i] = new TestBlock;

        // 释放对象
        for (int i = 0; i < BATCH_SIZE; i++)
            delete new_batch_ptrs[i];
    }

    auto new_end = std::chrono::high_resolution_clock::now();
    auto new_time = std::chrono::duration_cast<std::chrono::microseconds>(new_end - new_start).count();

    // ========== 结果汇总 ==========
    std::cout << "\n=== Performance Summary ===" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Slab Manager:" << std::endl;
    std::cout << "  Total time: " << slab_time << " μs" << std::endl;
    std::cout << "  Average per operation: " << (slab_time * 1000.0) / (TOTAL_OPS * 2) << " ns" << std::endl;
    std::cout << "  Operations per second: " << (TOTAL_OPS * 2 * 1000000.0) / slab_time << std::endl;

    std::cout << "\nStandard malloc/free:" << std::endl;
    std::cout << "  Total time: " << std_time << " μs" << std::endl;
    std::cout << "  Average per operation: " << (std_time * 1000.0) / (TOTAL_OPS * 2) << " ns" << std::endl;
    std::cout << "  Operations per second: " << (TOTAL_OPS * 2 * 1000000.0) / std_time << std::endl;

    std::cout << "\nStandard new/delete:" << std::endl;
    std::cout << "  Total time: " << new_time << " μs" << std::endl;
    std::cout << "  Average per operation: " << (new_time * 1000.0) / (TOTAL_OPS * 2) << " ns" << std::endl;
    std::cout << "  Operations per second: " << (TOTAL_OPS * 2 * 1000000.0) / new_time << std::endl;

    std::cout << "\n=== Performance Comparison ===" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    double speedup_vs_malloc = (double)std_time / slab_time;
    double speedup_vs_new = (double)new_time / slab_time;

    std::cout << "Slab Manager vs malloc/free: " << speedup_vs_malloc << "x ";
    if (speedup_vs_malloc > 1.0)
    {
        std::cout << "faster" << std::endl;
    }
    else
    {
        std::cout << "slower" << std::endl;
    }

    std::cout << "Slab Manager vs new/delete: " << speedup_vs_new << "x ";
    if (speedup_vs_new > 1.0)
    {
        std::cout << "faster" << std::endl;
    }
    else
    {
        std::cout << "slower" << std::endl;
    }

    return 0;
}
