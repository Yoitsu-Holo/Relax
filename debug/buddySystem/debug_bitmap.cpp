#define DEBUG_MODE
#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <iostream>
#include <iomanip>
#include <bitset>

// 使用继承方式访问私有成员（如果 BUDDY_DEBUG_MODE 改变了访问权限）
class BuddyAllocatorDebugAccess : public BuddyAllocator
{
public:
    void print_bitmap_state(int order)
    {
        std::cout << "\nOrder " << order << " bitmap state:\n";
        std::cout << "level0: " << std::bitset<64>(free_bitmap_[order].level0) << "\n";
        std::cout << "  (~level0): " << std::bitset<64>(~free_bitmap_[order].level0) << " (available groups)\n";

        for (int i = 0; i < 4; ++i)
        {
            std::cout << "level1[" << i << "]: " << std::bitset<64>(free_bitmap_[order].level1[i]) << "\n";
        }
    }
};

int main()
{
    void *mem = std::aligned_alloc(alignof(BuddyAllocator), sizeof(BuddyAllocator));
    BuddyAllocatorDebugAccess *buddy = new (mem) BuddyAllocatorDebugAccess();

    std::cout << "=== After initialization ===\n";

    std::cout << "\n=== Order 0 (4KiB) ===";
    buddy->print_bitmap_state(0);

    std::cout << "\n=== Order 6 (256KiB) ===";
    buddy->print_bitmap_state(6);

    std::cout << "\n=== Statistics ===\n";
    std::cout << "Order 6 free blocks: " << buddy->get_free_blocks(6) << " (expected: 64)\n";
    std::cout << "Order 5 free blocks: " << buddy->get_free_blocks(5) << "\n";
    std::cout << "Order 0 free blocks: " << buddy->get_free_blocks(0) << " (expected: 4096)\n";
    std::cout << "Total free memory: " << buddy->get_total_free_memory() << " (expected: 16777216)\n";

    buddy->~BuddyAllocatorDebugAccess();
    std::free(mem);
    return 0;
}
