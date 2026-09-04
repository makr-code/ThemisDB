/**
 * @file compute_future.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>

namespace themis {
namespace acceleration {

// =============================================================================
// CancellationToken
// =============================================================================

/**
 * @brief Shared, copyable cancellation handle.
 *
 * A token is a thin wrapper around a heap-allocated `atomic<bool>` so that
 * multiple owners (caller + future + kernel side) all observe the same flag.
 *
 * Usage:
 * @code
 *   CancellationToken tok;
 *   auto fut = dispatcher.submit(batch, config, tok);
 *   // From any thread at any time:
 *   tok.cancel();
 *   // cancel() after completion is safe (no-op, the future is already done).
 * @endcode
 */
class CancellationToken {
public:
    /// Construct a live (not-yet-cancelled) token.
    CancellationToken()
        : flag_(std::make_shared<std::atomic<bool>>(false)) {}

    // Copyable — each copy shares the same cancellation flag.
    CancellationToken(const CancellationToken&)            = default;
    CancellationToken& operator=(const CancellationToken&) = default;
    CancellationToken(CancellationToken&&)                 noexcept = default;
    CancellationToken& operator=(CancellationToken&&)      noexcept = default;

    /**
     * @brief Request cancellation.
     *
     * Idempotent and safe to call concurrently from any thread.
     * After cancellation the corresponding `ComputeFuture::get()` may still
     * return a valid result if the kernel completed before the flag was
     * observed.
     */
    void cancel() noexcept {
        if (flag_) {
          flag_->store(true, std::memory_order_release);
        }
    }

    /**
     * @brief Returns true if cancellation has been requested.
     *
     * Thread-safe; uses acquire memory order.
     */
    bool is_cancelled() const noexcept {
        return flag_ && flag_->load(std::memory_order_acquire);
    }

    /**
     * @brief Returns true if this token refers to a valid shared flag.
     *
     * A default-constructed token is always valid.  Moved-from tokens are
     * invalid.
     */
    bool valid() const noexcept { return flag_ != nullptr; }

private:
    std::shared_ptr<std::atomic<bool>> flag_;
};

// =============================================================================
// DispatchStats
// =============================================================================

/**
 * @brief Latency and queue-depth introspection data for a dispatched kernel.
 *
 * All timestamps are nanoseconds since an unspecified epoch
 * (CLOCK_MONOTONIC on Linux, QueryPerformanceCounter on Windows).
 * A value of 0 means "not recorded" (e.g. the kernel was served from cache).
 */
struct DispatchStats {
    /// Wall-clock time (ns) at the point `submit()` was called on the calling
    /// thread.  Always non-zero for kernels submitted via IAsyncComputeDispatch.
    uint64_t submit_time_ns  = 0;

    /// Wall-clock time (ns) when the kernel began executing on the device
    /// (or on the host for CPU-fallback kernels).  May be 0 for cached results.
    uint64_t start_time_ns   = 0;

    /// Wall-clock time (ns) when the kernel finished and the result was ready.
    uint64_t finish_time_ns  = 0;

    /// Depth of the kernel submission queue at the time this kernel was queued.
    /// A large value indicates back-pressure on the dispatch pipeline.
    uint32_t queue_depth     = 0;

    /// True when the result was replayed from a CUDA-graph or result cache
    /// rather than by executing the kernel from scratch.
    bool     from_cache      = false;

    // ── Derived helpers ────────────────────────────────────────────────────

    /**
     * @brief Pure kernel execution latency in nanoseconds.
     *
     * Returns 0 when @p start_time_ns or @p finish_time_ns was not recorded.
     */
    uint64_t kernel_latency_ns() const noexcept {
        return (finish_time_ns >= start_time_ns && start_time_ns > 0)
               ? finish_time_ns - start_time_ns : 0;
    }

    /**
     * @brief End-to-end latency (queue wait + execution) in nanoseconds.
     *
     * Returns 0 when @p submit_time_ns or @p finish_time_ns was not recorded.
     */
    uint64_t total_latency_ns() const noexcept {
        return (finish_time_ns >= submit_time_ns && submit_time_ns > 0)
               ? finish_time_ns - submit_time_ns : 0;
    }
};

// =============================================================================
// ComputeFuture<T>
// =============================================================================

/**
 * @brief Lightweight future handle returned by IAsyncComputeDispatch::submit().
 *
 * `ComputeFuture<T>` wraps a `std::shared_future<T>` so that multiple threads
 * may observe the same result (e.g. a query fan-out), combined with a
 * `CancellationToken` and `DispatchStats` snapshot.
 *
 * ### Lifecycle
 * 1. The dispatcher constructs the future and returns it to the caller.
 * 2. The caller may call `cancel()` at any time to signal early termination.
 * 3. The caller calls `get()` to block until the result is available and
 *    retrieve it.
 * 4. Optionally the caller registers a continuation with `then()` **before**
 *    the future becomes ready.  The continuation is invoked on an unspecified
 *    thread once the backing `std::promise` is set.
 *
 * ### Thread safety
 * - `get()` : safe from any thread (blocks on the shared_future).
 * - `cancel()` : safe from any thread (atomic write to shared flag).
 * - `stats()` : safe from any thread (immutable snapshot copy-constructed at
 *    submit() time; finish_time_ns populated when the promise is fulfilled).
 * - `then()` : must be called **before** the future is ready.
 *
 * @tparam T Result value type.  Must be default-constructible and movable.
 */
template <typename T>
class ComputeFuture {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    /**
     * @brief Construct a valid future backed by an already-created shared state.
     *
     * @param fut    Shared future for the result value.
     * @param token  Cancellation token (may be a default-constructed token).
     * @param stats  Dispatch statistics snapshot (finish_time_ns may be 0 until
     *               the promise is fulfilled; callers should re-query if needed).
     */
    ComputeFuture(std::shared_future<T>          fut,
                  CancellationToken               token,
                  DispatchStats                   stats)
        : fut_(std::move(fut))
        , token_(std::move(token))
        , stats_(stats) {}

    /**
     * @brief Construct a future that is already fulfilled with @p value.
     *
     * Convenience factory for CPU-side and cached results.
     */
    static ComputeFuture make_ready(T value, DispatchStats stats = {}) {
        std::promise<T> p;
        p.set_value(std::move(value));
        return ComputeFuture(p.get_future().share(), CancellationToken{}, stats);
    }

    /**
     * @brief Construct a future that immediately carries an exception.
     *
     * Convenience factory for error propagation without throwing at the call
     * site.
     */
    static ComputeFuture make_exceptional(std::exception_ptr exc,
                                          DispatchStats stats = {}) {
        std::promise<T> p;
        p.set_exception(exc);
        return ComputeFuture(p.get_future().share(), CancellationToken{}, stats);
    }

    /// Default constructor produces an invalid future (valid() == false).
    ComputeFuture() = default;

    // Non-copyable, movable.
    ComputeFuture(const ComputeFuture&)            = delete;
    ComputeFuture& operator=(const ComputeFuture&) = delete;
    ComputeFuture(ComputeFuture&&)                 noexcept = default;
    ComputeFuture& operator=(ComputeFuture&&)      noexcept = default;

    // ── Core API ─────────────────────────────────────────────────────────────

    /**
     * @brief Block until the kernel completes and return the result.
     *
     * Re-entrant: multiple threads may call get() on the same future
     * concurrently (because the backing future is a shared_future).
     *
     * @throws std::future_error if the future is invalid.
     * @throws Any exception stored in the promise by the kernel.
     */
    T get() {
        if (!fut_.valid()) {
            throw std::future_error(std::future_errc::no_state);
        }
        return fut_.get();
    }

    /**
     * @brief Register a continuation callback invoked once the result is ready.
     *
     * The callback is invoked on the thread that fulfils the associated promise
     * (typically the dispatch worker thread).  Callers must register the
     * continuation **before** the future becomes ready; behaviour is undefined
     * if the future is already ready at the time `then()` is called.
     *
     * @param callback Function object accepting `const T&`.
     */
    template <typename F>
    void then(F&& callback) {
        then_ = std::forward<F>(callback);
    }

    /**
     * @brief Invoke the registered continuation (if any) with @p result.
     *
     * Called internally by the dispatcher once the promise is fulfilled.
     * No-op if no continuation was registered.
     */
    void invoke_then(const T& result) {
        if (then_) {
          then_(result);
        }
    }

    // ── Cancellation ─────────────────────────────────────────────────────────

    /**
     * @brief Request early termination of the associated kernel.
     *
     * Safe to call from any thread at any time, including after the kernel
     * has already completed (no-op in that case).
     */
    void cancel() noexcept { token_.cancel(); }

    /**
     * @brief Returns true if cancellation has been requested on this future.
     */
    bool is_cancelled() const noexcept { return token_.is_cancelled(); }

    /**
     * @brief Returns the `CancellationToken` owned by this future.
     *
     * Callers may keep a copy and call `cancel()` on it after the future is
     * moved away.
     */
    const CancellationToken& cancellation_token() const noexcept { return token_; }

    // ── Diagnostics ──────────────────────────────────────────────────────────

    /**
     * @brief Returns true if this future holds a valid (non-default) state.
     */
    bool valid() const noexcept { return fut_.valid(); }

    /**
     * @brief Return the dispatch statistics snapshot.
     *
     * Thread-safe; returns a copy of the immutable stats struct.
     */
    DispatchStats stats() const noexcept { return stats_; }

private:
    std::shared_future<T>         fut_;
    CancellationToken             token_;
    DispatchStats                 stats_;
    std::function<void(const T&)> then_;
};

} // namespace acceleration
} // namespace themis
