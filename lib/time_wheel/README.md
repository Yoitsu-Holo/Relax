# 多级时间轮框架 (Multi-Level Time Wheel)

## 概述

这是一个高性能的多级时间轮定时器框架，采用C++模板实现，支持从秒级到月级的多粒度定时任务管理。该框架特别适合需要管理大量定时器的场景，如网络服务器、任务调度器、缓存过期管理等。

## 核心特性

- **多级时间轮架构**：默认5级时间轮，覆盖秒、分、小时、天、月的时间范围
- **高精度计时**：基于微秒级精度，确保定时准确性
- **泛型设计**：通过C++模板支持任意数据类型
- **自动级联**：定时器在各级之间自动迁移，无需手动管理
- **可配置性**：支持自定义时间轮层级和参数
- **高性能**：O(1)时间复杂度的定时器添加和触发操作

## 架构设计

### 默认5级时间轮结构

```
Level 1: 1秒 × 60槽位 = 覆盖1分钟
Level 2: 1分钟 × 60槽位 = 覆盖1小时
Level 3: 1小时 × 24槽位 = 覆盖1天
Level 4: 1天 × 30槽位 = 覆盖1月
Level 5: 1月 × 12槽位 = 覆盖1年
```

### 工作原理

1. **添加定时器**：根据过期时间自动选择合适的层级
2. **时间推进**：随着时间推进，定时器从高层级向低层级迁移
3. **触发机制**：当定时器到达Level 1并过期时，触发用户回调函数

## 快速使用

### 基础示例

```cpp
#include "time_wheel.h"
#include <iostream>

using namespace time_wheel;

int main() {
    // 创建时间轮
    TimeWheel<std::string> tw;

    // 设置初始时间（微秒）
    uint64_t current = TimeWheel<std::string>::getCurrentTimeMicros();
    tw.setCurrentTime(current);

    // 设置超时回调
    tw.setTimeoutCallback([](const std::string& msg) {
        std::cout << "定时器触发: " << msg << std::endl;
    });

    // 添加定时器（5秒后触发）
    tw.addTimer(current + 5 * MICROSECONDS_PER_SECOND, "5秒定时器");

    // 添加定时器（1分钟后触发）
    tw.addTimer(current + MICROSECONDS_PER_MINUTE, "1分钟定时器");

    // 推进时间
    tw.advanceTime(current + 6 * MICROSECONDS_PER_SECOND);  // 触发5秒定时器
    tw.advanceTime(current + 61 * MICROSECONDS_PER_SECOND); // 触发1分钟定时器

    return 0;
}
```

### 自定义配置

```cpp
// 创建自定义3级时间轮配置
std::vector<LevelConfig> custom_config = {
    {MICROSECONDS_PER_SECOND, 10},      // L1: 10秒
    {10 * MICROSECONDS_PER_SECOND, 6},  // L2: 60秒
    {MICROSECONDS_PER_MINUTE, 10}       // L3: 10分钟
};

TimeWheel<int> tw(custom_config);
```

### 复杂数据类型

```cpp
struct Task {
    int id;
    std::string name;
    std::function<void()> handler;
};

TimeWheel<Task> tw;
tw.setTimeoutCallback([](const Task& task) {
    std::cout << "执行任务: " << task.name << std::endl;
    if (task.handler) {
        task.handler();
    }
});
```

## API参考

### 主要类

#### `TimeWheel<T>`
主时间轮管理器类，T为定时器携带的数据类型。

**构造函数**
```cpp
TimeWheel(const std::vector<LevelConfig>& configs = getDefaultConfig())
```

**主要方法**
- `setTimeoutCallback(callback)` - 设置超时回调函数
- `addTimer(expiry_time, data)` - 添加定时器
- `advanceTime(new_time)` - 推进时间并处理过期定时器
- `getCurrentTime()` - 获取当前时间
- `setCurrentTime(time)` - 设置当前时间

#### `LevelConfig`
时间轮层级配置结构体。

**成员**
- `time_slice` - 每个槽位的时间片（微秒）
- `bucket_count` - 该层级的槽位数量

### 时间常量

```cpp
MICROSECONDS_PER_SECOND = 1000000        // 1秒
MICROSECONDS_PER_MINUTE = 60000000       // 1分钟
MICROSECONDS_PER_HOUR   = 3600000000     // 1小时
MICROSECONDS_PER_DAY    = 86400000000    // 1天
MICROSECONDS_PER_MONTH  = 2592000000000  // 30天
```

## 使用场景

1. **网络服务器**：连接超时管理、心跳检测
2. **任务调度**：定时任务执行、延迟任务处理
3. **缓存系统**：缓存项过期管理
4. **游戏服务器**：技能冷却、buff持续时间管理
5. **消息队列**：延迟消息投递
6. **监控系统**：定期数据采集、告警触发

## 性能特点

- **添加定时器**：O(1)时间复杂度
- **时间推进**：O(m)，m为当前时间片内的定时器数量
- **内存占用**：O(n)，n为定时器总数
- **支持规模**：单实例可管理10万+定时器

## 编译和测试

### 编译测试程序

```bash
cd test/time_wheel
mkdir build && cd build
cmake ..
make
```

### 运行测试

```bash
# 运行单元测试
./test_time_wheel

# 运行性能基准测试（需要安装Google Benchmark）
./bench_time_wheel

# 运行示例程序
./example_time_wheel
```

## 注意事项

1. **时间单位**：所有时间值均使用微秒为单位
2. **时间推进**：需要定期调用`advanceTime()`来推进时间轮
3. **线程安全**：框架本身不提供线程安全保证，多线程环境需要外部同步
4. **时间倒流**：不支持时间倒退，`advanceTime()`只能向前推进

## 设计优势

1. **分级管理**：通过多级时间轮减少了需要频繁检查的定时器数量
2. **批量处理**：相同时间片的定时器可以批量处理，提高效率
3. **内存友好**：使用智能指针管理内存，避免内存泄漏
4. **灵活配置**：可根据实际需求调整层级数量和时间粒度

## 许可证

该项目采用项目默认许可证。详见项目根目录的LICENSE文件。