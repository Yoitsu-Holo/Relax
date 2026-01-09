#ifndef SLAB_ALLOCATOR_H
#define SLAB_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include <cstring>

#define KiB 1024
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)

// 强制内联宏
#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

// 每个slab分配器占用64KiB内存
// 前512字节为metadata（64个uint64_t = 512字节）
// 后面的内存用于实际分配

union slab_mem
{
    struct
    {
        uint64_t manager;  // 管理信息
        uint64_t idx0;     // 第一层索引（位图）
        uint64_t idx1[62]; // 第二层索引（位图）
    } metadata;
    uint8_t data[64 * KiB];
};

// Slab分配器类, 严格内存布局, 不允许使用虚函数、继承等
class SlabAllocator
{
private:
    alignas(64) slab_mem slab_ptr; // 对齐cacheline

    static constexpr size_t METADATA_SIZE = (sizeof(uint64_t) * 64); // 元数据大小（始终是512字节）
    static constexpr size_t BLOCKS_PER_UINT64 = 64;                  // 每个uint64_t管理64个块（这个是固定的）

    // 这些值在init函数中动态设置
    size_t block_size_;   // 块大小
    size_t usable_size_;  // 可用空间
    size_t total_blocks_; // 总块数

public:
    // 建议的块大小枚举
    enum BlockSize
    {
        BLOCK_16 = 16,
        BLOCK_32 = 32,
        BLOCK_64 = 64,
        BLOCK_128 = 128,
        BLOCK_192 = 192,
        BLOCK_256 = 256,
        BLOCK_384 = 384,
        BLOCK_512 = 512,
        BLOCK_768 = 768,
        BLOCK_1024 = 1024,
        BLOCK_1536 = 1536,
        BLOCK_2048 = 2048,
        BLOCK_3072 = 3072,
        BLOCK_4096 = 4096
    };

    // 构造函数
    FORCE_INLINE SlabAllocator(int block_size, uint64_t manager = 0)
        : block_size_(0),
          usable_size_(0),
          total_blocks_(0)
    {
        // 检查块大小是否有效
        if (__builtin_expect(!is_valid_block_size(block_size), 0))
            return;

        // 设置成员变量
        block_size_ = block_size;
        usable_size_ = 64 * KiB - METADATA_SIZE;               // 可用空间
        size_t calculated_blocks = usable_size_ / block_size_; // 计算总块数

        // 由于idx1只有62个元素，最多只能管理62*64=3968个块
        const size_t max_manageable_blocks = 62 * 64;
        total_blocks_ = calculated_blocks > max_manageable_blocks ? max_manageable_blocks : calculated_blocks;

        // 初始化metadata
        memset(&slab_ptr.metadata, 0, sizeof(slab_ptr.metadata));

        // 设置manager_idx
        slab_ptr.metadata.manager = manager;

        int remaining_blocks = total_blocks_;
        slab_ptr.metadata.idx0 = ~0ULL; // 初始化为全1（假设所有组都满）

        for (int idx = 0; idx < 62; ++idx)
        {
            if (remaining_blocks > 0)
            {
                slab_ptr.metadata.idx0 &= ~(1ULL << idx); // 标记这个组有空闲块
                if (remaining_blocks >= 64)
                    slab_ptr.metadata.idx1[idx] = 0; // 全部可用
                else
                    slab_ptr.metadata.idx1[idx] = (~0ULL) << remaining_blocks; // 部分可用
            }
            else
            {
                // idx0已经初始化为全1，不需要修改
                slab_ptr.metadata.idx1[idx] = ~0ULL; // 全部不可用
            }
            remaining_blocks -= 64;
        }
        return;
    }

    // 析构函数
    ~SlabAllocator() = default;

    // 分配一个块 - 热路径函数，强制内联
    FORCE_INLINE void *allocate()
    {
        // 找到第一个不全满的组（idx0中为0的位）
        uint64_t available_groups = ~slab_ptr.metadata.idx0;
        if (__builtin_expect(available_groups == 0, 0))
            return nullptr;

        // 找到第一个可用组
        int group_idx = __builtin_ctzll(available_groups);

        // 在对应的第二层索引中找空闲块
        uint64_t &idx1_val = slab_ptr.metadata.idx1[group_idx];
        int block_idx_in_group = __builtin_ctzll(~idx1_val);

        // 标记该块为已分配
        idx1_val |= (1ULL << block_idx_in_group);

        // 如果这个组满了，更新第一层索引
        if (__builtin_expect(idx1_val == ~0ULL, 0))
            slab_ptr.metadata.idx0 |= (1ULL << group_idx);

        // 计算实际的块索引和返回地址（合并计算，减少中间变量）
        size_t block_idx = (static_cast<size_t>(group_idx) << 6) | block_idx_in_group;
        return slab_ptr.data + METADATA_SIZE + block_idx * block_size_;
    }

    // 释放一个块 - 热路径函数，强制内联
    FORCE_INLINE void deallocate(void *ptr)
    {
        if (__builtin_expect(!ptr, 0))
            return;

        uint8_t *block = static_cast<uint8_t *>(ptr);
        uint8_t *base = slab_ptr.data + METADATA_SIZE;

        // 计算块索引
        size_t offset = block - base;
        size_t block_idx = offset / block_size_;

        // 计算组索引和组内索引 - 使用位运算优化
        size_t group_idx = block_idx >> 6;            // 除以64
        size_t block_idx_in_group = block_idx & 0x3F; // 模64

        // 清除分配位
        uint64_t &idx1_val = slab_ptr.metadata.idx1[group_idx];
        idx1_val &= ~(1ULL << block_idx_in_group);

        // 更新第一层索引（该组不再是满的）
        slab_ptr.metadata.idx0 &= ~(1ULL << group_idx);
    }

    // 获取可用块数
    inline size_t get_free_blocks() const
    {
        size_t free_count = 0;
        for (int i = 0; i < 62; i++)
            free_count += __builtin_popcountll(~slab_ptr.metadata.idx1[i]);
        return free_count;
    }

    // 获取总块数
    FORCE_INLINE size_t get_total_blocks() const
    {
        return total_blocks_;
    }

    // 获取块大小
    FORCE_INLINE size_t get_block_size() const { return block_size_; }

    // 获取manager索引 - 从slab指针直接获取
    static FORCE_INLINE uint64_t get_metadata(void *ptr)
    {
        // 不保证上层处理，上层自己保证传入指针正确性
        if (__glibc_unlikely(!ptr))
            return 0;

        // 获取slab的起始地址（64KiB对齐）
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t slab_addr = addr & ~(64 * KiB - 1); // 清除低16位得到64KiB对齐的地址
        SlabAllocator *s = reinterpret_cast<SlabAllocator *>(slab_addr);

        return s->slab_ptr.metadata.manager;
    }

    // 获取使用率（百分比）
    inline double get_usage() const
    {
        size_t free_blocks = get_free_blocks();
        return (1.0 - static_cast<double>(free_blocks) / total_blocks_) * 100.0;
    }

    // 检查是否已满 - 热路径函数，强制内联
    FORCE_INLINE bool is_full() const
    {
        // 如果第一层索引全为1，表示所有组都满了
        return slab_ptr.metadata.idx0 == ~0ULL;
    }

    // 检查是否为空
    inline bool is_empty() const
    {
        size_t free_blocks = get_free_blocks();
        return free_blocks == total_blocks_;
    }

    // 静态方法：检查块大小是否有效
    static FORCE_INLINE bool is_valid_block_size(int block_size)
    {
        // 检查块大小是否有效
        if (__builtin_expect(block_size < 16 || block_size > 4 * KiB, 0))
            return false;
        if (block_size < 64 && (block_size & 15)) // 使用位运算检查%16
            return false;
        if (block_size >= 64 && (block_size & 63)) // 使用位运算检查%64
            return false;
        return true;
    }
};

#endif // SLAB_ALLOCATOR_H
