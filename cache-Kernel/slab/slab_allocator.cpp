#include "slab_allocator.h"
#include <cstring>
#include <cassert>

// 构造函数
SlabAllocator::SlabAllocator(slab *s)
    : slab_ptr(s),
      block_size_(0),
      metadata_size_(sizeof(uint64_t) * 64),
      usable_size_(0),
      total_blocks_(0)
{
    // 构造函数暂不初始化metadata，等待init调用
}

// 初始化slab分配器
int SlabAllocator::init(int block_size)
{
    // 检查块大小是否有效
    if (!is_valid_block_size(block_size))
        return -1;

    // 设置成员变量
    block_size_ = block_size;
    metadata_size_ = sizeof(uint64_t) * 64;                // 512字节
    usable_size_ = 64 * KiB - metadata_size_;              // 可用空间
    size_t calculated_blocks = usable_size_ / block_size_; // 计算总块数

    // 由于idx1只有63个元素，最多只能管理63*64=4032个块
    const size_t max_manageable_blocks = 63 * 64;
    total_blocks_ = calculated_blocks > max_manageable_blocks ? max_manageable_blocks : calculated_blocks;

    // 初始化metadata
    memset(&slab_ptr->metadata, 0, sizeof(slab_ptr->metadata));

    int remaining_blocks = total_blocks_;
    slab_ptr->metadata.idx0 = ~0ULL; // 初始化为全1（假设所有组都满）

    for (int idx = 0; idx < 63; ++idx)
    {
        if (remaining_blocks > 0)
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
void *SlabAllocator::allocate()
{
    // 检查第一层索引，找到有空闲块的组
    uint64_t idx0_val = slab_ptr->metadata.idx0;

    // 找到第一个不全满的组（idx0中为0的位）
    uint64_t available_groups = ~idx0_val;
    if (__builtin_expect(available_groups == 0, 0))
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
void SlabAllocator::deallocate(void *ptr)
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
size_t SlabAllocator::get_free_blocks() const
{
    size_t free_count = 0;
    for (int i = 0; i < 63; i++)
        free_count += __builtin_popcountll(~slab_ptr->metadata.idx1[i]);
    return free_count;
}

// 获取总块数
size_t SlabAllocator::get_total_blocks() const
{
    return total_blocks_;
}

// 检查是否已满
bool SlabAllocator::is_full() const
{
    // 如果第一层索引全为1，表示所有组都满了
    return slab_ptr->metadata.idx0 == ~0ULL;
}

// 检查是否为空
bool SlabAllocator::is_empty() const
{
    size_t free_blocks = get_free_blocks();
    return free_blocks == total_blocks_;
}

// 静态方法：检查块大小是否有效
bool SlabAllocator::is_valid_block_size(int block_size)
{
    // 检查块大小是否有效
    if (block_size < 16 || block_size > 4 * KiB)
        return false;
    if (block_size < 64 && block_size % 16)
        return false;
    if (block_size >= 64 && block_size % 64)
        return false;
    return true;
}