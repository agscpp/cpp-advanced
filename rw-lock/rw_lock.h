#pragma once

#include <condition_variable>
#include <mutex>

class RWLock {
public:
    void Read(auto func) {
        std::unique_lock lock(global_);
        ++blocked_readers_;
        lock.unlock();
        try {
            func();
        } catch (...) {
            EndRead();
            throw;
        }
        EndRead();
    }

    void Write(auto func) {
        std::unique_lock lock{global_};
        can_write_.wait(lock, [&]() { return blocked_readers_ == 0; });
        func();
        can_write_.notify_one();
    }

private:
    void EndRead() {
        std::unique_lock lock{global_};
        if (--blocked_readers_ == 0) {
            can_write_.notify_one();
        }
    }

    std::condition_variable can_write_;
    std::mutex global_;
    int blocked_readers_ = 0;
};
