#include <gtest/gtest.h>
#include "../../cache-Kernel/slab/slab_allocator.h"
#include <vector>
#include <set>
#include <random>
#include <chrono>
#include <thread>

// Test fixture for SlabAllocator tests
class SlabAllocatorTest : public ::testing::Test
{
protected:
    slab *test_slab;
    SlabAllocator *allocator;

    void SetUp() override
    {
        test_slab = new slab;
        allocator = new SlabAllocator(test_slab);
    }

    void TearDown() override
    {
        delete allocator;
        delete test_slab;
    }
};

// 测试参数化的测试夹具，用于测试不同的块大小
class SlabAllocatorParamTest : public ::testing::TestWithParam<int>
{
protected:
    slab *test_slab;
    SlabAllocator *allocator;

    void SetUp() override
    {
        test_slab = new slab;
        allocator = new SlabAllocator(test_slab);
    }

    void TearDown() override
    {
        delete allocator;
        delete test_slab;
    }
};

// ============= 基础功能测试 =============

TEST_F(SlabAllocatorTest, InitializationWithValidSizes)
{
    // 根据新规则：<64的块必须是16的倍数，>=64的块必须是64的倍数
    std::vector<int> valid_sizes = {16, 32, 48, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856, 1920, 1984, 2048, 2112, 2176, 2240, 2304, 2368, 2432, 2496, 2560, 2624, 2688, 2752, 2816, 2880, 2944, 3008, 3072, 3136, 3200, 3264, 3328, 3392, 3456, 3520, 3584, 3648, 3712, 3776, 3840, 3904, 3968, 4032, 4096};

    for (int size : valid_sizes)
    {
        EXPECT_EQ(allocator->init(size), 0) << "Failed to initialize with block size: " << size;
        EXPECT_EQ(allocator->get_block_size(), size);
        EXPECT_GT(allocator->get_total_blocks(), 0);
        EXPECT_EQ(allocator->get_free_blocks(), allocator->get_total_blocks());
        EXPECT_TRUE(allocator->is_empty());
        EXPECT_FALSE(allocator->is_full());
    }
}

TEST_F(SlabAllocatorTest, InitializationWithInvalidSizes)
{
    // 无效的块大小：不是16或64的倍数，超出范围等
    std::vector<int> invalid_sizes = {0, -1, 15, 17, 31, 33, 49, 50, 63, 65, 100, 127, 129, 191, 193, 255, 257, 300, 5000, 10000};

    for (int size : invalid_sizes)
    {
        EXPECT_EQ(allocator->init(size), -1) << "Should have failed with invalid block size: " << size;
    }
}

TEST_F(SlabAllocatorTest, StaticValidBlockSizeCheck)
{
    // 测试有效的块大小
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(16));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(32));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(48));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(64));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(128));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(256));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(512));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(1024));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(2048));
    EXPECT_TRUE(SlabAllocator::is_valid_block_size(4096));

    // 测试无效的块大小
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(0));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(-1));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(15));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(17));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(33));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(49));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(63));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(65));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(100));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(127));
    EXPECT_FALSE(SlabAllocator::is_valid_block_size(5000));
}

// ============= 分配和释放测试 =============

TEST_P(SlabAllocatorParamTest, BasicAllocationAndDeallocation)
{
    int block_size = GetParam();
    ASSERT_EQ(allocator->init(block_size), 0);

    size_t total_blocks = allocator->get_total_blocks();

    // 分配一个块
    void *ptr = allocator->allocate();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(allocator->get_free_blocks(), total_blocks - 1);
    EXPECT_FALSE(allocator->is_empty());

    // 释放块
    allocator->deallocate(ptr);
    EXPECT_EQ(allocator->get_free_blocks(), total_blocks);
    EXPECT_TRUE(allocator->is_empty());
}

TEST_P(SlabAllocatorParamTest, AllocateUntilFull)
{
    int block_size = GetParam();
    ASSERT_EQ(allocator->init(block_size), 0);

    size_t total_blocks = allocator->get_total_blocks();
    std::vector<void *> ptrs;

    // 分配所有块
    for (size_t i = 0; i < total_blocks; ++i)
    {
        void *ptr = allocator->allocate();
        ASSERT_NE(ptr, nullptr) << "Allocation failed at block " << i;
        ptrs.push_back(ptr);
    }

    EXPECT_EQ(allocator->get_free_blocks(), 0);
    EXPECT_TRUE(allocator->is_full());
    EXPECT_FALSE(allocator->is_empty());

    // 尝试再分配一个块，应该失败
    void *ptr = allocator->allocate();
    EXPECT_EQ(ptr, nullptr);

    // 释放所有块
    for (void *p : ptrs)
    {
        allocator->deallocate(p);
    }

    EXPECT_EQ(allocator->get_free_blocks(), total_blocks);
    EXPECT_TRUE(allocator->is_empty());
    EXPECT_FALSE(allocator->is_full());
}

TEST_P(SlabAllocatorParamTest, DataIntegrity)
{
    int block_size = GetParam();
    ASSERT_EQ(allocator->init(block_size), 0);

    std::vector<void *> ptrs;
    std::vector<int> values;

    // 分配100个块并写入数据
    for (int i = 0; i < 100 && i < allocator->get_total_blocks(); ++i)
    {
        void *ptr = allocator->allocate();
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
        values.push_back(i * 1000 + block_size);

        // 写入数据
        if (block_size >= sizeof(int))
        {
            *reinterpret_cast<int *>(ptr) = values.back();
        }
    }

    // 验证数据
    for (size_t i = 0; i < ptrs.size(); ++i)
    {
        if (block_size >= sizeof(int))
        {
            EXPECT_EQ(*reinterpret_cast<int *>(ptrs[i]), values[i])
                << "Data corruption at block " << i;
        }
    }

    // 释放所有块
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

TEST_P(SlabAllocatorParamTest, AddressUniqueness)
{
    int block_size = GetParam();
    ASSERT_EQ(allocator->init(block_size), 0);

    std::set<void *> addresses;
    std::vector<void *> ptrs;

    // 分配多个块并检查地址唯一性
    size_t alloc_count = std::min(size_t(100), allocator->get_total_blocks());
    for (size_t i = 0; i < alloc_count; ++i)
    {
        void *ptr = allocator->allocate();
        ASSERT_NE(ptr, nullptr);
        EXPECT_TRUE(addresses.insert(ptr).second)
            << "Duplicate address returned: " << ptr;
        ptrs.push_back(ptr);
    }

    // 释放所有块
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

// ============= 边界条件测试 =============

TEST_F(SlabAllocatorTest, DeallocateNullPointer)
{
    ASSERT_EQ(allocator->init(64), 0);

    // 释放空指针应该安全返回
    EXPECT_NO_THROW(allocator->deallocate(nullptr));
}

TEST_F(SlabAllocatorTest, DoubleFree)
{
    ASSERT_EQ(allocator->init(64), 0);

    void *ptr = allocator->allocate();
    ASSERT_NE(ptr, nullptr);

    // 双重释放（设计上允许，为了性能）
    EXPECT_NO_THROW(allocator->deallocate(ptr));
    EXPECT_NO_THROW(allocator->deallocate(ptr));

    // 注意：双重释放后的行为是未定义的，这里只是测试不会崩溃
}

TEST_F(SlabAllocatorTest, FragmentationAndReuse)
{
    ASSERT_EQ(allocator->init(128), 0);

    std::vector<void *> ptrs;
    size_t alloc_count = std::min(size_t(50), allocator->get_total_blocks());

    // 分配一批块
    for (size_t i = 0; i < alloc_count; ++i)
    {
        ptrs.push_back(allocator->allocate());
    }

    // 释放奇数索引的块（创建碎片）
    for (size_t i = 1; i < ptrs.size(); i += 2)
    {
        allocator->deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }

    // 重新分配，应该能重用已释放的块
    for (size_t i = 1; i < ptrs.size(); i += 2)
    {
        void *new_ptr = allocator->allocate();
        ASSERT_NE(new_ptr, nullptr);
        ptrs[i] = new_ptr;
    }

    // 释放所有块
    for (void *ptr : ptrs)
    {
        if (ptr)
            allocator->deallocate(ptr);
    }
}

// ============= 性能测试 =============

TEST_P(SlabAllocatorParamTest, AllocationPerformance)
{
    int block_size = GetParam();
    ASSERT_EQ(allocator->init(block_size), 0);

    const int iterations = 10000;
    const int batch_size = std::min(100, static_cast<int>(allocator->get_total_blocks()));
    std::vector<void *> ptrs(batch_size);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        // 分配
        for (int j = 0; j < batch_size; ++j)
        {
            ptrs[j] = allocator->allocate();
        }
        // 释放
        for (int j = 0; j < batch_size; ++j)
        {
            allocator->deallocate(ptrs[j]);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double ops_per_second = (2.0 * iterations * batch_size * 1000000) / duration;

    // 输出性能信息
    RecordProperty("block_size", block_size);
    RecordProperty("operations_per_second", ops_per_second);
    RecordProperty("duration_us", duration);

    std::cout << "Block size " << block_size << ": "
              << ops_per_second << " ops/sec" << std::endl;
}

// ============= 使用率测试 =============

TEST_F(SlabAllocatorTest, UsageCalculation)
{
    ASSERT_EQ(allocator->init(256), 0);

    size_t total_blocks = allocator->get_total_blocks();
    EXPECT_DOUBLE_EQ(allocator->get_usage(), 0.0);

    std::vector<void *> ptrs;

    // 分配25%的块
    size_t quarter = total_blocks / 4;
    for (size_t i = 0; i < quarter; ++i)
    {
        ptrs.push_back(allocator->allocate());
    }
    EXPECT_NEAR(allocator->get_usage(), 25.0, 5.0);

    // 分配到50%
    for (size_t i = 0; i < quarter; ++i)
    {
        ptrs.push_back(allocator->allocate());
    }
    EXPECT_NEAR(allocator->get_usage(), 50.0, 5.0);

    // 分配到100%
    while (allocator->allocate() != nullptr)
    {
        // 继续分配直到满
    }
    EXPECT_DOUBLE_EQ(allocator->get_usage(), 100.0);

    // 释放所有
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

// ============= 随机化测试 =============

TEST_F(SlabAllocatorTest, RandomOperations)
{
    ASSERT_EQ(allocator->init(64), 0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    std::vector<void *> allocated;
    const int operations = 10000;

    for (int i = 0; i < operations; ++i)
    {
        if (dis(gen) == 0 || allocated.empty())
        {
            // 分配
            void *ptr = allocator->allocate();
            if (ptr)
            {
                allocated.push_back(ptr);
            }
        }
        else
        {
            // 释放随机的块
            std::uniform_int_distribution<> idx_dis(0, allocated.size() - 1);
            int idx = idx_dis(gen);
            allocator->deallocate(allocated[idx]);
            allocated.erase(allocated.begin() + idx);
        }
    }

    // 清理
    for (void *ptr : allocated)
    {
        allocator->deallocate(ptr);
    }

    EXPECT_TRUE(allocator->is_empty());
}

// 实例化参数化测试
INSTANTIATE_TEST_SUITE_P(
    BlockSizes,
    SlabAllocatorParamTest,
    ::testing::Values(16, 32, 48, 64, 128, 192, 256, 320, 384, 448, 512, 640, 768, 896, 1024, 1280, 1536, 1792, 2048, 2560, 3072, 3584, 4096),
    [](const ::testing::TestParamInfo<int> &info)
    {
        return "BlockSize_" + std::to_string(info.param);
    });

// ============= 主函数 =============
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}