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

#include <algorithm>
#include <stdexcept>
#include <thread>

namespace themis {
namespace utils {

RateLimiter::RateLimiter(double rate_per_second, double burst_size)
    : rate_(rate_per_second)
    , burst_(burst_size)
    , tokens_(burst_size)
    , last_refill_(Clock::now())
{
    if (rate_per_second <= 0.0) throw std::invalid_argument("RateLimiter: rate must be positive");
    if (burst_size <= 0.0)      throw std::invalid_argument("RateLimiter: burst_size must be positive");
}

void RateLimiter::refill_locked() {
    auto now = Clock::now();
    double elapsed = std::chrono::duration<double>(now - last_refill_).count();
    tokens_ = std::min(burst_, tokens_ + elapsed * rate_);
    last_refill_ = now;
}

std::chrono::duration<double> RateLimiter::wait_for_locked(double tokens) const {
    double deficit = tokens - tokens_;
    if (deficit <= 0.0) return std::chrono::duration<double>::zero();
    return std::chrono::duration<double>(deficit / rate_);
}

bool RateLimiter::try_acquire(double tokens) {
    if (tokens <= 0.0) return true;
    std::lock_guard<std::mutex> lk(mutex_);
    refill_locked();
    if (tokens_ >= tokens) {
        tokens_ -= tokens;
        return true;
    }
    return false;
}

void RateLimiter::acquire(double tokens) {
    if (tokens <= 0.0) return;
    std::unique_lock<std::mutex> lk(mutex_);
    while (true) {
        refill_locked();
        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            return;
        }
        // Compute precise sleep duration so we don't busy-wait.
        auto wait = wait_for_locked(tokens);
        // Cap single sleep at 50 ms to remain responsive to rate changes.
        constexpr std::chrono::duration<double> max_sleep{0.05};
        auto sleep_dur = std::min(wait, max_sleep);
        lk.unlock();
        std::this_thread::sleep_for(sleep_dur);
        lk.lock();
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
    if (rate_per_second <= 0.0) throw std::invalid_argument("RateLimiter: rate must be positive");
    std::lock_guard<std::mutex> lk(mutex_);
    refill_locked(); // absorb tokens at old rate before switching
    rate_ = rate_per_second;
}

} // namespace utils
} // namespace themis
