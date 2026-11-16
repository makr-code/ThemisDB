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

class RequestContext {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    /// Unbounded, non-cancellable context.
    RequestContext() = default;

    explicit RequestContext(std::shared_ptr<QueryCancellationToken> token,
                            std::optional<TimePoint> deadline = std::nullopt) noexcept
        : token_(std::move(token)), deadline_(deadline) {}

    /// Context whose deadline is now + timeout, optionally sharing a token.
    [[nodiscard]] static RequestContext withTimeout(
        std::chrono::milliseconds timeout,
        std::shared_ptr<QueryCancellationToken> token = nullptr) {
        return RequestContext(std::move(token), Clock::now() + timeout);
    }

    /// Derived context sharing the same token with deadline = min(parent, now + timeout).
    [[nodiscard]] RequestContext child(std::chrono::milliseconds timeout) const {
        const auto candidate = Clock::now() + timeout;
        const auto effective = deadline_ ? std::min(*deadline_, candidate) : candidate;
        return RequestContext(token_, effective);
    }

    [[nodiscard]] bool hasDeadline() const noexcept { return deadline_.has_value(); }
    [[nodiscard]] std::optional<TimePoint> deadline() const noexcept { return deadline_; }
    [[nodiscard]] const std::shared_ptr<QueryCancellationToken>& token() const noexcept {
        return token_;
    }

    [[nodiscard]] bool isCancelled() const noexcept {
        return token_ && token_->isCancelled();
    }

    [[nodiscard]] bool isDeadlineExceeded() const noexcept {
        return deadline_ && Clock::now() >= *deadline_;
    }

    [[nodiscard]] StopReason stopReason() const noexcept {
        if (isCancelled()) {
            return StopReason::Cancelled;
        }
        if (isDeadlineExceeded()) {
            return StopReason::DeadlineExceeded;
        }
        return StopReason::None;
    }

    [[nodiscard]] bool shouldStop() const noexcept {
        return stopReason() != StopReason::None;
    }

    /// Time left until the deadline (zero when expired); nullopt when unbounded.
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
};

} // namespace query
} // namespace themis
