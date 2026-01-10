#include "buddy_allocator.h"
#include <cstring>

BuddyAllocator::BuddyAllocator()
{
    // 初始化二级位图：全部设置为满（全1）
    for (int order = 0; order < BUDDY_ALLOC_NUM_ORDERS; ++order)
    {
        free_bitmap_[order].level0 = 0ULL;
        for (int i = 0; i < 64; ++i)
            free_bitmap_[order].level1[i] = 0ULL;
    }

    // 初始化阶数记录：全部未分配
    std::memset(block_order_, 0xFF, sizeof(block_order_));
}

// 不负责释放base_addr_，由外部管理
BuddyAllocator::~BuddyAllocator() {}

void *BuddyAllocator::allocate(int order)
{
    if (order < 0 || order >= BUDDY_ALLOC_NUM_ORDERS)
        return nullptr;

    // 尝试从当前阶分配
    size_t global_index = find_free_block(order);

    // 分配失败
    if (global_index == static_cast<size_t>(-1))
        return nullptr;

    // 找到空闲块，直接分配
    set_allocated(global_index, order);

    // 记录阶数
    block_order_[global_index] = static_cast<uint8_t>(order);

    return get_block_addr(global_index, order);
}

void BuddyAllocator::deallocate(void *ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

    // 查询4KiB块索引
    size_t base_4k_index = get_block_index(ptr);
    if (base_4k_index >= BUDDY_ALLOC_TOTAL_BLOCKS)
    {
        return; // 无效指针
    }

    // 查询阶数
    int order = block_order_[base_4k_index];
    if (order == 0xFF)
    {
        return; // 未分配的块
    }

    // 清除阶数记录
    block_order_[base_4k_index] = 0xFF;

    // 标记为空闲（set_free 会自动向上传播合并信息）
    set_free(base_4k_index, order);
}

uint8_t BuddyAllocator::get_capability() const
{
    uint8_t capability = 0;

    for (int order = 0; order < BUDDY_ALLOC_NUM_ORDERS; ++order)
    {
        // 检查该order是否还有空闲块
        if (find_free_block(order) != static_cast<size_t>(-1))
        {
            capability |= (1 << order);
        }
    }

    return capability;
}

bool BuddyAllocator::owns_pointer(void *ptr) const
{
    if (ptr == nullptr)
    {
        return false;
    }

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = reinterpret_cast<uintptr_t>(data);
    return (addr >= base) && (addr < base + BUDDY_ALLOC_TOTAL_SIZE);
}

size_t BuddyAllocator::get_free_blocks(int order) const
{
    if (order < 0 || order >= BUDDY_ALLOC_NUM_ORDERS)
    {
        return 0;
    }

    size_t count = 0;
    size_t step = 1ULL << order;                    // 对于 order 的块，步长是 2^order
    size_t total_blocks = BUDDY_ALLOC_TOTAL_BLOCKS; // 4096

    // 遍历所有对齐的起始索引
    for (size_t global_index = 0; global_index < total_blocks; global_index += step)
    {
        size_t group_idx = global_index >> 6;
        size_t bit_pos = global_index & 0x3F;

        // 检查该位是否空闲（0表示空闲）
        if (!(free_bitmap_[order].level1[group_idx] & (1ULL << bit_pos)))
        {
            count++;
        }
    }

    return count;
}

size_t BuddyAllocator::get_total_free_memory() const
{
    // 统计已分配的内存，然后用总内存减去
    size_t allocated = 0;

    for (size_t i = 0; i < BUDDY_ALLOC_TOTAL_BLOCKS; ++i)
    {
        if (block_order_[i] != 0xFF)
        {
            // 这个块已分配，加上它的大小
            int order = block_order_[i];
            size_t block_size = BUDDY_ALLOC_BLOCK_SIZE << order;
            allocated += block_size;

            // 跳过该块占用的其他4KiB块
            size_t num_blocks = 1ULL << order;
            i += num_blocks - 1;
        }
    }

    return BUDDY_ALLOC_TOTAL_SIZE - allocated;
}
