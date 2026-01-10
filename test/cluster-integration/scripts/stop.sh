#!/bin/bash
# 集群集成测试 - 停止脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/.."
LOGS_DIR="$TEST_DIR/logs"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== 停止集群服务 ===${NC}"

# 停止集群代理
if [ -f "$LOGS_DIR/cluster.pid" ]; then
    CLUSTER_PID=$(cat "$LOGS_DIR/cluster.pid")
    if ps -p $CLUSTER_PID > /dev/null 2>&1; then
        echo -e "  停止集群代理 (PID: $CLUSTER_PID)..."
        kill $CLUSTER_PID 2>/dev/null || true
        sleep 1
        if ps -p $CLUSTER_PID > /dev/null 2>&1; then
            kill -9 $CLUSTER_PID 2>/dev/null || true
        fi
        echo -e "  ${GREEN}✓${NC} 集群代理已停止"
    else
        echo -e "  ${YELLOW}!${NC} 集群代理未运行"
    fi
    rm -f "$LOGS_DIR/cluster.pid"
else
    echo -e "  ${YELLOW}!${NC} 未找到集群代理 PID 文件"
fi

# 停止后端节点
if [ -f "$LOGS_DIR/node1.pid" ]; then
    NODE1_PID=$(cat "$LOGS_DIR/node1.pid")
    if ps -p $NODE1_PID > /dev/null 2>&1; then
        echo -e "  停止 node-1 (PID: $NODE1_PID)..."
        kill $NODE1_PID 2>/dev/null || true
        sleep 1
        if ps -p $NODE1_PID > /dev/null 2>&1; then
            kill -9 $NODE1_PID 2>/dev/null || true
        fi
        echo -e "  ${GREEN}✓${NC} node-1 已停止"
    else
        echo -e "  ${YELLOW}!${NC} node-1 未运行"
    fi
    rm -f "$LOGS_DIR/node1.pid"
else
    echo -e "  ${YELLOW}!${NC} 未找到 node-1 PID 文件"
fi

echo -e "\n${GREEN}所有服务已停止${NC}"

# 询问是否清理日志
read -p "是否清理日志文件? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f "$LOGS_DIR"/*.log
    echo -e "${GREEN}✓ 日志已清理${NC}"
fi
