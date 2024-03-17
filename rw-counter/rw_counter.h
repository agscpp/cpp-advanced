#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <thread>

constexpr std::size_t kHardwareDestructiveInterferenceSize = 64;
constexpr std::size_t kMaxCounters = 32;

class Incrementer {
public:
    explicit Incrementer(std::atomic_int64_t* counter) : counter_(counter) {
    }

    void Increment() {
        counter_->fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic_int64_t* counter_;
};

class ReadWriteAtomicCounter {
public:
    std::unique_ptr<Incrementer> GetIncrementer() {
        return std::make_unique<Incrementer>(&counters_[thread_counter_++ % kMaxCounters].value);
    }

    int64_t GetValue() {
        while (is_locked_.test_and_set()) {
            std::this_thread::yield();
        }
        int64_t result = 0;
        for (auto& counter : counters_) {
            result += counter.value.load();
        }
        is_locked_.clear();
        return result;
    }

private:
    struct AlignedInt {
        alignas(kHardwareDestructiveInterferenceSize) std::atomic_int64_t value;
    };

    std::atomic_int64_t thread_counter_;
    std::array<AlignedInt, kMaxCounters> counters_;
    std::atomic_flag is_locked_;
};
