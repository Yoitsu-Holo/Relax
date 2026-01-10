cmake_minimum_required(VERSION 3.10)
project(BuddySystem)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# 创建静态库
add_library(buddy_system STATIC
    buddy_allocator.cpp
    buddy_manager.cpp
)

# 设置包含目录
target_include_directories(buddy_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
