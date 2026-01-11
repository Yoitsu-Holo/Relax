#ifndef SLAB_MANAGER_H
#define SLAB_MANAGER_H

#include "slab_allocator.h"
#include <cstdint>
#include <cstddef>
#include <cstring>

// slab内存分配回调函数类型
// 返回一个新的SlabAllocator对象指针，失败返回nullptr
typedef SlabAllocator *(*slab_alloc_fn)(void *context);

// 强制内联宏
#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

// 索引组结构：64个uint64的紧凑布局
// [0]: 元数据（预留，可用于未来扩展）
// [1]: 一级索引（64位，但只使用低62位对应62个二级索引的状态）
// [2-63]: 62个二级索引，每个uint64的64位对应64个slab_allocator
struct IndexGroup
{
    uint64_t metadata;          // [0] 元数据
    uint64_t level1_bitmap;     // [1] 一级索引
    uint64_t level2_bitmap[62]; // [2-63] 二级索引
} __attribute__((aligned(64))); // 缓存行对齐优化

// 扩展的Slab管理器，对底层slab_allocator进行对象管理范围扩展
// 采用紧凑的两级页表管理多个slab_allocator实例
// 只对外暴露allocate()和deallocate()接口，内部自动扩容
class SlabManager
{
private:
    // 常量定义
    static constexpr size_t NUM_GROUPS = 8;                              // 索引组数量
    static constexpr size_t L2_PER_GROUP = 62;                           // 每组的L2索引数量
    static constexpr size_t TOTAL_L2 = NUM_GROUPS * L2_PER_GROUP;        // 总L2数：496
    static constexpr size_t BITS_PER_UINT64 = 64;                        // 每个uint64的位数
    static constexpr size_t MAX_ALLOCATORS = TOTAL_L2 * BITS_PER_UINT64; // 31744个allocator

    // 8个紧凑的索引组（用于管理分配状态）
    IndexGroup index_groups_[NUM_GROUPS];

    // 8个紧凑的初始化位图组（用于标记初始化状态）
    IndexGroup init_groups_[NUM_GROUPS];

    // SlabAllocator对象数组（动态扩容）
    SlabAllocator *allocators_[MAX_ALLOCATORS];

    // 块大小（所有slab_allocator使用相同的块大小）
    size_t block_size_;

    // 回调函数的上下文参数
    void *alloc_context_;

// 辅助宏：尝试在指定组中查找空闲allocator（极简版本）
// 前提：level1未标记的位对应的level2一定有可用空间
#define TRY_FIND_IN_GROUP(group_id)                                                         \
    do                                                                                      \
    {                                                                                       \
        uint64_t available_l2 = ~index_groups_[group_id].level1_bitmap;                     \
        if (available_l2)                                                                   \
        {                                                                                   \
            l2_in_group = __builtin_ctzll(available_l2);                                    \
            bit_idx = __builtin_ctzll(~index_groups_[group_id].level2_bitmap[l2_in_group]); \
            group_idx = group_id;                                                           \
            return 0;                                                                       \
        }                                                                                   \
    } while (0)

    // 辅助方法：查找可用的allocator（循环展开优化版本）
    FORCE_INLINE int find_free_allocator(size_t &group_idx, size_t &l2_in_group, size_t &bit_idx)
    {
        // 完全展开8个组的查找循环，消除循环开销
        TRY_FIND_IN_GROUP(0);
        TRY_FIND_IN_GROUP(1);
        TRY_FIND_IN_GROUP(2);
        TRY_FIND_IN_GROUP(3);
        TRY_FIND_IN_GROUP(4);
        TRY_FIND_IN_GROUP(5);
        TRY_FIND_IN_GROUP(6);
        TRY_FIND_IN_GROUP(7);

        return -1; // 未找到可用的allocator
    }

#undef TRY_FIND_IN_GROUP

    // 辅助方法：查找并创建新的allocator（当所有现有allocator都已满时调用）
    FORCE_INLINE int find_and_create_new_allocator(size_t &group_idx, size_t &l2_in_group, size_t &bit_idx)
    {
        // 利用 init_groups 的 level1_bitmap 快速查找未初始化的位置
        for (size_t g = 0; g < NUM_GROUPS; g++)
        {
            uint64_t uninit_l2 = ~init_groups_[g].level1_bitmap;
            if (uninit_l2)
            {
                // 找到有未初始化 L2 的组
                l2_in_group = __builtin_ctzll(uninit_l2);
                uint64_t uninit_bits = ~init_groups_[g].level2_bitmap[l2_in_group];
                bit_idx = __builtin_ctzll(uninit_bits);
                group_idx = g;

                // 直接计算索引并创建 allocator
                size_t allocator_idx = (group_idx * L2_PER_GROUP + l2_in_group) * BITS_PER_UINT64 + bit_idx;
                return create_allocator(allocator_idx);
            }
        }

        // 未找到可用的未初始化位置
        return -1;
    }

    // 创建一个新的slab_allocator（内部自动扩容）
    int create_allocator(size_t index);

public:
    // 构造函数
    SlabManager()
        : block_size_(0)
    {
        memset(index_groups_, 0, sizeof(index_groups_));
        memset(init_groups_, 0, sizeof(init_groups_));
        memset(allocators_, 0, sizeof(allocators_));
    }

    // 析构函数
    ~SlabManager() = default;

    // 初始化Slab管理器
    FORCE_INLINE int init(size_t block_size)
    {
        // 检查块大小是否有效
        if (__builtin_expect(!SlabAllocator::is_valid_block_size(block_size), 0))
            return -1;

        block_size_ = block_size;

        return 0;
    }

    // 分配一个块（内部自动扩容）
    FORCE_INLINE void *allocate()
    {
        size_t group_idx, l2_in_group, bit_idx;

        // 查找有空闲空间的allocator
        if (__glibc_unlikely(find_free_allocator(group_idx, l2_in_group, bit_idx) != 0))
        {
            // 所有现有allocator都已满，尝试创建新的allocator
            if (__glibc_unlikely(find_and_create_new_allocator(group_idx, l2_in_group, bit_idx) != 0))
                return nullptr;
        }

        // 计算allocator索引（内联计算，避免函数调用）
        size_t allocator_idx = (group_idx * L2_PER_GROUP + l2_in_group) * BITS_PER_UINT64 + bit_idx;

        // 检查是否需要创建allocator
        if (__glibc_unlikely(!allocators_[allocator_idx]))
        {
            if (__glibc_unlikely(create_allocator(allocator_idx) != 0))
                return nullptr;
        }

        // 从allocator中分配内存
        SlabAllocator *allocator = allocators_[allocator_idx];
        void *ptr = allocator->allocate();

        // 如果allocator已满，标记它（内联 mark_allocator_full）
        if (__glibc_unlikely(allocator->is_full()))
        {
            index_groups_[group_idx].level2_bitmap[l2_in_group] |= (1ULL << bit_idx);
            if (index_groups_[group_idx].level2_bitmap[l2_in_group] == ~0ULL)
                index_groups_[group_idx].level1_bitmap |= (1ULL << l2_in_group);
        }

        return ptr;
    }

    // 释放一个块
    FORCE_INLINE void deallocate(void *ptr)
    {
        if (__builtin_expect(!ptr, 0))
            return;

        // 内联 find_allocator_index: 直接从slab元数据中获取manager_idx
        uint64_t manager_idx = SlabAllocator::get_metadata(ptr);
        manager_idx &= 0x7FFF;

        // 内联 index_to_position: 将全局索引转换为位置信息
        size_t global_l2_idx = manager_idx / BITS_PER_UINT64;
        size_t group_idx = global_l2_idx / L2_PER_GROUP;
        size_t l2_in_group = global_l2_idx % L2_PER_GROUP;
        size_t bit_idx = manager_idx % BITS_PER_UINT64;

        // 验证该allocator是否已初始化
        if (__builtin_expect(group_idx >= NUM_GROUPS || l2_in_group >= L2_PER_GROUP, 0))
            return;

        if (__builtin_expect((init_groups_[group_idx].level2_bitmap[l2_in_group] & (1ULL << bit_idx)) == 0, 0))
            return;

        SlabAllocator *allocator = allocators_[manager_idx];
        if (__builtin_expect(!allocator, 0))
            return;

        // 检查allocator在释放前是否已满
        bool was_full = allocator->is_full();

        // 释放内存
        allocator->deallocate(ptr);

        // 如果之前是满的，现在不满了，更新位图（内联 mark_allocator_available）
        if (__builtin_expect(was_full, 0))
        {
            index_groups_[group_idx].level2_bitmap[l2_in_group] &= ~(1ULL << bit_idx);
            index_groups_[group_idx].level1_bitmap &= ~(1ULL << l2_in_group);
        }
    }

    // 获取块大小
    FORCE_INLINE size_t get_block_size() const { return block_size_; }

    // 获取已初始化的allocator数量
    inline size_t get_initialized_count() const
    {
        size_t count = 0;
        for (size_t g = 0; g < NUM_GROUPS; g++)
            for (size_t l2 = 0; l2 < L2_PER_GROUP; l2++)
                count += __builtin_popcountll(init_groups_[g].level2_bitmap[l2]);
        return count;
    }
};

#endif // SLAB_MANAGER_H
