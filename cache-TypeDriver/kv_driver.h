#ifndef KV_DRIVER_H
#define KV_DRIVER_H

#include <cstddef>
#include <cstdint>

// 前置声明
class KVEngine;

// 数据类型枚举
enum DataType
{
    DATA_TYPE_KV = 0,   // KV类型
    DATA_TYPE_HASH = 1, // Hash类型
    DATA_TYPE_SET = 2,  // Set类型
    DATA_TYPE_LIST = 3  // List类型
};

// KV驱动错误码
enum KVDriverError
{
    KVD_OK = 0,
    KVD_ERROR_NULL_POINTER = -1,      // 空指针错误
    KVD_ERROR_ENGINE_NOT_INIT = -2,   // 引擎未初始化
    KVD_ERROR_CRC_MISMATCH = -3,      // CRC校验不匹配（hash冲突）
    KVD_ERROR_KEY_NOT_FOUND = -4,     // 键未找到
    KVD_ERROR_ALLOCATION_FAILED = -5, // 内存分配失败
    KVD_ERROR_ENGINE_FAILED = -6,     // 底层引擎操作失败
    KVD_ERROR_INVALID_SIZE = -7,      // 无效的大小参数
    KVD_ERROR_TYPE_MISMATCH = -8      // 数据类型不匹配
};

// 存储在每个KV条目中的元数据结构
struct KVMetadata
{
    uint32_t key_crc32;       // 键的CRC32值，用于检测hash冲突
    uint32_t data_len;        // 实际存储的数据长度
    uint32_t block_size : 24; // 分配的总块大小（容量），最大 16MiB-1
    uint32_t type : 8;        // 数据类型：0=kv, 1=hash, 2=set, 3=list
    uint32_t exp_time;        // 过期时间（秒），预留供将来使用
    char data[];              // 柔性数组成员，存储实际数据
};

// KVDriver - 简单键值存储的类型驱动
// 封装 KVEngine，提供元数据管理和 CRC32 校验功能
class KVDriver
{
public:
    KVDriver();
    ~KVDriver();

    // 禁用拷贝和赋值
    KVDriver(const KVDriver &) = delete;
    KVDriver &operator=(const KVDriver &) = delete;

    // 使用 KVEngine 实例初始化 KV 驱动
    // @param engine: 指向已初始化的 KVEngine 的指针
    // @return 成功返回 KVD_OK，失败返回错误码
    int init(KVEngine *engine);

    // 设置键值对
    // 如果键存在且 CRC 匹配，则更新值
    // 如果键存在但 CRC 不匹配，返回错误（hash冲突）
    // @param key_hash: 键的 uint64 哈希值
    // @param key_crc32: 键的 CRC32 值，用于冲突检测
    // @param data: 指向要存储数据的指针
    // @param data_len: 数据长度
    // @return 成功返回 KVD_OK，失败返回错误码
    int set(uint64_t key_hash, uint32_t key_crc32, const char *data, size_t data_len);

    // 根据键获取值
    // @param key_hash: 键的 uint64 哈希值
    // @param key_crc32: 键的 CRC32 值，用于冲突检测
    // @param ret_data: 输出参数，指向数据的指针（不要释放此指针）
    // @param ret_len: 输出参数，数据长度
    // @return 成功返回 KVD_OK，失败返回错误码
    int get(uint64_t key_hash, uint32_t key_crc32, char **ret_data, size_t *ret_len);

    // 删除键值对
    // @param key_hash: 键的 uint64 哈希值
    // @param key_crc32: 键的 CRC32 值，用于冲突检测
    // @return 成功返回 KVD_OK，失败返回错误码
    int del(uint64_t key_hash, uint32_t key_crc32);

    // 检查键是否存在
    // @param key_hash: 键的 uint64 哈希值
    // @param key_crc32: 键的 CRC32 值，用于冲突检测
    // @return 存在返回 KVD_OK，不存在返回 KVD_ERROR_KEY_NOT_FOUND，CRC不匹配返回 KVD_ERROR_CRC_MISMATCH
    int exists(uint64_t key_hash, uint32_t key_crc32);

private:
    KVEngine *engine_;

    // 根据所需大小计算要分配的块大小
    // 使用固定的大小类别：16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072 字节
    // 以及 4, 8, 16, 32, 64, 128, 256 KiB
    // @param required_size: 所需大小（包括元数据）
    // @return 实际要分配的块大小
    size_t calculate_block_size(size_t required_size);
};

#endif // KV_DRIVER_H
