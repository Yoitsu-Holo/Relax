#include <iostream>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <utility>

int main() {
    // 定义测试参数
    const uint64_t PRELOAD_COUNT = 500000;      // 预加载数据量：50万
    const uint64_t TEST_ITERATIONS = 1000000000ULL; // 测试操作数：10亿次
    
    // 使用 unordered_map<uint64_t, uint64_t>
    std::unordered_map<uint64_t, uint64_t> test_map;
    
    // ==================== 预加载阶段 ====================
    
    std::cout << "Starting preload of " << PRELOAD_COUNT << " entries..." << std::endl;
    
    auto preload_start = std::chrono::high_resolution_clock::now();
    
    // 预加载 50万条数据
    for (uint64_t i = 0; i < PRELOAD_COUNT; ++i) {
        test_map[i] = i;  // key = (i, i), value = i
    }
    
    auto preload_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> preload_duration = preload_end - preload_start;
    
    std::cout << "Preload completed! Time: " << preload_duration.count() << " seconds" << std::endl;
    std::cout << "Current map size: " << test_map.size() << std::endl;
    std::cout << std::endl;
    
    // ==================== 性能测试阶段 ====================
    std::cout << "Starting performance test, executing " << TEST_ITERATIONS << " set+get operations..." << std::endl;
    
    auto test_start = std::chrono::high_resolution_clock::now();
    
    // 执行 10亿次 set+get 混合操作
    for (uint64_t i = 0; i < TEST_ITERATIONS; ++i) {
        uint64_t key = i % (PRELOAD_COUNT * 2); //使用循环变量取模生成 uint64 key
        
        // 50% set 操作，50% get 操作
        if (i % 2 == 0) {
            // set 操作
            test_map[key] = key; // value 使用 key 的低64位
        } else {
            // get 操作
            auto it = test_map.find(key);
            // 防止编译器优化掉 get 操作
            if (it != test_map.end()) {
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
    
    std::cout << "\n========== Test Results ==========" << std::endl;
    std::cout << "Preload count: " << PRELOAD_COUNT << " entries" << std::endl;
    std::cout << "Total test operations: " << TEST_ITERATIONS << " operations" << std::endl;
    std::cout << "Preload time: " << preload_duration.count() << " seconds" << std::endl;
    std::cout << "Performance test total time: " << total_time_seconds << " seconds" << std::endl;
    std::cout << "Average performance: " << static_cast<uint64_t>(ops) << " ops" << std::endl;
    std::cout << "Final map size: " << test_map.size() << std::endl;
    std::cout << "================================" << std::endl;
    
    return 0;
}
