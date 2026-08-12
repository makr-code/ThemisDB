/**
 * @file lockfree_metrics_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <memory>
#include "cycle_metrics.h"

namespace themis {
namespace performance {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324) // Intentional cache-line alignment padding for false-sharing avoidance.
#endif

/**
 * @brief Lock-free SPSC (Single Producer Single Consumer) ring buffer
 * 
 * Thread-safe for one writer and one reader. Uses atomic operations
 * for synchronization without mutexes.
 * 
 * @tparam T Element type
 * @tparam Capacity Buffer capacity (must be power of 2)
 */
template<typename T, size_t Capacity>
class LockFreeRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

public:
    LockFreeRingBuffer() noexcept
        : write_index_(0), read_index_(0), dropped_count_(0) {}

    /**
     * @brief Try to push element (non-blocking)
     * @param item Element to push
     * @return true if pushed, false if buffer full
     */
    bool tryPush(const T& item) noexcept {
        const size_t current_write = write_index_.load(std::memory_order_relaxed);
        const size_t next_write = (current_write + 1) & (Capacity - 1);
        
        // Check if buffer is full
        if (next_write == read_index_.load(std::memory_order_acquire)) {
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        
        // Write data
        buffer_[current_write] = item;
        
        // Update write index
        write_index_.store(next_write, std::memory_order_release);
        return true;
    }

    /**
     * @brief Try to push element (move semantics)
     * @param item Element to push
     * @return true if pushed, false if buffer full
     */
    bool tryPush(T&& item) noexcept {
        const size_t current_write = write_index_.load(std::memory_order_relaxed);
        const size_t next_write = (current_write + 1) & (Capacity - 1);
        
        // Check if buffer is full
        if (next_write == read_index_.load(std::memory_order_acquire)) {
            dropped_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        
        // Write data
        buffer_[current_write] = std::move(item);
        
        // Update write index
        write_index_.store(next_write, std::memory_order_release);
        return true;
    }

    /**
     * @brief Try to pop element (non-blocking)
     * @param item Output parameter for popped element
     * @return true if popped, false if buffer empty
     */
    bool tryPop(T& item) noexcept {
        const size_t current_read = read_index_.load(std::memory_order_relaxed);
        
        // Check if buffer is empty
        if (current_read == write_index_.load(std::memory_order_acquire)) {
            return false;
        }
        
        // Read data
        item = std::move(buffer_[current_read]);
        
        // Update read index
        const size_t next_read = (current_read + 1) & (Capacity - 1);
        read_index_.store(next_read, std::memory_order_release);
        return true;
    }

    /**
     * @brief Check if buffer is empty
     * @return true if empty
     */
    bool empty() const noexcept {
        return read_index_.load(std::memory_order_acquire) == 
               write_index_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get approximate buffer size
     * @return Number of elements in buffer (approximate)
     */
    size_t size() const noexcept {
        const size_t write = write_index_.load(std::memory_order_acquire);
        const size_t read = read_index_.load(std::memory_order_acquire);
        return (write - read) & (Capacity - 1);
    }

    /**
     * @brief Get number of dropped items due to buffer overflow
     * @return Dropped count
     */
    uint64_t dropped_count() const noexcept {
        return dropped_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset dropped count
     */
    void reset_dropped_count() noexcept {
        dropped_count_.store(0, std::memory_order_relaxed);
    }

private:
    // Cache line padding to avoid false sharing
    alignas(64) std::atomic<size_t> write_index_;
    alignas(64) std::atomic<size_t> read_index_;
    alignas(64) std::atomic<uint64_t> dropped_count_;
    alignas(64) std::array<T, Capacity> buffer_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/**
 * @brief Metrics entry for ring buffer
 */
struct MetricsEntry {
    std::string operation_name;
    OperationCycleMetrics metrics;
    uint64_t timestamp;
};

/**
 * @brief Thread-local metrics buffer with automatic deregistration
 * 
 * Each thread has its own buffer to avoid contention.
 * Background thread periodically drains all buffers.
 * RAII wrapper ensures buffer is deregistered when thread exits.
 */
class ThreadLocalMetricsBuffer {
public:
    static constexpr size_t BUFFER_CAPACITY = 1024;
    using BufferType = LockFreeRingBuffer<MetricsEntry, BUFFER_CAPACITY>;

    ThreadLocalMetricsBuffer() = default;

    /**
     * @brief Record operation metrics
     * @param operation_name Operation name
     * @param metrics Cycle metrics
     * @return true if recorded, false if buffer full
     */
    bool recordOperation(const std::string& operation_name, const OperationCycleMetrics& metrics) {
        MetricsEntry entry;
        entry.operation_name = operation_name;
        entry.metrics = metrics;
        entry.timestamp = HardwareCycleCounter::cpu_cycles();
        
        return buffer_.tryPush(std::move(entry));
    }

    /**
     * @brief Drain buffer (called by background thread)
     * @param output Output container
     * @return Number of entries drained
     */
    template<typename Container>
    size_t drain(Container& output) {
        MetricsEntry entry;
        size_t count = 0;
        // Only drain if buffer is still valid (not being destroyed)
        if (valid_.load(std::memory_order_acquire)) {
            while (buffer_.tryPop(entry)) {
                output.push_back(std::move(entry));
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Get dropped metrics count
     * @return Number of dropped metrics
     */
    uint64_t dropped_count() const noexcept {
        return buffer_.dropped_count();
    }

    /**
     * @brief Reset dropped count
     */
    void reset_dropped_count() noexcept {
        buffer_.reset_dropped_count();
    }

    /**
     * @brief Check if buffer is empty
     * @return true if empty
     */
    bool empty() const noexcept {
        return buffer_.empty();
    }

    /**
     * @brief Get approximate buffer size
     * @return Number of entries
     */
    size_t size() const noexcept {
        return buffer_.size();
    }
    
    /**
     * @brief Mark buffer as invalid (being destroyed)
     */
    void invalidate() noexcept {
        valid_.store(false, std::memory_order_release);
    }
    
    /**
     * @brief Check if buffer is valid
     */
    bool is_valid() const noexcept {
        return valid_.load(std::memory_order_acquire);
    }

private:
    BufferType buffer_;
    std::atomic<bool> valid_{true};
};

} // namespace performance
} // namespace themis
