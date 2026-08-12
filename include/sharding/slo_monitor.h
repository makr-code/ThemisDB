/**
 * @file slo_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>

namespace themis {
namespace sharding {

/**
 * Service Level Objective (SLO) definitions for sharding system.
 * Tracks availability, latency, durability, and consistency metrics.
 */
struct SLOTarget {
    // Availability SLO
    double availability_target = 0.9999;  // 99.99% (52.6 minutes/year downtime)
    
    // Latency SLOs (milliseconds)
    double single_shard_query_p50_ms = 10.0;
    double single_shard_query_p99_ms = 50.0;
    double cross_shard_query_p50_ms = 50.0;
    double cross_shard_query_p99_ms = 200.0;
    double cross_shard_transaction_p50_ms = 100.0;
    double cross_shard_transaction_p99_ms = 500.0;
    
    // Durability SLO
    double data_loss_tolerance = 0.0;  // RPO = 0 (no data loss)
    int min_replicas = 3;
    
    // Consistency SLO
    double max_leader_election_time_s = 5.0;
    double max_replication_lag_ms = 100.0;
};

/**
 * SLO measurement window for calculating compliance.
 */
class SLOWindow {
public:
    /**
     * @brief Create an SLO measurement window.
     * @param window_duration Rolling aggregation window duration.
     */
    explicit SLOWindow(std::chrono::seconds window_duration = std::chrono::seconds(3600));
    
    /** @brief Record uptime duration sample. */
    void recordUptime(std::chrono::milliseconds duration);
    /** @brief Record downtime duration sample. */
    void recordDowntime(std::chrono::milliseconds duration);
    
    /**
     * @brief Record latency observation in milliseconds.
     * @param latency_ms Observed latency.
     */
    void recordLatency(double latency_ms);
    
    /** @brief Record observed lost bytes for durability accounting. */
    void recordDataLoss(uint64_t bytes_lost);
    
    /**
     * @brief Record replication lag sample in milliseconds.
     * @param lag_ms Replication lag.
     */
    void recordReplicationLag(double lag_ms);
    
    /** @brief Compute availability ratio in [0,1]. */
    double getAvailability() const;
    /** @brief Compute p50 latency from current samples (ms). */
    double getLatencyP50() const;
    /** @brief Compute p99 latency from current samples (ms). */
    double getLatencyP99() const;
    /** @brief Compute data-loss rate as lost_bytes / written_bytes. */
    double getDataLossRate() const;
    /** @brief Compute average replication lag in milliseconds. */
    double getAvgReplicationLag() const;
    
    /**
     * @brief Compute remaining error budget fraction.
     * @param target_availability Target availability ratio in [0,1].
     * @return Remaining budget fraction where 1.0 means unused budget.
     */
    double getErrorBudget(double target_availability) const;
    
    /** @brief Reset all accumulated samples and counters for this window. */
    void reset();
    
private:
    mutable std::mutex mutex_;
    std::chrono::seconds window_duration_;
    std::chrono::steady_clock::time_point window_start_;
    
    // Availability tracking
    std::atomic<uint64_t> total_uptime_ms_{0};
    std::atomic<uint64_t> total_downtime_ms_{0};
    
    // Latency tracking (ring buffer)
    std::vector<double> latency_samples_;
    size_t max_latency_samples_{10000};
    
    // Data loss tracking
    std::atomic<uint64_t> total_bytes_lost_{0};
    std::atomic<uint64_t> total_bytes_written_{0};
    
    // Replication lag tracking
    std::vector<double> replication_lag_samples_;
    size_t max_lag_samples_{1000};
    
    double calculatePercentile(const std::vector<double>& samples, double percentile) const;
};

/**
 * SLO Monitor tracks and reports on service level objectives.
 * Monitors availability, latency, durability, and consistency targets.
 */
class SLOMonitor {
public:
    struct Config {
        SLOTarget targets;
        std::chrono::seconds window_duration = std::chrono::hours(24);
        bool enable_alerting = true;
        double alert_threshold = 0.9;  // Alert when error budget reaches 90%
        static Config defaults() { return {}; }
    };

    /**
     * Repair progress snapshot reported by ShardRepairEngine.
     * Allows operators to track time-to-full-repair for large shards.
     */
    struct RepairProgress {
        std::string job_id;
        uint64_t documents_scanned = 0;
        uint64_t documents_total = 0;    ///< 0 = unknown total
        uint64_t documents_repaired = 0;
        uint64_t documents_failed = 0;
        double percent_complete = 0.0;   ///< [0, 100]
        std::chrono::system_clock::time_point started_at;
        std::chrono::system_clock::time_point updated_at;
        bool completed = false;
    };

    /** @brief Construct SLO monitor with runtime targets and alert policy. */
    explicit SLOMonitor(const Config& config = Config::defaults());
    ~SLOMonitor() = default;
    
    /**
     * @brief Record shard availability sample.
     * @param shard_id Logical shard id.
     * @param is_available True for uptime sample, false for downtime sample.
     */
    void recordShardAvailability(const std::string& shard_id, bool is_available);

    /**
     * @brief Record query latency sample for query-type SLO tracking.
     * @param shard_id Shard id (reserved for future per-shard query slicing).
     * @param query_type Query-class label (for target selection).
     * @param latency_ms Observed latency in milliseconds.
     */
    void recordQueryLatency(const std::string& shard_id, const std::string& query_type, double latency_ms);

    /**
     * @brief Record transaction latency sample.
     * @param transaction_type Transaction-class label.
     * @param latency_ms Observed latency in milliseconds.
     */
    void recordTransactionLatency(const std::string& transaction_type, double latency_ms);

    /**
     * @brief Record data-loss event for durability SLO.
     * @param shard_id Shard id.
     * @param bytes_lost Number of bytes lost.
     */
    void recordDataLoss(const std::string& shard_id, uint64_t bytes_lost);

    /**
     * @brief Record replication lag observation.
     * @param shard_id Shard id.
     * @param lag_ms Replication lag in milliseconds.
     */
    void recordReplicationLag(const std::string& shard_id, double lag_ms);

    /**
     * @brief Record leader-election duration sample.
     * @param shard_id Shard id.
     * @param duration_s Election duration in seconds.
     */
    void recordLeaderElection(const std::string& shard_id, double duration_s);

    /** @brief Record or update repair-progress snapshot keyed by job id. */
    void recordRepairProgress(const RepairProgress& progress);
    /** @brief Return latest repair progress for one job id (empty shell if unknown). */
    RepairProgress getRepairProgress(const std::string& job_id) const;
    /** @brief Return all repair jobs that are not yet marked completed. */
    std::vector<RepairProgress> getActiveRepairJobs() const;

    /** @brief Check availability SLO compliance for a shard. */
    bool isAvailabilitySLOMet(const std::string& shard_id) const;
    /** @brief Check latency SLO compliance for a query class. */
    bool isLatencySLOMet(const std::string& query_type) const;
    /** @brief Check durability SLO compliance for a shard. */
    bool isDurabilitySLOMet(const std::string& shard_id) const;
    /** @brief Check consistency SLO compliance for a shard. */
    bool isConsistencySLOMet(const std::string& shard_id) const;
    
    /** @brief Get remaining error budget for a shard in [0,1]. */
    double getErrorBudget(const std::string& shard_id) const;
    /** @brief Get average remaining global error budget in [0,1]. */
    double getGlobalErrorBudget() const;
    /** @brief Return true when shard error budget is fully exhausted. */
    bool isErrorBudgetExhausted(const std::string& shard_id) const;
    
    /** @brief Render human-readable SLO compliance report. */
    std::string generateSLOReport() const;
    /** @brief Render machine-readable SLO report as JSON string. */
    std::string generateSLOReportJSON() const;
    /** @brief Return compliance map keyed by metric name. */
    std::map<std::string, double> getSLOCompliance() const;
    
    /** @brief Return currently active alert messages. */
    std::vector<std::string> getActiveAlerts() const;
    
    /** @brief Get active SLO target configuration. */
    const SLOTarget& getTargets() const { return config_.targets; }
    /** @brief Replace SLO target configuration at runtime. */
    void updateTargets(const SLOTarget& targets);
    
private:
    Config config_;
    mutable std::mutex mutex_;
    
    // Per-shard SLO windows
    std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows_;
    
    // Per-query-type latency windows
    std::map<std::string, std::shared_ptr<SLOWindow>> query_latency_windows_;
    
    // Transaction latency windows
    std::map<std::string, std::shared_ptr<SLOWindow>> transaction_latency_windows_;
    
    // Alert state
    mutable std::vector<std::string> active_alerts_;
    
    // Per-job repair progress registry
    std::map<std::string, RepairProgress> repair_progress_;

    // Helper methods
    std::shared_ptr<SLOWindow> getOrCreateShardWindow(const std::string& shard_id);
    std::shared_ptr<SLOWindow> getOrCreateQueryWindow(const std::string& query_type);
    std::shared_ptr<SLOWindow> getOrCreateTransactionWindow(const std::string& tx_type);
    void checkAndGenerateAlerts();
    std::string formatSLOViolation(const std::string& slo_name, double actual, double target) const;
};

/**
 * SLO Reporter generates periodic reports on SLO compliance.
 */
class SLOReporter {
public:
    enum class ReportFrequency {
        HOURLY,
        DAILY,
        WEEKLY,
        MONTHLY
    };
    
    struct Config {
        ReportFrequency frequency = ReportFrequency::DAILY;
        std::string output_path = "/var/log/themisdb/slo_reports/";
        bool enable_json_export = true;
        bool enable_prometheus_export = true;
        static Config defaults() { return {}; }
    };
    
    /** @brief Construct periodic SLO reporter bound to monitor instance. */
    explicit SLOReporter(SLOMonitor& monitor, const Config& config = Config::defaults());
    ~SLOReporter();
    
    /** @brief Start periodic report generation loop. */
    void start();
    /** @brief Stop periodic report generation loop. */
    void stop();
    
    /** @brief Generate and persist one report immediately. */
    void generateReport();
    
private:
    SLOMonitor& monitor_;
    Config config_;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> reporter_thread_;
    
    void reporterLoop();
    std::string generateReportFilename() const;
    void writeReport(const std::string& content);
};

} // namespace sharding
} // namespace themis
