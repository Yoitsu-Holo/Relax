#include "../../cache-Kernel/slab/slab.h"
#include "../../cache-Kernel/slab/slab_allocator.h"
#include <iostream>
#include <vector>

int main()
{
    constexpr int BATCH_SIZE = 100000;   // 每批次 1000 个对象
    constexpr int TOTAL_OPS = 500000000; // 1亿次操作（足够采集数据）
    constexpr int NUM_BATCHES = TOTAL_OPS / BATCH_SIZE;

    void *batch_ptrs[BATCH_SIZE];

    std::cout << "=== Slab Manager Profiling Test ===" << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << "Batch size: " << BATCH_SIZE << std::endl;
    std::cout << "Number of batches: " << NUM_BATCHES << std::endl;
    std::cout << std::endl;

    // 初始化 Slab Manager
    Slab slab_mgr;
    if (slab_mgr.init(32) != 0)
    {
        std::cerr << "Failed to initialize Slab manager" << std::endl;
        return 1;
    }

    std::cout << "Starting allocation/deallocation loop..." << std::endl;

    // 执行批次操作 - 这是主要的性能热点
    for (int batch = 0; batch < NUM_BATCHES; batch++)
    {
        // 分配对象
        for (int i = 0; i < BATCH_SIZE; i++)
            batch_ptrs[i] = slab_mgr.allocate();

        // 释放对象
        for (int i = 0; i < BATCH_SIZE; i++)
            slab_mgr.deallocate(batch_ptrs[i]);
    }

    std::cout << "Completed!" << std::endl;
    std::cout << "Total allocators created: " << slab_mgr.get_initialized_count() << std::endl;

    return 0;
}
