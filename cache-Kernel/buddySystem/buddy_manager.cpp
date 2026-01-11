#include "buddy_manager.h"
#include <cstring>
#include <cstdlib>

BuddyManager::BuddyManager()
    : allocators_(nullptr),
      max_allocators_(0),
      num_allocators_(0)
{
    // 初始化链表数组
    for (int order = 0; order < BUDDY_ALLOC_NUM_ORDERS; ++order)
    {
        for (int i = 0; i < 4096; ++i)
        {
            free_lists_[order][i].prev = -1;
            free_lists_[order][i].next = -1;
        }
    }

    // 初始化能力位图
    std::memset(capability_bitmap_, 0, sizeof(capability_bitmap_));
}

BuddyManager::~BuddyManager()
{
    // 释放所有allocator的内存
    if (allocators_)
    {
        for (size_t i = 0; i < num_allocators_; ++i)
        {
            if (allocators_[i])
            {
                // 显式调用析构函数并释放内存
                allocators_[i]->~BuddyAllocator();
                std::free(allocators_[i]);
            }
        }
        // 释放指针数组
        delete[] allocators_;
    }
}

bool BuddyManager::init(size_t max_allocators)
{
    if (max_allocators == 0 || max_allocators > 4096)
    {
        return false;
    }

    max_allocators_ = max_allocators;
    num_allocators_ = 0;

    // 分配 allocator 指针数组
    allocators_ = new BuddyAllocator *[max_allocators];

    // 初始化所有指针为 nullptr
    for (size_t i = 0; i < max_allocators; ++i)
    {
        allocators_[i] = nullptr;
    }

    return true;
}

void *BuddyManager::allocate(size_t size)
{
    if (size == 0 || size > BUDDY_ALLOC_BLOCK_SIZE * 64)
    {
        return nullptr;
    }

    // 计算order
    int order = size_to_order(size);
    if (order < 0)
    {
        return nullptr;
    }

    // 从链表数组获取可用allocator
    int head_idx = free_lists_[order][0].next;

    if (head_idx != -1)
    {
        // 找到可用allocator
        void *ptr = allocators_[head_idx]->allocate(order);

        if (ptr != nullptr)
        {
            // 分配成功，更新allocator能力
            uint8_t new_cap = allocators_[head_idx]->get_capability();
            update_allocator_lists(head_idx, new_cap);
            return ptr;
        }
    }

    // 当前order没有可用allocator，创建新的
    if (num_allocators_ >= max_allocators_)
    {
        return nullptr; // 无法扩展
    }

    // 使用 BuddyAllocator::get_new() 创建新的 allocator
    // 这会分配 16MiB 对齐的内存并使用 placement new 构造 BuddyAllocator
    size_t alloc_idx = num_allocators_++;
    allocators_[alloc_idx] = BuddyAllocator::get_new();

    if (allocators_[alloc_idx] == nullptr)
    {
        --num_allocators_;
        return nullptr; // 内存分配失败
    }

    // 尝试分配
    void *ptr = allocators_[alloc_idx]->allocate(order);

    if (ptr != nullptr)
    {
        // 分配成功，更新allocator能力
        uint8_t new_cap = allocators_[alloc_idx]->get_capability();
        update_allocator_lists(alloc_idx, new_cap);
        return ptr;
    }

    // 分配失败（不应该发生）
    return nullptr;
}

void BuddyManager::deallocate(void *ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

    // 定位allocator
    size_t alloc_idx = find_allocator_by_pointer(ptr);
    if (alloc_idx == static_cast<size_t>(-1))
    {
        return; // 无效指针
    }

    // 执行释放
    allocators_[alloc_idx]->deallocate(ptr);

    // 更新allocator能力
    uint8_t new_cap = allocators_[alloc_idx]->get_capability();
    update_allocator_lists(alloc_idx, new_cap);
}

size_t BuddyManager::find_allocator_by_pointer(void *ptr) const
{
    // 线性查找（allocator数量通常很小）
    for (size_t i = 0; i < num_allocators_; ++i)
    {
        if (allocators_[i]->owns_pointer(ptr))
        {
            return i;
        }
    }

    return static_cast<size_t>(-1);
}

void BuddyManager::update_allocator_lists(size_t alloc_idx, uint8_t new_capability)
{
    uint8_t old_capability = capability_bitmap_[alloc_idx];

    // 对每个order检查能力变化
    for (int order = 0; order < BUDDY_ALLOC_NUM_ORDERS; ++order)
    {
        bool old_in_list = (old_capability & (1 << order)) != 0;
        bool new_in_list = (new_capability & (1 << order)) != 0;

        if (old_in_list && !new_in_list)
        {
            // 从链表移除
            remove_from_list(order, alloc_idx);
        }
        else if (!old_in_list && new_in_list)
        {
            // 加入链表头
            add_to_list_head(order, alloc_idx);
        }
        else if (old_in_list && new_in_list)
        {
            // 仍在链表中，移到链表头（提高局部性）
            remove_from_list(order, alloc_idx);
            add_to_list_head(order, alloc_idx);
        }
    }

    // 更新能力位图
    capability_bitmap_[alloc_idx] = new_capability;
}

void BuddyManager::add_to_list_head(int order, int alloc_idx)
{
    // 插入到链表头（0号节点后面）
    int old_first = free_lists_[order][0].next;

    free_lists_[order][alloc_idx].prev = 0;
    free_lists_[order][alloc_idx].next = old_first;
    free_lists_[order][0].next = alloc_idx;

    if (old_first != -1)
    {
        free_lists_[order][old_first].prev = alloc_idx;
    }
}

void BuddyManager::remove_from_list(int order, int alloc_idx)
{
    int prev_idx = free_lists_[order][alloc_idx].prev;
    int next_idx = free_lists_[order][alloc_idx].next;

    if (prev_idx != -1)
    {
        free_lists_[order][prev_idx].next = next_idx;
    }

    if (next_idx != -1)
    {
        free_lists_[order][next_idx].prev = prev_idx;
    }

    // 标记为已移除（自指）
    free_lists_[order][alloc_idx].prev = -1;
    free_lists_[order][alloc_idx].next = alloc_idx;
}

size_t BuddyManager::get_total_free_memory() const
{
    size_t total = 0;

    for (size_t i = 0; i < num_allocators_; ++i)
    {
        total += allocators_[i]->get_total_free_memory();
    }

    return total;
}
