#include <iostream>
#include <chrono>
#include <cstdint>
#include <utility>
#include <random>
#include "../../lib/ankerl_unordered_dense/unordered_dense.h"

int main()
{
    // 定义测试参数
    const uint64_t PRELOAD_COUNT = 1000000;        // 预加载数据量：100万（更多数据）
    const uint64_t TEST_ITERATIONS = 100000000ULL; // 测试操作数：1亿次

    // 使用 ankerl::unordered_dense::map<uint64_t, uint64_t>
    ankerl::unordered_dense::map<uint64_t, uint64_t> test_map;

    // 预留空间
    test_map.reserve(PRELOAD_COUNT);

    // ==================== 预加载阶段 ====================
    std::cout << "Starting preload of " << PRELOAD_COUNT << " entries..." << std::endl;

    auto preload_start = std::chrono::high_resolution_clock::now();

    // 预加载数据
    for (uint64_t i = 0; i < PRELOAD_COUNT; ++i)
    {
        test_map[i] = i;
    }

    auto preload_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> preload_duration = preload_end - preload_start;

    std::cout << "Preload completed! Time: " << preload_duration.count() << " seconds" << std::endl;
    std::cout << "Current map size: " << test_map.size() << std::endl;
    std::cout << "Load factor: " << test_map.load_factor() << std::endl;
    std::cout << std::endl;

    // ==================== 纯查找性能测试 ====================
    std::cout << "Test 1: Pure LOOKUP performance (100% find operations)..." << std::endl;

    auto lookup_start = std::chrono::high_resolution_clock::now();

    // 执行 10亿次纯查找操作
    uint64_t found_count = 0;
    for (uint64_t i = 0; i < TEST_ITERATIONS; ++i)
    {
        uint64_t key = i % PRELOAD_COUNT; // 所有键都存在

        auto it = test_map.find(key);
        if (it != test_map.end())
        {
            volatile uint64_t dummy = it->second;
            (void)dummy;
            found_count++;
        }
    }

    auto lookup_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> lookup_duration = lookup_end - lookup_start;

    double lookup_time = lookup_duration.count();
    double lookup_ops = TEST_ITERATIONS / lookup_time;

    std::cout << "Pure lookup time: " << lookup_time << " seconds" << std::endl;
    std::cout << "Pure lookup performance: " << static_cast<uint64_t>(lookup_ops) << " ops/s" << std::endl;
    std::cout << "Found count: " << found_count << "/" << TEST_ITERATIONS << std::endl;
    std::cout << std::endl;

    // ==================== 读多写少测试 (90% read, 10% write) ====================
    std::cout << "Test 2: Read-heavy workload (90% read, 10% write)..." << std::endl;

    auto rw_start = std::chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < TEST_ITERATIONS; ++i)
    {
        uint64_t key = i % PRELOAD_COUNT;

        // 90% 读操作，10% 写操作
        if (i % 10 != 0) // 90% 概率
        {
            // 读操作
            auto it = test_map.find(key);
            if (it != test_map.end())
            {
                volatile uint64_t dummy = it->second;
                (void)dummy;
            }
        }
        else // 10% 概率
        {
            // 写操作 - 更新已存在的键
            test_map[key] = i;
        }
    }

    auto rw_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> rw_duration = rw_end - rw_start;

    double rw_time = rw_duration.count();
    double rw_ops = TEST_ITERATIONS / rw_time;

    std::cout << "Read-heavy time: " << rw_time << " seconds" << std::endl;
    std::cout << "Read-heavy performance: " << static_cast<uint64_t>(rw_ops) << " ops/s" << std::endl;

    // ==================== 结果总结 ====================
    std::cout << "\n========== Test Summary ==========\n";
    std::cout << "Library: ankerl::unordered_dense::map (Different Workloads)\n";
    std::cout << "Data size: " << PRELOAD_COUNT << " entries\n";
    std::cout << "Total operations per test: " << TEST_ITERATIONS << "\n";
    std::cout << "-----------------------------------\n";
    std::cout << "Pure Lookup (100% read): " << static_cast<uint64_t>(lookup_ops) << " ops/s\n";
    std::cout << "Read-Heavy (90% read, 10% write): " << static_cast<uint64_t>(rw_ops) << " ops/s\n";
    std::cout << "==================================\n";

    return 0;
}