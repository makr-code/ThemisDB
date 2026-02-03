// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/retry_strategy.h"
#include <algorithm>
#include <random>

namespace themisdb {
namespace sharding {

std::chrono::milliseconds calculateRetryDelay(
    const RetryConfig& config,
    size_t attempt_number
) {
    std::chrono::milliseconds delay;
    
    switch (config.strategy) {
        case RetryStrategy::NO_RETRY:
            return std::chrono::milliseconds(0);
        
        case RetryStrategy::IMMEDIATE:
            delay = std::chrono::milliseconds(0);
            break;
        
        case RetryStrategy::LINEAR_BACKOFF:
            delay = config.initial_delay * attempt_number;
            break;
        
        case RetryStrategy::EXPONENTIAL_BACKOFF:
        case RetryStrategy::ADAPTIVE: {
            double multiplier = std::pow(config.backoff_multiplier, attempt_number - 1);
            delay = std::chrono::milliseconds(
                static_cast<long long>(config.initial_delay.count() * multiplier)
            );
            break;
        }
    }
    
    // Cap at max_delay
    delay = std::min(delay, config.max_delay);
    
    // Add jitter if enabled
    if (config.jitter) {
        delay = addJitter(delay);
    }
    
    return delay;
}

std::chrono::milliseconds addJitter(
    std::chrono::milliseconds delay,
    double jitter_factor
) {
    if (delay.count() == 0) {
        return delay;
    }
    
    // Generate random jitter in range [-jitter_factor * delay, +jitter_factor * delay]
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(-jitter_factor, jitter_factor);
    
    double jitter = dis(gen);
    long long jittered_delay = delay.count() * (1.0 + jitter);
    
    // Ensure non-negative
    jittered_delay = std::max(0LL, jittered_delay);
    
    return std::chrono::milliseconds(jittered_delay);
}

} // namespace sharding
} // namespace themisdb
