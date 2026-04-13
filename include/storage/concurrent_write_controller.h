/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concurrent_write_controller.h                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-13 20:27:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     274                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 58364e3a6b  2026-04-09  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file concurrent_write_controller.h
 * @brief Bounded, FIFO-fair write concurrency controller for the storage layer.
 *
 * ## Motivation (PERF-D6 / P-6)
 *
 * Under 10 concurrent HTTP clients the storage layer exhibits a coefficient of
 * variation (CV) of ~20.74 % for write latency.  The root cause is uncontrolled
 * lock contention: many threads compete simultaneously for the same RocksDB
 * write-commit mutex, causing some requests to race to the front while others
 * wait much longer — producing high variance.
 *
 * ## Solution
 *
 * `ConcurrentWriteController` is a counting semaphore with FIFO wakeup
 * semantics.  It limits the number of concurrent writers to
 * `Config::max_concurrent_writes`.  Excess writers block in a FIFO queue and
 * are woken in order, converting the "thundering herd" into orderly, bounded
 * concurrency.
 *
 * Expected outcome:
 *  - CV drops from ~20.74 % to < 5 % under 10 concurrent clients.
 *  - P99 latency remains predictable (no outliers from contested locks).
 *
 * ## Usage
 *
 * ```cpp
 * ConcurrentWriteController wc; // default config
 *
 * // --- Acquire / Release (manual) ---
 * auto guard = wc.acquire();    // blocks if at capacity; RAII, auto-releases
 * // ... perform write ...
 * // guard destructor releases the slot and wakes the next waiter
 *
 * // --- Stats ---
 * auto s = wc.getStats();
 * printf("queue_depth=%zu avg_wait_us=%lld\n", s.queue_depth, s.avg_wait_us);
 * ```
 *
 * ## Thread Safety
 * All public methods are thread-safe.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

struct ConcurrentWriteControllerConfig {
    /// Maximum number of write operations that may proceed in parallel.
    /// Set to 0 to use `std::thread::hardware_concurrency() / 2` (min 1).
    size_t max_concurrent_writes = 0;

    /// Maximum number of callers that may queue waiting for a slot.
    /// Callers that arrive when the queue is full receive `AcquireError::QUEUE_FULL`.
    /// Set to 0 for unlimited queue depth.
    size_t max_queue_depth = 0;

    /// Maximum time a caller will wait for a slot.
    /// Set to 0 for unlimited wait (block forever).
    std::chrono::milliseconds acquire_timeout{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Errors
// ─────────────────────────────────────────────────────────────────────────────

enum class ConcurrentWriteAcquireError {
    QUEUE_FULL,   ///< The wait queue is at capacity
    TIMEOUT,      ///< Waited longer than acquire_timeout
    SHUTDOWN,     ///< The controller has been shut down
};

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

struct ConcurrentWriteStats {
    size_t   active_writes   = 0;   ///< Current in-flight write slots in use
    size_t   queue_depth     = 0;   ///< Current number of waiters in the FIFO queue
    size_t   total_acquired  = 0;   ///< Lifetime acquire successes
    size_t   total_rejected  = 0;   ///< Lifetime acquire failures (queue full or timeout)
    int64_t  avg_wait_us     = 0;   ///< Exponentially-weighted moving average wait (µs)
    int64_t  p99_wait_us     = 0;   ///< Approximate P99 wait (µs) from sliding window
    int64_t  max_wait_us     = 0;   ///< Lifetime maximum observed wait (µs)
};

// ─────────────────────────────────────────────────────────────────────────────
// WriteGuard  (RAII slot holder)
// ─────────────────────────────────────────────────────────────────────────────

class ConcurrentWriteController;

/**
 * @brief RAII guard that holds a write slot.
 *
 * The slot is released (and the next FIFO waiter woken) when the guard
 * goes out of scope.  Guards are move-only.
 */
class WriteGuard {
public:
    WriteGuard() = default;
    ~WriteGuard();

    WriteGuard(WriteGuard&& other) noexcept;
    WriteGuard& operator=(WriteGuard&& other) noexcept;

    WriteGuard(const WriteGuard&)            = delete;
    WriteGuard& operator=(const WriteGuard&) = delete;

    /// Returns true when the guard actually holds a slot (not default-constructed).
    explicit operator bool() const noexcept { return controller_ != nullptr; }

    /// Manually release the slot before the guard is destroyed.
    void release() noexcept;

private:
    friend class ConcurrentWriteController;
    explicit WriteGuard(ConcurrentWriteController* ctrl) noexcept;

    ConcurrentWriteController* controller_ = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// ConcurrentWriteController
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Bounded FIFO counting semaphore for storage write operations.
 *
 * Limits concurrent writers to `config.max_concurrent_writes`.  Excess
 * callers wait in FIFO order, removing the thundering-herd pattern that
 * causes high latency variance under concurrent HTTP clients.
 *
 * Lifecycle
 * ---------
 * - Default-constructed and ready to use.
 * - `shutdown()` unblocks all waiters and prevents new acquires.
 * - The destructor calls `shutdown()`.
 *
 * Acquiring a slot
 * ----------------
 * - `acquire()` → blocks until a slot is available; returns a `WriteGuard`.
 * - `tryAcquire()` → non-blocking; returns `std::nullopt` if no slot.
 * - Both raise `ConcurrentWriteAcquireError` on queue-full / timeout / shutdown.
 */
class ConcurrentWriteController {
public:
    explicit ConcurrentWriteController(
        ConcurrentWriteControllerConfig config = {});
    ~ConcurrentWriteController();

    // Not copyable or movable after construction (threads hold pointers to this).
    ConcurrentWriteController(const ConcurrentWriteController&) = delete;
    ConcurrentWriteController& operator=(const ConcurrentWriteController&) = delete;
    ConcurrentWriteController(ConcurrentWriteController&&)       = delete;
    ConcurrentWriteController& operator=(ConcurrentWriteController&&) = delete;

    // ── Slot Acquisition ─────────────────────────────────────────────────────

    /**
     * @brief Acquire a write slot, waiting in FIFO order if necessary.
     *
     * @return A RAII `WriteGuard`; the slot is released when the guard is
     *         destroyed or `release()` is called.
     *
     * @throws std::runtime_error wrapping `ConcurrentWriteAcquireError` if the
     *         queue is full, the timeout fires, or the controller is shut down.
     */
    [[nodiscard]] WriteGuard acquire();

    /**
     * @brief Try to acquire a slot without blocking.
     *
     * @return A `WriteGuard` on success; `std::nullopt` if no slot is
     *         immediately available.
     */
    [[nodiscard]] std::optional<WriteGuard> tryAcquire();

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @brief Shut down the controller.
     *
     * Unblocks all current waiters (they receive `SHUTDOWN` error) and
     * prevents future `acquire()` calls from succeeding.
     */
    void shutdown() noexcept;

    // ── Diagnostics ──────────────────────────────────────────────────────────

    ConcurrentWriteStats getStats() const noexcept;

    size_t maxConcurrentWrites() const noexcept { return max_slots_; }

private:
    friend class WriteGuard;

    /// Called by WriteGuard destructor / release() to return a slot.
    void releaseSlot() noexcept;

    /// Record a successful acquire wait time and update EWMA / sliding window.
    void recordWait(int64_t wait_us) noexcept;

    // ── Configuration ────────────────────────────────────────────────────────
    const size_t                          max_slots_;
    const size_t                          max_queue_depth_;
    const std::chrono::milliseconds       acquire_timeout_;

    // ── State (protected by mutex_) ──────────────────────────────────────────
    mutable std::mutex                    mutex_;
    size_t                                active_{0};
    std::queue<std::promise<void>>        waiters_;
    bool                                  shutdown_{false};

    // ── Statistics (lock-free) ───────────────────────────────────────────────
    std::atomic<uint64_t>                 total_acquired_{0};
    std::atomic<uint64_t>                 total_rejected_{0};
    // EWMA: int64 scaled by 1024 to avoid floating-point in the hot path
    std::atomic<int64_t>                  ewma_wait_us_scaled_{0};

    // Sliding window for P99: circular buffer of last 128 wait times (µs)
    static constexpr size_t               kWindowSize = 128;
    mutable std::mutex                    window_mutex_;
    int64_t                               wait_window_[kWindowSize]{};
    size_t                                window_pos_{0};
    size_t                                window_count_{0};

    std::atomic<int64_t>                  max_wait_us_{0};
};

} // namespace storage
} // namespace themis
