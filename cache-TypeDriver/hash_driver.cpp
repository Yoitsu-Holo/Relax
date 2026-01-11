#include "hash_driver.h"
#include "../cache-Kernel/kvEngine/kv_engine.h"
#include <cstring>
#include <new>

HashDriver::HashDriver() : engine_(nullptr)
{
}

HashDriver::~HashDriver()
{
    // engine_ 由外部管理，不在这里释放
}

int HashDriver::init(KVEngine *engine)
{
    if (engine == nullptr)
    {
        return HASH_ERROR_NULL_POINTER;
    }
    if (!engine->is_initialized())
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }
    engine_ = engine;
    return HASH_OK;
}

size_t HashDriver::calculate_block_size(size_t required_size)
{
    // 使用 2 的幂次方分配策略
    // 16, 32, 64, 128, 256, 512, 1024, 2048 Byte
    // 4, 8, 16, 32, 64, 128, 256 KiB
    static const size_t size_classes[] = {
        16, 32, 64, 128, 256, 512, 1024, 2048,               // Bytes
        4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, // KiB
        128 * 1024, 256 * 1024                               // KiB
    };
    static const size_t num_classes = sizeof(size_classes) / sizeof(size_classes[0]);

    // 向上取整到最近的大小类别
    for (size_t i = 0; i < num_classes; i++)
    {
        if (required_size <= size_classes[i])
        {
            return size_classes[i];
        }
    }

    // 如果超过最大类别，返回最大类别（256 KiB）
    return size_classes[num_classes - 1];
}

int HashDriver::find_field_in_meta(const hash_meta_data *meta,
                                   uint64_t field_hash, uint32_t field_crc32)
{
    if (meta == nullptr)
    {
        return -1;
    }

    for (size_t i = 0; i < meta->used_count; i++)
    {
        if (meta->entries[i].field_hash == field_hash)
        {
            // 找到了，检查 crc32
            if (meta->entries[i].field_crc32 != field_crc32)
            {
                // CRC 不匹配，hash冲突
                return -2;
            }
            return static_cast<int>(i);
        }
    }

    return -1; // 未找到
}

size_t HashDriver::compact_metadata(const hash_meta_data *old_meta, size_t old_meta_size,
                                    hash_meta_data *new_meta, size_t new_meta_capacity)
{
    if (old_meta == nullptr || new_meta == nullptr)
    {
        return 0;
    }

    size_t write_pos = 0;

    // 遍历所有条目，只保留有效的
    for (size_t i = 0; i < old_meta->used_count; i++)
    {
        if (old_meta->entries[i].field_hash != HASH_ENTRY_EMPTY &&
            old_meta->entries[i].field_hash != HASH_ENTRY_DELETED)
        {
            // 检查是否有足够空间
            if (sizeof(hash_meta_data) + (write_pos + 1) * sizeof(hash_meta_entry) > new_meta_capacity)
            {
                break;
            }
            new_meta->entries[write_pos++] = old_meta->entries[i];
        }
    }

    new_meta->entry_count = write_pos; // 压缩后，有效数量等于写入数量
    new_meta->used_count = write_pos;  // 压缩后，used = entry

    return sizeof(hash_meta_data) + write_pos * sizeof(hash_meta_entry);
}

// ==================== 单字段操作 ====================

int HashDriver::hash_set_m(uint64_t key_hash, uint32_t key_crc32, const hash_field *field)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }
    if (field == nullptr || field->field == nullptr || field->value == nullptr)
    {
        return HASH_ERROR_NULL_POINTER;
    }

    // Step 1: 构建 hash_field_data 结构
    size_t field_data_size = sizeof(hash_field_data) + field->field_len + field->value_len;
    size_t required_size = sizeof(hash_kv_metadata) + field_data_size;
    size_t block_size = calculate_block_size(required_size);

    if (block_size < required_size)
    {
        return HASH_ERROR_INVALID_SIZE;
    }

    // Step 2: 写入数据节点
    char *buffer = new (std::nothrow) char[block_size];
    if (buffer == nullptr)
    {
        return HASH_ERROR_ALLOCATION_FAILED;
    }

    hash_kv_metadata *header = reinterpret_cast<hash_kv_metadata *>(buffer);
    header->key_crc32 = field->field_crc32; // 对于数据节点，存储field的crc32
    header->data_len = field_data_size;
    header->block_size = block_size;
    header->type = TYPE_HASH_DATA;
    header->exp_time = 0;

    hash_field_data *fdata = reinterpret_cast<hash_field_data *>(header->data);
    fdata->field_len = field->field_len;
    fdata->value_len = field->value_len;
    memcpy(fdata->field_value, field->field, field->field_len);
    memcpy(fdata->field_value + field->field_len, field->value, field->value_len);

    int ret = engine_->add(field->field_hash, buffer, block_size);
    delete[] buffer;

    if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    // Step 3: 更新元数据节点
    char *meta_data = nullptr;
    size_t meta_len = 0;
    ret = engine_->get(key_hash, &meta_data, &meta_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        // 元数据不存在，创建新的
        size_t initial_size = 256; // 默认分配 256B，可容纳约12个entry
        char *meta_buffer = new (std::nothrow) char[initial_size];
        if (meta_buffer == nullptr)
        {
            return HASH_ERROR_ALLOCATION_FAILED;
        }

        hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_buffer);
        meta_header->key_crc32 = key_crc32;
        meta_header->data_len = sizeof(hash_meta_data) + sizeof(hash_meta_entry);
        meta_header->block_size = initial_size;
        meta_header->type = TYPE_HASH_META;
        meta_header->exp_time = 0;

        hash_meta_data *meta = reinterpret_cast<hash_meta_data *>(meta_header->data);
        meta->entry_count = 1;
        meta->used_count = 1;
        meta->entries[0] = hash_meta_entry(field->field_hash, field->field_crc32);

        ret = engine_->add(key_hash, meta_buffer, initial_size);
        delete[] meta_buffer;

        if (ret != KV_OK)
        {
            return HASH_ERROR_ENGINE_FAILED;
        }

        return HASH_OK;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    // 元数据存在，检查并更新
    if (meta_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_data);

    // 检查 CRC32
    if (meta_header->key_crc32 != key_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    // 检查类型
    if (meta_header->type != TYPE_HASH_META)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    hash_meta_data *meta = reinterpret_cast<hash_meta_data *>(meta_header->data);

    // 查找 field 是否已存在
    int find_result = find_field_in_meta(meta, field->field_hash, field->field_crc32);

    if (find_result >= 0)
    {
        // 找到了，是更新操作，数据节点已经写入，元数据无需修改
        return HASH_OK;
    }
    else if (find_result == -2)
    {
        // CRC 不匹配
        return HASH_ERROR_CRC_MISMATCH;
    }

    // 未找到，是新增操作，需要添加到元数据
    size_t current_block_size = meta_header->block_size;
    size_t used_space = sizeof(hash_kv_metadata) + sizeof(hash_meta_data) +
                        meta->used_count * sizeof(hash_meta_entry);
    size_t free_space = current_block_size - used_space;

    if (free_space >= sizeof(hash_meta_entry))
    {
        // 空间足够，直接添加
        meta->entries[meta->used_count] = hash_meta_entry(field->field_hash, field->field_crc32);
        meta->entry_count++;
        meta->used_count++;
        meta_header->data_len = sizeof(hash_meta_data) + meta->used_count * sizeof(hash_meta_entry);

        ret = engine_->add(key_hash, meta_data, current_block_size);
        if (ret != KV_OK)
        {
            return HASH_ERROR_ENGINE_FAILED;
        }

        return HASH_OK;
    }

    // 空间不足，先尝试整理
    size_t new_block_size = current_block_size;
    char *new_meta_buffer = new (std::nothrow) char[new_block_size];
    if (new_meta_buffer == nullptr)
    {
        return HASH_ERROR_ALLOCATION_FAILED;
    }

    hash_kv_metadata *new_meta_header = reinterpret_cast<hash_kv_metadata *>(new_meta_buffer);
    memcpy(new_meta_header, meta_header, sizeof(hash_kv_metadata));

    hash_meta_data *new_meta = reinterpret_cast<hash_meta_data *>(new_meta_header->data);
    size_t compacted_size = compact_metadata(meta, meta_header->data_len,
                                             new_meta, new_block_size - sizeof(hash_kv_metadata));

    // 检查整理后是否有空间
    size_t compacted_used_space = sizeof(hash_kv_metadata) + compacted_size;
    if (new_block_size - compacted_used_space >= sizeof(hash_meta_entry))
    {
        // 整理后有空间，添加新entry
        new_meta->entries[new_meta->used_count] = hash_meta_entry(field->field_hash, field->field_crc32);
        new_meta->entry_count++;
        new_meta->used_count++;
        new_meta_header->data_len = sizeof(hash_meta_data) + new_meta->used_count * sizeof(hash_meta_entry);

        ret = engine_->add(key_hash, new_meta_buffer, new_block_size);
        delete[] new_meta_buffer;

        if (ret != KV_OK)
        {
            return HASH_ERROR_ENGINE_FAILED;
        }

        return HASH_OK;
    }

    // 整理后还是不够，需要扩容
    delete[] new_meta_buffer;
    new_block_size = calculate_block_size(current_block_size + 1); // 确保扩容
    if (new_block_size <= current_block_size)
    {
        new_block_size = current_block_size * 2;
    }

    new_meta_buffer = new (std::nothrow) char[new_block_size];
    if (new_meta_buffer == nullptr)
    {
        return HASH_ERROR_ALLOCATION_FAILED;
    }

    new_meta_header = reinterpret_cast<hash_kv_metadata *>(new_meta_buffer);
    memcpy(new_meta_header, meta_header, sizeof(hash_kv_metadata));
    new_meta_header->block_size = new_block_size;

    new_meta = reinterpret_cast<hash_meta_data *>(new_meta_header->data);
    compacted_size = compact_metadata(meta, meta_header->data_len,
                                      new_meta, new_block_size - sizeof(hash_kv_metadata));

    // 添加新entry
    new_meta->entries[new_meta->used_count] = hash_meta_entry(field->field_hash, field->field_crc32);
    new_meta->entry_count++;
    new_meta->used_count++;
    new_meta_header->data_len = sizeof(hash_meta_data) + new_meta->used_count * sizeof(hash_meta_entry);

    ret = engine_->add(key_hash, new_meta_buffer, new_block_size);
    delete[] new_meta_buffer;

    if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    return HASH_OK;
}

int HashDriver::hash_get_m(uint64_t key_hash, uint32_t key_crc32, hash_field *field)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }
    if (field == nullptr)
    {
        return HASH_ERROR_NULL_POINTER;
    }

    // Step 1: 调用 kvEngine->get(field_hash) 直接获取数据节点
    char *data = nullptr;
    size_t data_len = 0;
    int ret = engine_->get(field->field_hash, &data, &data_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        return HASH_ERROR_FIELD_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    // Step 2: 检查数据完整性
    if (data_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *header = reinterpret_cast<hash_kv_metadata *>(data);

    // Step 3: 校验 field_crc32
    if (header->key_crc32 != field->field_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    // 检查类型
    if (header->type != TYPE_HASH_DATA)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    // Step 4: 解析 hash_field_data，提取 value
    hash_field_data *fdata = reinterpret_cast<hash_field_data *>(header->data);

    // Step 5: 分配内存拷贝 value
    char *value_copy = new (std::nothrow) char[fdata->value_len];
    if (value_copy == nullptr)
    {
        return HASH_ERROR_ALLOCATION_FAILED;
    }

    memcpy(value_copy, fdata->field_value + fdata->field_len, fdata->value_len);

    field->value = value_copy;
    field->value_len = fdata->value_len;

    return HASH_OK;
}

int HashDriver::hash_del_m(uint64_t key_hash, uint32_t key_crc32,
                           uint64_t field_hash, uint32_t field_crc32)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }

    // Step 1: 检查数据节点
    char *data = nullptr;
    size_t data_len = 0;
    int ret = engine_->get(field_hash, &data, &data_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        return HASH_ERROR_FIELD_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (data_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *header = reinterpret_cast<hash_kv_metadata *>(data);

    // 校验 field_crc32
    if (header->key_crc32 != field_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (header->type != TYPE_HASH_DATA)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    // Step 2: 删除数据节点
    ret = engine_->del(field_hash);
    if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    // Step 3: 更新元数据节点
    char *meta_data = nullptr;
    size_t meta_len = 0;
    ret = engine_->get(key_hash, &meta_data, &meta_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        // 元数据不存在，但数据节点存在（不一致状态），已删除数据节点，返回成功
        return HASH_OK;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (meta_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_data);

    // 检查 CRC32
    if (meta_header->key_crc32 != key_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (meta_header->type != TYPE_HASH_META)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    hash_meta_data *meta = reinterpret_cast<hash_meta_data *>(meta_header->data);

    // 查找并标记为删除
    int find_result = find_field_in_meta(meta, field_hash, field_crc32);

    if (find_result >= 0)
    {
        // 找到了，标记为删除
        meta->entries[find_result].field_hash = HASH_ENTRY_DELETED;
        meta->entry_count--;

        ret = engine_->add(key_hash, meta_data, meta_header->block_size);
        if (ret != KV_OK)
        {
            return HASH_ERROR_ENGINE_FAILED;
        }
    }
    else if (find_result == -2)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }
    // 如果未找到，也返回成功（数据节点已删除）

    return HASH_OK;
}

int HashDriver::hash_exists_m(uint64_t key_hash, uint32_t key_crc32,
                              uint64_t field_hash, uint32_t field_crc32)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }

    // 调用 kvEngine->get(field_hash)
    char *data = nullptr;
    size_t data_len = 0;
    int ret = engine_->get(field_hash, &data, &data_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        return HASH_ERROR_FIELD_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (data_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *header = reinterpret_cast<hash_kv_metadata *>(data);

    // 校验 field_crc32
    if (header->key_crc32 != field_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (header->type != TYPE_HASH_DATA)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    return HASH_OK;
}

// ==================== 整体操作 ====================

int HashDriver::hash_set(uint64_t key_hash, uint32_t key_crc32,
                         const hash_field *fields, size_t field_count)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }
    if (fields == nullptr && field_count > 0)
    {
        return HASH_ERROR_NULL_POINTER;
    }

    // 逐个调用 hash_set_m
    for (size_t i = 0; i < field_count; i++)
    {
        int ret = hash_set_m(key_hash, key_crc32, &fields[i]);
        if (ret != HASH_OK)
        {
            return ret;
        }
    }

    return HASH_OK;
}

int HashDriver::hash_get(uint64_t key_hash, uint32_t key_crc32,
                         hash_field **fields, size_t *field_count)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }
    if (fields == nullptr || field_count == nullptr)
    {
        return HASH_ERROR_NULL_POINTER;
    }

    // Step 1: 获取元数据节点
    char *meta_data = nullptr;
    size_t meta_len = 0;
    int ret = engine_->get(key_hash, &meta_data, &meta_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        *fields = nullptr;
        *field_count = 0;
        return HASH_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (meta_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_data);

    // Step 2: 校验 key_crc32
    if (meta_header->key_crc32 != key_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (meta_header->type != TYPE_HASH_META)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    hash_meta_data *meta = reinterpret_cast<hash_meta_data *>(meta_header->data);

    // Step 3: 分配结果数组
    if (meta->entry_count == 0)
    {
        *fields = nullptr;
        *field_count = 0;
        return HASH_OK;
    }

    hash_field *result = new (std::nothrow) hash_field[meta->entry_count];
    if (result == nullptr)
    {
        return HASH_ERROR_ALLOCATION_FAILED;
    }

    // Step 4: 遍历所有 entries，获取数据
    size_t result_idx = 0;
    for (size_t i = 0; i < meta->used_count && result_idx < meta->entry_count; i++)
    {
        if (meta->entries[i].field_hash == HASH_ENTRY_EMPTY ||
            meta->entries[i].field_hash == HASH_ENTRY_DELETED)
        {
            continue;
        }

        // 获取数据节点
        char *field_data = nullptr;
        size_t field_data_len = 0;
        ret = engine_->get(meta->entries[i].field_hash, &field_data, &field_data_len);

        if (ret != KV_OK || field_data_len < sizeof(hash_kv_metadata))
        {
            // 跳过损坏的数据
            continue;
        }

        hash_kv_metadata *field_header = reinterpret_cast<hash_kv_metadata *>(field_data);
        if (field_header->type != TYPE_HASH_DATA)
        {
            continue;
        }

        hash_field_data *fdata = reinterpret_cast<hash_field_data *>(field_header->data);

        // 分配并拷贝 field 和 value
        char *field_copy = new (std::nothrow) char[fdata->field_len];
        char *value_copy = new (std::nothrow) char[fdata->value_len];

        if (field_copy == nullptr || value_copy == nullptr)
        {
            delete[] field_copy;
            delete[] value_copy;
            // 释放已分配的内存
            for (size_t j = 0; j < result_idx; j++)
            {
                delete[] result[j].field;
                delete[] result[j].value;
            }
            delete[] result;
            return HASH_ERROR_ALLOCATION_FAILED;
        }

        memcpy(field_copy, fdata->field_value, fdata->field_len);
        memcpy(value_copy, fdata->field_value + fdata->field_len, fdata->value_len);

        result[result_idx].field_hash = meta->entries[i].field_hash;
        result[result_idx].field_crc32 = meta->entries[i].field_crc32;
        result[result_idx].field = field_copy;
        result[result_idx].field_len = fdata->field_len;
        result[result_idx].value = value_copy;
        result[result_idx].value_len = fdata->value_len;

        result_idx++;
    }

    *fields = result;
    *field_count = result_idx;

    return HASH_OK;
}

int HashDriver::hash_del(uint64_t key_hash, uint32_t key_crc32)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }

    // Step 1: 获取元数据节点
    char *meta_data = nullptr;
    size_t meta_len = 0;
    int ret = engine_->get(key_hash, &meta_data, &meta_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        return HASH_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (meta_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_data);

    // Step 2: 校验 key_crc32
    if (meta_header->key_crc32 != key_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (meta_header->type != TYPE_HASH_META)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    hash_meta_data *meta = reinterpret_cast<hash_meta_data *>(meta_header->data);

    // Step 3: 遍历所有 entries，删除数据节点
    for (size_t i = 0; i < meta->used_count; i++)
    {
        if (meta->entries[i].field_hash != HASH_ENTRY_EMPTY &&
            meta->entries[i].field_hash != HASH_ENTRY_DELETED)
        {
            // 删除数据节点（忽略错误，继续删除其他节点）
            engine_->del(meta->entries[i].field_hash);
        }
    }

    // Step 4: 删除元数据节点
    ret = engine_->del(key_hash);
    if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    return HASH_OK;
}

int HashDriver::hash_exists(uint64_t key_hash, uint32_t key_crc32)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }

    // 调用 kvEngine->get(key_hash)
    char *meta_data = nullptr;
    size_t meta_len = 0;
    int ret = engine_->get(key_hash, &meta_data, &meta_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        return HASH_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (meta_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_data);

    // 校验 key_crc32
    if (meta_header->key_crc32 != key_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (meta_header->type != TYPE_HASH_META)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    return HASH_OK;
}

int HashDriver::hash_len(uint64_t key_hash, uint32_t key_crc32, size_t *len)
{
    if (engine_ == nullptr)
    {
        return HASH_ERROR_ENGINE_NOT_INIT;
    }
    if (len == nullptr)
    {
        return HASH_ERROR_NULL_POINTER;
    }

    // 获取元数据节点
    char *meta_data = nullptr;
    size_t meta_len = 0;
    int ret = engine_->get(key_hash, &meta_data, &meta_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        *len = 0;
        return HASH_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    if (meta_len < sizeof(hash_kv_metadata))
    {
        return HASH_ERROR_ENGINE_FAILED;
    }

    hash_kv_metadata *meta_header = reinterpret_cast<hash_kv_metadata *>(meta_data);

    // 校验 key_crc32
    if (meta_header->key_crc32 != key_crc32)
    {
        return HASH_ERROR_CRC_MISMATCH;
    }

    if (meta_header->type != TYPE_HASH_META)
    {
        return HASH_ERROR_TYPE_MISMATCH;
    }

    hash_meta_data *meta = reinterpret_cast<hash_meta_data *>(meta_header->data);

    *len = meta->entry_count;

    return HASH_OK;
}
