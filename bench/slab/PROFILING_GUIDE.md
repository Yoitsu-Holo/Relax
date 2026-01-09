# 性能分析火焰图使用指南

## 快速开始

```bash
cd /home/yoitsuholo/Code/Relax

# 生成火焰图并自动打开
flamegraph --open -F 999 -o bench/slab/flamegraph.svg -- build/bench/slab/profile_slab

# 或使用确定性颜色（便于对比优化前后）
flamegraph --deterministic -F 999 -o bench/slab/flamegraph.svg -- build/bench/slab/profile_slab
```

## 火焰图解读

- **横轴（宽度）**: CPU 时间占比，越宽越耗时
- **纵轴（高度）**: 调用栈深度，从下到上是调用关系
- **可交互**: 点击函数框可以放大查看

重点关注：
1. **最宽的框** - 性能热点
2. **调用路径** - 从 `main` 向上追溯
3. **平顶** - 函数本身消耗 CPU（非其子函数）

## 常用命令

### 生成火焰图

```bash
# 基本用法
flamegraph -F 999 -o flamegraph.svg -- ./profile_slab

# 生成并打开
flamegraph --open -F 999 -o flamegraph.svg -- ./profile_slab

# 自定义标题
flamegraph -F 999 \
  --title "Slab Allocator Performance" \
  --subtitle "1亿次操作 @ 999Hz" \
  -o flamegraph.svg -- ./profile_slab

# 确定性颜色（对比测试用）
flamegraph --deterministic -F 999 -o flamegraph.svg -- ./profile_slab
```

### 对比优化前后

```bash
# 优化前
flamegraph --deterministic -F 999 -o flamegraph_before.svg -- ./profile_slab

# 修改代码并重新编译
cmake --build build

# 优化后
flamegraph --deterministic -F 999 -o flamegraph_after.svg -- ./profile_slab

# 并排打开对比
firefox flamegraph_before.svg flamegraph_after.svg
```

### 生成 perf 报告

```bash
# 采集数据
perf record -F 999 -g --call-graph dwarf ./profile_slab

# 交互式查看
perf report

# 生成文本报告
perf report --stdio -n --percent-limit 0.5 > perf_report.txt

# 从 perf.data 生成火焰图
perf script | flamegraph -o flamegraph.svg
```

## 采样频率选择

```bash
flamegraph -F 99 -o fg.svg -- ./program     # 低频（长时间运行）
flamegraph -F 499 -o fg.svg -- ./program    # 中频（平衡）
flamegraph -F 999 -o fg.svg -- ./program    # 高频（推荐）
flamegraph -F 4999 -o fg.svg -- ./program   # 超高频（短时测试）
```

## 常见问题

### 权限问题

```bash
# 使用 --root 标志
flamegraph --root -F 999 -o flamegraph.svg -- ./profile_slab

# 或临时允许非 root 用户
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### 火焰图太大

```bash
# 降低采样频率
flamegraph -F 99 -o flamegraph.svg -- ./profile_slab

# 或修改程序减少操作数
```

### 过滤函数

```bash
# 只看 Slab 相关函数
perf report --stdio --symbol-filter="Slab*"

# 排除标准库函数
perf report --stdio --symbol-filter="!std*,!__*"
```

### perf report 交互式命令

- `Enter`: 展开/折叠调用树
- `a`: 显示汇编代码
- `s`: 切换排序方式
- `/`: 搜索符号
- `q`: 退出

## 参考资料

- [Flamegraph Rust 实现](https://github.com/flamegraph-rs/flamegraph)
- [Brendan Gregg's FlameGraph](http://www.brendangregg.com/flamegraphs.html)
- [Linux perf Examples](http://www.brendangregg.com/perf.html)
- 完整分析报告：`PERFORMANCE_ANALYSIS.md`

## 快速参考

```bash
# flamegraph 常用命令
flamegraph -F 999 -o fg.svg -- ./program              # 生成火焰图
flamegraph --open -F 999 -o fg.svg -- ./program       # 生成并打开
flamegraph --deterministic -F 999 -o fg.svg -- ./prog # 确定性颜色
flamegraph --root -F 999 -o fg.svg -- ./program       # root 权限

# perf 常用命令
perf record -F 999 -g --call-graph dwarf ./program    # 采集数据
perf report                                           # 交互式查看
perf report --stdio > report.txt                      # 文本报告
perf script | flamegraph -o flamegraph.svg            # 生成火焰图
```
