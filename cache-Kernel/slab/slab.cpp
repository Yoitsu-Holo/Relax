#include "slab.h"
#include <new>
#include <cstdlib>

// 创建一个新的slab_allocator（内部自动扩容）
int Slab::create_allocator(size_t index)
{
    if (index >= MAX_ALLOCATORS)
        return -1;

    // 内联 index_to_position: 将全局索引转换为位置信息
    size_t global_l2_idx = index / BITS_PER_UINT64;
    size_t group_idx = global_l2_idx / L2_PER_GROUP;
    size_t l2_in_group = global_l2_idx % L2_PER_GROUP;
    size_t bit_idx = index % BITS_PER_UINT64;

    // 检查是否已初始化
    if ((init_groups_[group_idx].level2_bitmap[l2_in_group] & (1ULL << bit_idx)) != 0)
        return 0;

    // 通过 posix_memalign 分配 64KiB 对齐的内存
    void *ptr = nullptr;
    size_t alignment = 65536;
    int result = posix_memalign(&ptr, alignment, sizeof(SlabAllocator));
    if (result != 0 || !ptr)
        return -1;

    if ((uint64_t)(ptr) & (alignment - 1))
    {
        ::operator delete(ptr);
        return -1;
    }

    // 创建 SlabAllocator 对象，使用 placement new，并传入 index 作为 manager 参数
    SlabAllocator *allocator = new (ptr) SlabAllocator(block_size_, index);
    if (!allocator)
    {
        ::operator delete(ptr);
        return -1;
    }

    // 保存allocator
    allocators_[index] = allocator;

    // 标记为已初始化（内联 mark_allocator_initialized）
    init_groups_[group_idx].level2_bitmap[l2_in_group] |= (1ULL << bit_idx);
    if (init_groups_[group_idx].level2_bitmap[l2_in_group] == ~0ULL)
        init_groups_[group_idx].level1_bitmap |= (1ULL << l2_in_group);

    return 0;
}
