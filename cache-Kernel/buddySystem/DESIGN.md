# Buddy System 分层架构设计文档

## 1. 架构概述

### 1.1 设计目标
- 减少频繁的链表操作开销
- 支持管理多个16MiB内存块（可扩展性）
- 优化分配/释放的性能（O(1)查找allocator）
- 参考slab allocator的成熟架构

### 1.2 分层架构

```
┌─────────────────────────────────────────┐
│         BuddyManager (上层)              │
│  - 管理多个BuddyAllocator               │
│  - 7个链表数组（按块大小组织）          │
│  - 快速定位可用allocator                │
└──────────────┬──────────────────────────┘
               │ 管理
               ▼
┌─────────────────────────────────────────┐
│      BuddyAllocator (底层)               │
│  - 管理单个16MiB内存块                  │
│  - 二级位图 + 全局4KiB位图              │
│  - 快速分配/释放                        │
└─────────────────────────────────────────┘
```

## 2. BuddyAllocator（底层分配器）

### 2.1 职责
- 管理单个16MiB内存块
- 执行实际的内存分配和释放
- 维护位图和伙伴系统逻辑

### 2.2 数据结构

```cpp
class BuddyAllocator {
private:
    void* base_addr_;                          // 16MiB对齐的内存起始地址

    // 二级位图（7个order，每个order一个）
    BuddyTwoLevelBitmap free_bitmap_[7];

    // 全局4KiB粒度位图（64个uint64，共4096位）
    BuddyGlobalBitmap global_bitmap_;

    // 阶数记录：每个4KiB块记录其所属的分配阶数
    // 0xFF表示未分配，0-6表示阶数
    uint8_t block_order_[4096];

    // 统计信息
    size_t allocated_blocks_[7];               // 每个阶已分配的块数
};
```

### 2.3 核心接口

```cpp
// 初始化：传入16MiB对齐的内存
void init(void* base_addr);

// 分配：返回指针，失败返回nullptr
void* allocate(int order);

// 释放：传入指针，自动查询阶数
void deallocate(void* ptr);

// 查询能力：返回uint8位图，每位表示该order是否还能分配
// bit 0: 4KiB, bit 1: 8KiB, ..., bit 6: 256KiB
uint8_t get_capability() const;

// 检查指针是否属于此allocator
bool owns_pointer(void* ptr) const;
```

### 2.4 位图策略

**二级位图**：
- level0（64位）：每位表示一个level1组是否有空闲
- level1（64个uint64）：实际的块分配状态

**全局4KiB位图**：
- 64个uint64，共4096位
- 每位对应一个4KiB基本块
- 用于伙伴合并的快速检查

**分配标记**：
```
order 0 (4KiB):   掩码 = 0b1       (1位)
order 1 (8KiB):   掩码 = 0b11      (2位)
order 2 (16KiB):  掩码 = 0b1111    (4位)
order 3 (32KiB):  掩码 = 0xFF      (8位)
order 4 (64KiB):  掩码 = 0xFFFF    (16位)
order 5 (128KiB): 掩码 = 0xFFFFFFFF (32位)
order 6 (256KiB): 掩码 = ~0ULL     (64位)
```

### 2.5 阶数记录

每个4KiB块维护其分配时的阶数：
- **未分配**: `0xFF`
- **已分配**: `0-6`（对应order）

释放时：
1. 计算基础块索引：`(ptr - base_addr) / 4096`
2. 查询阶数：`order = block_order_[base_index]`
3. 清除连续块的标记：`block_order_[base_index + i] = 0xFF` (i = 0..2^order-1)

## 3. BuddyManager（上层管理器）

### 3.1 职责
- 管理多个BuddyAllocator实例
- 维护按大小组织的allocator链表
- 快速定位可用allocator
- 处理allocator的动态扩展

### 3.2 数据结构

```cpp
class BuddyManager {
private:
    // Allocator池
    BuddyAllocator* allocators_;               // allocator数组
    size_t max_allocators_;                    // 最大allocator数量
    size_t num_allocators_;                    // 当前allocator数量

    // 链表数组：7个order，每个维护可用allocator的链表
    // free_lists_[order][idx] 存储allocator索引
    // 0号元素是链表头
    struct AllocatorListNode {
        int prev;                               // 前驱索引（-1表示无）
        int next;                               // 后继索引（-1表示无）
    };
    AllocatorListNode free_lists_[7][4096];    // 每个order一个链表数组

    // 能力位图：标记哪些allocator有空闲
    // capability_bitmap_[alloc_idx] 的每位表示该allocator能否分配对应order
    uint8_t capability_bitmap_[4096];

    // 快速查找：指针到allocator索引的映射
    // 使用高位地址作为key
    // ptr >> 24 (16MiB对齐) -> allocator_index
};
```

### 3.3 链表数组设计

**数组模拟双向链表**：
```
free_lists_[order]:
  [0]: 链表头节点（prev=-1, next=第一个allocator索引）
  [1..4095]: allocator节点（prev/next指向其他allocator索引）

节点状态：
  - 在链表中: prev >= 0 || next >= 0
  - 已移除: prev = -1 && next = allocator_index（自指）
```

**示例**：
```
order=1的链表（8KiB）：
  [0] -> [5] -> [12] -> [3] -> NULL

表示：allocator 5, 12, 3 还能分配8KiB块
```

### 3.4 核心接口

```cpp
// 初始化：指定最大allocator数量
bool init(size_t max_allocators = 64);

// 分配：自动选择或创建allocator
void* allocate(size_t size);

// 释放：自动定位allocator
void deallocate(void* ptr);

// 统计
size_t get_total_free_memory() const;
size_t get_allocator_count() const;
```

## 4. 分配流程

### 4.1 Manager::allocate(size)

```
1. 计算order: order = size_to_order(size)

2. 从链表数组获取可用allocator:
   head_idx = free_lists_[order][0].next
   if (head_idx == -1):
       goto 创建新allocator

3. 尝试分配:
   alloc_idx = head_idx
   ptr = allocators_[alloc_idx].allocate(order)
   if (ptr == nullptr):
       goto 错误处理

4. 更新allocator能力:
   new_cap = allocators_[alloc_idx].get_capability()
   update_allocator_lists(alloc_idx, new_cap)

5. 返回ptr

创建新allocator:
   if (num_allocators_ >= max_allocators_):
       return nullptr  // 无法扩展

   alloc_idx = num_allocators_++
   void* mem = aligned_alloc(16*1024*1024, 16*1024*1024)
   allocators_[alloc_idx].init(mem)

   ptr = allocators_[alloc_idx].allocate(order)
   new_cap = allocators_[alloc_idx].get_capability()
   update_allocator_lists(alloc_idx, new_cap)

   return ptr
```

### 4.2 Allocator::allocate(order)

```
1. 从二级位图查找空闲块:
   available_groups = ~free_bitmap_[order].level0
   if (available_groups == 0):
       尝试从更高阶分裂

   group_idx = __builtin_ctzll(available_groups)
   free_blocks = ~free_bitmap_[order].level1[group_idx]
   block_idx = __builtin_ctzll(free_blocks)
   index = (group_idx << 6) | block_idx

2. 标记为已分配:
   set_allocated(index, order)

3. 更新全局4KiB位图:
   start_4k = index << order
   set_global_bits(start_4k, order)

4. 记录阶数:
   for (i = 0; i < (1 << order); i++):
       block_order_[start_4k + i] = order

5. 返回地址:
   return base_addr_ + (index << order) * 4096
```

## 5. 释放流程

### 5.1 Manager::deallocate(ptr)

```
1. 定位allocator:
   alloc_idx = find_allocator_by_pointer(ptr)
   if (alloc_idx == -1):
       return  // 错误：无效指针

2. 执行释放:
   allocators_[alloc_idx].deallocate(ptr)

3. 更新allocator能力:
   new_cap = allocators_[alloc_idx].get_capability()
   update_allocator_lists(alloc_idx, new_cap)
```

### 5.2 Allocator::deallocate(ptr)

```
1. 查询阶数:
   base_4k_index = (ptr - base_addr_) / 4096
   order = block_order_[base_4k_index]
   if (order == 0xFF):
       return  // 错误：未分配的块

2. 计算块索引:
   index = base_4k_index >> order

3. 清除全局4KiB位图:
   start_4k = index << order
   clear_global_bits(start_4k, order)

4. 清除阶数记录:
   for (i = 0; i < (1 << order); i++):
       block_order_[start_4k + i] = 0xFF

5. 标记为空闲:
   set_free(index, order)

6. 尝试伙伴合并:
   while (order < 6):
       buddy_idx = index ^ 1
       if (!can_merge(buddy_idx, order)):
           break
       merge_buddy(index, order)
       index >>= 1
       order++
```

### 5.3 update_allocator_lists(alloc_idx, new_cap)

```
对于每个order (0..6):
    old_in_list = (capability_bitmap_[alloc_idx] & (1 << order)) != 0
    new_in_list = (new_cap & (1 << order)) != 0

    if (old_in_list && !new_in_list):
        // 从链表移除
        remove_from_list(order, alloc_idx)

    elif (!old_in_list && new_in_list):
        // 加入链表头
        add_to_list_head(order, alloc_idx)

    elif (old_in_list && new_in_list):
        // 移到链表头（提高局部性）
        remove_from_list(order, alloc_idx)
        add_to_list_head(order, alloc_idx)

// 更新能力位图
capability_bitmap_[alloc_idx] = new_cap
```

## 6. 链表操作

### 6.1 add_to_list_head(order, alloc_idx)

```cpp
// 插入到链表头（0号节点后面）
old_first = free_lists_[order][0].next

free_lists_[order][alloc_idx].prev = 0
free_lists_[order][alloc_idx].next = old_first
free_lists_[order][0].next = alloc_idx

if (old_first != -1):
    free_lists_[order][old_first].prev = alloc_idx
```

### 6.2 remove_from_list(order, alloc_idx)

```cpp
prev_idx = free_lists_[order][alloc_idx].prev
next_idx = free_lists_[order][alloc_idx].next

if (prev_idx != -1):
    free_lists_[order][prev_idx].next = next_idx

if (next_idx != -1):
    free_lists_[order][next_idx].prev = prev_idx

// 标记为已移除（自指）
free_lists_[order][alloc_idx].prev = -1
free_lists_[order][alloc_idx].next = alloc_idx
```

## 7. 指针到Allocator的映射

### 7.1 方案：高位地址查找

由于allocator管理的内存都是16MiB对齐：
```cpp
size_t find_allocator_by_pointer(void* ptr) {
    // 16MiB = 2^24，取高位作为特征
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base_24 = addr >> 24;  // 去除低24位

    // 线性查找（allocator数量通常很小，<64个）
    for (size_t i = 0; i < num_allocators_; i++) {
        if (allocators_[i].owns_pointer(ptr)) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

bool BuddyAllocator::owns_pointer(void* ptr) const {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = reinterpret_cast<uintptr_t>(base_addr_);
    return (addr >= base) && (addr < base + 16*1024*1024);
}
```

### 7.2 优化：哈希表（可选）

如果allocator数量很多，可使用哈希表：
```cpp
std::unordered_map<uintptr_t, size_t> addr_to_allocator_;
// key: base_addr >> 24, value: allocator_index
```

## 8. 内存布局

### 8.1 单个Allocator元数据

```
BuddyAllocator元数据大小估算：
- base_addr_: 8字节
- free_bitmap_[7]: 7 * (8 + 64*8) = 7 * 520 = 3640字节
- global_bitmap_: 64 * 8 = 512字节
- block_order_[4096]: 4096字节
- allocated_blocks_[7]: 7 * 8 = 56字节

总计：约 8.3 KiB 元数据
```

### 8.2 Manager元数据

```
BuddyManager元数据（假设max_allocators=4096）：
- allocators_: 4096 * 指针 = 32 KiB
- free_lists_[7][4096]: 7 * 4096 * 8 = 224 KiB
- capability_bitmap_[4096]: 4 KiB

总计：约 260 KiB 元数据
```

## 9. 性能分析

### 9.1 时间复杂度

| 操作 | 复杂度 | 说明 |
|------|--------|------|
| allocate | O(1) | 从链表头获取allocator，二级位图查找 |
| deallocate | O(log N) | 线性查找allocator + 伙伴合并 |
| 链表更新 | O(1) | 数组索引，常数次操作 |

### 9.2 优化效果

**相比原实现**：
- ✅ 减少链表操作频率：从每次分配→仅在能力变化时
- ✅ 避免链表遍历：数组索引直接访问
- ✅ 提高缓存局部性：优先使用链表头allocator
- ✅ 可扩展性：支持多个16MiB块

**预期性能提升**：
- 分配：10-20% 提升（减少链表操作）
- 释放：5-10% 提升（简化链表维护）

## 10. 使用示例

```cpp
// 初始化manager
BuddyManager manager;
manager.init(64);  // 最多管理64个allocator（1GiB）

// 分配
void* ptr1 = manager.allocate(4096);   // 4KiB
void* ptr2 = manager.allocate(65536);  // 64KiB

// 使用...

// 释放
manager.deallocate(ptr1);
manager.deallocate(ptr2);

// 统计
size_t free_mem = manager.get_total_free_memory();
size_t num_allocs = manager.get_allocator_count();
```

## 11. 实现计划

### 阶段1：BuddyAllocator
1. 实现二级位图和全局4KiB位图
2. 实现阶数记录机制
3. 实现基本的allocate/deallocate
4. 实现get_capability()接口

### 阶段2：BuddyManager
1. 实现allocator池管理
2. 实现链表数组
3. 实现allocator查找机制
4. 集成allocate/deallocate

### 阶段3：测试和优化
1. 单元测试
2. 性能基准测试
3. 内存泄漏检测
4. 优化热点路径

## 12. 注意事项

1. **线程安全**：当前设计不是线程安全的，多线程需要外部同步
2. **内存对齐**：allocator的内存必须16MiB对齐
3. **错误处理**：所有接口都应有完善的错误检查
4. **资源释放**：Manager析构时需要释放所有allocator的内存
5. **扩展性**：考虑动态扩展allocator池的机制
