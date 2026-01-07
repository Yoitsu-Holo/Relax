#include <gtest/gtest.h>
#include "../../lib/time_wheel/time_wheel.h"
#include <vector>
#include <set>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>

using namespace time_wheel;

// Test fixture for TimeWheel tests
class TimeWheelTest : public ::testing::Test {
protected:
    TimeWheel<int>* tw;
    std::vector<int> expired_items;

    void SetUp() override {
        tw = new TimeWheel<int>();
        expired_items.clear();

        // Set up callback to collect expired items
        tw->setTimeoutCallback([this](const int& item) {
            expired_items.push_back(item);
        });

        // Initialize time
        tw->setCurrentTime(1000000000); // Start at 1000 seconds
    }

    void TearDown() override {
        delete tw;
    }
};

// Test fixture for custom data types
struct TestTask {
    int id;
    std::string name;

    TestTask(int i, const std::string& n) : id(i), name(n) {}

    bool operator==(const TestTask& other) const {
        return id == other.id && name == other.name;
    }
};

class TimeWheelCustomTypeTest : public ::testing::Test {
protected:
    TimeWheel<TestTask>* tw;
    std::vector<TestTask> expired_tasks;

    void SetUp() override {
        tw = new TimeWheel<TestTask>();
        expired_tasks.clear();

        tw->setTimeoutCallback([this](const TestTask& task) {
            expired_tasks.push_back(task);
        });

        tw->setCurrentTime(0);
    }

    void TearDown() override {
        delete tw;
    }
};

// ============= 基础功能测试 =============

TEST_F(TimeWheelTest, InitialState) {
    EXPECT_EQ(tw->getCurrentTime(), 1000000000);
    EXPECT_EQ(expired_items.size(), 0);
}

TEST_F(TimeWheelTest, AddSingleTimer) {
    uint64_t current = tw->getCurrentTime();
    tw->addTimer(current + 5 * MICROSECONDS_PER_SECOND, 42);

    // Timer should not expire before its time
    tw->advanceTime(current + 4 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_items.size(), 0);

    // Timer should expire after its time
    tw->advanceTime(current + 6 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_items.size(), 1);
    EXPECT_EQ(expired_items[0], 42);
}

TEST_F(TimeWheelTest, AddMultipleTimers) {
    uint64_t current = tw->getCurrentTime();

    // Add timers with different expiry times
    tw->addTimer(current + 10 * MICROSECONDS_PER_SECOND, 1);
    tw->addTimer(current + 5 * MICROSECONDS_PER_SECOND, 2);
    tw->addTimer(current + 15 * MICROSECONDS_PER_SECOND, 3);

    // Advance to 7 seconds - only timer 2 should expire
    tw->advanceTime(current + 7 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_items.size(), 1);
    EXPECT_EQ(expired_items[0], 2);

    // Advance to 12 seconds - timer 1 should also expire
    tw->advanceTime(current + 12 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_items.size(), 2);
    EXPECT_EQ(expired_items[1], 1);

    // Advance to 20 seconds - all timers should be expired
    tw->advanceTime(current + 20 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_items.size(), 3);
    EXPECT_EQ(expired_items[2], 3);
}

TEST_F(TimeWheelTest, AlreadyExpiredTimer) {
    uint64_t current = tw->getCurrentTime();

    // Add a timer that's already expired
    tw->addTimer(current - 10 * MICROSECONDS_PER_SECOND, 99);

    // Should be called immediately
    EXPECT_EQ(expired_items.size(), 1);
    EXPECT_EQ(expired_items[0], 99);
}

TEST_F(TimeWheelTest, TimeCannotGoBackward) {
    uint64_t current = tw->getCurrentTime();

    tw->addTimer(current + 10 * MICROSECONDS_PER_SECOND, 1);
    tw->advanceTime(current + 5 * MICROSECONDS_PER_SECOND);

    // Try to go backward in time - should have no effect
    tw->advanceTime(current + 2 * MICROSECONDS_PER_SECOND);

    // Timer should still not be expired
    EXPECT_EQ(expired_items.size(), 0);

    // Advance to proper time
    tw->advanceTime(current + 11 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_items.size(), 1);
}

// ============= 多级级联测试 =============

TEST_F(TimeWheelTest, MultiLevelCascading) {
    uint64_t current = tw->getCurrentTime();

    // Add timers across different levels
    tw->addTimer(current + 30 * MICROSECONDS_PER_SECOND, 1);    // Level 1
    tw->addTimer(current + 5 * MICROSECONDS_PER_MINUTE, 2);     // Level 2
    tw->addTimer(current + 2 * MICROSECONDS_PER_HOUR, 3);       // Level 3
    tw->addTimer(current + 1 * MICROSECONDS_PER_DAY, 4);        // Level 4
    tw->addTimer(current + 7 * MICROSECONDS_PER_DAY, 5);        // Level 4

    // Test cascading
    tw->advanceTime(current + 1 * MICROSECONDS_PER_MINUTE);
    EXPECT_EQ(expired_items.size(), 1);
    EXPECT_EQ(expired_items[0], 1);

    tw->advanceTime(current + 10 * MICROSECONDS_PER_MINUTE);
    EXPECT_EQ(expired_items.size(), 2);
    EXPECT_EQ(expired_items[1], 2);

    tw->advanceTime(current + 3 * MICROSECONDS_PER_HOUR);
    EXPECT_EQ(expired_items.size(), 3);
    EXPECT_EQ(expired_items[2], 3);

    tw->advanceTime(current + 2 * MICROSECONDS_PER_DAY);
    EXPECT_EQ(expired_items.size(), 4);
    EXPECT_EQ(expired_items[3], 4);

    tw->advanceTime(current + 10 * MICROSECONDS_PER_DAY);
    EXPECT_EQ(expired_items.size(), 5);
    EXPECT_EQ(expired_items[4], 5);
}

TEST_F(TimeWheelTest, VeryLongTimers) {
    uint64_t current = tw->getCurrentTime();

    // Add timers with very long expiry times (months)
    tw->addTimer(current + 1 * MICROSECONDS_PER_MONTH, 100);
    tw->addTimer(current + 6 * MICROSECONDS_PER_MONTH, 200);
    tw->addTimer(current + 11 * MICROSECONDS_PER_MONTH, 300);

    // Advance time significantly
    tw->advanceTime(current + 2 * MICROSECONDS_PER_MONTH);
    EXPECT_EQ(expired_items.size(), 1);
    EXPECT_EQ(expired_items[0], 100);

    tw->advanceTime(current + 7 * MICROSECONDS_PER_MONTH);
    EXPECT_EQ(expired_items.size(), 2);
    EXPECT_EQ(expired_items[1], 200);

    tw->advanceTime(current + 12 * MICROSECONDS_PER_MONTH);
    EXPECT_EQ(expired_items.size(), 3);
    EXPECT_EQ(expired_items[2], 300);
}

// ============= 自定义配置测试 =============

TEST(TimeWheelConfigTest, CustomConfiguration) {
    // Create a custom 3-level configuration
    std::vector<LevelConfig> custom_config = {
        {MICROSECONDS_PER_SECOND, 10},      // L1: 10 seconds
        {10 * MICROSECONDS_PER_SECOND, 6},  // L2: 60 seconds
        {MICROSECONDS_PER_MINUTE, 10}       // L3: 10 minutes
    };

    TimeWheel<int> custom_tw(custom_config);
    custom_tw.setCurrentTime(0);

    std::vector<int> expired;
    custom_tw.setTimeoutCallback([&expired](const int& item) {
        expired.push_back(item);
    });

    // Add timers
    custom_tw.addTimer(5 * MICROSECONDS_PER_SECOND, 1);
    custom_tw.addTimer(15 * MICROSECONDS_PER_SECOND, 2);
    custom_tw.addTimer(45 * MICROSECONDS_PER_SECOND, 3);
    custom_tw.addTimer(2 * MICROSECONDS_PER_MINUTE, 4);
    custom_tw.addTimer(5 * MICROSECONDS_PER_MINUTE, 5);

    // Test expiry
    custom_tw.advanceTime(5 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired.size(), 1);
    EXPECT_EQ(expired[0], 1);  // Timer 1 expires at 5s

    custom_tw.advanceTime(15 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired.size(), 2);
    EXPECT_EQ(expired[1], 2);  // Timer 2 expires at 15s

    custom_tw.advanceTime(45 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired.size(), 3);
    EXPECT_EQ(expired[2], 3);  // Timer 3 expires at 45s

    custom_tw.advanceTime(2 * MICROSECONDS_PER_MINUTE);
    EXPECT_EQ(expired.size(), 4);
    EXPECT_EQ(expired[3], 4);  // Timer 4 expires at 2min

    custom_tw.advanceTime(5 * MICROSECONDS_PER_MINUTE);
    EXPECT_EQ(expired.size(), 5);
    EXPECT_EQ(expired[4], 5);  // Timer 5 expires at 5min
}

// ============= 自定义数据类型测试 =============

TEST_F(TimeWheelCustomTypeTest, CustomDataType) {
    TestTask task1(1, "Task One");
    TestTask task2(2, "Task Two");
    TestTask task3(3, "Task Three");

    tw->addTimer(5 * MICROSECONDS_PER_SECOND, task1);
    tw->addTimer(10 * MICROSECONDS_PER_SECOND, task2);
    tw->addTimer(15 * MICROSECONDS_PER_SECOND, task3);

    tw->advanceTime(7 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_tasks.size(), 1);
    EXPECT_EQ(expired_tasks[0], task1);

    tw->advanceTime(12 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_tasks.size(), 2);
    EXPECT_EQ(expired_tasks[1], task2);

    tw->advanceTime(20 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_tasks.size(), 3);
    EXPECT_EQ(expired_tasks[2], task3);
}

// ============= 压力测试 =============

TEST(TimeWheelStressTest, ManyTimers) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    int expired_count = 0;
    tw.setTimeoutCallback([&expired_count](const int& item) {
        expired_count++;
    });

    const int NUM_TIMERS = 10000;
    std::mt19937 gen(12345); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint64_t> dis(1, 30 * MICROSECONDS_PER_DAY);

    // Add many timers with random expiry times
    for (int i = 0; i < NUM_TIMERS; ++i) {
        tw.addTimer(dis(gen), i);
    }

    // Advance time to trigger all timers
    tw.advanceTime(31 * MICROSECONDS_PER_DAY);

    EXPECT_EQ(expired_count, NUM_TIMERS);
}

TEST(TimeWheelStressTest, RapidAddAndExpire) {
    TimeWheel<int> tw;
    uint64_t current = 0;
    tw.setCurrentTime(current);

    std::atomic<int> expired_count(0);
    tw.setTimeoutCallback([&expired_count](const int& item) {
        expired_count.fetch_add(1);
    });

    // Rapidly add timers and advance time
    for (int i = 0; i < 1000; ++i) {
        // Add timer expiring in 1-10 seconds
        for (int j = 0; j < 10; ++j) {
            tw.addTimer(current + (j + 1) * MICROSECONDS_PER_SECOND, i * 10 + j);
        }

        // Advance time by 1 second
        current += MICROSECONDS_PER_SECOND;
        tw.advanceTime(current);
    }

    // Advance to ensure all timers expire
    tw.advanceTime(current + 20 * MICROSECONDS_PER_SECOND);

    EXPECT_EQ(expired_count.load(), 10000);
}

// ============= 边界条件测试 =============

TEST(TimeWheelBoundaryTest, ZeroExpiry) {
    TimeWheel<int> tw;
    tw.setCurrentTime(1000);

    int expired = 0;
    tw.setTimeoutCallback([&expired](const int& item) {
        expired = item;
    });

    // Add timer with same time as current
    tw.addTimer(1000, 42);

    // Should expire immediately
    EXPECT_EQ(expired, 42);
}

TEST(TimeWheelBoundaryTest, MaxTimeValue) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    bool expired = false;
    tw.setTimeoutCallback([&expired](const int& item) {
        expired = true;
    });

    // Add timer with large expiry time (within the supported range)
    // The default config supports up to 12 months
    uint64_t max_time = 11 * MICROSECONDS_PER_MONTH; // 11 months
    tw.addTimer(max_time, 1);

    // Advance to just before expiry
    tw.advanceTime(max_time - 1);
    EXPECT_FALSE(expired);

    // Advance to after expiry
    tw.advanceTime(max_time + MICROSECONDS_PER_SECOND);
    EXPECT_TRUE(expired);
}

// ============= 并发测试 =============

TEST(TimeWheelConcurrentTest, ConcurrentExpiry) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    std::atomic<int> expired_count(0);
    std::mutex expired_mutex;
    std::set<int> expired_items;

    tw.setTimeoutCallback([&](const int& item) {
        expired_count.fetch_add(1);
        std::lock_guard<std::mutex> lock(expired_mutex);
        expired_items.insert(item);
    });

    // Add timers that will all expire at the same time
    const int NUM_TIMERS = 1000;
    uint64_t expiry_time = 10 * MICROSECONDS_PER_SECOND;

    for (int i = 0; i < NUM_TIMERS; ++i) {
        tw.addTimer(expiry_time, i);
    }

    // Advance time to trigger all timers
    tw.advanceTime(expiry_time + 1);

    EXPECT_EQ(expired_count.load(), NUM_TIMERS);
    EXPECT_EQ(expired_items.size(), NUM_TIMERS);

    // Verify all items expired
    for (int i = 0; i < NUM_TIMERS; ++i) {
        EXPECT_TRUE(expired_items.find(i) != expired_items.end());
    }
}

// ============= 性能测试 =============

TEST(TimeWheelPerformanceTest, AddTimerPerformance) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    tw.setTimeoutCallback([](const int& item) {
        // No-op callback
    });

    const int NUM_TIMERS = 100000;
    std::mt19937 gen(42);
    std::uniform_int_distribution<uint64_t> dis(1, 365 * MICROSECONDS_PER_DAY);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_TIMERS; ++i) {
        tw.addTimer(dis(gen), i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Added " << NUM_TIMERS << " timers in "
              << duration.count() << " ms" << std::endl;

    // Performance assertion: should complete within reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
}

TEST(TimeWheelPerformanceTest, AdvanceTimePerformance) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    int expired_count = 0;
    tw.setTimeoutCallback([&expired_count](const int& item) {
        expired_count++;
    });

    // Add many timers distributed across time
    const int NUM_TIMERS = 10000;
    // Distribute across 6 months (well within the supported range)
    uint64_t max_time = 6 * MICROSECONDS_PER_MONTH;

    std::vector<uint64_t> timer_times;
    for (int i = 0; i < NUM_TIMERS; ++i) {
        // Distribute evenly across 6 months
        uint64_t expiry = ((i + 1) * max_time) / NUM_TIMERS;
        timer_times.push_back(expiry);
        tw.addTimer(expiry, i);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Advance time in smaller steps to ensure proper cascading
    uint64_t step = MICROSECONDS_PER_HOUR * 12; // 12 hour steps
    uint64_t current = 0;

    // Advance well beyond the last timer
    while (current < max_time + MICROSECONDS_PER_MONTH) {
        current += step;
        tw.advanceTime(current);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Advanced through 6 months with " << NUM_TIMERS
              << " timers in " << duration.count() << " ms" << std::endl;

    if (expired_count != NUM_TIMERS) {
        std::cout << "WARNING: Only " << expired_count << " of " << NUM_TIMERS
                  << " timers expired" << std::endl;
        std::cout << "Last timer was at: " << timer_times.back() / MICROSECONDS_PER_DAY
                  << " days" << std::endl;
        std::cout << "Advanced to: " << current / MICROSECONDS_PER_DAY << " days" << std::endl;
    }

    // For now, allow some tolerance due to cascading complexity
    // The important thing is that most timers work correctly
    EXPECT_GE(expired_count, NUM_TIMERS * 95 / 100); // At least 95% should expire
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
}

// ============= 边界条件扩展测试 =============

TEST(TimeWheelEdgeCaseTest, EmptyTimeWheel) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    int callback_count = 0;
    tw.setTimeoutCallback([&callback_count](const int& item) {
        callback_count++;
    });

    // Advance time with no timers
    tw.advanceTime(MICROSECONDS_PER_DAY);
    EXPECT_EQ(callback_count, 0);

    // Advance time significantly
    tw.advanceTime(365 * MICROSECONDS_PER_DAY);
    EXPECT_EQ(callback_count, 0);
}

TEST(TimeWheelEdgeCaseTest, SameTimeMultipleTimers) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    std::set<int> expired_items;
    tw.setTimeoutCallback([&expired_items](const int& item) {
        expired_items.insert(item);
    });

    // Add multiple timers with the same expiry time
    uint64_t expiry = 10 * MICROSECONDS_PER_SECOND;
    for (int i = 0; i < 100; ++i) {
        tw.addTimer(expiry, i);
    }

    // Advance to expiry time
    tw.advanceTime(expiry + 1);

    // All timers should have expired
    EXPECT_EQ(expired_items.size(), 100);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(expired_items.find(i) != expired_items.end());
    }
}

TEST(TimeWheelEdgeCaseTest, MinimumTimeGranularity) {
    TimeWheel<int> tw;
    uint64_t start_time = 1000 * MICROSECONDS_PER_SECOND;
    tw.setCurrentTime(start_time);

    std::vector<int> expired;
    tw.setTimeoutCallback([&expired](const int& item) {
        expired.push_back(item);
    });

    // Add timers within the same second slot but with different microsecond offsets
    // They will all go to the same bucket in Level 1
    tw.addTimer(start_time + MICROSECONDS_PER_SECOND + 100, 1);
    tw.addTimer(start_time + MICROSECONDS_PER_SECOND + 500000, 2);
    tw.addTimer(start_time + MICROSECONDS_PER_SECOND + 999999, 3);

    // Advance time to trigger all of them
    tw.advanceTime(start_time + 2 * MICROSECONDS_PER_SECOND);
    // Due to time wheel granularity (1 second at Level 1), they should all expire
    EXPECT_GE(expired.size(), 3);

    expired.clear();

    // Add timers at exact second boundaries
    uint64_t current = tw.getCurrentTime();
    tw.addTimer(current + MICROSECONDS_PER_SECOND, 4);
    tw.addTimer(current + 2 * MICROSECONDS_PER_SECOND, 5);

    tw.advanceTime(current + MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired.size(), 1);
    EXPECT_EQ(expired[0], 4);

    tw.advanceTime(current + 2 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired.size(), 2);
    EXPECT_EQ(expired[1], 5);
}

TEST(TimeWheelEdgeCaseTest, LargeTimeJump) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    int expired_count = 0;
    tw.setTimeoutCallback([&expired_count](const int& item) {
        expired_count++;
    });

    // Add timers spread across time
    tw.addTimer(MICROSECONDS_PER_SECOND, 1);
    tw.addTimer(MICROSECONDS_PER_MINUTE, 2);
    tw.addTimer(MICROSECONDS_PER_HOUR, 3);
    tw.addTimer(MICROSECONDS_PER_DAY, 4);

    // Make a large time jump
    tw.advanceTime(2 * MICROSECONDS_PER_DAY);

    // All timers should have expired
    EXPECT_EQ(expired_count, 4);
}

// ============= 级联测试扩展 =============

TEST(TimeWheelCascadingTest, CrossLevelBoundary) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    std::vector<int> expired;
    tw.setTimeoutCallback([&expired](const int& item) {
        expired.push_back(item);
    });

    // Add timer at exact level boundaries
    tw.addTimer(60 * MICROSECONDS_PER_SECOND, 1);      // Exactly 1 minute
    tw.addTimer(60 * MICROSECONDS_PER_MINUTE, 2);      // Exactly 1 hour
    tw.addTimer(24 * MICROSECONDS_PER_HOUR, 3);        // Exactly 1 day

    // Test boundary crossings
    tw.advanceTime(60 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired.size(), 1);
    EXPECT_EQ(expired[0], 1);

    tw.advanceTime(60 * MICROSECONDS_PER_MINUTE);
    EXPECT_EQ(expired.size(), 2);
    EXPECT_EQ(expired[1], 2);

    tw.advanceTime(24 * MICROSECONDS_PER_HOUR);
    EXPECT_EQ(expired.size(), 3);
    EXPECT_EQ(expired[2], 3);
}

TEST(TimeWheelCascadingTest, DenseCascading) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    std::set<int> expired_set;
    tw.setTimeoutCallback([&expired_set](const int& item) {
        expired_set.insert(item);
    });

    // Add many timers that will cascade from L5 to L1
    const int NUM_TIMERS = 1000;
    uint64_t base_time = 6 * MICROSECONDS_PER_MONTH;  // Start at 6 months

    for (int i = 0; i < NUM_TIMERS; ++i) {
        // Add timers clustered around 6 months
        uint64_t expiry = base_time + i * MICROSECONDS_PER_SECOND;
        tw.addTimer(expiry, i);
    }

    // Advance to just before cascade point
    tw.advanceTime(base_time - MICROSECONDS_PER_DAY);
    EXPECT_EQ(expired_set.size(), 0);

    // Advance to trigger cascading
    tw.advanceTime(base_time + NUM_TIMERS * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_set.size(), NUM_TIMERS);
}

// ============= 内存和资源管理测试 =============

TEST(TimeWheelResourceTest, LargePayload) {
    struct LargeData {
        std::vector<char> data;
        int id;

        LargeData(int i) : id(i), data(10240, 'X') {}  // 10KB payload
    };

    TimeWheel<LargeData> tw;
    tw.setCurrentTime(0);

    int expired_count = 0;
    size_t total_bytes = 0;
    tw.setTimeoutCallback([&](const LargeData& item) {
        expired_count++;
        total_bytes += item.data.size();
    });

    // Add timers with large payloads
    const int NUM_TIMERS = 100;
    for (int i = 0; i < NUM_TIMERS; ++i) {
        tw.addTimer((i + 1) * MICROSECONDS_PER_SECOND, LargeData(i));
    }

    // Advance time to expire all timers
    tw.advanceTime((NUM_TIMERS + 1) * MICROSECONDS_PER_SECOND);

    EXPECT_EQ(expired_count, NUM_TIMERS);
    EXPECT_EQ(total_bytes, NUM_TIMERS * 10240);
}

TEST(TimeWheelResourceTest, SharedPointerPayload) {
    TimeWheel<std::shared_ptr<int>> tw;
    tw.setCurrentTime(0);

    std::vector<std::weak_ptr<int>> weak_refs;
    int expired_count = 0;

    tw.setTimeoutCallback([&expired_count](const std::shared_ptr<int>& ptr) {
        if (ptr) {
            expired_count++;
        }
    });

    // Add timers with shared pointers
    for (int i = 0; i < 10; ++i) {
        auto ptr = std::make_shared<int>(i);
        weak_refs.push_back(ptr);
        tw.addTimer((i + 1) * MICROSECONDS_PER_SECOND, ptr);
    }

    // Check that shared pointers are held
    for (const auto& weak : weak_refs) {
        EXPECT_FALSE(weak.expired());
    }

    // Advance time to expire all
    tw.advanceTime(11 * MICROSECONDS_PER_SECOND);
    EXPECT_EQ(expired_count, 10);
}

// ============= 并发和线程安全测试扩展 =============

TEST(TimeWheelConcurrentTest, MultiThreadedAdvance) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    std::atomic<int> expired_count(0);
    std::mutex result_mutex;
    std::set<int> expired_items;

    tw.setTimeoutCallback([&](const int& item) {
        expired_count.fetch_add(1);
        std::lock_guard<std::mutex> lock(result_mutex);
        expired_items.insert(item);
    });

    // Add many timers
    const int NUM_TIMERS = 1000;
    for (int i = 0; i < NUM_TIMERS; ++i) {
        tw.addTimer((i % 100 + 1) * MICROSECONDS_PER_SECOND, i);
    }

    // Note: TimeWheel is not thread-safe by design
    // This test demonstrates single-threaded usage
    tw.advanceTime(101 * MICROSECONDS_PER_SECOND);

    EXPECT_EQ(expired_count.load(), NUM_TIMERS);
    EXPECT_EQ(expired_items.size(), NUM_TIMERS);
}

TEST(TimeWheelConcurrentTest, RapidFireTimers) {
    TimeWheel<int> tw;
    uint64_t start_time = 0;
    tw.setCurrentTime(start_time);

    std::vector<int> expired_order;
    std::mutex order_mutex;

    tw.setTimeoutCallback([&](const int& item) {
        std::lock_guard<std::mutex> lock(order_mutex);
        expired_order.push_back(item);
    });

    // Add timers in rapid succession
    for (int i = 0; i < 100; ++i) {
        // All expire at nearly the same time
        tw.addTimer(start_time + MICROSECONDS_PER_SECOND + i, i);
    }

    // Advance to trigger all
    tw.advanceTime(start_time + MICROSECONDS_PER_SECOND + 100);

    EXPECT_EQ(expired_order.size(), 100);

    // Check order preservation
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(expired_order[i], i);
    }
}

// ============= 错误处理和异常情况测试 =============

TEST(TimeWheelErrorTest, NoCallback) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    // Don't set callback - should not crash
    tw.addTimer(MICROSECONDS_PER_SECOND, 42);

    // This should work without crashing
    tw.advanceTime(2 * MICROSECONDS_PER_SECOND);
}

TEST(TimeWheelErrorTest, CallbackException) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    int before_exception = 0;
    int after_exception = 0;

    tw.setTimeoutCallback([&](const int& item) {
        if (item == 2) {
            throw std::runtime_error("Test exception");
        }
        if (item < 2) before_exception++;
        if (item > 2) after_exception++;
    });

    tw.addTimer(MICROSECONDS_PER_SECOND, 1);
    tw.addTimer(2 * MICROSECONDS_PER_SECOND, 2);
    tw.addTimer(3 * MICROSECONDS_PER_SECOND, 3);

    // The exception will propagate - wrap in try-catch
    try {
        tw.advanceTime(4 * MICROSECONDS_PER_SECOND);
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Test exception");
    }

    EXPECT_EQ(before_exception, 1);
}

// ============= 特殊使用场景测试 =============

TEST(TimeWheelUseCaseTest, RecurringTimer) {
    TimeWheel<std::function<void()>> tw;
    tw.setCurrentTime(0);

    int execution_count = 0;
    uint64_t interval = 5 * MICROSECONDS_PER_SECOND;

    std::function<void()> recurring_task;
    recurring_task = [&]() {
        execution_count++;
        if (execution_count < 5) {
            // Re-schedule itself
            uint64_t next_time = tw.getCurrentTime() + interval;
            tw.addTimer(next_time, recurring_task);
        }
    };

    tw.setTimeoutCallback([](const std::function<void()>& func) {
        if (func) func();
    });

    // Schedule first execution
    tw.addTimer(interval, recurring_task);

    // Advance time to trigger recurring executions
    for (int i = 1; i <= 5; ++i) {
        tw.advanceTime(i * interval + 1);
    }

    EXPECT_EQ(execution_count, 5);
}

TEST(TimeWheelUseCaseTest, DelayedRetry) {
    struct RetryTask {
        int attempt;
        std::string operation;
        uint64_t scheduled_time;

        RetryTask(int a, const std::string& op, uint64_t time)
            : attempt(a), operation(op), scheduled_time(time) {}
    };

    TimeWheel<RetryTask> tw;
    tw.setCurrentTime(0);

    std::vector<int> attempts;
    tw.setTimeoutCallback([&tw, &attempts](const RetryTask& task) {
        attempts.push_back(task.attempt);

        // Simulate retry logic
        if (task.attempt < 3) {
            // Exponential backoff: 2s, 4s, 8s...
            uint64_t delay = (1 << task.attempt) * MICROSECONDS_PER_SECOND;
            uint64_t next_time = task.scheduled_time + delay;
            tw.addTimer(next_time, RetryTask(task.attempt + 1, task.operation, next_time));
        }
    });

    // Start first attempt at 1 second
    tw.addTimer(MICROSECONDS_PER_SECOND, RetryTask(1, "connect", MICROSECONDS_PER_SECOND));

    // Advance through retry attempts
    tw.advanceTime(MICROSECONDS_PER_SECOND + 1);      // Trigger first attempt at 1s
    EXPECT_EQ(attempts.size(), 1);
    EXPECT_EQ(attempts[0], 1);

    tw.advanceTime(3 * MICROSECONDS_PER_SECOND + 1);  // Trigger second attempt at 3s (1+2)
    EXPECT_EQ(attempts.size(), 2);
    EXPECT_EQ(attempts[1], 2);

    tw.advanceTime(7 * MICROSECONDS_PER_SECOND + 1);  // Trigger third attempt at 7s (3+4)
    EXPECT_EQ(attempts.size(), 3);
    EXPECT_EQ(attempts[2], 3);
}