#define DEBUG_MODE
#include "../../cache-Kernel/buddySystem/buddy_allocator.h"

int main() {
    #ifdef DEBUG_MODE
    #warning "DEBUG_MODE is defined"
    #endif
    
    BuddyAllocator buddy;
    // 尝试直接访问成员来测试访问权限
    return 0;
}
