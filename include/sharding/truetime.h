/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            truetime.h                                         ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:38:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     262                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 2bbac9e442  2026-03-14  feat: implement Percolator-style distributed transaction ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <string>

namespace themis::sharding {

/**
 * @brief TrueTime interval representing a time with uncertainty bounds
 * 
 * Inspired by Google Spanner's TrueTime API, this provides a time interval
 * [earliest, latest] where the true time is guaranteed to be within this range.
 */
struct TTInterval {
    std::chrono::nanoseconds earliest;  // Lower bound of true time
    std::chrono::nanoseconds latest;    // Upper bound of true time
    
    TTInterval() 
        : earliest(0), latest(0) {}
    
    TTInterval(std::chrono::nanoseconds e, std::chrono::nanoseconds l)
        : earliest(e), latest(l) {}
    
    /**
     * @brief Get the uncertainty (epsilon) of this interval
     * @return The half-width of the uncertainty interval
     */
    std::chrono::nanoseconds uncertainty() const {
        return (latest - earliest) / 2;
    }
    
    /**
     * @brief Get the midpoint of the interval
     * @return The best estimate of the true time
     */
    std::chrono::nanoseconds midpoint() const {
        return earliest + uncertainty();
    }
    
    /**
     * @brief Check if this interval is definitely before another
     * @param other The other interval to compare
     * @return True if this.latest < other.earliest
     */
    bool definitelyBefore(const TTInterval& other) const {
        return latest < other.earliest;
    }
    
    /**
     * @brief Check if this interval is definitely after another
     * @param other The other interval to compare
     * @return True if this.earliest > other.latest
     */
    bool definitelyAfter(const TTInterval& other) const {
        return earliest > other.latest;
    }
    
    /**
     * @brief Check if intervals might overlap
     * @param other The other interval to compare
     * @return True if intervals could overlap
     */
    bool maybeOverlaps(const TTInterval& other) const {
        return !definitelyBefore(other) && !definitelyAfter(other);
    }
};

/**
 * @brief TrueTime clock providing time with uncertainty bounds
 * 
 * This implementation provides a TrueTime-inspired API with:
 * - Time intervals with uncertainty bounds
 * - Clock synchronization via NTP
 * - Drift detection and compensation
 * - Wait-until-certain operations for distributed transactions
 */
class TrueTime {
public:
    /**
     * @brief Configuration for TrueTime
     */
    struct Config {
        // Base uncertainty in microseconds (default: 1ms)
        uint64_t base_uncertainty_us = 1000;
        
        // Maximum allowed clock drift in microseconds (default: 100ms)
        uint64_t max_drift_us = 100000;
        
        // Clock sync interval in seconds (default: 30s)
        uint64_t sync_interval_s = 30;
        
        // NTP server addresses (empty = local system time only)
        std::vector<std::string> ntp_servers;
        
        // Enable aggressive wait optimization
        bool enable_wait_optimization = true;
    };
    
    /**
     * @brief Construct TrueTime with configuration
     * @param config TrueTime configuration
     */
    explicit TrueTime(const Config& config);
    
    /**
     * @brief Destructor - stops sync thread
     */
    ~TrueTime();
    
    // Prevent copying
    TrueTime(const TrueTime&) = delete;
    TrueTime& operator=(const TrueTime&) = delete;
    
    /**
     * @brief Get current time as an interval
     * @return TTInterval representing current time with uncertainty
     */
    TTInterval now() const;

    /**
     * @brief Get current time as an interval with explicit uncertainty bounds.
     *
     * Convenience alias for now() that emphasises the [earliest, latest] semantics
     * used by the Percolator commit-wait protocol:
     *
     *   auto tt = truetime->now_with_uncertainty();
     *   // commit_ts = tt.latest
     *   // wait until TT.now().earliest > commit_ts + max_uncertainty
     *
     * @return TTInterval with earliest = now - epsilon, latest = now + epsilon
     */
    TTInterval now_with_uncertainty() const;
    
    /**
     * @brief Wait until a specific timestamp is definitely in the past
     * 
     * This is the key operation for distributed transactions:
     * After calling waitUntil(t), we know that t < TT.now().earliest
     * 
     * @param timestamp The timestamp to wait for
     */
    void waitUntil(std::chrono::nanoseconds timestamp);
    
    /**
     * @brief Get the current uncertainty bound
     * @return Current epsilon (half-width of uncertainty interval)
     */
    std::chrono::nanoseconds getUncertainty() const;
    
    /**
     * @brief Get clock drift since last sync
     * @return Estimated drift in nanoseconds
     */
    std::chrono::nanoseconds getDrift() const;
    
    /**
     * @brief Force a clock synchronization
     * @return True if sync was successful
     */
    bool syncNow();
    
    /**
     * @brief Get statistics about clock sync
     * @return JSON with sync stats (last sync time, drift, etc.)
     */
    std::string getStats() const;
    
    /**
     * @brief Start background clock sync thread
     */
    void startSyncThread();
    
    /**
     * @brief Stop background clock sync thread
     */
    void stopSyncThread();

private:
    Config config_;
    
    // Current uncertainty estimate (epsilon)
    mutable std::atomic<uint64_t> uncertainty_ns_;
    
    // Estimated clock drift
    mutable std::atomic<int64_t> drift_ns_;
    
    // Last sync timestamp
    mutable std::atomic<uint64_t> last_sync_ns_;
    
    // Sync thread control
    std::atomic<bool> sync_thread_running_{false};
    std::thread sync_thread_;
    
    mutable std::mutex mutex_;
    
    /**
     * @brief Get raw system time
     * @return System time in nanoseconds since epoch
     */
    std::chrono::nanoseconds getSystemTime() const;
    
    /**
     * @brief Synchronize with NTP servers
     * Updates drift and uncertainty estimates
     * @return True if sync was successful
     */
    bool performSync();
    
    /**
     * @brief Query a single NTP server
     * @param server NTP server address
     * @param offset Output parameter for clock offset
     * @return True if query was successful
     */
    bool queryNTPServer(const std::string& server, int64_t& offset);
    
    /**
     * @brief Calculate uncertainty based on time since last sync
     * @return Current uncertainty in nanoseconds
     */
    uint64_t calculateUncertainty() const;
    
    /**
     * @brief Background thread for periodic clock sync
     */
    void syncThreadFunc();
};

} // namespace themis::sharding
