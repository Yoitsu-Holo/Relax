# SlabManager Benchmark 快速开始

## 方式一：独立构建（推荐用于快速测试）

```bash
cd bench/slab
mkdir -p build && cd build
cmake ..
make
./bench_slab
```

## 方式二：从顶层目录构建

```bash
# 从项目根目录
mkdir -p build && cd build
cmake -DBUILD_BENCHMARKS=ON ..
make bench_slab
./bench/slab/bench_slab
```

## 构建选项

- `BUILD_BENCHMARKS`: 构建所有 benchmark（包括 slab 和 simple_slab）
- `CMAKE_BUILD_TYPE`: 设置为 `Release` 以获得最佳性能（默认）

## 示例输出

```
=== SlabManager Performance Benchmark ===
Total operations: 500000000
Batch size: 1000
Number of batches: 500000
Block size: 27 bytes

=== Testing SlabManager (with auto-expansion) ===
Initialized SlabManager with block size: 32 bytes
Auto-expansion enabled

SlabManager completed!
Total allocators created: 1
Total slabs allocated: 1

=== Performance Summary ===
----------------------------------------
SlabManager:
  Total time: 9403152 μs
  Average per operation: 9.40315 ns
  Operations per second: 1.06347e+08

Standard malloc/free:
  Total time: 7096412 μs
  Average per operation: 7.09641 ns
  Operations per second: 1.40916e+08

=== Performance Comparison ===
----------------------------------------
SlabManager vs malloc/free: 0.75x slower
SlabManager vs new/delete: 0.99x slower
```

## 性能说明

在当前测试场景下（批量分配1000个32字节对象后批量释放）：
- SlabManager 与 malloc/free 性能接近（约 0.75x）
- SlabManager 与 new/delete 性能几乎相当（约 0.99x）
- 自动扩容功能仅在需要时才创建新的 allocator

## 自定义测试

可以修改 `bench_slab.cpp` 中的以下参数来测试不同场景：

- `WORD_LEN`: 分配的字长（默认 27 字节）
- `BATCH_SIZE`: 每批次的对象数量（默认 1000）
- `TOTAL_OPS`: 总操作数（默认 5 亿）
- 块大小（`init(32, ...)`）: 可以设置为 16, 32, 64, 128, 192, 256, 384, 512, 768, 1024 等
