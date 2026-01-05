# unordered_map 性能测试

这是一个用于测试 C++ `std::unordered_map<uint64_t, uint64_t>` 性能的基准测试程序。

## 测试内容

1. **预加载数据**：向 unordered_map 预加载 500,000 条 key-value 数据
2. **性能测试**：执行 1,000,000,000 次 set+get 混合操作（50% set + 50% get）
3. **性能指标**：计算并显示总耗时和 ops（操作数/秒）

## 快速开始（推荐）

### Windows 一键编译

在 Windows 环境下，最简单的方法是使用提供的批处理脚本：

```bash
cd bench
build.bat
```

该脚本会自动检测系统安装的编译器（g++ 或 MSVC）并进行编译。编译成功后直接运行生成的可执行文件。

## 编译方法

### 使用 CMake（需要安装 CMake）

```bash
cd bench
mkdir build && cd build
cmake ..
cmake --build .
```

### 使用 g++ 直接编译

```bash
cd bench
g++ -std=c++17 -O3 -DNDEBUG bench_unordered_map.cpp -o bench_unordered_map
```

### 使用 MSVC 编译（Windows）

```bash
cd bench
cl /std:c++17 /O2 /EHsc bench_unordered_map.cpp
```

### 使用批处理脚本（Windows）

```bash
cd bench
build.bat
```
```

## 运行测试

编译完成后，运行生成的可执行文件：

```bash
./bench_unordered_map
```

## 输出说明

程序将输出以下信息：

- 预加载数据量：500,000 条
- 总测试操作数：1,000,000,000 次
- 预加载耗时
- 性能测试总耗时（秒）
- 平均性能（ops = 操作数 / 总秒数）

## 编译选项说明

- `-std=c++17`：使用 C++17 标准
- `-O3` 或 `/O2`：启用最高级别优化
- `-DNDEBUG`：禁用断言以提高性能

## 系统要求

- C++17 或更高版本的编译器
- 最少 4GB 可用内存（用于存储 50W 条数据）
- 支持 64 位操作系统（uint64 类型）

## 测试环境建议

为了获得准确的性能测试结果，建议：
1. 关闭其他占用 CPU 的程序
2. 使用 Release 编译模式（启用优化）
3. 在性能稳定的机器上运行
4. 运行多次测试取平均值
