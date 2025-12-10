/**
 * ThemisDB Distributed Transaction Metrics
 * 
 * Comprehensive Prometheus metrics for:
 * - TrueTime clock synchronization
 * - Shard network latency
 * - Distributed transactions (2PC)
 * - SAGA patterns
 * - Transaction timestamps and ordering
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sharding/truetime_clock.h"
#include "sharding/shard_latency_monitor.h"
#include "sharding/distributed_transaction.h"
#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include <map>
#include <vector>
#include <mutex>

namespace themis::sharding::metrics {

/**
 * Histogram bucket for latency measurements
 */
struct HistogramBucket {
    double upper_bound;     // Upper bound in milliseconds
    uint64_t count;         // Number of observations <= upper_bound
};

/**
 * Transaction latency histogram
 */
class TransactionLatencyHistogram {
public:
    TransactionLatencyHistogram();
    
    /**
     * Record a transaction duration
     * @param duration_ms Duration in milliseconds
     */
    void observe(double duration_ms);
    
    /**
     * Get histogram buckets
     */
    std::vector<HistogramBucket> getBuckets() const;
    
    /**
     * Get count and sum for Prometheus
     */
    std::pair<uint64_t, double> getCountAndSum() const;
    
    /**
     * Reset histogram
     */
    void reset();
    
private:
    // Prometheus-compatible buckets (in milliseconds)
    // 1ms, 5ms, 10ms, 25ms, 50ms, 100ms, 250ms, 500ms, 1s, 2.5s, 5s, 10s, +Inf
    static constexpr double BUCKET_BOUNDS[] = {
        1.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0,
        1000.0, 2500.0, 5000.0, 10000.0
    };
    
    std::vector<std::atomic<uint64_t>> bucket_counts_;
    std::atomic<uint64_t> total_count_{0};
    std::atomic<double> total_sum_{0.0};
    mutable std::mutex mutex_;
};

/**
 * Comprehensive metrics for distributed transactions
 */
class DistributedTransactionMetrics {
public:
    DistributedTransactionMetrics(const std::string& local_shard_id);
    
    // Transaction lifecycle metrics
    void recordTransactionStart(IsolationLevel isolation);
    void recordTransactionCommit(uint64_t duration_ms);
    void recordTransactionAbort(const std::string& reason);
    void recordTransactionTimeout();
    
    // 2PC metrics
    void recordPreparePhase(uint64_t duration_ms, bool success);
    void recordCommitPhase(uint64_t duration_ms, bool success);
    void recordParticipantPrepared(const std::string& shard_id, uint64_t duration_ms);
    void recordParticipantCommitted(const std::string& shard_id, uint64_t duration_ms);
    void recordParticipantFailed(const std::string& shard_id, const std::string& phase);
    
    // SAGA metrics
    void recordSagaStart();
    void recordSagaComplete(uint64_t duration_ms, uint64_t steps_count);
    void recordSagaCompensation(uint64_t steps_compensated);
    void recordSagaStepExecution(const std::string& step_id, uint64_t duration_ms, bool success);
    void recordSagaStepCompensation(const std::string& step_id, uint64_t duration_ms, bool success);
    
    // Commit-wait metrics
    void recordCommitWait(uint64_t wait_duration_us);
    void recordCommitWaitTimeout();
    
    // Deadlock detection metrics
    void recordDeadlockDetected();
    void recordDeadlockAbort(const std::string& txn_id);
    
    // Timestamp metrics
    void recordTimestampUncertainty(uint64_t uncertainty_us);
    void recordTimestampSkew(int64_t skew_us);
    void recordConcurrentTransactions(const std::string& txn_id1, const std::string& txn_id2);
    
    // Network-aware metrics
    void recordNetworkAwareCommitWait(const std::string& shard_id, uint64_t network_latency_us);
    void recordCrossShardTransaction(uint64_t shard_count);
    
    // Active transaction tracking
    void incrementActiveTransactions();
    void decrementActiveTransactions();
    void setActiveTransactionsByIsolation(IsolationLevel isolation, uint64_t count);
    
    /**
     * Export all metrics in Prometheus format
     */
    std::string exportPrometheusMetrics() const;
    
private:
    std::string local_shard_id_;
    
    // Transaction counters
    std::atomic<uint64_t> txn_started_total_{0};
    std::atomic<uint64_t> txn_committed_total_{0};
    std::atomic<uint64_t> txn_aborted_total_{0};
    std::atomic<uint64_t> txn_timeout_total_{0};
    
    // Counters by isolation level
    std::map<IsolationLevel, std::atomic<uint64_t>> txn_by_isolation_;
    mutable std::mutex isolation_mutex_;
    
    // 2PC counters
    std::atomic<uint64_t> prepare_success_total_{0};
    std::atomic<uint64_t> prepare_failure_total_{0};
    std::atomic<uint64_t> commit_success_total_{0};
    std::atomic<uint64_t> commit_failure_total_{0};
    
    // SAGA counters
    std::atomic<uint64_t> saga_started_total_{0};
    std::atomic<uint64_t> saga_completed_total_{0};
    std::atomic<uint64_t> saga_compensated_total_{0};
    std::atomic<uint64_t> saga_steps_executed_total_{0};
    std::atomic<uint64_t> saga_steps_compensated_total_{0};
    
    // Commit-wait counters
    std::atomic<uint64_t> commit_wait_total_{0};
    std::atomic<uint64_t> commit_wait_timeout_total_{0};
    
    // Deadlock counters
    std::atomic<uint64_t> deadlock_detected_total_{0};
    std::atomic<uint64_t> deadlock_abort_total_{0};
    
    // Concurrent transaction counter
    std::atomic<uint64_t> concurrent_txn_total_{0};
    
    // Cross-shard transaction counter
    std::atomic<uint64_t> cross_shard_txn_total_{0};
    
    // Active transaction gauge
    std::atomic<int64_t> active_transactions_{0};
    
    // Histograms
    TransactionLatencyHistogram txn_duration_histogram_;
    TransactionLatencyHistogram prepare_duration_histogram_;
    TransactionLatencyHistogram commit_duration_histogram_;
    TransactionLatencyHistogram saga_duration_histogram_;
    TransactionLatencyHistogram commit_wait_histogram_;
    
    // Per-shard participant metrics
    struct ParticipantMetrics {
        std::atomic<uint64_t> prepare_count{0};
        std::atomic<uint64_t> commit_count{0};
        std::atomic<uint64_t> failure_count{0};
        TransactionLatencyHistogram prepare_histogram;
        TransactionLatencyHistogram commit_histogram;
    };
    std::map<std::string, ParticipantMetrics> participant_metrics_;
    mutable std::mutex participant_mutex_;
    
    // Abort reasons tracking
    std::map<std::string, std::atomic<uint64_t>> abort_reasons_;
    mutable std::mutex abort_mutex_;
    
    // Helper methods
    std::string formatHistogram(
        const TransactionLatencyHistogram& histogram,
        const std::string& metric_name,
        const std::string& labels = ""
    ) const;
    
    std::string isolationLevelToString(IsolationLevel level) const;
};

/**
 * Combined metrics aggregator
 * 
 * Aggregates metrics from all components:
 * - TrueTime clock
 * - Shard latency monitor
 * - Distributed transaction coordinator
 */
class ShardingMetricsAggregator {
public:
    ShardingMetricsAggregator(
        const std::string& local_shard_id,
        std::shared_ptr<TrueTimeClock> truetime_clock,
        std::shared_ptr<ShardLatencyMonitor> latency_monitor,
        std::shared_ptr<DistributedTransactionMetrics> txn_metrics
    );
    
    /**
     * Export all metrics in single Prometheus response
     * Includes:
     * - TrueTime clock metrics
     * - Network latency metrics
     * - Transaction metrics
     * - Aggregated cluster health
     */
    std::string exportAllMetrics() const;
    
    /**
     * Get summary statistics for monitoring dashboard
     */
    struct Summary {
        // Clock
        uint64_t clock_uncertainty_us;
        int64_t clock_offset_us;
        uint64_t max_clock_skew_us;
        
        // Network
        uint64_t avg_network_latency_us;
        uint64_t max_network_latency_us;
        uint64_t unreachable_shards;
        
        // Transactions
        uint64_t active_transactions;
        uint64_t txn_commit_rate;      // per second
        double txn_success_rate;        // percentage
        uint64_t avg_commit_wait_us;
        
        // Health
        bool cluster_healthy;
        std::vector<std::string> warnings;
    };
    
    Summary getSummary() const;
    
    /**
     * Check cluster health based on metrics
     * Returns health status and warnings
     */
    std::pair<bool, std::vector<std::string>> checkHealth() const;
    
private:
    std::string local_shard_id_;
    std::shared_ptr<TrueTimeClock> truetime_clock_;
    std::shared_ptr<ShardLatencyMonitor> latency_monitor_;
    std::shared_ptr<DistributedTransactionMetrics> txn_metrics_;
    
    // Cache for rate calculations
    mutable std::atomic<uint64_t> last_txn_count_{0};
    mutable std::atomic<uint64_t> last_check_time_{0};
};

/**
 * Metrics HTTP handler
 * 
 * Provides /metrics endpoint for Prometheus scraping
 */
class MetricsHttpHandler {
public:
    MetricsHttpHandler(std::shared_ptr<ShardingMetricsAggregator> aggregator);
    
    /**
     * Handle HTTP GET request to /metrics
     * @return Prometheus-formatted metrics response
     */
    std::string handleMetricsRequest() const;
    
    /**
     * Get metrics with specific labels
     * @param labels Additional Prometheus labels
     */
    std::string handleMetricsRequest(const std::map<std::string, std::string>& labels) const;
    
private:
    std::shared_ptr<ShardingMetricsAggregator> aggregator_;
};

/**
 * Grafana dashboard configuration generator
 */
class GrafanaDashboardGenerator {
public:
    /**
     * Generate Grafana dashboard JSON for distributed transactions
     * 
     * Includes panels for:
     * - Transaction throughput and latency
     * - Clock synchronization status
     * - Network latency heatmap
     * - SAGA execution tracking
     * - Alert status
     */
    static std::string generateDashboard(const std::string& prometheus_datasource = "Prometheus");
    
    /**
     * Generate alert rules for Prometheus Alertmanager
     */
    static std::string generateAlertRules();
};

} // namespace themis::sharding::metrics
