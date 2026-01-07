# Ankerl Dense Map 性能测试

该目录包含了对 `ankerl::unordered_dense::map` 的性能基准测试程序。

## 文件说明

- `bench_ankerl_dense_map.cpp` - 主要的性能测试代码
- `CMakeLists.txt` - CMake 构建配置文件
- `build/` - 构建输出目录

## 构建和运行

### 使用 CMake 构建

```bash
# 在 bench/dense_map 目录下
mkdir -p build
cd build
cmake ..
make
```

### 运行测试

```bash
./build/bench_ankerl_dense_map
```

## 测试参数

- **预加载数据量**: 500,000 条
- **测试操作数**: 1,000,000,000 次 (10亿)
- **操作类型**: 50% set 操作，50% get 操作
- **键值范围**: 0 到 999,999

## 性能结果示例

```
========== Test Results ==========
Library: ankerl::unordered_dense::map
Preload count: 500000 entries
Total test operations: 1000000000 operations
Preload time: 0.0595559 seconds
Performance test total time: 28.7235 seconds
Average performance: 34814745 ops/s
Final map size: 750000
==================================
```

## 与 std::unordered_map 的对比

该测试使用相同的参数和操作模式，以便与 `bench/umap/bench_unordered_map.cpp` 中的 `std::unordered_map` 进行公平对比。

## 优化选项

CMake 配置中使用了以下优化选项：
- `-O3`: 最高级别的优化
- `-march=native`: 针对本机 CPU 架构优化
- `-DNDEBUG`: 禁用调试断言