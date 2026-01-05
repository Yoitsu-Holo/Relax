# Slab Allocator 性能测试

这是一个用于测试 C++ Slab 分配器性能的基准测试程序，对比自定义 Slab 分配器与标准内存分配器（malloc/free 和 new/delete）的性能差异。

## 测试内容

1. **Slab 分配器实现**：使用两级位图索引实现的高效 Slab 分配器
2. **对比测试**：
   - 自定义 Slab 分配器
   - 标准 malloc/free 分配器
   - 标准 new/delete 分配器
3. **测试场景**：批次分配/释放操作（每批次分配1000个对象后立即释放）
4. **测试规模**：100,000,000 次操作（100,000 批次 × 1000 对象/批次）
5. **性能指标**：
   - 总耗时（微秒）
   - 每次操作平均耗时（纳秒）
   - 每秒操作数（ops/sec）
   - 相对性能对比

## Slab 分配器特点

### 内存布局
- **总大小**：64 KiB per slab
- **元数据**：前 512 字节（64 个 uint64_t）
  - 1 个一级索引位图（idx0）
  - 63 个二级索引位图（idx1）
- **可用空间**：64 KiB - 512 字节 = 65,024 字节
- **支持的块大小**：16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096 字节
- **动态块数**：根据块大小动态计算（例如：16字节=4032块，32字节=2032块）

### 实现特性
- **两级位图索引**：快速查找空闲块
  - 一级索引（idx0）：标记哪些组有空闲块
  - 二级索引（idx1）：每组64个块的具体分配状态
- **O(1) 分配**：使用 `__builtin_ctzll` 快速定位空闲块
- **O(1) 释放**：直接通过位操作释放
- **动态初始化**：运行时选择块大小，自动标记不可用空间
- **优化的边界检查**：通过预初始化避免运行时检查
- **缓存友好**：连续内存布局，提高缓存命中率
- **无内存碎片**：固定大小块分配

## 快速开始

### 使用 g++ 直接编译（推荐）

```bash
cd slab
g++ -std=c++17 -O3 -DNDEBUG bench_slab_allocator.cpp -o bench_slab_allocator
```

### 使用 clang 编译

```bash
cd slab
clang++ -std=c++17 -O3 -DNDEBUG bench_slab_allocator.cpp -o bench_slab_allocator
```

### 使用 MSVC 编译（Windows）

```bash
cd slab
cl /std:c++17 /O2 /EHsc bench_slab_allocator.cpp
```

## 运行测试

编译完成后，运行生成的可执行文件：

```bash
./bench_slab_allocator
```

## 代码示例

### 初始化不同块大小

```cpp
slab* my_slab = new slab;
SlabAllocator allocator(my_slab);

// 初始化为16字节块
if (allocator.init(16) == 0) {
    std::cout << "Total blocks: " << allocator.get_total_blocks() << std::endl;  // 4032块
}

// 或初始化为32字节块
if (allocator.init(32) == 0) {
    std::cout << "Total blocks: " << allocator.get_total_blocks() << std::endl;  // 2032块
}
```

### 分配和释放内存

```cpp
// 分配内存
void* ptr = allocator.allocate();
if (ptr != nullptr) {
    // 使用内存...

    // 释放内存
    allocator.deallocate(ptr);
}
```

## 输出说明

程序将按以下顺序执行测试并输出结果：

### 1. Slab 分配器测试
- 显示初始化的块大小
- 显示总可用块数
- 执行 1 亿次分配和 1 亿次释放操作

### 2. 标准 malloc/free 测试
- 执行相同数量的 malloc/free 操作（相同块大小）

### 3. 标准 new/delete 测试
- 执行相同数量的 new/delete 操作（相同块大小）

### 4. 性能汇总
- 各分配器的总耗时（微秒）
- 平均每次操作耗时（纳秒）
- 每秒操作数

### 5. 性能对比
- Slab vs malloc/free 的加速比
- Slab vs new/delete 的加速比

## 测试参数

```cpp
constexpr int BATCH_SIZE = 1000;        // 每批次1000个对象
constexpr int TOTAL_OPS = 100000000;    // 总共1亿次操作
constexpr int NUM_BATCHES = 100000;     // 10万个批次
```

## 编译选项说明

- `-std=c++17`：需要 C++17 支持
- `-O3`：启用最高级别优化
- `-DNDEBUG`：禁用断言以提高性能

## 系统要求

- C++17 或更高版本的编译器
- 支持 `__builtin_ctzll` 内建函数（GCC/Clang）
- 最少 64 KiB 可用内存
- 64 位操作系统（推荐）

## 测试环境建议

为了获得准确的性能测试结果，建议：
1. 关闭其他占用 CPU 的程序
2. 使用 Release 编译模式（启用 -O3 优化）
3. 禁用 CPU 频率调节（设置为性能模式）
4. 运行多次测试取平均值
5. 确保系统有足够的可用内存

## 性能测试结果示例

### 16字节块测试结果
- **Slab 分配器**：~3.4 ns/操作，2.9×10⁸ ops/sec
- **malloc/free**：~7.2 ns/操作，1.4×10⁸ ops/sec
- **new/delete**：~8.7 ns/操作，1.1×10⁸ ops/sec
- **性能提升**：比 malloc/free 快 2.1x，比 new/delete 快 2.5x

### 32字节块测试结果
- **Slab 分配器**：~4.5 ns/操作，2.2×10⁸ ops/sec
- **malloc/free**：~8.2 ns/操作，1.2×10⁸ ops/sec
- **new/delete**：~9.5 ns/操作，1.1×10⁸ ops/sec
- **性能提升**：比 malloc/free 快 1.8x，比 new/delete 快 2.1x

## 实现细节

### 核心数据结构

```cpp
union slab {
    struct {
        uint64_t idx0;        // 一级索引位图
        uint64_t idx1[63];    // 二级索引位图数组
    } metadata;
    uint8_t data[64*KiB];     // 原始内存数据
};
```

### SlabAllocator 类

```cpp
class SlabAllocator {
private:
    size_t block_size_;        // 块大小（运行时设置）
    size_t metadata_size_;     // 元数据大小（512字节）
    size_t usable_size_;       // 可用空间
    size_t total_blocks_;      // 总块数
    static constexpr size_t BLOCKS_PER_UINT64 = 64;
    slab* slab_ptr;

public:
    int init(int block_size);   // 初始化指定块大小
    void* allocate();            // 分配一个块
    void deallocate(void* ptr);  // 释放一个块
    size_t get_free_blocks();    // 获取空闲块数
    size_t get_total_blocks();   // 获取总块数
};
```

### 关键算法

1. **初始化算法（init）**：
   - 验证块大小是否支持
   - 计算总块数和可用空间
   - 初始化位图，标记超出范围的块为已分配
   - 更新索引状态

2. **分配算法（allocate）**：
   - 检查一级索引找到有空闲块的组
   - 使用 `__builtin_ctzll` 快速定位第一个空闲位
   - 标记该块为已分配
   - 返回块地址

3. **释放算法（deallocate）**：
   - 计算块在 slab 中的位置
   - 清除对应的分配位
   - 更新索引状态

## 性能优势分析

### Slab 分配器的优势
1. **减少系统调用**：预分配大块内存，避免频繁的系统调用
2. **位图索引**：O(1) 时间复杂度的分配和释放
3. **优化的边界检查**：通过预初始化消除运行时边界检查
4. **缓存局部性**：连续内存分配，提高 CPU 缓存命中率
5. **无碎片**：固定大小分配，完全避免内存碎片
6. **低开销**：简单的位操作，无需复杂的内存管理逻辑

### 适用场景
- 固定大小对象的频繁分配/释放
- 对延迟敏感的应用
- 内存池实现
- 游戏引擎中的对象池
- 网络服务器的连接管理
- 实时系统的内存管理

## 注意事项

1. 本实现支持多种固定大小的分配（16-4096字节）
2. 单个 slab 的容量取决于块大小（如16字节=4032块，32字节=2032块）
3. 不支持跨 slab 的内存管理（需要上层管理多个 slab）
4. 线程安全需要额外的同步机制
5. 通过 `init` 函数预初始化避免了运行时的边界检查

## 扩展建议

如需在生产环境使用，可以考虑以下扩展：
1. 实现 slab 链表管理多个 slab
2. 添加线程安全支持（互斥锁或无锁设计）
3. 实现内存使用统计和监控
4. 支持内存对齐选项
5. 添加调试模式的内存泄漏检测
6. 实现 slab 缓存层（多种块大小的 slab 池）