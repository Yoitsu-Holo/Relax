我现在想要实现一个类似redis的分布式缓存架构，在底层的数据存储，我打算只实现 kv 这一种基础类型来实现其他所有数据类型

具体来说，我的kv存储引擎只接受一个uint64 hash的hash值、一个char* 数组，一个len,表示数据长度

底层的内存分配使用 @cache-Kernel/ralloc 中实现的内存分配器。

uint64 -> addr 的映射采用 @lib/ankerl_unordered_dense 来完成。

整体的调用链为：uint64 --ankerl_dense_map--> addr --直接内存访问--> data(char*)