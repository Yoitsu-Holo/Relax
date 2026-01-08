cmake_minimum_required(VERSION 3.10)

# 加载代码覆盖率模块
include(CodeCoverage)

# 查找 Google Test
find_package(GTest REQUIRED)

# 添加时间轮库
add_library(time_wheel INTERFACE)
target_include_directories(time_wheel INTERFACE
    ${CMAKE_SOURCE_DIR}/lib/time_wheel
)

# 添加测试可执行文件
add_executable(test_time_wheel
    test_time_wheel.cpp
)

# 链接到time_wheel库和GTest
target_link_libraries(test_time_wheel
    time_wheel
    GTest::gtest
    GTest::gtest_main
    pthread
)

# 包含目录
target_include_directories(test_time_wheel PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/time_wheel
)

# 启用代码覆盖率
setup_target_for_coverage(test_time_wheel)

# 添加测试
include(GoogleTest)
gtest_discover_tests(test_time_wheel)

# 查找 Google Benchmark (可选)
find_package(benchmark QUIET)

if(benchmark_FOUND)
    # 添加benchmark可执行文件
    add_executable(bench_time_wheel
        bench_time_wheel.cpp
    )

    # 链接到time_wheel库和benchmark
    target_link_libraries(bench_time_wheel
        time_wheel
        benchmark::benchmark
        pthread
    )

    # 包含目录
    target_include_directories(bench_time_wheel PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/time_wheel
    )

    # 启用代码覆盖率（benchmark也可以进行覆盖率测试）
    setup_target_for_coverage(bench_time_wheel)

    message(STATUS "Google Benchmark found, time wheel benchmark executable will be built")
else()
    message(STATUS "Google Benchmark not found, skipping time wheel benchmark build")
endif()

# 启用测试
enable_testing()