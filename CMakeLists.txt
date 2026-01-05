cmake_minimum_required(VERSION 3.10)
project(RelaxProject)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 添加子目录
add_subdirectory(cache-Kernel/slab)

# 如果测试目录存在，添加测试
if(EXISTS ${CMAKE_SOURCE_DIR}/test/slab)
    option(BUILD_TESTS "Build tests" ON)
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(test/slab)
    endif()
endif()

# 添加原始的benchmark（如果需要）
option(BUILD_ORIGINAL_BENCH "Build original benchmark" OFF)
if(BUILD_ORIGINAL_BENCH AND EXISTS ${CMAKE_SOURCE_DIR}/bench/slab/bench_slab_allocator.cpp)
    add_executable(original_bench bench/slab/bench_slab_allocator.cpp)
    target_link_libraries(original_bench slab_allocator)
endif()