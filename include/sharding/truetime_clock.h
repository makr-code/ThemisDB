/**
 * ThemisDB TrueTime-Inspired Clock
 * 
 * Provides globally synchronized timestamps with uncertainty bounds,
 * inspired by Google Spanner's TrueTime API.
 * 
 * Key Features:
 * - Physical clock synchronization (NTP/PTP)
 * - Uncertainty interval tracking
 * - Hybrid Logical Clock (HLC) integration
 * - External consistency guarantees
 * - Commit-wait protocol support
 * 
 * Unlike Google's TrueTime which uses GPS and atomic clocks,
 * this implementation uses:
 * - NTP/PTP for clock synchronization
 * - Statistical models for uncertainty bounds
 * - HLC for ordering within uncertainty bounds
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <chrono>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <memory>

namespace themis::sharding {

/**
 * TrueTime Timestamp with uncertainty interval
 * 
 * Represents a time interval [earliest, latest] within which
 * the actual time is guaranteed to lie.
 */
struct TrueTimeStamp {
    // Physical time in microseconds since epoch
    uint64_t earliest_us;  // Lower bound (t.earliest)
    uint64_t latest_us;    // Upper bound (t.latest)
    
    // Logical component for ordering within uncertainty
    uint64_t logical;      // HLC logical counter
    std::string node_id;   // Originating node
    
    /**
     * Get the midpoint of the uncertainty interval
     */
    uint64_t midpoint() const {
        return (earliest_us + latest_us) / 2;
    }
    
    /**
     * Get the uncertainty (epsilon) in microseconds
     */
    uint64_t uncertainty() const {
        return (latest_us - earliest_us) / 2;
    }
    
    /**
     * Check if this timestamp is before another (accounting for uncertainty)
     */
    bool definitelyBefore(const TrueTimeStamp& other) const {
        return latest_us < other.earliest_us;
    }
    
    /**
     * Check if this timestamp is after another (accounting for uncertainty)
     */
    bool definitelyAfter(const TrueTimeStamp& other) const {
        return earliest_us > other.latest_us;
    }
    
    /**
     * Check if timestamps might overlap (uncertain ordering)
     */
    bool overlaps(const TrueTimeStamp& other) const {
        return !(definitelyBefore(other) || definitelyAfter(other));
    }
    
    /**
     * Compare timestamps for ordering
     * Returns: -1 if this < other, 0 if concurrent/uncertain, 1 if this > other
     */
    int compare(const TrueTimeStamp& other) const;
    
    /**
     * Serialize to JSON string
     */
    std::string toJson() const;
    
    /**
     * Deserialize from JSON string
     */
    static std::optional<TrueTimeStamp> fromJson(const std::string& json);
    
    // Operators
    bool operator<(const TrueTimeStamp& other) const;
    bool operator>(const TrueTimeStamp& other) const;
    bool operator==(const TrueTimeStamp& other) const;
};

/**
 * Clock Synchronization Source
 */
enum class ClockSource {
    SYSTEM_CLOCK,      // Basic system clock (no sync)
    NTP,               // Network Time Protocol
    PTP,               // Precision Time Protocol (IEEE 1588)
    GPS,               // GPS time source (if available)
    ATOMIC             // Atomic clock (if available)
};

/**
 * TrueTime Clock Configuration
 */
struct TrueTimeConfig {
    std::string node_id;
    
    // Clock source
    ClockSource source = ClockSource::SYSTEM_CLOCK;
    std::string ntp_server = "pool.ntp.org";
    std::string ptp_interface = "eth0";
    
    // Uncertainty bounds
    uint64_t base_uncertainty_us = 100;        // Base uncertainty (100µs)
    uint64_t max_uncertainty_us = 10000;       // Max uncertainty (10ms)
    uint64_t drift_rate_ppm = 200;             // Clock drift (200 parts per million)
    
    // Synchronization
    uint32_t sync_interval_ms = 30000;         // Sync every 30s
    uint32_t sync_timeout_ms = 5000;           // Sync timeout
    
    // Commit-wait
    bool enable_commit_wait = true;            // Enable external consistency
    uint64_t commit_wait_multiplier = 2;       // Wait multiplier (2x uncertainty)
};

/**
 * Clock Synchronization Statistics
 */
struct ClockSyncStats {
    uint64_t sync_count = 0;                   // Number of syncs
    uint64_t sync_failures = 0;                // Failed syncs
    uint64_t last_sync_us = 0;                 // Last sync time
    int64_t clock_offset_us = 0;               // Current offset from reference
    uint64_t current_uncertainty_us = 0;       // Current uncertainty
    uint64_t max_observed_skew_us = 0;         // Maximum observed skew
    double drift_rate_ppm = 0.0;               // Measured drift rate
    std::string sync_source;                   // Current sync source
};

/**
 * TrueTime Clock Manager
 * 
 * Manages a TrueTime-style clock with uncertainty tracking.
 * Provides timestamps for distributed transactions with
 * guaranteed ordering properties.
 */
class TrueTimeClock {
public:
    /**
     * Construct TrueTime clock
     * @param config Clock configuration
     */
    explicit TrueTimeClock(const TrueTimeConfig& config);
    
    /**
     * Destructor - stops background sync
     */
    ~TrueTimeClock();
    
    // Disable copy
    TrueTimeClock(const TrueTimeClock&) = delete;
    TrueTimeClock& operator=(const TrueTimeClock&) = delete;
    
    /**
     * Start clock synchronization
     */
    bool start();
    
    /**
     * Stop clock synchronization
     */
    void stop();
    
    /**
     * Get current time with uncertainty bounds
     * Equivalent to Spanner's TT.now()
     */
    TrueTimeStamp now();
    
    /**
     * Get timestamp after a specific physical time
     * Ensures timestamp is after the given time
     */
    TrueTimeStamp after(uint64_t physical_us);
    
    /**
     * Wait until timestamp is guaranteed to be in the past
     * Implements commit-wait protocol for external consistency
     * 
     * @param ts Timestamp to wait for
     * @return true if wait successful, false if timeout
     */
    bool waitUntilPast(const TrueTimeStamp& ts);
    
    /**
     * Update clock based on received timestamp from another node
     * Implements HLC update semantics
     */
    TrueTimeStamp receive(const TrueTimeStamp& received);
    
    /**
     * Force clock synchronization now
     * @return true if sync successful
     */
    bool syncNow();
    
    /**
     * Get current uncertainty interval size
     */
    uint64_t getCurrentUncertainty() const;
    
    /**
     * Get synchronization statistics
     */
    ClockSyncStats getStats() const;
    
    /**
     * Export Prometheus metrics
     */
    std::string exportPrometheusMetrics() const;
    
private:
    TrueTimeConfig config_;
    std::atomic<bool> running_{false};
    
    // Clock state
    std::atomic<uint64_t> last_physical_us_{0};
    std::atomic<uint64_t> logical_counter_{0};
    std::atomic<uint64_t> current_uncertainty_us_{0};
    std::atomic<int64_t> clock_offset_us_{0};
    
    // Synchronization state
    std::atomic<uint64_t> last_sync_us_{0};
    std::atomic<uint64_t> sync_count_{0};
    std::atomic<uint64_t> sync_failures_{0};
    std::atomic<uint64_t> max_observed_skew_us_{0};
    std::atomic<double> drift_rate_ppm_{0.0};
    
    // Thread safety
    mutable std::mutex clock_mutex_;
    
    // Background sync thread
    std::unique_ptr<std::thread> sync_thread_;
    std::atomic<bool> sync_thread_stop_{false};
    
    // Internal methods
    void syncLoop();
    bool performSync();
    bool syncWithNTP();
    bool syncWithPTP();
    bool syncWithGPS();
    
    uint64_t getPhysicalTimeUs() const;
    uint64_t calculateUncertainty() const;
    void updateDriftRate(int64_t offset_us, uint64_t elapsed_us);
};

/**
 * Commit-Wait Helper
 * 
 * Helps implement external consistency by waiting for
 * timestamps to become definite.
 */
class CommitWaitHelper {
public:
    /**
     * Wait for commit timestamp to be in the past
     * 
     * @param clock TrueTime clock
     * @param commit_ts Commit timestamp
     * @param timeout_ms Maximum wait time
     * @return true if wait successful, false if timeout
     */
    static bool waitForCommit(
        TrueTimeClock& clock,
        const TrueTimeStamp& commit_ts,
        uint32_t timeout_ms = 100
    );
    
    /**
     * Calculate required commit-wait duration
     */
    static uint64_t calculateWaitDuration(
        const TrueTimeStamp& commit_ts,
        const TrueTimeStamp& now_ts
    );
};

} // namespace themis::sharding
