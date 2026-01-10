#!/bin/bash
# 集群集成测试 - 启动脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
TEST_DIR="$SCRIPT_DIR/.."
LOGS_DIR="$TEST_DIR/logs"
CONFIG_FILE="$TEST_DIR/configs/cluster.yaml"

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
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${RED}错误: 配置文件不存在: $CONFIG_FILE${NC}"
    exit 1
fi

echo -e "${YELLOW}配置文件: $CONFIG_FILE${NC}"

# 启动单节点服务器
echo -e "\n${YELLOW}1. 启动后端节点...${NC}"

SINGLE_NODE_DIR="$PROJECT_ROOT/inf-SingleNode/server"

if [ ! -d "$SINGLE_NODE_DIR" ]; then
    echo -e "${RED}错误: SingleNode 目录不存在: $SINGLE_NODE_DIR${NC}"
    exit 1
fi

# 启动 node-1 (端口 50051)
echo -e "  启动 node-1 (localhost:50051)..."
cd "$SINGLE_NODE_DIR"
PORT=50051 go run main.go > "$LOGS_DIR/node1.log" 2>&1 &
NODE1_PID=$!
echo $NODE1_PID > "$LOGS_DIR/node1.pid"
echo -e "  ${GREEN}✓${NC} node-1 启动 (PID: $NODE1_PID)"

# 等待节点启动
sleep 2

# 启动 node-2 (端口 50053) - 需要修改 SingleNode 支持端口配置
# 暂时说明：SingleNode 需要支持 PORT 环境变量或命令行参数
echo -e "  ${YELLOW}注意: 当前 SingleNode 不支持多实例，仅启动 node-1${NC}"
echo -e "  ${YELLOW}如需测试多节点，请手动启动额外的 SingleNode 实例${NC}"

# 启动集群代理
echo -e "\n${YELLOW}2. 启动集群代理...${NC}"

CLUSTER_NODE_DIR="$PROJECT_ROOT/inf-ClusterNode"

if [ ! -d "$CLUSTER_NODE_DIR" ]; then
    echo -e "${RED}错误: ClusterNode 目录不存在: $CLUSTER_NODE_DIR${NC}"
    kill $NODE1_PID 2>/dev/null || true
    exit 1
fi

cd "$CLUSTER_NODE_DIR"
go run main.go -config="$CONFIG_FILE" > "$LOGS_DIR/cluster.log" 2>&1 &
CLUSTER_PID=$!
echo $CLUSTER_PID > "$LOGS_DIR/cluster.pid"
echo -e "  ${GREEN}✓${NC} 集群代理启动 (PID: $CLUSTER_PID)"

# 等待服务启动
echo -e "\n${YELLOW}等待服务启动...${NC}"
sleep 3

# 健康检查
echo -e "\n${YELLOW}3. 健康检查...${NC}"
MAX_RETRIES=10
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if curl -s http://localhost:8081/health > /dev/null 2>&1; then
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
echo -e "  - 后端节点: localhost:50051 (PID: $NODE1_PID)"
echo -e "  - 集群代理 gRPC: localhost:50052 (PID: $CLUSTER_PID)"
echo -e "  - 集群代理 HTTP: localhost:8081"
echo -e "  - 健康检查: http://localhost:8081/health"

echo -e "\n${YELLOW}日志位置:${NC}"
echo -e "  - node-1: $LOGS_DIR/node1.log"
echo -e "  - cluster: $LOGS_DIR/cluster.log"

echo -e "\n${YELLOW}PID 文件:${NC}"
echo -e "  - $LOGS_DIR/node1.pid"
echo -e "  - $LOGS_DIR/cluster.pid"

echo -e "\n${YELLOW}停止服务:${NC}"
echo -e "  bash $SCRIPT_DIR/stop.sh"

echo -e "\n${YELLOW}运行测试:${NC}"
echo -e "  bash $SCRIPT_DIR/test.sh"

echo -e "\n${GREEN}集群已就绪，可以开始测试${NC}"
