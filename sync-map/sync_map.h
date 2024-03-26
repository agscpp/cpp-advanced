#pragma once

#include "../hazard-ptr/hazard_ptr.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

constexpr size_t kMaxOperation = 10;

template <class K, class V>
class SyncMap {
public:
    SyncMap()
        : snapshot_(new Snapshot(std::make_shared<std::unordered_map<K, V>>(), false)),
          mutable_map_(std::make_shared<std::unordered_map<K, V>>()),
          operation_count_(0) {
    }

    ~SyncMap() {
        auto* snapshot = snapshot_.exchange(nullptr);
        delete snapshot;
    }

    bool Lookup(const K& key, V* value) {
        auto is_found = false;
        auto* snapshot = Acquire(&snapshot_);

        if (snapshot->dirty) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (auto it = mutable_map_->find(key); it != mutable_map_->end()) {
                *value = it->second;
                is_found = true;
            }
            ++operation_count_;
        }
        if (!is_found) {
            if (auto it = snapshot->read_only->find(key); it != snapshot->read_only->end()) {
                *value = it->second;
                is_found = true;
            }
        } else {
            UpdateSnapshotIfNeeded();
        }

        Release();
        return is_found;
    }

    bool Insert(const K& key, const V& value) {
        auto is_added = false;
        auto* snapshot = Acquire(&snapshot_);
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (!mutable_map_->contains(key) && !snapshot->read_only->contains(key)) {
                if (!snapshot->dirty) {
                    snapshot_.store(new Snapshot(snapshot->read_only, true));
                    Retire(snapshot);
                }
                mutable_map_->insert(std::pair(key, value));
                is_added = true;
            }
        }
        Release();
        return is_added;
    }

private:
    struct Snapshot {
        const std::shared_ptr<const std::unordered_map<K, V>> read_only;
        const bool dirty;
    };

    std::atomic<Snapshot*> snapshot_;

    std::shared_ptr<std::unordered_map<K, V>> mutable_map_;
    size_t operation_count_;
    std::mutex mutex_;

    void UpdateSnapshotIfNeeded() {
        std::lock_guard<std::mutex> guard(mutex_);
        if (operation_count_ > kMaxOperation) {
            operation_count_ = 0;

            auto* old_snapshot = Acquire(&snapshot_);
            std::shared_ptr<std::unordered_map<K, V>> data =
                std::const_pointer_cast<std::unordered_map<K, V>>(old_snapshot->read_only);

            for (auto it = mutable_map_->begin(); it != mutable_map_->end(); ++it) {
                data->insert(std::move(*it));
            }
            mutable_map_->clear();

            snapshot_.store(new Snapshot(data, false));
            Retire(old_snapshot);
            Release();
        }
    }
};
