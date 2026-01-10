#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <gtest/gtest.h>
#include <cstdlib>
#include <vector>
#include <set>
#include <random>

// 辅助函数：将size转换为order
inline int size_to_order(size_t size)
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
    return -1;
}

class BuddyAllocatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // BuddyAllocator 包含 16MiB 内置数据，需要堆分配
        // 使用 16MiB 对齐确保 data 成员也对齐
        void *mem = std::aligned_alloc(16 * MiB, sizeof(BuddyAllocator));
        ASSERT_NE(mem, nullptr);
        buddy_ = new (mem) BuddyAllocator();
    }

    void TearDown() override
    {
        if (buddy_)
        {
            buddy_->~BuddyAllocator();
            std::free(buddy_);
            buddy_ = nullptr;
        }
    }

    BuddyAllocator *buddy_ = nullptr;
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(BuddyAllocatorTest, InitialState)
{
    // 初始状态应该有 64 个 256KiB 的空闲块
    EXPECT_EQ(buddy_->get_free_blocks(0), 4096);
    EXPECT_EQ(buddy_->get_free_blocks(1), 2048);
    EXPECT_EQ(buddy_->get_free_blocks(2), 1024);
    EXPECT_EQ(buddy_->get_free_blocks(3), 512);
    EXPECT_EQ(buddy_->get_free_blocks(4), 256);
    EXPECT_EQ(buddy_->get_free_blocks(5), 128);
    EXPECT_EQ(buddy_->get_free_blocks(6), 64);
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

TEST_F(BuddyAllocatorTest, SingleAllocation)
{
    void *ptr = buddy_->allocate(0); // order 0 = 4KiB
    ASSERT_NE(ptr, nullptr);

    // 验证指针属于这个 allocator
    EXPECT_TRUE(buddy_->owns_pointer(ptr));

    // 释放后应该完全恢复
    buddy_->deallocate(ptr);
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

TEST_F(BuddyAllocatorTest, AllocationAlignment)
{
    void *ptr = buddy_->allocate(0); // order 0 = 4KiB
    ASSERT_NE(ptr, nullptr);

    // 验证地址是 4KiB 对齐的
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % BUDDY_ALLOC_BLOCK_SIZE, 0);

    buddy_->deallocate(ptr);
}

TEST_F(BuddyAllocatorTest, MultipleAllocations)
{
    std::vector<void *> ptrs;
    int orders[] = {0, 1, 2, 3, 4, 5, 6}; // 4K, 8K, 16K, 32K, 64K, 128K, 256K

    // 分配各种大小的块
    for (int order : orders)
    {
        void *ptr = buddy_->allocate(order);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    size_t allocated = 0;
    for (int order : orders)
    {
        allocated += BUDDY_ALLOC_BLOCK_SIZE << order;
    }

    // 验证总空闲内存
    EXPECT_LE(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE - allocated);

    // 释放所有块
    for (void *ptr : ptrs)
    {
        buddy_->deallocate(ptr);
    }

    // 验证内存完全恢复
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 边界测试
// ============================================================================

TEST_F(BuddyAllocatorTest, InvalidOrderAllocation)
{
    void *ptr = buddy_->allocate(-1);
    EXPECT_EQ(ptr, nullptr);

    ptr = buddy_->allocate(7);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(BuddyAllocatorTest, NullDeallocation)
{
    // 释放nullptr应该不会崩溃
    EXPECT_NO_THROW(buddy_->deallocate(nullptr));
}

// ============================================================================
// 伙伴合并测试
// ============================================================================

TEST_F(BuddyAllocatorTest, BuddyMerging)
{
    // 分配两个 4KiB 块
    void *ptr1 = buddy_->allocate(0);
    void *ptr2 = buddy_->allocate(0);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    // 记录当前状态
    size_t before_free = buddy_->get_total_free_memory();

    // 释放第一个块
    buddy_->deallocate(ptr1);
    size_t after_first = buddy_->get_total_free_memory();
    EXPECT_EQ(after_first, before_free + 4096);

    // 释放第二个块，应该触发伙伴合并
    buddy_->deallocate(ptr2);

    // 验证内存完全恢复
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

TEST_F(BuddyAllocatorTest, MultipleMerges)
{
    // 分配多个小块
    std::vector<void *> ptrs;
    for (int i = 0; i < 100; ++i)
    {
        void *ptr = buddy_->allocate(0); // order 0 = 4KiB
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // 按顺序释放所有块
    for (void *ptr : ptrs)
    {
        buddy_->deallocate(ptr);
    }

    // 验证最终状态：应该完全合并回64个256KiB块
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 碎片化测试
// ============================================================================

TEST_F(BuddyAllocatorTest, Fragmentation)
{
    std::vector<void *> ptrs;

    // 分配100个 4KiB 块
    for (int i = 0; i < 100; ++i)
    {
        void *ptr = buddy_->allocate(0);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // 释放偶数索引的块
    for (size_t i = 0; i < ptrs.size(); i += 2)
    {
        buddy_->deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }

    // 验证部分内存可用
    EXPECT_GT(buddy_->get_total_free_memory(), 0);
    EXPECT_LT(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);

    // 释放剩余的块
    for (size_t i = 1; i < ptrs.size(); i += 2)
    {
        buddy_->deallocate(ptrs[i]);
    }

    // 验证内存完全恢复
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 内存耗尽测试
// ============================================================================

TEST_F(BuddyAllocatorTest, MemoryExhaustion)
{
    std::vector<void *> ptrs;

    // 分配所有 256KiB 块
    for (int i = 0; i < 64; ++i)
    {
        void *ptr = buddy_->allocate(6); // order 6 = 256KiB
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // 内存应该耗尽
    EXPECT_EQ(buddy_->get_total_free_memory(), 0);

    // 再次分配应该失败
    void *ptr = buddy_->allocate(0);
    EXPECT_EQ(ptr, nullptr);

    // 释放所有块
    for (void *p : ptrs)
    {
        buddy_->deallocate(p);
    }

    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 块分裂测试
// ============================================================================

TEST_F(BuddyAllocatorTest, BlockSplitting)
{
    // 初始状态：64 个 256KiB 块
    EXPECT_EQ(buddy_->get_free_blocks(6), 64);

    // 分配一个 4KiB 块，应该触发多次分裂
    void *ptr = buddy_->allocate(0);
    ASSERT_NE(ptr, nullptr);

    // 验证分裂后的状态：应该有一些中间阶的空闲块
    EXPECT_LT(buddy_->get_free_blocks(6), 64);

    buddy_->deallocate(ptr);
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 指针唯一性测试
// ============================================================================

TEST_F(BuddyAllocatorTest, UniquePointers)
{
    std::set<void *> ptr_set;
    std::vector<void *> ptrs;

    // 分配多个块
    for (int i = 0; i < 100; ++i)
    {
        void *ptr = buddy_->allocate(0);
        ASSERT_NE(ptr, nullptr);

        // 验证指针唯一性
        EXPECT_EQ(ptr_set.count(ptr), 0) << "Duplicate pointer detected!";
        ptr_set.insert(ptr);
        ptrs.push_back(ptr);
    }

    // 释放所有块
    for (void *ptr : ptrs)
    {
        buddy_->deallocate(ptr);
    }
}

// ============================================================================
// 混合大小测试
// ============================================================================

TEST_F(BuddyAllocatorTest, MixedSizes)
{
    std::vector<void *> allocations;
    std::random_device rd;
    std::mt19937 gen(42); // 固定种子以保证可重复性
    std::uniform_int_distribution<> order_dist(0, 6);

    // 执行随机分配
    for (int i = 0; i < 50; ++i)
    {
        int order = order_dist(gen);
        void *ptr = buddy_->allocate(order);
        if (ptr != nullptr)
        {
            allocations.push_back(ptr);
        }
    }

    EXPECT_GT(allocations.size(), 0);

    // 释放所有分配
    for (void *ptr : allocations)
    {
        buddy_->deallocate(ptr);
    }

    // 验证内存完全恢复
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 顺序分配释放测试
// ============================================================================

TEST_F(BuddyAllocatorTest, SequentialAllocDealloc)
{
    // 反复分配和释放
    for (int iter = 0; iter < 10; ++iter)
    {
        std::vector<void *> ptrs;

        // 分配
        for (int i = 0; i < 20; ++i)
        {
            void *ptr = buddy_->allocate(1); // order 1 = 8KiB
            ASSERT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        }

        // 释放
        for (void *ptr : ptrs)
        {
            buddy_->deallocate(ptr);
        }

        // 每次迭代后都应该完全恢复
        EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
    }
}

// ============================================================================
// 统计信息测试
// ============================================================================

TEST_F(BuddyAllocatorTest, Statistics)
{
    // 分配一些块
    void *ptr1 = buddy_->allocate(0); // Order 0 = 4KiB
    void *ptr2 = buddy_->allocate(1); // Order 1 = 8KiB
    void *ptr3 = buddy_->allocate(6); // Order 6 = 256KiB

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // 验证总空闲内存减少
    size_t allocated = 4096 + 8192 + 262144;
    EXPECT_LE(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE - allocated);

    // 释放
    buddy_->deallocate(ptr1);
    buddy_->deallocate(ptr2);
    buddy_->deallocate(ptr3);

    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 大量分配测试
// ============================================================================

TEST_F(BuddyAllocatorTest, LargeScale)
{
    std::vector<void *> ptrs;
    const int num_allocs = 1000;

    // 分配大量小块
    for (int i = 0; i < num_allocs; ++i)
    {
        void *ptr = buddy_->allocate(0);
        if (ptr != nullptr)
        {
            ptrs.push_back(ptr);
        }
    }

    EXPECT_GT(ptrs.size(), 0);
    std::cout << "Successfully allocated " << ptrs.size() << " blocks out of " << num_allocs << std::endl;

    // 释放所有块
    for (void *ptr : ptrs)
    {
        buddy_->deallocate(ptr);
    }

    // 验证内存完全恢复
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 各阶分配测试
// ============================================================================

TEST_F(BuddyAllocatorTest, AllOrderAllocations)
{
    std::vector<void *> allocations;

    // 为每个阶分配一个块
    for (int order = 0; order < BUDDY_ALLOC_NUM_ORDERS; ++order)
    {
        void *ptr = buddy_->allocate(order);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate for order " << order;
        allocations.push_back(ptr);
    }

    // 释放所有块
    for (void *ptr : allocations)
    {
        buddy_->deallocate(ptr);
    }

    // 验证内存完全恢复
    EXPECT_EQ(buddy_->get_total_free_memory(), BUDDY_ALLOC_TOTAL_SIZE);
}

// ============================================================================
// 能力查询测试
// ============================================================================

TEST_F(BuddyAllocatorTest, CapabilityQuery)
{
    // 初始状态：应该能分配所有order的块（因为有64个256KiB块）
    uint8_t cap = buddy_->get_capability();
    EXPECT_NE(cap & (1 << 6), 0) << "Should be able to allocate order 6";

    // 分配所有256KiB块
    std::vector<void *> ptrs;
    for (int i = 0; i < 64; ++i)
    {
        void *ptr = buddy_->allocate(6);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // 现在应该无法分配任何块
    cap = buddy_->get_capability();
    EXPECT_EQ(cap, 0) << "Should not be able to allocate any blocks";

    // 释放一个块
    buddy_->deallocate(ptrs[0]);
    ptrs[0] = nullptr;

    // 现在应该能分配order 6的块
    cap = buddy_->get_capability();
    EXPECT_NE(cap & (1 << 6), 0) << "Should be able to allocate order 6 again";

    // 释放所有块
    for (void *ptr : ptrs)
        if (ptr)
            buddy_->deallocate(ptr);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
