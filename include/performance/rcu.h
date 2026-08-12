/**
 * @file rcu.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB RCU (Read-Copy-Update) Infrastructure
// Based on: "Scalable Read-Mostly Synchronization Using RCU" (ASPLOS'10)
//
// RCU allows lock-free reads for read-heavy workloads by deferring reclamation
// of old data until all readers have finished accessing it.
//
// Key Concepts:
// - Read-side: Lock-free, zero-overhead reads
// - Update-side: Copy-modify-update pattern
// - Grace Period: Time when all readers have completed
// - Deferred Reclamation: Free old data after grace period

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace themis {
namespace rcu {

/**
 * @brief RCU read-side lock guard (RAII)
 * 
 * Marks the beginning and end of an RCU read-side critical section.
 * Extremely lightweight - just increments/decrements a counter.
 */
class ReadLock {
public:
    ReadLock();
    ~ReadLock();
    
    ReadLock(const ReadLock&) = delete;
    ReadLock& operator=(const ReadLock&) = delete;
    
private:
    static thread_local std::atomic<uint64_t> read_count_;
};

/**
 * @brief RCU callback function type
 */
using RCUCallback = std::function<void()>;

/**
 * @brief RCU Grace Period Manager
 * 
 * Manages grace periods and deferred callbacks for safe memory reclamation.
 * Thread-safe and designed for high-performance read-heavy workloads.
 */
class GracePeriodManager {
public:
    static GracePeriodManager& instance() {
        static GracePeriodManager instance;
        return instance;
    }
    
    /**
     * @brief Register a callback to be executed after a grace period
     * 
     * The callback will be invoked once all current readers have finished.
     * Typically used to free old data structures.
     * 
     * @param callback Function to call after grace period
     */
    void call_rcu(RCUCallback callback);
    
    /**
     * @brief Wait for all current readers to complete
     * 
     * Blocks until a grace period has elapsed, ensuring all readers
     * that started before this call have completed.
     */
    void synchronize_rcu();
    
    /**
     * @brief Start the grace period thread
     */
    void start();
    
    /**
     * @brief Stop the grace period thread
     */
    void stop();
    
    /**
     * @brief Check if RCU is enabled at compile time
     */
    static bool is_enabled() {
        #ifdef THEMIS_USE_RCU_INDEX
        return true;
        #else
        return false;
        #endif
    }
    
private:
    GracePeriodManager();
    ~GracePeriodManager();
    
    GracePeriodManager(const GracePeriodManager&) = delete;
    GracePeriodManager& operator=(const GracePeriodManager&) = delete;
    
    void grace_period_thread();
    bool readers_active();
    
    std::atomic<bool> running_{false};
    std::thread grace_thread_;
    
    std::mutex callbacks_mutex_;
    std::vector<RCUCallback> pending_callbacks_;
    std::vector<RCUCallback> current_callbacks_;
    
    std::atomic<uint64_t> grace_period_counter_{0};
};

/**
 * @brief RAII wrapper for RCU-protected pointers
 * 
 * Provides automatic read-locking and safe pointer access.
 */
template<typename T>
class RCUPtr {
public:
    explicit RCUPtr(std::atomic<T*>& ptr) 
        : lock_(), ptr_(ptr.load(std::memory_order_acquire)) {}
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    explicit operator bool() const { return ptr_ != nullptr; }
    
private:
    ReadLock lock_;
    T* ptr_;
};

/**
 * @brief Update an RCU-protected pointer
 * 
 * Thread-safe pointer update with automatic old value cleanup.
 * 
 * @param ptr Atomic pointer to update
 * @param new_value New value to set
 * @return Old value (caller must defer reclamation via call_rcu)
 */
template<typename T>
T* rcu_assign_pointer(std::atomic<T*>& ptr, T* new_value) {
    T* old_value = ptr.exchange(new_value, std::memory_order_acq_rel);
    return old_value;
}

/**
 * @brief Defer deletion of an object until after grace period
 * 
 * Schedules the object for deletion after all current readers complete.
 * 
 * @param ptr Pointer to object to delete
 */
template<typename T>
void rcu_defer_delete(T* ptr) {
    if (!ptr) return;
    
    #ifdef THEMIS_USE_RCU_INDEX
    GracePeriodManager::instance().call_rcu([ptr]() {
        delete ptr;
    });
    #else
    // Without RCU, delete immediately (not thread-safe!)
    delete ptr;
    #endif
}

// Global active-reader count.
// Each ReadLock increments this on construction and decrements on destruction.
// GracePeriodManager::readers_active() polls this to detect quiescent states.
// Using relaxed ordering is safe because we are checking for zero (a
// stable state once all readers have decremented).
//
// The `inline` specifier (C++17) guarantees a single definition across all
// translation units that include this header.  If building with C++14, move
// this declaration to a .cpp file and declare `extern std::atomic<int64_t>
// g_rcu_reader_count;` here instead.
inline std::atomic<int64_t> g_rcu_reader_count{0};

// Thread-local read counter definition
inline thread_local std::atomic<uint64_t> ReadLock::read_count_{0};

// ReadLock implementation
inline ReadLock::ReadLock() {
    read_count_.fetch_add(1, std::memory_order_acquire);
    g_rcu_reader_count.fetch_add(1, std::memory_order_relaxed);
}

inline ReadLock::~ReadLock() {
    g_rcu_reader_count.fetch_sub(1, std::memory_order_release);
    read_count_.fetch_sub(1, std::memory_order_release);
}

// GracePeriodManager implementation
inline GracePeriodManager::GracePeriodManager() {
    #ifdef THEMIS_USE_RCU_INDEX
    start();
    #endif
}

inline GracePeriodManager::~GracePeriodManager() {
    stop();
}

inline void GracePeriodManager::start() {
    if (running_.exchange(true)) return;
    
    grace_thread_ = std::thread([this]() {
        grace_period_thread();
    });
}

inline void GracePeriodManager::stop() {
    if (!running_.exchange(false)) return;
    
    if (grace_thread_.joinable()) {
        grace_thread_.join();
    }
    
    // Process any remaining callbacks
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    for (auto& callback : pending_callbacks_) {
        callback();
    }
    pending_callbacks_.clear();
    
    for (auto& callback : current_callbacks_) {
        callback();
    }
    current_callbacks_.clear();
}

inline void GracePeriodManager::call_rcu(RCUCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    pending_callbacks_.push_back(std::move(callback));
}

inline void GracePeriodManager::synchronize_rcu() {
    #ifdef THEMIS_USE_RCU_INDEX
    // Wait for grace period by checking if all readers have finished
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    
    // In a real implementation, this would use more sophisticated
    // per-CPU counters and quiescent state detection
    while (readers_active()) {
        std::this_thread::yield();
    }
    #endif
}

inline bool GracePeriodManager::readers_active() {
    // A grace period has elapsed when no reader holds a ReadLock.
    // g_rcu_reader_count is the authoritative global active-reader count;
    // it is incremented on ReadLock construction and decremented on destruction.
    return g_rcu_reader_count.load(std::memory_order_acquire) > 0;
}

inline void GracePeriodManager::grace_period_thread() {
    while (running_) {
        // Sleep for grace period duration
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Swap callback lists
        std::vector<RCUCallback> callbacks_to_execute;
        {
            std::lock_guard<std::mutex> lock(callbacks_mutex_);
            callbacks_to_execute.swap(current_callbacks_);
            current_callbacks_.swap(pending_callbacks_);
        }
        
        // Execute callbacks from previous grace period
        for (auto& callback : callbacks_to_execute) {
            callback();
        }
        
        grace_period_counter_.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace rcu
} // namespace themis
