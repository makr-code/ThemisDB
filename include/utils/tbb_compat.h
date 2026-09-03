#pragma once

#if defined(__has_include) && __has_include(<tbb/concurrent_hash_map.h>)
#include <tbb/concurrent_hash_map.h>
#else
#include <mutex>
#include <unordered_map>
#include <utility>

namespace tbb {

/**
 * @brief Mutex-backed compatibility replacement for `tbb::concurrent_hash_map`.
 * @tparam Key Key type used for map lookup.
 * @tparam Value Value type stored in the map.
 */

template <typename Key, typename Value>
class concurrent_hash_map {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using iterator = typename std::unordered_map<Key, Value>::iterator;
    using const_iterator = typename std::unordered_map<Key, Value>::const_iterator;

    /**
     * @brief Mutable accessor for a locked map entry.
     */

    class accessor {
    public:
        accessor() = default;
        accessor(const accessor&) = delete;
        accessor& operator=(const accessor&) = delete;
        accessor(accessor&&) noexcept = default;
        accessor& operator=(accessor&&) noexcept = default;

        /**
         * @brief Binds this accessor to an entry in an owning map.
         * @param owner Owning map instance.
         * @param it Iterator to the entry.
         */
        void bind(concurrent_hash_map* owner, iterator it) {
            owner_ = owner;
            it_ = it;
        }

        /**
         * @brief Clears the accessor state.
         */
        void reset() {
            owner_ = nullptr;
            it_ = iterator{};
        }

        /**
         * @brief Releases this accessor.
         */
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

    /**
     * @brief Const accessor for a locked map entry.
     */

    class const_accessor {
    public:
        const_accessor() = default;
        const_accessor(const const_accessor&) = delete;
        const_accessor& operator=(const const_accessor&) = delete;
        const_accessor(const_accessor&&) noexcept = default;
        const_accessor& operator=(const_accessor&&) noexcept = default;

        /**
         * @brief Binds this accessor to a const entry in an owning map.
         * @param owner Owning map instance.
         * @param it Iterator to the entry.
         */
        void bind(concurrent_hash_map* owner, const_iterator it) {
            owner_ = owner;
            it_ = it;
        }

        /**
         * @brief Clears the accessor state.
         */
        void reset() {
            owner_ = nullptr;
            it_ = const_iterator{};
        }

        /**
         * @brief Releases this accessor.
         */
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

    /**
     * @brief Constructs an empty compatibility map.
     */
    concurrent_hash_map() = default;
    /**
     * @brief Destroys the compatibility map.
     */
    ~concurrent_hash_map() = default;

    /**
     * @brief Finds an entry by key and binds an accessor when found.
     * @tparam AccessorT Accessor type (`accessor` or `const_accessor`).
     * @param acc Accessor updated to reference the found entry.
     * @param key Key to search for.
     * @return true when the key exists, otherwise false.
     */
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

    /**
     * @brief Erases an entry by key.
     * @param key Key to erase.
     * @return true when an entry was erased, otherwise false.
     */
    bool erase(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.erase(key) > 0;
    }

    /**
     * @brief Erases an entry referenced by accessor.
     * @param acc Accessor bound to the entry to erase.
     * @return true when an entry was erased, otherwise false.
     */
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

    /**
     * @brief Inserts an entry and binds accessor to the inserted element.
     * @param acc Accessor that receives the inserted entry on success.
     * @param value Key/value pair to insert.
     * @return true when insertion happened, otherwise false.
     */
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

    /**
     * @brief Move-inserts an entry and binds accessor to the inserted element.
     * @param acc Accessor that receives the inserted entry on success.
     * @param value Key/value pair to insert.
     * @return true when insertion happened, otherwise false.
     */
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

    /**
     * @brief Inserts an entry without exposing an accessor.
     * @param value Key/value pair to insert.
     */
    void insert(const value_type& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.insert(value);
    }

    /**
     * @brief Move-inserts an entry without exposing an accessor.
     * @param value Key/value pair to insert.
     */
    void insert(value_type&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.insert(std::move(value));
    }

    /**
     * @brief Returns the number of stored entries.
     * @return Entry count.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    /**
     * @brief Removes all entries from the map.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }

    /**
     * @brief Returns iterator to the first element.
     * @return Mutable iterator.
     */
    iterator begin() { return map_.begin(); }
    /**
     * @brief Returns iterator to one-past-last element.
     * @return Mutable end iterator.
     */
    iterator end() { return map_.end(); }
    /**
     * @brief Returns const iterator to the first element.
     * @return Const iterator.
     */
    const_iterator begin() const { return map_.begin(); }
    /**
     * @brief Returns const iterator to one-past-last element.
     * @return Const end iterator.
     */
    const_iterator end() const { return map_.end(); }
    /**
     * @brief Returns const iterator to the first element.
     * @return Const iterator.
     */
    const_iterator cbegin() const { return map_.cbegin(); }
    /**
     * @brief Returns const iterator to one-past-last element.
     * @return Const end iterator.
     */
    const_iterator cend() const { return map_.cend(); }

private:
    mutable std::mutex mutex_;
    std::unordered_map<Key, Value> map_;
};

} // namespace tbb
#endif
