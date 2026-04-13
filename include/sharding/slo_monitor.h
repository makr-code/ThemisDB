/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            slo_monitor.h                                      ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:20:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     262                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 096960f501  2026-03-13  feat(sharding): implement Reed-Solomon repair engine para... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 92608937d0  2026-02-26  fix: GCC default-arg error in 18 headers - add ::defaults... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_SHARDING_SLO_MONITOR_H
#define THEMIS_SHARDING_SLO_MONITOR_H

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
    explicit SLOWindow(std::chrono::seconds window_duration = std::chrono::seconds(3600));
    
    // Record an uptime/downtime event
    void recordUptime(std::chrono::milliseconds duration);
    void recordDowntime(std::chrono::milliseconds duration);
    
    // Record latency observation
    void recordLatency(double latency_ms);
    
    // Record data loss event
    void recordDataLoss(uint64_t bytes_lost);
    
    // Record replication lag
    void recordReplicationLag(double lag_ms);
    
    // Calculate SLO compliance
    double getAvailability() const;
    double getLatencyP50() const;
    double getLatencyP99() const;
    double getDataLossRate() const;
    double getAvgReplicationLag() const;
    
    // Get error budget remaining
    double getErrorBudget(double target_availability) const;
    
    // Reset the window
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

    explicit SLOMonitor(const Config& config = Config::defaults());
    ~SLOMonitor() = default;
    
    // Update SLO measurements
    void recordShardAvailability(const std::string& shard_id, bool is_available);
    void recordQueryLatency(const std::string& shard_id, const std::string& query_type, double latency_ms);
    void recordTransactionLatency(const std::string& transaction_type, double latency_ms);
    void recordDataLoss(const std::string& shard_id, uint64_t bytes_lost);
    void recordReplicationLag(const std::string& shard_id, double lag_ms);
    void recordLeaderElection(const std::string& shard_id, double duration_s);

    // Repair progress tracking (for time-to-full-repair observability)
    void recordRepairProgress(const RepairProgress& progress);
    RepairProgress getRepairProgress(const std::string& job_id) const;
    std::vector<RepairProgress> getActiveRepairJobs() const;

    // SLO compliance checks
    bool isAvailabilitySLOMet(const std::string& shard_id) const;
    bool isLatencySLOMet(const std::string& query_type) const;
    bool isDurabilitySLOMet(const std::string& shard_id) const;
    bool isConsistencySLOMet(const std::string& shard_id) const;
    
    // Error budget tracking
    double getErrorBudget(const std::string& shard_id) const;
    double getGlobalErrorBudget() const;
    bool isErrorBudgetExhausted(const std::string& shard_id) const;
    
    // SLO reporting
    std::string generateSLOReport() const;
    std::string generateSLOReportJSON() const;
    std::map<std::string, double> getSLOCompliance() const;
    
    // Alert generation
    std::vector<std::string> getActiveAlerts() const;
    
    // Configuration
    const SLOTarget& getTargets() const { return config_.targets; }
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
    
    explicit SLOReporter(SLOMonitor& monitor, const Config& config = Config::defaults());
    ~SLOReporter();
    
    // Start/stop periodic reporting
    void start();
    void stop();
    
    // Generate report immediately
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

#endif // THEMIS_SHARDING_SLO_MONITOR_H
