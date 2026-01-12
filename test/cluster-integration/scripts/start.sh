#!/bin/bash
# 集群集成测试 - 启动脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
TEST_DIR="$SCRIPT_DIR/.."
LOGS_DIR="$TEST_DIR/logs"
CONFIG_DIR="$TEST_DIR/configs"
CLUSTER_CONFIG="$CONFIG_DIR/cluster.yaml"

# 设置库路径
export LD_LIBRARY_PATH="$PROJECT_ROOT/build/cache-Interface:$LD_LIBRARY_PATH"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== 集群集成测试启动 ===${NC}"

# 创建日志目录
mkdir -p "$LOGS_DIR"

# 清理旧的日志
rm -f "$LOGS_DIR"/*.log "$LOGS_DIR"/*.pid

# 检查配置文件
if [ ! -f "$CLUSTER_CONFIG" ]; then
    echo -e "${RED}错误: 集群配置文件不存在: $CLUSTER_CONFIG${NC}"
    exit 1
fi

echo -e "${YELLOW}配置文件: $CLUSTER_CONFIG${NC}"

# 启动多个 Slave 节点
echo -e "\n${YELLOW}1. 启动 Slave 节点 (后端缓存节点)...${NC}"

SINGLE_NODE_DIR="$PROJECT_ROOT/inf-SlaveNode"

if [ ! -d "$SINGLE_NODE_DIR" ]; then
    echo -e "${RED}错误: SingleNode 目录不存在: $SINGLE_NODE_DIR${NC}"
    exit 1
fi

cd "$SINGLE_NODE_DIR"

# 启动 node-1 (端口 25001:28091)
echo -e "  启动 slave node-1 (gRPC: localhost:25001, HTTP: localhost:28091)..."
go run main.go -config="$CONFIG_DIR/node1.yaml" > "$LOGS_DIR/node1.log" 2>&1 &
NODE1_PID=$!
echo $NODE1_PID > "$LOGS_DIR/node1.pid"
echo -e "  ${GREEN}✓${NC} slave node-1 启动 (PID: $NODE1_PID)"

# 启动 node-2 (端口 25002:28092)
echo -e "  启动 slave node-2 (gRPC: localhost:25002, HTTP: localhost:28092)..."
go run main.go -config="$CONFIG_DIR/node2.yaml" > "$LOGS_DIR/node2.log" 2>&1 &
NODE2_PID=$!
echo $NODE2_PID > "$LOGS_DIR/node2.pid"
echo -e "  ${GREEN}✓${NC} slave node-2 启动 (PID: $NODE2_PID)"

# 启动 node-3 (端口 25003:28093)
echo -e "  启动 slave node-3 (gRPC: localhost:25003, HTTP: localhost:28093)..."
go run main.go -config="$CONFIG_DIR/node3.yaml" > "$LOGS_DIR/node3.log" 2>&1 &
NODE3_PID=$!
echo $NODE3_PID > "$LOGS_DIR/node3.pid"
echo -e "  ${GREEN}✓${NC} slave node-3 启动 (PID: $NODE3_PID)"

# 等待节点启动
sleep 3

# 启动 Master 节点（集群代理）
echo -e "\n${YELLOW}2. 启动 Master 节点 (集群代理)...${NC}"

CLUSTER_NODE_DIR="$PROJECT_ROOT/inf-MasterNode"

if [ ! -d "$CLUSTER_NODE_DIR" ]; then
    echo -e "${RED}错误: ClusterNode 目录不存在: $CLUSTER_NODE_DIR${NC}"
    kill $NODE1_PID $NODE2_PID $NODE3_PID 2>/dev/null || true
    exit 1
fi

cd "$CLUSTER_NODE_DIR"
go run main.go -config="$CLUSTER_CONFIG" > "$LOGS_DIR/master.log" 2>&1 &
MASTER_PID=$!
echo $MASTER_PID > "$LOGS_DIR/master.pid"
echo -e "  ${GREEN}✓${NC} master 节点启动 (PID: $MASTER_PID)"

# 等待服务启动
echo -e "\n${YELLOW}等待服务启动...${NC}"
sleep 3

# 健康检查
echo -e "\n${YELLOW}3. 健康检查...${NC}"
MAX_RETRIES=10
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if curl -s http://localhost:28080/health > /dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} 集群代理健康检查通过"
        break
    fi
    RETRY_COUNT=$((RETRY_COUNT + 1))
    if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
        echo -e "  ${RED}✗${NC} 健康检查失败，请查看日志"
        echo -e "\n${YELLOW}日志位置:${NC}"
        echo -e "  - node-1: $LOGS_DIR/node1.log"
        echo -e "  - cluster: $LOGS_DIR/cluster.log"
        exit 1
    fi
    sleep 1
done

# 显示运行信息
echo -e "\n${GREEN}=== 集群启动成功 ===${NC}"
echo -e "\n${YELLOW}服务信息:${NC}"
echo -e "  ${GREEN}Slave 节点 (后端缓存):${NC}"
echo -e "    - slave node-1: gRPC localhost:25001, HTTP localhost:28091 (PID: $NODE1_PID)"
echo -e "    - slave node-2: gRPC localhost:25002, HTTP localhost:28092 (PID: $NODE2_PID)"
echo -e "    - slave node-3: gRPC localhost:25003, HTTP localhost:28093 (PID: $NODE3_PID)"
echo -e "  ${GREEN}Master 节点 (集群代理):${NC}"
echo -e "    - master: gRPC localhost:25000, HTTP localhost:28080 (PID: $MASTER_PID)"
echo -e "    - 健康检查: http://localhost:28080/health"

echo -e "\n${YELLOW}日志位置:${NC}"
echo -e "  - slave node-1: $LOGS_DIR/node1.log"
echo -e "  - slave node-2: $LOGS_DIR/node2.log"
echo -e "  - slave node-3: $LOGS_DIR/node3.log"
echo -e "  - master: $LOGS_DIR/master.log"

echo -e "\n${YELLOW}PID 文件:${NC}"
echo -e "  - $LOGS_DIR/node1.pid"
echo -e "  - $LOGS_DIR/node2.pid"
echo -e "  - $LOGS_DIR/node3.pid"
echo -e "  - $LOGS_DIR/master.pid"

echo -e "\n${YELLOW}停止服务:${NC}"
echo -e "  bash $SCRIPT_DIR/stop.sh"

echo -e "\n${YELLOW}运行测试:${NC}"
echo -e "  bash $SCRIPT_DIR/test.sh"

echo -e "\n${GREEN}集群已就绪，可以开始测试${NC}"
