cmake_minimum_required(VERSION 3.10)
project(RelaxProject)

# 设置CMake模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 代码覆盖率选项
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
    else()
        message(WARNING "Code coverage requires GCC or Clang compiler")
    endif()
endif()

# 加载代码覆盖率模块
include(CodeCoverage)

# 添加子目录
add_subdirectory(cache-Kernel/slab)
add_subdirectory(cache-Kernel/buddySystem)
add_subdirectory(cache-Kernel/ralloc)

# 如果测试目录存在，添加测试
if(EXISTS ${CMAKE_SOURCE_DIR}/test)
    option(BUILD_TESTS "Build tests" ON)
    if(BUILD_TESTS)
        enable_testing()

        # 添加各个测试子目录
        if(EXISTS ${CMAKE_SOURCE_DIR}/test/slab)
            add_subdirectory(test/slab)
        endif()

        if(EXISTS ${CMAKE_SOURCE_DIR}/test/ralloc)
            add_subdirectory(test/ralloc)
        endif()

        if(EXISTS ${CMAKE_SOURCE_DIR}/test/time_wheel)
            add_subdirectory(test/time_wheel)
        endif()

        # kv_engine目录为空，暂不添加
        # if(EXISTS ${CMAKE_SOURCE_DIR}/test/kv_engine AND EXISTS ${CMAKE_SOURCE_DIR}/test/kv_engine/CMakeLists.txt)
        #     add_subdirectory(test/kv_engine)
        # endif()

        # 创建统一的代码覆盖率报告（包含所有测试模块）
        if(ENABLE_COVERAGE)
            # 创建统一的覆盖率报告（包含 lcov 和 gcovr）
            create_coverage_report("all" "")

            message(STATUS "Coverage target created:")
            message(STATUS "  - all_coverage: Generate all coverage reports")
            message(STATUS "    * LCOV HTML report at coverage/lcov/")
            message(STATUS "    * gcovr HTML report at coverage/gcovr/")
            message(STATUS "    * gcovr XML report at coverage/gcovr/coverage.xml")
        endif()
    endif()
endif()

# 添加benchmark
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
if(BUILD_BENCHMARKS)
    # 添加 slab benchmark
    if(EXISTS ${CMAKE_SOURCE_DIR}/bench/slab)
        add_subdirectory(bench/slab)
    endif()

    # 添加 ralloc benchmark
    if(EXISTS ${CMAKE_SOURCE_DIR}/bench/ralloc)
        add_subdirectory(bench/ralloc)
    endif()

    # 添加 simple_slab benchmark
    if(EXISTS ${CMAKE_SOURCE_DIR}/bench/simple_slab)
        add_subdirectory(bench/simple_slab)
    endif()
endif()

# 添加原始的benchmark（如果需要）- 保留向后兼容性
option(BUILD_ORIGINAL_BENCH "Build original benchmark" OFF)
if(BUILD_ORIGINAL_BENCH AND EXISTS ${CMAKE_SOURCE_DIR}/bench/slab/bench_slab_allocator.cpp)
    add_executable(original_bench bench/slab/bench_slab_allocator.cpp)
    target_link_libraries(original_bench slab_allocator)
endif()