# Relax Project - 代码覆盖率测试指南

## 概述

本项目已集成代码覆盖率测试功能，支持使用 LCOV 和 gcovr 两种工具生成覆盖率报告。

## 前置要求

### 安装必要的工具

#### Ubuntu/Debian
```bash
sudo apt-get install lcov gcovr
```

#### Fedora/RHEL
```bash
sudo dnf install lcov gcovr
```

#### Arch Linux
```bash
sudo pacman -S lcov gcovr
```

#### macOS
```bash
brew install lcov gcovr
```

## 快速开始

### 方法1: 使用脚本（推荐）

```bash
# 运行测试并生成LCOV覆盖率报告
./run_coverage.sh

# 生成gcovr覆盖率报告
./run_coverage.sh gcovr

# 生成所有类型的报告
./run_coverage.sh all
```

### 方法2: 使用Makefile

```bash
# 查看帮助
make -f Makefile.coverage help

# 运行测试并生成覆盖率报告
make -f Makefile.coverage coverage

# 生成gcovr覆盖率报告
make -f Makefile.coverage coverage-gcovr

# 清理覆盖率文件
make -f Makefile.coverage coverage-clean
```

### 方法3: 手动使用CMake

```bash
# 创建构建目录
mkdir build_coverage
cd build_coverage

# 配置CMake（启用覆盖率）
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON -DBUILD_TESTS=ON

# 构建
make -j$(nproc)

# 运行测试
ctest --verbose

# 生成覆盖率报告
make all_coverage        # LCOV报告
make all_gcovr_coverage  # gcovr报告
```

## 查看覆盖率报告

报告生成后将保存在 `build_coverage/coverage/` 目录下，具体结构如下：

```
build_coverage/coverage/
├── all/             # LCOV生成的总体覆盖率HTML报告
│   ├── index.html   # 主页面
│   └── coverage.xml # CI集成用的XML报告（gcovr生成）
├── gcovr/           # gcovr生成的HTML报告（使用gcovr选项时）
│   └── index.html   # 主页面
├── slab/            # slab模块单独的覆盖率报告
└── time_wheel/      # time_wheel模块单独的覆盖率报告
```

### LCOV HTML报告
```bash
# 默认位置
firefox build_coverage/coverage/all/index.html

# 或使用系统默认浏览器
xdg-open build_coverage/coverage/all/index.html
```

### gcovr HTML报告
```bash
# 默认位置（使用 ./run_coverage.sh gcovr 生成）
firefox build_coverage/coverage/gcovr/index.html
```

### XML报告（用于CI集成）
```bash
# 位置
build_coverage/coverage/all/coverage.xml
```

## 模块特定的覆盖率

### Slab模块
```bash
make -f Makefile.coverage coverage-slab
```

### Time Wheel模块
```bash
make -f Makefile.coverage coverage-time-wheel
```

## CMake选项说明

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ENABLE_COVERAGE` | OFF | 启用代码覆盖率支持 |
| `BUILD_TESTS` | ON | 构建测试 |
| `CMAKE_BUILD_TYPE` | Release | 构建类型（覆盖率测试建议使用Debug） |

## 覆盖率目标

每个测试模块都有以下覆盖率目标：

- `<module>_coverage` - 使用LCOV生成覆盖率报告
- `<module>_gcovr_coverage` - 使用gcovr生成HTML报告
- `<module>_gcovr_xml` - 使用gcovr生成XML报告

例如：
- `slab_coverage`
- `time_wheel_coverage`
- `all_coverage` (所有测试的总体覆盖率)

## 故障排除

### 问题1: 找不到lcov或gcovr
确保已安装必要的工具，参见"前置要求"部分。

### 问题2: 覆盖率为0%
1. 确保使用Debug构建类型
2. 确保启用了ENABLE_COVERAGE选项
3. 确保测试实际运行了

### 问题3: 构建失败
清理构建目录并重新构建：
```bash
make -f Makefile.coverage coverage-clean
make -f Makefile.coverage coverage-fresh
```

## CI集成

在CI环境中，可以使用XML格式的覆盖率报告：

```yaml
# GitHub Actions示例
- name: Run Coverage Tests
  run: |
    ./run_coverage.sh gcovr

- name: Upload Coverage
  uses: codecov/codecov-action@v3
  with:
    file: ./build_coverage/coverage.xml
```

## 注意事项

1. 代码覆盖率测试会显著增加编译时间和二进制文件大小
2. 覆盖率测试应使用Debug构建类型以获得准确结果
3. 生产环境不应启用覆盖率选项
4. 覆盖率报告会排除测试代码本身、系统头文件和第三方库