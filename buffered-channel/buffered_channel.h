#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <stack>
#include <stdexcept>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(uint32_t size) : size_(size), is_closed_(false) {
    }

    void Send(const T& value) {
        T v(value);
        Send(std::move(v));
    }

    void Send(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        can_send_.wait(lock, [&]() { return is_closed_ || queue_.size() < size_; });
        if (is_closed_) {
            throw std::runtime_error("Канал закрыт");
        }

        queue_.push(std::move(value));
        can_recv_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> lock(mutex_);
        can_recv_.wait(lock, [&]() { return is_closed_ || !queue_.empty(); });
        if (is_closed_ && queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.top());
        queue_.pop();
        can_send_.notify_one();

        return value;
    }

    void Close() {
        std::lock_guard<std::mutex> guard(mutex_);
        is_closed_ = true;
        can_send_.notify_all();
        can_recv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable can_send_;
    std::condition_variable can_recv_;

    uint32_t size_;
    std::stack<T> queue_;
    bool is_closed_;
};
