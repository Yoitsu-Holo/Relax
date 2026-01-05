#ifndef SLAB_ALLOCATOR_H
#define SLAB_ALLOCATOR_H

#include <cstdint>
#include <cstddef>

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
    // 支持的块大小枚举
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
    SlabAllocator(slab *s);

    // 析构函数
    ~SlabAllocator() = default;

    // 初始化slab分配器
    // @param block_size: 块大小，必须是支持的大小之一
    // @return: 0成功，-1失败
    int init(int block_size);

    // 分配一个块
    // @return: 分配的内存指针，失败返回nullptr
    void *allocate();

    // 释放一个块
    // @param ptr: 要释放的内存指针
    void deallocate(void *ptr);

    // 获取可用块数
    size_t get_free_blocks() const;

    // 获取总块数
    size_t get_total_blocks() const;

    // 获取块大小
    size_t get_block_size() const { return block_size_; }

    // 获取使用率（百分比）
    double get_usage() const
    {
        size_t free_blocks = get_free_blocks();
        return (1.0 - (double)free_blocks / total_blocks_) * 100.0;
    }

    // 检查是否已满
    bool is_full() const;

    // 检查是否为空
    bool is_empty() const;

    // 静态方法：检查块大小是否有效
    static bool is_valid_block_size(int block_size);
};

#endif // SLAB_ALLOCATOR_H