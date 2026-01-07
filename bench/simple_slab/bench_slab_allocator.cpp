
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <chrono>

#define KiB 1024
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)

// 每个slab分配器占用64KiB内存
// 前512字节为metadata（64个uint64_t = 512字节）
// 后面的内存用于实际分配
union slab {
    struct {
        uint64_t idx0;        // 第一层索引（位图）
        uint64_t idx1[63];    // 第二层索引（位图）
    } metadata;
    uint8_t data[64*KiB];
};

// Slab分配器类
class SlabAllocator {
private:
    static constexpr size_t BLOCK_SIZE = 16;  // 每个块16字节
    static constexpr size_t METADATA_SIZE = sizeof(uint64_t) * 64;  // 512字节
    static constexpr size_t USABLE_SIZE = 64*KiB - METADATA_SIZE;  // 可用空间
    static constexpr size_t TOTAL_BLOCKS = USABLE_SIZE / BLOCK_SIZE;  // 总块数 = 4032
    static constexpr size_t BLOCKS_PER_UINT64 = 64;  // 每个uint64_t管理64个块

    slab* slab_ptr;

public:
    SlabAllocator(slab* s) : slab_ptr(s) {
        // 初始化metadata，所有块都是空闲的（位设置为0）
        memset(&slab_ptr->metadata, 0, sizeof(slab_ptr->metadata));
    }

    // 分配一个块
    void* allocate() {
        // 检查第一层索引，找到有空闲块的组
        uint64_t idx0_val = slab_ptr->metadata.idx0;

        // 找到第一个不全满的组（idx0中为0的位）
        // 注意：idx0中为1表示对应的idx1全满
        uint64_t available_groups = ~idx0_val;
        if (__glibc_unlikely(available_groups == 0)) {
            // 所有组都满了
            return nullptr;
        }

        // 使用__builtin_ctzll找到第一个可用组
        int group_idx = __builtin_ctzll(available_groups);

        // 如果group_idx超出有效范围，返回nullptr
        if (__glibc_unlikely(group_idx >= 63)) {
            return nullptr;
        }

        // 在对应的第二层索引中找空闲块
        uint64_t& idx1_val = slab_ptr->metadata.idx1[group_idx];
        uint64_t free_blocks = ~idx1_val;

        if (__glibc_unlikely(free_blocks == 0)) {
            // 这个组实际上是满的，更新idx0
            slab_ptr->metadata.idx0 |= (1ULL << group_idx);
            return allocate(); // 递归重试
        }

        // 找到第一个空闲块
        int block_idx_in_group = __builtin_ctzll(free_blocks);

        // 标记该块为已分配
        idx1_val |= (1ULL << block_idx_in_group);

        // 如果这个组满了，更新第一层索引
        if (idx1_val == ~0ULL) {
            slab_ptr->metadata.idx0 |= (1ULL << group_idx);
        }

        // 计算实际的块索引
        size_t block_idx = group_idx * BLOCKS_PER_UINT64 + block_idx_in_group;

        // 计算返回的地址
        uint8_t* base = slab_ptr->data + METADATA_SIZE;
        return base + block_idx * BLOCK_SIZE;
    }

    // 释放一个块
    void deallocate(void* ptr) {
        if (!ptr) return;

        uint8_t* block = static_cast<uint8_t*>(ptr);
        uint8_t* base = slab_ptr->data + METADATA_SIZE;

        // 检查指针是否在有效范围内
        if (block < base || block >= base + USABLE_SIZE) {
            return; // 无效指针
        }

        // 计算块索引
        size_t offset = block - base;
        if (offset % BLOCK_SIZE != 0) {
            return; // 非对齐的指针
        }

        size_t block_idx = offset / BLOCK_SIZE;
        if (block_idx >= TOTAL_BLOCKS) {
            return; // 索引越界
        }

        // 计算组索引和组内索引
        size_t group_idx = block_idx / BLOCKS_PER_UINT64;
        size_t block_idx_in_group = block_idx % BLOCKS_PER_UINT64;

        // 清除分配位
        uint64_t& idx1_val = slab_ptr->metadata.idx1[group_idx];
        idx1_val &= ~(1ULL << block_idx_in_group);

        // 更新第一层索引（该组不再是满的）
        slab_ptr->metadata.idx0 &= ~(1ULL << group_idx);
    }

    // 获取可用块数
    size_t get_free_blocks() const {
        size_t free_count = 0;
        for (int i = 0; i < 63; i++) {
            if (i * BLOCKS_PER_UINT64 >= TOTAL_BLOCKS) break;

            uint64_t used_mask = slab_ptr->metadata.idx1[i];
            // 计算这个组中实际的块数
            size_t blocks_in_group = BLOCKS_PER_UINT64;
            if ((i + 1) * BLOCKS_PER_UINT64 > TOTAL_BLOCKS) {
                blocks_in_group = TOTAL_BLOCKS - i * BLOCKS_PER_UINT64;
            }

            // 统计空闲块
            for (size_t j = 0; j < blocks_in_group; j++) {
                if (!(used_mask & (1ULL << j))) {
                    free_count++;
                }
            }
        }
        return free_count;
    }

    // 获取总块数
    size_t get_total_blocks() const {
        return TOTAL_BLOCKS;
    }
};

// 测试和基准测试
int main() {
    constexpr int BATCH_SIZE = 2000;          // 每批次2000个对象
    constexpr int TOTAL_OPS = 100000000;       // 总共1亿次操作
    constexpr int NUM_BATCHES = TOTAL_OPS / BATCH_SIZE;  // 批次数

    void* batch_ptrs[BATCH_SIZE];  // 用于保存每批次的指针

    std::cout << "=== Slab Allocator Performance Benchmark ===" << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << "Batch size: " << BATCH_SIZE << std::endl;
    std::cout << "Number of batches: " << NUM_BATCHES << std::endl;
    std::cout << "Block size: 16 bytes" << std::endl;
    std::cout << std::endl;

    // ========== Slab Allocator测试 ==========
    std::cout << "=== Testing Slab Allocator ===" << std::endl;
    slab* my_slab = new slab;
    SlabAllocator allocator(my_slab);

    auto slab_start = std::chrono::high_resolution_clock::now();

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++) {
        // 分配2000个对象
        for (int i = 0; i < BATCH_SIZE; i++) {
            batch_ptrs[i] = allocator.allocate();
        }

        // 释放这2000个对象
        for (int i = 0; i < BATCH_SIZE; i++) {
            allocator.deallocate(batch_ptrs[i]);
        }
    }

    auto slab_end = std::chrono::high_resolution_clock::now();
    auto slab_time = std::chrono::duration_cast<std::chrono::microseconds>(slab_end - slab_start).count();

    delete my_slab;

    // ========== 标准库malloc/free测试 ==========
    std::cout << "\n=== Testing Standard Library (malloc/free) ===" << std::endl;

    auto std_start = std::chrono::high_resolution_clock::now();

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++) {
        // 分配2000个对象
        for (int i = 0; i < BATCH_SIZE; i++) {
            batch_ptrs[i] = malloc(16);  // 分配16字节
        }

        // 释放这2000个对象
        for (int i = 0; i < BATCH_SIZE; i++) {
            free(batch_ptrs[i]);
        }
    }

    auto std_end = std::chrono::high_resolution_clock::now();
    auto std_time = std::chrono::duration_cast<std::chrono::microseconds>(std_end - std_start).count();

    // ========== 标准库new/delete测试 ==========
    std::cout << "\n=== Testing Standard Library (new/delete) ===" << std::endl;

    auto new_start = std::chrono::high_resolution_clock::now();

    // 定义16字节的对象类型
    struct Block16 {
        char data[16];
    };
    Block16* new_batch_ptrs[BATCH_SIZE];

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++) {
        // 分配2000个对象
        for (int i = 0; i < BATCH_SIZE; i++) {
            new_batch_ptrs[i] = new Block16;
        }

        // 释放这2000个对象
        for (int i = 0; i < BATCH_SIZE; i++) {
            delete new_batch_ptrs[i];
        }
    }

    auto new_end = std::chrono::high_resolution_clock::now();
    auto new_time = std::chrono::duration_cast<std::chrono::microseconds>(new_end - new_start).count();

    // ========== 结果汇总 ==========
    std::cout << "\n=== Performance Summary ===" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::cout << "Slab Allocator:" << std::endl;
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

    std::cout << "Slab vs malloc/free: " << speedup_vs_malloc << "x ";
    if (speedup_vs_malloc > 1.0) {
        std::cout << "faster" << std::endl;
    } else {
        std::cout << "slower" << std::endl;
    }

    std::cout << "Slab vs new/delete: " << speedup_vs_new << "x ";
    if (speedup_vs_new > 1.0) {
        std::cout << "faster" << std::endl;
    } else {
        std::cout << "slower" << std::endl;
    }

    return 0;
}
