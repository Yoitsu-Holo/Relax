#include "type_driver_c.h"
#include "../cache-TypeDriver/type_driver.h"
#include "../cache-TypeDriver/hash_driver.h"
#include "../cache-TypeDriver/set_driver.h"
#include <new>
#include <cstdlib>
#include <cstring>

// ==================== 辅助函数 ====================

// 将 C 的 CHashField 转换为 C++ 的 hash_field
static void c_hash_field_to_cpp(const CHashField* c_field, hash_field* cpp_field)
{
    cpp_field->field_hash = c_field->field_hash;
    cpp_field->field_crc32 = c_field->field_crc32;
    cpp_field->field = c_field->field;
    cpp_field->field_len = c_field->field_len;
    cpp_field->value = c_field->value;
    cpp_field->value_len = c_field->value_len;
}

// 将 C++ 的 hash_field 转换为 C 的 CHashField
static void cpp_hash_field_to_c(const hash_field* cpp_field, CHashField* c_field)
{
    c_field->field_hash = cpp_field->field_hash;
    c_field->field_crc32 = cpp_field->field_crc32;
    c_field->field = cpp_field->field;
    c_field->field_len = cpp_field->field_len;
    c_field->value = cpp_field->value;
    c_field->value_len = cpp_field->value_len;
}

// 将 C 的 CSetMember 转换为 C++ 的 set_member
static void c_set_member_to_cpp(const CSetMember* c_member, set_member* cpp_member)
{
    cpp_member->member_hash = c_member->member_hash;
    cpp_member->member_crc32 = c_member->member_crc32;
    cpp_member->member = c_member->member;
    cpp_member->member_len = c_member->member_len;
}

// 将 C++ 的 set_member 转换为 C 的 CSetMember
static void cpp_set_member_to_c(const set_member* cpp_member, CSetMember* c_member)
{
    c_member->member_hash = cpp_member->member_hash;
    c_member->member_crc32 = cpp_member->member_crc32;
    c_member->member = cpp_member->member;
    c_member->member_len = cpp_member->member_len;
}

// ==================== TypeDriver 生命周期管理 ====================

extern "C" {

TypeDriverHandle type_driver_create(void)
{
    TypeDriver* driver = new (std::nothrow) TypeDriver();
    return static_cast<TypeDriverHandle>(driver);
}

void type_driver_destroy(TypeDriverHandle handle)
{
    if (handle != nullptr)
    {
        TypeDriver* driver = static_cast<TypeDriver*>(handle);
        delete driver;
    }
}

int type_driver_init(TypeDriverHandle handle, size_t max_buddies)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->init(max_buddies);
}

int type_driver_is_initialized(TypeDriverHandle handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->is_initialized() ? 1 : 0;
}

// ==================== KV 操作 ====================

int type_driver_kv_set(TypeDriverHandle handle,
                       uint64_t key_hash,
                       uint32_t key_crc32,
                       const char* data,
                       size_t data_len)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->kv_set(key_hash, key_crc32, data, data_len);
}

int type_driver_kv_get(TypeDriverHandle handle,
                       uint64_t key_hash,
                       uint32_t key_crc32,
                       char** ret_data,
                       size_t* ret_len)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->kv_get(key_hash, key_crc32, ret_data, ret_len);
}

int type_driver_kv_del(TypeDriverHandle handle,
                       uint64_t key_hash,
                       uint32_t key_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->kv_del(key_hash, key_crc32);
}

int type_driver_kv_exists(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->kv_exists(key_hash, key_crc32);
}

// ==================== Hash 单字段操作 ====================

int type_driver_hash_set_m(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32,
                           const CHashField* field)
{
    if (handle == nullptr || field == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    hash_field cpp_field;
    c_hash_field_to_cpp(field, &cpp_field);
    return driver->hash_set_m(key_hash, key_crc32, &cpp_field);
}

int type_driver_hash_get_m(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32,
                           CHashField* field)
{
    if (handle == nullptr || field == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    hash_field cpp_field;
    c_hash_field_to_cpp(field, &cpp_field);

    int ret = driver->hash_get_m(key_hash, key_crc32, &cpp_field);
    if (ret == 0)
    {
        cpp_hash_field_to_c(&cpp_field, field);
    }
    return ret;
}

int type_driver_hash_del_m(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32,
                           uint64_t field_hash,
                           uint32_t field_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->hash_del_m(key_hash, key_crc32, field_hash, field_crc32);
}

int type_driver_hash_exists_m(TypeDriverHandle handle,
                              uint64_t key_hash,
                              uint32_t key_crc32,
                              uint64_t field_hash,
                              uint32_t field_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->hash_exists_m(key_hash, key_crc32, field_hash, field_crc32);
}

// ==================== Hash 整体操作 ====================

int type_driver_hash_set(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32,
                         const CHashField* fields,
                         size_t field_count)
{
    if (handle == nullptr || fields == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);

    // 分配 C++ hash_field 数组
    hash_field* cpp_fields = new (std::nothrow) hash_field[field_count];
    if (cpp_fields == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    // 转换所有 field
    for (size_t i = 0; i < field_count; i++)
    {
        c_hash_field_to_cpp(&fields[i], &cpp_fields[i]);
    }

    int ret = driver->hash_set(key_hash, key_crc32, cpp_fields, field_count);
    delete[] cpp_fields;
    return ret;
}

int type_driver_hash_get(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32,
                         CHashField** fields,
                         size_t* field_count)
{
    if (handle == nullptr || fields == nullptr || field_count == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    hash_field* cpp_fields = nullptr;

    int ret = driver->hash_get(key_hash, key_crc32, &cpp_fields, field_count);
    if (ret != 0 || cpp_fields == nullptr)
    {
        *fields = nullptr;
        *field_count = 0;
        return ret;
    }

    // 分配 C hash field 数组
    CHashField* c_fields = static_cast<CHashField*>(malloc(sizeof(CHashField) * (*field_count)));
    if (c_fields == nullptr)
    {
        // 清理 C++ 分配的内存
        for (size_t i = 0; i < *field_count; i++)
        {
            free(const_cast<char*>(cpp_fields[i].field));
            free(const_cast<char*>(cpp_fields[i].value));
        }
        delete[] cpp_fields;
        return C_TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    // 转换所有 field
    for (size_t i = 0; i < *field_count; i++)
    {
        cpp_hash_field_to_c(&cpp_fields[i], &c_fields[i]);
    }

    delete[] cpp_fields;
    *fields = c_fields;
    return ret;
}

int type_driver_hash_del(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->hash_del(key_hash, key_crc32);
}

int type_driver_hash_exists(TypeDriverHandle handle,
                            uint64_t key_hash,
                            uint32_t key_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->hash_exists(key_hash, key_crc32);
}

int type_driver_hash_len(TypeDriverHandle handle,
                         uint64_t key_hash,
                         uint32_t key_crc32,
                         size_t* len)
{
    if (handle == nullptr || len == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->hash_len(key_hash, key_crc32, len);
}

// ==================== Set 单成员操作 ====================

int type_driver_set_add_m(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32,
                          const CSetMember* member)
{
    if (handle == nullptr || member == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    set_member cpp_member;
    c_set_member_to_cpp(member, &cpp_member);
    return driver->set_add_m(key_hash, key_crc32, &cpp_member);
}

int type_driver_set_exists_m(TypeDriverHandle handle,
                             uint64_t key_hash,
                             uint32_t key_crc32,
                             uint64_t member_hash,
                             uint32_t member_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->set_exists_m(key_hash, key_crc32, member_hash, member_crc32);
}

int type_driver_set_del_m(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32,
                          uint64_t member_hash,
                          uint32_t member_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->set_del_m(key_hash, key_crc32, member_hash, member_crc32);
}

int type_driver_set_get_m(TypeDriverHandle handle,
                          uint64_t key_hash,
                          uint32_t key_crc32,
                          CSetMember* member)
{
    if (handle == nullptr || member == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    set_member cpp_member;
    c_set_member_to_cpp(member, &cpp_member);

    int ret = driver->set_get_m(key_hash, key_crc32, &cpp_member);
    if (ret == 0)
    {
        cpp_set_member_to_c(&cpp_member, member);
    }
    return ret;
}

// ==================== Set 整体操作 ====================

int type_driver_set_set(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32,
                        const CSetMember* members,
                        size_t member_count)
{
    if (handle == nullptr || members == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);

    // 分配 C++ set_member 数组
    set_member* cpp_members = new (std::nothrow) set_member[member_count];
    if (cpp_members == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    // 转换所有 member
    for (size_t i = 0; i < member_count; i++)
    {
        c_set_member_to_cpp(&members[i], &cpp_members[i]);
    }

    int ret = driver->set_set(key_hash, key_crc32, cpp_members, member_count);
    delete[] cpp_members;
    return ret;
}

int type_driver_set_get(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32,
                        CSetMember** members,
                        size_t* member_count)
{
    if (handle == nullptr || members == nullptr || member_count == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }

    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    set_member* cpp_members = nullptr;

    int ret = driver->set_get(key_hash, key_crc32, &cpp_members, member_count);
    if (ret != 0 || cpp_members == nullptr)
    {
        *members = nullptr;
        *member_count = 0;
        return ret;
    }

    // 分配 C set member 数组
    CSetMember* c_members = static_cast<CSetMember*>(malloc(sizeof(CSetMember) * (*member_count)));
    if (c_members == nullptr)
    {
        // 清理 C++ 分配的内存
        for (size_t i = 0; i < *member_count; i++)
        {
            free(const_cast<char*>(cpp_members[i].member));
        }
        delete[] cpp_members;
        return C_TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    // 转换所有 member
    for (size_t i = 0; i < *member_count; i++)
    {
        cpp_set_member_to_c(&cpp_members[i], &c_members[i]);
    }

    delete[] cpp_members;
    *members = c_members;
    return ret;
}

int type_driver_set_del(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->set_del(key_hash, key_crc32);
}

int type_driver_set_exists(TypeDriverHandle handle,
                           uint64_t key_hash,
                           uint32_t key_crc32)
{
    if (handle == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->set_exists(key_hash, key_crc32);
}

int type_driver_set_len(TypeDriverHandle handle,
                        uint64_t key_hash,
                        uint32_t key_crc32,
                        size_t* len)
{
    if (handle == nullptr || len == nullptr)
    {
        return C_TYPE_DRIVER_ERROR_NULL_POINTER;
    }
    TypeDriver* driver = static_cast<TypeDriver*>(handle);
    return driver->set_len(key_hash, key_crc32, len);
}

} // extern "C"
