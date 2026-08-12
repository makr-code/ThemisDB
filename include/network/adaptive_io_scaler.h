/**
 * @file adaptive_io_scaler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

namespace themis {
namespace network {

/**
 * @brief Adaptive I/O thread scaler for the network layer.
 *
 * Monitors a caller-supplied "active connection" metric and adjusts the
 * recommended number of I/O threads up or down so that the server always
 * has an appropriate level of parallelism without wasting resources.
 *
 * The scaler does *not* directly manage threads; it computes a target
 * thread count and notifies the caller via an optional callback so that
 * the server can adjust its own thread pool at the appropriate point.
 *
 * Scale-up rule:
 *   If (active_connections / current_threads) >= scale_up_threshold
 *   and current_threads < max_threads → increase by one thread.
 *
 * Scale-down rule:
 *   If (active_connections / current_threads) <= scale_down_threshold
 *   and current_threads > min_threads → decrease by one thread.
 *
 * A background thread polls the metric at `check_interval` and applies
 * the rules.  The caller must provide a `ConnectionCountProvider`
 * callable that returns the current number of active connections.
 *
 * Usage:
 * @code
 * AdaptiveIOScaler::Config cfg;
 * cfg.min_threads = 2;
 * cfg.max_threads = 16;
 * cfg.scale_up_threshold   = 200;  // conns per thread
 * cfg.scale_down_threshold = 50;   // conns per thread
 *
 * AdaptiveIOScaler scaler(cfg, []{ return server.getActiveConnections(); });
 * scaler.setScaleCallback([&](size_t n){ server.resizeIOThreadPool(n); });
 * scaler.start();
 * @endcode
 */
class AdaptiveIOScaler {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Minimum number of I/O threads (never scale below this).
        size_t min_threads = 2;

        /// Maximum number of I/O threads (never scale above this).
        size_t max_threads = 16;

        /**
         * @brief Connections-per-thread ratio that triggers a scale-up.
         *
         * When active_connections / current_threads >= this value and
         * current_threads < max_threads the scaler recommends adding a
         * thread.  Default: 200 connections per thread.
         */
        double scale_up_threshold = 200.0;

        /**
         * @brief Connections-per-thread ratio that triggers a scale-down.
         *
         * When active_connections / current_threads <= this value and
         * current_threads > min_threads the scaler recommends removing a
         * thread.  Default: 50 connections per thread.
         */
        double scale_down_threshold = 50.0;

        /// How often the background monitor evaluates the scaling rules.
        std::chrono::milliseconds check_interval{5000};

        /// Initial / starting number of I/O threads.
        size_t initial_threads = 4;

        Config() = default;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    struct Stats {
        size_t   current_threads    = 0;
        uint64_t scale_up_events    = 0;
        uint64_t scale_down_events  = 0;
        uint64_t check_iterations   = 0;
        uint64_t last_connection_count = 0;
    };

    // -----------------------------------------------------------------------
    // Type aliases
    // -----------------------------------------------------------------------

    /// Callable that returns the current number of active connections.
    using ConnectionCountProvider = std::function<uint64_t()>;

    /**
     * @brief Callback invoked when the recommended thread count changes.
     *
     * @param new_thread_count  The new recommended number of I/O threads.
     *
     * Called *outside* the internal mutex so the callback may freely
     * call back into the scaler (e.g., to read stats).
     */
    using ScaleCallback = std::function<void(size_t new_thread_count)>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @param config    Scaling parameters.
     * @param provider  Callable returning the current active-connection count.
     *                  Must remain valid for the lifetime of the scaler.
     */
    explicit AdaptiveIOScaler(const Config& config,
                              ConnectionCountProvider provider);
    ~AdaptiveIOScaler();

    AdaptiveIOScaler(const AdaptiveIOScaler&)            = delete;
    AdaptiveIOScaler& operator=(const AdaptiveIOScaler&) = delete;
    AdaptiveIOScaler(AdaptiveIOScaler&&)                 = delete;
    AdaptiveIOScaler& operator=(AdaptiveIOScaler&&)      = delete;

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Start the background monitoring thread.
     *
     * Safe to call multiple times; subsequent calls are no-ops.
     */
    void start();

    /**
     * @brief Stop the background monitoring thread and join it.
     *
     * Blocks until the monitor thread has exited.  Safe to call if
     * start() was never called.
     */
    void stop();

    // -----------------------------------------------------------------------
    // Scale callback
    // -----------------------------------------------------------------------

    /**
     * @brief Register a callback invoked whenever the thread count changes.
     *
     * Pass nullptr to clear an existing callback.  Can be called before or
     * after start().
     */
    void setScaleCallback(ScaleCallback cb);

    // -----------------------------------------------------------------------
    // Manual trigger (for testing / immediate reaction)
    // -----------------------------------------------------------------------

    /**
     * @brief Evaluate the scaling rules once synchronously.
     *
     * Useful in tests or for immediate reactions to a connection-count
     * spike without waiting for the next background poll interval.
     *
     * @return The (potentially updated) recommended thread count.
     */
    size_t evaluateNow();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Return the current recommended thread count.
     */
    size_t getCurrentThreadCount() const;

    /**
     * @brief Return a snapshot of scaling statistics.
     */
    Stats getStats() const;

    /**
     * @brief Return the configuration used to construct this scaler.
     */
    const Config& getConfig() const { return config_; }

private:
    Config                  config_;
    ConnectionCountProvider provider_;
    ScaleCallback           scale_cb_;

    mutable std::mutex mutex_;
    size_t  current_threads_;

    // Stats counters (updated under mutex_)
    uint64_t scale_up_events_   = 0;
    uint64_t scale_down_events_ = 0;
    uint64_t check_iterations_  = 0;
    uint64_t last_conn_count_   = 0;

    // Background thread
    std::thread        monitor_thread_;
    std::atomic<bool>  running_{false};

    void monitorLoop();
};

} // namespace network
} // namespace themis
