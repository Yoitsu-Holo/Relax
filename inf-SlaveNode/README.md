# Relax 单机缓存节点

Redis-like 单机缓存服务器实现，支持多种数据类型和 gRPC/HTTP 接口。

## 架构特性

- **模块化设计**: 清晰的接口层和实现层分离，支持多种缓存实现
- **分片存储**: 默认 32 个独立 shard，使用 xxhash128 低 64 位进行路由
- **并发安全**: 每个 shard 独立的读写锁，最大化并发性能
- **哈希冲突检测**: 使用 CRC32 校验和验证 key 完整性
- **类型支持**: String / Hash / Set / List（Redis-like 数据结构）
- **双协议**: 同时支持 gRPC 和 HTTP REST API
- **可扩展**: 预留高性能缓存实现接口

## 启动服务

```bash
cd server
go build
./server

# 或直接运行
go run main.go
```

服务端口：
- gRPC: `:50051`
- HTTP: `:8080`

## 测试接口

### KV 操作

```bash
# Set
curl -X POST http://localhost:8080/v1/kv/set \
  -d '{"key":"user:1001","value":"Alice"}'

# Get
curl http://localhost:8080/v1/kv/get/user:1001

# Delete
curl -X DELETE http://localhost:8080/v1/kv/del/user:1001

# Exists
curl http://localhost:8080/v1/kv/exists/user:1001
```

### Hash 操作

```bash
# HSet
curl -X POST http://localhost:8080/v1/hash/set \
  -d '{"key":"profile:1001","field":"name","value":"Alice"}'

# HGet
curl http://localhost:8080/v1/hash/get/profile:1001/name

# HMSet (批量设置)
curl -X POST http://localhost:8080/v1/hash/mset \
  -d '{"key":"profile:1001","fields":{"name":"Alice","age":"25"}}'

# HGetAll
curl http://localhost:8080/v1/hash/getall/profile:1001

# HLen
curl http://localhost:8080/v1/hash/len/profile:1001
```

### Set 操作

```bash
# SAdd
curl -X POST http://localhost:8080/v1/set/add \
  -d '{"key":"tags:golang","member":"backend"}'

# SMembers
curl http://localhost:8080/v1/set/members/tags:golang

# SIsMember
curl http://localhost:8080/v1/set/ismember/tags:golang/backend

# SCard
curl http://localhost:8080/v1/set/card/tags:golang
```

### List 操作

```bash
# LPush (左侧插入)
curl -X POST http://localhost:8080/v1/list/push \
  -d '{"key":"queue:tasks","value":"task1"}'

# RPush (右侧插入)
curl -X POST http://localhost:8080/v1/list/rpush \
  -d '{"key":"queue:tasks","value":"task2"}'

# LRange
curl "http://localhost:8080/v1/list/range/queue:tasks?start=0&stop=-1"

# LPop
curl -X POST http://localhost:8080/v1/list/lpop \
  -d '{"key":"queue:tasks"}'

# LLen
curl http://localhost:8080/v1/list/len/queue:tasks
```

## 目录结构

```
server/
├── main.go                    # 服务入口（gRPC + HTTP Gateway）
├── service/
│   └── cache_service.go       # gRPC 服务实现层
└── cache/
    ├── cache.go               # Cache 接口定义（41个方法）
    ├── simple.go              # 简单单锁缓存实现
    ├── simpleCache/           # 分片缓存实现（默认使用）
    │   ├── base.go           # 核心结构和辅助函数
    │   ├── kv.go             # KV 操作实现
    │   ├── hash.go           # Hash 操作实现
    │   ├── set.go            # Set 操作实现
    │   └── list.go           # List 操作实现
    └── highPerfCache/         # 预留：高性能缓存实现
```

## 缓存实现说明

### simpleCache (当前使用)
- **架构**: 分片哈希表 + 独立锁
- **分片数**: 默认 32（可配置，必须为 2 的幂）
- **哈希算法**: xxHash128（取低 64 位）
- **冲突检测**: CRC32 校验和
- **并发策略**: 每个 shard 独立 RWMutex
- **路由算法**: `shard_index = hash & (shard_count - 1)`

### highPerfCache (预留)
未来可实现的高性能方案：
- 无锁数据结构（lock-free）
- SIMD 优化哈希
- 内存池优化
- 零拷贝 I/O

## 配置模块路径

项目使用 Go modules，确保 `go.mod` 中正确配置：

```go
replace github.com/yoitsuholo/relax/proto => ../../proto
```

## 技术实现细节

### 数据类型存储

每个 key 通过 xxHash128 计算哈希值，并附加 CRC32 校验和：

```go
type cacheValue struct {
    valueType dataType  // typeString/typeHash/typeSet/typeList
    checksum  uint32    // CRC32 校验和（防止哈希碰撞）

    // 类型特定数据（只使用一个）
    stringValue string
    hashValue   map[string]string
    setValue    map[string]struct{}
    listValue   []string
}
```

### 分片路由

```go
// 计算 key 的哈希值
hash := xxh3.Hash128([]byte(key)).Lo  // 取低 64 位

// 快速取模（等价于 hash % shard_count，但更快）
shard_index := hash & (shard_count - 1)
```

### 并发控制

- **读操作**: 使用 `RLock()`，允许并发读
- **写操作**: 使用 `Lock()`，独占访问
- **粒度**: 每个 shard 独立锁，32 个并发通道

## 性能优化

1. **哈希算法**: xxHash128（比 CRC32 快 10x+）
2. **位运算取模**: `&` 代替 `%`（要求 shard 数量为 2 的幂）
3. **分片设计**: 减少锁竞争，提升并发性能
4. **预分配**: `make(map[T]V, capacity)` 减少扩容
5. **空检查**: 删除操作会清理空的 hash/set/list

## 开发计划

- [ ] 实现 TTL 过期机制
- [ ] 添加持久化支持（AOF/RDB）
- [ ] 实现 Pub/Sub 功能
- [ ] 添加性能监控和统计
- [ ] 开发 highPerfCache 实现
- [ ] 支持集群模式
