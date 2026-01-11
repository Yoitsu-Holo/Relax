cmake_minimum_required(VERSION 3.10)
project(kv_engine_bench)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置优化级别
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# 性能分析时需要调试符号
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -fno-omit-frame-pointer")

# 设置库的源文件路径
set(KV_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/kvEngine)
set(RALLOC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/ralloc)
set(SLAB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/slab)
set(BUDDY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/buddySystem)
set(ANKERL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/ankerl_unordered_dense)

# 添加benchmark可执行文件
add_executable(bench_kv_engine
    bench_kv_engine.cpp
    ${KV_ENGINE_DIR}/kv_engine.cpp
    ${RALLOC_DIR}/ralloc.cpp
    ${SLAB_DIR}/slab_allocator.cpp
    ${SLAB_DIR}/slab_manager.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
    ${BUDDY_DIR}/buddy_manager.cpp
)

# 包含目录
target_include_directories(bench_kv_engine PRIVATE
    ${KV_ENGINE_DIR}
    ${RALLOC_DIR}
    ${SLAB_DIR}
    ${BUDDY_DIR}
    ${ANKERL_DIR}
)

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(bench_kv_engine Threads::Threads)

# 设置 Release 模式为默认
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 输出编译信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "KV Engine source directory: ${KV_ENGINE_DIR}")
