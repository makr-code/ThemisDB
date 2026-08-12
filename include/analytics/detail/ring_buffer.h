/**
 * @file ring_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Analytics – EventRingBuffer
 *
 * Bounded Multi-Producer Multi-Consumer (MPMC) lock-free ring buffer for the
 * CEP engine event queue.  Replaces std::queue<std::pair<std::string,Event>>
 * + mutex to eliminate per-node heap allocations and mutex contention on the
 * hot event-ingestion path.
 *
 * Design: Dmitry Vyukov's cache-efficient MPMC queue using per-slot sequence
 * numbers.  Each slot carries its own atomic<size_t> that encodes whether
 * the slot is empty (readable) or filled (writable).
 *
 *   - push() returns false when the queue is full (caller handles drop/back-pressure).
 *   - pop()  returns false when the queue is empty (caller should sleep/retry).
 *
 * The capacity must be a power of two and is fixed at construction time.
 *
 * Type requirements: T must be DefaultConstructible and MoveAssignable.
 * push() moves the item into the slot; pop() moves it out.
 *
 * Thread-safety: fully thread-safe for arbitrary numbers of producers and consumers.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>          // intptr_t
#include <memory>           // std::unique_ptr
#include <string>
#include <type_traits>      // std::is_nothrow_move_assignable_v
#include <utility>

namespace themis {
namespace analytics {
namespace detail {

#ifdef _MSC_VER
// EventRingBuffer intentionally uses cache-line alignment to avoid false sharing.
// This may introduce structural padding, which is expected and harmless here.
#pragma warning(push)
#pragma warning(disable : 4324)
#endif

/**
 * @brief Bounded MPMC ring buffer.
 *
 * @tparam T  Type of element stored.  Must be DefaultConstructible and
 *             MoveAssignable (slots are value-initialised at construction,
 *             and push/pop transfer ownership via move-assignment).
 *
 * Capacity is rounded up to the nearest power of two internally if not
 * already a power of two.
 */
template <typename T>
class EventRingBuffer {
    static_assert(std::is_default_constructible_v<T>,
                  "EventRingBuffer<T>: T must be DefaultConstructible "
                  "(slots are value-initialised at construction)");
    static_assert(std::is_move_assignable_v<T>,
                  "EventRingBuffer<T>: T must be MoveAssignable "
                  "(push/pop transfer ownership via move-assignment)");

public:
    // Cache-line size for padding to avoid false sharing.
    // Using a fixed constant (64) avoids the GCC -Winterference-size warning
    // and is correct for all x86/ARM targets we support.
    static constexpr size_t kCacheLineSize = 64;

    explicit EventRingBuffer(size_t capacity) {
        // Round capacity up to next power of two (minimum 2).
        size_t cap = 2;
        while (cap < capacity) cap <<= 1;
        mask_ = cap - 1;
        // Use a heap array; std::atomic is non-movable so std::vector won't work.
        slots_.reset(new Slot[cap]);
        for (size_t i = 0; i < cap; ++i) {
            slots_[i].seq.store(i, std::memory_order_relaxed);
        }
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    ~EventRingBuffer() = default;

    // Non-copyable, non-movable.
    EventRingBuffer(const EventRingBuffer&)            = delete;
    EventRingBuffer& operator=(const EventRingBuffer&) = delete;
    EventRingBuffer(EventRingBuffer&&)                 = delete;
    EventRingBuffer& operator=(EventRingBuffer&&)      = delete;

    /**
     * @brief Try to push an element.
     *
     * @param item  Item to push (moved into the slot).
     * @returns true on success, false if the queue is full.
     */
    bool push(T item) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t pos = head_.load(std::memory_order_relaxed);
        for (;;) {
            Slot& slot = slots_[pos & mask_];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

            if (diff == 0) {
                // Slot is ready to be written.
                if (head_.compare_exchange_weak(pos, pos + 1,
                        std::memory_order_relaxed)) {
                    slot.data = std::move(item);
                    slot.seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
                // CAS failed — retry with updated pos (already updated by CAS).
            } else if (diff < 0) {
                // Queue is full.
                return false;
            } else {
                // Another producer moved ahead; reload head.
                pos = head_.load(std::memory_order_relaxed);
            }
        }
    }

    /**
     * @brief Try to pop an element.
     *
     * @param item  Receives the popped element (moved out of the slot).
     * @returns true on success, false if the queue is empty.
     */
    bool pop(T& item) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t pos = tail_.load(std::memory_order_relaxed);
        for (;;) {
            Slot& slot = slots_[pos & mask_];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

            if (diff == 0) {
                // Slot has data ready.
                if (tail_.compare_exchange_weak(pos, pos + 1,
                        std::memory_order_relaxed)) {
                    item = std::move(slot.data);
                    slot.seq.store(pos + mask_ + 1, std::memory_order_release);
                    return true;
                }
                // CAS failed — retry.
            } else if (diff < 0) {
                // Queue is empty.
                return false;
            } else {
                // Consumer moved ahead; reload tail.
                pos = tail_.load(std::memory_order_relaxed);
            }
        }
    }

    /**
     * @brief Returns the approximate number of items in the queue.
     *
     * This is an estimate and may be stale by the time the caller reads it.
     */
    size_t size_approx() const noexcept {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_relaxed);
        return h >= t ? h - t : 0;
    }

    bool empty() const noexcept { return size_approx() == 0; }

    size_t capacity() const noexcept { return mask_ + 1; }

private:
    struct alignas(kCacheLineSize) Slot {
        std::atomic<size_t> seq{0};
        T                   data{};
    };

    // Separate cache lines for head and tail to avoid false sharing.
    alignas(kCacheLineSize) std::atomic<size_t> head_{0};
    alignas(kCacheLineSize) std::atomic<size_t> tail_{0};

    std::unique_ptr<Slot[]> slots_;
    size_t                  mask_{0};
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace detail
} // namespace analytics
} // namespace themis
