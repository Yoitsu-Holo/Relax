# Relax 集群代理节点

分布式缓存集群代理，使用 xxhash128 高 64 位路由请求到后端节点。

## 架构说明

```
客户端 → 集群代理 → [Node-1] [Node-2] [Node-3] ...
           ↓
     xxhash128(key).Hi % node_count
```

## 路由策略

- **Hash 算法**: xxhash128
- **路由位**: 高 64 位 (Hi) 用于节点选择
- **节点选择**: `hash % node_count` (支持任意节点数量)
- **故障转移**: 自动切换到健康节点
- **健康检查**: 定期检测节点状态

## 快速开始

### 1. 创建配置文件

复制示例配置并修改：

```bash
cp config.example.yaml config.yaml
# 编辑 config.yaml，配置后端节点地址
```

配置示例：

```yaml
nodes:
  - id: node-1
    address: localhost:50051
  - id: node-2
    address: localhost:50052

health_check_interval: 10s
grpc_port: ":50060"
http_port: ":8080"
```

### 2. 启动服务

```bash
# 直接运行
go run main.go -config=config.yaml

# 或构建后运行
go build -o cluster-node main.go
./cluster-node -config=config.yaml
```

### 3. 验证服务

```bash
# 健康检查
curl http://localhost:8080/health

# 基础操作
curl -X POST http://localhost:8080/v1/kv/set \
  -H "Content-Type: application/json" \
  -d '{"key":"test","value":"hello"}'

curl http://localhost:8080/v1/kv/get/test
```

## 目录结构

```
inf-MasterNode/
├── main.go              # 入口文件
├── cluster/             # 集群管理
│   ├── config.go        # 配置加载
│   ├── manager.go       # 节点管理器（路由逻辑）
│   └── node.go          # 节点连接
├── proxy/               # 代理层
│   └── cache_proxy.go   # 请求转发（所有 proto 操作）
├── config.example.yaml  # 配置示例
├── go.mod               # Go 模块
├── go.sum
└── README.md
```

## 测试

### 单元测试

```bash
# 测试所有包
go test -v ./...

# 只测试 cluster 包
go test -v ./cluster/...

# 带竞态检测
go test -v -race ./...
```

### 集成测试

完整的集成测试环境位于 `../test/` 目录：

```bash
# 单元测试（路由、分布、哈希）
cd ../test/cluster-test
go test -v ./...

# 集成测试（完整功能测试）
cd ../test/inf-cluster-server
go test -v ./...

# E2E 测试（启动真实服务）
cd ../test/cluster-integration
bash scripts/start.sh
bash scripts/test.sh
bash scripts/stop.sh
```

详见：
- [单元测试文档](../test/cluster-test/README.md)
- [集成测试文档](../test/inf-cluster-server/README.md)
- [E2E 测试文档](../test/cluster-integration/README.md)

## 构建

```bash
# 构建二进制
go build -o bin/cluster-node main.go

# 输出: bin/cluster-node
./bin/cluster-node -config=config.yaml
```

## 配置说明

配置文件支持 YAML 格式，参数说明：

- `nodes`: 后端节点列表
  - `id`: 节点唯一标识
  - `address`: 节点地址（host:port）
- `health_check_interval`: 健康检查间隔（如 `10s`, `1m`）
- `grpc_port`: gRPC 服务端口
- `http_port`: HTTP 网关端口

## 开发

```bash
# 格式化代码
go fmt ./...

# 静态检查
go vet ./...

# 下载依赖
go mod download
go mod tidy

# 清理构建文件
rm -rf bin/
```
