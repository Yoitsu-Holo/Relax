#include "type_driver.h"
#include "../cache-Kernel/kvEngine/kv_engine.h"
#include <new>

TypeDriver::TypeDriver()
    : kv_engine_(nullptr),
      kv_driver_(nullptr),
      hash_driver_(nullptr),
      set_driver_(nullptr),
      initialized_(false)
{
}

TypeDriver::~TypeDriver()
{
    // 按照依赖关系的逆序释放资源
    // Set -> Hash -> KV -> Engine
    if (set_driver_ != nullptr)
    {
        delete set_driver_;
        set_driver_ = nullptr;
    }

    if (hash_driver_ != nullptr)
    {
        delete hash_driver_;
        hash_driver_ = nullptr;
    }

    if (kv_driver_ != nullptr)
    {
        delete kv_driver_;
        kv_driver_ = nullptr;
    }

    if (kv_engine_ != nullptr)
    {
        delete kv_engine_;
        kv_engine_ = nullptr;
    }

    initialized_ = false;
}

int TypeDriver::init(size_t max_buddies)
{
    if (initialized_)
    {
        return TYPE_DRIVER_ERROR_ALREADY_INIT;
    }

    // Step 1: 创建并初始化 KVEngine
    kv_engine_ = new (std::nothrow) KVEngine();
    if (kv_engine_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    int ret = kv_engine_->init(max_buddies);
    if (ret != KV_OK)
    {
        delete kv_engine_;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_INIT_FAILED;
    }

    // Step 2: 创建并初始化 KVDriver
    kv_driver_ = new (std::nothrow) KVDriver();
    if (kv_driver_ == nullptr)
    {
        delete kv_engine_;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    ret = kv_driver_->init(kv_engine_);
    if (ret != KVD_OK)
    {
        delete kv_driver_;
        delete kv_engine_;
        kv_driver_ = nullptr;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_INIT_FAILED;
    }

    // Step 3: 创建并初始化 HashDriver
    hash_driver_ = new (std::nothrow) HashDriver();
    if (hash_driver_ == nullptr)
    {
        delete kv_driver_;
        delete kv_engine_;
        kv_driver_ = nullptr;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    ret = hash_driver_->init(kv_engine_);
    if (ret != HASH_OK)
    {
        delete hash_driver_;
        delete kv_driver_;
        delete kv_engine_;
        hash_driver_ = nullptr;
        kv_driver_ = nullptr;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_INIT_FAILED;
    }

    // Step 4: 创建并初始化 SetDriver
    set_driver_ = new (std::nothrow) SetDriver();
    if (set_driver_ == nullptr)
    {
        delete hash_driver_;
        delete kv_driver_;
        delete kv_engine_;
        hash_driver_ = nullptr;
        kv_driver_ = nullptr;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_ALLOCATION_FAILED;
    }

    ret = set_driver_->init(kv_engine_);
    if (ret != SET_OK)
    {
        delete set_driver_;
        delete hash_driver_;
        delete kv_driver_;
        delete kv_engine_;
        set_driver_ = nullptr;
        hash_driver_ = nullptr;
        kv_driver_ = nullptr;
        kv_engine_ = nullptr;
        return TYPE_DRIVER_ERROR_INIT_FAILED;
    }

    initialized_ = true;
    return TYPE_DRIVER_OK;
}

bool TypeDriver::is_initialized() const
{
    return initialized_;
}

// ==================== KV 操作 ====================

int TypeDriver::kv_set(uint64_t key_hash, uint32_t key_crc32, const char *data, size_t data_len)
{
    if (!initialized_ || kv_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return kv_driver_->set(key_hash, key_crc32, data, data_len);
}

int TypeDriver::kv_get(uint64_t key_hash, uint32_t key_crc32, char **ret_data, size_t *ret_len)
{
    if (!initialized_ || kv_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return kv_driver_->get(key_hash, key_crc32, ret_data, ret_len);
}

int TypeDriver::kv_del(uint64_t key_hash, uint32_t key_crc32)
{
    if (!initialized_ || kv_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return kv_driver_->del(key_hash, key_crc32);
}

int TypeDriver::kv_exists(uint64_t key_hash, uint32_t key_crc32)
{
    if (!initialized_ || kv_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return kv_driver_->exists(key_hash, key_crc32);
}

// ==================== Hash 单字段操作 ====================

int TypeDriver::hash_set_m(uint64_t key_hash, uint32_t key_crc32, const hash_field* field)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_set_m(key_hash, key_crc32, field);
}

int TypeDriver::hash_get_m(uint64_t key_hash, uint32_t key_crc32, hash_field* field)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_get_m(key_hash, key_crc32, field);
}

int TypeDriver::hash_del_m(uint64_t key_hash, uint32_t key_crc32,
                           uint64_t field_hash, uint32_t field_crc32)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_del_m(key_hash, key_crc32, field_hash, field_crc32);
}

int TypeDriver::hash_exists_m(uint64_t key_hash, uint32_t key_crc32,
                              uint64_t field_hash, uint32_t field_crc32)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_exists_m(key_hash, key_crc32, field_hash, field_crc32);
}

// ==================== Hash 整体操作 ====================

int TypeDriver::hash_set(uint64_t key_hash, uint32_t key_crc32,
                         const hash_field* fields, size_t field_count)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_set(key_hash, key_crc32, fields, field_count);
}

int TypeDriver::hash_get(uint64_t key_hash, uint32_t key_crc32,
                         hash_field** fields, size_t* field_count)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_get(key_hash, key_crc32, fields, field_count);
}

int TypeDriver::hash_del(uint64_t key_hash, uint32_t key_crc32)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_del(key_hash, key_crc32);
}

int TypeDriver::hash_exists(uint64_t key_hash, uint32_t key_crc32)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_exists(key_hash, key_crc32);
}

int TypeDriver::hash_len(uint64_t key_hash, uint32_t key_crc32, size_t* len)
{
    if (!initialized_ || hash_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return hash_driver_->hash_len(key_hash, key_crc32, len);
}

// ==================== Set 单成员操作 ====================

int TypeDriver::set_add_m(uint64_t key_hash, uint32_t key_crc32, const set_member* member)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_add_m(key_hash, key_crc32, member);
}

int TypeDriver::set_exists_m(uint64_t key_hash, uint32_t key_crc32,
                             uint64_t member_hash, uint32_t member_crc32)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_exists_m(key_hash, key_crc32, member_hash, member_crc32);
}

int TypeDriver::set_del_m(uint64_t key_hash, uint32_t key_crc32,
                          uint64_t member_hash, uint32_t member_crc32)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_del_m(key_hash, key_crc32, member_hash, member_crc32);
}

int TypeDriver::set_get_m(uint64_t key_hash, uint32_t key_crc32, set_member* member)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_get_m(key_hash, key_crc32, member);
}

// ==================== Set 整体操作 ====================

int TypeDriver::set_set(uint64_t key_hash, uint32_t key_crc32,
                        const set_member* members, size_t member_count)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_set(key_hash, key_crc32, members, member_count);
}

int TypeDriver::set_get(uint64_t key_hash, uint32_t key_crc32,
                        set_member** members, size_t* member_count)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_get(key_hash, key_crc32, members, member_count);
}

int TypeDriver::set_del(uint64_t key_hash, uint32_t key_crc32)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_del(key_hash, key_crc32);
}

int TypeDriver::set_exists(uint64_t key_hash, uint32_t key_crc32)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_exists(key_hash, key_crc32);
}

int TypeDriver::set_len(uint64_t key_hash, uint32_t key_crc32, size_t* len)
{
    if (!initialized_ || set_driver_ == nullptr)
    {
        return TYPE_DRIVER_ERROR_NOT_INIT;
    }
    return set_driver_->set_len(key_hash, key_crc32, len);
}
