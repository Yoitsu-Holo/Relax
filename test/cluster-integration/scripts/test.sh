#!/bin/bash
# 集群集成测试 - 测试脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_URL="http://localhost:8081"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

TEST_PASSED=0
TEST_FAILED=0

# 测试辅助函数
test_api() {
    local method=$1
    local path=$2
    local data=$3
    local expected_status=$4
    local test_name=$5

    echo -e "\n${BLUE}[TEST]${NC} $test_name"
    echo -e "  ${YELLOW}→${NC} $method $path"

    if [ -n "$data" ]; then
        response=$(curl -s -w "\n%{http_code}" -X $method "$BASE_URL$path" \
            -H "Content-Type: application/json" \
            -d "$data")
    else
        response=$(curl -s -w "\n%{http_code}" -X $method "$BASE_URL$path")
    fi

    status_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | sed '$d')

    if [ "$status_code" -eq "$expected_status" ]; then
        echo -e "  ${GREEN}✓${NC} Status: $status_code"
        echo -e "  ${GREEN}✓${NC} Response: $body"
        TEST_PASSED=$((TEST_PASSED + 1))
        return 0
    else
        echo -e "  ${RED}✗${NC} Status: $status_code (expected $expected_status)"
        echo -e "  ${RED}✗${NC} Response: $body"
        TEST_FAILED=$((TEST_FAILED + 1))
        return 1
    fi
}

echo -e "${GREEN}=== 集群集成测试 ===${NC}"

# 检查服务是否运行
echo -e "\n${YELLOW}0. 检查服务状态...${NC}"
if ! curl -s "$BASE_URL/health" > /dev/null; then
    echo -e "${RED}错误: 集群服务未运行${NC}"
    echo -e "${YELLOW}请先运行: bash $SCRIPT_DIR/start.sh${NC}"
    exit 1
fi
echo -e "${GREEN}✓ 服务正在运行${NC}"

# 1. 健康检查
echo -e "\n${YELLOW}=== 1. 健康检查 ===${NC}"
test_api "GET" "/health" "" 200 "Health check"

# 2. KV 操作测试
echo -e "\n${YELLOW}=== 2. KV 操作测试 ===${NC}"
test_api "POST" "/v1/kv/set" '{"key":"test:key1","value":"value1"}' 200 "Set key1"
test_api "GET" "/v1/kv/get/test:key1" "" 200 "Get key1"
test_api "GET" "/v1/kv/exists/test:key1" "" 200 "Check key1 exists"
test_api "DELETE" "/v1/kv/del/test:key1" "" 200 "Delete key1"

# 3. Hash 操作测试
echo -e "\n${YELLOW}=== 3. Hash 操作测试 ===${NC}"
test_api "POST" "/v1/hash/set" '{"key":"user:1001","field":"name","value":"Alice"}' 200 "HSet name"
test_api "GET" "/v1/hash/get/user:1001/name" "" 200 "HGet name"
test_api "POST" "/v1/hash/mset" '{"key":"user:1001","fields":{"age":"25","city":"Beijing"}}' 200 "HMSet multiple fields"
test_api "GET" "/v1/hash/getall/user:1001" "" 200 "HGetAll"
test_api "GET" "/v1/hash/len/user:1001" "" 200 "HLen"

# 4. Set 操作测试
echo -e "\n${YELLOW}=== 4. Set 操作测试 ===${NC}"
test_api "POST" "/v1/set/add" '{"key":"tags:golang","member":"backend"}' 200 "SAdd member1"
test_api "POST" "/v1/set/add" '{"key":"tags:golang","member":"distributed"}' 200 "SAdd member2"
test_api "GET" "/v1/set/members/tags:golang" "" 200 "SMembers"
test_api "GET" "/v1/set/ismember/tags:golang/backend" "" 200 "SIsMember"
test_api "GET" "/v1/set/card/tags:golang" "" 200 "SCard"

# 5. List 操作测试
echo -e "\n${YELLOW}=== 5. List 操作测试 ===${NC}"
test_api "POST" "/v1/list/push" '{"key":"queue:tasks","value":"task1"}' 200 "LPush task1"
test_api "POST" "/v1/list/rpush" '{"key":"queue:tasks","value":"task2"}' 200 "RPush task2"
test_api "GET" "/v1/list/range/queue:tasks?start=0&stop=-1" "" 200 "LRange all"
test_api "GET" "/v1/list/len/queue:tasks" "" 200 "LLen"

# 6. 路由验证测试
echo -e "\n${YELLOW}=== 6. 路由一致性测试 ===${NC}"
for i in {1..3}; do
    test_api "POST" "/v1/kv/set" "{\"key\":\"route:test$i\",\"value\":\"value$i\"}" 200 "Set route:test$i (attempt $i)"
done

for i in {1..3}; do
    test_api "GET" "/v1/kv/get/route:test$i" "" 200 "Get route:test$i (verify routing)"
done

# 7. 并发测试
echo -e "\n${YELLOW}=== 7. 并发写入测试 ===${NC}"
echo -e "${BLUE}[TEST]${NC} Concurrent writes (10 keys)"

concurrent_test() {
    local key=$1
    local value=$2
    curl -s -X POST "$BASE_URL/v1/kv/set" \
        -H "Content-Type: application/json" \
        -d "{\"key\":\"$key\",\"value\":\"$value\"}" > /dev/null 2>&1
}

for i in {1..10}; do
    concurrent_test "concurrent:key$i" "value$i" &
done

wait

# 验证并发写入
ALL_SUCCESS=true
for i in {1..10}; do
    response=$(curl -s "$BASE_URL/v1/kv/get/concurrent:key$i")
    if ! echo "$response" | grep -q "value$i"; then
        ALL_SUCCESS=false
        echo -e "  ${RED}✗${NC} Key concurrent:key$i verification failed"
        break
    fi
done

if [ "$ALL_SUCCESS" = true ]; then
    echo -e "  ${GREEN}✓${NC} All 10 concurrent writes verified"
    TEST_PASSED=$((TEST_PASSED + 1))
else
    echo -e "  ${RED}✗${NC} Some concurrent writes failed"
    TEST_FAILED=$((TEST_FAILED + 1))
fi

# 测试总结
echo -e "\n${GREEN}=== 测试总结 ===${NC}"
echo -e "  ${GREEN}通过:${NC} $TEST_PASSED"
echo -e "  ${RED}失败:${NC} $TEST_FAILED"
echo -e "  ${BLUE}总计:${NC} $((TEST_PASSED + TEST_FAILED))"

if [ $TEST_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✓ 所有测试通过!${NC}"
    exit 0
else
    echo -e "\n${RED}✗ 部分测试失败${NC}"
    exit 1
fi
