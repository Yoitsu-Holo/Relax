cmake_minimum_required(VERSION 3.10)
project(RAlloc)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# 创建静态库
add_library(ralloc STATIC
    ralloc.cpp
)

# 链接依赖库
target_link_libraries(ralloc
    slab_allocator
    buddy_system
)

# 设置包含目录
target_include_directories(ralloc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/cache-Kernel/slab
    ${CMAKE_SOURCE_DIR}/cache-Kernel/buddySystem
)

# 链接线程库（如果需要）
find_package(Threads REQUIRED)
target_link_libraries(ralloc Threads::Threads)

