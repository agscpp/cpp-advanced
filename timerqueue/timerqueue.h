#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <set>

template <class T>
class TimerQueue {
public:
    template <class... Args>
    void Add(std::chrono::system_clock::time_point at, Args&&... args) {
        std::unique_lock lock(mutex_);
        queue_.emplace(at, T{std::forward<Args>(args)...});
        cv_.notify_one();
    }

    T Pop() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&]() { return !queue_.empty(); });

        std::chrono::system_clock::time_point time;
        do {
            time = queue_.begin()->time;
            cv_.wait_until(lock, time);
        } while (time != queue_.begin()->time || time > std::chrono::system_clock::now());

        auto result = std::move(queue_.extract(queue_.begin()).value().value);
        cv_.notify_one();
        return result;
    }

private:
    struct Item {
        std::chrono::system_clock::time_point time;
        T value;

        bool operator<(const Item& rhs) const {
            return this->time < rhs.time;
        }
    };

    std::mutex mutex_;
    std::condition_variable cv_;

    std::set<Item> queue_;
};
