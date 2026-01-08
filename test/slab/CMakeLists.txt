cmake_minimum_required(VERSION 3.10)

# 加载代码覆盖率模块
include(CodeCoverage)

# 查找 Google Test
find_package(GTest REQUIRED)

# 添加测试可执行文件
add_executable(test_slab_allocator
    test_slab_allocator.cpp
)

# 链接到slab_allocator库和GTest
target_link_libraries(test_slab_allocator
    slab_allocator
    GTest::gtest
    GTest::gtest_main
    pthread
)

# 包含目录
target_include_directories(test_slab_allocator PRIVATE
    ${CMAKE_SOURCE_DIR}/cache-Kernel/slab
)

# 启用代码覆盖率
setup_target_for_coverage(test_slab_allocator)

# 添加测试
include(GoogleTest)
gtest_discover_tests(test_slab_allocator)

# 查找 Google Benchmark (可选)
find_package(benchmark QUIET)

if(benchmark_FOUND)
    # 添加benchmark可执行文件
    add_executable(bench_slab_allocator
        bench_slab_allocator.cpp
    )

    # 链接到slab_allocator库和benchmark
    target_link_libraries(bench_slab_allocator
        slab_allocator
        benchmark::benchmark
        pthread
    )

    # 包含目录
    target_include_directories(bench_slab_allocator PRIVATE
        ${CMAKE_SOURCE_DIR}/cache-Kernel/slab
    )

    # 启用代码覆盖率（benchmark也可以进行覆盖率测试）
    setup_target_for_coverage(bench_slab_allocator)

    message(STATUS "Google Benchmark found, benchmark executable will be built")
else()
    message(STATUS "Google Benchmark not found, skipping benchmark build")
endif()

# 启用测试
enable_testing()