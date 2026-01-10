# Cluster Unit Tests

集群代理的单元测试套件，测试路由逻辑、负载分布和哈希一致性。

## 测试结构

```
cluster-test/
├── unit/            # 单元测试
│   └── hash_test.go  # 哈希函数和配置测试
├── routing/         # 路由测试
│   └── routing_test.go  # 节点路由逻辑测试
└── distribution/    # 分布测试
    └── distribution_test.go  # 负载均衡和分布测试
```

## 测试内容

### Unit Tests (单元测试)

**hash_test.go**:
- `TestHashKeyConsistency` - 哈希一致性测试
- `TestHashKeyUniqueness` - 哈希唯一性测试
- `TestHashKeyHigh128` - 高 64 位提取测试
- `TestDefaultConfig` - 默认配置测试
- `TestLoadConfigFromYAML` - YAML 配置加载测试
- `TestNodeConfigValidation` - 节点配置验证测试

### Routing Tests (路由测试)

**routing_test.go**:
- `TestManagerCreation` - 管理器创建测试
- `TestGetNodeForKey` - 键到节点路由测试
- `TestConsistentRouting` - 一致性路由测试（100 次迭代）
- `TestGetNodeIndexForKey` - 节点索引计算测试
- `TestDifferentNodeCounts` - 不同节点数量测试（1, 2, 3, 5, 7, 10 个节点）
- `TestGetNodeByID` - 根据 ID 获取节点测试
- `TestGetHealthyNodeCount` - 健康节点统计测试

### Distribution Tests (分布测试)

**distribution_test.go**:
- `TestKeyDistribution` - 键分布测试（多种节点和键数量组合）
- `TestUserKeyDistribution` - 用户类型键分布测试（user:*, session:*, cache:*）
- `TestHotKeyDistribution` - 热点键分布测试
- `TestSequentialKeyDistribution` - 顺序键分布测试（数字和字母）
- `TestScalability` - 扩展性测试（2-50 个节点）

## 运行测试

### 运行所有测试

```bash
cd /home/yoitsuholo/Code/Relax/test/cluster-test
go test -v ./...
```

### 运行特定测试套件

```bash
# 单元测试
go test -v ./unit

# 路由测试
go test -v ./routing

# 分布测试
go test -v ./distribution
```

### 运行单个测试

```bash
go test -v ./unit -run TestHashKeyConsistency
go test -v ./routing -run TestConsistentRouting
go test -v ./distribution -run TestKeyDistribution
```

## 测试特点

1. **无需外部依赖**: 所有测试都是纯单元测试，不需要启动实际的服务器
2. **快速执行**: 所有测试在几秒内完成
3. **详细日志**: 每个测试都输出详细的分析结果
4. **全面覆盖**: 涵盖哈希、路由、分布等所有核心功能

## 测试覆盖的场景

### 哈希一致性
- ✅ 同一个键多次哈希结果一致
- ✅ 不同键产生不同哈希
- ✅ Unicode 键支持
- ✅ 空键处理
- ✅ 特殊字符键处理

### 路由逻辑
- ✅ 键到节点的正确路由
- ✅ 路由一致性（同一键总是路由到同一节点）
- ✅ 支持任意节点数量（1-50 个节点）
- ✅ 节点索引计算正确性

### 负载分布
- ✅ 键在节点间的均衡分布
- ✅ 不同键模式的分布（user:*, session:*, cache:*）
- ✅ 热点键不会集中在单个节点
- ✅ 顺序键的分布
- ✅ 扩展性（节点数从 2 增加到 50）

## 预期结果

运行测试后，你将看到：

1. **哈希测试**: 确认所有键哈希一致且唯一
2. **路由测试**: 确认键正确且一致地路由到节点
3. **分布测试**: 详细的分布统计，显示每个节点的负载百分比

典型输出示例：

```
Distribution of 1000 keys across 3 nodes:
  Node 0: 334 keys (33.40%)
  Node 1: 332 keys (33.20%)
  Node 2: 334 keys (33.40%)
```

## 与集成测试的区别

- **cluster-test/**: 单元测试，测试路由逻辑和算法，无需启动服务
- **inf-cluster-server/**: 集成测试，测试完整的客户端-代理-服务端交互，需要启动服务
