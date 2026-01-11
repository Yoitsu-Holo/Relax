# TypeDriver 使用示例

本目录包含 TypeDriver 的使用示例。

## TypeDriver 简介

TypeDriver 是一个统一的类型驱动封装层，管理底层的 KVEngine 以及所有类型驱动（KV、Hash、Set）。它提供了简洁统一的接口来操作不同类型的数据结构。

### 架构设计

```
TypeDriver
├── KVEngine      (底层存储引擎)
├── KVDriver      (简单 KV 类型)
├── HashDriver    (Hash 类型，支持 field-value 映射)
└── SetDriver     (Set 类型，内部复用 HashDriver)
```

## 编译和运行

```bash
# 配置项目（启用 benchmark）
cmake -S . -B build -DBUILD_BENCHMARKS=ON

# 编译示例程序
cmake --build build --target example_type_driver

# 运行示例
./build/bench/typeDriver/example_type_driver
```

## 基本使用

### 1. 初始化 TypeDriver

```cpp
#include "type_driver.h"

TypeDriver driver;
int ret = driver.init();  // 使用默认参数：64 个 buddy
if (ret != TYPE_DRIVER_OK) {
    // 处理错误
}
```

### 2. KV 操作

```cpp
// 设置 KV
driver.kv_set(key_hash, key_crc32, data, data_len);

// 获取 KV
char* ret_data = nullptr;
size_t ret_len = 0;
driver.kv_get(key_hash, key_crc32, &ret_data, &ret_len);

// 删除 KV
driver.kv_del(key_hash, key_crc32);

// 检查存在性
driver.kv_exists(key_hash, key_crc32);
```

### 3. Hash 操作

```cpp
// 设置单个 field
hash_field field;
field.field_hash = ...;
field.field_crc32 = ...;
field.field = "name";
field.field_len = 4;
field.value = "Alice";
field.value_len = 5;
driver.hash_set_m(key_hash, key_crc32, &field);

// 获取单个 field
driver.hash_get_m(key_hash, key_crc32, &field);
// 注意：需要释放 field.value

// 获取所有 fields
hash_field* fields = nullptr;
size_t field_count = 0;
driver.hash_get(key_hash, key_crc32, &fields, &field_count);
// 注意：需要释放 fields 数组及其中的 field 和 value

// 删除整个 hash
driver.hash_del(key_hash, key_crc32);
```

### 4. Set 操作

```cpp
// 添加单个 member
set_member member;
member.member_hash = ...;
member.member_crc32 = ...;
member.member = "golang";
member.member_len = 6;
driver.set_add_m(key_hash, key_crc32, &member);

// 检查 member 是否存在
driver.set_exists_m(key_hash, key_crc32, member_hash, member_crc32);

// 获取所有 members
set_member* members = nullptr;
size_t member_count = 0;
driver.set_get(key_hash, key_crc32, &members, &member_count);
// 注意：需要释放 members 数组及其中的 member

// 删除整个 set
driver.set_del(key_hash, key_crc32);
```

## 注意事项

1. **内存管理**：
   - `hash_get` 和 `set_get` 等操作返回的内存由调用者负责释放
   - 需要使用 `delete[]` 释放返回的字符串和数组

2. **Hash 和 CRC32**：
   - 所有操作都需要提供 `hash` 和 `crc32` 值
   - `hash` 用于快速定位，`crc32` 用于冲突检测
   - 示例中使用简单的 hash 函数，生产环境建议使用标准库或专业的 hash 函数

3. **错误处理**：
   - 所有操作都返回错误码
   - 成功返回对应的 OK 值（KVD_OK、HASH_OK、SET_OK）
   - 失败返回具体的错误码

4. **线程安全**：
   - 当前实现不包含锁机制
   - 多线程使用需要在上层添加同步机制

## 性能特点

- **KV 操作**：O(1) 时间复杂度
- **Hash 单字段操作**：O(1) 时间复杂度（hash_set_m、hash_get_m、hash_del_m）
- **Hash 整体操作**：O(n) 时间复杂度（n 为 field 数量）
- **Set 单成员操作**：O(1) 时间复杂度（复用 Hash）
- **Set 整体操作**：O(n) 时间复杂度（n 为 member 数量）

## 文件说明

- `example_usage.cpp` - 完整的使用示例，演示所有基本操作
- `CMakeLists.txt` - 构建配置
- `README.md` - 本文件
