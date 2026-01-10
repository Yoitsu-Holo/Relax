# inf-slave-server 缓存测试套件

这是为 inf-SingleNode 内部缓存设计的完善测试套件，采用标准 Go test 方式。

## 目录结构

```
test/inf-slave-server/
├── kv/              # KV 操作测试
├── hash/            # Hash 操作测试
├── set/             # Set 操作测试
├── list/            # List 操作测试
├── concurrency/     # 并发测试
├── benchmark/       # 性能基准测试
├── go.mod           # Go 模块文件
└── README.md        # 本文档
```

## 测试覆盖内容

### 1. KV 操作测试 (kv/)

测试基本的键值对操作：
- `TestKVBasicSetGet`: 基本的 Set/Get 操作
- `TestKVGetNonExistent`: 获取不存在的键
- `TestKVSetOverwrite`: 覆盖已存在的键
- `TestKVDel`: 删除操作
- `TestKVExists`: 键存在性检查
- `TestKVWrongType`: 类型错误处理
- `TestKVMultipleKeys`: 多键操作
- `TestKVSharding`: 分片分布测试

### 2. Hash 操作测试 (hash/)

测试哈希表操作：
- `TestHashBasicHSetHGet`: 基本的 HSet/HGet 操作
- `TestHashHSetOverwrite`: 字段覆盖
- `TestHashHGetNonExistent`: 获取不存在的哈希/字段
- `TestHashHDel`: 字段删除
- `TestHashHMSet`: 批量设置字段
- `TestHashHMGet`: 批量获取字段
- `TestHashHLen`: 哈希长度
- `TestHashHGetAll`: 获取所有字段
- `TestHashHExists`: 字段存在性检查
- `TestHashWrongType`: 类型错误处理
- `TestHashEmptyFieldCleanup`: 空哈希清理
- `TestHashMultipleFields`: 大量字段测试
- `TestHashMixedKeys`: 多哈希操作

### 3. Set 操作测试 (set/)

测试集合操作：
- `TestSetBasicSAdd`: 基本的 SAdd 操作
- `TestSetSMembers`: 获取所有成员
- `TestSetSRem`: 移除成员
- `TestSetSIsMember`: 成员存在性检查
- `TestSetSCard`: 集合基数
- `TestSetWrongType`: 类型错误处理
- `TestSetEmptyCleanup`: 空集合清理
- `TestSetLargeSet`: 大集合测试
- `TestSetMultipleSets`: 多集合操作
- `TestSetDuplicateAdd`: 重复添加
- `TestSetUnicodeMembers`: Unicode 成员

### 4. List 操作测试 (list/)

测试列表操作：
- `TestListBasicLPush/RPush`: 基本的 Push 操作
- `TestListLPushRPushOrder`: Push 顺序验证
- `TestListLRange`: 范围获取（含负索引）
- `TestListLPop/RPop`: Pop 操作
- `TestListLLen`: 列表长度
- `TestListLIndex`: 索引访问
- `TestListLSet`: 设置索引值
- `TestListLRem`: 移除元素
- `TestListWrongType`: 类型错误处理
- `TestListEmptyCleanup`: 空列表清理
- `TestListLargeList`: 大列表测试

### 5. 并发测试 (concurrency/)

测试多线程场景：
- `TestConcurrentKVOperations`: 并发 KV 操作
- `TestConcurrentReadWrite`: 并发读写
- `TestConcurrentHashOperations`: 并发 Hash 操作
- `TestConcurrentSetOperations`: 并发 Set 操作
- `TestConcurrentListOperations`: 并发 List 操作
- `TestConcurrentMixedOperations`: 混合并发操作
- `TestConcurrentDeleteOperations`: 并发删除
- `TestConcurrentShardAccess`: 分片并发访问
- `TestConcurrentTypeConflicts`: 类型冲突处理
- `TestRaceConditionDetection`: 竞态条件检测

### 6. 性能基准测试 (benchmark/)

各种性能基准测试：
- **KV 基准**: Set, Get, SetParallel, GetParallel, Mixed
- **Hash 基准**: HSet, HGet, HSetParallel, HGetParallel, HMSet, HGetAll
- **Set 基准**: SAdd, SIsMember, SAddParallel, SMembers
- **List 基准**: LPush, RPush, LPushParallel, LRange, LPop, RPop
- **分片基准**: 8/32/64/128 分片对比
- **高级基准**: MemoryFootprint, HighConcurrency, Contention
- **综合基准**: RealisticWorkload, ReadHeavy, WriteHeavy

## 运行测试

### 运行所有单元测试

```bash
cd test/inf-slave-server
go test ./...
```

### 运行特定类型的测试

```bash
# 运行 KV 测试
go test -v ./kv

# 运行 Hash 测试
go test -v ./hash

# 运行 Set 测试
go test -v ./set

# 运行 List 测试
go test -v ./list

# 运行并发测试
go test -v ./concurrency
```

### 运行特定的测试用例

```bash
# 运行特定测试
go test -v ./kv -run TestKVBasicSetGet

# 运行匹配模式的测试
go test -v ./hash -run "^TestHash.*Get"
```

### 运行竞态检测

```bash
# 运行带竞态检测的测试
go test -race ./concurrency
```

### 运行基准测试

```bash
# 运行所有基准测试
go test -bench=. ./benchmark

# 运行特定基准测试
go test -bench=BenchmarkKV ./benchmark

# 运行基准测试并显示内存分配
go test -bench=. -benchmem ./benchmark

# 运行基准测试并指定时间
go test -bench=BenchmarkKVSet -benchtime=5s ./benchmark
```

### 性能分析

```bash
# CPU 分析
go test -bench=BenchmarkKVSetParallel -cpuprofile=cpu.prof ./benchmark
go tool pprof cpu.prof

# 内存分析
go test -bench=BenchmarkKVSetParallel -memprofile=mem.prof ./benchmark
go tool pprof mem.prof
```

### 生成测试覆盖率报告

```bash
# 生成覆盖率报告
go test -coverprofile=coverage.out ./...

# 查看覆盖率统计
go tool cover -func=coverage.out

# 在浏览器中查看详细报告
go tool cover -html=coverage.out
```

## 测试特性

### 1. 全面的功能覆盖
- 涵盖所有数据类型（String, Hash, Set, List）
- 测试正常流程和边界情况
- 验证错误处理和类型安全

### 2. 并发安全性
- 大量并发测试确保线程安全
- 支持 `-race` 标志进行竞态检测
- 测试多种并发场景和压力情况

### 3. 性能基准
- 针对各种操作的性能测试
- 并行和串行性能对比
- 不同分片数量的性能对比
- 真实工作负载模拟

### 4. 边界情况
- 空值、大数据量测试
- Unicode 字符支持
- 负索引处理
- 类型冲突处理

## 测试最佳实践

1. **定期运行测试**: 在修改代码后运行完整测试套件
2. **使用竞态检测**: 对并发代码使用 `-race` 标志
3. **监控性能**: 定期运行基准测试追踪性能变化
4. **覆盖率目标**: 保持高测试覆盖率（目标 > 80%）
5. **CI 集成**: 将测试集成到 CI/CD 流程中

## 测试结果示例

```bash
$ go test ./...
ok      github.com/yoitsuholo/relax/test/inf-slave-server/kv           0.010s
ok      github.com/yoitsuholo/relax/test/inf-slave-server/hash         0.006s
ok      github.com/yoitsuholo/relax/test/inf-slave-server/set          0.008s
ok      github.com/yoitsuholo/relax/test/inf-slave-server/list         0.010s
ok      github.com/yoitsuholo/relax/test/inf-slave-server/concurrency  0.250s
```

## 已知限制

1. SimpleMapCache 的 List 操作未完全实现（返回错误）
2. 测试主要针对 ShardedCache 实现
3. 某些边界情况可能需要额外测试

## 贡献指南

添加新测试时，请遵循以下规范：
1. 使用描述性的测试名称
2. 每个测试应该测试单一功能点
3. 使用表驱动测试处理多个输入
4. 添加适当的注释说明测试目的
5. 确保测试是独立的，不依赖其他测试

## 许可证

与主项目相同
