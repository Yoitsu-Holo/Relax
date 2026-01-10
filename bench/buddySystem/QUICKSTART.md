# Quick Start Guide

快速开始使用 Buddy System 性能测试。

## 快速编译和运行

```bash
# 进入 bench 目录
cd bench/buddySystem

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# 运行性能测试
./bench_buddy
```

## 预期输出示例

```
=== Buddy System Performance Benchmark ===
Total operations: 10000000
Batch size: 10000
Number of batches: 1000

========================================
Testing with block size: 4KiB (4096 bytes)
========================================

=== Buddy System ===
Initialized Buddy System (16 MiB)
Buddy System completed!

=== Standard Library (malloc/free) ===

=== Standard Library (new/delete) ===

=== Performance Summary ===
----------------------------------------

Buddy System:
  Total time: 2847123 μs (2847123456 ns)
  Average per operation: 142.36 ns
  Average per alloc: 284.71 ns
  Average per dealloc: 284.71 ns
  Operations per second: 7029384.23

Standard malloc/free:
  Total time: 3201456 μs (3201456789 ns)
  Average per operation: 160.07 ns
  Average per alloc: 320.15 ns
  Average per dealloc: 320.15 ns
  Operations per second: 6247156.89

Standard new/delete:
  Total time: 3189234 μs (3189234567 ns)
  Average per operation: 159.46 ns
  Average per alloc: 318.92 ns
  Average per dealloc: 318.92 ns
  Operations per second: 6271345.78

=== Performance Comparison ===
----------------------------------------
Buddy System vs malloc/free: 1.12x faster (12.45% improvement)
Buddy System vs new/delete: 1.12x faster (12.01% improvement)
```

## 性能分析（火焰图）

```bash
# 编译 profile 版本
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make profile_buddy

# 运行性能分析
perf record -F 99 -g ./profile_buddy

# 生成火焰图（需要先安装 FlameGraph 工具）
perf script | \
    /path/to/FlameGraph/stackcollapse-perf.pl | \
    /path/to/FlameGraph/flamegraph.pl > buddy_flamegraph.svg

# 在浏览器中查看
firefox buddy_flamegraph.svg
```

## 测试不同配置

### 修改操作数量

编辑 `bench_buddy.cpp`:

```cpp
constexpr int TOTAL_OPS = 50000000;  // 增加到 5000万
```

### 修改批次大小

```cpp
constexpr int BATCH_SIZE = 20000;  // 增加批次大小
```

### 测试特定块大小

注释掉不需要的大小:

```cpp
size_t test_sizes[] = {4096, 32768};  // 只测试 4KiB 和 32KiB
```

## 常见问题

### Q: 编译失败 - 找不到头文件

A: 确保在正确的目录下运行 cmake，路径应该是 `bench/buddySystem/build`

### Q: perf 权限错误

A: 运行以下命令临时允许非 root 用户使用 perf:
```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### Q: 性能结果波动很大

A:
- 关闭其他程序减少系统负载
- 多次运行取平均值
- 使用 `taskset` 绑定到特定 CPU 核心：
```bash
taskset -c 0 ./bench_buddy
```

## 下一步

- 查看 `README.md` 了解完整文档
- 查看 `PROFILING_GUIDE.md` 学习深度性能分析
- 修改测试参数进行自定义测试
