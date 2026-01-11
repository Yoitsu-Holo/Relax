#ifndef SET_DRIVER_H
#define SET_DRIVER_H

#include <cstddef>
#include <cstdint>

// 前置声明
class KVEngine;
class HashDriver;

// Set 驱动错误码
enum SetDriverError
{
    SET_OK = 0,
    SET_ERROR_NULL_POINTER = -1,       // 空指针错误
    SET_ERROR_ENGINE_NOT_INIT = -2,    // 引擎未初始化
    SET_ERROR_CRC_MISMATCH = -3,       // CRC校验不匹配（hash冲突）
    SET_ERROR_KEY_NOT_FOUND = -4,      // 键未找到
    SET_ERROR_MEMBER_NOT_FOUND = -5,   // 成员未找到
    SET_ERROR_ALLOCATION_FAILED = -6,  // 内存分配失败
    SET_ERROR_ENGINE_FAILED = -7,      // 底层引擎操作失败
    SET_ERROR_INVALID_SIZE = -8,       // 无效的大小参数
    SET_ERROR_TYPE_MISMATCH = -9       // 数据类型不匹配
};

// 用户接口的数据结构
struct set_member
{
    uint64_t member_hash;    // member的hash值
    uint32_t member_crc32;   // member的crc32校验值
    const char* member;      // member字符串（输入时使用，输出时会分配新内存）
    size_t member_len;

    set_member() : member_hash(0), member_crc32(0), member(nullptr), member_len(0) {}
};

// SetDriver - Set 类型的驱动
// 实现 Redis-like 的 Set 数据结构
// 内部通过复用 HashDriver 实现（Set 是 value 为空的特化 Hash）
class SetDriver
{
public:
    SetDriver();
    ~SetDriver();

    // 禁用拷贝和赋值
    SetDriver(const SetDriver &) = delete;
    SetDriver &operator=(const SetDriver &) = delete;

    // 使用 KVEngine 实例初始化 Set 驱动
    // @param engine: 指向已初始化的 KVEngine 的指针
    // @return 成功返回 SET_OK，失败返回错误码
    int init(KVEngine *engine);

    // ==================== 单成员操作（O(1)） ====================

    // 添加单个member（O(1)操作，幂等）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param member: 成员信息（包含member_hash, member_crc32, member, member_len）
    // @return 成功返回 SET_OK，失败返回错误码
    int set_add_m(uint64_t key_hash, uint32_t key_crc32, const set_member* member);

    // 检查单个member是否存在（O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param member_hash: member的hash值
    // @param member_crc32: member的crc32值
    // @return 存在返回 SET_OK，不存在返回 SET_ERROR_MEMBER_NOT_FOUND，
    //         CRC不匹配返回 SET_ERROR_CRC_MISMATCH
    int set_exists_m(uint64_t key_hash, uint32_t key_crc32,
                     uint64_t member_hash, uint32_t member_crc32);

    // 删除单个member（O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param member_hash: member的hash值
    // @param member_crc32: member的crc32值
    // @return 成功返回 SET_OK，失败返回错误码
    int set_del_m(uint64_t key_hash, uint32_t key_crc32,
                  uint64_t member_hash, uint32_t member_crc32);

    // 获取单个member的数据（主要用于验证，O(1)操作）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param member: 输入member信息（member_hash, member_crc32），
    //               输出member信息（member, member_len），调用者负责释放member内存
    // @return 成功返回 SET_OK，失败返回错误码
    int set_get_m(uint64_t key_hash, uint32_t key_crc32, set_member* member);

    // ==================== 整体操作（O(n)） ====================

    // 批量添加多个member（会逐个调用set_add_m）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param members: 成员数组
    // @param member_count: 成员数量
    // @return 成功返回 SET_OK，失败返回错误码
    int set_set(uint64_t key_hash, uint32_t key_crc32,
                const set_member* members, size_t member_count);

    // 获取set的所有member
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param members: 输出参数，成员数组（调用者负责释放）
    // @param member_count: 输出参数，成员数量
    // @return 成功返回 SET_OK，失败返回错误码
    int set_get(uint64_t key_hash, uint32_t key_crc32,
                set_member** members, size_t* member_count);

    // 删除整个set（包括所有member数据节点）
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @return 成功返回 SET_OK，失败返回错误码
    int set_del(uint64_t key_hash, uint32_t key_crc32);

    // 检查set是否存在
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @return 存在返回 SET_OK，不存在返回 SET_ERROR_KEY_NOT_FOUND，
    //         CRC不匹配返回 SET_ERROR_CRC_MISMATCH
    int set_exists(uint64_t key_hash, uint32_t key_crc32);

    // 获取set中member的数量
    // @param key_hash: key的hash值
    // @param key_crc32: key的crc32值
    // @param len: 输出参数，成员数量
    // @return 成功返回 SET_OK，失败返回错误码
    int set_len(uint64_t key_hash, uint32_t key_crc32, size_t* len);

private:
    HashDriver* hash_driver_;  // 内部使用 HashDriver 实现

    // 将 HashDriver 的错误码转换为 SetDriver 的错误码
    int convert_hash_error(int hash_error);
};

#endif // SET_DRIVER_H
