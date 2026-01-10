# Buddy System Memory Manager

高性能的伙伴系统内存分配器，使用空闲链表和位图进行快速内存管理。

## 特性

- **零循环优化**: 使用宏展开和 switch 语句避免动态循环
- **位运算加速**: 使用 `__builtin_ctzll` 和 `__builtin_popcountll` 进行快速位操作
- **伙伴合并**: 自动合并相邻的空闲块以减少碎片
- **多级管理**: 7 个阶层，从 4KiB 到 256KiB
- **无命名空间**: 简洁的API设计，避免命名空间嵌套

## 配置参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 总内存大小 | 16 MiB | 单个伙伴系统管理的内存 |
| 基本块大小 | 4 KiB | 最小分配单元 |
| 最小分配 | 4 KiB (Order 0) | 最小可分配块 |
| 最大分配 | 256 KiB (Order 6) | 最大可分配块 |
| 总块数 | 4096 | 基本块总数 |

## 内存层级

```
Order 0:   4 KiB  (4096 blocks)
Order 1:   8 KiB  (2048 blocks)
Order 2:  16 KiB  (1024 blocks)
Order 3:  32 KiB  ( 512 blocks)
Order 4:  64 KiB  ( 256 blocks)
Order 5: 128 KiB  ( 128 blocks)
Order 6: 256 KiB  (  64 blocks)
```

## 使用示例

```cpp
#include "buddy_system.h"
#include <cstdlib>

int main() {
    // 1. 分配 16MiB 对齐内存
    void* memory = std::aligned_alloc(BUDDY_BLOCK_SIZE, BUDDY_TOTAL_SIZE);

    // 2. 创建并初始化伙伴系统
    BuddySystem buddy;
    buddy.init(memory);

    // 3. 分配内存
    void* ptr1 = buddy.allocate(4096);      // 分配 4KiB
    void* ptr2 = buddy.allocate(65536);     // 分配 64KiB
    void* ptr3 = buddy.allocate(131072);    // 分配 128KiB

    // 4. 使用分配的内存
    // ... your code here ...

    // 5. 释放内存
    buddy.deallocate(ptr1, 4096);
    buddy.deallocate(ptr2, 65536);
    buddy.deallocate(ptr3, 131072);

    // 6. 查看统计信息
    for (int order = 0; order <= BUDDY_MAX_ORDER; ++order) {
        size_t free_blocks = buddy.get_free_blocks(order);
        std::cout << "Order " << order << ": "
                  << free_blocks << " free blocks\n";
    }

    // 7. 释放原始内存
    std::free(memory);

    return 0;
}
```

## API 接口

### 初始化

```cpp
void init(void* base_addr);
```

初始化伙伴系统，传入要管理的 16MiB 内存区域的起始地址。

### 内存分配

```cpp
void* allocate(size_t size);
```

分配指定大小的内存块。
- **参数**: `size` - 请求的字节数（4KiB ~ 256KiB）
- **返回**: 分配的内存地址，失败返回 `nullptr`

### 内存释放

```cpp
void deallocate(void* ptr, size_t size);
```

释放之前分配的内存块。
- **参数**:
  - `ptr` - 要释放的内存地址
  - `size` - 分配时的大小
- **注意**: 必须提供正确的 size，否则可能导致内存损坏

### 统计信息

```cpp
size_t get_free_blocks(int order) const;
size_t get_total_free_memory() const;
```

获取指定阶的空闲块数量或总空闲内存。

## 测试和基准测试

### 单元测试

单元测试位于 `../../test/buddySystem/`:

```bash
cd ../../test/buddySystem
mkdir build && cd build
cmake ..
make
./test_buddy_system
```

测试包括：
- 基本功能测试
- 边界条件测试
- 伙伴合并测试
- 碎片化处理测试
- 内存耗尽测试
- 大规模分配测试

### 性能基准测试

性能测试位于 `../../bench/buddySystem/`:

```bash
cd ../../bench/buddySystem
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# 运行基准测试（对比 malloc/free）
./bench_buddy

# 运行性能分析（用于火焰图）
./profile_buddy
```

详细信息请参考：
- `../../bench/buddySystem/README.md` - 基准测试说明
- `../../bench/buddySystem/PROFILING_GUIDE.md` - 性能分析指南
- `../../bench/buddySystem/QUICKSTART.md` - 快速入门

## 性能优化技术

### 1. 二级位图加速查找

**核心优化**：参考 slab_allocator 的设计，使用二级位图结构大幅提升空闲块查找速度。

**数据结构**：
```cpp
struct BuddyTwoLevelBitmap {
    uint64_t level0;     // 第一级：64位，每个bit表示一个组（64个块）
    uint64_t level1[64]; // 第二级：64个uint64，每个管理64个块
};
```

**工作原理**：
- **level0**（第一级索引）：每个bit对应level1中的一个uint64
  - bit为0：该组有空闲块
  - bit为1：该组已满
- **level1**（第二级索引）：实际的位图
  - bit为0：该块空闲
  - bit为1：该块已分配

**查找算法**（O(1)时间复杂度）：
```cpp
// 1. 在level0中找第一个有空闲的组（~level0找第一个1）
uint64_t available_groups = ~level0;
size_t group_idx = __builtin_ctzll(available_groups);

// 2. 在对应的level1中找空闲块
uint64_t free_blocks = ~level1[group_idx];
size_t block_idx = __builtin_ctzll(free_blocks);

// 3. 计算最终索引：group_idx * 64 + block_idx
return (group_idx << 6) | block_idx;
```

**性能提升**（4KiB块，1000万次操作）：
- 优化前：600.7 μs，与malloc相比慢 44.75%
- 优化后：462.3 μs，与malloc相比慢 22.30%
- **绝对提升**：~23% 性能提升
- **相对提升**：与malloc的差距缩小了约一半

### 2. 位运算加速

- **`__builtin_ctzll`**: 计算尾随零位数，用于快速查找第一个空闲块
- **`__builtin_popcountll`**: 计算置位数，用于快速统计空闲块数量
- **位移运算**: 使用 `>>` 和 `<<` 替代除法和乘法，避免昂贵的整数运算

### 3. 空闲链表

使用侵入式链表减少额外内存开销：
```cpp
struct BuddyFreeNode {
    BuddyFreeNode* next;
};
```

空闲块本身存储链表指针，无需额外分配。

## 注意事项

1. **大小限制**: 只支持 4KiB ~ 256KiB 的分配
2. **对齐要求**: 基础内存必须按 4KiB 对齐
3. **释放时提供大小**: `deallocate` 必须提供正确的分配大小
4. **非线程安全**: 多线程环境需要外部同步
5. **内存归属**: 伙伴系统不负责释放传入的基础内存

## 性能特点

- **分配时间复杂度**: O(1) 到 O(log N)，取决于是否需要分裂
- **释放时间复杂度**: O(log N)，包括伙伴合并
- **空间开销**:
  - 位图: 7 * 64 * 8 = 3.5 KB
  - 链表数组: 4096 * 8 = 32 KB
  - 总开销: ~36 KB（< 0.22% 的管理开销）

## 项目结构

```
cache-Kernel/buddySystem/    - 核心实现
  ├── buddy_system.h         - 头文件
  ├── buddy_system.cpp       - 实现文件
  ├── design.md              - 设计文档
  └── README.md              - 本文档

test/buddySystem/            - 单元测试
  ├── test_buddy_system.cpp  - 测试代码
  └── CMakeLists.txt         - 测试构建配置

bench/buddySystem/           - 性能测试
  ├── bench_buddy.cpp        - 基准测试（vs malloc/free）
  ├── profile_buddy.cpp      - 性能分析版本
  ├── CMakeLists.txt         - 构建配置
  ├── README.md              - 基准测试文档
  ├── PROFILING_GUIDE.md     - 性能分析指南
  └── QUICKSTART.md          - 快速入门
```

## 许可

本实现是 Relax 缓存内核项目的一部分。

