# Buddy System 性能基准测试

本目录包含 Buddy System 内存分配器的性能基准测试。

## 概述

基准测试套件在各种块大小下对比 Buddy System 与标准内存分配方法（malloc/free 和 new/delete）的性能。

## 文件列表

- `bench_buddy.cpp` - 主基准测试，对比 Buddy System vs malloc/free vs new/delete
- `profile_buddy.cpp` - 优化用于火焰图分析的性能分析版本
- `CMakeLists.txt` - CMake 构建配置
- `PROFILING_GUIDE.md` - 性能分析和火焰图生成指南
- `PERFORMANCE_REPORT.md` - 详细性能测试报告（**推荐阅读**）

## 构建

### 快速构建

```bash
mkdir build
cd build
cmake ..
make
```

### 构建选项

```bash
# Release 构建（最大优化）
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# RelWithDebInfo 构建（优化 + 调试符号，用于性能分析）
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```

## 运行基准测试

### 性能基准测试

```bash
./bench_buddy
```

测试将覆盖多种块大小（4KiB 到 256KiB）并输出：
- 微秒和纳秒为单位的总执行时间
- 纳秒为单位的平均分配/释放时间
- 每秒操作数
- 性能对比（加速因子）

### 火焰图性能分析

```bash
# 使用 perf 运行
perf record -F 99 -g ./profile_buddy

# 生成火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > buddy_flamegraph.svg
```

详细的性能分析说明请参见 `PROFILING_GUIDE.md`。

## 基准测试配置

`bench_buddy.cpp` 中的默认配置：
- **总操作数**：10,000,000（1000 万次）
- **批次大小**：动态调整（基于块大小和可用内存）
  - 4KiB: 2000 个/批次
  - 8KiB: 2000 个/批次
  - 16KiB: 1000 个/批次
  - 32KiB: 500 个/批次
  - 64KiB: 250 个/批次
  - 128KiB: 100 个/批次
  - 256KiB: 50 个/批次
- **测试大小**：4KiB, 8KiB, 16KiB, 32KiB, 64KiB, 128KiB, 256KiB

**关键改进**：批次大小现在会动态调整，避免内存耗尽同时最大化分配器压力。这解决了早期版本中 32KiB 块无法正确测试的问题。

您可以修改这些常量来调整测试工作负载。

## 测试结果（2026-01-10）

实际性能表现（基于最新测试）：

### 性能汇总

| 块大小 | Buddy (ns/分配) | malloc (ns/分配) | 性能对比 | 结果 |
|--------|----------------|------------------|---------|------|
| 4 KiB  | 108.24         | 99.25            | 0.92x   | 慢 8% |
| 8 KiB  | 75.59          | 116.98           | 1.55x   | **快 55%** ⭐ |
| 16 KiB | 81.30          | 91.72            | 1.13x   | **快 13%** |
| 32 KiB | 68.42          | 88.33            | 1.29x   | **快 29%** |
| 64 KiB | 87.63          | 100.02           | 1.14x   | **快 14%** |
| 128 KiB| 70.52          | 104.78           | 1.49x   | **快 49%** ⭐ |
| 256 KiB| 69.43          | 96.13            | 1.38x   | **快 38%** |

**胜率**：7 个测试中赢得 6 个（85.7%）

### Buddy System 优势

✅ **适用场景：**
- 中大型块分配（8KiB - 256KiB）：平均快 33%
- 可预测的分配时间（O(1) 到 O(log N)）
- 通过 buddy 合并实现低碎片
- 2 的幂次大小零外部碎片
- 优秀的缓存局部性

⚠️ **局限性：**
- 小块（4KiB）：由于位图开销慢 8-18%
- 固定内存池：16MiB 上限
- 仅支持 2 的幂次大小（4KiB, 8KiB, 16KiB, 32KiB, 64KiB, 128KiB, 256KiB）

📊 **详细分析请参阅 `PERFORMANCE_REPORT.md`**

## 理解测试结果

### 时间指标
- **总时间**：所有操作的整体执行时间
- **每操作平均时间**：一次分配+释放对的时间
- **每次分配/释放平均时间**：单次操作的时间
- **每秒操作数**：吞吐量指标

### 性能对比
- 值 > 1.0x 表示 Buddy System 更快
- 值 < 1.0x 表示 Buddy System 更慢
- 百分比显示相对改进/退化程度

## 注意事项

- 基准测试对 Buddy System 使用对齐内存分配
- 所有测试执行相同数量的操作以确保公平对比
- 性能分析版本运行更多迭代以获得更好的统计采样
- 结果可能因系统负载和硬件特性而异
- **关键修复**：动态批次大小确保所有块大小都能正确测试，避免内存耗尽

## 快速开始

```bash
# 编译并运行基准测试
g++ -std=c++17 -O2 -o bench_buddy bench_buddy.cpp ../../cache-Kernel/buddySystem/buddy_allocator.cpp
./bench_buddy

# 或使用 CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./bench_buddy
```

## 参考文档

- 📊 **PERFORMANCE_REPORT.md** - 完整的性能分析报告（强烈推荐）
- 🔥 **PROFILING_GUIDE.md** - 性能分析和火焰图指南
- 💻 **bench_buddy.cpp** - 基准测试源代码
- 🧪 **../../cache-Kernel/buddySystem/** - Buddy 分配器实现

---

**最后更新**：2026-01-10
**版本**：v2.0（修复了 32KiB 分配问题）
