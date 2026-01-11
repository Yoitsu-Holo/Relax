# Cache Interface 使用指南

本文档说明如何在 Go 语言中通过 cgo 调用 cache_interface 库。

## 编译选项

### 编译动态库 (.so)

```bash
cd build
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF
make cache_interface -j$(nproc)
```

生成的库文件位于：`build/cache-Interface/libcache_interface.so`

### 编译静态库 (.a)

```bash
cd build
cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_BENCHMARKS=OFF
make cache_interface -j$(nproc)
```

生成的库文件位于：`build/cache-Interface/libcache_interface.a`

## Go 语言使用示例

### 1. 准备 C 头文件

确保 Go 代码可以访问到 `cache-Interface/type_driver_c.h` 头文件。

### 2. Go 代码示例

```go
package main

/*
#cgo CFLAGS: -I/path/to/Relax/cache-Interface
#cgo LDFLAGS: -L/path/to/Relax/build/cache-Interface -lcache_interface -lstdc++

#include "type_driver_c.h"
#include <stdlib.h>
*/
import "C"
import (
    "fmt"
    "unsafe"
)

func main() {
    // 创建 TypeDriver 实例
    handle := C.type_driver_create()
    if handle == nil {
        fmt.Println("Failed to create TypeDriver")
        return
    }
    defer C.type_driver_destroy(handle)

    // 初始化 TypeDriver
    ret := C.type_driver_init(handle, 64)
    if ret != C.C_TYPE_DRIVER_OK {
        fmt.Printf("Failed to initialize TypeDriver: %d\n", ret)
        return
    }

    // 示例：设置一个 KV 对
    key := "test_key"
    value := "test_value"

    keyHash := uint64(12345)  // 实际使用时应该用哈希函数计算
    keyCrc32 := uint32(67890) // 实际使用时应该用 CRC32 函数计算

    cValue := C.CString(value)
    defer C.free(unsafe.Pointer(cValue))

    ret = C.type_driver_kv_set(
        handle,
        C.uint64_t(keyHash),
        C.uint32_t(keyCrc32),
        cValue,
        C.size_t(len(value)),
    )

    if ret != C.C_TYPE_DRIVER_OK {
        fmt.Printf("Failed to set KV: %d\n", ret)
        return
    }

    fmt.Println("KV set successfully!")

    // 示例：获取值
    var retData *C.char
    var retLen C.size_t

    ret = C.type_driver_kv_get(
        handle,
        C.uint64_t(keyHash),
        C.uint32_t(keyCrc32),
        &retData,
        &retLen,
    )

    if ret != C.C_TYPE_DRIVER_OK {
        fmt.Printf("Failed to get KV: %d\n", ret)
        return
    }

    retrievedValue := C.GoStringN(retData, C.int(retLen))
    fmt.Printf("Retrieved value: %s\n", retrievedValue)
}
```

### 3. Hash 操作示例

```go
// 设置 Hash 字段
field := C.CHashField{
    field_hash:  C.uint64_t(123),
    field_crc32: C.uint32_t(456),
    field:       C.CString("field1"),
    field_len:   6,
    value:       C.CString("value1"),
    value_len:   6,
}
defer C.free(unsafe.Pointer(field.field))
defer C.free(unsafe.Pointer(field.value))

ret := C.type_driver_hash_set_m(
    handle,
    C.uint64_t(hashKey),
    C.uint32_t(hashCrc32),
    &field,
)
```

### 4. Set 操作示例

```go
// 添加 Set 成员
member := C.CSetMember{
    member_hash:  C.uint64_t(789),
    member_crc32: C.uint32_t(101),
    member:       C.CString("member1"),
    member_len:   7,
}
defer C.free(unsafe.Pointer(member.member))

ret := C.type_driver_set_add_m(
    handle,
    C.uint64_t(setKey),
    C.uint32_t(setCrc32),
    &member,
)
```

## 错误码

所有函数返回的错误码定义如下：

- `C_TYPE_DRIVER_OK` (0): 操作成功
- `C_TYPE_DRIVER_ERROR_NULL_POINTER` (-1): 空指针错误
- `C_TYPE_DRIVER_ERROR_ALREADY_INIT` (-2): 已经初始化
- `C_TYPE_DRIVER_ERROR_NOT_INIT` (-3): 未初始化
- `C_TYPE_DRIVER_ERROR_INIT_FAILED` (-4): 初始化失败
- `C_TYPE_DRIVER_ERROR_ALLOCATION_FAILED` (-5): 内存分配失败

## 注意事项

1. 使用动态库时，需要确保 `libcache_interface.so` 在系统的库搜索路径中，或者设置 `LD_LIBRARY_PATH` 环境变量。

2. 使用静态库时，需要链接所有依赖的静态库：
   ```go
   #cgo LDFLAGS: -L/path/to/build/cache-Interface -lcache_interface \
                 -L/path/to/build/cache-TypeDriver -ltype_driver -lkv_driver -lhash_driver -lset_driver \
                 -L/path/to/build/cache-Kernel/kvEngine -lkv_engine \
                 -L/path/to/build/cache-Kernel/ralloc -lralloc \
                 -L/path/to/build/cache-Kernel/buddySystem -lbuddy_system \
                 -L/path/to/build/cache-Kernel/slab -lslab_allocator \
                 -lstdc++
   ```

3. 所有从 C 返回的字符串内存（如 `hash_get`, `set_get` 等）都需要调用者负责释放。

4. 在计算 hash 和 CRC32 时，应使用相同的哈希函数，建议使用 `hash/crc32` 包和 MurmurHash 或 xxHash。

## 项目结构

```
cache-Interface/
├── type_driver_c.h       # C 接口头文件
├── type_driver_c.cpp     # C 接口实现
└── CMakeLists.txt        # 构建配置
```

## 构建系统说明

- 通过 `-DBUILD_SHARED_LIBS=ON/OFF` 控制编译动态库还是静态库
- 默认编译动态库
- 所有依赖库会自动启用位置无关代码 (PIC)
- 支持版本号管理（当前版本 1.0.0）
