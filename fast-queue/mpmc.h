#pragma once

#include <atomic>
#include <cstddef>
#include <queue>
#include <thread>

constexpr std::size_t kHardwareDestructiveInterferenceSize = 64;

template <class T>
class MPMCBoundedQueue {
public:
    explicit MPMCBoundedQueue(size_t size) : queue_(std::vector<Node>(size)), max_size_{size} {
        for (size_t i = 0; i < max_size_; ++i) {
            queue_[i].generation = i;
        }
    }

    bool Enqueue(const T& value) {
        while (true) {
            size_t tail = tail_.load();
            if (head_.load() + max_size_ == tail) {
                return false;
            }
            Node& tail_node = queue_[tail % max_size_];
            if (tail == tail_node.generation.load() &&
                tail_.compare_exchange_weak(tail, tail + 1)) {
                tail_node.value = value;
                tail_node.generation.store(tail + 1);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }

    bool Dequeue(T& data) {
        while (true) {
            size_t head = head_.load();
            if (head == tail_.load()) {
                return false;
            }
            Node& head_node = queue_[head % max_size_];
            if (head + 1 == head_node.generation.load() &&
                head_.compare_exchange_weak(head, head + 1)) {
                data = head_node.value;
                head_node.generation.store(head + max_size_);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }

private:
    struct Node {
        alignas(kHardwareDestructiveInterferenceSize) std::atomic<size_t> generation;
        alignas(kHardwareDestructiveInterferenceSize) T value;
    };

    std::vector<Node> queue_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    const size_t max_size_;
};
