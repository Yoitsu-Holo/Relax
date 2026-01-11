#include <gtest/gtest.h>
#include "../../cache-Kernel/ralloc/ralloc.h"
#include "../../cache-Kernel/slab/slab_allocator.h"
#include <chrono>

// Expected slab sizes (from ralloc.h)
constexpr size_t SLAB_SIZES[] = {
    16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096
};
constexpr size_t SLAB_COUNT = 14;

// Helper to find expected slab index
uint8_t find_expected_slab_index(size_t size)
{
    for (uint8_t i = 0; i < SLAB_COUNT; ++i)
    {
        if (size <= SLAB_SIZES[i])
            return i;
    }
    return SLAB_COUNT - 1;
}

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
    for (size_t size = 1; size <= 4096; ++size)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate size " << size;

        // Get metadata from the allocated block
        uint64_t metadata = SlabAllocator::get_metadata(ptr);
        uint8_t actual_slab_idx = static_cast<uint8_t>(metadata & 0xFF);

        // Calculate expected slab index
        uint8_t expected_slab_idx = find_expected_slab_index(size);

        EXPECT_EQ(actual_slab_idx, expected_slab_idx)
            << "Size " << size << " mapped to wrong slab. Expected slab "
            << (int)expected_slab_idx << " (" << SLAB_SIZES[expected_slab_idx]
            << "B), got slab " << (int)actual_slab_idx
            << " (" << SLAB_SIZES[actual_slab_idx] << "B)";

        allocator->deallocate(ptr);
    }
}

// Test boundary cases
TEST_F(LookupTableTest, BoundaryCases)
{
    size_t boundary_sizes[] = {1, 16, 17, 32, 33, 64, 65, 128, 129,
                               192, 193, 256, 257, 384, 385, 512, 513,
                               768, 769, 1024, 1025, 1536, 1537, 2048,
                               2049, 3072, 3073, 4096};

    for (size_t size : boundary_sizes)
    {
        void *ptr = allocator->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate size " << size;

        uint64_t metadata = SlabAllocator::get_metadata(ptr);
        uint8_t actual_slab_idx = static_cast<uint8_t>(metadata & 0xFF);
        uint8_t expected_slab_idx = find_expected_slab_index(size);

        EXPECT_EQ(actual_slab_idx, expected_slab_idx)
            << "Boundary size " << size << " mapped incorrectly";

        allocator->deallocate(ptr);
    }
}

// Test compact table format
TEST_F(LookupTableTest, CompactTableFormat)
{
    // Verify that consecutive sizes use reasonable slab indices
    for (size_t size = 1; size <= 4080; size += 16)
    {
        void *ptr1 = allocator->allocate(size);
        void *ptr2 = allocator->allocate(size + 1);

        ASSERT_NE(ptr1, nullptr);
        ASSERT_NE(ptr2, nullptr);

        uint64_t meta1 = SlabAllocator::get_metadata(ptr1);
        uint64_t meta2 = SlabAllocator::get_metadata(ptr2);

        uint8_t idx1 = static_cast<uint8_t>(meta1 & 0xFF);
        uint8_t idx2 = static_cast<uint8_t>(meta2 & 0xFF);

        // Indices should be valid
        ASSERT_LT(idx1, SLAB_COUNT);
        ASSERT_LT(idx2, SLAB_COUNT);

        // idx2 should be >= idx1 (size+1 needs same or larger slab)
        EXPECT_GE(idx2, idx1)
            << "Size " << (size+1) << " needs smaller slab than size " << size;

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
