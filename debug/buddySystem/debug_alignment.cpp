#include "../../cache-Kernel/buddySystem/buddy_allocator.h"
#include <iostream>
#include <iomanip>

int main()
{
    void *mem = std::aligned_alloc(16 * MiB, sizeof(BuddyAllocator));
    BuddyAllocator *buddy = new (mem) BuddyAllocator();

    std::cout << "=== Checking alignment ===\n";

    for (int i = 0; i < 5; ++i)
    {
        void *ptr = buddy->allocate(0);
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t offset = addr % 4096;
        std::cout << "allocate(0) #" << i << ": " << ptr
                  << " offset=" << offset
                  << (offset == 0 ? " ✓" : " ✗") << "\n";

        if (ptr)
            buddy->deallocate(ptr);
    }

    buddy->~BuddyAllocator();
    std::free(mem);
    return 0;
}
