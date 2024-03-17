#pragma once

#include <atomic>
#include <thread>

class SpinLock {
public:
    void Lock() {
        while (is_locked_.test_and_set()) {
            std::this_thread::yield();
        }
    }

    void Unlock() {
        is_locked_.clear();
    }

private:
    std::atomic_flag is_locked_;
};
