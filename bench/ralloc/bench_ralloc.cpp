#include "../../cache-Kernel/ralloc/ralloc.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <cstring>

// Performance test for different size ranges
struct BenchmarkResult
{
    std::string name;
    size_t total_time_us;
    size_t total_ops;
    double avg_ns_per_op;
    double ops_per_second;
};

BenchmarkResult run_benchmark(const std::string &name, size_t block_size, size_t total_ops, RAlloc *allocator)
{
    // Adjust batch size based on block size to avoid excessive memory usage
    size_t BATCH_SIZE = 10000;
    if (block_size > 256 * 1024)     // For large allocations (>256KiB)
        BATCH_SIZE = 100;            // Reduce to 100 to avoid OOM
    else if (block_size > 64 * 1024) // For medium-large allocations (>64KiB)
        BATCH_SIZE = 1000;           // Reduce to 1000

    const size_t NUM_BATCHES = total_ops / BATCH_SIZE;

    void *batch_ptrs[10000]; // Use max size, but only use up to BATCH_SIZE

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t batch = 0; batch < NUM_BATCHES; batch++)
    {
        // Allocate
        for (size_t i = 0; i < BATCH_SIZE; i++)
        {
            batch_ptrs[i] = allocator->allocate(block_size);
        }

        // Deallocate
        for (size_t i = 0; i < BATCH_SIZE; i++)
        {
            allocator->deallocate(batch_ptrs[i]);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    BenchmarkResult result;
    result.name = name;
    result.total_time_us = time_us;
    result.total_ops = total_ops * 2; // allocate + deallocate
    result.avg_ns_per_op = (time_us * 1000.0) / result.total_ops;
    result.ops_per_second = (result.total_ops * 1000000.0) / time_us;

    return result;
}

BenchmarkResult run_malloc_benchmark(size_t block_size, size_t total_ops)
{
    // Adjust batch size based on block size to avoid excessive memory usage
    size_t BATCH_SIZE = 10000;
    if (block_size > 256 * 1024)     // For large allocations (>256KiB)
        BATCH_SIZE = 100;            // Reduce to 100 to avoid OOM
    else if (block_size > 64 * 1024) // For medium-large allocations (>64KiB)
        BATCH_SIZE = 1000;           // Reduce to 1000

    const size_t NUM_BATCHES = total_ops / BATCH_SIZE;

    void *batch_ptrs[10000]; // Use max size, but only use up to BATCH_SIZE

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t batch = 0; batch < NUM_BATCHES; batch++)
    {
        // Allocate
        for (size_t i = 0; i < BATCH_SIZE; i++)
        {
            batch_ptrs[i] = malloc(block_size);
        }

        // Deallocate
        for (size_t i = 0; i < BATCH_SIZE; i++)
        {
            free(batch_ptrs[i]);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto time_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    BenchmarkResult result;
    result.name = "malloc/free";
    result.total_time_us = time_us;
    result.total_ops = total_ops * 2;
    result.avg_ns_per_op = (time_us * 1000.0) / result.total_ops;
    result.ops_per_second = (result.total_ops * 1000000.0) / time_us;

    return result;
}

void print_result(const BenchmarkResult &result)
{
    std::cout << result.name << ":" << std::endl;
    std::cout << "  Total time: " << result.total_time_us << " μs" << std::endl;
    std::cout << "  Average per operation: " << result.avg_ns_per_op << " ns" << std::endl;
    std::cout << "  Operations per second: " << result.ops_per_second << std::endl;
    std::cout << std::endl;
}

void print_comparison(const BenchmarkResult &ralloc_result, const BenchmarkResult &malloc_result)
{
    double speedup = (double)malloc_result.total_time_us / ralloc_result.total_time_us;

    std::cout << "=== Performance Comparison ===" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "RAlloc vs malloc/free: " << speedup << "x ";
    if (speedup > 1.0)
    {
        std::cout << "faster" << std::endl;
    }
    else
    {
        std::cout << "slower (1/" << (1.0 / speedup) << "x)" << std::endl;
    }
    std::cout << std::endl;
}

int main()
{
    std::cout << "=== RAlloc Performance Benchmark ===" << std::endl;
    std::cout << std::endl;

    // Create allocator instance
    RAlloc allocator;
    if (!allocator.init())
    {
        std::cerr << "Failed to initialize RAlloc" << std::endl;
        return 1;
    }

    // Test 1: Small allocations (Slab)
    {
        std::cout << "=== Test 1: Small Allocations (64B - Slab) ===" << std::endl;
        constexpr size_t TOTAL_OPS = 100000000; // 100M operations
        constexpr size_t BLOCK_SIZE = 64;

        auto ralloc_result = run_benchmark("RAlloc (Slab)", BLOCK_SIZE, TOTAL_OPS, &allocator);
        auto malloc_result = run_malloc_benchmark(BLOCK_SIZE, TOTAL_OPS);

        print_result(ralloc_result);
        print_result(malloc_result);
        print_comparison(ralloc_result, malloc_result);
    }

    // Test 2: Medium allocations (Buddy)
    {
        std::cout << "=== Test 2: Medium Allocations (8KiB - Buddy) ===" << std::endl;
        constexpr size_t TOTAL_OPS = 1000000; // 1M operations
        constexpr size_t BLOCK_SIZE = 8192;

        auto ralloc_result = run_benchmark("RAlloc (Buddy)", BLOCK_SIZE, TOTAL_OPS, &allocator);
        auto malloc_result = run_malloc_benchmark(BLOCK_SIZE, TOTAL_OPS);

        print_result(ralloc_result);
        print_result(malloc_result);
        print_comparison(ralloc_result, malloc_result);
    }

    // Test 3: Large allocations (malloc fallback)
    {
        std::cout << "=== Test 3: Large Allocations (512KiB - Malloc) ===" << std::endl;
        constexpr size_t TOTAL_OPS = 10000; // Reduced from 100K to 10K to avoid OOM
        constexpr size_t BLOCK_SIZE = 524288;

        auto ralloc_result = run_benchmark("RAlloc (Malloc)", BLOCK_SIZE, TOTAL_OPS, &allocator);
        auto malloc_result = run_malloc_benchmark(BLOCK_SIZE, TOTAL_OPS);

        print_result(ralloc_result);
        print_result(malloc_result);
        print_comparison(ralloc_result, malloc_result);
    }

    // Test 4: Mixed sizes
    {
        std::cout << "=== Test 4: Mixed Size Allocations ===" << std::endl;
        constexpr size_t TOTAL_OPS = 1000000;
        constexpr size_t BATCH_SIZE = 10000;
        const size_t NUM_BATCHES = TOTAL_OPS / BATCH_SIZE;

        size_t sizes[] = {32, 128, 512, 2048, 4096, 16384, 65536, 262144};
        const size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

        void *batch_ptrs[BATCH_SIZE];

        // RAlloc mixed test
        auto ralloc_start = std::chrono::high_resolution_clock::now();

        for (size_t batch = 0; batch < NUM_BATCHES; batch++)
        {
            for (size_t i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = allocator.allocate(sizes[i % num_sizes]);
            }

            for (size_t i = 0; i < BATCH_SIZE; i++)
            {
                allocator.deallocate(batch_ptrs[i]);
            }
        }

        auto ralloc_end = std::chrono::high_resolution_clock::now();
        auto ralloc_time = std::chrono::duration_cast<std::chrono::microseconds>(ralloc_end - ralloc_start).count();

        // malloc mixed test
        auto malloc_start = std::chrono::high_resolution_clock::now();

        for (size_t batch = 0; batch < NUM_BATCHES; batch++)
        {
            for (size_t i = 0; i < BATCH_SIZE; i++)
            {
                batch_ptrs[i] = malloc(sizes[i % num_sizes]);
            }

            for (size_t i = 0; i < BATCH_SIZE; i++)
            {
                free(batch_ptrs[i]);
            }
        }

        auto malloc_end = std::chrono::high_resolution_clock::now();
        auto malloc_time = std::chrono::duration_cast<std::chrono::microseconds>(malloc_end - malloc_start).count();

        BenchmarkResult ralloc_result;
        ralloc_result.name = "RAlloc (Mixed)";
        ralloc_result.total_time_us = ralloc_time;
        ralloc_result.total_ops = TOTAL_OPS * 2;
        ralloc_result.avg_ns_per_op = (ralloc_time * 1000.0) / ralloc_result.total_ops;
        ralloc_result.ops_per_second = (ralloc_result.total_ops * 1000000.0) / ralloc_time;

        BenchmarkResult malloc_result;
        malloc_result.name = "malloc/free (Mixed)";
        malloc_result.total_time_us = malloc_time;
        malloc_result.total_ops = TOTAL_OPS * 2;
        malloc_result.avg_ns_per_op = (malloc_time * 1000.0) / malloc_result.total_ops;
        malloc_result.ops_per_second = (malloc_result.total_ops * 1000000.0) / malloc_time;

        print_result(ralloc_result);
        print_result(malloc_result);
        print_comparison(ralloc_result, malloc_result);
    }

    // Print overall statistics
    std::cout << "=== Final Statistics ===" << std::endl;
    std::cout << "Total slab allocators created: " << allocator.get_slab_count() << std::endl;
    std::cout << "Total buddy allocators created: " << allocator.get_buddy_count() << std::endl;
    std::cout << "Total free memory: " << allocator.get_total_free_memory() << " bytes" << std::endl;

    return 0;
}
