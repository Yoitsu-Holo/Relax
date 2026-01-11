// TypeDriver 使用示例
// 展示如何使用统一的 TypeDriver 接口进行 KV、Hash、Set 操作

#include "../../cache-TypeDriver/type_driver.h"
#include <iostream>
#include <cstring>
#include <cstdint>

// 简单的 hash 函数（实际使用中应该使用更好的 hash 函数）
uint64_t simple_hash(const char* str, size_t len) {
    uint64_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

// 简单的 crc32（实际使用中应该使用真正的 CRC32）
uint32_t simple_crc32(const char* str, size_t len) {
    uint32_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = (crc << 1) ^ str[i];
    }
    return crc;
}

void example_kv_operations(TypeDriver& driver) {
    std::cout << "\n=== KV 操作示例 ===" << std::endl;

    const char* key = "user:1001";
    const char* value = "Alice";

    uint64_t key_hash = simple_hash(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 设置 KV
    int ret = driver.kv_set(key_hash, key_crc32, value, strlen(value));
    if (ret == KVD_OK) {
        std::cout << "✓ KV Set: " << key << " -> " << value << std::endl;
    } else {
        std::cout << "✗ KV Set failed: " << ret << std::endl;
    }

    // 获取 KV
    char* ret_data = nullptr;
    size_t ret_len = 0;
    ret = driver.kv_get(key_hash, key_crc32, &ret_data, &ret_len);
    if (ret == KVD_OK) {
        std::cout << "✓ KV Get: " << key << " -> "
                  << std::string(ret_data, ret_len) << std::endl;
    } else {
        std::cout << "✗ KV Get failed: " << ret << std::endl;
    }

    // 检查存在性
    ret = driver.kv_exists(key_hash, key_crc32);
    if (ret == KVD_OK) {
        std::cout << "✓ KV Exists: " << key << std::endl;
    }

    // 删除 KV
    ret = driver.kv_del(key_hash, key_crc32);
    if (ret == KVD_OK) {
        std::cout << "✓ KV Deleted: " << key << std::endl;
    }
}

void example_hash_operations(TypeDriver& driver) {
    std::cout << "\n=== Hash 操作示例 ===" << std::endl;

    const char* key = "user:1001:profile";
    uint64_t key_hash = simple_hash(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 设置多个 field
    hash_field fields[3];

    // field 1: name
    const char* field1 = "name";
    const char* value1 = "Alice";
    fields[0].field = field1;
    fields[0].field_len = strlen(field1);
    fields[0].field_hash = simple_hash(field1, strlen(field1));
    fields[0].field_crc32 = simple_crc32(field1, strlen(field1));
    fields[0].value = value1;
    fields[0].value_len = strlen(value1);

    // field 2: age
    const char* field2 = "age";
    const char* value2 = "25";
    fields[1].field = field2;
    fields[1].field_len = strlen(field2);
    fields[1].field_hash = simple_hash(field2, strlen(field2));
    fields[1].field_crc32 = simple_crc32(field2, strlen(field2));
    fields[1].value = value2;
    fields[1].value_len = strlen(value2);

    // field 3: city
    const char* field3 = "city";
    const char* value3 = "Beijing";
    fields[2].field = field3;
    fields[2].field_len = strlen(field3);
    fields[2].field_hash = simple_hash(field3, strlen(field3));
    fields[2].field_crc32 = simple_crc32(field3, strlen(field3));
    fields[2].value = value3;
    fields[2].value_len = strlen(value3);

    // 批量设置
    int ret = driver.hash_set(key_hash, key_crc32, fields, 3);
    if (ret == HASH_OK) {
        std::cout << "✓ Hash Set: " << key << " with 3 fields" << std::endl;
    } else {
        std::cout << "✗ Hash Set failed: " << ret << std::endl;
    }

    // 获取字段数量
    size_t len = 0;
    ret = driver.hash_len(key_hash, key_crc32, &len);
    if (ret == HASH_OK) {
        std::cout << "✓ Hash Len: " << len << " fields" << std::endl;
    }

    // 获取所有字段
    hash_field* ret_fields = nullptr;
    size_t ret_count = 0;
    ret = driver.hash_get(key_hash, key_crc32, &ret_fields, &ret_count);
    if (ret == HASH_OK) {
        std::cout << "✓ Hash Get all fields:" << std::endl;
        for (size_t i = 0; i < ret_count; i++) {
            std::cout << "  - " << std::string(ret_fields[i].field, ret_fields[i].field_len)
                      << ": " << std::string(ret_fields[i].value, ret_fields[i].value_len)
                      << std::endl;
            // 释放内存
            delete[] ret_fields[i].field;
            delete[] ret_fields[i].value;
        }
        delete[] ret_fields;
    }

    // 删除单个字段
    ret = driver.hash_del_m(key_hash, key_crc32, fields[1].field_hash, fields[1].field_crc32);
    if (ret == HASH_OK) {
        std::cout << "✓ Hash Del field: age" << std::endl;
    }

    // 删除整个 hash
    ret = driver.hash_del(key_hash, key_crc32);
    if (ret == HASH_OK) {
        std::cout << "✓ Hash Deleted: " << key << std::endl;
    }
}

void example_set_operations(TypeDriver& driver) {
    std::cout << "\n=== Set 操作示例 ===" << std::endl;

    const char* key = "user:1001:tags";
    uint64_t key_hash = simple_hash(key, strlen(key));
    uint32_t key_crc32 = simple_crc32(key, strlen(key));

    // 添加多个成员
    set_member members[3];

    const char* member1 = "developer";
    members[0].member = member1;
    members[0].member_len = strlen(member1);
    members[0].member_hash = simple_hash(member1, strlen(member1));
    members[0].member_crc32 = simple_crc32(member1, strlen(member1));

    const char* member2 = "golang";
    members[1].member = member2;
    members[1].member_len = strlen(member2);
    members[1].member_hash = simple_hash(member2, strlen(member2));
    members[1].member_crc32 = simple_crc32(member2, strlen(member2));

    const char* member3 = "backend";
    members[2].member = member3;
    members[2].member_len = strlen(member3);
    members[2].member_hash = simple_hash(member3, strlen(member3));
    members[2].member_crc32 = simple_crc32(member3, strlen(member3));

    // 批量添加
    int ret = driver.set_set(key_hash, key_crc32, members, 3);
    if (ret == SET_OK) {
        std::cout << "✓ Set Add: " << key << " with 3 members" << std::endl;
    } else {
        std::cout << "✗ Set Add failed: " << ret << std::endl;
    }

    // 获取成员数量
    size_t len = 0;
    ret = driver.set_len(key_hash, key_crc32, &len);
    if (ret == SET_OK) {
        std::cout << "✓ Set Len: " << len << " members" << std::endl;
    }

    // 检查成员是否存在
    ret = driver.set_exists_m(key_hash, key_crc32, members[0].member_hash, members[0].member_crc32);
    if (ret == SET_OK) {
        std::cout << "✓ Set Contains: " << member1 << std::endl;
    }

    // 获取所有成员
    set_member* ret_members = nullptr;
    size_t ret_count = 0;
    ret = driver.set_get(key_hash, key_crc32, &ret_members, &ret_count);
    if (ret == SET_OK) {
        std::cout << "✓ Set Get all members:" << std::endl;
        for (size_t i = 0; i < ret_count; i++) {
            std::cout << "  - " << std::string(ret_members[i].member, ret_members[i].member_len)
                      << std::endl;
            // 释放内存
            delete[] ret_members[i].member;
        }
        delete[] ret_members;
    }

    // 删除单个成员
    ret = driver.set_del_m(key_hash, key_crc32, members[1].member_hash, members[1].member_crc32);
    if (ret == SET_OK) {
        std::cout << "✓ Set Del member: golang" << std::endl;
    }

    // 删除整个 set
    ret = driver.set_del(key_hash, key_crc32);
    if (ret == SET_OK) {
        std::cout << "✓ Set Deleted: " << key << std::endl;
    }
}

int main() {
    std::cout << "TypeDriver 使用示例" << std::endl;
    std::cout << "==================" << std::endl;

    // 创建 TypeDriver 实例
    TypeDriver driver;

    // 初始化（使用默认参数：64 个 buddy）
    int ret = driver.init();
    if (ret != TYPE_DRIVER_OK) {
        std::cerr << "Failed to initialize TypeDriver: " << ret << std::endl;
        return 1;
    }

    std::cout << "✓ TypeDriver initialized successfully" << std::endl;

    // 演示各种操作
    example_kv_operations(driver);
    example_hash_operations(driver);
    example_set_operations(driver);

    std::cout << "\n所有操作完成！" << std::endl;

    return 0;
}
