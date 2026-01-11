# Cache Interface C 语言测试

本目录包含使用纯 C 语言编写的单元测试，用于验证 cache_interface 动态链接库的功能。

## 测试文件

- `test_cache_interface.c` - 纯 C 语言实现的测试套件

## 测试内容

测试套件包含 10 个测试用例，全面覆盖 cache_interface 库的所有功能：

### 1. TypeDriver 基础功能
- ✓ `test_create_destroy` - 测试创建和销毁 TypeDriver 实例
- ✓ `test_init` - 测试 TypeDriver 初始化

### 2. KV 操作
- ✓ `test_kv_operations` - 测试 KV 基本操作（set/get/exists/del）
- ✓ `test_kv_update` - 测试 KV 更新操作
- ✓ `test_multiple_kvs` - 测试多个键值对操作

### 3. Hash 操作
- ✓ `test_hash_single_field` - 测试 Hash 单字段操作
- ✓ `test_hash_batch_operations` - 测试 Hash 批量操作

### 4. Set 操作
- ✓ `test_set_single_member` - 测试 Set 单成员操作
- ✓ `test_set_batch_operations` - 测试 Set 批量操作

### 5. 错误处理
- ✓ `test_null_pointer_handling` - 测试 NULL 指针错误处理

## 编译和运行测试

### 编译测试

```bash
cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_BENCHMARKS=OFF
make test_cache_interface
```

### 直接运行测试

```bash
./test/cache-Interface/test_cache_interface
```

### 通过 CTest 运行

```bash
ctest -R test_cache_interface -V
```

## 测试输出示例

```
========================================
Cache Interface C Library Test Suite
========================================

[1] Running: test_create_destroy
✓ PASS: TypeDriver create and destroy

[2] Running: test_init
✓ PASS: TypeDriver initialization

...

========================================
Test Summary:
  Total:  10
  Passed: 10
  Failed: 0
========================================
✓ All tests passed!
```

## 测试特点

1. **纯 C 语言实现** - 不依赖任何 C++ 测试框架（如 GTest）
2. **自定义断言宏** - 提供清晰的测试失败信息
3. **完整覆盖** - 测试所有公开的 C 接口函数
4. **内存安全** - 正确处理内存分配和释放
5. **简单哈希实现** - 包含测试用的哈希和 CRC32 函数

## 测试设计说明

### 哈希函数

测试使用简单的字符串哈希函数和 CRC32 实现：
- `simple_hash()` - DJB2 哈希算法
- `simple_crc32()` - 标准 CRC32 算法

这些函数仅用于测试目的，实际使用时应该使用更健壮的哈希库。

### Set 操作说明

需要注意的是，`set_get_m()` 主要用于验证成员是否存在。Set 内部只存储成员的哈希值，不存储实际的成员字符串，因此：
- 可以使用 `set_exists_m()` 检查成员是否存在
- `set_get_m()` 不会返回成员的原始字符串数据
- 使用 `set_get()` 可以获取所有成员的列表

## 集成到 CI/CD

测试已经集成到 CMake 测试系统中，可以通过以下命令运行：

```bash
# 运行所有测试
ctest

# 只运行 cache_interface 测试
ctest -R test_cache_interface

# 详细输出
ctest -R test_cache_interface -V
```

## 故障排查

如果测试失败，检查：
1. 确保所有依赖库已正确编译
2. 检查是否有内存泄漏（使用 valgrind）
3. 查看详细的错误输出定位问题

使用 valgrind 检查内存泄漏：

```bash
valgrind --leak-check=full ./test/cache-Interface/test_cache_interface
```

## 扩展测试

要添加新的测试用例：

1. 在 `test_cache_interface.c` 中添加新的测试函数
2. 在 `main()` 函数中使用 `RUN_TEST()` 宏注册测试
3. 重新编译并运行测试

测试函数模板：

```c
int test_new_feature(void) {
    TypeDriverHandle handle = type_driver_create();
    TEST_ASSERT(handle != NULL, "TypeDriver should be created");

    // ... 测试代码 ...

    type_driver_destroy(handle);
    TEST_PASSED("New feature test");
    return 0;
}
```
