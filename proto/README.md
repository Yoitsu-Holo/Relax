# Relax Proto 定义

Protocol Buffers 定义文件，用于 Relax 分布式缓存系统的 gRPC 接口。

## 文件说明

- `cache.proto` - 缓存服务 protobuf 定义
- `cache.pb.go` - 生成的 protobuf Go 代码
- `cache_grpc.pb.go` - 生成的 gRPC 服务代码
- `cache.pb.gw.go` - 生成的 gRPC-Gateway HTTP 代理代码

## 重新生成代码

### 前置依赖

```bash
# 安装 protoc 编译器
sudo pacman -S protobuf  # Arch Linux
# 或
sudo apt-get install protobuf-compiler  # Ubuntu/Debian

# 安装 Go 插件
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
go install github.com/grpc-ecosystem/grpc-gateway/v2/protoc-gen-grpc-gateway@latest

# 确保 GOPATH/bin 在 PATH 中
export PATH=$PATH:$(go env GOPATH)/bin
```

### 生成命令

```bash
cd proto
protoc \
  --proto_path=. \
  --go_out=. \
  --go_opt=paths=source_relative \
  --go-grpc_out=. \
  --go-grpc_opt=paths=source_relative \
  --grpc-gateway_out=. \
  --grpc-gateway_opt=paths=source_relative \
  cache.proto
```

## 接口定义

### KV 操作
- `SET` / `GET` / `DEL` / `EXISTS`

### Hash 操作
- `HSET` / `HGET` / `HDEL` / `HMSET` / `HMGET` / `HLEN` / `HGETALL` / `HEXISTS`

### Set 操作
- `SADD` / `SMEMBERS` / `SREM` / `SISMEMBER` / `SCARD`

### List 操作
- `LPUSH` / `LPOP` / `RPUSH` / `RPOP` / `LLEN` / `LINDEX` / `LSET` / `LRANGE` / `LREM`
