/**
 * @file grpc_channel_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>
#include <unordered_map>

namespace themis {
namespace utils {

/**
 * @brief gRPC Channel Pool for reusing channels and connections
 * 
 * Provides:
 * - Channel pooling (reduce connection overhead)
 * - Keep-Alive support
 * - Configurable timeouts and pool size
 * - Thread-safe access
 * - Per-target channel pooling
 * 
 * Performance Gains:
 * - 10-15% throughput improvement by reusing channels
 * - Reduced connection establishment overhead
 * - Better resource utilization under high concurrency
 * 
 * Sources:
 * - Benchmark Analysis: benchmarks/BENCHMARK_ANALYSIS_20251210.md
 * - Quick Wins: docs/de/performance/OPTIMIZATION_QUICK_WINS.md
 */
class GrpcChannelPool {
public:
    struct Config {
        size_t max_channels_per_target = 10;          ///< Max channels per target
        std::chrono::seconds idle_timeout{30};        ///< Channel idle timeout
        std::chrono::seconds connect_timeout{5};      ///< Connection timeout
        std::chrono::seconds acquire_timeout{10};     ///< Timeout for acquiring channel
        bool enable_keepalive = true;                 ///< Enable HTTP/2 keepalive
        std::chrono::seconds keepalive_time{30};      ///< Keepalive time
        std::chrono::seconds keepalive_timeout{10};   ///< Keepalive timeout
        int max_concurrent_streams = 100;             ///< Max concurrent streams per channel
    };
    
    GrpcChannelPool();
    explicit GrpcChannelPool(const Config& config);
    ~GrpcChannelPool();
    
    // Disable copy, allow move
    GrpcChannelPool(const GrpcChannelPool&) = delete;
    GrpcChannelPool& operator=(const GrpcChannelPool&) = delete;
    GrpcChannelPool(GrpcChannelPool&&) = default;
    GrpcChannelPool& operator=(GrpcChannelPool&&) = default;
    
    /**
     * @brief Acquire a channel for the given target
     * @param target Target address (e.g., "localhost:50051")
     * @param credentials Channel credentials (optional, uses insecure if nullptr)
     * @return Shared pointer to gRPC channel
     */
    std::shared_ptr<grpc::Channel> acquireChannel(
        const std::string& target,
        std::shared_ptr<grpc::ChannelCredentials> credentials = nullptr
    );
    
    /**
     * @brief Release a channel back to the pool
     * @param target Target address
     * @param channel Channel to release
     */
    void releaseChannel(const std::string& target, std::shared_ptr<grpc::Channel> channel);
    
    /**
     * @brief Get pool statistics
     */
    struct Stats {
        size_t total_channels = 0;
        size_t available_channels = 0;
        size_t in_use_channels = 0;
        size_t stale_channels_removed = 0;
        size_t acquire_timeouts = 0;
        size_t channels_created = 0;
        size_t channels_reused = 0;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Clear all pooled channels
     */
    void clear();
    
    /**
     * @brief Prune stale channels from pool
     */
    void pruneStaleChannels();
    
    /**
     * @brief Warm up pool for target
     * 
     * Pre-creates minimum number of channels in advance for faster
     * initial request handling. Useful for production deployments
     * to avoid cold-start latency.
     * 
     * @param target Target address (e.g., "localhost:50051")
     * @param credentials Channel credentials (optional)
     * @param num_channels Number of channels to pre-create (default: half of max)
     */
    void warmup(
        const std::string& target,
        std::shared_ptr<grpc::ChannelCredentials> credentials = nullptr,
        size_t num_channels = 0
    );

    // -----------------------------------------------------------------------
    // Phase 6: Circuit Breaker & Health Check
    // -----------------------------------------------------------------------

    /**
     * @brief Circuit-breaker state for a target.
     */
    enum class CircuitState {
        CLOSED,    ///< Normal operation – calls are forwarded.
        OPEN,      ///< Tripped – calls are rejected immediately.
        HALF_OPEN, ///< Probe state – one call is allowed to test recovery.
    };

    /**
     * @brief Get the current circuit-breaker state for the given target.
     */
    CircuitState getCircuitState(const std::string& target) const;

    /**
     * @brief Report a successful RPC call for the circuit breaker.
     *
     * Call this after every successful RPC.  It decrements the failure
     * counter and may transition HALF_OPEN → CLOSED.
     */
    void reportSuccess(const std::string& target);

    /**
     * @brief Report a failed RPC call for the circuit breaker.
     *
     * Call this after every failed RPC.  It increments the failure counter
     * and may trip the circuit (CLOSED → OPEN) when the threshold is reached.
     */
    void reportFailure(const std::string& target);

    /**
     * @brief Perform a lightweight health-check ping on a target.
     *
    * Checks the gRPC channel state. Does NOT send an actual RPC;
    * instead it calls GetState(true) on a pooled
     * channel to prompt a connection attempt and returns true when the
     * channel reaches READY or IDLE state within `timeout`.
     *
     * @param target  The endpoint to check (e.g. "localhost:50051").
     * @param timeout Maximum time to wait for the channel to become ready.
     * @return true if the channel is healthy, false otherwise.
     */
    bool healthCheck(const std::string& target,
                     std::chrono::milliseconds timeout = std::chrono::milliseconds(500));

private:
    /**
     * @brief Pooled channel with metadata
     */
    struct PooledChannel {
        std::shared_ptr<grpc::Channel> channel;
        std::chrono::steady_clock::time_point last_used;
        bool in_use = false;
        
        bool isStale(std::chrono::seconds timeout) const {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::seconds>(now - last_used) > timeout;
        }
        
        bool isReady() const {
            auto state = channel->GetState(false);
            return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
        }
    };
    
    /**
     * @brief Per-target channel pool
     */
    struct TargetPool {
        std::mutex mutex;
        std::condition_variable cv;
        std::queue<std::shared_ptr<PooledChannel>> available;
        std::unordered_map<std::shared_ptr<grpc::Channel>, std::shared_ptr<PooledChannel>> all_channels;
    };
    
    /**
     * @brief Create new channel for target
     */
    std::shared_ptr<grpc::Channel> createChannel(
        const std::string& target,
        std::shared_ptr<grpc::ChannelCredentials> credentials
    );
    
    /**
     * @brief Get or create target pool
     */
    std::shared_ptr<TargetPool> getOrCreateTargetPool(const std::string& target);
    
    Config config_;
    mutable std::mutex pools_mutex_;
    std::unordered_map<std::string, std::shared_ptr<TargetPool>> target_pools_;
    
    // Statistics
    std::atomic<size_t> total_channels_{0};
    std::atomic<size_t> channels_created_{0};
    std::atomic<size_t> channels_reused_{0};
    std::atomic<size_t> stale_removed_{0};
    std::atomic<size_t> acquire_timeouts_{0};
    std::atomic<bool> shutdown_{false};

    // Circuit breaker per target
    static constexpr size_t CB_FAILURE_THRESHOLD  = 5;   ///< Consecutive failures before tripping
    static constexpr size_t CB_SUCCESS_THRESHOLD  = 2;   ///< Successes in HALF_OPEN before closing
    static constexpr auto   CB_OPEN_TIMEOUT = std::chrono::seconds(30); ///< Time before HALF_OPEN probe

    struct CircuitBreakerState {
        CircuitState            state{CircuitState::CLOSED};
        size_t                  failure_count{0};
        size_t                  success_count{0};
        std::chrono::steady_clock::time_point tripped_at{};
    };
    mutable std::mutex                                    cb_mutex_;
    std::unordered_map<std::string, CircuitBreakerState>  circuit_breakers_;
};

} // namespace utils
} // namespace themis
