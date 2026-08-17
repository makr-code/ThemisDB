/**
 * @file adaptive_rate_limiter.cpp
 * @brief Implementation of adaptive rate limiting for API connectors.
 *
 * Phase 2.9 (Bounded Resources + Stress Tests) — ING-IMPL-008
 */

#include "ingestion/adaptive_rate_limiter.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace themis {
namespace ingestion {

// ============================================================================
// Rate limit header parsing
// ============================================================================

RateLimitInfo parseRateLimitHeaders(
    const std::map<std::string, std::string>& response_headers) {
    RateLimitInfo info;

    // Try standard RateLimit-* headers
    {
        auto limit_it = response_headers.find("RateLimit-Limit");
        auto remaining_it = response_headers.find("RateLimit-Remaining");
        auto reset_it = response_headers.find("RateLimit-Reset");

        if (limit_it != response_headers.end() &&
            remaining_it != response_headers.end()) {
            try {
                info.limit_per_window = std::stoull(limit_it->second);
                info.remaining_in_window = std::stoull(remaining_it->second);
                info.is_rate_limited =
                    (info.remaining_in_window < info.limit_per_window / 10);
                info.header_source = "RateLimit-*";

                if (reset_it != response_headers.end()) {
                    info.reset_timestamp = std::stoull(reset_it->second);
                }
                return info;
            } catch (const std::exception&) {
                // Parse error; try next format
            }
        }
    }

    // Try X-RateLimit-* headers (GitHub, etc.)
    {
        auto limit_it = response_headers.find("X-RateLimit-Limit");
        auto remaining_it = response_headers.find("X-RateLimit-Remaining");
        auto reset_it = response_headers.find("X-RateLimit-Reset");

        if (limit_it != response_headers.end() &&
            remaining_it != response_headers.end()) {
            try {
                info.limit_per_window = std::stoull(limit_it->second);
                info.remaining_in_window = std::stoull(remaining_it->second);
                info.is_rate_limited =
                    (info.remaining_in_window < info.limit_per_window / 10);
                info.header_source = "X-RateLimit-*";

                if (reset_it != response_headers.end()) {
                    info.reset_timestamp = std::stoull(reset_it->second);
                }
                return info;
            } catch (const std::exception&) {
                // Parse error; try next format
            }
        }
    }

    // Try Retry-After header
    {
        auto retry_it = response_headers.find("Retry-After");
        if (retry_it != response_headers.end()) {
            try {
                int seconds = std::stoi(retry_it->second);
                info.retry_after = std::chrono::seconds(seconds);
                info.is_rate_limited = true;
                info.header_source = "Retry-After";
                return info;
            } catch (const std::exception&) {
                // Might be an HTTP-date format; not parsing for now
            }
        }
    }

    return info;
}

// ============================================================================
// AdaptiveRateLimiter implementation
// ============================================================================

bool AdaptiveRateLimiter::tryAcquireToken(bool allow_wait) {
    std::lock_guard<std::mutex> lock(mutex_);

    refillTokens();

    if (tokens_available_ >= 1.0) {
        tokens_available_ -= 1.0;
        return true;
    }

    if (!allow_wait) {
        return false;
    }

    // Calculate time to wait for next token
    const double tokens_needed = 1.0 - tokens_available_;
    const double rate = current_rate_limit_;
    if (rate <= 0.0) {
        return false;  // Invalid state
    }

    const std::chrono::duration<double> wait_duration(tokens_needed / rate);
    const auto now = std::chrono::steady_clock::now();
    const auto target_time = now + wait_duration;

    // Release lock and wait
    // Note: In production, this would use a condition variable
    // For now, we'll just spin
    lock.unlock();
    std::this_thread::sleep_until(target_time);
    lock.lock();

    tokens_available_ -= 1.0;
    return true;
}

void AdaptiveRateLimiter::recordResponse(
    int http_status_code,
    const std::map<std::string, std::string>& response_headers) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (http_status_code == 429) {  // Too Many Requests
        stats_.throttled_requests++;
        stats_.consecutive_throttles++;

        if (stats_.consecutive_throttles >= config_.max_consecutive_throttles) {
            // Give up; reset to minimum
            current_rate_limit_ = config_.min_requests_per_sec;
            stats_.consecutive_throttles = 0;
            return;
        }

        // Backoff: reduce rate limit
        current_rate_limit_ *= config_.backoff_multiplier;
        current_rate_limit_ =
            std::max(current_rate_limit_, config_.min_requests_per_sec);

        // Honor Retry-After if present
        if (config_.honor_retry_after) {
            auto info = parseRateLimitHeaders(response_headers);
            if (info.retry_after.count() > 0) {
                // In production, we'd actually sleep here or return an error
                // For now, just track it
            }
        }
    } else if (http_status_code >= 200 && http_status_code < 400) {
        stats_.successful_requests++;
        stats_.consecutive_throttles = 0;

        // Try to adapt rate based on remaining capacity
        auto info = parseRateLimitHeaders(response_headers);
        if (info.limit_per_window > 0 && info.remaining_in_window > 0) {
            const double utilization =
                static_cast<double>(info.remaining_in_window) /
                info.limit_per_window;

            // If we're using < 50% of capacity, try to speed up
            if (utilization > 0.5) {
                current_rate_limit_ *= config_.speedup_multiplier;
                current_rate_limit_ =
                    std::min(current_rate_limit_, config_.max_requests_per_sec);
            }
        }
    }

    stats_.total_requests++;
    if (stats_.total_requests > 0) {
        stats_.observed_success_rate = static_cast<double>(stats_.successful_requests) /
                                        stats_.total_requests;
    }
}

AdaptiveRateLimiter::Stats AdaptiveRateLimiter::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s = stats_;
    s.current_rate_limit = current_rate_limit_;
    return s;
}

void AdaptiveRateLimiter::setRateLimit(double requests_per_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_rate_limit_ = std::clamp(
        requests_per_sec, 
        config_.min_requests_per_sec,
        config_.max_requests_per_sec);
}

double AdaptiveRateLimiter::getRateLimit() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_rate_limit_;
}

void AdaptiveRateLimiter::resetToInitial() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_rate_limit_ = config_.initial_requests_per_sec;
    stats_ = Stats();
    tokens_available_ = 0.0;
    last_token_time_ = std::chrono::steady_clock::now();
    measurement_window_start_ = last_token_time_;
}

void AdaptiveRateLimiter::refillTokens() {
    const auto now = std::chrono::steady_clock::now();
    if (last_token_time_ == std::chrono::steady_clock::time_point{}) {
        last_token_time_ = now;
        tokens_available_ = 0.0;
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        now - last_token_time_);
    const double new_tokens = elapsed.count() * current_rate_limit_;
    tokens_available_ = std::min(tokens_available_ + new_tokens,
                                  current_rate_limit_ * 2.0);  // Cap at 2s worth
    last_token_time_ = now;
}

void AdaptiveRateLimiter::updateStatsSuccess() {
    stats_.successful_requests++;
    stats_.total_requests++;
}

void AdaptiveRateLimiter::updateStatsThrottle() {
    stats_.throttled_requests++;
    stats_.total_requests++;
    stats_.consecutive_throttles++;
}

void AdaptiveRateLimiter::adjustRateLimit(const RateLimitInfo& info) {
    if (!info.is_rate_limited || info.limit_per_window == 0) {
        return;
    }

    // Simple adjustment: if we're using most of the limit, back off
    const double utilization =
        static_cast<double>(info.remaining_in_window) / info.limit_per_window;
    if (utilization < 0.2) {  // Less than 20% remaining
        current_rate_limit_ *= config_.backoff_multiplier;
        current_rate_limit_ =
            std::max(current_rate_limit_, config_.min_requests_per_sec);
    }
}

// ============================================================================
// RateLimiterPool implementation
// ============================================================================

AdaptiveRateLimiter& RateLimiterPool::registerLimiter(
    const std::string& connector_name,
    const AdaptiveRateLimiterConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& limiter = limiters_[connector_name];
    if (!limiter) {
        limiter = std::make_unique<AdaptiveRateLimiter>(connector_name, config);
    }
    return *limiter;
}

AdaptiveRateLimiter* RateLimiterPool::getLimiter(
    const std::string& connector_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = limiters_.find(connector_name);
    if (it != limiters_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const AdaptiveRateLimiter* RateLimiterPool::getLimiter(
    const std::string& connector_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = limiters_.find(connector_name);
    if (it != limiters_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool RateLimiterPool::unregisterLimiter(const std::string& connector_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return limiters_.erase(connector_name) > 0;
}

std::vector<std::string> RateLimiterPool::listLimiters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& kv : limiters_) {
        names.push_back(kv.first);
    }
    return names;
}

void RateLimiterPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    limiters_.clear();
}

}  // namespace ingestion
}  // namespace themis
