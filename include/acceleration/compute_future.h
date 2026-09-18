/**
 * @file compute_future.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

class CancellationToken {
public:
    CancellationToken()
        : flag_(std::make_shared<std::atomic<bool>>(false)) {}

    // Copyable — each copy shares the same cancellation flag.
    CancellationToken(const CancellationToken&)            = default;
    CancellationToken& operator=(const CancellationToken&) = default;
    CancellationToken(CancellationToken&&)                 noexcept = default;
    CancellationToken& operator=(CancellationToken&&)      noexcept = default;

    void cancel() noexcept {
        if (flag_) {
          flag_->store(true, std::memory_order_release);
        }
    }

    bool is_cancelled() const noexcept {
        return flag_ && flag_->load(std::memory_order_acquire);
    }

    bool valid() const noexcept { return flag_ != nullptr; }

private:
    std::shared_ptr<std::atomic<bool>> flag_;
};

// =============================================================================
// DispatchStats
// =============================================================================

struct DispatchStats {
    uint64_t submit_time_ns  = 0;

    uint64_t start_time_ns   = 0;

    uint64_t finish_time_ns  = 0;

    uint32_t queue_depth     = 0;

    bool     from_cache      = false;

    // ── Derived helpers ────────────────────────────────────────────────────

    uint64_t kernel_latency_ns() const noexcept {
        return (finish_time_ns >= start_time_ns && start_time_ns > 0)
               ? finish_time_ns - start_time_ns : 0;
    }

    uint64_t total_latency_ns() const noexcept {
        return (finish_time_ns >= submit_time_ns && submit_time_ns > 0)
               ? finish_time_ns - submit_time_ns : 0;
    }
};

// =============================================================================
// ComputeFuture<T>
// =============================================================================

template <typename T>
class ComputeFuture {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    ComputeFuture(std::shared_future<T>          fut,
                  CancellationToken               token,
                  DispatchStats                   stats)
        : fut_(std::move(fut))
        , token_(std::move(token))
        , stats_(stats) {}

    static ComputeFuture make_ready(T value, DispatchStats stats = {}) {
        std::promise<T> p;
        p.set_value(std::move(value));
        return ComputeFuture(p.get_future().share(), CancellationToken{}, stats);
    }

    static ComputeFuture make_exceptional(std::exception_ptr exc,
                                          DispatchStats stats = {}) {
        std::promise<T> p;
        p.set_exception(exc);
        return ComputeFuture(p.get_future().share(), CancellationToken{}, stats);
    }

    ComputeFuture() = default;

    // Non-copyable, movable.
    ComputeFuture(const ComputeFuture&)            = delete;
    ComputeFuture& operator=(const ComputeFuture&) = delete;
    ComputeFuture(ComputeFuture&&)                 noexcept = default;
    ComputeFuture& operator=(ComputeFuture&&)      noexcept = default;

    // ── Core API ─────────────────────────────────────────────────────────────

    T get() {
        if (!fut_.valid()) {
            /**
             * @brief Future error.
             * @param[in] no_state Input parameter.
             * @return Return value.
             */
            throw std::future_error(std::future_errc::no_state);
        }
        return fut_.get();
    }

    template <typename F>
    /**
     * @brief Then.
     * @param[in] callback Input parameter.
     * @details Implements then without additional internal calls.
     */
    void then(F&& callback) {
        then_ = std::forward<F>(callback);
    }

    /**
     * @brief Invoke then.
     * @param[in] result Input parameter.
     * @details Calls: then_().
     */
    void invoke_then(const T& result) {
        if (then_) {
          then_(result);
        }
    }

    // ── Cancellation ─────────────────────────────────────────────────────────

    void cancel() noexcept { token_.cancel(); }

    bool is_cancelled() const noexcept { return token_.is_cancelled(); }

    const CancellationToken& cancellation_token() const noexcept { return token_; }

    // ── Diagnostics ──────────────────────────────────────────────────────────

    bool valid() const noexcept { return fut_.valid(); }

    DispatchStats stats() const noexcept { return stats_; }

private:
    std::shared_future<T>         fut_;
    CancellationToken             token_;
    DispatchStats                 stats_;
    std::function<void(const T&)> then_;
};

} // namespace acceleration
} // namespace themis
