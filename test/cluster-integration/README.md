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
- 启动 3 个 Slave 后端节点（node-1, node-2, node-3）
- 启动 1 个 Master 集群代理节点
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

**Slave 节点（后端缓存节点）**:
- **node-1**: gRPC `localhost:25001`, HTTP `localhost:28091`
- **node-2**: gRPC `localhost:25002`, HTTP `localhost:28092`
- **node-3**: gRPC `localhost:25003`, HTTP `localhost:28093`

**Master 节点（集群代理）**:
- **gRPC**: `localhost:25000`
- **HTTP**: `localhost:28080`
- **健康检查**: `http://localhost:28080/health`

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
curl http://localhost:28080/health

# KV 操作
curl -X POST http://localhost:28080/v1/kv/set \
  -H "Content-Type: application/json" \
  -d '{"key":"test","value":"hello"}'

curl http://localhost:28080/v1/kv/get/test

# Hash 操作
curl -X POST http://localhost:28080/v1/hash/set \
  -H "Content-Type: application/json" \
  -d '{"key":"user:1","field":"name","value":"Alice"}'

curl http://localhost:28080/v1/hash/get/user:1/name
```

## 注意事项

1. **端口占用**: 确保以下端口未被占用：
   - Slave 节点: 25001, 25002, 25003 (gRPC), 28091, 28092, 28093 (HTTP)
   - Master 节点: 25000 (gRPC), 28080 (HTTP)
2. **架构说明**:
   - Slave 节点使用 inf-SlaveNode，通过 YAML 配置文件指定端口
   - Master 节点使用 inf-MasterNode，负责路由和负载均衡
   - 所有节点支持 YAML 配置化

## 故障排查

### 服务启动失败

1. 检查端口是否被占用：
   ```bash
   lsof -i :25001
   lsof -i :25002
   lsof -i :25003
   lsof -i :25000
   lsof -i :28091
   lsof -i :28092
   lsof -i :28093
   lsof -i :28080
   ```

2. 查看日志：
   ```bash
   tail -f logs/node1.log
   tail -f logs/node2.log
   tail -f logs/node3.log
   tail -f logs/master.log
   ```

### 测试失败

1. 确认服务正在运行：
   ```bash
   curl http://localhost:28080/health
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
