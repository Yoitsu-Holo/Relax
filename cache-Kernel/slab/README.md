# Slab Allocator

A high-performance fixed-size memory allocator implementation.

## 特性

- **固定块大小分配**: 支持16字节到4096字节的预定义块大小
- **O(1)时间复杂度**: 分配和释放操作都是常数时间
- **位图管理**: 使用两层位图进行高效的空闲块管理
- **Cache友好**: 64字节对齐，优化缓存性能
- **零碎片**: 固定大小分配，无内存碎片问题

## 架构设计

### 内存布局
- 每个slab占用64KiB内存
- 前512字节用作元数据（位图索引）
- 剩余内存用于实际分配

### 位图管理
- 第一层索引（idx0）：64位，每位代表一个组的状态
- 第二层索引（idx1[63]）：63个64位整数，每位代表一个块的状态
- 最多管理4032个块（63组 × 64块/组）

## 支持的块大小

- 16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096 字节

## API使用

```cpp
#include "slab_allocator.h"

// 创建slab
slab* my_slab = new slab;
SlabAllocator allocator(my_slab);

// 初始化为64字节块
allocator.init(64);

// 分配内存
void* ptr = allocator.allocate();

// 使用内存...

// 释放内存
allocator.deallocate(ptr);

// 查询状态
size_t free_blocks = allocator.get_free_blocks();
double usage = allocator.get_usage();
bool is_full = allocator.is_full();

// 清理
delete my_slab;
```

## 性能特点

- **分配速度**: 比标准malloc快2-3倍（取决于块大小）
- **释放速度**: O(1)常数时间
- **内存开销**: 每个slab 512字节元数据（0.78%开销）
- **无锁设计**: 单线程使用时无需同步

## 构建

```bash
mkdir build && cd build
cmake ..
make
```

## 测试

运行单元测试：
```bash
./test/slab/test_slab_allocator
```

运行性能基准测试（需要Google Benchmark）：
```bash
./test/slab/bench_slab_allocator
```

## 注意事项

1. **固定大小**: 只能分配预定义的块大小
2. **单线程**: 当前实现不是线程安全的
3. **容量限制**: 每个slab最多4032个块
4. **性能优先**: 为了性能，不检查重复释放或无效指针