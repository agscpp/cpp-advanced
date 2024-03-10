#pragma once

#include <cstddef>
#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    explicit Semaphore(int count) : count_{count} {
    }

    void Acquire(auto callback) {
        std::unique_lock lock{mutex_};
        size_t position(queue_tail_++);
        cv_.wait(lock, [this, position] { return count_ && queue_head_ == position; });
        ++queue_head_;
        callback(count_);
    }

    void Acquire() {
        Acquire([](int& value) { --value; });
    }

    void Release() {
        std::lock_guard lock{mutex_};
        ++count_;
        cv_.notify_all();
    }

private:
    int count_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t queue_head_ = 0;
    size_t queue_tail_ = 0;
};
