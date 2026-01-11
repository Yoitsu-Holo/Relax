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

# 停止 Master 节点（集群代理）
if [ -f "$LOGS_DIR/master.pid" ]; then
    MASTER_PID=$(cat "$LOGS_DIR/master.pid")
    if ps -p $MASTER_PID > /dev/null 2>&1; then
        echo -e "  停止 master 节点 (PID: $MASTER_PID)..."
        kill $MASTER_PID 2>/dev/null || true
        sleep 1
        if ps -p $MASTER_PID > /dev/null 2>&1; then
            kill -9 $MASTER_PID 2>/dev/null || true
        fi
        echo -e "  ${GREEN}✓${NC} master 节点已停止"
    else
        echo -e "  ${YELLOW}!${NC} master 节点未运行"
    fi
    rm -f "$LOGS_DIR/master.pid"
else
    echo -e "  ${YELLOW}!${NC} 未找到 master 节点 PID 文件"
fi

# 停止 Slave 节点
for i in {1..3}; do
    if [ -f "$LOGS_DIR/node$i.pid" ]; then
        NODE_PID=$(cat "$LOGS_DIR/node$i.pid")
        if ps -p $NODE_PID > /dev/null 2>&1; then
            echo -e "  停止 slave node-$i (PID: $NODE_PID)..."
            kill $NODE_PID 2>/dev/null || true
            sleep 1
            if ps -p $NODE_PID > /dev/null 2>&1; then
                kill -9 $NODE_PID 2>/dev/null || true
            fi
            echo -e "  ${GREEN}✓${NC} slave node-$i 已停止"
        else
            echo -e "  ${YELLOW}!${NC} slave node-$i 未运行"
        fi
        rm -f "$LOGS_DIR/node$i.pid"
    else
        echo -e "  ${YELLOW}!${NC} 未找到 slave node-$i PID 文件"
    fi
done

echo -e "\n${GREEN}所有服务已停止${NC}"

# 询问是否清理日志
read -p "是否清理日志文件? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f "$LOGS_DIR"/*.log
    echo -e "${GREEN}✓ 日志已清理${NC}"
fi
