#pragma once

#include "../hazard-ptr/hazard_ptr.h"

#include <atomic>

template <class T>
class Stack {
public:
    Stack() : head_(nullptr) {
    }

    virtual ~Stack() {
        Clear();
    }

    void Push(const T& value) {
        Node* new_head = new Node(value);
        new_head->next = head_.load();

        while (!head_.compare_exchange_strong(new_head->next, new_head)) {
        };
    }

    bool Pop(T* value) {
        for (;;) {
            auto* old_head = Acquire(&head_);
            if (old_head == nullptr) {
                return false;
            }
            if (head_.compare_exchange_strong(old_head, old_head->next)) {
                *value = std::move(old_head->value);
                Retire(old_head);
                Release();
                return true;
            }
        }
    }

    void Clear() {
        Node* head = head_.exchange(nullptr);
        while (head != nullptr) {
            Node* old_head = head;
            head = old_head->next;
            Retire(old_head);
        }
    }

private:
    struct Node {
        T value;
        Node* next;
    };

    std::atomic<Node*> head_;
};
