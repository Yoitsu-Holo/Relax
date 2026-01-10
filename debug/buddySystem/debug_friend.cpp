#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <iostream>
#include <iomanip>
#include <bitset>
#include <cstring>

// 使用内存布局访问私有成员的辅助结构
struct BitmapAccess
{
    char padding[16 * MiB]; // data + metadata_
    uint64_t metadata;
    BuddyAllocTwoLevelBitmap free_bitmap_[7];
    uint8_t block_order_[4096];
};

void print_bitmap_state(BuddyAllocator *buddy, int order)
{
    auto *access = reinterpret_cast<BitmapAccess *>(buddy);
    auto &bitmap = access->free_bitmap_[order];

    std::cout << "\n=== Order " << order << " bitmap state ===\n";
    std::cout << "level0 (0=has free, 1=full): " << std::bitset<64>(bitmap.level0) << "\n";
    std::cout << "  (~level0 = available):     " << std::bitset<64>(~bitmap.level0) << "\n";
    for (int i = 0; i < 4; ++i)
        std::cout << "level1[" << i << "] (0=free, 1=alloc): " << std::bitset<64>(bitmap.level1[i]) << "\n";
}

int main()
{
    void *mem = std::aligned_alloc(16 * MiB, sizeof(BuddyAllocator));
    BuddyAllocator *buddy = new (mem) BuddyAllocator();

    std::cout << "=== After Initialization ===\n";

    // print_bitmap_state(buddy, 0);
    // print_bitmap_state(buddy, 1);
    // print_bitmap_state(buddy, 2);
    // print_bitmap_state(buddy, 6);

    // std::cout << "\n=== Statistics ===\n";
    // std::cout << "Order 0 free blocks: " << buddy->get_free_blocks(0) << " (expected: 4096)\n";
    // std::cout << "Order 1 free blocks: " << buddy->get_free_blocks(1) << " (expected: 2048)\n";
    // std::cout << "Order 2 free blocks: " << buddy->get_free_blocks(2) << " (expected: 1024)\n";
    // std::cout << "Order 6 free blocks: " << buddy->get_free_blocks(6) << " (expected: 64)\n";
    // std::cout << "Total free memory: " << buddy->get_total_free_memory() << " (expected: 16777216)\n";

    void *ptr;

    std::cout << "\n=== Trying allocations ===\n";
    ptr = buddy->allocate(0);
    std::cout << "allocate(0) returned: " << ptr << "\n";

    ptr = buddy->allocate(0);
    std::cout << "allocate(0) returned: " << ptr << "\n";

    ptr = buddy->allocate(0);
    std::cout << "allocate(0) returned: " << ptr << "\n";

    // print_bitmap_state(buddy, 0);
    // print_bitmap_state(buddy, 1);

    ptr = buddy->allocate(1);
    std::cout << "allocate(1) returned: " << ptr << "\n";

    // print_bitmap_state(buddy, 0);
    // print_bitmap_state(buddy, 1);

    void *ptr1 = buddy->allocate(6);
    std::cout << "allocate(6) returned: " << ptr1 << "\n";

    ptr = buddy->allocate(6);
    std::cout << "allocate(6) returned: " << ptr << "\n";

    // print_bitmap_state(buddy, 0);
    // print_bitmap_state(buddy, 1);
    // print_bitmap_state(buddy, 2);
    // print_bitmap_state(buddy, 6);

    buddy->deallocate(ptr1);
    std::cout << "deallocate(6) returned: " << ptr1 << "\n";

    print_bitmap_state(buddy, 0);
    print_bitmap_state(buddy, 1);
    print_bitmap_state(buddy, 2);
    print_bitmap_state(buddy, 6);
    ptr = buddy->allocate(6);
    std::cout << "allocate(6) returned: " << ptr << "\n";

    buddy->~BuddyAllocator();
    std::free(mem);
    return 0;
}
