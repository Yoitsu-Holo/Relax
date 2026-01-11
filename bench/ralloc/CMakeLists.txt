cmake_minimum_required(VERSION 3.10)
project(ralloc_bench)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置优化级别
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# 性能分析时需要调试符号
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")

# 设置库的源文件路径
set(RALLOC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/ralloc)
set(SLAB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/slab)
set(BUDDY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/buddySystem)

# 添加可执行文件 - benchmark
add_executable(bench_ralloc
    bench_ralloc.cpp
    ${RALLOC_DIR}/ralloc.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${SLAB_DIR}/slab_allocator.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
    ${BUDDY_DIR}/buddy_manager.cpp
)

# 添加可执行文件 - comprehensive benchmark
add_executable(benchmark_ralloc
    benchmark_ralloc.cpp
    ${RALLOC_DIR}/ralloc.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${SLAB_DIR}/slab_allocator.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
    ${BUDDY_DIR}/buddy_manager.cpp
)

# 添加可执行文件 - profiling
add_executable(profile_ralloc
    profile_ralloc.cpp
    ${RALLOC_DIR}/ralloc.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${SLAB_DIR}/slab_allocator.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
    ${BUDDY_DIR}/buddy_manager.cpp
)

# 添加可执行文件 - example
add_executable(example_ralloc
    example.cpp
    ${RALLOC_DIR}/ralloc.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${SLAB_DIR}/slab_allocator.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
    ${BUDDY_DIR}/buddy_manager.cpp
)

# 为 profiling 版本添加调试符号
target_compile_options(profile_ralloc PRIVATE -O2 -g -fno-omit-frame-pointer)

# 添加头文件搜索路径
target_include_directories(bench_ralloc PRIVATE
    ${RALLOC_DIR}
    ${SLAB_DIR}
    ${BUDDY_DIR}
)

target_include_directories(benchmark_ralloc PRIVATE
    ${RALLOC_DIR}
    ${SLAB_DIR}
    ${BUDDY_DIR}
)

target_include_directories(profile_ralloc PRIVATE
    ${RALLOC_DIR}
    ${SLAB_DIR}
    ${BUDDY_DIR}
)

target_include_directories(example_ralloc PRIVATE
    ${RALLOC_DIR}
    ${SLAB_DIR}
    ${BUDDY_DIR}
)

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(bench_ralloc Threads::Threads)
target_link_libraries(benchmark_ralloc Threads::Threads)
target_link_libraries(profile_ralloc Threads::Threads)
target_link_libraries(example_ralloc Threads::Threads)

# 设置 Release 模式为默认
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 输出编译信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "RAlloc source directory: ${RALLOC_DIR}")


