# SlabManager Performance Benchmark

这个目录包含了扩展版 Slab 分配器的性能测试代码。

## 功能特性

- **自动扩容**：当现有的 slab_allocator 都已满时，自动创建新的 allocator
- **两级页表管理**：高效管理最多 32,768 个 slab_allocator
- **简洁接口**：只需使用 `allocate()` 和 `deallocate()`，内部自动处理扩容

## 构建和运行

### 编译

```bash
cd bench/slab
mkdir -p build && cd build
cmake ..
make
```

### 运行

```bash
./bench_slab
```

## 测试内容

测试对比了以下三种内存分配方式的性能：

1. **SlabManager**：自动扩容的 Slab 分配器
2. **malloc/free**：标准库的 malloc/free
3. **new/delete**：C++ 的 new/delete 操作符

## 测试参数

- 总操作数：5 亿次（分配+释放）
- 批次大小：1000 个对象
- 块大小：32 字节
- 测试模式：批量分配后批量释放

## 性能指标

测试输出包括：
- 总耗时（微秒）
- 平均每次操作耗时（纳秒）
- 每秒操作数
- 创建的 allocator 数量
- 分配的 slab 数量
- 与标准库的性能对比

## 实现文件

- `bench_slab.cpp`：性能测试主程序
- `../../cache-Kernel/slab/slab_manager.h`：SlabManager 头文件
- `../../cache-Kernel/slab/slab_manager.cpp`：SlabManager 实现
- `../../cache-Kernel/slab/slab_allocator.h`：底层 SlabAllocator 头文件
- `../../cache-Kernel/slab/slab_allocator.cpp`：底层 SlabAllocator 实现
