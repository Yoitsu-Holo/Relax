#ifndef BUDDY_MANAGER_H
#define BUDDY_MANAGER_H

#include "buddy_allocator.h"
#include <cstddef>
#include <cstdint>

class BuddyManager
{
public:
    BuddyManager();
    ~BuddyManager();

    // 禁用拷贝和赋值
    BuddyManager(const BuddyManager &) = delete;
    BuddyManager &operator=(const BuddyManager &) = delete;

    // 初始化：指定最大allocator数量
    bool init(size_t max_allocators = 64);

    // 分配：自动选择或创建allocator
    void *allocate(size_t size);

    // 释放：自动定位allocator
    void deallocate(void *ptr);

    // 统计信息
    size_t get_total_free_memory() const;
    size_t get_allocator_count() const { return num_allocators_; }

private:
    // 链表节点结构
    struct AllocatorListNode
    {
        int prev; // 前驱索引（-1表示无）
        int next; // 后继索引（-1表示无）
    };

    // 辅助函数
    inline int size_to_order(size_t size) const;
    size_t find_allocator_by_pointer(void *ptr) const;
    void update_allocator_lists(size_t alloc_idx, uint8_t new_capability);
    void add_to_list_head(int order, int alloc_idx);
    void remove_from_list(int order, int alloc_idx);

private:
    // Allocator池
    BuddyAllocator **allocators_; // allocator指针数组（按需分配）
    size_t max_allocators_;       // 最大allocator数量
    size_t num_allocators_;       // 当前allocator数量

    // 链表数组：7个order，每个维护可用allocator的链表
    // free_lists_[order][idx] 存储allocator索引的链表节点
    // 0号元素是链表头
    AllocatorListNode free_lists_[BUDDY_ALLOC_NUM_ORDERS][4096];

    // 能力位图：标记每个allocator能分配哪些order
    // capability_bitmap_[alloc_idx] 的每位表示该allocator能否分配对应order
    uint8_t capability_bitmap_[4096];
};

// 内联函数实现

inline int BuddyManager::size_to_order(size_t size) const
{
    if (size <= BUDDY_ALLOC_BLOCK_SIZE)
        return 0;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 2)
        return 1;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 4)
        return 2;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 8)
        return 3;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 16)
        return 4;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 32)
        return 5;
    if (size <= BUDDY_ALLOC_BLOCK_SIZE * 64)
        return 6;
    return -1; // 超出范围
}

#endif // BUDDY_MANAGER_H
