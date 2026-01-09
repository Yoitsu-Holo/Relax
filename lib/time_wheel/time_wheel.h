#ifndef TIME_WHEEL_H
#define TIME_WHEEL_H

#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <queue>
#include <algorithm>

namespace time_wheel {

// Time unit constants (in microseconds)
constexpr uint64_t MICROSECONDS_PER_SECOND = 1000000;
constexpr uint64_t MICROSECONDS_PER_MINUTE = 60 * MICROSECONDS_PER_SECOND;
constexpr uint64_t MICROSECONDS_PER_HOUR = 60 * MICROSECONDS_PER_MINUTE;
constexpr uint64_t MICROSECONDS_PER_DAY = 24 * MICROSECONDS_PER_HOUR;
constexpr uint64_t MICROSECONDS_PER_MONTH = 30 * MICROSECONDS_PER_DAY;

// Configuration for time wheel levels
struct LevelConfig {
    uint64_t time_slice;    // Time per slot in microseconds
    size_t bucket_count;     // Number of buckets in this level

    LevelConfig(uint64_t slice, size_t count)
        : time_slice(slice), bucket_count(count) {}
};

// Default configuration for 5-level time wheel
// L1: 1s * 60, L2: 1min * 60, L3: 1h * 24, L4: 1d * 30, L5: 1month * 12
inline std::vector<LevelConfig> getDefaultConfig() {
    return {
        {MICROSECONDS_PER_SECOND, 60},     // Level 1: seconds
        {MICROSECONDS_PER_MINUTE, 60},     // Level 2: minutes
        {MICROSECONDS_PER_HOUR, 24},       // Level 3: hours
        {MICROSECONDS_PER_DAY, 30},        // Level 4: days
        {MICROSECONDS_PER_MONTH, 12}       // Level 5: months
    };
}

// Timer entry that stores expiry time and user data
template<typename T>
struct TimerEntry {
    uint64_t expiry_time;   // Expiry time in microseconds
    T data;                 // User data

    TimerEntry(uint64_t time, T&& d)
        : expiry_time(time), data(std::forward<T>(d)) {}

    TimerEntry(uint64_t time, const T& d)
        : expiry_time(time), data(d) {}
};

// Single level of time wheel
template<typename T>
class TimeWheelLevel {
private:
    using EntryPtr = std::shared_ptr<TimerEntry<T>>;
    using Bucket = std::queue<EntryPtr>;

    std::vector<Bucket> buckets_;
    LevelConfig config_;
    uint64_t current_time_;
    size_t current_index_;

public:
    TimeWheelLevel(const LevelConfig& config)
        : config_(config),
          buckets_(config.bucket_count),
          current_time_(0),
          current_index_(0) {}

    // Add timer entry to appropriate bucket
    void add(EntryPtr entry) {
        uint64_t time_diff = entry->expiry_time - current_time_;

        // Calculate which bucket this entry belongs to
        size_t bucket_offset = time_diff / config_.time_slice;

        if (bucket_offset >= config_.bucket_count) {
            // This should be handled by a higher level
            bucket_offset = config_.bucket_count - 1;
        }

        size_t bucket_index = (current_index_ + bucket_offset) % config_.bucket_count;
        buckets_[bucket_index].push(entry);
    }

    // Advance time and return expired entries
    std::vector<EntryPtr> advance(uint64_t new_time) {
        std::vector<EntryPtr> expired;

        while (current_time_ + config_.time_slice <= new_time) {
            // Advance time first
            current_time_ += config_.time_slice;
            current_index_ = (current_index_ + 1) % config_.bucket_count;

            // Collect all entries from current bucket
            // They need to be cascaded to lower level or triggered
            Bucket& bucket = buckets_[current_index_];
            while (!bucket.empty()) {
                expired.push_back(bucket.front());
                bucket.pop();
            }
        }

        return expired;
    }

    // Check if an entry belongs to this level
    bool canHandle(uint64_t expiry_time) const {
        uint64_t time_diff = expiry_time - current_time_;
        return time_diff < (config_.bucket_count * config_.time_slice);
    }

    uint64_t getCurrentTime() const { return current_time_; }
    void setCurrentTime(uint64_t time) {
        current_time_ = time;
        current_index_ = 0;
    }

    const LevelConfig& getConfig() const { return config_; }
};

// Multi-level time wheel manager
template<typename T>
class TimeWheel {
private:
    using EntryPtr = std::shared_ptr<TimerEntry<T>>;
    using CallbackFunc = std::function<void(const T&)>;

    std::vector<std::unique_ptr<TimeWheelLevel<T>>> levels_;
    CallbackFunc timeout_callback_;
    uint64_t current_time_;

    // Cascade entries from higher level to lower level
    void cascade(int from_level, const std::vector<EntryPtr>& entries) {
        if (from_level == 0) {
            // Level 0 entries have expired, invoke callback
            for (const auto& entry : entries) {
                if (entry->expiry_time <= current_time_ && timeout_callback_) {
                    timeout_callback_(entry->data);
                }
            }
        } else {
            // Try to add to lower level
            int target_level = from_level - 1;
            auto& level = levels_[target_level];

            for (const auto& entry : entries) {
                if (entry->expiry_time <= current_time_) {
                    // Already expired, cascade immediately
                    cascade(target_level, {entry});
                } else if (level->canHandle(entry->expiry_time)) {
                    level->add(entry);
                } else {
                    // Still too far in future for this level
                    // This shouldn't happen in normal operation
                    levels_[from_level]->add(entry);
                }
            }
        }
    }

public:
    TimeWheel(const std::vector<LevelConfig>& configs = getDefaultConfig())
        : current_time_(0) {
        for (const auto& config : configs) {
            levels_.emplace_back(std::make_unique<TimeWheelLevel<T>>(config));
        }
    }

    // Set timeout callback
    void setTimeoutCallback(CallbackFunc callback) {
        timeout_callback_ = callback;
    }

    // Add a timer
    void addTimer(uint64_t expiry_time, T data) {
        if (expiry_time <= current_time_) {
            // Already expired
            if (timeout_callback_) {
                timeout_callback_(data);
            }
            return;
        }

        auto entry = std::make_shared<TimerEntry<T>>(expiry_time, std::move(data));

        // Find appropriate level for this timer
        for (size_t i = 0; i < levels_.size(); ++i) {
            if (levels_[i]->canHandle(expiry_time)) {
                levels_[i]->add(entry);
                return;
            }
        }

        // If no level can handle, add to highest level
        levels_.back()->add(entry);
    }

    // Advance time to new_time and process all expired timers
    void advanceTime(uint64_t new_time) {
        if (new_time <= current_time_) {
            return;  // Time cannot go backward
        }

        current_time_ = new_time;

        // Process from highest to lowest level
        for (int i = levels_.size() - 1; i >= 0; --i) {
            auto expired = levels_[i]->advance(new_time);
            if (!expired.empty()) {
                cascade(i, expired);
            }
        }
    }

    // Get current time
    uint64_t getCurrentTime() const {
        return current_time_;
    }

    // Set initial time (should be called before any operations)
    void setCurrentTime(uint64_t time) {
        current_time_ = time;
        for (auto& level : levels_) {
            level->setCurrentTime(time);
        }
    }

    // Helper function to get current time in microseconds
    static uint64_t getCurrentTimeMicros() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
};

} // namespace time_wheel

#endif // TIME_WHEEL_H