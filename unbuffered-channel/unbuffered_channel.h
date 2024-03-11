#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>

template <class T>
class UnbufferedChannel {
public:
    void Send(const T& value) {
        T v(value);
        Send(std::move(v));
    }

    void Send(T&& value) {
        std::lock_guard<std::mutex> guard(send_mutex_);
        std::unique_lock<std::mutex> lock(global_);
        can_send_.wait(lock, [&]() { return is_closed_ || (is_value_required_ && !has_value_); });
        if (is_closed_) {
            throw std::runtime_error("Канал закрыт");
        }
        has_value_ = true;
        value_ = std::move(value);
        can_recv_.notify_one();
    }

    std::optional<T> Recv() {
        std::lock_guard<std::mutex> guard(read_mutex_);
        std::unique_lock<std::mutex> lock(global_);
        is_value_required_ = true;
        can_send_.notify_one();
        can_recv_.wait(lock, [&]() { return is_closed_ || has_value_; });
        if (is_closed_ && !has_value_) {
            return std::nullopt;
        }
        is_value_required_ = false;
        has_value_ = false;
        can_send_.notify_one();
        return std::move(value_);
    }

    void Close() {
        std::lock_guard<std::mutex> guard(global_);
        is_closed_ = true;
        can_send_.notify_all();
        can_recv_.notify_all();
    }

private:
    std::mutex global_;
    std::mutex read_mutex_;
    std::mutex send_mutex_;
    std::condition_variable can_send_;
    std::condition_variable can_recv_;

    T value_;
    bool is_closed_ = false;
    bool has_value_ = false;
    bool is_value_required_ = false;
};
