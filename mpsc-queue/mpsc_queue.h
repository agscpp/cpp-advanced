#pragma once

#include <atomic>
#include <thread>
#include <unordered_map>

template <class T>
class StackEasy {
public:
    StackEasy() : head_(nullptr) {
    }

    virtual ~StackEasy() {
        DequeueAll([](T) {});
    }

    void Push(const T& value) {
        Node* new_head = new Node(value);
        new_head->next = head_.load();

        while (!head_.compare_exchange_strong(new_head->next, new_head)) {
        }
    }

    void DequeueAll(auto&& callback) {
        Node* head = head_.exchange(nullptr);
        while (head != nullptr) {
            Node* old_head = head;
            head = old_head->next;
            callback(old_head->value);
            delete old_head;
        }
    }

protected:
    struct Node {
        T value;
        Node* next;
    };

    std::atomic<Node*> head_;
};

template <class T>
class MPSCQueue : public StackEasy<T> {
private:
    using Node = StackEasy<T>::Node;

public:
    std::pair<T, bool> Pop() {
        for (;;) {
            Node* old_head = deleter_.ForbidDeletion(this->head_);
            if (old_head == nullptr) {
                return {{}, false};
            }
            if (this->head_.compare_exchange_strong(old_head, old_head->next)) {
                T value(old_head->value);
                deleter_.AllowDeletion();
                deleter_.QueueDeletion(old_head);
                return {value, true};
            }
            deleter_.AllowDeletion();
        }
    }

private:
    class NodeDeleter {
    public:
        void QueueDeletion(Node* node) {
            delete_queue_.Push(node);
            delete_queue_.DequeueAll([&](Node* node) {
                for (auto& [id, saved_node] : thread_nodes_) {
                    if (saved_node == node) {
                        delete_queue_.Push(node);
                        return;
                    }
                }
                delete node;
            });
        }

        void AllowDeletion() {
            thread_nodes_[std::this_thread::get_id()] = nullptr;
        }

        Node* ForbidDeletion(std::atomic<Node*>& node) {
            Node* old_node = node.load();
            do {
                thread_nodes_[std::this_thread::get_id()] = old_node;
            } while (!node.compare_exchange_strong(old_node, old_node));
            return old_node;
        }

    private:
        StackEasy<Node*> delete_queue_;
        std::unordered_map<std::thread::id, Node*> thread_nodes_;
    };

    NodeDeleter deleter_;
};
