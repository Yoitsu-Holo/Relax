#include <gtest/gtest.h>
#include "../../cache-Kernel/kvEngine/kv_engine.h"
#include <vector>
#include <string>
#include <random>
#include <cstring>

// Test fixture for KVEngine tests
class KVEngineTest : public ::testing::Test
{
protected:
    KVEngine *engine;

    void SetUp() override
    {
        engine = new KVEngine();
        ASSERT_NE(engine, nullptr) << "Failed to create KVEngine";
        int ret = engine->init();
        ASSERT_EQ(ret, KV_OK) << "Failed to initialize KVEngine";
    }

    void TearDown() override
    {
        delete engine;
    }
};

// ============= Initialization Tests =============

TEST_F(KVEngineTest, InitializationSuccess)
{
    EXPECT_TRUE(engine->is_initialized());
    EXPECT_EQ(engine->get_size(), 0);
}

TEST_F(KVEngineTest, DoubleInitializationFails)
{
    // Try to initialize again
    int ret = engine->init();
    EXPECT_EQ(ret, KV_ERROR_INIT_FAILED);
}

// ============= Add Function Tests =============

TEST_F(KVEngineTest, AddBasic)
{
    const char *data = "Hello, World!";
    size_t len = strlen(data);
    uint64_t hash = 12345;

    int ret = engine->add(hash, data, len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(engine->get_size(), 1);
}

TEST_F(KVEngineTest, AddMultiple)
{
    std::vector<std::pair<uint64_t, std::string>> test_data = {
        {1, "data1"},
        {2, "data2"},
        {3, "data3"},
        {4, "data4"},
        {5, "data5"}};

    for (const auto &pair : test_data)
    {
        int ret = engine->add(pair.first, pair.second.c_str(), pair.second.length());
        EXPECT_EQ(ret, KV_OK);
    }

    EXPECT_EQ(engine->get_size(), test_data.size());
}

TEST_F(KVEngineTest, AddUpdate)
{
    uint64_t hash = 100;
    const char *data1 = "original data";
    const char *data2 = "updated data!";

    // Add original
    int ret = engine->add(hash, data1, strlen(data1));
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(engine->get_size(), 1);

    // Update with new data
    ret = engine->add(hash, data2, strlen(data2));
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(engine->get_size(), 1); // Size should not change

    // Verify the data is updated
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(retrieved_len, strlen(data2));
    EXPECT_EQ(memcmp(retrieved_data, data2, retrieved_len), 0);
}

TEST_F(KVEngineTest, AddUpdateSameLength)
{
    uint64_t hash = 200;
    const char *data1 = "data1";
    const char *data2 = "data2";

    // Add original
    int ret = engine->add(hash, data1, strlen(data1));
    EXPECT_EQ(ret, KV_OK);

    // Update with same length (should reuse memory)
    ret = engine->add(hash, data2, strlen(data2));
    EXPECT_EQ(ret, KV_OK);

    // Verify
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(memcmp(retrieved_data, data2, retrieved_len), 0);
}

TEST_F(KVEngineTest, AddDifferentSizes)
{
    std::vector<size_t> sizes = {1, 10, 100, 1024, 4096, 10000, 100000};

    for (size_t i = 0; i < sizes.size(); i++)
    {
        size_t size = sizes[i];
        std::vector<char> data(size, 'A' + (i % 26));

        int ret = engine->add(i, data.data(), size);
        EXPECT_EQ(ret, KV_OK) << "Failed to add data of size " << size;
    }

    EXPECT_EQ(engine->get_size(), sizes.size());
}

TEST_F(KVEngineTest, AddNullPointer)
{
    int ret = engine->add(1, nullptr, 10);
    EXPECT_EQ(ret, KV_ERROR_NULL_POINTER);
}

TEST_F(KVEngineTest, AddZeroLength)
{
    const char *data = "test";
    int ret = engine->add(1, data, 0);
    EXPECT_EQ(ret, KV_ERROR_INVALID_LENGTH);
}

// ============= Get Function Tests =============

TEST_F(KVEngineTest, GetBasic)
{
    uint64_t hash = 123;
    const char *data = "test data";
    size_t len = strlen(data);

    // Add data
    int ret = engine->add(hash, data, len);
    EXPECT_EQ(ret, KV_OK);

    // Get data
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(retrieved_len, len);
    EXPECT_EQ(memcmp(retrieved_data, data, len), 0);
}

TEST_F(KVEngineTest, GetMultiple)
{
    std::vector<std::pair<uint64_t, std::string>> test_data = {
        {10, "value10"},
        {20, "value20"},
        {30, "value30"}};

    // Add all data
    for (const auto &pair : test_data)
    {
        engine->add(pair.first, pair.second.c_str(), pair.second.length());
    }

    // Get and verify all data
    for (const auto &pair : test_data)
    {
        char *retrieved_data = nullptr;
        size_t retrieved_len = 0;
        int ret = engine->get(pair.first, &retrieved_data, &retrieved_len);
        EXPECT_EQ(ret, KV_OK);
        EXPECT_EQ(retrieved_len, pair.second.length());
        EXPECT_EQ(memcmp(retrieved_data, pair.second.c_str(), retrieved_len), 0);
    }
}

TEST_F(KVEngineTest, GetNonExistent)
{
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    int ret = engine->get(99999, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_ERROR_KEY_NOT_FOUND);
}

TEST_F(KVEngineTest, GetNullPointers)
{
    uint64_t hash = 1;
    const char *data = "test";
    engine->add(hash, data, strlen(data));

    // Test null ret_data
    size_t retrieved_len = 0;
    int ret = engine->get(hash, nullptr, &retrieved_len);
    EXPECT_EQ(ret, KV_ERROR_NULL_POINTER);

    // Test null ret_len
    char *retrieved_data = nullptr;
    ret = engine->get(hash, &retrieved_data, nullptr);
    EXPECT_EQ(ret, KV_ERROR_NULL_POINTER);
}

// ============= Delete Function Tests =============

TEST_F(KVEngineTest, DeleteBasic)
{
    uint64_t hash = 456;
    const char *data = "to be deleted";

    // Add and verify
    engine->add(hash, data, strlen(data));
    EXPECT_EQ(engine->get_size(), 1);

    // Delete
    int ret = engine->del(hash);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(engine->get_size(), 0);

    // Verify deletion
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_ERROR_KEY_NOT_FOUND);
}

TEST_F(KVEngineTest, DeleteNonExistent)
{
    int ret = engine->del(99999);
    EXPECT_EQ(ret, KV_ERROR_KEY_NOT_FOUND);
}

TEST_F(KVEngineTest, DeleteMultiple)
{
    std::vector<uint64_t> hashes = {1, 2, 3, 4, 5};
    const char *data = "data";

    // Add multiple
    for (uint64_t hash : hashes)
    {
        engine->add(hash, data, strlen(data));
    }
    EXPECT_EQ(engine->get_size(), hashes.size());

    // Delete all
    for (uint64_t hash : hashes)
    {
        int ret = engine->del(hash);
        EXPECT_EQ(ret, KV_OK);
    }
    EXPECT_EQ(engine->get_size(), 0);
}

// ============= Comprehensive Tests =============

TEST_F(KVEngineTest, AddGetDeleteCycle)
{
    uint64_t hash = 777;
    const char *data = "cycle test";

    // Add
    int ret = engine->add(hash, data, strlen(data));
    EXPECT_EQ(ret, KV_OK);

    // Get
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(memcmp(retrieved_data, data, strlen(data)), 0);

    // Delete
    ret = engine->del(hash);
    EXPECT_EQ(ret, KV_OK);

    // Verify deleted
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_ERROR_KEY_NOT_FOUND);
}

TEST_F(KVEngineTest, LargeDataset)
{
    const size_t COUNT = 10000;
    std::vector<uint64_t> hashes;

    // Add many entries
    for (size_t i = 0; i < COUNT; i++)
    {
        std::string data = "data_" + std::to_string(i);
        int ret = engine->add(i, data.c_str(), data.length());
        EXPECT_EQ(ret, KV_OK);
        hashes.push_back(i);
    }

    EXPECT_EQ(engine->get_size(), COUNT);

    // Verify some random entries
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, COUNT - 1);

    for (int i = 0; i < 100; i++)
    {
        size_t idx = dis(gen);
        std::string expected_data = "data_" + std::to_string(idx);

        char *retrieved_data = nullptr;
        size_t retrieved_len = 0;
        int ret = engine->get(idx, &retrieved_data, &retrieved_len);
        EXPECT_EQ(ret, KV_OK);
        EXPECT_EQ(retrieved_len, expected_data.length());
        EXPECT_EQ(memcmp(retrieved_data, expected_data.c_str(), retrieved_len), 0);
    }

    // Delete half
    for (size_t i = 0; i < COUNT / 2; i++)
    {
        engine->del(i);
    }
    EXPECT_EQ(engine->get_size(), COUNT / 2);
}

TEST_F(KVEngineTest, BinaryData)
{
    uint64_t hash = 888;

    // Create binary data with null bytes
    std::vector<uint8_t> binary_data = {0x00, 0x01, 0x02, 0xFF, 0x00, 0xAB, 0xCD};

    int ret = engine->add(hash, reinterpret_cast<const char *>(binary_data.data()), binary_data.size());
    EXPECT_EQ(ret, KV_OK);

    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(retrieved_len, binary_data.size());
    EXPECT_EQ(memcmp(retrieved_data, binary_data.data(), binary_data.size()), 0);
}

// ============= Statistics Tests =============

TEST_F(KVEngineTest, Statistics)
{
    // Initially should have zero allocators (lazy initialization)
    EXPECT_EQ(engine->get_slab_count(), 0);
    EXPECT_EQ(engine->get_buddy_count(), 0);

    // Add some data and check statistics
    // Use data >= 16 bytes to trigger slab allocation
    std::string base_data = "data_entry_000000"; // At least 16 bytes
    for (int i = 0; i < 100; i++)
    {
        std::string data = base_data + std::to_string(i);
        engine->add(i, data.c_str(), data.length());
    }

    EXPECT_EQ(engine->get_size(), 100);
    // After allocations, we should have created some slabs
    EXPECT_GT(engine->get_slab_count(), 0);
}

// ============= Edge Cases =============

TEST_F(KVEngineTest, SingleByteData)
{
    uint64_t hash = 1;
    char data = 'X';

    int ret = engine->add(hash, &data, 1);
    EXPECT_EQ(ret, KV_OK);

    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(retrieved_len, 1);
    EXPECT_EQ(*retrieved_data, 'X');
}

TEST_F(KVEngineTest, VeryLargeData)
{
    uint64_t hash = 999;
    size_t size = 1024 * 1024; // 1 MB
    std::vector<char> large_data(size, 'L');

    int ret = engine->add(hash, large_data.data(), size);
    EXPECT_EQ(ret, KV_OK);

    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(retrieved_len, size);
    EXPECT_EQ(memcmp(retrieved_data, large_data.data(), size), 0);
}

TEST_F(KVEngineTest, HashCollisionSimulation)
{
    // Use same hash (simulating collision - in reality the map will just overwrite)
    uint64_t hash = 12345;
    const char *data1 = "first";
    const char *data2 = "second";

    engine->add(hash, data1, strlen(data1));
    engine->add(hash, data2, strlen(data2)); // Should overwrite

    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;
    int ret = engine->get(hash, &retrieved_data, &retrieved_len);
    EXPECT_EQ(ret, KV_OK);
    EXPECT_EQ(memcmp(retrieved_data, data2, strlen(data2)), 0);
}

// ============= Error Handling Tests =============

TEST(KVEngineNoFixture, UninitializedOperations)
{
    KVEngine engine;

    // Try operations without initialization
    const char *data = "test";
    char *retrieved_data = nullptr;
    size_t retrieved_len = 0;

    EXPECT_EQ(engine.add(1, data, 4), KV_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(engine.get(1, &retrieved_data, &retrieved_len), KV_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(engine.del(1), KV_ERROR_NOT_INITIALIZED);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
