cmake_minimum_required(VERSION 3.10)

# KV Driver 库
add_library(kv_driver STATIC
    kv_driver.cpp
    kv_driver.h
)

# 设置包含目录
target_include_directories(kv_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 链接 kvEngine
target_link_libraries(kv_driver PUBLIC
    kv_engine
)

# 设置编译选项
target_compile_options(kv_driver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Hash Driver 库
add_library(hash_driver STATIC
    hash_driver.cpp
    hash_driver.h
)

# 设置包含目录
target_include_directories(hash_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 链接 kvEngine
target_link_libraries(hash_driver PUBLIC
    kv_engine
)

# 设置编译选项
target_compile_options(hash_driver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Set Driver 库
add_library(set_driver STATIC
    set_driver.cpp
    set_driver.h
)

# 设置包含目录
target_include_directories(set_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 链接 hash_driver（Set 内部使用 Hash 实现）
target_link_libraries(set_driver PUBLIC
    hash_driver
)

# 设置编译选项
target_compile_options(set_driver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# TypeDriver 库 - 统一封装所有类型驱动
add_library(type_driver STATIC
    type_driver.cpp
    type_driver.h
)

# 设置包含目录
target_include_directories(type_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 链接所有驱动
target_link_libraries(type_driver PUBLIC
    kv_driver
    hash_driver
    set_driver
    kv_engine
)

# 设置编译选项
target_compile_options(type_driver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
