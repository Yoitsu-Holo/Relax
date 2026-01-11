cmake_minimum_required(VERSION 3.10)

# TypeDriver 使用示例程序
add_executable(example_type_driver
    example_usage.cpp
)

# 链接 type_driver 库（会自动链接所有依赖）
target_link_libraries(example_type_driver
    type_driver
)

# 设置编译选项
target_compile_options(example_type_driver PRIVATE
    -Wall
    -Wextra
)
