#include "set_driver.h"
#include "hash_driver.h"
#include <new>

SetDriver::SetDriver() : hash_driver_(nullptr)
{
}

SetDriver::~SetDriver()
{
    if (hash_driver_ != nullptr)
    {
        delete hash_driver_;
        hash_driver_ = nullptr;
    }
}

int SetDriver::init(KVEngine *engine)
{
    if (engine == nullptr)
    {
        return SET_ERROR_NULL_POINTER;
    }

    // 创建内部的 HashDriver
    hash_driver_ = new (std::nothrow) HashDriver();
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ALLOCATION_FAILED;
    }

    // 初始化 HashDriver
    int ret = hash_driver_->init(engine);
    if (ret != HASH_OK)
    {
        delete hash_driver_;
        hash_driver_ = nullptr;
        return convert_hash_error(ret);
    }

    return SET_OK;
}

int SetDriver::convert_hash_error(int hash_error)
{
    // HashDriver 和 SetDriver 的错误码定义相同，直接映射
    switch (hash_error)
    {
    case HASH_OK:
        return SET_OK;
    case HASH_ERROR_NULL_POINTER:
        return SET_ERROR_NULL_POINTER;
    case HASH_ERROR_ENGINE_NOT_INIT:
        return SET_ERROR_ENGINE_NOT_INIT;
    case HASH_ERROR_CRC_MISMATCH:
        return SET_ERROR_CRC_MISMATCH;
    case HASH_ERROR_KEY_NOT_FOUND:
        return SET_ERROR_KEY_NOT_FOUND;
    case HASH_ERROR_FIELD_NOT_FOUND:
        return SET_ERROR_MEMBER_NOT_FOUND;  // field -> member
    case HASH_ERROR_ALLOCATION_FAILED:
        return SET_ERROR_ALLOCATION_FAILED;
    case HASH_ERROR_ENGINE_FAILED:
        return SET_ERROR_ENGINE_FAILED;
    case HASH_ERROR_INVALID_SIZE:
        return SET_ERROR_INVALID_SIZE;
    case HASH_ERROR_TYPE_MISMATCH:
        return SET_ERROR_TYPE_MISMATCH;
    default:
        return SET_ERROR_ENGINE_FAILED;
    }
}

// ==================== 单成员操作 ====================

int SetDriver::set_add_m(uint64_t key_hash, uint32_t key_crc32, const set_member *member)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }
    if (member == nullptr)
    {
        return SET_ERROR_NULL_POINTER;
    }

    // 将 set_member 转换为 hash_field（value 为空）
    hash_field field;
    field.field_hash = member->member_hash;
    field.field_crc32 = member->member_crc32;
    field.field = member->member;
    field.field_len = member->member_len;
    field.value = "";  // Set 不存储 value
    field.value_len = 0;

    // 调用 HashDriver 的 hash_set_m
    int ret = hash_driver_->hash_set_m(key_hash, key_crc32, &field);
    return convert_hash_error(ret);
}

int SetDriver::set_exists_m(uint64_t key_hash, uint32_t key_crc32,
                            uint64_t member_hash, uint32_t member_crc32)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }

    // 直接调用 HashDriver 的 hash_exists_m
    int ret = hash_driver_->hash_exists_m(key_hash, key_crc32, member_hash, member_crc32);
    return convert_hash_error(ret);
}

int SetDriver::set_del_m(uint64_t key_hash, uint32_t key_crc32,
                         uint64_t member_hash, uint32_t member_crc32)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }

    // 直接调用 HashDriver 的 hash_del_m
    int ret = hash_driver_->hash_del_m(key_hash, key_crc32, member_hash, member_crc32);
    return convert_hash_error(ret);
}

int SetDriver::set_get_m(uint64_t key_hash, uint32_t key_crc32, set_member *member)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }
    if (member == nullptr)
    {
        return SET_ERROR_NULL_POINTER;
    }

    // 将 set_member 转换为 hash_field
    hash_field field;
    field.field_hash = member->member_hash;
    field.field_crc32 = member->member_crc32;
    field.field = nullptr;
    field.field_len = 0;
    field.value = nullptr;
    field.value_len = 0;

    // 调用 HashDriver 的 hash_get_m
    int ret = hash_driver_->hash_get_m(key_hash, key_crc32, &field);
    if (ret != HASH_OK)
    {
        return convert_hash_error(ret);
    }

    // 将 hash_field 的结果转换回 set_member（只取 field 部分，忽略 value）
    member->member = field.field;
    member->member_len = field.field_len;

    // 注意：field.value 需要释放（虽然 Set 的 value 应该是空的）
    if (field.value != nullptr)
    {
        delete[] field.value;
    }

    return SET_OK;
}

// ==================== 整体操作 ====================

int SetDriver::set_set(uint64_t key_hash, uint32_t key_crc32,
                       const set_member *members, size_t member_count)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }
    if (members == nullptr && member_count > 0)
    {
        return SET_ERROR_NULL_POINTER;
    }

    // 逐个调用 set_add_m
    for (size_t i = 0; i < member_count; i++)
    {
        int ret = set_add_m(key_hash, key_crc32, &members[i]);
        if (ret != SET_OK)
        {
            return ret;
        }
    }

    return SET_OK;
}

int SetDriver::set_get(uint64_t key_hash, uint32_t key_crc32,
                       set_member **members, size_t *member_count)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }
    if (members == nullptr || member_count == nullptr)
    {
        return SET_ERROR_NULL_POINTER;
    }

    // 调用 HashDriver 的 hash_get
    hash_field *fields = nullptr;
    size_t field_count = 0;
    int ret = hash_driver_->hash_get(key_hash, key_crc32, &fields, &field_count);

    if (ret != HASH_OK)
    {
        return convert_hash_error(ret);
    }

    if (field_count == 0)
    {
        *members = nullptr;
        *member_count = 0;
        return SET_OK;
    }

    // 将 hash_field 数组转换为 set_member 数组
    set_member *result = new (std::nothrow) set_member[field_count];
    if (result == nullptr)
    {
        // 释放 fields 中的内存
        for (size_t i = 0; i < field_count; i++)
        {
            delete[] fields[i].field;
            delete[] fields[i].value;
        }
        delete[] fields;
        return SET_ERROR_ALLOCATION_FAILED;
    }

    for (size_t i = 0; i < field_count; i++)
    {
        result[i].member_hash = fields[i].field_hash;
        result[i].member_crc32 = fields[i].field_crc32;
        result[i].member = fields[i].field;  // 转移所有权
        result[i].member_len = fields[i].field_len;

        // 释放 value（Set 不需要）
        if (fields[i].value != nullptr)
        {
            delete[] fields[i].value;
        }
    }

    delete[] fields;

    *members = result;
    *member_count = field_count;

    return SET_OK;
}

int SetDriver::set_del(uint64_t key_hash, uint32_t key_crc32)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }

    // 直接调用 HashDriver 的 hash_del
    int ret = hash_driver_->hash_del(key_hash, key_crc32);
    return convert_hash_error(ret);
}

int SetDriver::set_exists(uint64_t key_hash, uint32_t key_crc32)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }

    // 直接调用 HashDriver 的 hash_exists
    int ret = hash_driver_->hash_exists(key_hash, key_crc32);
    return convert_hash_error(ret);
}

int SetDriver::set_len(uint64_t key_hash, uint32_t key_crc32, size_t *len)
{
    if (hash_driver_ == nullptr)
    {
        return SET_ERROR_ENGINE_NOT_INIT;
    }
    if (len == nullptr)
    {
        return SET_ERROR_NULL_POINTER;
    }

    // 直接调用 HashDriver 的 hash_len
    int ret = hash_driver_->hash_len(key_hash, key_crc32, len);
    return convert_hash_error(ret);
}
