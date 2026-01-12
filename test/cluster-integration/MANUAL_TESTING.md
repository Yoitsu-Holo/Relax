# 集群集成测试 - 手动测试指南

本指南介绍如何手动启动集群并使用 curl 命令进行测试。

## 前置要求

1. 确保已经编译了所需的共享库：
   ```bash
   cd /home/yoitsuholo/Code/Relax
   # 如果需要，运行 CMake 构建
   ```

2. 确保 Go 依赖已安装：
   ```bash
   cd test/cluster-integration
   go mod download
   ```

## 启动集群

### 1. 启动服务

```bash
cd /home/yoitsuholo/Code/Relax/test/cluster-integration
bash scripts/start.sh
```

启动脚本将会：
- 启动 3 个 Slave 节点（后端缓存节点）
  - node-1: gRPC `localhost:25001`, HTTP `localhost:28091`
  - node-2: gRPC `localhost:25002`, HTTP `localhost:28092`
  - node-3: gRPC `localhost:25003`, HTTP `localhost:28093`
- 启动 1 个 Master 节点（集群代理）
  - gRPC: `localhost:25000`
  - HTTP: `localhost:28080`
- 执行健康检查

### 2. 验证服务启动

检查所有服务是否正常运行：

```bash
# 查看进程
ps aux | grep -E "(inf-MasterNode|inf-SlaveNode)"

# 查看日志
tail -f logs/master.log
tail -f logs/node1.log
```

## 手动 curl 测试

**重要提示**：如果你的系统设置了 HTTP 代理，需要在测试前禁用代理，否则 curl 会尝试通过代理连接 localhost：

```bash
# 禁用代理（每次测试前执行）
unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY
```

或者在每个 curl 命令前加上 `--noproxy '*'` 参数。

### 1. 健康检查

```bash
curl http://localhost:28080/health
# 预期输出: {"status":"ok","service":"cluster-proxy"}
```

### 2. KV 操作测试

#### Set（设置键值）

```bash
curl -X POST http://localhost:28080/v1/kv/set \
  -H "Content-Type: application/json" \
  -d '{"key":"test-key","value":"test-value"}'
# 预期输出: {"success":true}
```

#### Get（获取值）

```bash
curl http://localhost:28080/v1/kv/get/test-key
# 预期输出: {"value":"test-value","exists":true}
```

#### Exists（检查键是否存在）

```bash
curl http://localhost:28080/v1/kv/exists/test-key
# 预期输出: {"exists":true}
```

#### Del（删除键）

```bash
curl -X DELETE http://localhost:28080/v1/kv/del/test-key
# 预期输出: {"success":true,"deletedCount":1}
```

### 3. Hash 操作测试

#### HMSet（设置哈希字段）

```bash
# 设置单个字段
curl -X POST http://localhost:28080/v1/hash/mset \
  -H "Content-Type: application/json" \
  -d '{"key":"user:1001","field":"name","value":"Alice"}'
# 预期输出: {"success":true,"created":false}
```

```bash
# 再设置一个字段
curl -X POST http://localhost:28080/v1/hash/mset \
  -H "Content-Type: application/json" \
  -d '{"key":"user:1001","field":"age","value":"25"}'
```

#### HMGet（获取单个字段）

```bash
curl http://localhost:28080/v1/hash/mget/user:1001/name
# 预期输出: {"value":"Alice","exists":true}
```

#### HGetAll（获取所有字段）

```bash
curl http://localhost:28080/v1/hash/getall/user:1001
# 预期输出: {"fields":{"age":"25","name":"Alice"}}
```

#### HLen（获取字段数量）

```bash
curl http://localhost:28080/v1/hash/len/user:1001
# 预期输出: {"length":2}
```

### 4. Set 操作测试

#### SAdd（添加成员）

```bash
curl -X POST http://localhost:28080/v1/set/add \
  -H "Content-Type: application/json" \
  -d '{"key":"tags","member":"golang"}'
# 预期输出: {"success":true,"added":false}
```

```bash
curl -X POST http://localhost:28080/v1/set/add \
  -H "Content-Type: application/json" \
  -d '{"key":"tags","member":"redis"}'
```

#### SMembers（获取所有成员）

```bash
curl http://localhost:28080/v1/set/members/tags
# 预期输出: {"members":["golang","redis"]}
```

#### SIsMember（检查成员是否存在）

```bash
curl http://localhost:28080/v1/set/ismember/tags/golang
# 预期输出: {"isMember":true}
```

#### SCard（获取成员数量）

```bash
curl http://localhost:28080/v1/set/card/tags
# 预期输出: {"length":2}
```

### 5. List 操作测试

#### LPush（从左侧推入）

```bash
curl -X POST http://localhost:28080/v1/list/push \
  -H "Content-Type: application/json" \
  -d '{"key":"queue","value":"task1"}'
# 预期输出: {"success":true,"length":1}
```

#### RPush（从右侧推入）

```bash
curl -X POST http://localhost:28080/v1/list/rpush \
  -H "Content-Type: application/json" \
  -d '{"key":"queue","value":"task2"}'
# 预期输出: {"success":true,"length":2}
```

#### LRange（获取范围内的元素）

```bash
curl "http://localhost:28080/v1/list/range/queue?start=0&stop=-1"
# 预期输出: {"values":["task1","task2"]}
```

#### LLen（获取列表长度）

```bash
curl http://localhost:28080/v1/list/len/queue
# 预期输出: {"length":2}
```

### 6. 路由一致性测试

测试同一个键总是路由到同一个节点：

```bash
# 设置多个键
for i in {1..5}; do
  curl -X POST http://localhost:28080/v1/kv/set \
    -H "Content-Type: application/json" \
    -d "{\"key\":\"routing:test$i\",\"value\":\"value$i\"}"
done

# 验证所有键都能获取到
for i in {1..5}; do
  curl http://localhost:28080/v1/kv/get/routing:test$i
done
```

### 7. 查看日志

在测试过程中，可以查看日志了解请求路由情况：

```bash
# 查看 Master 日志（可以看到请求路由到哪个节点）
tail -f logs/master.log

# 查看特定 Slave 节点日志
tail -f logs/node1.log
tail -f logs/node2.log
tail -f logs/node3.log
```

日志中会显示：
- 键路由到哪个节点
- 节点健康检查状态
- 请求处理详情

## 停止集群

测试完成后，停止集群：

```bash
bash scripts/stop.sh
```

## 自动化测试

如果不想手动执行每个测试，可以运行自动化测试脚本：

```bash
bash scripts/test.sh
```

这将自动执行所有测试用例并显示测试结果。

## 故障排查

### 服务无法启动

1. 检查端口是否被占用：
   ```bash
   lsof -i :25000  # Master gRPC
   lsof -i :28080  # Master HTTP
   lsof -i :25001  # Slave-1 gRPC
   lsof -i :28091  # Slave-1 HTTP
   # 以此类推...
   ```

2. 检查共享库是否存在：
   ```bash
   ls -la /home/yoitsuholo/Code/Relax/build/cache-Interface/
   ```

3. 查看日志错误信息：
   ```bash
   cat logs/master.log
   cat logs/node1.log
   ```

### curl 返回 503 Service Unavailable

1. 检查是否设置了 HTTP 代理：
   ```bash
   echo $http_proxy
   echo $https_proxy
   ```
   如果有输出，执行 `unset http_proxy https_proxy HTTP_PROXY HTTPS_PROXY`

2. 检查服务是否真正在运行：
   ```bash
   curl -v http://localhost:28080/health
   ```

### curl 返回 404 Not Found

检查 API 路径是否正确。常见错误：
- ❌ `/v1/hash/set` （不存在）
- ✅ `/v1/hash/mset` （正确）

参考本文档中的 API 路径示例。

## API 端点总结

### KV 操作
- `POST /v1/kv/set` - 设置键值
- `GET /v1/kv/get/{key}` - 获取值
- `DELETE /v1/kv/del/{key}` - 删除键
- `GET /v1/kv/exists/{key}` - 检查键是否存在

### Hash 操作
- `POST /v1/hash/mset` - 设置哈希字段
- `GET /v1/hash/mget/{key}/{field}` - 获取单个字段
- `GET /v1/hash/getall/{key}` - 获取所有字段
- `GET /v1/hash/len/{key}` - 获取字段数量

### Set 操作
- `POST /v1/set/add` - 添加成员
- `GET /v1/set/members/{key}` - 获取所有成员
- `GET /v1/set/ismember/{key}/{member}` - 检查成员
- `GET /v1/set/card/{key}` - 获取成员数量

### List 操作
- `POST /v1/list/push` - 从左侧推入
- `POST /v1/list/rpush` - 从右侧推入
- `GET /v1/list/range/{key}?start=0&stop=-1` - 获取范围
- `GET /v1/list/len/{key}` - 获取长度

## 进阶测试

### 测试节点故障转移

1. 停止一个 Slave 节点：
   ```bash
   kill $(cat logs/node1.pid)
   ```

2. 观察 Master 日志中的健康检查：
   ```bash
   tail -f logs/master.log
   ```

3. 尝试访问原本路由到该节点的键（可能失败或路由到其他健康节点）

### 测试负载分布

使用脚本创建大量键，观察它们如何分布到不同节点：

```bash
for i in {1..100}; do
  curl -s -X POST http://localhost:28080/v1/kv/set \
    -H "Content-Type: application/json" \
    -d "{\"key\":\"loadtest:$i\",\"value\":\"value$i\"}"
done
```

查看 Master 日志，可以看到键分布到不同节点的情况。
