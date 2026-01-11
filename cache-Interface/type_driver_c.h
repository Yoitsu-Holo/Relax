#ifndef TYPE_DRIVER_C_H
#define TYPE_DRIVER_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误码定义 ====================

// TypeDriver 错误码（C 接口）
// 注意：这些值与 C++ 头文件中的 TypeDriverError 枚举值相同
#define C_TYPE_DRIVER_OK 0
#define C_TYPE_DRIVER_ERROR_NULL_POINTER -1
#define C_TYPE_DRIVER_ERROR_ALREADY_INIT -2
#define C_TYPE_DRIVER_ERROR_NOT_INIT -3
#define C_TYPE_DRIVER_ERROR_INIT_FAILED -4
#define C_TYPE_DRIVER_ERROR_ALLOCATION_FAILED -5

// ==================== 数据结构定义 ====================

// 不透明的 TypeDriver 句柄
typedef void* TypeDriverHandle;

// Hash field 结构体（C 语言版本）
typedef struct {
    uint64_t field_hash;
    uint32_t field_crc32;
    const char* field;
    size_t field_len;
    const char* value;
    size_t value_len;
} CHashField;

// Set member 结构体（C 语言版本）
typedef struct {
    uint64_t member_hash;
    uint32_t member_crc32;
    const char* member;
    size_t member_len;
} CSetMember;

// ==================== TypeDriver 生命周期管理 ====================

// 创建 TypeDriver 实例
// @return TypeDriver 句柄，失败返回 NULL
TypeDriverHandle type_driver_create(void);

// 销毁 TypeDriver 实例
// @param handle: TypeDriver 句柄
void type_driver_destroy(TypeDriverHandle handle);

// 初始化 TypeDriver
// @param handle: TypeDriver 句柄
// @param max_buddies: 最大 buddy 分配器数量
// @return 成功返回 C_TYPE_DRIVER_OK，失败返回错误码
int type_driver_init(TypeDriverHandle handle, size_t max_buddies);

// 检查是否已初始化
// @param handle: TypeDriver 句柄
// @return 已初始化返回 1，否则返回 0
int type_driver_is_initialized(TypeDriverHandle handle);

// ==================== KV 操作 ====================

// 设置键值对
int type_driver_kv_set(TypeDriverHandle handle,
                       uint64_t key_hash,
                       uint32_t key_crc32,
                       const char* data,
                       size_t data_len);

// 获取值
int type_driver_kv_get(TypeDriverHandle handle,
                       uint64_t key_hash,
                       uint32_t key_crc32,
                       char** ret_data,
                       size_t* ret_len);

// 删除键值对
int type_driver_kv_del(TypeDriverHandle handle,
                       uint64_t key_hash,
                       uint32_t key_crc32);

// 检查键是否存在
int type_driver_kv_exists(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32);

// ==================== Hash 单字段操作 ====================

// 设置单个 field
int type_driver_hash_set_m(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32,
                           const CHashField* field);

// 获取单个 field 的 value
int type_driver_hash_get_m(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32,
                           CHashField* field);

// 删除单个 field
int type_driver_hash_del_m(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32,
                           uint64_t field_hash,
                           uint32_t field_crc32);

// 检查 field 是否存在
int type_driver_hash_exists_m(TypeDriverHandle handle,
                              uint64_t key_hash,
                              uint32_t key_crc32,
                              uint64_t field_hash,
                              uint32_t field_crc32);

// ==================== Hash 整体操作 ====================

// 批量设置多个 field
int type_driver_hash_set(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32,
                         const CHashField* fields,
                         size_t field_count);

// 获取 hash 的所有 field-value 对
int type_driver_hash_get(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32,
                         CHashField** fields,
                         size_t* field_count);

// 删除整个 hash
int type_driver_hash_del(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32);

// 检查 hash 是否存在
int type_driver_hash_exists(TypeDriverHandle handle,
                            uint64_t key_hash,
                            uint32_t key_crc32);

// 获取 hash 中 field 的数量
int type_driver_hash_len(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32,
                         size_t* len);

// ==================== Set 单成员操作 ====================

// 添加单个 member
int type_driver_set_add_m(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32,
                          const CSetMember* member);

// 检查 member 是否存在
int type_driver_set_exists_m(TypeDriverHandle handle,
                             uint64_t key_hash,
                             uint32_t key_crc32,
                             uint64_t member_hash,
                             uint32_t member_crc32);

// 删除单个 member
int type_driver_set_del_m(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32,
                          uint64_t member_hash,
                          uint32_t member_crc32);

// 获取单个 member 的数据
int type_driver_set_get_m(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32,
                          CSetMember* member);

// ==================== Set 整体操作 ====================

// 批量添加多个 member
int type_driver_set_set(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32,
                        const CSetMember* members,
                        size_t member_count);

// 获取 set 的所有 member
int type_driver_set_get(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32,
                        CSetMember** members,
                        size_t* member_count);

// 删除整个 set
int type_driver_set_del(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32);

// 检查 set 是否存在
int type_driver_set_exists(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32);

// 获取 set 中 member 的数量
int type_driver_set_len(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32,
                        size_t* len);

#ifdef __cplusplus
}
#endif

#endif // TYPE_DRIVER_C_H
