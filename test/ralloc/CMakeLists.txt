cmake_minimum_required(VERSION 3.10)

# 加载代码覆盖率模块
include(CodeCoverage)

# 查找 Google Test
find_package(GTest REQUIRED)

# 添加测试可执行文件
add_executable(test_ralloc
    test_ralloc.cpp
)

# 链接到ralloc库和GTest
target_link_libraries(test_ralloc
    ralloc
    GTest::gtest
    GTest::gtest_main
    pthread
)

# 包含目录
target_include_directories(test_ralloc PRIVATE
    ${CMAKE_SOURCE_DIR}/cache-Kernel/ralloc
    ${CMAKE_SOURCE_DIR}/cache-Kernel/slab
    ${CMAKE_SOURCE_DIR}/cache-Kernel/buddySystem
)

# 启用代码覆盖率
setup_target_for_coverage(test_ralloc)

# 添加测试
include(GoogleTest)
gtest_discover_tests(test_ralloc)

# 启用测试
enable_testing()
