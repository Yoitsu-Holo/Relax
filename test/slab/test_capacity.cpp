#include "../../cache-Kernel/slab/slab_allocator.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>

int main() {
    std::cout << "Slab Allocator Capacity Analysis (With Flexible Block Sizes)" << std::endl;
    std::cout << "=============================================================" << std::endl;
    std::cout << std::endl;

    // 测试一些典型的块大小
    std::vector<int> block_sizes = {
        // 小于64字节，16的倍数
        16, 32, 48,
        // 64字节及以上，64的倍数
        64, 128, 192, 256, 320, 384, 448, 512, 640, 768, 896,
        1024, 1280, 1536, 1792, 2048, 2560, 3072, 3584, 4096
    };

    std::cout << std::left << std::setw(12) << "Block Size"
              << std::setw(12) << "Max Blocks"
              << std::setw(15) << "Batch (80%)"
              << std::setw(20) << "Batch (min 1000)"
              << std::setw(15) << "Memory Usage"
              << "Efficiency" << std::endl;
    std::cout << std::string(85, '-') << std::endl;

    for (int block_size : block_sizes) {
        slab test_slab;
        SlabAllocator allocator(&test_slab);

        if (allocator.init(block_size) != 0) {
            std::cout << "Failed to init block size " << block_size << std::endl;
            continue;
        }

        int max_blocks = allocator.get_total_blocks();
        int batch_80 = static_cast<int>(max_blocks * 0.8);
        int batch_min_1000 = std::min(1000, batch_80);
        double memory_usage_percent = (double)(batch_min_1000 * block_size) / (64 * 1024) * 100;
        double efficiency = (double)(max_blocks * block_size) / (64 * 1024) * 100;

        std::cout << std::left << std::setw(12) << block_size
                  << std::setw(12) << max_blocks
                  << std::setw(15) << batch_80
                  << std::setw(20) << batch_min_1000
                  << std::fixed << std::setprecision(1)
                  << std::setw(15) << (std::to_string(static_cast<int>(memory_usage_percent)) + "%")
                  << efficiency << "%"
                  << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Notes:" << std::endl;
    std::cout << "- Each slab is 64KiB with 512 bytes metadata" << std::endl;
    std::cout << "- Maximum manageable blocks: 4032 (63 groups × 64 blocks)" << std::endl;
    std::cout << "- Block sizes < 64 must be multiples of 16" << std::endl;
    std::cout << "- Block sizes >= 64 must be multiples of 64" << std::endl;
    std::cout << "- Efficiency shows the percentage of slab memory used for actual allocation" << std::endl;

    return 0;
}