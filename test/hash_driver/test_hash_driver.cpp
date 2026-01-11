#include <gtest/gtest.h>
#include "../../cache-TypeDriver/hash_driver.h"
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

// 简单的 hash64 实现用于测试
uint64_t simple_hash64(const char *data, size_t len)
{
    uint64_t hash = 0xcbf29ce484222325ULL;  // FNV offset basis
    for (size_t i = 0; i < len; i++)
    {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= 0x100000001b3ULL;  // FNV prime
    }
    return hash;
}

// 计算 field_hash（包含 key 上下文）
uint64_t calculate_field_hash(const char* key, size_t key_len,
                              const char* field, size_t field_len)
{
    char buffer[1024];
    size_t offset = 0;

    // 格式: [key][field][magic(4B)]
    memcpy(buffer + offset, key, key_len);
    offset += key_len;
    memcpy(buffer + offset, field, field_len);
    offset += field_len;
    *(uint32_t*)(buffer + offset) = 0x0D000721;
    offset += 4;

    return simple_hash64(buffer, offset);
}

// 测试夹具
class HashDriverTest : public ::testing::Test
{
protected:
    KVEngine *engine_;
    HashDriver *driver_;

    void SetUp() override
    {
        engine_ = new KVEngine();
        ASSERT_EQ(engine_->init(), KV_OK);

        driver_ = new HashDriver();
        ASSERT_EQ(driver_->init(engine_), HASH_OK);
    }

    void TearDown() override
    {
        delete driver_;
        delete engine_;
    }
};

// 测试基本的单字段 set 和 get 操作
TEST_F(HashDriverTest, BasicSetGetSingleField)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *field_str = "field1";
    const char *value_str = "value1";

    hash_field field;
    field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
    field.field_crc32 = simple_crc32(field_str, strlen(field_str));
    field.field = field_str;
    field.field_len = strlen(field_str);
    field.value = value_str;
    field.value_len = strlen(value_str);

    // 设置单个字段
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 获取单个字段
    hash_field get_field;
    get_field.field_hash = field.field_hash;
    get_field.field_crc32 = field.field_crc32;
    get_field.field = field_str;
    get_field.field_len = strlen(field_str);

    ASSERT_EQ(driver_->hash_get_m(key_hash, key_crc32, &get_field), HASH_OK);
    ASSERT_NE(get_field.value, nullptr);
    ASSERT_EQ(get_field.value_len, strlen(value_str));
    ASSERT_EQ(memcmp(get_field.value, value_str, get_field.value_len), 0);

    // 清理
    delete[] get_field.value;
}

// 测试更新已存在的字段
TEST_F(HashDriverTest, UpdateExistingField)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *field_str = "field1";
    const char *value1 = "value1";
    const char *value2 = "value2_longer";

    hash_field field;
    field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
    field.field_crc32 = simple_crc32(field_str, strlen(field_str));
    field.field = field_str;
    field.field_len = strlen(field_str);

    // 设置初始值
    field.value = value1;
    field.value_len = strlen(value1);
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 更新值
    field.value = value2;
    field.value_len = strlen(value2);
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 获取更新后的值
    hash_field get_field;
    get_field.field_hash = field.field_hash;
    get_field.field_crc32 = field.field_crc32;
    get_field.field = field_str;
    get_field.field_len = strlen(field_str);

    ASSERT_EQ(driver_->hash_get_m(key_hash, key_crc32, &get_field), HASH_OK);
    ASSERT_NE(get_field.value, nullptr);
    ASSERT_EQ(get_field.value_len, strlen(value2));
    ASSERT_EQ(memcmp(get_field.value, value2, get_field.value_len), 0);

    // 清理
    delete[] get_field.value;
}

// 测试多个字段
TEST_F(HashDriverTest, MultipleFields)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 设置多个字段
    const int field_count = 5;
    for (int i = 0; i < field_count; i++)
    {
        char field_str[32];
        char value_str[32];
        snprintf(field_str, sizeof(field_str), "field%d", i);
        snprintf(value_str, sizeof(value_str), "value%d", i);

        hash_field field;
        field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
        field.field_crc32 = simple_crc32(field_str, strlen(field_str));
        field.field = field_str;
        field.field_len = strlen(field_str);
        field.value = value_str;
        field.value_len = strlen(value_str);

        ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);
    }

    // 检查字段数量
    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_OK);
    ASSERT_EQ(len, field_count);

    // 获取所有字段
    hash_field* fields = nullptr;
    size_t count = 0;
    ASSERT_EQ(driver_->hash_get(key_hash, key_crc32, &fields, &count), HASH_OK);
    ASSERT_EQ(count, field_count);
    ASSERT_NE(fields, nullptr);

    // 验证字段
    for (size_t i = 0; i < count; i++)
    {
        ASSERT_NE(fields[i].field, nullptr);
        ASSERT_NE(fields[i].value, nullptr);
    }

    // 清理
    for (size_t i = 0; i < count; i++)
    {
        delete[] fields[i].field;
        delete[] fields[i].value;
    }
    delete[] fields;
}

// 测试删除单个字段
TEST_F(HashDriverTest, DeleteSingleField)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *field_str = "field1";
    const char *value_str = "value1";

    hash_field field;
    field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
    field.field_crc32 = simple_crc32(field_str, strlen(field_str));
    field.field = field_str;
    field.field_len = strlen(field_str);
    field.value = value_str;
    field.value_len = strlen(value_str);

    // 设置字段
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 检查字段存在
    ASSERT_EQ(driver_->hash_exists_m(key_hash, key_crc32, field.field_hash, field.field_crc32), HASH_OK);

    // 删除字段
    ASSERT_EQ(driver_->hash_del_m(key_hash, key_crc32, field.field_hash, field.field_crc32), HASH_OK);

    // 检查字段不存在
    ASSERT_EQ(driver_->hash_exists_m(key_hash, key_crc32, field.field_hash, field.field_crc32), HASH_ERROR_FIELD_NOT_FOUND);

    // 尝试获取已删除的字段
    hash_field get_field;
    get_field.field_hash = field.field_hash;
    get_field.field_crc32 = field.field_crc32;
    ASSERT_EQ(driver_->hash_get_m(key_hash, key_crc32, &get_field), HASH_ERROR_FIELD_NOT_FOUND);
}

// 测试删除整个 hash
TEST_F(HashDriverTest, DeleteEntireHash)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 设置多个字段
    const int field_count = 3;
    for (int i = 0; i < field_count; i++)
    {
        char field_str[32];
        char value_str[32];
        snprintf(field_str, sizeof(field_str), "field%d", i);
        snprintf(value_str, sizeof(value_str), "value%d", i);

        hash_field field;
        field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
        field.field_crc32 = simple_crc32(field_str, strlen(field_str));
        field.field = field_str;
        field.field_len = strlen(field_str);
        field.value = value_str;
        field.value_len = strlen(value_str);

        ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);
    }

    // 检查 hash 存在
    ASSERT_EQ(driver_->hash_exists(key_hash, key_crc32), HASH_OK);

    // 删除整个 hash
    ASSERT_EQ(driver_->hash_del(key_hash, key_crc32), HASH_OK);

    // 检查 hash 不存在
    ASSERT_EQ(driver_->hash_exists(key_hash, key_crc32), HASH_ERROR_KEY_NOT_FOUND);

    // 检查字段数量
    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_ERROR_KEY_NOT_FOUND);
}

// 测试批量设置
TEST_F(HashDriverTest, BatchSet)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const int field_count = 3;
    hash_field fields[field_count];
    char field_strs[field_count][32];
    char value_strs[field_count][32];

    for (int i = 0; i < field_count; i++)
    {
        snprintf(field_strs[i], sizeof(field_strs[i]), "field%d", i);
        snprintf(value_strs[i], sizeof(value_strs[i]), "value%d", i);

        fields[i].field_hash = calculate_field_hash(key, strlen(key), field_strs[i], strlen(field_strs[i]));
        fields[i].field_crc32 = simple_crc32(field_strs[i], strlen(field_strs[i]));
        fields[i].field = field_strs[i];
        fields[i].field_len = strlen(field_strs[i]);
        fields[i].value = value_strs[i];
        fields[i].value_len = strlen(value_strs[i]);
    }

    // 批量设置
    ASSERT_EQ(driver_->hash_set(key_hash, key_crc32, fields, field_count), HASH_OK);

    // 检查字段数量
    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_OK);
    ASSERT_EQ(len, field_count);

    // 验证每个字段
    for (int i = 0; i < field_count; i++)
    {
        ASSERT_EQ(driver_->hash_exists_m(key_hash, key_crc32, fields[i].field_hash, fields[i].field_crc32), HASH_OK);
    }
}

// 测试 CRC 冲突检测（key）
TEST_F(HashDriverTest, KeyCRCMismatchDetection)
{
    const char *key1 = "key1";
    const char *key2 = "key2";
    uint64_t key_hash = 12345;  // 相同的 hash
    uint32_t key1_crc32 = simple_crc32(key1, strlen(key1));
    uint32_t key2_crc32 = simple_crc32(key2, strlen(key2));

    const char *field_str = "field1";
    const char *value_str = "value1";

    hash_field field;
    field.field_hash = calculate_field_hash(key1, strlen(key1), field_str, strlen(field_str));
    field.field_crc32 = simple_crc32(field_str, strlen(field_str));
    field.field = field_str;
    field.field_len = strlen(field_str);
    field.value = value_str;
    field.value_len = strlen(value_str);

    // 使用 key1 设置
    ASSERT_EQ(driver_->hash_set_m(key_hash, key1_crc32, &field), HASH_OK);

    // 尝试使用 key2（不同的 crc32）访问
    ASSERT_EQ(driver_->hash_exists(key_hash, key2_crc32), HASH_ERROR_CRC_MISMATCH);
    ASSERT_EQ(driver_->hash_del(key_hash, key2_crc32), HASH_ERROR_CRC_MISMATCH);

    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key2_crc32, &len), HASH_ERROR_CRC_MISMATCH);
}

// 测试 CRC 冲突检测（field）
TEST_F(HashDriverTest, FieldCRCMismatchDetection)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *field1 = "field1";
    const char *field2 = "field2";
    uint64_t field_hash = 54321;  // 相同的 field_hash
    uint32_t field1_crc32 = simple_crc32(field1, strlen(field1));
    uint32_t field2_crc32 = simple_crc32(field2, strlen(field2));

    const char *value_str = "value1";

    hash_field field;
    field.field_hash = field_hash;
    field.field_crc32 = field1_crc32;
    field.field = field1;
    field.field_len = strlen(field1);
    field.value = value_str;
    field.value_len = strlen(value_str);

    // 使用 field1 设置
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 尝试使用 field2（不同的 crc32）访问
    ASSERT_EQ(driver_->hash_exists_m(key_hash, key_crc32, field_hash, field2_crc32), HASH_ERROR_CRC_MISMATCH);
    ASSERT_EQ(driver_->hash_del_m(key_hash, key_crc32, field_hash, field2_crc32), HASH_ERROR_CRC_MISMATCH);
}

// 测试元数据扩容（添加多个字段以触发扩容）
TEST_F(HashDriverTest, MetadataExpansion)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 添加足够多的字段以触发扩容（256B 初始大小大约可以容纳 12 个 entry）
    const int field_count = 20;
    for (int i = 0; i < field_count; i++)
    {
        char field_str[32];
        char value_str[32];
        snprintf(field_str, sizeof(field_str), "field%d", i);
        snprintf(value_str, sizeof(value_str), "value%d", i);

        hash_field field;
        field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
        field.field_crc32 = simple_crc32(field_str, strlen(field_str));
        field.field = field_str;
        field.field_len = strlen(field_str);
        field.value = value_str;
        field.value_len = strlen(value_str);

        ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);
    }

    // 验证所有字段都存在
    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_OK);
    ASSERT_EQ(len, field_count);

    // 验证每个字段都能正确获取
    for (int i = 0; i < field_count; i++)
    {
        char field_str[32];
        snprintf(field_str, sizeof(field_str), "field%d", i);

        hash_field get_field;
        get_field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
        get_field.field_crc32 = simple_crc32(field_str, strlen(field_str));
        get_field.field = field_str;
        get_field.field_len = strlen(field_str);

        ASSERT_EQ(driver_->hash_get_m(key_hash, key_crc32, &get_field), HASH_OK);
        ASSERT_NE(get_field.value, nullptr);

        delete[] get_field.value;
    }
}

// 测试元数据整理（添加和删除字段以触发整理）
TEST_F(HashDriverTest, MetadataCompaction)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 添加多个字段
    const int field_count = 15;
    uint64_t field_hashes[field_count];
    uint32_t field_crc32s[field_count];

    for (int i = 0; i < field_count; i++)
    {
        char field_str[32];
        char value_str[32];
        snprintf(field_str, sizeof(field_str), "field%d", i);
        snprintf(value_str, sizeof(value_str), "value%d", i);

        hash_field field;
        field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
        field.field_crc32 = simple_crc32(field_str, strlen(field_str));
        field.field = field_str;
        field.field_len = strlen(field_str);
        field.value = value_str;
        field.value_len = strlen(value_str);

        field_hashes[i] = field.field_hash;
        field_crc32s[i] = field.field_crc32;

        ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);
    }

    // 删除一半的字段
    for (int i = 0; i < field_count / 2; i++)
    {
        ASSERT_EQ(driver_->hash_del_m(key_hash, key_crc32, field_hashes[i], field_crc32s[i]), HASH_OK);
    }

    // 验证剩余字段数量
    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_OK);
    ASSERT_EQ(len, field_count - field_count / 2);

    // 添加新字段（可能触发整理）
    for (int i = 0; i < 10; i++)
    {
        char field_str[32];
        char value_str[32];
        snprintf(field_str, sizeof(field_str), "new_field%d", i);
        snprintf(value_str, sizeof(value_str), "new_value%d", i);

        hash_field field;
        field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
        field.field_crc32 = simple_crc32(field_str, strlen(field_str));
        field.field = field_str;
        field.field_len = strlen(field_str);
        field.value = value_str;
        field.value_len = strlen(value_str);

        ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);
    }

    // 验证最终字段数量
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_OK);
    ASSERT_EQ(len, (field_count - field_count / 2) + 10);
}

// 测试空值处理
TEST_F(HashDriverTest, EmptyValues)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *field_str = "field1";
    const char *value_str = "";  // 空值

    hash_field field;
    field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
    field.field_crc32 = simple_crc32(field_str, strlen(field_str));
    field.field = field_str;
    field.field_len = strlen(field_str);
    field.value = value_str;
    field.value_len = 0;

    // 设置空值
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 获取空值
    hash_field get_field;
    get_field.field_hash = field.field_hash;
    get_field.field_crc32 = field.field_crc32;
    get_field.field = field_str;
    get_field.field_len = strlen(field_str);

    ASSERT_EQ(driver_->hash_get_m(key_hash, key_crc32, &get_field), HASH_OK);
    ASSERT_EQ(get_field.value_len, 0);

    // 清理
    if (get_field.value != nullptr)
    {
        delete[] get_field.value;
    }
}

// 测试不存在的 key
TEST_F(HashDriverTest, NonExistentKey)
{
    const char *key = "nonexistent_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 检查不存在的 key
    ASSERT_EQ(driver_->hash_exists(key_hash, key_crc32), HASH_ERROR_KEY_NOT_FOUND);

    size_t len = 0;
    ASSERT_EQ(driver_->hash_len(key_hash, key_crc32, &len), HASH_ERROR_KEY_NOT_FOUND);

    hash_field* fields = nullptr;
    size_t count = 0;
    ASSERT_EQ(driver_->hash_get(key_hash, key_crc32, &fields, &count), HASH_ERROR_KEY_NOT_FOUND);
}

// 测试不存在的 field
TEST_F(HashDriverTest, NonExistentField)
{
    const char *key = "test_key";
    uint64_t key_hash = simple_hash64(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    const char *field_str = "field1";
    const char *value_str = "value1";

    hash_field field;
    field.field_hash = calculate_field_hash(key, strlen(key), field_str, strlen(field_str));
    field.field_crc32 = simple_crc32(field_str, strlen(field_str));
    field.field = field_str;
    field.field_len = strlen(field_str);
    field.value = value_str;
    field.value_len = strlen(value_str);

    // 设置一个字段
    ASSERT_EQ(driver_->hash_set_m(key_hash, key_crc32, &field), HASH_OK);

    // 尝试获取不存在的字段
    const char *nonexistent_field = "nonexistent";
    hash_field get_field;
    get_field.field_hash = calculate_field_hash(key, strlen(key), nonexistent_field, strlen(nonexistent_field));
    get_field.field_crc32 = simple_crc32(nonexistent_field, strlen(nonexistent_field));
    get_field.field = nonexistent_field;
    get_field.field_len = strlen(nonexistent_field);

    ASSERT_EQ(driver_->hash_get_m(key_hash, key_crc32, &get_field), HASH_ERROR_FIELD_NOT_FOUND);
    ASSERT_EQ(driver_->hash_exists_m(key_hash, key_crc32, get_field.field_hash, get_field.field_crc32), HASH_ERROR_FIELD_NOT_FOUND);
}
