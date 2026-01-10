cmake_minimum_required(VERSION 3.10)

# 设置源文件目录
set(BUDDY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../cache-Kernel/buddySystem)

# 查找 Google Test
find_package(GTest REQUIRED)

# BuddyAllocator 测试
add_executable(test_buddy_system
    test_buddy_system.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
)

target_link_libraries(test_buddy_system
    GTest::GTest
    GTest::Main
    pthread
)

target_include_directories(test_buddy_system PRIVATE
    ${BUDDY_DIR}
)

# BuddyManager 测试
add_executable(test_buddy_manager
    test_buddy_manager.cpp
    ${BUDDY_DIR}/buddy_allocator.cpp
    ${BUDDY_DIR}/buddy_manager.cpp
)

target_link_libraries(test_buddy_manager
    pthread
)

target_include_directories(test_buddy_manager PRIVATE
    ${BUDDY_DIR}
)

# 启用测试
enable_testing()
add_test(NAME test_buddy_system COMMAND test_buddy_system)
add_test(NAME test_buddy_manager COMMAND test_buddy_manager)

# 输出信息
message(STATUS "Buddy System tests configured")
message(STATUS "Buddy directory: ${BUDDY_DIR}")
