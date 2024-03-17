#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

class RWSpinLock {
public:
    void LockRead() {
        uint64_t state = state_.load();
        while ((state & 1) || !state_.compare_exchange_strong(state, (state + 1) << 1)) {
            std::this_thread::yield();
            state = state_.load();
        }
    }

    void UnlockRead() {
        uint64_t state = state_.load();
        while (!state_.compare_exchange_strong(state, (state >> 1) - 1)) {
        }
    }

    void LockWrite() {
        for (uint64_t state = 0; !state_.compare_exchange_strong(state, 1); state = 0) {
            std::this_thread::yield();
        }
    }

    void UnlockWrite() {
        state_.store(0);
    }

private:
    std::atomic_uint64_t state_;
};
