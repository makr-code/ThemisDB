/**
 * @file rate_limiter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/rate_limiter.h"
#include "utils/error_contracts.h"

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace utils {

RateLimiter::RateLimiter(double rate_per_second, double burst_size)
    : rate_(rate_per_second)
    , burst_(burst_size)
    , tokens_(burst_size)
    , last_refill_(Clock::now())
{
    if (rate_per_second <= 0.0) {
      throw std::invalid_argument("RateLimiter: rate must be positive");
    }
    if (burst_size <= 0.0) {
      throw std::invalid_argument("RateLimiter: burst_size must be positive");
    }
}

void RateLimiter::refill_locked() {
    auto now = Clock::now();
    double elapsed = std::chrono::duration<double>(now - last_refill_).count();
    tokens_ = std::min(burst_, tokens_ + elapsed * rate_);
    last_refill_ = now;
}

std::chrono::duration<double> RateLimiter::wait_for_locked(double tokens) const {
    double deficit = tokens - tokens_;
    if (deficit <= 0.0) {
      return std::chrono::duration<double>::zero();
    }
    return std::chrono::duration<double>(deficit / rate_);
}

bool RateLimiter::try_acquire(double tokens) {
    if (tokens <= 0.0) {
      return true;
    }
    std::lock_guard<std::mutex> lk(mutex_);
    refill_locked();
    if (tokens_ >= tokens) {
        tokens_ -= tokens;
        return true;
    }
    // Explicit structured diagnostic on token exhaustion (RATE_LIMIT_EXHAUSTED).
    auto ctx = themis::utils::makeErrorContext(
        themis::utils::ErrorCode::RATELIMIT_EXCEEDED,
        "Rate limit token bucket exhausted – request rejected; "
        "requested=" + std::to_string(tokens) +
        "; available=" + std::to_string(tokens_) +
        "; rate_per_s=" + std::to_string(rate_),
        "RateLimiter::try_acquire",
        themis::utils::ErrorSeverity::Warning,
        true);
    themis::utils::logErrorWithContext(ctx);
    return false;
}

bool RateLimiter::acquire_with_timeout(double tokens, std::chrono::milliseconds timeout) {
    if (tokens <= 0.0) {
      return true;
    }
    std::unique_lock<std::mutex> lk(mutex_);
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        refill_locked();
        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            return true;
        }
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return false;
        }

        // Compute the smaller of the refill time and the remaining timeout.
        auto wait = wait_for_locked(tokens);
        auto remaining = std::chrono::duration_cast<std::chrono::duration<double>>(deadline - now);
        auto sleep_dur = std::min(wait, remaining);
        cv_.wait_for(lk, sleep_dur);
    }
}

void RateLimiter::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    tokens_ = burst_;
    last_refill_ = Clock::now();
    cv_.notify_all();
}

double RateLimiter::available() const {
    std::lock_guard<std::mutex> lk(mutex_);
    double elapsed = std::chrono::duration<double>(Clock::now() - last_refill_).count();
    return std::min(burst_, tokens_ + elapsed * rate_);
}

void RateLimiter::set_rate(double rate_per_second) {
    if (rate_per_second <= 0.0) {
      throw std::invalid_argument("RateLimiter: rate must be positive");
    }
    std::lock_guard<std::mutex> lk(mutex_);
    refill_locked(); // absorb tokens at old rate before switching
    rate_ = rate_per_second;
}

bool RateLimiter::try_acquire_for(double tokens, std::chrono::milliseconds timeout) noexcept {
    if (tokens <= 0.0) {
      return true;
    }
    std::unique_lock<std::mutex> lk(mutex_);
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        refill_locked();
        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
          return false;
        }
        auto remaining = std::chrono::duration_cast<std::chrono::duration<double>>(deadline - now);
        auto wait = wait_for_locked(tokens);
        constexpr std::chrono::duration<double> max_sleep{0.05};
        auto sleep_dur = std::min({wait, max_sleep, remaining});
        cv_.wait_for(lk, sleep_dur);
        if (std::chrono::steady_clock::now() >= deadline) {
          return false;
        }
    }
}

} // namespace utils
} // namespace themis
