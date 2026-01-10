#include <gtest/gtest.h>
#include "../../cache-Kernel/ralloc/ralloc.h"
#include <vector>
#include <cstring>
#include <random>
#include <algorithm>

// Test fixture for RAlloc tests
class RAllocTest : public ::testing::Test
{
protected:
    RAlloc *allocator;

    void SetUp() override
    {
        allocator = new RAlloc();
        ASSERT_TRUE(allocator->init()) << "Failed to initialize RAlloc";
    }

    void TearDown() override
    {
        delete allocator;
    }
};

// Basic allocation tests
TEST_F(RAllocTest, BasicSmallAllocation)
{
    void *ptr = allocator->allocate(128);
    ASSERT_NE(ptr, nullptr);

    // Write and verify data
    memset(ptr, 0xAB, 128);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], 0xAB);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[127], 0xAB);

    allocator->deallocate(ptr);
}

TEST_F(RAllocTest, BasicMediumAllocation)
{
    void *ptr = allocator->allocate(8192);
    ASSERT_NE(ptr, nullptr);

    // Verify 4KiB alignment for buddy allocations
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % (4 * 1024), 0);

    memset(ptr, 0xCD, 8192);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], 0xCD);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[8191], 0xCD);

    allocator->deallocate(ptr);
}

TEST_F(RAllocTest, BasicLargeAllocation)
{
    void *ptr = allocator->allocate(512 * 1024);
    ASSERT_NE(ptr, nullptr);

    memset(ptr, 0xEF, 512 * 1024);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], 0xEF);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[512 * 1024 - 1], 0xEF);

    allocator->deallocate(ptr);
}

// Test all slab size classes
TEST_F(RAllocTest, SlabSizeClasses)
{
    size_t slab_sizes[] = {16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072};
    std::vector<void *> ptrs;

    for (size_t size : slab_sizes)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate " << size << " bytes";
        memset(ptr, static_cast<int>(size & 0xFF), size);
        ptrs.push_back(ptr);
    }

    // Free all allocations
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

// Test buddy system orders
TEST_F(RAllocTest, BuddyOrders)
{
    size_t buddy_sizes[] = {4096, 8192, 16384, 32768, 65536, 131072, 262144};
    std::vector<void *> ptrs;

    for (size_t size : buddy_sizes)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate " << size << " bytes";

        // Verify alignment
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % (4 * 1024), 0)
            << "Buddy allocation not 4KiB aligned for size " << size;

        memset(ptr, static_cast<int>(size & 0xFF), size);
        ptrs.push_back(ptr);
    }

    // Free all allocations
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

// Test mixed allocations
TEST_F(RAllocTest, MixedAllocations)
{
    std::vector<std::pair<void *, size_t>> allocations;
    size_t sizes[] = {64, 4096, 500000, 128, 16384, 2000000, 1024, 131072, 800000};

    for (size_t size : sizes)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate " << size << " bytes";
        memset(ptr, static_cast<int>(size & 0xFF), size);
        allocations.push_back({ptr, size});
    }

    // Free in reverse order
    for (auto it = allocations.rbegin(); it != allocations.rend(); ++it)
    {
        allocator->deallocate(it->first);
    }
}

// Test edge cases
TEST_F(RAllocTest, ZeroSizeAllocation)
{
    void *ptr = allocator->allocate(0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(RAllocTest, NullPointerFree)
{
    EXPECT_NO_THROW(allocator->deallocate(nullptr));
}

TEST_F(RAllocTest, BelowSlabMinimum)
{
    // Below 16B should use malloc
    void *ptr = allocator->allocate(8);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0x42, 8);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], 0x42);
    allocator->deallocate(ptr);
}

TEST_F(RAllocTest, BoundarySize16B)
{
    void *ptr = allocator->allocate(16);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0x11, 16);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[15], 0x11);
    allocator->deallocate(ptr);
}

TEST_F(RAllocTest, BoundarySize4KiB)
{
    void *ptr = allocator->allocate(4096);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % (4 * 1024), 0);
    memset(ptr, 0x44, 4096);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[4095], 0x44);
    allocator->deallocate(ptr);
}

TEST_F(RAllocTest, BoundarySize256KiB)
{
    void *ptr = allocator->allocate(262144);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0x55, 262144);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[262143], 0x55);
    allocator->deallocate(ptr);
}

TEST_F(RAllocTest, AboveBuddyMaximum)
{
    void *ptr = allocator->allocate(300 * 1024);
    ASSERT_NE(ptr, nullptr);
    memset(ptr, 0x66, 300 * 1024);
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[300 * 1024 - 1], 0x66);
    allocator->deallocate(ptr);
}

// Test statistics
TEST_F(RAllocTest, Statistics)
{
    size_t initial_slab_count = allocator->get_slab_count();
    size_t initial_buddy_count = allocator->get_buddy_count();

    void *ptr1 = allocator->allocate(256);
    void *ptr2 = allocator->allocate(8192);

    // Should have created at least one slab and one buddy allocator
    EXPECT_GT(allocator->get_slab_count(), initial_slab_count);
    EXPECT_GT(allocator->get_buddy_count(), initial_buddy_count);

    allocator->deallocate(ptr1);
    allocator->deallocate(ptr2);
}

TEST_F(RAllocTest, FreeMemory)
{
    // Allocate and track free memory
    void *ptr1 = allocator->allocate(1024);
    ASSERT_NE(ptr1, nullptr);

    size_t free_mem = allocator->get_total_free_memory();
    EXPECT_GT(free_mem, 0);

    allocator->deallocate(ptr1);
}

// Stress test with multiple allocations
TEST_F(RAllocTest, StressTestMultipleAllocations)
{
    const int num_allocs = 100;
    std::vector<void *> ptrs;

    // Allocate
    for (int i = 0; i < num_allocs; ++i)
    {
        size_t size = 64 + (i % 10) * 64; // Sizes from 64 to 640
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed at allocation " << i;
        ptrs.push_back(ptr);
    }

    // Free
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

TEST_F(RAllocTest, StressTestRandomSizes)
{
    const int num_allocs = 100;
    std::vector<void *> ptrs;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(16, 10000);

    // Allocate random sizes
    for (int i = 0; i < num_allocs; ++i)
    {
        size_t size = size_dist(gen);
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed at allocation " << i << " size " << size;
        ptrs.push_back(ptr);
    }

    // Shuffle and free in random order
    std::shuffle(ptrs.begin(), ptrs.end(), gen);
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

// Test global interface
TEST(RAllocGlobalTest, GlobalInterface)
{
    void *ptr1 = ralloc_malloc(128);
    void *ptr2 = ralloc_malloc(8192);
    void *ptr3 = ralloc_malloc(1048576);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // Write and verify
    memset(ptr1, 0xAA, 128);
    memset(ptr2, 0xBB, 8192);
    memset(ptr3, 0xCC, 1048576);

    EXPECT_EQ(static_cast<uint8_t *>(ptr1)[0], 0xAA);
    EXPECT_EQ(static_cast<uint8_t *>(ptr2)[0], 0xBB);
    EXPECT_EQ(static_cast<uint8_t *>(ptr3)[0], 0xCC);

    ralloc_free(ptr1);
    ralloc_free(ptr2);
    ralloc_free(ptr3);
}

TEST(RAllocGlobalTest, GlobalInterfaceNullFree)
{
    EXPECT_NO_THROW(ralloc_free(nullptr));
}

// Test allocation patterns
TEST_F(RAllocTest, AllocateFreePattern)
{
    // Allocate-free-allocate pattern (common in caching)
    for (int i = 0; i < 10; ++i)
    {
        void *ptr = allocator->allocate(256);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, 0x77, 256);
        allocator->deallocate(ptr);
    }
}

TEST_F(RAllocTest, MultipleSlabAllocators)
{
    // Force creation of multiple slab allocators by filling one
    std::vector<void *> ptrs;

    // Allocate many blocks of the same size to potentially fill one allocator
    for (int i = 0; i < 1000; ++i)
    {
        void *ptr = allocator->allocate(64);
        if (ptr)
        {
            ptrs.push_back(ptr);
        }
    }

    // Free all
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }

    EXPECT_GT(ptrs.size(), 0);
}

TEST_F(RAllocTest, MultipleBuddyAllocators)
{
    // Allocate large amounts to potentially create multiple buddy allocators
    std::vector<void *> ptrs;

    for (int i = 0; i < 10; ++i)
    {
        void *ptr = allocator->allocate(128 * 1024);
        if (ptr)
        {
            ptrs.push_back(ptr);
        }
    }

    size_t buddy_count = allocator->get_buddy_count();
    EXPECT_GT(buddy_count, 0);

    // Free all
    for (void *ptr : ptrs)
    {
        allocator->deallocate(ptr);
    }
}

// Test data integrity
TEST_F(RAllocTest, DataIntegrity)
{
    const size_t size = 4096;
    void *ptr = allocator->allocate(size);
    ASSERT_NE(ptr, nullptr);

    // Write pattern
    uint8_t *data = static_cast<uint8_t *>(ptr);
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Verify pattern
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i], static_cast<uint8_t>(i & 0xFF))
            << "Data corruption at offset " << i;
    }

    allocator->deallocate(ptr);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
