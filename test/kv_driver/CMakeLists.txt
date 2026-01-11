cmake_minimum_required(VERSION 3.10)

# 加载代码覆盖率模块
include(CodeCoverage)

# 查找 Google Test
find_package(GTest REQUIRED)

# 添加测试可执行文件
add_executable(test_kv_driver
    test_kv_driver.cpp
)

# 链接到kv_driver库和GTest
target_link_libraries(test_kv_driver
    kv_driver
    kv_engine
    ralloc
    slab_allocator
    buddy_system
    GTest::gtest
    GTest::gtest_main
    pthread
)

# 包含目录
target_include_directories(test_kv_driver PRIVATE
    ${CMAKE_SOURCE_DIR}/cache-TypeDriver
    ${CMAKE_SOURCE_DIR}/cache-Kernel/kvEngine
    ${CMAKE_SOURCE_DIR}/cache-Kernel/ralloc
    ${CMAKE_SOURCE_DIR}/lib/ankerl_unordered_dense
)

# 启用代码覆盖率
setup_target_for_coverage(test_kv_driver)

# 添加测试
include(GoogleTest)
gtest_discover_tests(test_kv_driver)

# 启用测试
enable_testing()
