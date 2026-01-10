#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <iostream>
#include <iomanip>

int main() {
    void *mem = std::aligned_alloc(alignof(BuddyAllocator), sizeof(BuddyAllocator));
    BuddyAllocator *buddy = new (mem) BuddyAllocator();

    std::cout << "After initialization:\n";
    std::cout << "Order 6 free blocks: " << buddy->get_free_blocks(6) << "\n";
    std::cout << "Order 0 free blocks: " << buddy->get_free_blocks(0) << "\n";
    std::cout << "Total free memory: " << buddy->get_total_free_memory() << "\n";
    std::cout << "Expected: 16777216\n";

    std::cout << "\nTrying to allocate order 6 block...\n";
    void *ptr = buddy->allocate(6);
    if (ptr) {
        std::cout << "Success! ptr = " << ptr << "\n";
    } else {
        std::cout << "Failed!\n";
    }

    std::cout << "\nTrying to allocate order 0 block...\n";
    ptr = buddy->allocate(0);
    if (ptr) {
        std::cout << "Success! ptr = " << ptr << "\n";
    } else {
        std::cout << "Failed!\n";
    }

    buddy->~BuddyAllocator();
    std::free(mem);
    return 0;
}
