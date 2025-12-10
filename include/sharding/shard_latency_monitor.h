/**
 * ThemisDB Shard Network Latency Monitor
 * 
 * Measures network latency between shards using ping-back mechanism.
 * Uses measurements to improve clock synchronization accuracy.
 * 
 * Features:
 * - Active ping-back between shards
 * - RTT (Round-Trip Time) measurement
 * - Moving average and percentile calculation
 * - Integration with TrueTime clock for uncertainty adjustment
 * - Prometheus metrics export
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sharding/truetime_clock.h"
#include "sharding/shard_topology.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <deque>
#include <optional>

namespace themis::sharding {

/**
 * Network latency measurement for a single shard
 */
struct ShardLatencyMeasurement {
    std::string shard_id;
    uint64_t rtt_us;                    // Round-trip time in microseconds
    uint64_t one_way_us;                // Estimated one-way latency (RTT/2)
    std::chrono::system_clock::time_point measured_at;
    bool success;
    std::string error_msg;
};

/**
 * Aggregated latency statistics for a shard
 */
struct ShardLatencyStats {
    std::string shard_id;
    
    // RTT statistics (microseconds)
    uint64_t min_rtt_us = 0;
    uint64_t max_rtt_us = 0;
    uint64_t avg_rtt_us = 0;
    uint64_t median_rtt_us = 0;
    uint64_t p95_rtt_us = 0;
    uint64_t p99_rtt_us = 0;
    
    // One-way latency estimates
    uint64_t avg_one_way_us = 0;
    uint64_t p95_one_way_us = 0;
    
    // Measurement statistics
    uint64_t total_pings = 0;
    uint64_t successful_pings = 0;
    uint64_t failed_pings = 0;
    double success_rate = 0.0;
    
    // Clock offset estimation
    int64_t estimated_clock_offset_us = 0;
    uint64_t offset_uncertainty_us = 0;
    
    // Last measurement
    std::chrono::system_clock::time_point last_ping_at;
    bool is_reachable = false;
};

/**
 * Ping request/response payload
 */
struct PingMessage {
    std::string sender_id;              // Originating shard
    std::string target_id;              // Target shard
    uint64_t sequence_number;           // Monotonic sequence
    TrueTimeStamp sender_timestamp;     // Sender's TrueTime when sent
    bool is_response;                   // false=request, true=response
    
    // Response-only fields
    TrueTimeStamp receiver_timestamp;   // Receiver's TrueTime when received
    TrueTimeStamp response_timestamp;   // Receiver's TrueTime when responding
    
    // Serialize to JSON
    std::string toJson() const;
    static std::optional<PingMessage> fromJson(const std::string& json);
};

/**
 * Configuration for latency monitor
 */
struct LatencyMonitorConfig {
    std::string local_shard_id;
    
    // Ping intervals
    uint32_t ping_interval_ms = 10000;      // Ping every 10 seconds
    uint32_t ping_timeout_ms = 5000;        // Timeout for ping response
    
    // Statistics
    uint32_t history_size = 100;            // Keep last 100 measurements
    bool enable_percentiles = true;         // Calculate P95/P99
    
    // Integration with TrueTime
    bool adjust_truetime_uncertainty = true; // Adjust TrueTime based on network latency
    double uncertainty_multiplier = 1.5;    // Multiply avg RTT by this for uncertainty
    
    // HTTP endpoint
    std::string ping_endpoint_path = "/shard/ping";
    bool use_mtls = true;                   // Use mTLS for ping requests
};

/**
 * Shard Latency Monitor
 * 
 * Continuously monitors network latency to other shards using
 * ping-back mechanism. Provides statistics for clock synchronization.
 */
class ShardLatencyMonitor {
public:
    /**
     * Construct latency monitor
     * @param config Monitor configuration
     * @param topology Shard topology (to discover peers)
     * @param truetime_clock Optional TrueTime clock to adjust uncertainty
     */
    ShardLatencyMonitor(
        const LatencyMonitorConfig& config,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<TrueTimeClock> truetime_clock = nullptr
    );
    
    /**
     * Destructor - stops monitoring
     */
    ~ShardLatencyMonitor();
    
    // Disable copy
    ShardLatencyMonitor(const ShardLatencyMonitor&) = delete;
    ShardLatencyMonitor& operator=(const ShardLatencyMonitor&) = delete;
    
    /**
     * Start background latency monitoring
     */
    bool start();
    
    /**
     * Stop background monitoring
     */
    void stop();
    
    /**
     * Check if monitor is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * Manually ping a specific shard
     * @param shard_id Target shard ID
     * @return Measurement result
     */
    std::optional<ShardLatencyMeasurement> pingShard(const std::string& shard_id);
    
    /**
     * Handle incoming ping request (called by HTTP handler)
     * @param request Ping request message
     * @return Ping response message
     */
    PingMessage handlePingRequest(const PingMessage& request);
    
    /**
     * Handle incoming ping response (internal)
     * @param response Ping response message
     */
    void handlePingResponse(const PingMessage& response);
    
    /**
     * Get latency statistics for a specific shard
     * @param shard_id Target shard ID
     * @return Statistics, or nullopt if no data
     */
    std::optional<ShardLatencyStats> getStats(const std::string& shard_id) const;
    
    /**
     * Get latency statistics for all known shards
     * @return Map of shard_id -> statistics
     */
    std::map<std::string, ShardLatencyStats> getAllStats() const;
    
    /**
     * Get average network latency across all shards
     * @return Average one-way latency in microseconds
     */
    uint64_t getAverageNetworkLatency() const;
    
    /**
     * Get maximum network latency across all shards
     * @return Maximum one-way latency in microseconds
     */
    uint64_t getMaxNetworkLatency() const;
    
    /**
     * Estimate clock offset to a specific shard
     * Uses Cristian's algorithm with multiple measurements
     * 
     * @param shard_id Target shard ID
     * @return Estimated offset in microseconds (+ means target is ahead)
     */
    std::optional<int64_t> estimateClockOffset(const std::string& shard_id) const;
    
    /**
     * Export Prometheus metrics
     */
    std::string exportPrometheusMetrics() const;
    
private:
    LatencyMonitorConfig config_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<TrueTimeClock> truetime_clock_;
    std::atomic<bool> running_{false};
    
    // Measurement history per shard
    struct ShardHistory {
        std::deque<ShardLatencyMeasurement> measurements;
        std::atomic<uint64_t> sequence_number{0};
        std::mutex mutex;
    };
    std::map<std::string, ShardHistory> shard_history_;
    mutable std::mutex history_mutex_;
    
    // Pending ping requests (sequence -> start time)
    struct PendingPing {
        uint64_t sequence;
        std::string target_shard;
        std::chrono::steady_clock::time_point sent_at;
        TrueTimeStamp sent_timestamp;
    };
    std::map<uint64_t, PendingPing> pending_pings_;
    mutable std::mutex pending_mutex_;
    std::atomic<uint64_t> next_sequence_{1};
    
    // Background monitoring thread
    std::unique_ptr<std::thread> monitor_thread_;
    std::atomic<bool> monitor_stop_{false};
    
    // Internal methods
    void monitorLoop();
    void pingAllShards();
    bool sendPingRequest(const std::string& shard_id, PingMessage& request);
    
    void recordMeasurement(const std::string& shard_id, const ShardLatencyMeasurement& measurement);
    ShardLatencyStats calculateStats(const std::string& shard_id) const;
    
    void updateTrueTimeUncertainty();
    uint64_t calculatePercentile(const std::vector<uint64_t>& values, double percentile) const;
};

/**
 * Helper class for integrating latency measurements with TrueTime
 */
class LatencyAwareTrueTime {
public:
    /**
     * Create a latency-aware TrueTime clock
     * @param truetime_clock Base TrueTime clock
     * @param latency_monitor Latency monitor for network measurements
     */
    LatencyAwareTrueTime(
        std::shared_ptr<TrueTimeClock> truetime_clock,
        std::shared_ptr<ShardLatencyMonitor> latency_monitor
    );
    
    /**
     * Get current time with network-adjusted uncertainty
     * Adds network latency to uncertainty bound
     */
    TrueTimeStamp now();
    
    /**
     * Get timestamp for cross-shard operation to a specific shard
     * Adjusts uncertainty based on network latency to that shard
     * 
     * @param target_shard_id Target shard ID
     * @return Timestamp with adjusted uncertainty
     */
    TrueTimeStamp nowForShard(const std::string& target_shard_id);
    
    /**
     * Wait for timestamp considering network latency
     * @param ts Timestamp to wait for
     * @param target_shard_id Optional target shard (for adjusted wait)
     */
    bool waitUntilPast(
        const TrueTimeStamp& ts,
        const std::string& target_shard_id = ""
    );
    
private:
    std::shared_ptr<TrueTimeClock> truetime_clock_;
    std::shared_ptr<ShardLatencyMonitor> latency_monitor_;
};

} // namespace themis::sharding
