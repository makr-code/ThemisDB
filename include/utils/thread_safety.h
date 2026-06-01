/*
 * ThemisDB | File: thread_safety.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <optional>
#include <functional>
#include <utility>  // for std::declval

/**
 * @file thread_safety.h
 * @brief Thread-safety utilities for ThemisDB
 * 
 * This header provides utility classes and patterns for thread-safe code:
 * - Synchronized<T>: RAII wrapper for mutex-protected access
 * - SharedSynchronized<T>: RAII wrapper for reader-writer locks
 * 
 * Usage Examples:
 * 
 * 1. Exclusive access with Synchronized:
 *    ```cpp
 *    Synchronized<std::vector<Task>> queue;
 *    
 *    // Thread-safe modify
 *    queue.with_lock([](auto& q) {
 *        q.push_back(task);
 *    });
 *    
 *    // Thread-safe read
 *    auto size = queue.with_lock([](const auto& q) {
 *        return q.size();
 *    });
 *    ```
 * 
 * 2. Reader-writer access with SharedSynchronized:
 *    ```cpp
 *    SharedSynchronized<std::unordered_map<std::string, Value>> cache;
 *    
 *    // Thread-safe read (shared lock - multiple readers allowed)
 *    auto value = cache.with_shared_lock([&](const auto& c) {
 *        auto it = c.find(key);
 *        return it != c.end() ? std::optional(it->second) : std::nullopt;
 *    });
 *    
 *    // Thread-safe write (exclusive lock - only one writer)
 *    cache.with_unique_lock([&](auto& c) {
 *        c[key] = value;
 *    });
 *    ```
 */

namespace themis {
namespace utils {
namespace threading {

/**
 * @brief RAII wrapper for thread-safe access with exclusive locking
 * 
 * Provides thread-safe access to a value using a mutex. All accesses
 * are protected by the mutex, preventing data races.
 * 
 * @tparam T The type of value to protect
 */
template<typename T>
class Synchronized {
public:
    /**
     * @brief Construct with forwarded arguments to T's constructor
     */
    template<typename... Args>
    explicit Synchronized(Args&&... args) 
        : value_(std::forward<Args>(args)...) {}
    
    /**
     * @brief Thread-safe read access (const)
     * 
     * Executes the provided function with the protected value under lock.
     * The lock is held for the duration of the function execution.
     * 
     * @param func Function to execute with const reference to value
     * @return Return value of func
     */
    template<typename Func>
    auto with_lock(Func&& func) const -> decltype(func(std::declval<const T&>())) {
        std::lock_guard<std::mutex> lock(mutex_);
        return func(value_);
    }
    
    /**
     * @brief Thread-safe modify access (non-const)
     * 
     * Executes the provided function with the protected value under lock.
     * The lock is held for the duration of the function execution.
     * 
     * @param func Function to execute with mutable reference to value
     * @return Return value of func
     */
    template<typename Func>
    auto with_lock(Func&& func) -> decltype(func(std::declval<T&>())) {
        std::lock_guard<std::mutex> lock(mutex_);
        return func(value_);
    }
    
private:
    mutable std::mutex mutex_;
    T value_;
};

/**
 * @brief RAII wrapper for thread-safe access with reader-writer locking
 * 
 * Provides thread-safe access to a value using a shared_mutex (reader-writer lock).
 * Multiple readers can access simultaneously, but writes are exclusive.
 * 
 * Use this for data structures with frequent reads and infrequent writes
 * to improve concurrency performance.
 * 
 * @tparam T The type of value to protect
 */
template<typename T>
class SharedSynchronized {
public:
    /**
     * @brief Construct with forwarded arguments to T's constructor
     */
    template<typename... Args>
    explicit SharedSynchronized(Args&&... args)
        : value_(std::forward<Args>(args)...) {}
    
    /**
     * @brief Thread-safe read access with shared lock
     * 
     * Executes the provided function with the protected value under shared lock.
     * Multiple threads can hold shared locks simultaneously for reading.
     * 
     * @param func Function to execute with const reference to value
     * @return Return value of func
     */
    template<typename Func>
    auto with_shared_lock(Func&& func) const -> decltype(func(std::declval<const T&>())) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return func(value_);
    }
    
    /**
     * @brief Thread-safe modify access with unique lock
     * 
     * Executes the provided function with the protected value under unique lock.
     * Only one thread can hold a unique lock, and no shared locks are allowed.
     * 
     * @param func Function to execute with mutable reference to value
     * @return Return value of func
     */
    template<typename Func>
    auto with_unique_lock(Func&& func) -> decltype(func(std::declval<T&>())) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return func(value_);
    }
    
private:
    mutable std::shared_mutex mutex_;
    T value_;
};

/**
 * @brief Helper to check if a type is thread-safe
 * 
 * Use this trait to check if a type has thread-safe operations.
 * By default, only std::atomic types are considered thread-safe.
 */
template<typename T>
struct is_thread_safe : std::false_type {};

template<typename T>
struct is_thread_safe<std::atomic<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_thread_safe_v = is_thread_safe<T>::value;

} // namespace threading
} // namespace utils
} // namespace themis
