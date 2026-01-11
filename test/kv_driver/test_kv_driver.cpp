#include <gtest/gtest.h>
#include "../../cache-TypeDriver/kv_driver.h"
#include "../../cache-Kernel/kvEngine/kv_engine.h"
#include <cstring>

// 简单的 CRC32 实现用于测试
uint32_t simple_crc32(const char *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

// 测试夹具
class KVDriverTest : public ::testing::Test
{
protected:
    KVEngine *engine_;
    KVDriver *driver_;

    void SetUp() override
    {
        engine_ = new KVEngine();
        ASSERT_EQ(engine_->init(), KV_OK);

        driver_ = new KVDriver();
        ASSERT_EQ(driver_->init(engine_), KVD_OK);
    }

    void TearDown() override
    {
        delete driver_;
        delete engine_;
    }
};

// 测试基本的 set 和 get 操作
TEST_F(KVDriverTest, BasicSetGet)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 设置键值对
    ASSERT_EQ(driver_->set(key_hash, key_crc32, value, value_len), KVD_OK);

    // 获取键值对
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_OK);
    ASSERT_NE(ret_data, nullptr);
    ASSERT_EQ(ret_len, value_len);
    ASSERT_EQ(memcmp(ret_data, value, value_len), 0);
}

// 测试更新已存在的键值对
TEST_F(KVDriverTest, UpdateExistingKey)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *value1 = "value1";
    size_t value1_len = strlen(value1);

    const char *value2 = "value2_longer";
    size_t value2_len = strlen(value2);

    // 设置初始值
    ASSERT_EQ(driver_->set(key_hash, key_crc32, value1, value1_len), KVD_OK);

    // 更新值
    ASSERT_EQ(driver_->set(key_hash, key_crc32, value2, value2_len), KVD_OK);

    // 获取更新后的值
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_OK);
    ASSERT_NE(ret_data, nullptr);
    ASSERT_EQ(ret_len, value2_len);
    ASSERT_EQ(memcmp(ret_data, value2, value2_len), 0);
}

// 测试 CRC32 冲突检测
TEST_F(KVDriverTest, CRCMismatchDetection)
{
    const char *key1 = "test_key1";
    uint64_t key_hash = 12345;  // 相同的hash
    uint32_t key1_crc32 = simple_crc32(key1, strlen(key1));

    const char *key2 = "test_key2";
    uint32_t key2_crc32 = simple_crc32(key2, strlen(key2));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 使用 key1 设置值
    ASSERT_EQ(driver_->set(key_hash, key1_crc32, value, value_len), KVD_OK);

    // 尝试使用相同的 hash 但不同的 CRC32 设置值（模拟 hash 冲突）
    ASSERT_EQ(driver_->set(key_hash, key2_crc32, value, value_len), KVD_ERROR_CRC_MISMATCH);

    // 尝试使用错误的 CRC32 获取值
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key2_crc32, &ret_data, &ret_len), KVD_ERROR_CRC_MISMATCH);
}

// 测试删除操作
TEST_F(KVDriverTest, DeleteKey)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 设置键值对
    ASSERT_EQ(driver_->set(key_hash, key_crc32, value, value_len), KVD_OK);

    // 删除键值对
    ASSERT_EQ(driver_->del(key_hash, key_crc32), KVD_OK);

    // 尝试获取已删除的键值对
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_ERROR_KEY_NOT_FOUND);
}

// 测试删除时的 CRC32 冲突检测
TEST_F(KVDriverTest, DeleteCRCMismatch)
{
    const char *key1 = "test_key1";
    uint64_t key_hash = 12345;
    uint32_t key1_crc32 = simple_crc32(key1, strlen(key1));

    const char *key2 = "test_key2";
    uint32_t key2_crc32 = simple_crc32(key2, strlen(key2));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 使用 key1 设置值
    ASSERT_EQ(driver_->set(key_hash, key1_crc32, value, value_len), KVD_OK);

    // 尝试使用错误的 CRC32 删除值（防止误删除）
    ASSERT_EQ(driver_->del(key_hash, key2_crc32), KVD_ERROR_CRC_MISMATCH);

    // 验证值仍然存在
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key1_crc32, &ret_data, &ret_len), KVD_OK);
}

// 测试获取不存在的键
TEST_F(KVDriverTest, GetNonExistentKey)
{
    const char *key = "nonexistent_key";
    uint64_t key_hash = 99999;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_ERROR_KEY_NOT_FOUND);
}

// 测试删除不存在的键
TEST_F(KVDriverTest, DeleteNonExistentKey)
{
    const char *key = "nonexistent_key";
    uint64_t key_hash = 99999;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    ASSERT_EQ(driver_->del(key_hash, key_crc32), KVD_ERROR_KEY_NOT_FOUND);
}

// 测试空数据
TEST_F(KVDriverTest, EmptyData)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 设置空数据
    ASSERT_EQ(driver_->set(key_hash, key_crc32, "", 0), KVD_OK);

    // 获取空数据
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_OK);
    ASSERT_EQ(ret_len, 0);
}

// 测试大数据
TEST_F(KVDriverTest, LargeData)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 创建 10KB 的数据
    const size_t large_size = 10 * 1024;
    char *large_data = new char[large_size];
    for (size_t i = 0; i < large_size; i++)
    {
        large_data[i] = 'A' + (i % 26);
    }

    // 设置大数据
    ASSERT_EQ(driver_->set(key_hash, key_crc32, large_data, large_size), KVD_OK);

    // 获取大数据
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_OK);
    ASSERT_NE(ret_data, nullptr);
    ASSERT_EQ(ret_len, large_size);
    ASSERT_EQ(memcmp(ret_data, large_data, large_size), 0);

    delete[] large_data;
}

// 测试多个键值对
TEST_F(KVDriverTest, MultipleKeys)
{
    const int num_keys = 100;

    // 设置多个键值对
    for (int i = 0; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        std::string value = "value_" + std::to_string(i);

        ASSERT_EQ(driver_->set(key_hash, key_crc32, value.c_str(), value.length()), KVD_OK);
    }

    // 验证所有键值对
    for (int i = 0; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        std::string expected_value = "value_" + std::to_string(i);

        char *ret_data = nullptr;
        size_t ret_len = 0;
        ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_OK);
        ASSERT_NE(ret_data, nullptr);
        ASSERT_EQ(ret_len, expected_value.length());
        ASSERT_EQ(memcmp(ret_data, expected_value.c_str(), ret_len), 0);
    }

    // 删除所有键值对
    for (int i = 0; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        ASSERT_EQ(driver_->del(key_hash, key_crc32), KVD_OK);
    }

    // 验证所有键都已删除
    for (int i = 0; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        char *ret_data = nullptr;
        size_t ret_len = 0;
        ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_ERROR_KEY_NOT_FOUND);
    }
}

// 测试 exists 方法 - 基本功能
TEST_F(KVDriverTest, ExistsBasic)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 键不存在时
    ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_ERROR_KEY_NOT_FOUND);

    // 设置键值对后
    ASSERT_EQ(driver_->set(key_hash, key_crc32, value, value_len), KVD_OK);

    // 键应该存在
    ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_OK);

    // 删除键值对后
    ASSERT_EQ(driver_->del(key_hash, key_crc32), KVD_OK);

    // 键应该不存在
    ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_ERROR_KEY_NOT_FOUND);
}

// 测试 exists 方法 - CRC32 不匹配
TEST_F(KVDriverTest, ExistsCRCMismatch)
{
    const char *key1 = "test_key1";
    uint64_t key_hash = 12345;  // 相同的hash
    uint32_t key1_crc32 = simple_crc32(key1, strlen(key1));

    const char *key2 = "test_key2";
    uint32_t key2_crc32 = simple_crc32(key2, strlen(key2));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 使用 key1 设置值
    ASSERT_EQ(driver_->set(key_hash, key1_crc32, value, value_len), KVD_OK);

    // 使用正确的 CRC32 检查应该返回存在
    ASSERT_EQ(driver_->exists(key_hash, key1_crc32), KVD_OK);

    // 使用错误的 CRC32 检查应该返回 CRC 不匹配
    ASSERT_EQ(driver_->exists(key_hash, key2_crc32), KVD_ERROR_CRC_MISMATCH);
}

// 测试 exists 方法 - 多个键
TEST_F(KVDriverTest, ExistsMultipleKeys)
{
    const int num_keys = 10;

    // 设置多个键值对
    for (int i = 0; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        std::string value = "value_" + std::to_string(i);

        ASSERT_EQ(driver_->set(key_hash, key_crc32, value.c_str(), value.length()), KVD_OK);
    }

    // 验证所有键都存在
    for (int i = 0; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_OK);
    }

    // 删除部分键
    for (int i = 0; i < num_keys / 2; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        ASSERT_EQ(driver_->del(key_hash, key_crc32), KVD_OK);
    }

    // 验证已删除的键不存在
    for (int i = 0; i < num_keys / 2; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_ERROR_KEY_NOT_FOUND);
    }

    // 验证未删除的键仍然存在
    for (int i = num_keys / 2; i < num_keys; i++)
    {
        std::string key = "key_" + std::to_string(i);
        uint64_t key_hash = i;
        uint32_t key_crc32 = simple_crc32(key.c_str(), key.length());

        ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_OK);
    }
}

// 测试初始化错误
TEST_F(KVDriverTest, InitializationErrors)
{
    KVDriver driver;

    // 使用空指针初始化
    ASSERT_EQ(driver.init(nullptr), KVD_ERROR_NULL_POINTER);

    // 在未初始化的引擎上初始化
    KVEngine engine;
    ASSERT_EQ(driver.init(&engine), KVD_ERROR_ENGINE_NOT_INIT);
}

// 测试类型字段设置和校验
TEST_F(KVDriverTest, TypeFieldValidation)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 设置键值对
    ASSERT_EQ(driver_->set(key_hash, key_crc32, value, value_len), KVD_OK);

    // 直接从底层引擎获取数据，验证类型字段
    char *raw_data = nullptr;
    size_t raw_len = 0;
    ASSERT_EQ(engine_->get(key_hash, &raw_data, &raw_len), KV_OK);
    ASSERT_NE(raw_data, nullptr);

    // 检查类型字段是否设置为 DATA_TYPE_KV (0)
    KVMetadata *metadata = reinterpret_cast<KVMetadata *>(raw_data);
    ASSERT_EQ(metadata->type, DATA_TYPE_KV);
}

// 测试类型不匹配错误
TEST_F(KVDriverTest, TypeMismatchError)
{
    const char *key = "test_key";
    uint64_t key_hash = 12345;
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *value = "test_value";
    size_t value_len = strlen(value);

    // 手动创建一个错误类型的元数据结构
    size_t block_size = 128;
    char *buffer = new char[block_size];
    KVMetadata *metadata = reinterpret_cast<KVMetadata *>(buffer);
    metadata->key_crc32 = key_crc32;
    metadata->data_len = value_len;
    metadata->block_size = block_size;
    metadata->type = DATA_TYPE_HASH;  // 设置为 HASH 类型（错误的类型）
    metadata->exp_time = 0;
    memcpy(metadata->data, value, value_len);

    // 直接通过底层引擎添加这个错误类型的数据
    ASSERT_EQ(engine_->add(key_hash, buffer, block_size), KV_OK);
    delete[] buffer;

    // 尝试使用 kv_driver 的 get 方法获取，应该返回类型不匹配错误
    char *ret_data = nullptr;
    size_t ret_len = 0;
    ASSERT_EQ(driver_->get(key_hash, key_crc32, &ret_data, &ret_len), KVD_ERROR_TYPE_MISMATCH);

    // 尝试使用 exists 方法，应该返回类型不匹配错误
    ASSERT_EQ(driver_->exists(key_hash, key_crc32), KVD_ERROR_TYPE_MISMATCH);

    // 尝试使用 set 方法更新，应该返回类型不匹配错误
    ASSERT_EQ(driver_->set(key_hash, key_crc32, "new_value", 9), KVD_ERROR_TYPE_MISMATCH);

    // 尝试使用 del 方法删除，应该返回类型不匹配错误
    ASSERT_EQ(driver_->del(key_hash, key_crc32), KVD_ERROR_TYPE_MISMATCH);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
