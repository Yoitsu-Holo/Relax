#ifndef TYPE_DRIVER_H
#define TYPE_DRIVER_H

#include <cstddef>
#include <cstdint>
#include "kv_driver.h"
#include "hash_driver.h"
#include "set_driver.h"

// 前置声明
class KVEngine;

// TypeDriver 错误码
enum TypeDriverError
{
    TYPE_DRIVER_OK = 0,
    TYPE_DRIVER_ERROR_NULL_POINTER = -1,
    TYPE_DRIVER_ERROR_ALREADY_INIT = -2,
    TYPE_DRIVER_ERROR_NOT_INIT = -3,
    TYPE_DRIVER_ERROR_INIT_FAILED = -4,
    TYPE_DRIVER_ERROR_ALLOCATION_FAILED = -5
};

// TypeDriver - 统一的类型驱动封装层
// 管理 KVEngine 和所有类型驱动（KV, Hash, Set）
// 提供统一的初始化和操作接口
class TypeDriver
{
public:
    TypeDriver();
    ~TypeDriver();

    // 禁用拷贝和赋值
    TypeDriver(const TypeDriver &) = delete;
    TypeDriver &operator=(const TypeDriver &) = delete;

    // 初始化类型驱动
    // 内部会创建 KVEngine 并初始化所有 Driver
    // @param max_buddies: 最大 buddy 分配器数量（传递给 KVEngine）
    // @return 成功返回 TYPE_DRIVER_OK，失败返回错误码
    int init(size_t max_buddies = 64);

    // 检查是否已初始化
    bool is_initialized() const;

    // ==================== KV 操作 ====================

    // 设置键值对
    int kv_set(uint64_t key_hash, uint32_t key_crc32, const char *data, size_t data_len);

    // 获取值
    int kv_get(uint64_t key_hash, uint32_t key_crc32, char **ret_data, size_t *ret_len);

    // 删除键值对
    int kv_del(uint64_t key_hash, uint32_t key_crc32);

    // 检查键是否存在
    int kv_exists(uint64_t key_hash, uint32_t key_crc32);

    // ==================== Hash 单字段操作 ====================

    // 设置单个 field
    int hash_set_m(uint64_t key_hash, uint32_t key_crc32, const hash_field* field);

    // 获取单个 field 的 value
    int hash_get_m(uint64_t key_hash, uint32_t key_crc32, hash_field* field);

    // 删除单个 field
    int hash_del_m(uint64_t key_hash, uint32_t key_crc32,
                   uint64_t field_hash, uint32_t field_crc32);

    // 检查 field 是否存在
    int hash_exists_m(uint64_t key_hash, uint32_t key_crc32,
                      uint64_t field_hash, uint32_t field_crc32);

    // ==================== Hash 整体操作 ====================

    // 批量设置多个 field
    int hash_set(uint64_t key_hash, uint32_t key_crc32,
                 const hash_field* fields, size_t field_count);

    // 获取 hash 的所有 field-value 对
    int hash_get(uint64_t key_hash, uint32_t key_crc32,
                 hash_field** fields, size_t* field_count);

    // 删除整个 hash
    int hash_del(uint64_t key_hash, uint32_t key_crc32);

    // 检查 hash 是否存在
    int hash_exists(uint64_t key_hash, uint32_t key_crc32);

    // 获取 hash 中 field 的数量
    int hash_len(uint64_t key_hash, uint32_t key_crc32, size_t* len);

    // ==================== Set 单成员操作 ====================

    // 添加单个 member
    int set_add_m(uint64_t key_hash, uint32_t key_crc32, const set_member* member);

    // 检查 member 是否存在
    int set_exists_m(uint64_t key_hash, uint32_t key_crc32,
                     uint64_t member_hash, uint32_t member_crc32);

    // 删除单个 member
    int set_del_m(uint64_t key_hash, uint32_t key_crc32,
                  uint64_t member_hash, uint32_t member_crc32);

    // 获取单个 member 的数据
    int set_get_m(uint64_t key_hash, uint32_t key_crc32, set_member* member);

    // ==================== Set 整体操作 ====================

    // 批量添加多个 member
    int set_set(uint64_t key_hash, uint32_t key_crc32,
                const set_member* members, size_t member_count);

    // 获取 set 的所有 member
    int set_get(uint64_t key_hash, uint32_t key_crc32,
                set_member** members, size_t* member_count);

    // 删除整个 set
    int set_del(uint64_t key_hash, uint32_t key_crc32);

    // 检查 set 是否存在
    int set_exists(uint64_t key_hash, uint32_t key_crc32);

    // 获取 set 中 member 的数量
    int set_len(uint64_t key_hash, uint32_t key_crc32, size_t* len);

    // ==================== 访问底层组件（高级用法） ====================

    // 获取 KVEngine 实例（用于高级操作）
    KVEngine* get_kv_engine() const { return kv_engine_; }

    // 获取 KVDriver 实例
    KVDriver* get_kv_driver() const { return kv_driver_; }

    // 获取 HashDriver 实例
    HashDriver* get_hash_driver() const { return hash_driver_; }

    // 获取 SetDriver 实例
    SetDriver* get_set_driver() const { return set_driver_; }

private:
    KVEngine* kv_engine_;      // KV 存储引擎
    KVDriver* kv_driver_;      // KV 类型驱动
    HashDriver* hash_driver_;  // Hash 类型驱动
    SetDriver* set_driver_;    // Set 类型驱动
    bool initialized_;         // 初始化标志
};

#endif // TYPE_DRIVER_H
