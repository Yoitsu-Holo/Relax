#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <bit>

#define KiB 1024
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)

// 强制内联宏
// 强制内联宏
#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

// 常量定义
constexpr size_t BUDDY_ALLOC_BLOCK_SIZE = 4 * KiB;  // 4KiB 基本块大小
constexpr size_t BUDDY_ALLOC_TOTAL_SIZE = 16 * MiB; // 16MiB 总大小
constexpr size_t BUDDY_ALLOC_TOTAL_BLOCKS = 4096;   // 总块数
constexpr int BUDDY_ALLOC_NUM_ORDERS = 7;           // 阶数：0-6

// 分治式构造 64 位全 1 掩码：从最高位逐步“复制填充”
// 每一步右移量 = 1 << (6-i)，用于“倍增扩散”
constexpr uint64_t order_shift[] = {(1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6)};

constexpr uint64_t order_wide[] = {
    (1ULL << order_shift[0]) - 1, // 0b 1000 0000 ...
    (1ULL << order_shift[1]) - 1, // 0b 1100 0000 ...
    (1ULL << order_shift[2]) - 1, // 0b 1111 0000 ...
    (1ULL << order_shift[3]) - 1, // 0b 1111 1111 ...
    (1ULL << order_shift[4]) - 1, // 0x FFFF 0000 0000 0000
    (1ULL << order_shift[5]) - 1, // 0x FFFF FFFF 0000 0000
    ~0ULL                         // 0x FFFF FFFF FFFF FFFF 特殊处理
};

constexpr uint64_t build_mask(int step)
{
    if (step == 6)
        return 1ULL;
    uint64_t prev = build_mask(step + 1);
    return prev | (prev << order_shift[step]);
}

constexpr uint64_t order_mask[] = {
    build_mask(0), // 0b 1111 1111 ...
    build_mask(1), // 0b 1010 1010 ...
    build_mask(2), // 0b 1000 1000 ...
    build_mask(3), // 0b 1000 0000 ...
    build_mask(4), // ...
    build_mask(5), // ...
    build_mask(6)  // ...
};

// 二级位图结构
struct BuddyAllocTwoLevelBitmap
{
    uint64_t level0;     // 第一级：64位，每位表示一个组
    uint64_t level1[64]; // 第二级：64个uint64
};

// 全局4KiB粒度位图
struct BuddyAllocGlobalBitmap
{
    uint64_t bits[64]; // 4096位，每位对应一个4KiB块
};

class BuddyAllocator
{
public:
    BuddyAllocator();
    ~BuddyAllocator();

    // 禁用拷贝和赋值
    BuddyAllocator(const BuddyAllocator &) = delete;
    BuddyAllocator &operator=(const BuddyAllocator &) = delete;

    // 分配：传入order（0-6），返回指针
    void *allocate(int order);

    // 释放：传入指针，自动查询阶数
    void deallocate(void *ptr);

    // 查询能力：返回uint8位图，每位表示该order是否还能分配
    // bit 0: 4KiB, bit 1: 8KiB, ..., bit 6: 256KiB
    uint8_t get_capability() const;

    // 检查指针是否属于此allocator
    bool owns_pointer(void *ptr) const;

    // 统计信息
    size_t get_free_blocks(int order) const;
    size_t get_total_free_memory() const;

    static BuddyAllocator *get_new()
    {
        void *mem = aligned_alloc(16 * MiB, sizeof(BuddyAllocator));
        if (!mem)
            return nullptr;
        BuddyAllocator *buddy = new (mem) BuddyAllocator();
        return buddy;
    }

private:
    FORCE_INLINE uint8_t get_order(size_t size)
    {
        if (size == 0)
            return 1;
        size--;
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        size |= size >> 32;
        size++;
        return size;

        // C++ 20 方案
        // return 1U << std::bit_width(size - 1U);
    }

    // 位图操作
    FORCE_INLINE void set_free(size_t index, int order);
    FORCE_INLINE void set_allocated(size_t index, int order);
    FORCE_INLINE size_t find_free_block(int order) const;

    // 辅助函数
    FORCE_INLINE void *get_block_addr(size_t global_index, int order)
    {
        // global_index 是全局 4KiB 块索引（0-4095）
        // order 参数保留用于文档说明，但不影响计算
        (void)order;                        // 避免未使用警告
        return data + (global_index << 12); // 12 = log2(4KiB)
    }

    FORCE_INLINE size_t get_block_index(void *ptr)
    {
        // 返回全局 4KiB 块索引（0-4095）
        uintptr_t offset = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(data);
        return offset >> 12; // 12 = log2(4KiB)
    }

    FORCE_INLINE uint64_t get_metadata(void *ptr)
    {
        if (__glibc_unlikely(!ptr))
            return 0;

        // 获取slab的起始地址（64KiB对齐）
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t buddy_addr = addr & ~(16 * MiB - 1); // 清除低位得到16MiB对齐的地址
        BuddyAllocator *s = reinterpret_cast<BuddyAllocator *>(buddy_addr);
        return s->metadata_;
    }

private:
    char data[16 * MiB]; // 16MiB对齐的内存起始地址

    uint64_t metadata_;

    // 二级位图：每个order一个
    BuddyAllocTwoLevelBitmap free_bitmap_[BUDDY_ALLOC_NUM_ORDERS];

    // 阶数记录：每个4KiB块记录其所属的分配阶数
    // 0xFF表示未分配，0-6表示阶数
    uint8_t block_order_[BUDDY_ALLOC_TOTAL_BLOCKS];
};

// 内联函数实现

// 将所有分配块标记
FORCE_INLINE void BuddyAllocator::set_free(size_t index, int order)
{
    /*
    分配前我的分配标记就应该是（演示采用三级位图）：
    L0: 0 1 0 0 0 0 0 0
    L1: 10  00  00  00
    L2: 1000    0000

    如果我分配了一个8K大小的块，那么我的分配标记就应该是：
    L0: 0 1 0 0 1 1 0 0
    L1: 10  00  10  00
    L2: 1000    1000

    在他释放后：我会想向下扩散分配信息，取消标记（第一个switch干的事情）：
    L0: 0 1 0 0 0 0 0 0
    L1: 10  00  00  00
    L2: 1000    1000 (目前只是向下扩散，不改变上级分配情况)

    随后向上扩散，尝试合并（第二个switch干的事情）：
    L0: 0 1 0 0 0 0 0 0
    L1: 10  00  00  00
    L2: 1000    0000 (因为他对应的L1 的两个块，即第三个和第四个 00 都为，也就是4、7位均0,更新4位为0)
    */

    size_t group_idx = index >> 6;
    size_t bit_pos = index & 0x3F;

    // 高位向低位扩散分配信息
    uint64_t mask = order_wide[order];
    switch (order) // switch-case fallthrough (类 Duff's Device)
    {
    case 6:
        free_bitmap_[6].level1[group_idx] &= ~((mask & order_mask[6]) << bit_pos);
    case 5:
        free_bitmap_[5].level1[group_idx] &= ~((mask & order_mask[5]) << bit_pos);
    case 4:
        free_bitmap_[4].level1[group_idx] &= ~((mask & order_mask[4]) << bit_pos);
    case 3:
        free_bitmap_[3].level1[group_idx] &= ~((mask & order_mask[3]) << bit_pos);
    case 2:
        free_bitmap_[2].level1[group_idx] &= ~((mask & order_mask[2]) << bit_pos);
    case 1:
        free_bitmap_[1].level1[group_idx] &= ~((mask & order_mask[1]) << bit_pos);
    case 0:
        free_bitmap_[0].level1[group_idx] &= ~((mask & order_mask[0]) << bit_pos);
        break;
    }

    // 由低位向高位扩散合并信息，只要低位有一位占用，那么高位就无法分配
    switch (order)
    {
    case 0:
        free_bitmap_[1].level1[group_idx] =
            (free_bitmap_[0].level1[group_idx] & order_mask[1]) |
            ((free_bitmap_[0].level1[group_idx] & (order_mask[1] << order_shift[0])) >> order_shift[0]);
    case 1:
        free_bitmap_[2].level1[group_idx] =
            (free_bitmap_[1].level1[group_idx] & order_mask[2]) |
            ((free_bitmap_[1].level1[group_idx] & (order_mask[2] << order_shift[1])) >> order_shift[1]);
    case 2:
        free_bitmap_[3].level1[group_idx] =
            (free_bitmap_[2].level1[group_idx] & order_mask[3]) |
            ((free_bitmap_[2].level1[group_idx] & (order_mask[3] << order_shift[2])) >> order_shift[2]);
    case 3:
        free_bitmap_[4].level1[group_idx] =
            (free_bitmap_[3].level1[group_idx] & order_mask[4]) |
            ((free_bitmap_[3].level1[group_idx] & (order_mask[4] << order_shift[3])) >> order_shift[3]);
    case 4:
        free_bitmap_[5].level1[group_idx] =
            (free_bitmap_[4].level1[group_idx] & order_mask[5]) |
            ((free_bitmap_[4].level1[group_idx] & (order_mask[5] << order_shift[4])) >> order_shift[4]);
    case 5:
        free_bitmap_[6].level1[group_idx] =
            (free_bitmap_[5].level1[group_idx] & order_mask[6]) |
            ((free_bitmap_[5].level1[group_idx] & (order_mask[6] << order_shift[5])) >> order_shift[5]);
    case 6: // 不需要执行任何合并操作
        break;
    }

    // 更新level0：该组不再是满的
    free_bitmap_[0].level0 &= ~(((free_bitmap_[0].level1[group_idx] & order_mask[0]) != order_mask[0]) ? (1ULL << group_idx) : 0);
    free_bitmap_[1].level0 &= ~(((free_bitmap_[1].level1[group_idx] & order_mask[1]) != order_mask[1]) ? (1ULL << group_idx) : 0);
    free_bitmap_[2].level0 &= ~(((free_bitmap_[2].level1[group_idx] & order_mask[2]) != order_mask[2]) ? (1ULL << group_idx) : 0);
    free_bitmap_[3].level0 &= ~(((free_bitmap_[3].level1[group_idx] & order_mask[3]) != order_mask[3]) ? (1ULL << group_idx) : 0);
    free_bitmap_[4].level0 &= ~(((free_bitmap_[4].level1[group_idx] & order_mask[4]) != order_mask[4]) ? (1ULL << group_idx) : 0);
    free_bitmap_[5].level0 &= ~(((free_bitmap_[5].level1[group_idx] & order_mask[5]) != order_mask[5]) ? (1ULL << group_idx) : 0);
    free_bitmap_[6].level0 &= ~(((free_bitmap_[6].level1[group_idx] & order_mask[6]) != order_mask[5]) ? (1ULL << group_idx) : 0);
}

FORCE_INLINE void BuddyAllocator::set_allocated(size_t index, int order)
{
    /*
    分配前我的分配标记就应该是（演示采用三级位图）：
    L0: 0 1 0 0 0 0 0 0
    L1: 10  00  00  00
    L2: 1000    0000

    如果我要分配一个8K大小的块（order=1, bit_pos=4）：

    第一步：向下扩散设置标记（第一个switch干的事情）：
    L0: 0 1 0 0 1 1 0 0 (设置bit4, bit5，标记底层4KiB块被占用)
    L1: 10  00  10  00 (设置bit4，标记8KiB块被占用)
    L2: 1000    1000 (设置bit4，防止更大块分配到此位置)

    第二步：向上传播合并信息（第二个switch干的事情）：
    L0: 0 1 0 0 1 1 0 0 (不变)
    L1: 10  00  10  00 (不变)
    L2: 1000    1000 (基于L1重新计算：bit4=1是因为L1的bit4或bit5有占用)

    最终结果：完成分配标记，所有层级都正确反映了占用情况
    */

    size_t group_idx = index >> 6;
    size_t bit_pos = index & 0x3F;

    // 高位向低位扩散分配信息
    uint64_t mask = order_wide[order];
    switch (order) // switch-case fallthrough (类 Duff's Device)
    {
    case 6:
        free_bitmap_[6].level1[group_idx] |= ((mask & order_mask[6]) << bit_pos);
    case 5:
        free_bitmap_[5].level1[group_idx] |= ((mask & order_mask[5]) << bit_pos);
    case 4:
        free_bitmap_[4].level1[group_idx] |= ((mask & order_mask[4]) << bit_pos);
    case 3:
        free_bitmap_[3].level1[group_idx] |= ((mask & order_mask[3]) << bit_pos);
    case 2:
        free_bitmap_[2].level1[group_idx] |= ((mask & order_mask[2]) << bit_pos);
    case 1:
        free_bitmap_[1].level1[group_idx] |= ((mask & order_mask[1]) << bit_pos);
    case 0:
        free_bitmap_[0].level1[group_idx] |= ((mask & order_mask[0]) << bit_pos);
        break;
    }

    // 由低位向高位扩散合并信息，只要低位有一位占用，那么高位就无法分配
    switch (order)
    {
    case 0:
        free_bitmap_[1].level1[group_idx] =
            (free_bitmap_[0].level1[group_idx] & order_mask[1]) |
            ((free_bitmap_[0].level1[group_idx] & (order_mask[1] << order_shift[0])) >> order_shift[0]);
    case 1:
        free_bitmap_[2].level1[group_idx] =
            (free_bitmap_[1].level1[group_idx] & order_mask[2]) |
            ((free_bitmap_[1].level1[group_idx] & (order_mask[2] << order_shift[1])) >> order_shift[1]);
    case 2:
        free_bitmap_[3].level1[group_idx] =
            (free_bitmap_[2].level1[group_idx] & order_mask[3]) |
            ((free_bitmap_[2].level1[group_idx] & (order_mask[3] << order_shift[2])) >> order_shift[2]);
    case 3:
        free_bitmap_[4].level1[group_idx] =
            (free_bitmap_[3].level1[group_idx] & order_mask[4]) |
            ((free_bitmap_[3].level1[group_idx] & (order_mask[4] << order_shift[3])) >> order_shift[3]);
    case 4:
        free_bitmap_[5].level1[group_idx] =
            (free_bitmap_[4].level1[group_idx] & order_mask[5]) |
            ((free_bitmap_[4].level1[group_idx] & (order_mask[5] << order_shift[4])) >> order_shift[4]);
    case 5:
        free_bitmap_[6].level1[group_idx] =
            (free_bitmap_[5].level1[group_idx] & order_mask[6]) |
            ((free_bitmap_[5].level1[group_idx] & (order_mask[6] << order_shift[5])) >> order_shift[5]);
    case 6: // 不需要执行任何合并操作
        break;
    }

    // 更新level0：该组可能变满
    free_bitmap_[0].level0 |= (((free_bitmap_[0].level1[group_idx] & order_mask[0]) == order_mask[0]) ? (1ULL << group_idx) : 0);
    free_bitmap_[1].level0 |= (((free_bitmap_[1].level1[group_idx] & order_mask[1]) == order_mask[1]) ? (1ULL << group_idx) : 0);
    free_bitmap_[2].level0 |= (((free_bitmap_[2].level1[group_idx] & order_mask[2]) == order_mask[2]) ? (1ULL << group_idx) : 0);
    free_bitmap_[3].level0 |= (((free_bitmap_[3].level1[group_idx] & order_mask[3]) == order_mask[3]) ? (1ULL << group_idx) : 0);
    free_bitmap_[4].level0 |= (((free_bitmap_[4].level1[group_idx] & order_mask[4]) == order_mask[4]) ? (1ULL << group_idx) : 0);
    free_bitmap_[5].level0 |= (((free_bitmap_[5].level1[group_idx] & order_mask[5]) == order_mask[5]) ? (1ULL << group_idx) : 0);
    free_bitmap_[6].level0 |= (((free_bitmap_[6].level1[group_idx] & order_mask[6]) == order_mask[6]) ? (1ULL << group_idx) : 0);
}

FORCE_INLINE size_t BuddyAllocator::find_free_block(int order) const
{
    // 二级位图查找：O(1)
    uint64_t available_groups = ~free_bitmap_[order].level0;
    if (available_groups == 0)
    {
        return static_cast<size_t>(-1);
    }

    size_t group_idx = __builtin_ctzll(available_groups);
    uint64_t free_blocks = ~(free_bitmap_[order].level1[group_idx] | (~order_mask[order]));

    if (free_blocks == 0)
    {
        return static_cast<size_t>(-1);
    }

    size_t block_idx_in_group = __builtin_ctzll(free_blocks);
    return (group_idx << 6) | block_idx_in_group;
}

#endif // BUDDY_ALLOCATOR_H
