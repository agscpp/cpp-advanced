#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <memory>
#include <unordered_set>

constexpr size_t kMaxFreeListSize = 1'000;

namespace {
thread_local std::atomic<void*> hazard_ptr{nullptr};

struct ThreadState {
    std::atomic<void*>* ptr;
};

struct RetiredPtr {
    void* value;
    std::function<void()> deleter;
    RetiredPtr* next;
};

std::mutex threads_lock;
std::unordered_set<ThreadState*> threads = {};

std::mutex scan_lock;
std::atomic_size_t approximate_free_list_size = 0;
std::atomic<RetiredPtr*> free_list = nullptr;

void AddRetiredQueue(RetiredPtr* new_head) {
    while (!free_list.compare_exchange_weak(new_head->next, new_head)) {
    }

    ++approximate_free_list_size;
}

void ScanFreeList() {
    std::lock_guard<std::mutex> guard(scan_lock);

    auto* retired = free_list.exchange(nullptr);
    approximate_free_list_size = 0;

    std::vector<void*> hazard;
    {
        std::lock_guard guard{threads_lock};
        for (auto* thread : threads) {
            if (auto* ptr = thread->ptr->load()) {
                hazard.push_back(ptr);
            }
        }
    }

    while (retired) {
        if (std::find(hazard.begin(), hazard.end(), retired->value) != hazard.end()) {
            auto* old_retired = retired;
            retired = old_retired->next;
            AddRetiredQueue(old_retired);
            continue;
        }

        auto* old_retired = retired;
        retired = old_retired->next;
        old_retired->deleter();
        delete old_retired;
    }
}

}  // namespace

void RegisterThread() {
    std::lock_guard<std::mutex> guard(threads_lock);
    auto* state_ptr = new ThreadState(&hazard_ptr);
    threads.insert(state_ptr);
}

void UnregisterThread() {
    std::unique_lock<std::mutex> lock(threads_lock);
    for (auto it = threads.begin(); it != threads.end(); ++it) {
        if ((*it)->ptr == &hazard_ptr) {
            delete (*it);
            threads.erase(it);
            break;
        }
    }

    if (threads.empty()) {
        lock.unlock();
        ScanFreeList();
    }
}

template <class T>
T* Acquire(std::atomic<T*>* ptr) {
    auto* value = ptr->load();

    do {
        hazard_ptr.store(value);

        auto* new_value = ptr->load();
        if (value == new_value) {
            return value;
        }

        value = new_value;
    } while (true);
}

inline void Release() {
    hazard_ptr.store(nullptr);
}

template <class T, class Deleter = std::default_delete<T>>
void Retire(T* value, Deleter deleter = {}) {
    AddRetiredQueue(
        new RetiredPtr{value, [value, deleter]() { deleter(value); }, free_list.load()});

    if (approximate_free_list_size > kMaxFreeListSize) {
        ScanFreeList();
    }
}
