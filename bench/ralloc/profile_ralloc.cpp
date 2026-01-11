#include "../../cache-Kernel/ralloc/ralloc.h"
#include <iostream>
#include <chrono>
#include <cstdlib>

// Simplified profiling version for perf/valgrind analysis
int main()
{
    constexpr size_t TOTAL_OPS = 50000000; // 50M operations for profiling
    constexpr size_t BATCH_SIZE = 1000;
    const size_t NUM_BATCHES = TOTAL_OPS / BATCH_SIZE;

    void *batch_ptrs[BATCH_SIZE];

    // Create allocator instance
    RAlloc allocator;
    if (!allocator.init())
    {
        std::cerr << "Failed to initialize RAlloc" << std::endl;
        return 1;
    }

    std::cout << "Starting RAlloc profiling run..." << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS * 2 << std::endl;
    std::cout << "Batch size: " << BATCH_SIZE << std::endl;
    std::cout << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // Test different size classes
    size_t sizes[] = {64, 128, 512, 2048, 8192, 32768};
    // size_t sizes[] = {8192};
    const size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t size_idx = 0; size_idx < num_sizes; size_idx++)
    {
        size_t size = sizes[size_idx];
        std::cout << "Testing size: " << size << " bytes" << std::endl;

        for (size_t batch = 0; batch < NUM_BATCHES / num_sizes; batch++)
        {
            // Allocate
            for (size_t i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = allocator.allocate(size);
            }

            // Deallocate
            for (size_t i = 0; i < BATCH_SIZE; i++)
            {
                allocator.deallocate(batch_ptrs[i]);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << std::endl;
    std::cout << "Profiling run completed!" << std::endl;
    std::cout << "Total time: " << time_ms << " ms" << std::endl;
    std::cout << "Slab allocators: " << allocator.get_slab_count() << std::endl;
    std::cout << "Buddy allocators: " << allocator.get_buddy_count() << std::endl;

    return 0;
}
