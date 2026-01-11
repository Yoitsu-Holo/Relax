#include "ralloc.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <functional>

using namespace std::chrono;

struct BenchmarkResult
{
    std::string name;
    size_t iterations;
    long long duration_us;
    double ops_per_sec;

    void print() const
    {
        std::cout << "  " << std::left << std::setw(40) << name
                  << ": " << std::right << std::setw(10) << duration_us << " μs"
                  << ", " << std::setw(10) << std::fixed << std::setprecision(2)
                  << (ops_per_sec / 1000000.0) << " M ops/sec" << std::endl;
    }
};

BenchmarkResult run_benchmark(const std::string &name, size_t iterations,
                               std::function<void()> func)
{
    // Warmup
    for (size_t i = 0; i < iterations / 10; ++i)
    {
        func();
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i)
    {
        func();
    }
    auto end = high_resolution_clock::now();

    long long duration_us = duration_cast<microseconds>(end - start).count();
    double ops_per_sec = (double)iterations / (duration_us / 1000000.0);

    return {name, iterations, duration_us, ops_per_sec};
}

void benchmark_sequential_alloc_free()
{
    std::cout << "\n=== Sequential Allocation/Deallocation ===" << std::endl;

    RAlloc allocator;
    allocator.init();

    const size_t ITERATIONS = 1000000;

    // Small sizes (slab)
    std::vector<size_t> small_sizes = {16, 64, 256, 1024, 4096};

    for (size_t size : small_sizes)
    {
        auto result = run_benchmark(
            "Alloc+Free " + std::to_string(size) + "B",
            ITERATIONS,
            [&allocator, size]()
            {
                void *ptr = allocator.allocate(size);
                allocator.deallocate(ptr);
            });
        result.print();
    }
}

void benchmark_batch_operations()
{
    std::cout << "\n=== Batch Operations ===" << std::endl;

    RAlloc allocator;
    allocator.init();

    const size_t BATCH_SIZE = 10000;
    const size_t ITERATIONS = 100;

    std::vector<void *> ptrs(BATCH_SIZE);

    // Batch allocate then batch free
    auto result1 = run_benchmark(
        "Batch alloc (64B x " + std::to_string(BATCH_SIZE) + ")",
        ITERATIONS,
        [&allocator, &ptrs]()
        {
            for (size_t i = 0; i < BATCH_SIZE; ++i)
            {
                ptrs[i] = allocator.allocate(64);
            }
            for (size_t i = 0; i < BATCH_SIZE; ++i)
            {
                allocator.deallocate(ptrs[i]);
            }
        });
    result1.print();
}

void benchmark_mixed_sizes()
{
    std::cout << "\n=== Mixed Size Allocations ===" << std::endl;

    RAlloc allocator;
    allocator.init();

    const size_t ITERATIONS = 500000;
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(16, 4096);

    std::vector<size_t> sizes(ITERATIONS);
    for (size_t i = 0; i < ITERATIONS; ++i)
    {
        sizes[i] = dist(rng);
    }

    size_t idx = 0;
    auto result = run_benchmark(
        "Random size [16, 4096]B",
        ITERATIONS,
        [&allocator, &sizes, &idx]()
        {
            void *ptr = allocator.allocate(sizes[idx % sizes.size()]);
            allocator.deallocate(ptr);
            idx++;
        });
    result.print();
}

void benchmark_allocation_patterns()
{
    std::cout << "\n=== Real-world Allocation Patterns ===" << std::endl;

    RAlloc allocator;
    allocator.init();

    const size_t ITERATIONS = 100000;

    // Pattern 1: Small object allocation (typical for cache entries)
    std::vector<void *> cache_entries(100);
    auto result1 = run_benchmark(
        "Cache pattern (100 x 256B, cyclic)",
        ITERATIONS,
        [&allocator, &cache_entries]()
        {
            static size_t idx = 0;
            if (cache_entries[idx])
            {
                allocator.deallocate(cache_entries[idx]);
            }
            cache_entries[idx] = allocator.allocate(256);
            idx = (idx + 1) % 100;
        });
    result1.print();

    // Cleanup
    for (void *ptr : cache_entries)
    {
        if (ptr)
            allocator.deallocate(ptr);
    }

    // Pattern 2: Variable-size allocations (80% small, 20% medium)
    std::mt19937 rng(42);
    std::uniform_real_distribution<> prob(0.0, 1.0);

    auto result2 = run_benchmark(
        "80% small (≤1KB), 20% medium (≤16KB)",
        ITERATIONS,
        [&allocator, &rng, &prob]()
        {
            size_t size = (prob(rng) < 0.8) ? (64 + rand() % 960) : (1024 + rand() % 15360);
            void *ptr = allocator.allocate(size);
            allocator.deallocate(ptr);
        });
    result2.print();
}

void benchmark_lookup_table_performance()
{
    std::cout << "\n=== Lookup Table Performance ===" << std::endl;

    RAlloc allocator;
    allocator.init();

    const size_t ITERATIONS = 10000000;

    // Sequential access pattern (best case for cache)
    auto result1 = run_benchmark(
        "Sequential sizes 1-4096",
        ITERATIONS,
        [&allocator]()
        {
            static size_t size = 1;
            void *ptr = allocator.allocate(size);
            allocator.deallocate(ptr);
            size = (size % 4096) + 1;
        });
    result1.print();

    // Random access pattern (worst case for cache)
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(1, 4096);
    std::vector<size_t> random_sizes(ITERATIONS);
    for (size_t i = 0; i < ITERATIONS; ++i)
    {
        random_sizes[i] = dist(rng);
    }

    size_t idx = 0;
    auto result2 = run_benchmark(
        "Random sizes 1-4096",
        ITERATIONS,
        [&allocator, &random_sizes, &idx]()
        {
            void *ptr = allocator.allocate(random_sizes[idx % random_sizes.size()]);
            allocator.deallocate(ptr);
            idx++;
        });
    result2.print();
}

void benchmark_memory_overhead()
{
    std::cout << "\n=== Memory Overhead Analysis ===" << std::endl;

    RAlloc allocator;
    allocator.init();

    std::cout << "  Initial slab count: " << allocator.get_slab_count() << std::endl;
    std::cout << "  Slabs allocated: " << allocator.get_slab_count() << " x 64 KiB = "
              << (allocator.get_slab_count() * 64) << " KiB" << std::endl;

    // Allocate some memory
    std::vector<void *> ptrs;
    for (size_t size = 16; size <= 4096; size *= 2)
    {
        for (int i = 0; i < 10; ++i)
        {
            ptrs.push_back(allocator.allocate(size));
        }
    }

    std::cout << "  Total free memory: " << allocator.get_total_free_memory() / 1024 << " KiB" << std::endl;
    std::cout << "  Buddy allocators created: " << allocator.get_buddy_count() << std::endl;

    // Cleanup
    for (void *ptr : ptrs)
    {
        allocator.deallocate(ptr);
    }
}

int main()
{
    std::cout << "=== RAlloc Performance Benchmark ===" << std::endl;
    std::cout << "Architecture: Fixed slabs + Compact O(1) lookup table" << std::endl;

    benchmark_sequential_alloc_free();
    benchmark_batch_operations();
    benchmark_mixed_sizes();
    benchmark_allocation_patterns();
    benchmark_lookup_table_performance();
    benchmark_memory_overhead();

    std::cout << "\n=== Benchmark completed ===" << std::endl;

    return 0;
}
