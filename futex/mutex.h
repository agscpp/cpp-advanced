#pragma once

#include <cstdint>
#include <atomic>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>

// Atomically do the following:
//    if (*value == expected_value) {
//        sleep_on_address(value)
//    }
inline void FutexWait(int* value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value, nullptr, nullptr, 0);
}

// Wakeup 'count' threads sleeping on address of value (-1 wakes all)
inline void FutexWake(int* value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

class Mutex {
public:
    void Lock() {
        int32_t old_state = 0;
        if (state_.compare_exchange_strong(old_state, 1)) {
            return;
        }
        if (old_state != 2) {
            old_state = state_.exchange(2);
        }
        while (old_state != 0) {
            FutexWait(reinterpret_cast<int*>(&state_), 2);
            old_state = state_.exchange(2);
        }
    }

    void Unlock() {
        if (state_.fetch_sub(1) != 1) {
            state_.store(0);
            FutexWake(reinterpret_cast<int*>(&state_), 1);
        }
    }

private:
    std::atomic<int32_t> state_ = 0;
};
