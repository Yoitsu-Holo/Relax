#include "../../cache-Kernel/slab/slab_allocator.h"
#include <benchmark/benchmark.h>
#include <cstdlib>
#include <vector>
#include <algorithm>

// Slab allocator benchmark
static void BM_SlabAllocator(benchmark::State& state) {
    const int block_size = state.range(0);

    slab* test_slab = new slab;
    SlabAllocator allocator(test_slab);
    allocator.init(block_size);

    // 动态计算batch_size，确保不超过slab容量的80%
    // 留出20%的余量以避免边界问题
    const int max_blocks = allocator.get_total_blocks();
    const int batch_size = std::min(1000, static_cast<int>(max_blocks * 0.8));

    std::vector<void*> ptrs(batch_size);

    for (auto _ : state) {
        // 分配
        for (int i = 0; i < batch_size; ++i) {
            ptrs[i] = allocator.allocate();
            if (!ptrs[i]) {
                state.SkipWithError("Slab allocator OOM");
                return;
            }
        }
        // 释放
        for (int i = 0; i < batch_size; ++i) {
            allocator.deallocate(ptrs[i]);
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size * 2);
    state.counters["batch_size"] = batch_size;
    state.counters["max_blocks"] = max_blocks;
    delete test_slab;
}

// malloc/free benchmark
static void BM_Malloc(benchmark::State& state) {
    const int block_size = state.range(0);

    // 为了公平比较，使用与SlabAllocator相同的batch_size计算方式
    slab temp_slab;
    SlabAllocator temp_allocator(&temp_slab);
    temp_allocator.init(block_size);
    const int max_blocks = temp_allocator.get_total_blocks();
    const int batch_size = std::min(1000, static_cast<int>(max_blocks * 0.8));

    std::vector<void*> ptrs(batch_size);

    for (auto _ : state) {
        // 分配
        for (int i = 0; i < batch_size; ++i) {
            ptrs[i] = malloc(block_size);
            if (!ptrs[i]) {
                state.SkipWithError("malloc OOM");
                return;
            }
        }
        // 释放
        for (int i = 0; i < batch_size; ++i) {
            free(ptrs[i]);
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size * 2);
    state.counters["batch_size"] = batch_size;
}

// new/delete benchmark
static void BM_NewDelete(benchmark::State& state) {
    const int block_size = state.range(0);

    // 为了公平比较，使用与SlabAllocator相同的batch_size计算方式
    slab temp_slab;
    SlabAllocator temp_allocator(&temp_slab);
    temp_allocator.init(block_size);
    const int max_blocks = temp_allocator.get_total_blocks();
    const int batch_size = std::min(1000, static_cast<int>(max_blocks * 0.8));

    std::vector<uint8_t*> ptrs(batch_size);

    for (auto _ : state) {
        // 分配
        for (int i = 0; i < batch_size; ++i) {
            ptrs[i] = new(std::nothrow) uint8_t[block_size];
            if (!ptrs[i]) {
                state.SkipWithError("new OOM");
                return;
            }
        }
        // 释放
        for (int i = 0; i < batch_size; ++i) {
            delete[] ptrs[i];
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size * 2);
    state.counters["batch_size"] = batch_size;
}

// 单个分配/释放的延迟测试
static void BM_SlabAllocator_Single(benchmark::State& state) {
    const int block_size = state.range(0);

    slab* test_slab = new slab;
    SlabAllocator allocator(test_slab);
    allocator.init(block_size);

    for (auto _ : state) {
        void* ptr = allocator.allocate();
        benchmark::DoNotOptimize(ptr);
        allocator.deallocate(ptr);
    }

    state.SetItemsProcessed(state.iterations() * 2);
    delete test_slab;
}

static void BM_Malloc_Single(benchmark::State& state) {
    const int block_size = state.range(0);

    for (auto _ : state) {
        void* ptr = malloc(block_size);
        benchmark::DoNotOptimize(ptr);
        free(ptr);
    }

    state.SetItemsProcessed(state.iterations() * 2);
}

// 碎片化场景测试 - 模拟真实使用场景
static void BM_SlabAllocator_Fragmented(benchmark::State& state) {
    const int block_size = state.range(0);

    slab* test_slab = new slab;
    SlabAllocator allocator(test_slab);
    allocator.init(block_size);

    const int max_blocks = allocator.get_total_blocks();
    const int working_set = std::min(100, static_cast<int>(max_blocks * 0.5));

    // 预先分配一些块来创建碎片化
    std::vector<void*> long_lived(working_set);
    std::vector<void*> short_lived(working_set);

    for (int i = 0; i < working_set; ++i) {
        long_lived[i] = allocator.allocate();
    }

    for (auto _ : state) {
        // 分配短生命周期的块
        for (int i = 0; i < working_set; ++i) {
            short_lived[i] = allocator.allocate();
        }
        // 释放一半
        for (int i = 0; i < working_set; i += 2) {
            allocator.deallocate(short_lived[i]);
        }
        // 再分配
        for (int i = 0; i < working_set; i += 2) {
            short_lived[i] = allocator.allocate();
        }
        // 全部释放
        for (int i = 0; i < working_set; ++i) {
            allocator.deallocate(short_lived[i]);
        }
    }

    // 清理
    for (int i = 0; i < working_set; ++i) {
        allocator.deallocate(long_lived[i]);
    }

    state.SetItemsProcessed(state.iterations() * working_set * 3);
    state.counters["working_set"] = working_set;
    delete test_slab;
}

static void BM_Malloc_Fragmented(benchmark::State& state) {
    const int block_size = state.range(0);

    // 计算working_set以匹配slab测试
    slab temp_slab;
    SlabAllocator temp_allocator(&temp_slab);
    temp_allocator.init(block_size);
    const int max_blocks = temp_allocator.get_total_blocks();
    const int working_set = std::min(100, static_cast<int>(max_blocks * 0.5));

    std::vector<void*> long_lived(working_set);
    std::vector<void*> short_lived(working_set);

    for (int i = 0; i < working_set; ++i) {
        long_lived[i] = malloc(block_size);
    }

    for (auto _ : state) {
        for (int i = 0; i < working_set; ++i) {
            short_lived[i] = malloc(block_size);
        }
        for (int i = 0; i < working_set; i += 2) {
            free(short_lived[i]);
        }
        for (int i = 0; i < working_set; i += 2) {
            short_lived[i] = malloc(block_size);
        }
        for (int i = 0; i < working_set; ++i) {
            free(short_lived[i]);
        }
    }

    for (int i = 0; i < working_set; ++i) {
        free(long_lived[i]);
    }

    state.SetItemsProcessed(state.iterations() * working_set * 3);
    state.counters["working_set"] = working_set;
}

// 注册benchmark
// 批量操作测试
BENCHMARK(BM_SlabAllocator)->Arg(16)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->Arg(4096);
BENCHMARK(BM_Malloc)->Arg(16)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->Arg(4096);
BENCHMARK(BM_NewDelete)->Arg(16)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->Arg(4096);

// 单个操作延迟测试
BENCHMARK(BM_SlabAllocator_Single)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024);
BENCHMARK(BM_Malloc_Single)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024);

// 碎片化场景测试
BENCHMARK(BM_SlabAllocator_Fragmented)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512);
BENCHMARK(BM_Malloc_Fragmented)->Arg(32)->Arg(64)->Arg(128)->Arg(256)->Arg(512);

BENCHMARK_MAIN();