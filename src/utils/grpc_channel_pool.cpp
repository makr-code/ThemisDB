/**
 * @file grpc_channel_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=13, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/grpc_channel_pool.h"
#include <stdexcept>

namespace themis {
namespace utils {

GrpcChannelPool::GrpcChannelPool()
    : GrpcChannelPool(Config{}) {
}

GrpcChannelPool::GrpcChannelPool(const Config& config)
    : config_(config)
{
}

GrpcChannelPool::~GrpcChannelPool() {
    shutdown_.store(true);
    clear();
}

std::shared_ptr<grpc::Channel> GrpcChannelPool::acquireChannel(
    const std::string& target,
    std::shared_ptr<grpc::ChannelCredentials> credentials
) {
    if (shutdown_.load()) {
        throw std::runtime_error("Channel pool is shutting down");
    }
    
    auto pool = getOrCreateTargetPool(target);
    std::unique_lock<std::mutex> lock(pool->mutex);
    
    // Try to get available channel from pool
    auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
    
    while (true) {
        // Remove stale channels
        while (!pool->available.empty()) {
            auto pooled_channel = pool->available.front();
            pool->available.pop();
            
            if (pooled_channel->isStale(config_.idle_timeout)) {
                // Remove stale channel
                pool->all_channels.erase(pooled_channel->channel);
                total_channels_.fetch_sub(1);
                stale_removed_.fetch_add(1);
                continue;
            }
            
            // Check if channel is ready
            if (!pooled_channel->isReady()) {
                // Channel not ready, remove it
                pool->all_channels.erase(pooled_channel->channel);
                total_channels_.fetch_sub(1);
                continue;
            }
            
            // Found good channel
            pooled_channel->in_use = true;
            pooled_channel->last_used = std::chrono::steady_clock::now();
            channels_reused_.fetch_add(1);
            return pooled_channel->channel;
        }
        
        // No available channels, check if we can create new one
        if (pool->all_channels.size() < config_.max_channels_per_target) {
            lock.unlock();

            // Create new channel outside the lock (may throw).
            std::shared_ptr<grpc::Channel> channel;
            try {
                channel = createChannel(target, credentials);
            } catch (...) {
                lock.lock();
                throw;
            }
            auto pooled_channel = std::make_shared<PooledChannel>();
            pooled_channel->channel = channel;
            pooled_channel->in_use = true;
            pooled_channel->last_used = std::chrono::steady_clock::now();

            lock.lock();
            pool->all_channels[channel] = pooled_channel;
            total_channels_.fetch_add(1);
            channels_created_.fetch_add(1);
            
            return channel;
        }
        
        // Wait for channel to become available
        if (std::chrono::steady_clock::now() >= deadline) {
            acquire_timeouts_.fetch_add(1);
            throw std::runtime_error("Timeout acquiring gRPC channel for target: " + target);
        }
        
        pool->cv.wait_until(lock, deadline);
    }
}

void GrpcChannelPool::releaseChannel(const std::string& target, std::shared_ptr<grpc::Channel> channel) {
    if (!channel) {
        return;
    }
    
    std::unique_lock<std::mutex> pools_lock(pools_mutex_);
    auto pool_it = target_pools_.find(target);
    if (pool_it == target_pools_.end()) {
        return;
    }
    auto pool = pool_it->second;
    pools_lock.unlock();
    
    std::unique_lock<std::mutex> lock(pool->mutex);
    
    auto it = pool->all_channels.find(channel);
    if (it == pool->all_channels.end()) {
        return;
    }
    
    auto pooled_channel = it->second;
    pooled_channel->in_use = false;
    pooled_channel->last_used = std::chrono::steady_clock::now();
    
    // Return to available pool
    pool->available.push(pooled_channel);
    lock.unlock();
    
    // Notify waiting threads
    pool->cv.notify_one();
}

GrpcChannelPool::Stats GrpcChannelPool::getStats() const {
    Stats stats;
    stats.total_channels = total_channels_.load();
    stats.channels_created = channels_created_.load();
    stats.channels_reused = channels_reused_.load();
    stats.stale_channels_removed = stale_removed_.load();
    stats.acquire_timeouts = acquire_timeouts_.load();
    
    std::lock_guard<std::mutex> lock(pools_mutex_);
    for (const auto& [target, pool] : target_pools_) {
        std::lock_guard<std::mutex> pool_lock(pool->mutex);
        stats.available_channels += pool->available.size();
        
        for (const auto& [ch, pooled_ch] : pool->all_channels) {
            if (pooled_ch->in_use) {
                stats.in_use_channels++;
            }
        }
    }
    
    return stats;
}

void GrpcChannelPool::clear() {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    for (auto& [target, pool] : target_pools_) {
        std::lock_guard<std::mutex> pool_lock(pool->mutex);
        pool->available = std::queue<std::shared_ptr<PooledChannel>>();
        pool->all_channels.clear();
    }
    
    target_pools_.clear();
    total_channels_.store(0);
}

void GrpcChannelPool::pruneStaleChannels() {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    for (auto& [target, pool] : target_pools_) {
        std::lock_guard<std::mutex> pool_lock(pool->mutex);
        
        // Prune stale channels from available queue
        std::queue<std::shared_ptr<PooledChannel>> new_available;
        while (!pool->available.empty()) {
            auto pooled_channel = pool->available.front();
            pool->available.pop();
            
            if (!pooled_channel->isStale(config_.idle_timeout)) {
                new_available.push(pooled_channel);
            } else {
                pool->all_channels.erase(pooled_channel->channel);
                total_channels_.fetch_sub(1);
                stale_removed_.fetch_add(1);
            }
        }
        pool->available = std::move(new_available);
    }
}

std::shared_ptr<grpc::Channel> GrpcChannelPool::createChannel(
    const std::string& target,
    std::shared_ptr<grpc::ChannelCredentials> credentials
) {
    grpc::ChannelArguments args;
    
    // Set keepalive options
    if (config_.enable_keepalive) {
        // Convert seconds to milliseconds safely
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 
                    static_cast<int>(std::min<int64_t>(config_.keepalive_time.count() * 1000, INT_MAX)));
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 
                    static_cast<int>(std::min<int64_t>(config_.keepalive_timeout.count() * 1000, INT_MAX)));
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    }
    
    // Set max concurrent streams
    args.SetInt(GRPC_ARG_MAX_CONCURRENT_STREAMS, config_.max_concurrent_streams);
    
    // Set connection timeout
    args.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 
                static_cast<int>(config_.connect_timeout.count() * 1000));
    
    // Use insecure credentials if none provided
    if (!credentials) {
        credentials = grpc::InsecureChannelCredentials();
    }
    
    return grpc::CreateCustomChannel(target, credentials, args);
}

std::shared_ptr<GrpcChannelPool::TargetPool> GrpcChannelPool::getOrCreateTargetPool(
    const std::string& target
) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    auto it = target_pools_.find(target);
    if (it != target_pools_.end()) {
        return it->second;
    }
    
    auto pool = std::make_shared<TargetPool>();
    target_pools_[target] = pool;
    return pool;
}

void GrpcChannelPool::warmup(
    const std::string& target,
    std::shared_ptr<grpc::ChannelCredentials> credentials,
    size_t num_channels
) {
    if (shutdown_.load()) {
        return;
    }
    
    // Default to half of max channels if not specified
    if (num_channels == 0) {
        num_channels = config_.max_channels_per_target / 2;
    }
    
    // Cap at max channels
    num_channels = std::min(num_channels, config_.max_channels_per_target);
    
    auto pool = getOrCreateTargetPool(target);
    std::lock_guard<std::mutex> lock(pool->mutex);
    
    // Create channels up to the requested amount
    for (size_t i = pool->all_channels.size(); i < num_channels; ++i) {
        try {
            auto channel = createChannel(target, credentials);
            auto pooled_channel = std::make_shared<PooledChannel>();
            pooled_channel->channel = channel;
            pooled_channel->in_use = false;
            pooled_channel->last_used = std::chrono::steady_clock::now();
            
            pool->all_channels[channel] = pooled_channel;
            pool->available.push(pooled_channel);
            
            total_channels_.fetch_add(1);
            channels_created_.fetch_add(1);
        } catch (const std::exception &) {
            // Continue best-effort warmup on individual channel creation failures.
        } catch (const std::string &) {
            // Continue best-effort warmup on individual channel creation failures.
        } catch (const char *) {
            // Continue best-effort warmup on individual channel creation failures.
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 6: Circuit Breaker & Health Check
// ─────────────────────────────────────────────────────────────────────────────

GrpcChannelPool::CircuitState GrpcChannelPool::getCircuitState(
    const std::string& target) const {
    std::lock_guard<std::mutex> lk(cb_mutex_);
    auto it = circuit_breakers_.find(target);
    if (it == circuit_breakers_.end()) {
      return CircuitState::CLOSED;
    }

    auto& cb = it->second;
    // Auto-transition OPEN → HALF_OPEN after the open timeout
    if (cb.state == CircuitState::OPEN) {
        auto elapsed = std::chrono::steady_clock::now() - cb.tripped_at;
        if (elapsed >= CB_OPEN_TIMEOUT) {
            // Transition happens on next read; we return HALF_OPEN here
            return CircuitState::HALF_OPEN;
        }
    }
    return cb.state;
}

void GrpcChannelPool::reportSuccess(const std::string& target) {
    std::lock_guard<std::mutex> lk(cb_mutex_);
    auto& cb = circuit_breakers_[target];

    if (cb.state == CircuitState::HALF_OPEN) {
        ++cb.success_count;
        if (cb.success_count >= CB_SUCCESS_THRESHOLD) {
            cb.state         = CircuitState::CLOSED;
            cb.failure_count = 0;
            cb.success_count = 0;
        }
    } else if (cb.state == CircuitState::CLOSED) {
        // Reset consecutive failure counter on success
        cb.failure_count = 0;
    }
}

void GrpcChannelPool::reportFailure(const std::string& target, uint16_t error_code) {
    (void)error_code;
    std::lock_guard<std::mutex> lk(cb_mutex_);
    auto& cb = circuit_breakers_[target];

    if (cb.state == CircuitState::OPEN) return; // Already tripped

    ++cb.failure_count;
    if (cb.state == CircuitState::HALF_OPEN || cb.failure_count >= CB_FAILURE_THRESHOLD) {
        cb.state         = CircuitState::OPEN;
        cb.tripped_at    = std::chrono::steady_clock::now();
        cb.success_count = 0;
    }
}

bool GrpcChannelPool::healthCheck(const std::string& target,
                                   std::chrono::milliseconds timeout) {
    // Don't health-check a tripped circuit
    if (getCircuitState(target) == CircuitState::OPEN) {
        return false;
    }

    try {
        auto pool = getOrCreateTargetPool(target);
        std::unique_lock<std::mutex> lock(pool->mutex);

        std::shared_ptr<grpc::Channel> ch;

        // Prefer an existing channel for the probe
        if (!pool->available.empty()) {
            auto pooled = pool->available.front();
            ch = pooled->channel;
        } else if (!pool->all_channels.empty()) {
            ch = pool->all_channels.begin()->first;
        } else {
            // Create a temporary probe channel outside the lock (may throw).
            lock.unlock();
            try {
                ch = createChannel(target, nullptr);
            } catch (...) {
                lock.lock();
                throw;
            }
            lock.lock();
        }

        if (!ch) {
          return false;
        }

        // Trigger a connection attempt and wait up to `timeout`.
        // IDLE is not considered healthy here because it does not prove that a
        // transport connection can actually be established to the target.
        auto state = ch->GetState(/*try_to_connect=*/true);
        if (state == GRPC_CHANNEL_READY) {
            return true;
        }

        auto deadline = std::chrono::system_clock::now() + timeout;
        if (ch->WaitForStateChange(state, deadline)) {
            state = ch->GetState(false);
            return state == GRPC_CHANNEL_READY;
        }
        return false;
    } catch (const std::exception &) {
        return false;
    } catch (const std::string &) {
        return false;
    } catch (const char *) {
        return false;
    }
}

} // namespace utils
} // namespace themis
