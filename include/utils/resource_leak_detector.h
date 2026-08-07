/**
 * @file resource_leak_detector.h
 * @brief Utilities for detecting resource leaks in tests
 *
 * Provides helpers for RAII verification in unit tests.
 * Tracks resource allocation/deallocation to detect leaks.
 *
 * @version 0.1.0
 * @since 2026-08-07
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace themis::utils::testing {

/**
 * @brief Resource leak detector for testing
 *
 * Tracks allocation and deallocation counts to verify
 * that all resources are properly cleaned up.
 *
 * Example:
 * ```cpp
 * ResourceCounter counter;
 * {
 *     auto ptr = std::make_unique<MyResource>();
 *     EXPECT_EQ(counter.allocations(), 1);
 *     EXPECT_EQ(counter.deallocations(), 0);
 * }
 * EXPECT_EQ(counter.allocations(), 1);
 * EXPECT_EQ(counter.deallocations(), 1);
 * EXPECT_EQ(counter.outstanding(), 0) << "Memory leaked!";
 * ```
 */
class ResourceCounter {
public:
    /**
     * @brief Increment allocation counter
     *
     * Call this when a resource is allocated.
     */
    void record_allocation() noexcept {
        ++allocations_;
    }

    /**
     * @brief Increment deallocation counter
     *
     * Call this when a resource is deallocated.
     */
    void record_deallocation() noexcept {
        ++deallocations_;
    }

    /**
     * @brief Get total allocations recorded
     */
    [[nodiscard]] std::size_t allocations() const noexcept {
        return allocations_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get total deallocations recorded
     */
    [[nodiscard]] std::size_t deallocations() const noexcept {
        return deallocations_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get outstanding resources (allocations - deallocations)
     *
     * Should be 0 at the end of test to verify no leaks.
     */
    [[nodiscard]] std::ptrdiff_t outstanding() const noexcept {
        return static_cast<std::ptrdiff_t>(allocations()) -
               static_cast<std::ptrdiff_t>(deallocations());
    }

    /**
     * @brief Reset counters
     */
    void reset() noexcept {
        allocations_.store(0, std::memory_order_release);
        deallocations_.store(0, std::memory_order_release);
    }

    /**
     * @brief Check if balanced (allocations == deallocations)
     */
    [[nodiscard]] bool is_balanced() const noexcept {
        return outstanding() == 0;
    }

private:
    std::atomic<std::size_t> allocations_{0};
    std::atomic<std::size_t> deallocations_{0};
};

/**
 * @brief RAII wrapper for tracked resources
 *
 * Automatically increments/decrements counters for RAII testing.
 *
 * Example:
 * ```cpp
 * ResourceCounter counter;
 * {
 *     TrackedResource<int> res(&counter);
 *     EXPECT_EQ(counter.allocations(), 1);
 * }
 * EXPECT_EQ(counter.deallocations(), 1);
 * ```
 */
template<typename T>
class TrackedResource {
public:
    /**
     * @brief Construct and record allocation
     *
     * @param counter Pointer to ResourceCounter (must outlive this object)
     * @param args Arguments to construct T
     */
    template<typename... Args>
    explicit TrackedResource(ResourceCounter* counter, Args&&... args)
        : counter_(counter)
        , value_(std::forward<Args>(args)...)
    {
        if (counter_) {
            counter_->record_allocation();
        }
    }

    /**
     * @brief Destruct and record deallocation
     */
    ~TrackedResource() noexcept {
        if (counter_) {
            counter_->record_deallocation();
        }
    }

    // Prevent copying
    TrackedResource(const TrackedResource&) = delete;
    TrackedResource& operator=(const TrackedResource&) = delete;

    // Allow moving
    TrackedResource(TrackedResource&& other) noexcept
        : counter_(other.counter_)
        , value_(std::move(other.value_))
    {
        other.counter_ = nullptr;
    }

    TrackedResource& operator=(TrackedResource&& other) noexcept {
        if (this != &other) {
            // Record deallocation for old value
            if (counter_) {
                counter_->record_deallocation();
            }

            counter_ = other.counter_;
            value_ = std::move(other.value_);
            other.counter_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Access the wrapped value
     */
    T& operator*() noexcept { return value_; }
    const T& operator*() const noexcept { return value_; }

    T* operator->() noexcept { return &value_; }
    const T* operator->() const noexcept { return &value_; }

private:
    ResourceCounter* counter_ = nullptr;
    T value_;
};

/**
 * @brief Assertion helper for resource cleanup
 *
 * Use in test teardown to verify no resources leaked.
 *
 * Example:
 * ```cpp
 * TEST_F(MyTest, ResourceCleanup) {
 *     ResourceCounter counter;
 *     // ... test code ...
 *     EXPECT_NO_LEAKS(counter);
 * }
 * ```
 */
#define EXPECT_NO_LEAKS(counter)                                               \
    EXPECT_TRUE((counter).is_balanced())                                       \
        << "Resource leak detected: " << (counter).allocations()               \
        << " allocations, " << (counter).deallocations() << " deallocations"

#define ASSERT_NO_LEAKS(counter)                                               \
    ASSERT_TRUE((counter).is_balanced())                                       \
        << "Resource leak detected: " << (counter).allocations()               \
        << " allocations, " << (counter).deallocations() << " deallocations"

} // namespace themis::utils::testing
