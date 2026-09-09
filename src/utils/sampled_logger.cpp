/**
 * @file sampled_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=0, L=2
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/logger.h"

#include <chrono>
#include <spdlog/spdlog.h>

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Per-call-site token bucket
// ---------------------------------------------------------------------------

struct SampledLogger::Bucket {
    double tokens = 0;
    std::chrono::steady_clock::time_point last_refill;

    explicit Bucket(double initial) : tokens(initial), last_refill(std::chrono::steady_clock::now()) {}

    /// Refill and try to consume one token. Returns true if allowed.
    bool try_consume(double rate, double burst) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_refill).count();
        tokens = std::min(burst, tokens + elapsed * rate);
        last_refill = now;

        if (tokens >= 1.0) {
            tokens -= 1.0;
            return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// SampledLogger
// ---------------------------------------------------------------------------

SampledLogger::SampledLogger(std::shared_ptr<Logger> underlying, SampledLoggerConfig cfg)
    : underlying_(std::move(underlying))
    , cfg_(cfg)
{
    // underlying_ is informational — SampledLogger delegates to Logger's
    // static API (which the shared_ptr wraps conceptually).  We keep the
    // pointer so callers can store a shared_ptr<Logger> for lifetime control.
}

SampledLogger::~SampledLogger() = default;

bool SampledLogger::should_log(Logger::Level level, const char* file, int line) {
    // Step 1: probabilistic sample-rate check per level.
    double rate = 1.0;
    switch (level) {
        case Logger::Level::TRACE:    rate = cfg_.trace_sample_rate; break;
        case Logger::Level::DEBUG:    rate = cfg_.debug_sample_rate; break;
        case Logger::Level::INFO:     rate = cfg_.info_sample_rate;  break;
        case Logger::Level::WARN:     rate = cfg_.warn_sample_rate;  break;
        case Logger::Level::ERROR:    rate = cfg_.error_sample_rate; break;
        case Logger::Level::CRITICAL: rate = 1.0;                   break;
    }

    if (rate < 1.0) {
        // Thread-local RNG to avoid contention on a shared generator.
        thread_local std::mt19937 rng{std::random_device{}()};
        thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) > rate) {
            return false;
        }
    }

    // Step 2: per-(file:line:level) token-bucket rate limit.
    // Build a compact key string.
    std::string key = {};
    key.reserve(128);
    key += (file ? file : "?");
    key += ':';
    key += std::to_string(line);
    key += ':';
    key += std::to_string(static_cast<int>(level));

    std::lock_guard<std::mutex> lk(buckets_mutex_);
    auto it = buckets_.find(key);
    if (it == buckets_.end()) {
        // Create a new bucket starting full.
        auto [ins_it, ok] = buckets_.emplace(key, std::make_unique<Bucket>(cfg_.burst_size));
        it = ins_it;
    }
    return it->second->try_consume(cfg_.burst_rate, cfg_.burst_size);
}

void SampledLogger::log(Logger::Level level, const std::string& msg,
                        const char* file, int line)
{
    if (!should_log(level, file, line)) {
        suppressed_.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    // Delegate to Logger's static API.
    switch (level) {
        case Logger::Level::TRACE:    Logger::trace("{}", msg);    break;
        case Logger::Level::DEBUG:    Logger::debug("{}", msg);    break;
        case Logger::Level::INFO:     Logger::info("{}", msg);     break;
        case Logger::Level::WARN:     Logger::warn("{}", msg);     break;
        case Logger::Level::ERROR:    Logger::error("{}", msg);    break;
        case Logger::Level::CRITICAL: Logger::critical("{}", msg); break;
    }
}

uint64_t SampledLogger::suppressed_total() const {
    return suppressed_.load(std::memory_order_relaxed);
}

void SampledLogger::reset_stats() {
    suppressed_.store(0, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(buckets_mutex_);
    buckets_.clear();
}

void SampledLogger::set_config(SampledLoggerConfig cfg) {
    std::lock_guard<std::mutex> lk(buckets_mutex_);
    cfg_ = std::move(cfg);
    // Clear existing buckets so new rate takes effect immediately.
    buckets_.clear();
}

} // namespace utils
} // namespace themis

