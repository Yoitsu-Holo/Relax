#include <iostream>
#include <chrono>
#include <cstdint>
#include <utility>
#include <random>
#include <vector>
#include "../../lib/ankerl_unordered_dense/unordered_dense.h"

int main()
{
    // 定义测试参数
    const uint64_t PRELOAD_COUNT = 500000;               // 预加载数据量：50万
    const uint64_t TEST_ITERATIONS = 100000000ULL;       // 测试操作数：1亿次
    const uint64_t RANDOM_POOL_SIZE = PRELOAD_COUNT * 2; // 随机数池大小

    // ==================== 预生成随机数 ====================
    std::cout << "Generating random numbers pool..." << std::endl;

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    // 预生成随机键池
    std::vector<uint64_t> random_keys;
    random_keys.reserve(RANDOM_POOL_SIZE);
    for (uint64_t i = 0; i < RANDOM_POOL_SIZE; ++i)
    {
        random_keys.push_back(dist(gen));
    }

    std::cout << "Random pool generated: " << random_keys.size() << " keys" << std::endl;

    // 使用 ankerl::unordered_dense::map<uint64_t, uint64_t>
    ankerl::unordered_dense::map<uint64_t, uint64_t> test_map;

    // 预留空间以减少rehash
    test_map.reserve(PRELOAD_COUNT * 2);

    // ==================== 预加载阶段 ====================
    std::cout << "Starting preload of " << PRELOAD_COUNT << " entries with random keys..." << std::endl;
    std::cout << "Initial bucket count: " << test_map.bucket_count() << std::endl;

    auto preload_start = std::chrono::high_resolution_clock::now();

    // 预加载 50万条数据，使用随机键
    for (uint64_t i = 0; i < PRELOAD_COUNT; ++i)
    {
        uint64_t key = random_keys[i];
        test_map[key] = key; // value 使用 key 的值
    }

    auto preload_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> preload_duration = preload_end - preload_start;

    std::cout << "Preload completed! Time: " << preload_duration.count() << " seconds" << std::endl;
    std::cout << "Current map size: " << test_map.size() << std::endl;
    std::cout << "Bucket count after preload: " << test_map.bucket_count() << std::endl;
    std::cout << "Load factor: " << test_map.load_factor() << std::endl;
    std::cout << std::endl;

    // ==================== 性能测试阶段 ====================
    std::cout << "Starting performance test with random keys, executing " << TEST_ITERATIONS << " set+get operations..." << std::endl;

    auto test_start = std::chrono::high_resolution_clock::now();

    // 执行 10亿次 set+get 混合操作
    for (uint64_t i = 0; i < TEST_ITERATIONS; ++i)
    {
        // 使用预生成的随机键，循环使用随机数池
        uint64_t key = random_keys[i % RANDOM_POOL_SIZE];

        // 50% set 操作，50% get 操作
        if (i % 2 == 0)
        {
            // set 操作
            test_map[key] = key;
        }
        else
        {
            // get 操作
            auto it = test_map.find(key);
            // 防止编译器优化掉 get 操作
            if (it != test_map.end())
            {
                volatile uint64_t dummy = it->second;
                (void)dummy;
            }
        }
    }

    auto test_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> test_duration = test_end - test_start;

    // ==================== 结果统计 ====================
    double total_time_seconds = test_duration.count();
    double ops = TEST_ITERATIONS / total_time_seconds;

    std::cout << "\n========== Test Results ==========\n";
    std::cout << "Library: ankerl::unordered_dense::map (RANDOM KEYS)\n";
    std::cout << "Key type: 64-bit random integers\n";
    std::cout << "Random pool size: " << RANDOM_POOL_SIZE << " keys\n";
    std::cout << "Preload count: " << PRELOAD_COUNT << " entries\n";
    std::cout << "Total test operations: " << TEST_ITERATIONS << " operations\n";
    std::cout << "Preload time: " << preload_duration.count() << " seconds\n";
    std::cout << "Performance test total time: " << total_time_seconds << " seconds\n";
    std::cout << "Average performance: " << static_cast<uint64_t>(ops) << " ops/s\n";
    std::cout << "Final map size: " << test_map.size() << "\n";
    std::cout << "Final bucket count: " << test_map.bucket_count() << "\n";
    std::cout << "Final load factor: " << test_map.load_factor() << "\n";
    std::cout << "==================================\n";

    return 0;
}