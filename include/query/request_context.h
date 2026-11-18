/**
 * @file request_context.h
 * @brief Shared request context: absolute deadline + cooperative cancellation token.
 *
 * RequestContext is the single value that search/query layers pass down so that
 * every stage can answer two questions with the same vocabulary:
 *   - "has the client cancelled?"      (StopReason::Cancelled)
 *   - "has the request deadline hit?"  (StopReason::DeadlineExceeded)
 *
 * It is header-only, copyable and cheap (one optional time_point + shared_ptr).
 * The cancellation token is the canonical themis::query::QueryCancellationToken.
 *
 * Scope: this type only *reports* stop conditions. Cancellation is cooperative:
 * a backend call that does not poll the context cannot be interrupted by it.
 * Each consumer documents whether it polls (cooperative) or only checks at
 * entry/exit (checkpoint-only).
 */

#pragma once

#include "query/query_canceller.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace themis {
namespace query {

/// Why a request must stop. Cancellation takes precedence over deadline expiry.
enum class StopReason : std::uint8_t {
    None = 0,
    Cancelled,
    DeadlineExceeded,
};

/**
 * @brief Carries a request's absolute deadline and cooperative cancellation sources.
 * @note Context checks report cancellation and expiry; only consumers that poll
 *       this context can interrupt ongoing work. Borrowed probe state must remain
 *       valid for every context that retains it.
 */
class RequestContext {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using CancellationProbe = bool (*)(const void*) noexcept;

    /// Unbounded, non-cancellable context.
    RequestContext() = default;

    explicit RequestContext(std::shared_ptr<QueryCancellationToken> token,
                            std::optional<TimePoint> deadline = std::nullopt) noexcept
        : token_(std::move(token)), deadline_(deadline) {}

    /// @brief Creates a request context with an additional borrowed cancellation source.
    /// @param token Optional shared cancellation token.
    /// @param deadline Optional absolute steady-clock deadline.
    /// @param probe Non-throwing callback polled by isCancelled(); may be null.
    /// @param probe_state Opaque borrowed state passed to probe; must remain valid while
    ///        this context is used. The callback must be safe to call from polling threads.
    /// @note No worker is created. Consumers observe external cancellation only when
    ///       they poll this context; callers must not retain it beyond probe_state's lifetime.
    RequestContext(std::shared_ptr<QueryCancellationToken> token,
                   std::optional<TimePoint> deadline,
                   CancellationProbe probe,
                   const void* probe_state) noexcept
        : token_(std::move(token)),
          deadline_(deadline),
          cancellation_probe_(probe),
          cancellation_probe_state_(probe_state) {}

    /// Context whose deadline is now + timeout, optionally sharing a token.
    [[nodiscard]] static RequestContext withTimeout(
        std::chrono::milliseconds timeout,
        std::shared_ptr<QueryCancellationToken> token = nullptr) {
        return RequestContext(std::move(token), Clock::now() + timeout);
    }

    /// Derived context shares the token and borrowed cancellation probe, with deadline
    /// = min(parent, now + timeout). The probe state must remain valid for the child.
    [[nodiscard]] RequestContext child(std::chrono::milliseconds timeout) const {
        const auto candidate = Clock::now() + timeout;
        const auto effective = deadline_ ? std::min(*deadline_, candidate) : candidate;
        return RequestContext(token_, effective, cancellation_probe_,
                              cancellation_probe_state_);
    }

    [[nodiscard]] bool hasDeadline() const noexcept { return deadline_.has_value(); }
    /**
     * @brief Returns the absolute steady-clock deadline, if one is set.
     * @return Deadline time point, or std::nullopt for an unbounded request.
     */
    [[nodiscard]] std::optional<TimePoint> deadline() const noexcept { return deadline_; }

    /**
     * @brief Returns the shared cancellation token, if present.
     * @return A const reference to the token pointer; its lifetime is tied to this context.
     */
    [[nodiscard]] const std::shared_ptr<QueryCancellationToken>& token() const noexcept {
        return token_;
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return (token_ && token_->isCancelled()) ||
               (cancellation_probe_ && cancellation_probe_(cancellation_probe_state_));
    }

    [[nodiscard]] bool isDeadlineExceeded() const noexcept {
        return deadline_ && Clock::now() >= *deadline_;
    }

    /**
     * @brief Determines whether cancellation or the deadline requires stopping.
     * @return Cancelled when a cancellation source is set, DeadlineExceeded when
     *         expired, or None otherwise; cancellation takes precedence.
     */
    [[nodiscard]] StopReason stopReason() const noexcept {
        if (isCancelled()) {
            return StopReason::Cancelled;
        }
        if (isDeadlineExceeded()) {
            return StopReason::DeadlineExceeded;
        }
        return StopReason::None;
    }

    /**
     * @brief Checks whether this request has been cancelled or has expired.
     * @return True when stopReason() is not None.
     */
    [[nodiscard]] bool shouldStop() const noexcept {
        return stopReason() != StopReason::None;
    }

    /**
     * @brief Returns the time remaining before the deadline.
     * @return Zero when expired, positive milliseconds while active, or std::nullopt
     *         when the request has no deadline.
     */
    [[nodiscard]] std::optional<std::chrono::milliseconds> remaining() const noexcept {
        if (!deadline_) {
            return std::nullopt;
        }
        const auto now = Clock::now();
        if (now >= *deadline_) {
            return std::chrono::milliseconds{0};
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(*deadline_ - now);
    }

private:
    std::shared_ptr<QueryCancellationToken> token_;
    std::optional<TimePoint> deadline_;
    CancellationProbe cancellation_probe_{nullptr};
    const void* cancellation_probe_state_{nullptr};
};

} // namespace query
} // namespace themis
