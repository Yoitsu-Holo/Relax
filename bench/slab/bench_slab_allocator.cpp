
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
union slab
{
    struct
    {
        uint64_t idx0;     // 第一层索引（位图）
        uint64_t idx1[63]; // 第二层索引（位图）
    } metadata;
    uint8_t data[64 * KiB];
};

// Slab分配器类
class SlabAllocator
{
private:
    // 这些值在init函数中动态设置
    size_t block_size_;                             // 块大小
    size_t metadata_size_;                          // 元数据大小（始终是512字节）
    size_t usable_size_;                            // 可用空间
    size_t total_blocks_;                           // 总块数
    static constexpr size_t BLOCKS_PER_UINT64 = 64; // 每个uint64_t管理64个块（这个是固定的）

    alignas(64) slab *slab_ptr; // 对齐cacheline

public:
    SlabAllocator(slab *s) : slab_ptr(s),
                             block_size_(0),
                             metadata_size_(sizeof(uint64_t) * 64),
                             usable_size_(0),
                             total_blocks_(0)
    {
        // 构造函数暂不初始化metadata，等待init调用
    }

    int init(int block_size)
    {
        switch (block_size)
        {
        case 16:
        case 32:
        case 64:
        case 128:
        case 192:
        case 256:
        case 384:
        case 512:
        case 768:
        case 1024:
        case 1536:
        case 2048:
        case 3072:
        case 4096:
            break;
        default:
            return -1;
        }

        // 设置成员变量
        block_size_ = block_size;
        metadata_size_ = sizeof(uint64_t) * 64;     // 512字节
        usable_size_ = 64 * KiB - metadata_size_;   // 可用空间
        total_blocks_ = usable_size_ / block_size_; // 计算总块数

        // 初始化metadata
        memset(&slab_ptr->metadata, 0, sizeof(slab_ptr->metadata));

        int remaining_blocks = total_blocks_;
        slab_ptr->metadata.idx0 = ~0ULL; // 初始化为全1（假设所有组都满）

        for (int idx = 0; idx < 63; ++idx)
        {
            if (remaining_blocks)
            {
                slab_ptr->metadata.idx0 &= ~(1ULL << idx); // 标记这个组有空闲块
                if (remaining_blocks >= 64)
                    slab_ptr->metadata.idx1[idx] = 0; // 全部可用
                else
                    slab_ptr->metadata.idx1[idx] = (~0ULL) << remaining_blocks; // 部分可用
            }
            else
            {
                // idx0已经初始化为全1，不需要修改
                slab_ptr->metadata.idx1[idx] = ~0ULL; // 全部不可用
            }
            remaining_blocks -= 64;
        }
        return 0;
    }

    // 分配一个块
    void *allocate()
    {
        // 检查第一层索引，找到有空闲块的组
        uint64_t idx0_val = slab_ptr->metadata.idx0;

        // 找到第一个不全满的组（idx0中为0的位）
        uint64_t available_groups = ~idx0_val;
        if (__glibc_unlikely(available_groups == 0))
        {
            // 所有组都满了
            return nullptr;
        }

        // 使用__builtin_ctzll找到第一个可用组
        int group_idx = __builtin_ctzll(available_groups);

        // 在对应的第二层索引中找空闲块
        uint64_t &idx1_val = slab_ptr->metadata.idx1[group_idx];
        uint64_t free_blocks = ~idx1_val;

        // 找到第一个空闲块
        int block_idx_in_group = __builtin_ctzll(free_blocks);
        // 如果初始化正确设置了不可用位为1，这里不会访问超出范围的块

        // 标记该块为已分配
        idx1_val |= (1ULL << block_idx_in_group);

        // 如果这个组满了，更新第一层索引
        if (idx1_val == ~0ULL)
            slab_ptr->metadata.idx0 |= (1ULL << group_idx);

        // 计算实际的块索引
        size_t block_idx = group_idx * BLOCKS_PER_UINT64 + block_idx_in_group;

        // 计算返回的地址
        uint8_t *base = slab_ptr->data + metadata_size_;
        return base + block_idx * block_size_;
    }

    // 释放一个块
    void deallocate(void *ptr)
    {
        if (!ptr)
            return;

        uint8_t *block = static_cast<uint8_t *>(ptr);
        uint8_t *base = slab_ptr->data + metadata_size_;

        // 计算块索引
        size_t offset = block - base;
        // 为了最佳性能，不检查offset的有效性（信任调用者）
        size_t block_idx = offset / block_size_;

        // 计算组索引和组内索引
        size_t group_idx = block_idx / BLOCKS_PER_UINT64;
        size_t block_idx_in_group = block_idx % BLOCKS_PER_UINT64;

        // 清除分配位
        uint64_t &idx1_val = slab_ptr->metadata.idx1[group_idx];
        // 允许重复释放以获得最佳性能，不检查块是否已经被释放
        idx1_val &= ~(1ULL << block_idx_in_group);

        // 更新第一层索引（该组不再是满的）
        slab_ptr->metadata.idx0 &= ~(1ULL << group_idx);
    }

    // 获取可用块数
    size_t get_free_blocks() const
    {
        size_t free_count = 0;
        for (int i = 0; i < 63; i++)
            free_count += __builtin_popcountll(~slab_ptr->metadata.idx1[i]);
        return free_count;
    }

    // 获取总块数
    size_t get_total_blocks() const
    {
        return total_blocks_;
    }
};

int main()
{
    constexpr int WORD_LEN = 27;                        // 分配字长
    constexpr int BATCH_SIZE = 1000;                    // 每批次 BATCH_SIZE 个对象
    constexpr int TOTAL_OPS = 500000000;                // 总共 5亿 次操作
    constexpr int NUM_BATCHES = TOTAL_OPS / BATCH_SIZE; // 批次数

    void *batch_ptrs[BATCH_SIZE]; // 用于保存每批次的指针

    std::cout << "=== Slab Allocator Performance Benchmark ===" << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << "Batch size: " << BATCH_SIZE << std::endl;
    std::cout << "Number of batches: " << NUM_BATCHES << std::endl;
    std::cout << "Block size: " << WORD_LEN << " bytes" << std::endl;
    std::cout << std::endl;

    // ========== Slab Allocator测试 ==========
    std::cout << "=== Testing Slab Allocator ===" << std::endl;
    slab *my_slab = new slab;
    SlabAllocator allocator(my_slab);

    // 初始化为32字节块大小进行测试
    if (allocator.init(32) != 0)
    {
        std::cerr << "Failed to initialize slab allocator with block size 32" << std::endl;
        return 1;
    }

    std::cout << "Initialized with block size: " << WORD_LEN << " bytes" << std::endl;
    std::cout << "Total blocks available: " << allocator.get_total_blocks() << std::endl;
    // 32字节块时总共有2032个块，而BATCH_SIZE是1000，不会超出容量
    std::cout << std::endl;

    auto slab_start = std::chrono::high_resolution_clock::now();

    // 执行批次操作
    for (int batch = 0; batch < NUM_BATCHES; batch++)
    {
        // 分配对象
        for (int i = 0; i < BATCH_SIZE; i++)
            batch_ptrs[i] = allocator.allocate();

        // 释放对象
        for (int i = 0; i < BATCH_SIZE; i++)
            allocator.deallocate(batch_ptrs[i]);
    }

    auto slab_end = std::chrono::high_resolution_clock::now();
    auto slab_time = std::chrono::duration_cast<std::chrono::microseconds>(slab_end - slab_start).count();

    delete my_slab;

    // ========== 标准库malloc/free测试 ==========
    std::cout << "\n=== Testing Standard Library (malloc/free) ===" << std::endl;

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
    std::cout << "\n=== Testing Standard Library (new/delete) ===" << std::endl;

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

        // 释放这对象
        for (int i = 0; i < BATCH_SIZE; i++)
            delete new_batch_ptrs[i];
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
    if (speedup_vs_malloc > 1.0)
    {
        std::cout << "faster" << std::endl;
    }
    else
    {
        std::cout << "slower" << std::endl;
    }

    std::cout << "Slab vs new/delete: " << speedup_vs_new << "x ";
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
