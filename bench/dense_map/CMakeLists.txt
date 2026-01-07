cmake_minimum_required(VERSION 3.10)
project(ankerl_dense_map_bench)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置优化级别
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# 添加包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/ankerl_unordered_dense)

# 添加可执行文件
add_executable(bench_ankerl_dense_map bench_ankerl_dense_map.cpp)
add_executable(bench_ankerl_dense_map_optimized bench_ankerl_dense_map_optimized.cpp)
add_executable(bench_ankerl_workloads bench_ankerl_workloads.cpp)
add_executable(bench_ankerl_dense_map_random bench_ankerl_dense_map_random.cpp)

# 设置 Release 模式为默认
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 如果是 MSVC，使用不同的优化级别
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /EHsc")
endif()

# 输出编译信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Library: ankerl::unordered_dense::map")