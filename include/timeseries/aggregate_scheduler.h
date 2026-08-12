/**
 * @file aggregate_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "timeseries/continuous_agg.h"  // for AggConfig, ContinuousAggWatermarkStore
#include "timeseries/timeseries_metrics.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>

namespace themis {

class TSStore;
class ContinuousAggregateManager;

/**
 * Automatic scheduler for continuous aggregates.
 * 
 * Runs background thread that periodically refreshes pre-configured
 * continuous aggregates (rollups) for time-series data.
 * 
 * Features:
 * - Configurable refresh intervals per aggregate
 * - Automatic catch-up for missed windows
 * - Parallel refresh for independent metrics
 * - Graceful shutdown with flush
 * - Health monitoring and error tracking
 * 
 * Usage:
 *   AggregateScheduler scheduler(tsstore);
 *   
 *   // Register aggregates
 *   scheduler.registerAggregate({
 *       .metric = "cpu_usage",
 *       .entity = "server01",
 *       .window = {std::chrono::minutes(1)},
 *       .refresh_interval = std::chrono::minutes(5)
 *   });
 *   
 *   scheduler.start();
 *   // ... scheduler runs in background ...
 *   scheduler.stop();
 */
class AggregateScheduler {
public:
    struct ScheduledAggregate {
        std::string id;  // Unique identifier for this aggregate
        AggConfig config;
        std::chrono::milliseconds refresh_interval{std::chrono::minutes(5)};
        int64_t last_refresh_ms = 0;  // Timestamp of last refresh
        bool enabled = true;
        
        // Incremental refresh: when true, refreshAggregate() uses the
        // watermark-based scan path via ContinuousAggregateManager::refreshIncremental().
        bool use_incremental_refresh = true;

        // Statistics
        size_t total_refreshes = 0;
        size_t failed_refreshes = 0;
        double avg_refresh_time_ms = 0.0;
    };
    
    struct Config {
        size_t max_parallel_refreshes = 4;  // Max concurrent refresh operations
        std::chrono::milliseconds check_interval{std::chrono::seconds(30)};  // Scheduler loop interval
        bool catch_up_missed_windows = true;  // Refresh missed windows on startup
        size_t max_catch_up_windows = 100;  // Max windows to catch up
    };
    
    explicit AggregateScheduler(TSStore* store);
    AggregateScheduler(TSStore* store, const Config& config);
    ~AggregateScheduler();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }
    
    // Aggregate management
    /**
     * Register a continuous aggregate for automatic refresh
     * @param agg Scheduled aggregate configuration
     */
    void registerAggregate(const ScheduledAggregate& agg);
    
    /**
     * Register a continuous aggregate from basic config
     * @param config Aggregate configuration
     * @param refresh_interval How often to refresh (default: 5 minutes)
     * @return Aggregate ID
     */
    std::string registerAggregate(const AggConfig& config, std::chrono::milliseconds refresh_interval = std::chrono::minutes(5));
    
    void unregisterAggregate(const std::string& id);
    void enableAggregate(const std::string& id);
    void disableAggregate(const std::string& id);
    
    // Manual operations
    void refreshNow(const std::string& id);  // Force refresh immediately
    void refreshAll();  // Force refresh all aggregates
    
    // Statistics
    struct Stats {
        size_t registered_aggregates = 0;
        size_t active_aggregates = 0;
        size_t total_refreshes = 0;
        size_t failed_refreshes = 0;
        std::chrono::system_clock::time_point last_run;
        std::chrono::system_clock::time_point next_run;
    };
    
    Stats getStats() const;
    std::vector<ScheduledAggregate> listAggregates() const;

    /**
     * @brief Attach a TimeSeriesMetrics collector to receive per-aggregate
     *        refresh latency and lag measurements.
     *
     * Must be called before start() to ensure all refreshes are recorded.
     * Passing nullptr disables metric recording.
     *
     * @param metrics Shared pointer to the metrics collector (may be null)
     */
    void setMetrics(std::shared_ptr<TimeSeriesMetrics> metrics);

    /**
     * @brief Backfill a specific time range for a registered aggregate.
     *
     * Processes the range [start_ms, end_ms) without touching the watermark,
     * so this can be used to recover from gaps in watermark history.
     *
     * @param agg_id   Aggregate ID as returned by registerAggregate()
     * @param start_ms Range start (ms since epoch, inclusive)
     * @param end_ms   Range end (ms since epoch, exclusive)
     */
    void backfill_range(const std::string& agg_id, int64_t start_ms, int64_t end_ms);

private:
    TSStore* store_;
    Config config_;
    std::unique_ptr<ContinuousAggregateManager> agg_manager_;
    std::unique_ptr<ContinuousAggWatermarkStore> wm_store_;
    std::shared_ptr<TimeSeriesMetrics> metrics_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread scheduler_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // Aggregates
    std::map<std::string, ScheduledAggregate> aggregates_;
    
    // Statistics
    std::atomic<size_t> total_refreshes_{0};
    std::atomic<size_t> failed_refreshes_{0};
    std::chrono::system_clock::time_point last_run_;
    
    // Scheduler loop
    void schedulerLoop();
    void refreshAggregate(ScheduledAggregate& agg);
    bool needsRefresh(const ScheduledAggregate& agg, int64_t current_time_ms) const;
    void catchUpMissedWindows(ScheduledAggregate& agg, int64_t current_time_ms);
    
    // Helpers
    int64_t getCurrentTimeMs() const;
    std::string generateAggregateId(const AggConfig& config) const;
};

} // namespace themis
