cmake_minimum_required(VERSION 3.10)
project(KVEngine)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")

# 创建静态库
add_library(kv_engine STATIC
    kv_engine.cpp
)

# 链接依赖库
target_link_libraries(kv_engine
    ralloc
)

# 设置包含目录
target_include_directories(kv_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/cache-Kernel/ralloc
    ${CMAKE_SOURCE_DIR}/lib/ankerl_unordered_dense
)

# 链接线程库（如果需要）
find_package(Threads REQUIRED)
target_link_libraries(kv_engine Threads::Threads)
