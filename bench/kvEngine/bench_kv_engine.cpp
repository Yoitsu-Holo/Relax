#include "../../cache-Kernel/kvEngine/kv_engine.h"
#include "../../lib/ankerl_unordered_dense/unordered_dense.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <random>
#include <unordered_map>
#include <map>
#include <cstring>

int main()
{
    constexpr size_t PRELOAD_COUNT = 100000;     // 预加载10万条数据
    constexpr size_t TOTAL_OPS = 10000000;       // 总共1000万次操作
    constexpr size_t DATA_SIZE = 120;            // 数据大小120字节

    std::cout << "=== KV Engine Performance Benchmark ===\n" << std::endl;
    std::cout << "Preload count: " << PRELOAD_COUNT << std::endl;
    std::cout << "Total operations: " << TOTAL_OPS << std::endl;
    std::cout << "Data size: " << DATA_SIZE << " bytes" << std::endl;
    std::cout << "Operation mix: 50% add, 40% get, 10% del\n" << std::endl;

    // 准备固定的随机种子以保证所有测试使用相同的操作序列
    std::random_device rd;
    unsigned int seed = rd();
    std::vector<char> data_buffer(DATA_SIZE, 'A');

    // ========== 1. KV Engine 测试 ==========
    std::cout << "=== 1. Testing KV Engine (with ralloc) ===\n" << std::endl;

    KVEngine kv_engine;
    if (kv_engine.init() != KV_OK)
    {
        std::cerr << "Failed to initialize KV Engine" << std::endl;
        return 1;
    }

    // 预加载数据
    std::cout << "Preloading " << PRELOAD_COUNT << " entries..." << std::endl;
    auto kv_preload_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < PRELOAD_COUNT; i++)
    {
        kv_engine.add(i, data_buffer.data(), DATA_SIZE);
    }

    auto kv_preload_end = std::chrono::high_resolution_clock::now();
    auto kv_preload_time = std::chrono::duration_cast<std::chrono::microseconds>(kv_preload_end - kv_preload_start).count();

    std::cout << "Preload completed in " << kv_preload_time / 1000.0 << " ms" << std::endl;
    std::cout << "Current size: " << kv_engine.get_size() << std::endl;
    std::cout << "Slab allocators: " << kv_engine.get_slab_count() << std::endl;
    std::cout << "Buddy allocators: " << kv_engine.get_buddy_count() << std::endl;

    // 性能测试 - 混合操作
    std::cout << "Starting mixed operations benchmark..." << std::endl;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<uint64_t> hash_dist(0, PRELOAD_COUNT * 2 - 1);
    std::uniform_int_distribution<int> op_dist(0, 99);

    auto kv_test_start = std::chrono::high_resolution_clock::now();

    size_t kv_add_count = 0, kv_get_count = 0, kv_del_count = 0;
    size_t kv_get_success = 0, kv_del_success = 0;

    for (size_t i = 0; i < TOTAL_OPS; i++)
    {
        uint64_t hash = hash_dist(gen);
        int op = op_dist(gen);

        if (op < 50)
        {
            kv_engine.add(hash, data_buffer.data(), DATA_SIZE);
            kv_add_count++;
        }
        else if (op < 90)
        {
            char *ret_data = nullptr;
            size_t ret_len = 0;
            int ret = kv_engine.get(hash, &ret_data, &ret_len);
            if (ret == KV_OK)
                kv_get_success++;
            kv_get_count++;
        }
        else
        {
            int ret = kv_engine.del(hash);
            if (ret == KV_OK)
                kv_del_success++;
            kv_del_count++;
        }
    }

    auto kv_test_end = std::chrono::high_resolution_clock::now();
    auto kv_time = std::chrono::duration_cast<std::chrono::microseconds>(kv_test_end - kv_test_start).count();

    std::cout << "Completed!" << std::endl;
    std::cout << "  Add: " << kv_add_count << ", Get: " << kv_get_count
              << " (success: " << kv_get_success << "), Del: " << kv_del_count
              << " (success: " << kv_del_success << ")" << std::endl;
    std::cout << "  Final size: " << kv_engine.get_size() << std::endl;
    std::cout << std::endl;

    // ========== 2. ankerl::unordered_dense::map 测试 ==========
    std::cout << "=== 2. Testing ankerl::unordered_dense::map ===\n" << std::endl;

    ankerl::unordered_dense::map<uint64_t, std::vector<char>> ankerl_map;

    // 预加载
    std::cout << "Preloading " << PRELOAD_COUNT << " entries..." << std::endl;
    auto ankerl_preload_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < PRELOAD_COUNT; i++)
    {
        ankerl_map[i] = std::vector<char>(DATA_SIZE, 'A');
    }

    auto ankerl_preload_end = std::chrono::high_resolution_clock::now();
    auto ankerl_preload_time = std::chrono::duration_cast<std::chrono::microseconds>(ankerl_preload_end - ankerl_preload_start).count();

    std::cout << "Preload completed in " << ankerl_preload_time / 1000.0 << " ms" << std::endl;
    std::cout << "Current size: " << ankerl_map.size() << std::endl;

    // 性能测试
    std::cout << "Starting mixed operations benchmark..." << std::endl;
    gen.seed(seed); // 使用相同的种子

    auto ankerl_test_start = std::chrono::high_resolution_clock::now();

    size_t ankerl_add_count = 0, ankerl_get_count = 0, ankerl_del_count = 0;
    size_t ankerl_get_success = 0, ankerl_del_success = 0;

    for (size_t i = 0; i < TOTAL_OPS; i++)
    {
        uint64_t hash = hash_dist(gen);
        int op = op_dist(gen);

        if (op < 50)
        {
            ankerl_map[hash] = std::vector<char>(DATA_SIZE, 'A');
            ankerl_add_count++;
        }
        else if (op < 90)
        {
            auto it = ankerl_map.find(hash);
            if (it != ankerl_map.end())
                ankerl_get_success++;
            ankerl_get_count++;
        }
        else
        {
            auto it = ankerl_map.find(hash);
            if (it != ankerl_map.end())
            {
                ankerl_map.erase(it);
                ankerl_del_success++;
            }
            ankerl_del_count++;
        }
    }

    auto ankerl_test_end = std::chrono::high_resolution_clock::now();
    auto ankerl_time = std::chrono::duration_cast<std::chrono::microseconds>(ankerl_test_end - ankerl_test_start).count();

    std::cout << "Completed!" << std::endl;
    std::cout << "  Add: " << ankerl_add_count << ", Get: " << ankerl_get_count
              << " (success: " << ankerl_get_success << "), Del: " << ankerl_del_count
              << " (success: " << ankerl_del_success << ")" << std::endl;
    std::cout << "  Final size: " << ankerl_map.size() << std::endl;
    std::cout << std::endl;

    // ========== 3. std::unordered_map 测试 ==========
    std::cout << "=== 3. Testing std::unordered_map ===\n" << std::endl;

    std::unordered_map<uint64_t, std::vector<char>> std_umap;

    // 预加载
    std::cout << "Preloading " << PRELOAD_COUNT << " entries..." << std::endl;
    auto std_umap_preload_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < PRELOAD_COUNT; i++)
    {
        std_umap[i] = std::vector<char>(DATA_SIZE, 'A');
    }

    auto std_umap_preload_end = std::chrono::high_resolution_clock::now();
    auto std_umap_preload_time = std::chrono::duration_cast<std::chrono::microseconds>(std_umap_preload_end - std_umap_preload_start).count();

    std::cout << "Preload completed in " << std_umap_preload_time / 1000.0 << " ms" << std::endl;
    std::cout << "Current size: " << std_umap.size() << std::endl;

    // 性能测试
    std::cout << "Starting mixed operations benchmark..." << std::endl;
    gen.seed(seed); // 使用相同的种子

    auto std_umap_test_start = std::chrono::high_resolution_clock::now();

    size_t std_umap_add_count = 0, std_umap_get_count = 0, std_umap_del_count = 0;
    size_t std_umap_get_success = 0, std_umap_del_success = 0;

    for (size_t i = 0; i < TOTAL_OPS; i++)
    {
        uint64_t hash = hash_dist(gen);
        int op = op_dist(gen);

        if (op < 50)
        {
            std_umap[hash] = std::vector<char>(DATA_SIZE, 'A');
            std_umap_add_count++;
        }
        else if (op < 90)
        {
            auto it = std_umap.find(hash);
            if (it != std_umap.end())
                std_umap_get_success++;
            std_umap_get_count++;
        }
        else
        {
            auto it = std_umap.find(hash);
            if (it != std_umap.end())
            {
                std_umap.erase(it);
                std_umap_del_success++;
            }
            std_umap_del_count++;
        }
    }

    auto std_umap_test_end = std::chrono::high_resolution_clock::now();
    auto std_umap_time = std::chrono::duration_cast<std::chrono::microseconds>(std_umap_test_end - std_umap_test_start).count();

    std::cout << "Completed!" << std::endl;
    std::cout << "  Add: " << std_umap_add_count << ", Get: " << std_umap_get_count
              << " (success: " << std_umap_get_success << "), Del: " << std_umap_del_count
              << " (success: " << std_umap_del_success << ")" << std::endl;
    std::cout << "  Final size: " << std_umap.size() << std::endl;
    std::cout << std::endl;

    // ========== 4. std::map 测试 ==========
    std::cout << "=== 4. Testing std::map (ordered) ===\n" << std::endl;

    std::map<uint64_t, std::vector<char>> std_map;

    // 预加载
    std::cout << "Preloading " << PRELOAD_COUNT << " entries..." << std::endl;
    auto std_map_preload_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < PRELOAD_COUNT; i++)
    {
        std_map[i] = std::vector<char>(DATA_SIZE, 'A');
    }

    auto std_map_preload_end = std::chrono::high_resolution_clock::now();
    auto std_map_preload_time = std::chrono::duration_cast<std::chrono::microseconds>(std_map_preload_end - std_map_preload_start).count();

    std::cout << "Preload completed in " << std_map_preload_time / 1000.0 << " ms" << std::endl;
    std::cout << "Current size: " << std_map.size() << std::endl;

    // 性能测试
    std::cout << "Starting mixed operations benchmark..." << std::endl;
    gen.seed(seed); // 使用相同的种子

    auto std_map_test_start = std::chrono::high_resolution_clock::now();

    size_t std_map_add_count = 0, std_map_get_count = 0, std_map_del_count = 0;
    size_t std_map_get_success = 0, std_map_del_success = 0;

    for (size_t i = 0; i < TOTAL_OPS; i++)
    {
        uint64_t hash = hash_dist(gen);
        int op = op_dist(gen);

        if (op < 50)
        {
            std_map[hash] = std::vector<char>(DATA_SIZE, 'A');
            std_map_add_count++;
        }
        else if (op < 90)
        {
            auto it = std_map.find(hash);
            if (it != std_map.end())
                std_map_get_success++;
            std_map_get_count++;
        }
        else
        {
            auto it = std_map.find(hash);
            if (it != std_map.end())
            {
                std_map.erase(it);
                std_map_del_success++;
            }
            std_map_del_count++;
        }
    }

    auto std_map_test_end = std::chrono::high_resolution_clock::now();
    auto std_map_time = std::chrono::duration_cast<std::chrono::microseconds>(std_map_test_end - std_map_test_start).count();

    std::cout << "Completed!" << std::endl;
    std::cout << "  Add: " << std_map_add_count << ", Get: " << std_map_get_count
              << " (success: " << std_map_get_success << "), Del: " << std_map_del_count
              << " (success: " << std_map_del_success << ")" << std::endl;
    std::cout << "  Final size: " << std_map.size() << std::endl;
    std::cout << std::endl;

    // ========== 结果汇总 ==========
    std::cout << "\n===============================================" << std::endl;
    std::cout << "=== Performance Summary ===" << std::endl;
    std::cout << "===============================================\n" << std::endl;

    // 表格头
    std::cout << "Implementation                  | Time (ms) | Ops/sec    | Avg (ns)" << std::endl;
    std::cout << "--------------------------------|-----------|------------|---------" << std::endl;

    // KV Engine
    std::cout << "1. KV Engine (ralloc)           | "
              << std::setw(9) << std::fixed << std::setprecision(2) << kv_time / 1000.0 << " | "
              << std::setw(10) << std::fixed << std::setprecision(0) << (TOTAL_OPS * 1000000.0) / kv_time << " | "
              << std::setw(7) << std::fixed << std::setprecision(2) << (kv_time * 1000.0) / TOTAL_OPS << std::endl;

    // ankerl dense map
    std::cout << "2. ankerl::unordered_dense::map | "
              << std::setw(9) << std::fixed << std::setprecision(2) << ankerl_time / 1000.0 << " | "
              << std::setw(10) << std::fixed << std::setprecision(0) << (TOTAL_OPS * 1000000.0) / ankerl_time << " | "
              << std::setw(7) << std::fixed << std::setprecision(2) << (ankerl_time * 1000.0) / TOTAL_OPS << std::endl;

    // std::unordered_map
    std::cout << "3. std::unordered_map           | "
              << std::setw(9) << std::fixed << std::setprecision(2) << std_umap_time / 1000.0 << " | "
              << std::setw(10) << std::fixed << std::setprecision(0) << (TOTAL_OPS * 1000000.0) / std_umap_time << " | "
              << std::setw(7) << std::fixed << std::setprecision(2) << (std_umap_time * 1000.0) / TOTAL_OPS << std::endl;

    // std::map
    std::cout << "4. std::map (ordered)           | "
              << std::setw(9) << std::fixed << std::setprecision(2) << std_map_time / 1000.0 << " | "
              << std::setw(10) << std::fixed << std::setprecision(0) << (TOTAL_OPS * 1000000.0) / std_map_time << " | "
              << std::setw(7) << std::fixed << std::setprecision(2) << (std_map_time * 1000.0) / TOTAL_OPS << std::endl;

    std::cout << "\n===============================================" << std::endl;
    std::cout << "=== Relative Performance (vs KV Engine) ===" << std::endl;
    std::cout << "===============================================\n" << std::endl;

    double kv_vs_ankerl = (double)ankerl_time / kv_time;
    double kv_vs_std_umap = (double)std_umap_time / kv_time;
    double kv_vs_std_map = (double)std_map_time / kv_time;

    std::cout << "KV Engine vs ankerl::unordered_dense::map: ";
    if (kv_vs_ankerl > 1.0)
        std::cout << std::fixed << std::setprecision(2) << kv_vs_ankerl << "x faster" << std::endl;
    else
        std::cout << std::fixed << std::setprecision(2) << (1.0 / kv_vs_ankerl) << "x slower" << std::endl;

    std::cout << "KV Engine vs std::unordered_map:           ";
    if (kv_vs_std_umap > 1.0)
        std::cout << std::fixed << std::setprecision(2) << kv_vs_std_umap << "x faster" << std::endl;
    else
        std::cout << std::fixed << std::setprecision(2) << (1.0 / kv_vs_std_umap) << "x slower" << std::endl;

    std::cout << "KV Engine vs std::map:                     ";
    if (kv_vs_std_map > 1.0)
        std::cout << std::fixed << std::setprecision(2) << kv_vs_std_map << "x faster" << std::endl;
    else
        std::cout << std::fixed << std::setprecision(2) << (1.0 / kv_vs_std_map) << "x slower" << std::endl;

    std::cout << "\n=== Memory Statistics (KV Engine) ===" << std::endl;
    std::cout << "Slab allocators:  " << kv_engine.get_slab_count() << std::endl;
    std::cout << "Buddy allocators: " << kv_engine.get_buddy_count() << std::endl;
    std::cout << "Free memory:      " << kv_engine.get_total_free_memory() << " bytes" << std::endl;

    return 0;
}
