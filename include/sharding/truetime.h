/**
 * @file truetime.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
        /** @brief Base epsilon contribution in microseconds after successful sync. */
        // Base uncertainty in microseconds (default: 1ms)
        uint64_t base_uncertainty_us = 1000;
        
        /** @brief Upper cap for uncertainty growth in microseconds. */
        // Maximum allowed clock drift in microseconds (default: 100ms)
        uint64_t max_drift_us = 100000;
        
        /** @brief Periodic synchronization cadence in seconds. */
        // Clock sync interval in seconds (default: 30s)
        uint64_t sync_interval_s = 30;
        
        /** @brief NTP server hostnames/addresses used for offset sampling. */
        // NTP server addresses (empty = local system time only)
        std::vector<std::string> ntp_servers;
        
        /** @brief Enable chunked waiting for improved interruption responsiveness. */
        // Enable aggressive wait optimization
        bool enable_wait_optimization = true;
    };
    
    /**
     * @brief Construct TrueTime with configuration
     * @param config TrueTime configuration
     */
    explicit TrueTime(const Config& config);
    
    /** @brief Destructor; stops synchronization thread if active. */
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
    
    /** @brief Force immediate clock synchronization attempt.
     *  @return true when synchronization succeeded or local fallback was applied.
     */
    bool syncNow();
    
    /** @brief Return JSON statistics for sync/uncertainty state. */
    std::string getStats() const;
    
    /** @brief Start background clock synchronization thread. */
    void startSyncThread();
    
    /** @brief Stop background clock synchronization thread. */
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
