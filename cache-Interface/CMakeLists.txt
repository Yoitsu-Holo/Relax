cmake_minimum_required(VERSION 3.10)

# cache-Interface 库 - C 语言封装接口
# 提供 C ABI 接口，可以被 Go 语言通过 cgo 调用

# 选项：构建共享库（.so）或静态库（.a）
option(BUILD_SHARED_LIBS "Build shared libraries (.so)" ON)

if(BUILD_SHARED_LIBS)
    # 构建动态库
    add_library(cache_interface SHARED
        type_driver_c.cpp
        type_driver_c.h
    )
    message(STATUS "Building cache_interface as SHARED library (.so)")
else()
    # 构建静态库
    add_library(cache_interface STATIC
        type_driver_c.cpp
        type_driver_c.h
    )
    message(STATUS "Building cache_interface as STATIC library (.a)")
endif()

# 设置库的版本信息
set_target_properties(cache_interface PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER type_driver_c.h
)

# 设置包含目录
target_include_directories(cache_interface PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 链接所有依赖
target_link_libraries(cache_interface PUBLIC
    type_driver
    kv_driver
    hash_driver
    set_driver
    kv_engine
)

# 设置编译选项
target_compile_options(cache_interface PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# 对于共享库，设置 fPIC 选项
if(BUILD_SHARED_LIBS)
    set_target_properties(cache_interface PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    # 确保所有依赖库也启用 fPIC
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# 安装规则（可选）
install(TARGETS cache_interface
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# 打印输出库文件位置信息
message(STATUS "cache_interface library will be built at: ${CMAKE_BINARY_DIR}/cache-Interface/")
