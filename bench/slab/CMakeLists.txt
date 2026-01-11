cmake_minimum_required(VERSION 3.10)
project(slab_bench)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置优化级别
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 性能分析时需要调试符号
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

# 设置 slab 库的源文件路径
set(SLAB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/slab)

# 添加可执行文件 - benchmark
add_executable(bench_slab
    bench_slab.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${SLAB_DIR}/slab_allocator.cpp
)

# 添加可执行文件 - profiling
add_executable(profile_slab
    profile_slab.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${SLAB_DIR}/slab_allocator.cpp
)

# 为 profiling 版本添加调试符号
target_compile_options(profile_slab PRIVATE -O2 -g -fno-omit-frame-pointer)

# 添加头文件搜索路径
target_include_directories(bench_slab PRIVATE ${SLAB_DIR})
target_include_directories(profile_slab PRIVATE ${SLAB_DIR})

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
message(STATUS "Slab source directory: ${SLAB_DIR}")
