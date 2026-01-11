#include "kv_driver.h"
#include "../cache-Kernel/kvEngine/kv_engine.h"
#include <cstring>

KVDriver::KVDriver() : engine_(nullptr)
{
}

KVDriver::~KVDriver()
{
    // engine_ 由外部管理，不在这里释放
}

int KVDriver::init(KVEngine *engine)
{
    if (engine == nullptr)
    {
        return KVD_ERROR_NULL_POINTER;
    }
    if (!engine->is_initialized())
    {
        return KVD_ERROR_ENGINE_NOT_INIT;
    }
    engine_ = engine;
    return KVD_OK;
}

size_t KVDriver::calculate_block_size(size_t required_size)
{
    // 定义所有的大小类别（字节）
    static const size_t size_classes[] = {
        16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072,        // Bytes
        4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024  // KiB
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

int KVDriver::set(uint64_t key_hash, uint32_t key_crc32, const char *data, size_t data_len)
{
    if (engine_ == nullptr)
    {
        return KVD_ERROR_ENGINE_NOT_INIT;
    }
    if (data == nullptr && data_len > 0)
    {
        return KVD_ERROR_NULL_POINTER;
    }

    // Step 1: 检查键是否已存在
    char *existing_data = nullptr;
    size_t existing_len = 0;
    int ret = engine_->get(key_hash, &existing_data, &existing_len);

    if (ret == KV_OK)
    {
        // 键存在，检查 CRC32 是否匹配
        if (existing_len < sizeof(KVMetadata))
        {
            return KVD_ERROR_ENGINE_FAILED;  // 数据损坏
        }
        KVMetadata *metadata = reinterpret_cast<KVMetadata *>(existing_data);
        if (metadata->key_crc32 != key_crc32)
        {
            // CRC 不匹配，说明发生了 hash 冲突
            return KVD_ERROR_CRC_MISMATCH;
        }
        // 检查类型是否匹配
        if (metadata->type != DATA_TYPE_KV)
        {
            // 类型不匹配，不能操作
            return KVD_ERROR_TYPE_MISMATCH;
        }
        // CRC 匹配，类型正确，可以继续更新
    }
    else if (ret != KV_ERROR_KEY_NOT_FOUND)
    {
        // 其他错误
        return KVD_ERROR_ENGINE_FAILED;
    }
    // 如果键不存在（KV_ERROR_KEY_NOT_FOUND），继续创建新条目

    // Step 2: 计算需要的块大小
    size_t required_size = sizeof(KVMetadata) + data_len;
    size_t block_size = calculate_block_size(required_size);

    if (block_size < required_size)
    {
        // 数据太大，超过了最大支持的大小
        return KVD_ERROR_INVALID_SIZE;
    }

    // Step 3: 构建新的元数据和数据
    char *buffer = new (std::nothrow) char[block_size];
    if (buffer == nullptr)
    {
        return KVD_ERROR_ALLOCATION_FAILED;
    }

    KVMetadata *metadata = reinterpret_cast<KVMetadata *>(buffer);
    metadata->key_crc32 = key_crc32;
    metadata->data_len = data_len;
    metadata->block_size = block_size;
    metadata->type = DATA_TYPE_KV;  // 设置为 KV 类型
    metadata->exp_time = 0;         // 暂时不使用过期时间

    // 复制数据
    if (data_len > 0)
    {
        memcpy(metadata->data, data, data_len);
    }

    // Step 4: 调用底层 kvEngine 的 add 方法
    ret = engine_->add(key_hash, buffer, block_size);
    delete[] buffer;

    if (ret != KV_OK)
    {
        return KVD_ERROR_ENGINE_FAILED;
    }

    return KVD_OK;
}

int KVDriver::get(uint64_t key_hash, uint32_t key_crc32, char **ret_data, size_t *ret_len)
{
    if (engine_ == nullptr)
    {
        return KVD_ERROR_ENGINE_NOT_INIT;
    }
    if (ret_data == nullptr || ret_len == nullptr)
    {
        return KVD_ERROR_NULL_POINTER;
    }

    // 调用 kvEngine 的 get 方法
    char *raw_data = nullptr;
    size_t raw_len = 0;
    int ret = engine_->get(key_hash, &raw_data, &raw_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        // 键不存在，返回空指针和 0 长度
        *ret_data = nullptr;
        *ret_len = 0;
        return KVD_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return KVD_ERROR_ENGINE_FAILED;
    }

    // 检查数据完整性
    if (raw_len < sizeof(KVMetadata))
    {
        return KVD_ERROR_ENGINE_FAILED;  // 数据损坏
    }

    // 检查 CRC32 是否匹配
    KVMetadata *metadata = reinterpret_cast<KVMetadata *>(raw_data);
    if (metadata->key_crc32 != key_crc32)
    {
        // CRC 不匹配，说明发生了 hash 冲突
        return KVD_ERROR_CRC_MISMATCH;
    }

    // 检查类型是否匹配
    if (metadata->type != DATA_TYPE_KV)
    {
        // 类型不匹配
        return KVD_ERROR_TYPE_MISMATCH;
    }

    // CRC 匹配，类型正确，返回数据部分
    *ret_data = metadata->data;
    *ret_len = metadata->data_len;

    return KVD_OK;
}

int KVDriver::del(uint64_t key_hash, uint32_t key_crc32)
{
    if (engine_ == nullptr)
    {
        return KVD_ERROR_ENGINE_NOT_INIT;
    }

    // Step 1: 先获取数据，检查 CRC32 是否匹配
    char *raw_data = nullptr;
    size_t raw_len = 0;
    int ret = engine_->get(key_hash, &raw_data, &raw_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        // 键不存在
        return KVD_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return KVD_ERROR_ENGINE_FAILED;
    }

    // 检查数据完整性
    if (raw_len < sizeof(KVMetadata))
    {
        return KVD_ERROR_ENGINE_FAILED;  // 数据损坏
    }

    // 检查 CRC32 是否匹配（防止 hash 冲突误删除）
    KVMetadata *metadata = reinterpret_cast<KVMetadata *>(raw_data);
    if (metadata->key_crc32 != key_crc32)
    {
        // CRC 不匹配，说明发生了 hash 冲突，不能删除
        return KVD_ERROR_CRC_MISMATCH;
    }

    // 检查类型是否匹配
    if (metadata->type != DATA_TYPE_KV)
    {
        // 类型不匹配，不能删除
        return KVD_ERROR_TYPE_MISMATCH;
    }

    // Step 2: CRC 匹配，类型正确，调用 kvEngine 的 del 方法删除
    ret = engine_->del(key_hash);
    if (ret != KV_OK)
    {
        return KVD_ERROR_ENGINE_FAILED;
    }

    return KVD_OK;
}

int KVDriver::exists(uint64_t key_hash, uint32_t key_crc32)
{
    if (engine_ == nullptr)
    {
        return KVD_ERROR_ENGINE_NOT_INIT;
    }

    // 调用 kvEngine 的 get 方法检查键是否存在
    char *raw_data = nullptr;
    size_t raw_len = 0;
    int ret = engine_->get(key_hash, &raw_data, &raw_len);

    if (ret == KV_ERROR_KEY_NOT_FOUND)
    {
        // 键不存在
        return KVD_ERROR_KEY_NOT_FOUND;
    }
    else if (ret != KV_OK)
    {
        return KVD_ERROR_ENGINE_FAILED;
    }

    // 检查数据完整性
    if (raw_len < sizeof(KVMetadata))
    {
        return KVD_ERROR_ENGINE_FAILED;  // 数据损坏
    }

    // 检查 CRC32 是否匹配
    KVMetadata *metadata = reinterpret_cast<KVMetadata *>(raw_data);
    if (metadata->key_crc32 != key_crc32)
    {
        // CRC 不匹配，说明发生了 hash 冲突
        return KVD_ERROR_CRC_MISMATCH;
    }

    // 检查类型是否匹配
    if (metadata->type != DATA_TYPE_KV)
    {
        // 类型不匹配
        return KVD_ERROR_TYPE_MISMATCH;
    }

    // 键存在且 CRC32 匹配，类型正确
    return KVD_OK;
}
