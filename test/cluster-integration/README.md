# 集群集成测试

完整的端到端集群测试环境，包含启动、测试和停止脚本。

## 目录结构

```
cluster-integration/
├── configs/              # 配置文件
│   ├── cluster.yaml      # 开发环境配置
│   └── cluster.production.yaml  # 生产环境配置示例
├── scripts/              # 测试脚本
│   ├── start.sh         # 启动集群
│   ├── stop.sh          # 停止集群
│   └── test.sh          # 运行测试
└── logs/                # 运行日志（自动生成）
```

## 快速开始

### 1. 启动集群

```bash
cd /home/yoitsuholo/Code/Relax/test/cluster-integration
bash scripts/start.sh
```

这将自动：
- 启动后端节点（SingleNode）
- 启动集群代理（ClusterNode）
- 执行健康检查
- 显示服务信息

### 2. 运行测试

```bash
bash scripts/test.sh
```

测试内容包括：
- ✅ 健康检查
- ✅ KV 操作（Set, Get, Del, Exists）
- ✅ Hash 操作（HSet, HGet, HMSet, HGetAll, HLen）
- ✅ Set 操作（SAdd, SMembers, SIsMember, SCard）
- ✅ List 操作（LPush, RPush, LRange, LLen）
- ✅ 路由一致性验证
- ✅ 并发写入测试

### 3. 停止集群

```bash
bash scripts/stop.sh
```

## 服务端口

启动后的服务端口：

- **后端节点**: `localhost:50051`
- **集群代理 gRPC**: `localhost:50052`
- **集群代理 HTTP**: `localhost:8081`
- **健康检查**: `http://localhost:8081/health`

## 配置说明

### 开发环境配置 (configs/cluster.yaml)

```yaml
nodes:
  - id: node-1
    address: localhost:50051
  - id: node-2
    address: localhost:50053
  - id: node-3
    address: localhost:50054

health_check_interval: 10s
grpc_port: ":50052"
http_port: ":8081"
```

### 修改配置

1. 编辑 `configs/cluster.yaml`
2. 重启集群：
   ```bash
   bash scripts/stop.sh
   bash scripts/start.sh
   ```

## 日志查看

日志文件位于 `logs/` 目录：

```bash
# 查看后端节点日志
tail -f logs/node1.log

# 查看集群代理日志
tail -f logs/cluster.log
```

## 手动测试

启动集群后，可以手动测试 API：

```bash
# 健康检查
curl http://localhost:8081/health

# KV 操作
curl -X POST http://localhost:8081/v1/kv/set \
  -H "Content-Type: application/json" \
  -d '{"key":"test","value":"hello"}'

curl http://localhost:8081/v1/kv/get/test

# Hash 操作
curl -X POST http://localhost:8081/v1/hash/set \
  -H "Content-Type: application/json" \
  -d '{"key":"user:1","field":"name","value":"Alice"}'

curl http://localhost:8081/v1/hash/get/user:1/name
```

## 注意事项

1. **端口占用**: 确保端口 50051, 50052, 8081 未被占用
2. **单节点限制**: 当前 SingleNode 不支持同时运行多个实例，因此只启动一个后端节点
3. **多节点测试**: 如需测试多节点集群，需要手动启动额外的 SingleNode 实例

## 故障排查

### 服务启动失败

1. 检查端口是否被占用：
   ```bash
   lsof -i :50051
   lsof -i :50052
   lsof -i :8081
   ```

2. 查看日志：
   ```bash
   tail -f logs/node1.log
   tail -f logs/cluster.log
   ```

### 测试失败

1. 确认服务正在运行：
   ```bash
   curl http://localhost:8081/health
   ```

2. 检查配置文件是否正确：
   ```bash
   cat configs/cluster.yaml
   ```

3. 重启服务：
   ```bash
   bash scripts/stop.sh
   bash scripts/start.sh
   ```

## 清理

停止服务并清理日志：

```bash
bash scripts/stop.sh
# 选择 'y' 清理日志文件
```

## 与其他测试的区别

- **cluster-test/**: 单元测试，无需启动服务，测试路由逻辑
- **inf-cluster-server/**: 集成测试，需要完整的测试集群，测试所有操作
- **cluster-integration/**: E2E 测试，模拟生产环境，手动启动和测试

## 持续集成

在 CI 环境中使用：

```bash
# 启动
bash scripts/start.sh

# 测试
bash scripts/test.sh

# 停止
bash scripts/stop.sh
```

测试脚本返回值：
- `0`: 所有测试通过
- `1`: 部分测试失败
