/**
 * @file test_cache_interface.c
 * @brief C 语言单元测试 - 测试 cache_interface 动态链接库
 *
 * 本测试文件使用纯 C 语言编写，测试通过 C 接口调用 cache_interface 库的功能
 */

#include "type_driver_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ==================== 测试辅助宏 ====================

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "❌ FAIL: %s\n   Location: %s:%d\n   Condition: %s\n", \
                    message, __FILE__, __LINE__, #condition); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "❌ FAIL: %s\n   Location: %s:%d\n   Expected: %d, Got: %d\n", \
                    message, __FILE__, __LINE__, (int)(expected), (int)(actual)); \
            return -1; \
        } \
    } while (0)

#define TEST_PASSED(test_name) \
    printf("✓ PASS: %s\n", test_name)

// ==================== 测试计数器 ====================

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

#define RUN_TEST(test_func) \
    do { \
        total_tests++; \
        printf("\n[%d] Running: %s\n", total_tests, #test_func); \
        if (test_func() == 0) { \
            passed_tests++; \
        } else { \
            failed_tests++; \
        } \
    } while (0)

// ==================== 简单的哈希函数（用于测试） ====================

// 简单的字符串哈希函数
static uint64_t simple_hash(const char* str) {
    uint64_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// 简单的 CRC32 实现（用于测试）
static uint32_t simple_crc32(const char* str, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t)str[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

// ==================== 测试用例 ====================

/**
 * 测试 1: TypeDriver 创建和销毁
 */
int test_create_destroy(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created successfully");

    type_driver_destroy(handle);
    TEST_PASSED("TypeDriver create and destroy");
    return 0;
}

/**
 * 测试 2: TypeDriver 初始化
 */
int test_init(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize successfully");

    int is_init = type_driver_is_initialized(handle);
    TEST_ASSERT(is_init == 1, "TypeDriver should be initialized");

    type_driver_destroy(handle);
    TEST_PASSED("TypeDriver initialization");
    return 0;
}

/**
 * 测试 3: KV 基本操作 - set/get/exists/del
 */
int test_kv_operations(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    // 准备测试数据
    const char* key = "test_key_1";
    const char* value = "test_value_1";
    uint64_t key_hash = simple_hash(key);
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 测试 set
    ret = type_driver_kv_set(handle, key_hash, key_crc32, value, strlen(value));
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV set should succeed");

    // 测试 exists
    ret = type_driver_kv_exists(handle, key_hash, key_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV should exist");

    // 测试 get
    char* ret_data = NULL;
    size_t ret_len = 0;
    ret = type_driver_kv_get(handle, key_hash, key_crc32, &ret_data, &ret_len);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV get should succeed");
    TEST_ASSERT(ret_data != NULL, "Retrieved data should not be NULL");
    TEST_ASSERT(ret_len == strlen(value), "Retrieved length should match");
    TEST_ASSERT(memcmp(ret_data, value, ret_len) == 0, "Retrieved value should match");

    // 测试 del
    ret = type_driver_kv_del(handle, key_hash, key_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV delete should succeed");

    // 验证删除后不存在
    ret = type_driver_kv_exists(handle, key_hash, key_crc32);
    TEST_ASSERT(ret != C_TYPE_DRIVER_OK, "KV should not exist after deletion");

    type_driver_destroy(handle);
    TEST_PASSED("KV basic operations (set/get/exists/del)");
    return 0;
}

/**
 * 测试 4: KV 更新操作
 */
int test_kv_update(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    const char* key = "update_key";
    const char* value1 = "value_1";
    const char* value2 = "value_2_updated";
    uint64_t key_hash = simple_hash(key);
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 设置初始值
    ret = type_driver_kv_set(handle, key_hash, key_crc32, value1, strlen(value1));
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Initial KV set should succeed");

    // 更新值
    ret = type_driver_kv_set(handle, key_hash, key_crc32, value2, strlen(value2));
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV update should succeed");

    // 验证更新后的值
    char* ret_data = NULL;
    size_t ret_len = 0;
    ret = type_driver_kv_get(handle, key_hash, key_crc32, &ret_data, &ret_len);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV get should succeed");
    TEST_ASSERT(ret_len == strlen(value2), "Updated length should match");
    TEST_ASSERT(memcmp(ret_data, value2, ret_len) == 0, "Updated value should match");

    type_driver_destroy(handle);
    TEST_PASSED("KV update operation");
    return 0;
}

/**
 * 测试 5: Hash 单字段操作
 */
int test_hash_single_field(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    const char* hash_key = "my_hash";
    uint64_t hash_key_hash = simple_hash(hash_key);
    uint32_t hash_key_crc32 = simple_crc32(hash_key, strlen(hash_key));

    const char* field_name = "field1";
    const char* field_value = "value1";

    CHashField field;
    field.field_hash = simple_hash(field_name);
    field.field_crc32 = simple_crc32(field_name, strlen(field_name));
    field.field = field_name;
    field.field_len = strlen(field_name);
    field.value = field_value;
    field.value_len = strlen(field_value);

    // 测试 hash_set_m
    ret = type_driver_hash_set_m(handle, hash_key_hash, hash_key_crc32, &field);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash set_m should succeed");

    // 测试 hash_exists_m
    ret = type_driver_hash_exists_m(handle, hash_key_hash, hash_key_crc32,
                                     field.field_hash, field.field_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash field should exist");

    // 测试 hash_get_m
    CHashField get_field;
    get_field.field_hash = field.field_hash;
    get_field.field_crc32 = field.field_crc32;
    get_field.field = field_name;
    get_field.field_len = strlen(field_name);

    ret = type_driver_hash_get_m(handle, hash_key_hash, hash_key_crc32, &get_field);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash get_m should succeed");
    TEST_ASSERT(get_field.value != NULL, "Retrieved value should not be NULL");
    TEST_ASSERT(get_field.value_len == strlen(field_value), "Value length should match");
    TEST_ASSERT(memcmp(get_field.value, field_value, get_field.value_len) == 0,
                "Retrieved value should match");

    // 测试 hash_del_m
    ret = type_driver_hash_del_m(handle, hash_key_hash, hash_key_crc32,
                                  field.field_hash, field.field_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash del_m should succeed");

    // 释放返回的内存
    free((void*)get_field.value);

    type_driver_destroy(handle);
    TEST_PASSED("Hash single field operations");
    return 0;
}

/**
 * 测试 6: Hash 批量操作
 */
int test_hash_batch_operations(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    const char* hash_key = "batch_hash";
    uint64_t hash_key_hash = simple_hash(hash_key);
    uint32_t hash_key_crc32 = simple_crc32(hash_key, strlen(hash_key));

    // 准备多个字段
    const int field_count = 3;
    CHashField fields[3];

    const char* field_names[] = {"f1", "f2", "f3"};
    const char* field_values[] = {"v1", "v2", "v3"};

    for (int i = 0; i < field_count; i++) {
        fields[i].field_hash = simple_hash(field_names[i]);
        fields[i].field_crc32 = simple_crc32(field_names[i], strlen(field_names[i]));
        fields[i].field = field_names[i];
        fields[i].field_len = strlen(field_names[i]);
        fields[i].value = field_values[i];
        fields[i].value_len = strlen(field_values[i]);
    }

    // 测试批量设置
    ret = type_driver_hash_set(handle, hash_key_hash, hash_key_crc32, fields, field_count);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash batch set should succeed");

    // 测试 hash_len
    size_t len = 0;
    ret = type_driver_hash_len(handle, hash_key_hash, hash_key_crc32, &len);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash len should succeed");
    TEST_ASSERT(len == field_count, "Hash length should match field count");

    // 测试 hash_exists
    ret = type_driver_hash_exists(handle, hash_key_hash, hash_key_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash should exist");

    // 测试批量获取
    CHashField* ret_fields = NULL;
    size_t ret_count = 0;
    ret = type_driver_hash_get(handle, hash_key_hash, hash_key_crc32, &ret_fields, &ret_count);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash batch get should succeed");
    TEST_ASSERT(ret_count == field_count, "Retrieved field count should match");

    // 释放内存
    for (size_t i = 0; i < ret_count; i++) {
        free((void*)ret_fields[i].field);
        free((void*)ret_fields[i].value);
    }
    free(ret_fields);

    // 测试 hash_del
    ret = type_driver_hash_del(handle, hash_key_hash, hash_key_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Hash delete should succeed");

    type_driver_destroy(handle);
    TEST_PASSED("Hash batch operations");
    return 0;
}

/**
 * 测试 7: Set 单成员操作
 */
int test_set_single_member(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    const char* set_key = "my_set";
    uint64_t set_key_hash = simple_hash(set_key);
    uint32_t set_key_crc32 = simple_crc32(set_key, strlen(set_key));

    const char* member_name = "member1";

    CSetMember member;
    member.member_hash = simple_hash(member_name);
    member.member_crc32 = simple_crc32(member_name, strlen(member_name));
    member.member = member_name;
    member.member_len = strlen(member_name);

    // 测试 set_add_m
    ret = type_driver_set_add_m(handle, set_key_hash, set_key_crc32, &member);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set add_m should succeed");

    // 测试 set_exists_m
    ret = type_driver_set_exists_m(handle, set_key_hash, set_key_crc32,
                                    member.member_hash, member.member_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set member should exist");

    // 注意：set_get_m 主要用于验证，Set 只存储成员的哈希，不存储实际字符串
    // 所以这里不测试获取成员数据的功能

    // 测试 set_del_m
    ret = type_driver_set_del_m(handle, set_key_hash, set_key_crc32,
                                 member.member_hash, member.member_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set del_m should succeed");

    type_driver_destroy(handle);
    TEST_PASSED("Set single member operations");
    return 0;
}

/**
 * 测试 8: Set 批量操作
 */
int test_set_batch_operations(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    const char* set_key = "batch_set";
    uint64_t set_key_hash = simple_hash(set_key);
    uint32_t set_key_crc32 = simple_crc32(set_key, strlen(set_key));

    // 准备多个成员
    const int member_count = 3;
    CSetMember members[3];

    const char* member_names[] = {"m1", "m2", "m3"};

    for (int i = 0; i < member_count; i++) {
        members[i].member_hash = simple_hash(member_names[i]);
        members[i].member_crc32 = simple_crc32(member_names[i], strlen(member_names[i]));
        members[i].member = member_names[i];
        members[i].member_len = strlen(member_names[i]);
    }

    // 测试批量添加
    ret = type_driver_set_set(handle, set_key_hash, set_key_crc32, members, member_count);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set batch add should succeed");

    // 测试 set_len
    size_t len = 0;
    ret = type_driver_set_len(handle, set_key_hash, set_key_crc32, &len);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set len should succeed");
    TEST_ASSERT(len == member_count, "Set length should match member count");

    // 测试 set_exists
    ret = type_driver_set_exists(handle, set_key_hash, set_key_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set should exist");

    // 测试批量获取
    CSetMember* ret_members = NULL;
    size_t ret_count = 0;
    ret = type_driver_set_get(handle, set_key_hash, set_key_crc32, &ret_members, &ret_count);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set batch get should succeed");
    TEST_ASSERT(ret_count == member_count, "Retrieved member count should match");

    // 释放内存
    for (size_t i = 0; i < ret_count; i++) {
        free((void*)ret_members[i].member);
    }
    free(ret_members);

    // 测试 set_del
    ret = type_driver_set_del(handle, set_key_hash, set_key_crc32);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "Set delete should succeed");

    type_driver_destroy(handle);
    TEST_PASSED("Set batch operations");
    return 0;
}

/**
 * 测试 9: 错误处理 - NULL 指针
 */
int test_null_pointer_handling(void) {
    // 测试 NULL handle
    int ret = type_driver_init(NULL, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_ERROR_NULL_POINTER,
                   "NULL handle should return NULL_POINTER error");

    int is_init = type_driver_is_initialized(NULL);
    TEST_ASSERT(is_init == 0, "NULL handle should return not initialized");

    TEST_PASSED("NULL pointer error handling");
    return 0;
}

/**
 * 测试 10: 多个键值对
 */
int test_multiple_kvs(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    int ret = type_driver_init(handle, 64);
    TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "TypeDriver should initialize");

    // 插入多个键值对
    const int kv_count = 5;
    for (int i = 0; i < kv_count; i++) {
        char key[32], value[32];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(value, sizeof(value), "value_%d", i);

        uint64_t key_hash = simple_hash(key);
        uint32_t key_crc32 = simple_crc32(key, strlen(key));

        ret = type_driver_kv_set(handle, key_hash, key_crc32, value, strlen(value));
        TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV set should succeed for all keys");
    }

    // 验证所有键值对都存在
    for (int i = 0; i < kv_count; i++) {
        char key[32], expected_value[32];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(expected_value, sizeof(expected_value), "value_%d", i);

        uint64_t key_hash = simple_hash(key);
        uint32_t key_crc32 = simple_crc32(key, strlen(key));

        char* ret_data = NULL;
        size_t ret_len = 0;
        ret = type_driver_kv_get(handle, key_hash, key_crc32, &ret_data, &ret_len);
        TEST_ASSERT_EQ(ret, C_TYPE_DRIVER_OK, "KV get should succeed for all keys");
        TEST_ASSERT(ret_len == strlen(expected_value), "Value length should match");
        TEST_ASSERT(memcmp(ret_data, expected_value, ret_len) == 0, "Value should match");
    }

    type_driver_destroy(handle);
    TEST_PASSED("Multiple KV operations");
    return 0;
}

// ==================== 主测试函数 ====================

int main(void) {
    printf("========================================\n");
    printf("Cache Interface C Library Test Suite\n");
    printf("========================================\n");

    // 运行所有测试
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_init);
    RUN_TEST(test_kv_operations);
    RUN_TEST(test_kv_update);
    RUN_TEST(test_hash_single_field);
    RUN_TEST(test_hash_batch_operations);
    RUN_TEST(test_set_single_member);
    RUN_TEST(test_set_batch_operations);
    RUN_TEST(test_null_pointer_handling);
    RUN_TEST(test_multiple_kvs);

    // 打印测试结果摘要
    printf("\n========================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", total_tests);
    printf("  Passed: %d\n", passed_tests);
    printf("  Failed: %d\n", failed_tests);
    printf("========================================\n");

    if (failed_tests == 0) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ Some tests failed!\n");
        return 1;
    }
}
