#pragma once

#if defined(__has_include) && __has_include(<tbb/concurrent_hash_map.h>)
#include <tbb/concurrent_hash_map.h>
#else
#include <mutex>
#include <unordered_map>
#include <utility>

namespace tbb {

template <typename Key, typename Value>
class concurrent_hash_map {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using iterator = typename std::unordered_map<Key, Value>::iterator;
    using const_iterator = typename std::unordered_map<Key, Value>::const_iterator;

    class accessor {
    public:
        accessor() = default;
        accessor(const accessor&) = delete;
        accessor& operator=(const accessor&) = delete;
        accessor(accessor&&) noexcept = default;
        accessor& operator=(accessor&&) noexcept = default;

        void bind(concurrent_hash_map* owner, iterator it) {
            owner_ = owner;
            it_ = it;
        }

        void reset() {
            owner_ = nullptr;
            it_ = iterator{};
        }

        void release() {
            reset();
        }

        value_type* operator->() {
            return &(*it_);
        }

        const value_type* operator->() const {
            return &(*it_);
        }

    private:
        concurrent_hash_map* owner_ = nullptr;
        iterator it_{};
        friend class concurrent_hash_map;
    };

    class const_accessor {
    public:
        const_accessor() = default;
        const_accessor(const const_accessor&) = delete;
        const_accessor& operator=(const const_accessor&) = delete;
        const_accessor(const_accessor&&) noexcept = default;
        const_accessor& operator=(const_accessor&&) noexcept = default;

        void bind(concurrent_hash_map* owner, const_iterator it) {
            owner_ = owner;
            it_ = it;
        }

        void reset() {
            owner_ = nullptr;
            it_ = const_iterator{};
        }

        void release() {
            reset();
        }

        const value_type* operator->() const {
            return &(*it_);
        }

    private:
        concurrent_hash_map* owner_ = nullptr;
        const_iterator it_{};
        friend class concurrent_hash_map;
    };

    concurrent_hash_map() = default;
    ~concurrent_hash_map() = default;

    template <typename AccessorT>
    bool find(AccessorT& acc, const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            acc.reset();
            return false;
        }
        acc.bind(this, it);
        return true;
    }

    bool erase(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.erase(key) > 0;
    }

    bool erase(accessor& acc) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (acc.owner_ != this || acc.it_ == iterator{}) {
            return false;
        }
        auto it = acc.it_;
        acc.reset();
        map_.erase(it);
        return true;
    }

    bool insert(accessor& acc, const value_type& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto inserted = map_.insert(value);
        if (!inserted.second) {
            acc.reset();
            return false;
        }
        acc.bind(this, inserted.first);
        return true;
    }

    bool insert(accessor& acc, value_type&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto inserted = map_.insert(std::move(value));
        if (!inserted.second) {
            acc.reset();
            return false;
        }
        acc.bind(this, inserted.first);
        return true;
    }

    void insert(const value_type& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.insert(value);
    }

    void insert(value_type&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.insert(std::move(value));
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }

    iterator begin() { return map_.begin(); }
    iterator end() { return map_.end(); }
    const_iterator begin() const { return map_.begin(); }
    const_iterator end() const { return map_.end(); }
    const_iterator cbegin() const { return map_.cbegin(); }
    const_iterator cend() const { return map_.cend(); }

private:
    mutable std::mutex mutex_;
    std::unordered_map<Key, Value> map_;
};

} // namespace tbb
#endif
