#include <gtest/gtest.h>
#include "../../cache-Kernel/ralloc/ralloc.h"
#include <chrono>
#include <cstring>

// Expected slab sizes (from ralloc.h)
constexpr size_t SLAB_SIZES[] = {
    16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096
};
constexpr size_t SLAB_COUNT = 14;

// Test fixture for lookup table tests
class LookupTableTest : public ::testing::Test
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

// Test all sizes from 1 to 4096
TEST_F(LookupTableTest, ComprehensiveMappingTest)
{
    // Test that all sizes from 1 to 4096 can be successfully allocated
    // We cannot directly access the slab index from metadata, so we verify
    // that allocation succeeds and memory is usable
    for (size_t size = 1; size <= 4096; ++size)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate size " << size;

        // Verify memory is usable by writing and reading
        if (size > 0)
        {
            uint8_t test_val = static_cast<uint8_t>(size & 0xFF);
            memset(ptr, test_val, size);
            EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], test_val)
                << "Memory corruption at size " << size;
            if (size > 1)
                EXPECT_EQ(static_cast<uint8_t *>(ptr)[size - 1], test_val)
                    << "Memory corruption at size " << size;
        }

        allocator->deallocate(ptr);
    }
}

// Test boundary cases
TEST_F(LookupTableTest, BoundaryCases)
{
    // Test boundary sizes to ensure correct allocation
    size_t boundary_sizes[] = {1, 16, 17, 32, 33, 64, 65, 128, 129,
                               192, 193, 256, 257, 384, 385, 512, 513,
                               768, 769, 1024, 1025, 1536, 1537, 2048,
                               2049, 3072, 3073, 4096};

    for (size_t size : boundary_sizes)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate size " << size;

        // Verify memory is usable
        uint8_t test_val = static_cast<uint8_t>(size & 0xFF);
        memset(ptr, test_val, size);
        EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], test_val)
            << "Memory corruption at boundary size " << size;
        EXPECT_EQ(static_cast<uint8_t *>(ptr)[size - 1], test_val)
            << "Memory corruption at boundary size " << size;

        allocator->deallocate(ptr);
    }
}

// Test compact table format
TEST_F(LookupTableTest, CompactTableFormat)
{
    // Verify that consecutive sizes are handled correctly
    // Test allocation success for a range of consecutive sizes
    for (size_t size = 1; size <= 4080; size += 16)
    {
        void *ptr1 = allocator->allocate(size);
        void *ptr2 = allocator->allocate(size + 1);

        ASSERT_NE(ptr1, nullptr) << "Failed to allocate size " << size;
        ASSERT_NE(ptr2, nullptr) << "Failed to allocate size " << (size + 1);

        // Verify memory is usable
        uint8_t test_val1 = static_cast<uint8_t>(size & 0xFF);
        uint8_t test_val2 = static_cast<uint8_t>((size + 1) & 0xFF);

        if (size > 0)
            memset(ptr1, test_val1, size);
        if (size + 1 > 0)
            memset(ptr2, test_val2, size + 1);

        allocator->deallocate(ptr1);
        allocator->deallocate(ptr2);
    }
}

// Performance test
TEST_F(LookupTableTest, PerformanceTest)
{
    const int ITERATIONS = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; ++i)
    {
        size_t size = (i % 4000) + 1;
        void *ptr = allocator->allocate(size);
        allocator->deallocate(ptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double ops_per_sec = (ITERATIONS * 2.0) / (duration.count() / 1000000.0);

    // Just log the result, no assertion needed
    std::cout << "  " << ITERATIONS << " alloc/free pairs: "
              << duration.count() << " μs (~"
              << (int)(ops_per_sec / 1000000.0) << "M ops/sec)" << std::endl;

    // Basic sanity check: should be reasonably fast (>10M ops/sec)
    EXPECT_GT(ops_per_sec, 10000000.0)
        << "Performance seems too slow: " << (ops_per_sec / 1000000.0) << " M ops/sec";
}
