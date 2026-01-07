#include <benchmark/benchmark.h>
#include "../../lib/time_wheel/time_wheel.h"
#include <random>
#include <vector>
#include <memory>

using namespace time_wheel;

// Benchmark fixture for TimeWheel
class TimeWheelBenchmark : public benchmark::Fixture {
protected:
    std::unique_ptr<TimeWheel<int>> tw;
    std::mt19937 gen{42};  // Fixed seed for reproducibility

    void SetUp(const ::benchmark::State& state) override {
        tw = std::make_unique<TimeWheel<int>>();
        tw->setCurrentTime(0);
        tw->setTimeoutCallback([](const int& item) {
            // No-op callback for benchmarking
        });
    }

    void TearDown(const ::benchmark::State& state) override {
        tw.reset();
    }
};

// ============= 添加定时器性能测试 =============

// Benchmark adding timers with uniform distribution
BENCHMARK_F(TimeWheelBenchmark, AddTimer_Uniform)(benchmark::State& state) {
    int64_t num_timers = state.range(0);
    std::uniform_int_distribution<uint64_t> dis(
        1 * MICROSECONDS_PER_SECOND,
        365 * MICROSECONDS_PER_DAY
    );

    for (auto _ : state) {
        for (int64_t i = 0; i < num_timers; ++i) {
            tw->addTimer(dis(gen), i);
        }
        state.PauseTiming();
        // Reset for next iteration
        tw = std::make_unique<TimeWheel<int>>();
        tw->setCurrentTime(0);
        tw->setTimeoutCallback([](const int& item) {});
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * num_timers);
}
BENCHMARK(TimeWheelBenchmark::AddTimer_Uniform)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// Benchmark adding timers to same level (Level 1 - seconds)
BENCHMARK_F(TimeWheelBenchmark, AddTimer_SameLevel_L1)(benchmark::State& state) {
    int64_t num_timers = state.range(0);
    std::uniform_int_distribution<uint64_t> dis(1, 59);  // Within 60 seconds

    for (auto _ : state) {
        for (int64_t i = 0; i < num_timers; ++i) {
            tw->addTimer(dis(gen) * MICROSECONDS_PER_SECOND, i);
        }
        state.PauseTiming();
        tw = std::make_unique<TimeWheel<int>>();
        tw->setCurrentTime(0);
        tw->setTimeoutCallback([](const int& item) {});
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * num_timers);
}
BENCHMARK(TimeWheelBenchmark::AddTimer_SameLevel_L1)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// Benchmark adding timers to different levels
BENCHMARK_F(TimeWheelBenchmark, AddTimer_MultiLevel)(benchmark::State& state) {
    int64_t num_timers = state.range(0);

    // Prepare timer values across different levels
    std::vector<uint64_t> timer_values;
    timer_values.reserve(num_timers);

    for (int64_t i = 0; i < num_timers; ++i) {
        uint64_t expiry_time = 0;
        switch (i % 5) {
            case 0: // Level 1 - seconds
                expiry_time = (i % 60 + 1) * MICROSECONDS_PER_SECOND;
                break;
            case 1: // Level 2 - minutes
                expiry_time = (i % 60 + 1) * MICROSECONDS_PER_MINUTE;
                break;
            case 2: // Level 3 - hours
                expiry_time = (i % 24 + 1) * MICROSECONDS_PER_HOUR;
                break;
            case 3: // Level 4 - days
                expiry_time = (i % 30 + 1) * MICROSECONDS_PER_DAY;
                break;
            case 4: // Level 5 - months
                expiry_time = (i % 12 + 1) * MICROSECONDS_PER_MONTH;
                break;
        }
        timer_values.push_back(expiry_time);
    }

    for (auto _ : state) {
        for (int64_t i = 0; i < num_timers; ++i) {
            tw->addTimer(timer_values[i], i);
        }
        state.PauseTiming();
        tw = std::make_unique<TimeWheel<int>>();
        tw->setCurrentTime(0);
        tw->setTimeoutCallback([](const int& item) {});
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations() * num_timers);
}
BENCHMARK(TimeWheelBenchmark::AddTimer_MultiLevel)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============= 时间推进性能测试 =============

// Benchmark advancing time with no timers
static void BM_AdvanceTime_Empty(benchmark::State& state) {
    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    uint64_t time_step = state.range(0) * MICROSECONDS_PER_SECOND;
    uint64_t current_time = 0;

    for (auto _ : state) {
        current_time += time_step;
        tw.advanceTime(current_time);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AdvanceTime_Empty)
    ->Arg(1)    // 1 second steps
    ->Arg(60)   // 1 minute steps
    ->Arg(3600) // 1 hour steps
    ->Arg(86400); // 1 day steps

// Benchmark advancing time with many timers
static void BM_AdvanceTime_WithTimers(benchmark::State& state) {
    int64_t num_timers = state.range(0);
    int64_t time_step = state.range(1) * MICROSECONDS_PER_SECOND;

    TimeWheel<int> tw;
    tw.setCurrentTime(0);

    int expired_count = 0;
    tw.setTimeoutCallback([&expired_count](const int& item) {
        expired_count++;
    });

    // Add timers distributed across time
    std::mt19937 gen(42);
    std::uniform_int_distribution<uint64_t> dis(
        1 * MICROSECONDS_PER_SECOND,
        365 * MICROSECONDS_PER_DAY
    );

    for (int64_t i = 0; i < num_timers; ++i) {
        tw.addTimer(dis(gen), i);
    }

    uint64_t current_time = 0;
    for (auto _ : state) {
        current_time += time_step;
        tw.advanceTime(current_time);

        if (current_time >= 365 * MICROSECONDS_PER_DAY) {
            state.PauseTiming();
            // Reset everything
            tw = TimeWheel<int>();
            tw.setCurrentTime(0);
            tw.setTimeoutCallback([&expired_count](const int& item) {
                expired_count++;
            });
            for (int64_t i = 0; i < num_timers; ++i) {
                tw.addTimer(dis(gen), i);
            }
            current_time = 0;
            expired_count = 0;
            state.ResumeTiming();
        }
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AdvanceTime_WithTimers)
    ->Args({1000, 1})    // 1000 timers, 1 second steps
    ->Args({10000, 1})   // 10000 timers, 1 second steps
    ->Args({1000, 60})   // 1000 timers, 1 minute steps
    ->Args({10000, 60})  // 10000 timers, 1 minute steps
    ->Args({1000, 3600}) // 1000 timers, 1 hour steps
    ->Args({10000, 3600}); // 10000 timers, 1 hour steps

// ============= 级联性能测试 =============

static void BM_Cascading_Performance(benchmark::State& state) {
    int64_t num_timers = state.range(0);

    for (auto _ : state) {
        TimeWheel<int> tw;
        tw.setCurrentTime(0);

        int expired_count = 0;
        tw.setTimeoutCallback([&expired_count](const int& item) {
            expired_count++;
        });

        // Add all timers to high level (will cascade down)
        uint64_t base_time = 30 * MICROSECONDS_PER_DAY;
        for (int64_t i = 0; i < num_timers; ++i) {
            // Add timers that will cascade from L4 to L1
            tw.addTimer(base_time + i * MICROSECONDS_PER_SECOND, i);
        }

        // Advance time to trigger cascading
        state.PauseTiming();
        tw.advanceTime(base_time - MICROSECONDS_PER_HOUR);
        state.ResumeTiming();

        // This advance will trigger cascading
        tw.advanceTime(base_time + num_timers * MICROSECONDS_PER_SECOND);
    }

    state.SetItemsProcessed(state.iterations() * num_timers);
}
BENCHMARK(BM_Cascading_Performance)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============= 内存使用测试 =============

static void BM_Memory_Usage(benchmark::State& state) {
    int64_t num_timers = state.range(0);

    for (auto _ : state) {
        auto tw = std::make_unique<TimeWheel<std::vector<char>>>();
        tw->setCurrentTime(0);

        tw->setTimeoutCallback([](const std::vector<char>& data) {
            // No-op
        });

        std::mt19937 gen(42);
        std::uniform_int_distribution<uint64_t> dis(
            1 * MICROSECONDS_PER_SECOND,
            365 * MICROSECONDS_PER_DAY
        );

        // Add timers with payload
        for (int64_t i = 0; i < num_timers; ++i) {
            std::vector<char> data(1024, 'A'); // 1KB payload
            tw->addTimer(dis(gen), std::move(data));
        }
    }

    state.SetItemsProcessed(state.iterations() * num_timers);
    state.SetBytesProcessed(state.iterations() * num_timers * 1024);
}
BENCHMARK(BM_Memory_Usage)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============= 自定义配置性能测试 =============

static void BM_CustomConfig_Performance(benchmark::State& state) {
    int64_t num_timers = state.range(0);

    // Create a more granular configuration
    std::vector<LevelConfig> custom_config = {
        {100000, 10},           // 100ms * 10 = 1 second
        {MICROSECONDS_PER_SECOND, 60},      // 1s * 60 = 1 minute
        {MICROSECONDS_PER_MINUTE, 60},      // 1min * 60 = 1 hour
        {MICROSECONDS_PER_HOUR, 24},        // 1h * 24 = 1 day
    };

    for (auto _ : state) {
        TimeWheel<int> tw(custom_config);
        tw.setCurrentTime(0);

        tw.setTimeoutCallback([](const int& item) {});

        std::mt19937 gen(42);
        std::uniform_int_distribution<uint64_t> dis(
            100000,  // 100ms
            24 * MICROSECONDS_PER_HOUR
        );

        for (int64_t i = 0; i < num_timers; ++i) {
            tw.addTimer(dis(gen), i);
        }
    }

    state.SetItemsProcessed(state.iterations() * num_timers);
}
BENCHMARK(BM_CustomConfig_Performance)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============= 实时模拟性能测试 =============

static void BM_RealTimeSimulation(benchmark::State& state) {
    int64_t num_events_per_second = state.range(0);

    TimeWheel<int> tw;
    uint64_t start_time = TimeWheel<int>::getCurrentTimeMicros();
    tw.setCurrentTime(start_time);

    int expired_count = 0;
    tw.setTimeoutCallback([&expired_count](const int& item) {
        expired_count++;
    });

    for (auto _ : state) {
        uint64_t current = TimeWheel<int>::getCurrentTimeMicros();

        // Add new timers for this iteration
        for (int64_t i = 0; i < num_events_per_second; ++i) {
            // Schedule events 1-10 seconds in the future
            uint64_t delay = ((i % 10) + 1) * MICROSECONDS_PER_SECOND;
            tw.addTimer(current + delay, i);
        }

        // Advance time
        tw.advanceTime(current);

        // Small delay to simulate real-time
        benchmark::DoNotOptimize(expired_count);
    }

    state.SetItemsProcessed(state.iterations() * num_events_per_second);
}
BENCHMARK(BM_RealTimeSimulation)
    ->Arg(10)    // 10 events/second
    ->Arg(100)   // 100 events/second
    ->Arg(1000)  // 1000 events/second
    ->Arg(10000); // 10000 events/second

// Main function for benchmark
BENCHMARK_MAIN();