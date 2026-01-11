我现在想要实现一个类似redis的分布式缓存架构，在底层的数据存储，我打算只实现 kv 这一种基础类型，hash、set、list（双向）由基础kv扩展。
具体来说，对于kv，这是我的存储引擎基础能力，他支持uint64 -> byte* 的映射

# KV 存储

这是平台提供的基础存储能力，它已经在 @cache-Kernel/kvEngine 实现，对于外部来说提供以下几个接口：

- int add(uint64_t hash, const char *data, size_t len);
- int get(uint64_t hash, char **ret_data, size_t *ret_len);
- int del(uint64_t hash);

**基本假设**：
- add 操作是原子的 upsert（update or insert），如果 hash 已存在则覆盖，不存在则插入
- 所有操作都能保证一致性（单次调用是原子的）
- hash 冲突由上层通过 crc32 校验来检测和处理

# 对于kv存储的数据和元数据

对于简单kv存储，他有以下基础接口：

```cpp
// 错误码定义
#define KV_OK           0
#define KV_ERR_NOTFOUND -1  // key不存在
#define KV_ERR_CONFLICT -2  // hash冲突（crc32不匹配）
#define KV_ERR_NOMEM    -3  // 内存不足
#define KV_ERR_INVALID  -4  // 参数非法

int kv_set(
    uint64_t key_hash, uint32_t key_crc32, // in
    const char* data, size_t data_len      // in
);

int kv_get(
    uint64_t key_hash, uint32_t key_crc32, // in
    char** data, size_t *data_len          // out - 调用者负责free(*data)
);

int kv_del(
    uint64_t key_hash, uint32_t key_crc32  // in
);

bool kv_exists(
    uint64_t key_hash, uint32_t key_crc32  // in
);
```

**操作语义**：

- **kv_set**:
  1. 调用 kvEngine->get(key_hash) 检查是否存在
  2. 如果存在，校验 crc32 是否一致，不一致返回 KV_ERR_CONFLICT
  3. 构建新的 data 结构（含元数据头），调用 kvEngine->add() 写入
  4. 返回 KV_OK

- **kv_get**:
  1. 调用 kvEngine->get(key_hash) 获取数据
  2. 如果不存在，返回 KV_ERR_NOTFOUND
  3. 校验 crc32，不一致返回 KV_ERR_CONFLICT
  4. 分配内存拷贝 data 部分，返回给调用者

- **kv_del**:
  1. 调用 kvEngine->get(key_hash) 检查
  2. 校验 crc32，不一致返回 KV_ERR_CONFLICT
  3. 调用 kvEngine->del(key_hash) 删除
  4. 返回 KV_OK

- **kv_exists**:
  1. 调用 kvEngine->get(key_hash)
  2. 如果存在且 crc32 匹配，返回 true，否则返回 false

## 通用元数据结构

对于所有存储在kv引擎中的数据，都有统一的元数据头：

```cpp
struct kv_data_header {
    uint32_t key_crc32;      // 用于防止hash冲突的校验值
    uint32_t data_len;       // 实际存储的数据长度（不含header）
    uint32_t block_size:24;  // 分配的块大小（含header），最大16MiB-1
    uint32_t type:8;         // 数据类型
    uint32_t exp_time;       // 过期时间（Unix时间戳），0表示永不过期
    char data[];             // 实际数据
};
```

**type 类型定义**：
```cpp
#define TYPE_KV            0  // 简单KV
#define TYPE_HASH_META     1  // Hash元数据节点
#define TYPE_HASH_DATA     2  // Hash数据节点
#define TYPE_SET_META      3  // Set元数据节点
#define TYPE_SET_DATA      4  // Set数据节点
#define TYPE_LIST_META     5  // List元数据节点
#define TYPE_LIST_NODE     6  // List节点
```

**字段说明**：

- **key_crc32**: 用于检测 hash 冲突。对于不同类型的节点，存储内容不同：
  - TYPE_KV: 存储 key 的 crc32
  - TYPE_HASH_META: 存储 key 的 crc32
  - TYPE_HASH_DATA: 存储 field 的 crc32

- **data_len**: 实际使用的 data 部分长度，满足 `sizeof(kv_data_header) + data_len <= block_size`

- **block_size**: 向 kvEngine 申请的总大小（含 header），用于判断是否需要扩容

- **exp_time**: 过期时间，当前版本暂不实现过期逻辑，预留字段

## Block 大小分配策略

使用 2 的幂次方分配，保证与底层 slab+ralloc 对齐：

```
16, 32, 64, 128, 256, 512, 1024, 2048 Byte
4, 8, 16, 32, 64, 128, 256 KiB
```

分配时向上取整到最近的档位，例如需要 100B，则分配 128B。

# 对于 Hash 类型

Hash 类型采用 **元数据节点 + 多个数据节点** 的分离存储结构。

## 核心设计思想

1. **元数据节点**：通过 `key_hash` 访问，存储所有 field 的索引信息
2. **数据节点**：通过 `field_hash` 访问，存储单个 field-value 对
3. **O(1) 单字段操作**：通过 `field_hash` 直接访问数据节点，无需读取元数据
4. **延迟压缩**：元数据采用追加写入，删除时标记，空间不足时整理

## Hash 相关结构定义

```cpp
// Hash 元数据节点中的条目
struct hash_meta_entry {
    uint64_t field_hash;     // field的hash值（0表示空位，~0ULL表示已删除）
    uint32_t field_crc32;    // field的crc32校验值
    uint32_t reserved;       // 预留字段
};

#define HASH_ENTRY_EMPTY   0ULL          // 空位标记
#define HASH_ENTRY_DELETED (~0ULL)       // 删除标记

// Hash 元数据节点的 data 部分结构
struct hash_meta_data {
    uint32_t entry_count;    // 当前有效的field数量（不含已删除）
    uint32_t used_count;     // 已使用的entry槽位数量（含已删除）
    hash_meta_entry entries[]; // 动态数组
};

// Hash 数据节点的 data 部分结构
struct hash_field_data {
    uint32_t field_len;      // field字符串长度
    uint32_t value_len;      // value字符串长度
    char field_value[];      // field和value连续存储: [field][value]
};

// 用户接口的数据结构
struct hash_field {
    uint64_t field_hash;     // field的hash值
    uint32_t field_crc32;    // field的crc32校验值
    const char* field;       // field字符串
    size_t field_len;
    const char* value;       // value字符串
    size_t value_len;
};
```

## Hash 外部接口

```cpp
// ==================== 整体操作 ====================

// 批量设置多个field（会逐个调用hash_set_m）
int hash_set(
    uint64_t key_hash, uint32_t key_crc32,  // in
    hash_field* fields, size_t field_count  // in
);

// 获取hash的所有field-value对
int hash_get(
    uint64_t key_hash, uint32_t key_crc32,  // in
    hash_field** fields, size_t* field_count // out - 调用者负责释放
);

// 删除整个hash（包括所有field数据节点）
int hash_del(
    uint64_t key_hash, uint32_t key_crc32   // in
);

// 检查hash是否存在
bool hash_exists(
    uint64_t key_hash, uint32_t key_crc32   // in
);

// 获取hash中field的数量
int hash_len(
    uint64_t key_hash, uint32_t key_crc32,  // in
    size_t* len                              // out
);

// ==================== 单字段操作 ====================

// 设置单个field（O(1)操作）
int hash_set_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    const hash_field* field                  // in
);

// 获取单个field的value（O(1)操作）
int hash_get_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    hash_field* field                        // in: field_hash/field_crc32/field/field_len
                                             // out: value/value_len (调用者负责free)
);

// 删除单个field（O(1)操作）
int hash_del_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    uint64_t field_hash, uint32_t field_crc32 // in
);

// 检查field是否存在（O(1)操作）
bool hash_exists_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    uint64_t field_hash, uint32_t field_crc32 // in
);
```

## Hash 值计算规则

为了防止不同 (key, field) 组合产生相同的拼接结果，使用长度前缀：

```cpp
// key_hash: 直接对key字符串计算
key_hash = hash128(key, key_len); // 取低64位，这是上层调用时传递的，底层可以忽略，单元测试可以使用其他hash函数
key_crc32 = crc32(key, key_len);

// field_hash: 包含key上下文，防止不同hash的field冲突
uint8_t buffer[1024];
size_t offset = 0;

// 格式: [key][field][magic(4B)]
memcpy(buffer + offset, key, key_len);     offset += key_len;
memcpy(buffer + offset, field, field_len); offset += field_len;
*(uint64_t*)(buffer + offset) = 0x0D000721; offset += 4;

field_hash = hash64(buffer, offset);
field_crc32 = crc32(field, field_len); // 只对field本身计算crc32
```

## Hash 操作详细流程

### hash_set_m (单字段设置，O(1))

```
1. 构建 hash_field_data 结构
   - 计算所需大小: sizeof(kv_data_header) + sizeof(hash_field_data) + field_len + value_len
   - 向上取整到block_size档位

2. 写入数据节点
   - 调用 kvEngine->add(field_hash, data, block_size)
   - 如果失败，返回错误

3. 更新元数据节点（如果是新field）
   a. 调用 kvEngine->get(key_hash) 获取元数据节点
   b. 如果不存在，创建新的元数据节点：
      - 默认分配 256B (可容纳约12个entry)
      - 初始化 entry_count=1, used_count=1
      - entries[0] = {field_hash, field_crc32, 0}
      - 调用 kvEngine->add(key_hash, meta_data, 256)
   c. 如果存在：
      - 解析 hash_meta_data
      - 遍历 entries，查找 field_hash
        * 如果找到且crc32匹配：是更新操作，无需修改元数据，直接返回
        * 如果找到但crc32不匹配：返回 KV_ERR_CONFLICT
      - 如果未找到：是新增操作
        * 检查剩余容量：(block_size - sizeof(kv_data_header) - sizeof(hash_meta_data) - used_count * sizeof(hash_meta_entry)) >= sizeof(hash_meta_entry)
        * 如果容量足够：
          - entries[used_count] = {field_hash, field_crc32, 0}
          - entry_count++, used_count++
        * 如果容量不足：
          - 先尝试整理（compact）：移除所有 DELETED 标记的条目
          - 整理后重新检查容量
          - 如果还不够，扩容到 block_size * 2
        * 调用 kvEngine->add(key_hash, new_meta_data, new_block_size) 更新

4. 返回 KV_OK
```

### hash_get_m (单字段获取，O(1))

```
1. 调用 kvEngine->get(field_hash) 直接获取数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 header 中的 key_crc32（实际存储的是 field_crc32）
   - 如果不匹配，返回 KV_ERR_CONFLICT
4. 解析 hash_field_data，提取 field 和 value
5. 分配内存拷贝 value，返回给调用者
6. 返回 KV_OK
```

### hash_del_m (单字段删除，O(1))

```
1. 调用 kvEngine->get(field_hash) 检查数据节点
2. 校验 field_crc32，不匹配返回 KV_ERR_CONFLICT
3. 调用 kvEngine->del(field_hash) 删除数据节点
4. 更新元数据节点：
   a. 调用 kvEngine->get(key_hash) 获取元数据
   b. 遍历 entries，找到匹配的 field_hash
   c. 标记为删除：entries[i].field_hash = HASH_ENTRY_DELETED
   d. entry_count--
   e. 调用 kvEngine->add(key_hash, meta_data, block_size) 更新
5. 返回 KV_OK
```

### hash_exists_m (单字段存在性检查，O(1))

```
1. 调用 kvEngine->get(field_hash)
2. 如果存在且 field_crc32 匹配，返回 true
3. 否则返回 false
```

### hash_get (获取所有字段，O(n))

```
1. 调用 kvEngine->get(key_hash) 获取元数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 key_crc32，不匹配返回 KV_ERR_CONFLICT
4. 解析 hash_meta_data，遍历所有 entries：
   a. 跳过 EMPTY 和 DELETED 标记
   b. 对于有效的 field_hash，调用 kvEngine->get(field_hash) 获取数据节点
   c. 解析 hash_field_data，提取 field 和 value
5. 分配 hash_field 数组，填充所有数据
6. 返回 entry_count 个字段
```

### hash_del (删除整个hash，O(n))

```
1. 调用 kvEngine->get(key_hash) 获取元数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 key_crc32，不匹配返回 KV_ERR_CONFLICT
4. 解析 hash_meta_data，遍历所有 entries：
   a. 跳过 EMPTY 和 DELETED 标记
   b. 对于有效的 field_hash，调用 kvEngine->del(field_hash) 删除数据节点
5. 调用 kvEngine->del(key_hash) 删除元数据节点
6. 返回 KV_OK

注意：即使部分数据节点删除失败，仍然删除元数据节点，因为：
- 孤立的数据节点不会被访问（没有元数据引用）
- 可以通过后台GC任务清理
```

### hash_len (获取字段数量，O(1))

```
1. 调用 kvEngine->get(key_hash) 获取元数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 key_crc32，不匹配返回 KV_ERR_CONFLICT
4. 解析 hash_meta_data，返回 entry_count
```

## 元数据整理（Compact）

当元数据节点空间不足时，先尝试整理再扩容：

```cpp
void hash_meta_compact(hash_meta_data* old_meta, hash_meta_data* new_meta) {
    size_t write_pos = 0;

    // 遍历所有条目，只保留有效的
    for (size_t i = 0; i < old_meta->used_count; i++) {
        if (old_meta->entries[i].field_hash != HASH_ENTRY_EMPTY &&
            old_meta->entries[i].field_hash != HASH_ENTRY_DELETED) {
            new_meta->entries[write_pos++] = old_meta->entries[i];
        }
    }

    new_meta->entry_count = write_pos;  // 保持不变
    new_meta->used_count = write_pos;   // 压缩后，used = entry
}
```

整理后：
- 如果 `(block_size - used_space) >= sizeof(hash_meta_entry)`，则无需扩容
- 否则，扩容到 `block_size * 2`

## 性能分析

| 操作 | 时间复杂度 | kvEngine调用次数 | 说明 |
|------|-----------|----------------|------|
| hash_set_m | O(1) | 2-3次 | 1次写数据节点 + 1-2次读写元数据 |
| hash_get_m | O(1) | 1次 | 直接读数据节点 |
| hash_del_m | O(1) | 3次 | 1次读+1次删数据节点 + 1次读写元数据 |
| hash_exists_m | O(1) | 1次 | 直接检查数据节点 |
| hash_len | O(1) | 1次 | 只读元数据 |
| hash_get | O(n) | n+1次 | 1次读元数据 + n次读数据节点 |
| hash_del | O(n) | n+2次 | 1次读元数据 + n次删数据节点 + 1次删元数据 |

## 示例：存储 test->{"hello":"world"}

1. **计算 hash 值**：
   ```
   key = "test"
   key_hash = hash64("test") = 0x1234567890ABCDEF
   key_crc32 = crc32("test") = 0x12345678

   field = "hello"
   field_hash = hash64([4]["test"][5]["hello"][0xDEADBEEFCAFEBABE]) = 0xABCDEF1234567890
   field_crc32 = crc32("hello") = 0xABCDEF01
   ```

2. **写入数据节点**（通过 field_hash）：
   ```
   kvEngine->add(0xABCDEF1234567890, data, 128)

   data 内容:
   +------------------+
   | kv_data_header   |
   |   key_crc32 = 0xABCDEF01 (field的crc32) |
   |   data_len = 22                         |
   |   block_size = 128                      |
   |   type = TYPE_HASH_DATA                 |
   |   exp_time = 0                          |
   +------------------+
   | hash_field_data  |
   |   field_len = 5                         |
   |   value_len = 5                         |
   |   field_value[] = "helloworld"          |
   +------------------+
   ```

3. **创建/更新元数据节点**（通过 key_hash）：
   ```
   kvEngine->add(0x1234567890ABCDEF, meta, 256)

   meta 内容:
   +------------------+
   | kv_data_header   |
   |   key_crc32 = 0x12345678 (key的crc32)  |
   |   data_len = 24                         |
   |   block_size = 256                      |
   |   type = TYPE_HASH_META                 |
   |   exp_time = 0                          |
   +------------------+
   | hash_meta_data   |
   |   entry_count = 1                       |
   |   used_count = 1                        |
   +------------------+
   | hash_meta_entry  |
   |   field_hash = 0xABCDEF1234567890       |
   |   field_crc32 = 0xABCDEF01              |
   |   reserved = 0                          |
   +------------------+
   ```

## 注意事项

1. **内存管理**：
   - `hash_get` 和 `hash_get_m` 返回的数据由函数内部分配（malloc）
   - 调用者必须调用 `free()` 释放
   - `hash_field` 结构中的 `field` 和 `value` 指针指向调用者分配的内存

2. **并发控制**：
   - 当前设计未包含锁机制
   - 建议在上层实现 per-key 的读写锁
   - 或使用分片锁降低锁竞争

3. **过期处理**：
   - 当前版本 `exp_time` 字段预留但不实现
   - 未来实现时，hash 的过期应该是整体的（元数据节点过期，所有数据节点视为过期）

4. **错误处理**：
   - 所有操作都需要检查返回值
   - hash 冲突（KV_ERR_CONFLICT）需要上层处理（例如 rehash）

5. **碎片处理**：
   - 频繁的 `hash_del_m` 会导致元数据节点产生空洞
   - 整理操作（compact）会在空间不足时自动触发
   - 也可以定期调用 compact 减少碎片

6. **元数据初始大小建议**：
   - 如果预期 field 数量少（< 10），使用 256B
   - 如果预期 field 数量多（> 100），初始分配 1KiB 或更大
   - 避免频繁扩容的开销

---


# 对于 Set

Set 类型采用与 Hash 相同的 **元数据节点 + 多个数据节点** 的分离存储结构，本质上是一个不存储 value 的特化 Hash。

## 核心设计思想

1. **复用 Hash 结构**：Set 是 Hash 的简化版本，member 对应 field，不存储 value
2. **元数据节点**：通过 `key_hash` 访问，存储所有 member 的索引信息（复用 `hash_meta_data`）
3. **数据节点**：通过 `member_hash` 访问，存储单个 member（复用 `hash_field_data`，value_len=0）
4. **O(1) 单成员操作**：通过 `member_hash` 直接访问数据节点，无需读取元数据
5. **延迟压缩**：元数据采用追加写入，删除时标记，空间不足时整理

## Set 相关结构定义

```cpp
// Set 复用 Hash 的元数据结构
// hash_meta_entry 中的 field_hash 对应 member_hash
// hash_meta_entry 中的 field_crc32 对应 member_crc32

// Set 数据节点的 data 部分结构
struct set_member_data {
    uint32_t member_len;     // member字符串长度
    uint32_t reserved;       // 预留字段（对齐）
    char member[];           // member字符串
};

// 用户接口的数据结构
struct set_member {
    uint64_t member_hash;    // member的hash值
    uint32_t member_crc32;   // member的crc32校验值
    const char* member;      // member字符串
    size_t member_len;
};
```

## Set 外部接口

```cpp
// ==================== 整体操作 ====================

// 批量添加多个member（会逐个调用set_add_m）
int set_set(
    uint64_t key_hash, uint32_t key_crc32,  // in
    set_member* members, size_t member_count // in
);

// 获取set的所有member
int set_get(
    uint64_t key_hash, uint32_t key_crc32,  // in
    set_member** members, size_t* member_count // out - 调用者负责释放
);

// 删除整个set（包括所有member数据节点）
int set_del(
    uint64_t key_hash, uint32_t key_crc32   // in
);

// 检查set是否存在
bool set_exists(
    uint64_t key_hash, uint32_t key_crc32   // in
);

// 获取set中member的数量
int set_len(
    uint64_t key_hash, uint32_t key_crc32,  // in
    size_t* len                              // out
);

// ==================== 单成员操作 ====================

// 添加单个member（O(1)操作）
int set_add_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    const set_member* member                 // in
);

// 检查单个member是否存在（O(1)操作）
bool set_exists_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    uint64_t member_hash, uint32_t member_crc32 // in
);

// 删除单个member（O(1)操作）
int set_del_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    uint64_t member_hash, uint32_t member_crc32 // in
);

// 获取单个member的数据（主要用于验证，O(1)操作）
int set_get_m(
    uint64_t key_hash, uint32_t key_crc32,  // in
    set_member* member                       // in: member_hash/member_crc32
                                             // out: member/member_len (调用者负责free)
);
```

## Set 值计算规则

与 Hash 相同的计算方式，但用于 member：

```cpp
// key_hash: 直接对key字符串计算
key_hash = hash128(key, key_len); // 取低64位
key_crc32 = crc32(key, key_len);

// member_hash: 包含key上下文，防止不同set的member冲突
uint8_t buffer[1024];
size_t offset = 0;

// 格式: [key][member][magic(4B)]
memcpy(buffer + offset, key, key_len);       offset += key_len;
memcpy(buffer + offset, member, member_len); offset += member_len;
*(uint64_t*)(buffer + offset) = 0x0D000721;  offset += 4;

member_hash = hash64(buffer, offset);
member_crc32 = crc32(member, member_len); // 只对member本身计算crc32
```

## Set 操作详细流程

### set_add_m (单成员添加，O(1))

```
1. 构建 set_member_data 结构
   - 计算所需大小: sizeof(kv_data_header) + sizeof(set_member_data) + member_len
   - 向上取整到block_size档位

2. 写入数据节点
   - 调用 kvEngine->add(member_hash, data, block_size)
   - 如果失败，返回错误

3. 更新元数据节点（复用 hash_meta_data 结构）
   a. 调用 kvEngine->get(key_hash) 获取元数据节点
   b. 如果不存在，创建新的元数据节点：
      - 默认分配 256B (可容纳约12个entry)
      - 初始化 entry_count=1, used_count=1
      - entries[0] = {member_hash, member_crc32, 0}
      - 调用 kvEngine->add(key_hash, meta_data, 256)
   c. 如果存在：
      - 解析 hash_meta_data (作为set的元数据)
      - 遍历 entries，查找 member_hash
        * 如果找到且crc32匹配：member已存在，直接返回 KV_OK（幂等操作）
        * 如果找到但crc32不匹配：返回 KV_ERR_CONFLICT
      - 如果未找到：是新增操作
        * 检查剩余容量
        * 如果容量足够：添加新entry
        * 如果容量不足：整理或扩容（与hash相同逻辑）
        * 调用 kvEngine->add(key_hash, new_meta_data, new_block_size) 更新

4. 返回 KV_OK
```

### set_exists_m (单成员存在性检查，O(1))

```
1. 调用 kvEngine->get(member_hash)
2. 如果存在且 member_crc32 匹配，返回 true
3. 否则返回 false
```

### set_del_m (单成员删除，O(1))

```
1. 调用 kvEngine->get(member_hash) 检查数据节点
2. 校验 member_crc32，不匹配返回 KV_ERR_CONFLICT
3. 调用 kvEngine->del(member_hash) 删除数据节点
4. 更新元数据节点：
   a. 调用 kvEngine->get(key_hash) 获取元数据
   b. 遍历 entries，找到匹配的 member_hash
   c. 标记为删除：entries[i].field_hash = HASH_ENTRY_DELETED
   d. entry_count--
   e. 调用 kvEngine->add(key_hash, meta_data, block_size) 更新
5. 返回 KV_OK
```

### set_get_m (单成员获取，O(1))

```
1. 调用 kvEngine->get(member_hash) 直接获取数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 header 中的 key_crc32（实际存储的是 member_crc32）
   - 如果不匹配，返回 KV_ERR_CONFLICT
4. 解析 set_member_data，提取 member
5. 分配内存拷贝 member，返回给调用者
6. 返回 KV_OK
```

### set_get (获取所有成员，O(n))

```
1. 调用 kvEngine->get(key_hash) 获取元数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 key_crc32，不匹配返回 KV_ERR_CONFLICT
4. 解析 hash_meta_data（作为set元数据），遍历所有 entries：
   a. 跳过 EMPTY 和 DELETED 标记
   b. 对于有效的 member_hash，调用 kvEngine->get(member_hash) 获取数据节点
   c. 解析 set_member_data，提取 member
5. 分配 set_member 数组，填充所有数据
6. 返回 entry_count 个成员
```

### set_del (删除整个set，O(n))

```
1. 调用 kvEngine->get(key_hash) 获取元数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 key_crc32，不匹配返回 KV_ERR_CONFLICT
4. 解析 hash_meta_data（作为set元数据），遍历所有 entries：
   a. 跳过 EMPTY 和 DELETED 标记
   b. 对于有效的 member_hash，调用 kvEngine->del(member_hash) 删除数据节点
5. 调用 kvEngine->del(key_hash) 删除元数据节点
6. 返回 KV_OK

注意：即使部分数据节点删除失败，仍然删除元数据节点，原因同hash。
```

### set_len (获取成员数量，O(1))

```
1. 调用 kvEngine->get(key_hash) 获取元数据节点
2. 如果不存在，返回 KV_ERR_NOTFOUND
3. 校验 key_crc32，不匹配返回 KV_ERR_CONFLICT
4. 解析 hash_meta_data（作为set元数据），返回 entry_count
```

## 性能分析

| 操作 | 时间复杂度 | kvEngine调用次数 | 说明 |
|------|-----------|----------------|------|
| set_add_m | O(1) | 2-3次 | 1次写数据节点 + 1-2次读写元数据 |
| set_exists_m | O(1) | 1次 | 直接检查数据节点 |
| set_del_m | O(1) | 3次 | 1次读+1次删数据节点 + 1次读写元数据 |
| set_get_m | O(1) | 1次 | 直接读数据节点 |
| set_len | O(1) | 1次 | 只读元数据 |
| set_get | O(n) | n+1次 | 1次读元数据 + n次读数据节点 |
| set_del | O(n) | n+2次 | 1次读元数据 + n次删数据节点 + 1次删元数据 |

## 示例：存储 myset->{"apple", "banana", "cherry"}

1. **计算 hash 值**：
   ```
   key = "myset"
   key_hash = hash64("myset") = 0x1122334455667788
   key_crc32 = crc32("myset") = 0x11223344

   member1 = "apple"
   member1_hash = hash64([5]["myset"][5]["apple"][0x0D000721]) = 0xAABBCCDD11223344
   member1_crc32 = crc32("apple") = 0xAAAA1111

   member2 = "banana"
   member2_hash = hash64([5]["myset"][6]["banana"][0x0D000721]) = 0xBBCCDDEE22334455
   member2_crc32 = crc32("banana") = 0xBBBB2222

   member3 = "cherry"
   member3_hash = hash64([5]["myset"][6]["cherry"][0x0D000721]) = 0xCCDDEEFF33445566
   member3_crc32 = crc32("cherry") = 0xCCCC3333
   ```

2. **写入数据节点**（通过 member_hash，以 "apple" 为例）：
   ```
   kvEngine->add(0xAABBCCDD11223344, data, 64)

   data 内容:
   +------------------+
   | kv_data_header   |
   |   key_crc32 = 0xAAAA1111 (member的crc32) |
   |   data_len = 13                           |
   |   block_size = 64                         |
   |   type = TYPE_SET_DATA                    |
   |   exp_time = 0                            |
   +------------------+
   | set_member_data  |
   |   member_len = 5                          |
   |   reserved = 0                            |
   |   member[] = "apple"                      |
   +------------------+
   ```

3. **创建/更新元数据节点**（通过 key_hash）：
   ```
   kvEngine->add(0x1122334455667788, meta, 256)

   meta 内容:
   +------------------+
   | kv_data_header   |
   |   key_crc32 = 0x11223344 (key的crc32)    |
   |   data_len = 56                           |
   |   block_size = 256                        |
   |   type = TYPE_SET_META                    |
   |   exp_time = 0                            |
   +------------------+
   | hash_meta_data   | (复用hash结构)
   |   entry_count = 3                         |
   |   used_count = 3                          |
   +------------------+
   | hash_meta_entry  |
   |   field_hash = 0xAABBCCDD11223344 (apple) |
   |   field_crc32 = 0xAAAA1111                |
   |   reserved = 0                            |
   +------------------+
   | hash_meta_entry  |
   |   field_hash = 0xBBCCDDEE22334455 (banana)|
   |   field_crc32 = 0xBBBB2222                |
   |   reserved = 0                            |
   +------------------+
   | hash_meta_entry  |
   |   field_hash = 0xCCDDEEFF33445566 (cherry)|
   |   field_crc32 = 0xCCCC3333                |
   |   reserved = 0                            |
   +------------------+
   ```

## 实现建议

由于 Set 与 Hash 高度相似，可以通过以下方式实现：

### 方式一：直接调用 Hash 接口（推荐）

```cpp
int set_add_m(uint64_t key_hash, uint32_t key_crc32, const set_member* member) {
    hash_field field = {
        .field_hash = member->member_hash,
        .field_crc32 = member->member_crc32,
        .field = member->member,
        .field_len = member->member_len,
        .value = "",
        .value_len = 0
    };
    return hash_set_m(key_hash, key_crc32, &field);
}

bool set_exists_m(uint64_t key_hash, uint32_t key_crc32,
                  uint64_t member_hash, uint32_t member_crc32) {
    return hash_exists_m(key_hash, key_crc32, member_hash, member_crc32);
}

int set_del_m(uint64_t key_hash, uint32_t key_crc32,
              uint64_t member_hash, uint32_t member_crc32) {
    return hash_del_m(key_hash, key_crc32, member_hash, member_crc32);
}

// 其他接口类似...
```

### 方式二：独立实现（优化存储）

如果需要节省存储空间，可以独立实现 Set，使用更紧凑的数据结构：
- 元数据节点：复用 `hash_meta_data`，type = TYPE_SET_META
- 数据节点：使用 `set_member_data`（不存储 value），type = TYPE_SET_DATA

这种方式可以节省每个成员约 4 字节（不需要 value_len 字段）。

## 注意事项

1. **内存管理**：
   - `set_get` 和 `set_get_m` 返回的数据由函数内部分配（malloc）
   - 调用者必须调用 `free()` 释放

2. **幂等性**：
   - `set_add_m` 是幂等操作，重复添加相同 member 不会报错
   - 这与 Hash 的 upsert 语义不同（Hash 会覆盖 value）

3. **集合运算**：
   - 当前接口不包含集合运算（交集、并集、差集）
   - 如需要，可在上层实现，或添加辅助接口：
     ```cpp
     int set_union(key1, key2, result_key);
     int set_intersect(key1, key2, result_key);
     int set_diff(key1, key2, result_key);
     ```

4. **顺序性**：
   - Set 不保证成员的顺序
   - `set_get` 返回的成员顺序取决于元数据中的存储顺序

5. **并发控制**：
   - 与 Hash 相同，需要在上层实现锁机制

6. **碎片处理**：
   - 与 Hash 相同，频繁删除会产生碎片
   - 元数据整理逻辑完全复用 Hash 的 compact

7. **与 Hash 的关系**：
   - 如果使用方式一（直接调用 Hash），Set 和 Hash 共享相同的 type 定义
   - 需要确保 TYPE_SET_META/TYPE_SET_DATA 与 TYPE_HASH_META/TYPE_HASH_DATA 正确区分
   - 或者在元数据中添加额外标识位区分 Set 和 Hash

---




# 下面的内容还未完成设计，可以忽略

# 对于 List
他的结构如下：
