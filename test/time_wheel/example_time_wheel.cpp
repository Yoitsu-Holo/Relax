#include "../../lib/time_wheel/time_wheel.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

using namespace time_wheel;

// Simple task scheduler example using TimeWheel
class SimpleScheduler {
private:
    struct Task {
        std::string name;
        std::function<void()> callback;

        Task(const std::string& n, std::function<void()> cb)
            : name(n), callback(cb) {}
    };

    TimeWheel<std::shared_ptr<Task>> time_wheel_;
    std::atomic<bool> running_;
    std::thread worker_thread_;

public:
    SimpleScheduler() : running_(false) {
        time_wheel_.setTimeoutCallback([](const std::shared_ptr<Task>& task) {
            std::cout << "[" << std::chrono::system_clock::now().time_since_epoch().count()
                      << "] Executing: " << task->name << std::endl;
            if (task->callback) {
                task->callback();
            }
        });
    }

    ~SimpleScheduler() {
        stop();
    }

    void start() {
        if (running_.exchange(true)) return;

        uint64_t current = TimeWheel<int>::getCurrentTimeMicros();
        time_wheel_.setCurrentTime(current);

        worker_thread_ = std::thread([this]() {
            while (running_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                uint64_t current = TimeWheel<int>::getCurrentTimeMicros();
                time_wheel_.advanceTime(current);
            }
        });

        std::cout << "Scheduler started" << std::endl;
    }

    void stop() {
        if (!running_.exchange(false)) return;

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        std::cout << "Scheduler stopped" << std::endl;
    }

    void schedule(const std::string& name, uint64_t delay_micros, std::function<void()> callback) {
        uint64_t current = TimeWheel<int>::getCurrentTimeMicros();
        auto task = std::make_shared<Task>(name, callback);
        time_wheel_.addTimer(current + delay_micros, task);
        std::cout << "Scheduled task: " << name << " (delay: "
                  << delay_micros / 1000000.0 << "s)" << std::endl;
    }
};

int main() {
    std::cout << "=== Time Wheel Example ===" << std::endl;

    SimpleScheduler scheduler;
    scheduler.start();

    // Schedule some tasks
    scheduler.schedule("Task 1", 1 * MICROSECONDS_PER_SECOND, []() {
        std::cout << "  Task 1 completed!" << std::endl;
    });

    scheduler.schedule("Task 2", 3 * MICROSECONDS_PER_SECOND, []() {
        std::cout << "  Task 2 completed!" << std::endl;
    });

    scheduler.schedule("Task 3", 5 * MICROSECONDS_PER_SECOND, []() {
        std::cout << "  Task 3 completed!" << std::endl;
    });

    scheduler.schedule("Task 4", 2 * MICROSECONDS_PER_SECOND, []() {
        std::cout << "  Task 4 completed!" << std::endl;
    });

    // Run for 7 seconds
    std::cout << "\nRunning for 7 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(7));

    scheduler.stop();
    std::cout << "\nExample completed!" << std::endl;

    return 0;
}