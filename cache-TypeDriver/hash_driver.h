#ifndef HASH_DRIVER_H
#define HASH_DRIVER_H

#include <cstddef>
#include <cstdint>

// 前置声明
class KVEngine;

// Hash 类型的数据类型定义
#define TYPE_HASH_META 1  // Hash元数据节点
#define TYPE_HASH_DATA 2  // Hash数据节点

// Hash Entry 标记
#define HASH_ENTRY_EMPTY   0ULL       // 空位标记
#define HASH_ENTRY_DELETED (~0ULL)    // 删除标记

// Hash 驱动错误码
enum HashDriverError
{
    HASH_OK = 0,
    HASH_ERROR_NULL_POINTER = -1,       // 空指针错误
    HASH_ERROR_ENGINE_NOT_INIT = -2,    // 引擎未初始化
    HASH_ERROR_CRC_MISMATCH = -3,       // CRC校验不匹配（hash冲突）
    HASH_ERROR_KEY_NOT_FOUND = -4,      // 键未找到
    HASH_ERROR_FIELD_NOT_FOUND = -5,    // 字段未找到
    HASH_ERROR_ALLOCATION_FAILED = -6,  // 内存分配失败
    HASH_ERROR_ENGINE_FAILED = -7,      // 底层引擎操作失败
    HASH_ERROR_INVALID_SIZE = -8,       // 无效的大小参数
    HASH_ERROR_TYPE_MISMATCH = -9       // 数据类型不匹配
};

// Hash 元数据节点中的条目
struct hash_meta_entry
{
    uint64_t field_hash;     // field的hash值（0表示空位，~0ULL表示已删除）
    uint32_t field_crc32;    // field的crc32校验值
    uint32_t reserved;       // 预留字段

    hash_meta_entry() : field_hash(HASH_ENTRY_EMPTY), field_crc32(0), reserved(0) {}
    hash_meta_entry(uint64_t fh, uint32_t fc) : field_hash(fh), field_crc32(fc), reserved(0) {}
};

// Hash 元数据节点的 data 部分结构
struct hash_meta_data
{
    uint32_t entry_count;    // 当前有效的field数量（不含已删除）
    uint32_t used_count;     // 已使用的entry槽位数量（含已删除）
    hash_meta_entry entries[]; // 动态数组

    hash_meta_data() : entry_count(0), used_count(0) {}
};

// Hash 数据节点的 data 部分结构
struct hash_field_data
{
    uint32_t field_len;      // field字符串长度
    uint32_t value_len;      // value字符串长度
    char field_value[];      // field和value连续存储: [field][value]

    hash_field_data() : field_len(0), value_len(0) {}
};

// 用户接口的数据结构
struct hash_field
{
    uint64_t field_hash;     // field的hash值
    uint32_t field_crc32;    // field的crc32校验值
    const char* field;       // field字符串（输入时使用，输出时会分配新内存）
    size_t field_len;
    const char* value;       // value字符串（输入时使用，输出时会分配新内存）
    size_t value_len;

    hash_field() : field_hash(0), field_crc32(0), field(nullptr), field_len(0),
                   value(nullptr), value_len(0) {}
};

// 存储在每个条目中的元数据结构（与 KVDriver 中的 KVMetadata 相同）
struct hash_kv_metadata
{
    uint32_t key_crc32;       // 键的CRC32值（对于数据节点，存储field的crc32）
    uint32_t data_len;        // 实际存储的数据长度（不含header）
    uint32_t block_size : 24; // 分配的总块大小（含header），最大 16MiB-1
    uint32_t type : 8;        // 数据类型：1=HASH_META, 2=HASH_DATA
    uint32_t exp_time;        // 过期时间（秒），预留供将来使用
    char data[];              // 柔性数组成员，存储实际数据
};

// HashDriver - Hash 类型的驱动
// 实现 Redis-like 的 Hash 数据结构
class HashDriver
{
public:
    HashDriver();
    ~HashDriver();

    // 禁用拷贝和赋值
    HashDriver(const HashDriver &) = delete;
    HashDriver &operator=(const HashDriver &) = delete;

    // 使用 KVEngine 实例初始化 Hash 驱动
    // @param engine: 指向已初始化的 KVEngine 的指针
    // @return 成功返回 HASH_OK，失败返回错误码
    int init(KVEngine *engine);

    // ==================== 单字段操作（O(1)） ====================

    // 设置单个field（O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param field: 字段信息（包含field_hash, field_crc32, field, field_len, value, value_len）
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_set_m(uint64_t key_hash, uint32_t key_crc32, const hash_field* field);

    // 获取单个field的value（O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param field: 输入field信息（field_hash, field_crc32, field, field_len），
    //              输出value信息（value, value_len），调用者负责释放value内存
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_get_m(uint64_t key_hash, uint32_t key_crc32, hash_field* field);

    // 删除单个field（O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param field_hash: field的hash值
    // @param field_crc32: field的crc32值
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_del_m(uint64_t key_hash, uint32_t key_crc32,
                   uint64_t field_hash, uint32_t field_crc32);

    // 检查field是否存在（O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param field_hash: field的hash值
    // @param field_crc32: field的crc32值
    // @return 存在返回 HASH_OK，不存在返回 HASH_ERROR_FIELD_NOT_FOUND，
    //         CRC不匹配返回 HASH_ERROR_CRC_MISMATCH
    int hash_exists_m(uint64_t key_hash, uint32_t key_crc32,
                      uint64_t field_hash, uint32_t field_crc32);

    // ==================== 整体操作（O(n)） ====================

    // 批量设置多个field（会逐个调用hash_set_m）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param fields: 字段数组
    // @param field_count: 字段数量
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_set(uint64_t key_hash, uint32_t key_crc32,
                 const hash_field* fields, size_t field_count);

    // 获取hash的所有field-value对
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param fields: 输出参数，字段数组（调用者负责释放）
    // @param field_count: 输出参数，字段数量
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_get(uint64_t key_hash, uint32_t key_crc32,
                 hash_field** fields, size_t* field_count);

    // 删除整个hash（包括所有field数据节点）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_del(uint64_t key_hash, uint32_t key_crc32);

    // 检查hash是否存在
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @return 存在返回 HASH_OK，不存在返回 HASH_ERROR_KEY_NOT_FOUND，
    //         CRC不匹配返回 HASH_ERROR_CRC_MISMATCH
    int hash_exists(uint64_t key_hash, uint32_t key_crc32);

    // 获取hash中field的数量
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param len: 输出参数，字段数量
    // @return 成功返回 HASH_OK，失败返回错误码
    int hash_len(uint64_t key_hash, uint32_t key_crc32, size_t* len);

private:
    KVEngine *engine_;

    // 根据所需大小计算要分配的块大小
    // 使用 2 的幂次方分配策略
    // @param required_size: 所需大小（包括元数据）
    // @return 实际要分配的块大小
    size_t calculate_block_size(size_t required_size);

    // 元数据整理（压缩）
    // 移除所有标记为 DELETED 的条目
    // @param old_meta: 旧的元数据
    // @param old_meta_size: 旧元数据的大小（字节数）
    // @param new_meta: 新的元数据缓冲区
    // @param new_meta_capacity: 新元数据缓冲区的容量
    // @return 整理后使用的字节数
    size_t compact_metadata(const hash_meta_data* old_meta, size_t old_meta_size,
                           hash_meta_data* new_meta, size_t new_meta_capacity);

    // 在元数据中查找field
    // @param meta: 元数据
    // @param field_hash: field的hash值
    // @param field_crc32: field的crc32值
    // @return 找到返回索引，未找到返回 -1，CRC不匹配返回 -2
    int find_field_in_meta(const hash_meta_data* meta,
                          uint64_t field_hash, uint32_t field_crc32);
};

#endif // HASH_DRIVER_H
