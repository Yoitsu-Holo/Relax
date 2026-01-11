cmake_minimum_required(VERSION 3.10)

# 加载代码覆盖率模块
include(CodeCoverage)

# 添加测试可执行文件（纯 C 语言测试）
add_executable(test_cache_interface
    test_cache_interface.c
)

# 设置 C 标准
set_property(TARGET test_cache_interface PROPERTY C_STANDARD 11)
set_property(TARGET test_cache_interface PROPERTY C_STANDARD_REQUIRED ON)

# 链接到 cache_interface 库
target_link_libraries(test_cache_interface
    cache_interface
    type_driver
    kv_driver
    hash_driver
    set_driver
    kv_engine
    ralloc
    buddy_system
    slab_allocator
    pthread
)

# 包含目录
target_include_directories(test_cache_interface PRIVATE
    ${CMAKE_SOURCE_DIR}/cache-Interface
    ${CMAKE_SOURCE_DIR}/cache-TypeDriver
    ${CMAKE_SOURCE_DIR}/cache-Kernel/kvEngine
    ${CMAKE_SOURCE_DIR}/cache-Kernel/ralloc
    ${CMAKE_SOURCE_DIR}/cache-Kernel/buddySystem
    ${CMAKE_SOURCE_DIR}/cache-Kernel/slab
    ${CMAKE_SOURCE_DIR}/lib/ankerl_unordered_dense
)

# 设置编译选项
target_compile_options(test_cache_interface PRIVATE
    -Wall
    -Wextra
)

# 启用代码覆盖率
setup_target_for_coverage(test_cache_interface)

# 添加测试
add_test(NAME test_cache_interface COMMAND test_cache_interface)

# 启用测试
enable_testing()

# 打印信息
message(STATUS "Cache Interface C test configured")
