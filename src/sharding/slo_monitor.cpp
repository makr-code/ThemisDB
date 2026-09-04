/**
 * @file slo_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#include "sharding/slo_monitor.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <thread>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// ==================== SLOWindow Implementation ====================

/**
 * @brief Initialize SLO measurement window buffers and counters.
 * @param window_duration Rolling window duration.
 */
SLOWindow::SLOWindow(std::chrono::seconds window_duration)
    : window_duration_(window_duration)
    , window_start_(std::chrono::steady_clock::now()) {
    latency_samples_.reserve(max_latency_samples_);
    replication_lag_samples_.reserve(max_lag_samples_);
}

/**
 * @brief Record uptime duration in milliseconds.
 * @param duration Uptime duration sample.
 */
void SLOWindow::recordUptime(std::chrono::milliseconds duration) {
    total_uptime_ms_.fetch_add(duration.count(), std::memory_order_relaxed);
}

/**
 * @brief Record downtime duration in milliseconds.
 * @param duration Downtime duration sample.
 */
void SLOWindow::recordDowntime(std::chrono::milliseconds duration) {
    total_downtime_ms_.fetch_add(duration.count(), std::memory_order_relaxed);
}

/**
 * @brief Append a latency sample.
 * @param latency_ms Latency value in milliseconds.
 */
void SLOWindow::recordLatency([[maybe_unused]] double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    latency_samples_.push_back(latency_ms);
    
    // Keep only recent samples
    if (static_cast<int>(latency_samples_.size()) > max_latency_samples_) {
        latency_samples_.erase(latency_samples_.begin());
    }
}

/**
 * @brief Record bytes lost for durability accounting.
 * @param bytes_lost Lost bytes.
 */
void SLOWindow::recordDataLoss([[maybe_unused]] uint64_t bytes_lost) {
    total_bytes_lost_.fetch_add(bytes_lost, std::memory_order_relaxed);
}

/**
 * @brief Append replication lag sample.
 * @param lag_ms Lag value in milliseconds.
 */
void SLOWindow::recordReplicationLag([[maybe_unused]] double lag_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    replication_lag_samples_.push_back(lag_ms);
    
    // Keep only recent samples
    if (static_cast<int>(replication_lag_samples_.size()) > max_lag_samples_) {
        replication_lag_samples_.erase(replication_lag_samples_.begin());
    }
}

/**
 * @brief Compute availability ratio from accumulated uptime/downtime.
 * @return Availability in [0,1].
 */
double SLOWindow::getAvailability() const {
    uint64_t uptime = total_uptime_ms_.load(std::memory_order_relaxed);
    uint64_t downtime = total_downtime_ms_.load(std::memory_order_relaxed);
    uint64_t total = uptime + downtime;
    
    if (total == 0) {
        return 1.0;  // Assume 100% if no data
    }
    
    return static_cast<double>(uptime) / static_cast<double>(total);
}

/** @brief Compute p50 latency in milliseconds. */
double SLOWindow::getLatencyP50() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calculatePercentile(latency_samples_, 0.5);
}

/** @brief Compute p99 latency in milliseconds. */
double SLOWindow::getLatencyP99() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calculatePercentile(latency_samples_, 0.99);
}

/**
 * @brief Compute data-loss rate.
 * @return lost_bytes / written_bytes (0 when no bytes written are known).
 */
double SLOWindow::getDataLossRate() const {
    uint64_t bytes_lost = total_bytes_lost_.load(std::memory_order_relaxed);
    uint64_t bytes_written = total_bytes_written_.load(std::memory_order_relaxed);
    
    if (bytes_written == 0) {
        return 0.0;
    }
    
    return static_cast<double>(bytes_lost) / static_cast<double>(bytes_written);
}

/**
 * @brief Compute average replication lag.
 * @return Average lag in milliseconds.
 */
double SLOWindow::getAvgReplicationLag() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (replication_lag_samples_.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double lag : replication_lag_samples_) {
        sum += lag;
    }
    
    return sum / replication_lag_samples_.size();
}

/**
 * @brief Compute remaining error budget for a target availability.
 * @param target_availability SLO target in [0,1].
 * @return Remaining budget ratio where 1.0 is full budget remaining.
 */
double SLOWindow::getErrorBudget([[maybe_unused]] double target_availability) const {
    double current_availability = getAvailability();
    double error_budget = 1.0 - target_availability;
    double error_used = 1.0 - current_availability;
    
    return 1.0 - (error_used / error_budget);
}

/**
 * @brief Reset counters and sample buffers.
 */
void SLOWindow::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_uptime_ms_.store(0, std::memory_order_relaxed);
    total_downtime_ms_.store(0, std::memory_order_relaxed);
    total_bytes_lost_.store(0, std::memory_order_relaxed);
    total_bytes_written_.store(0, std::memory_order_relaxed);
    
    latency_samples_.clear();
    replication_lag_samples_.clear();
    
    window_start_ = std::chrono::steady_clock::now();
}

/**
 * @brief Compute percentile for a sample vector.
 * @param samples Input samples.
 * @param percentile Percentile as fraction (0..1).
 * @return Percentile value or 0 when samples are empty.
 */
double SLOWindow::calculatePercentile(const std::vector<double>& samples, double percentile) const {
    if (samples.empty()) {
        return 0.0;
    }
    
    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(percentile * (static_cast<int>(sorted.size()) - 1));
    return sorted[index];
}

// ==================== SLOMonitor Implementation ====================

/** @brief Construct SLO monitor with target thresholds and alerting config. */
SLOMonitor::SLOMonitor(const Config& config)
    : config_(config) {
}

/** @brief Record shard availability tick sample and evaluate alert state. */
void SLOMonitor::recordShardAvailability(const std::string& shard_id, bool is_available) {
    // Sample interval for availability tracking
    static constexpr auto kAvailabilitySampleInterval = std::chrono::milliseconds(1000);
    
    auto window = getOrCreateShardWindow(shard_id);
    
    if (is_available) {
        window->recordUptime(kAvailabilitySampleInterval);
    } else {
        window->recordDowntime(kAvailabilitySampleInterval);
    }
    
    checkAndGenerateAlerts();
}

/** @brief Record query latency sample into per-query rolling window. */
void SLOMonitor::recordQueryLatency([[maybe_unused]] const std::string& shard_id, const std::string& query_type, double latency_ms) {
    auto window = getOrCreateQueryWindow(query_type);
    window->recordLatency(latency_ms);
    
    checkAndGenerateAlerts();
}

/** @brief Record transaction latency sample into per-type rolling window. */
void SLOMonitor::recordTransactionLatency(const std::string& transaction_type, double latency_ms) {
    auto window = getOrCreateTransactionWindow(transaction_type);
    window->recordLatency(latency_ms);
    
    checkAndGenerateAlerts();
}

/** @brief Record durability-impacting data-loss observation for shard. */
void SLOMonitor::recordDataLoss(const std::string& shard_id, uint64_t bytes_lost) {
    auto window = getOrCreateShardWindow(shard_id);
    window->recordDataLoss(bytes_lost);
    
    checkAndGenerateAlerts();
}

/** @brief Record replication lag observation for shard consistency tracking. */
void SLOMonitor::recordReplicationLag(const std::string& shard_id, double lag_ms) {
    auto window = getOrCreateShardWindow(shard_id);
    window->recordReplicationLag(lag_ms);
    
    checkAndGenerateAlerts();
}

/** @brief Record leader-election duration and emit violation alert when exceeded. */
void SLOMonitor::recordLeaderElection([[maybe_unused]] const std::string& shard_id, double duration_s) {
    double max_leader_election_time_s = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        max_leader_election_time_s = config_.targets.max_leader_election_time_s;
    }

    // Leader election duration impacts consistency SLO
    if (duration_s > max_leader_election_time_s) {
        std::lock_guard<std::mutex> lock(mutex_);
        active_alerts_.push_back(
            formatSLOViolation("leader_election_time", duration_s, max_leader_election_time_s)
        );
    }
}

/** @brief Check shard availability against configured target. */
bool SLOMonitor::isAvailabilitySLOMet(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_windows_.find(shard_id);
    if (it == shard_windows_.end()) {
        return true;  // No data means meeting SLO
    }
    
    double availability = it->second->getAvailability();
    return availability >= config_.targets.availability_target;
}

/** @brief Check query latency p99 against target selected by query type label. */
bool SLOMonitor::isLatencySLOMet(const std::string& query_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = query_latency_windows_.find(query_type);
    if (it == query_latency_windows_.end()) {
        return true;
    }
    
    double p99 = it->second->getLatencyP99();
    
    // Check appropriate target based on query type
    if (query_type.find("single") != std::string::npos) {
        return p99 <= config_.targets.single_shard_query_p99_ms;
    } else if (query_type.find("cross") != std::string::npos) {
        return p99 <= config_.targets.cross_shard_query_p99_ms;
    }
    
    return true;
}

/** @brief Check shard durability (data-loss rate) against configured tolerance. */
bool SLOMonitor::isDurabilitySLOMet(const std::string& shard_id) const {
    std::shared_ptr<SLOWindow> window;
    double data_loss_tolerance = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = shard_windows_.find(shard_id);
        if (it == shard_windows_.end()) {
            return true;
        }
        window = it->second;
        data_loss_tolerance = config_.targets.data_loss_tolerance;
    }

    if (!window) {
        return true;
    }

    const double data_loss_rate = window->getDataLossRate();
    return data_loss_rate <= data_loss_tolerance;
}

/** @brief Check shard consistency (avg replication lag) against configured cap. */
bool SLOMonitor::isConsistencySLOMet(const std::string& shard_id) const {
    std::shared_ptr<SLOWindow> window;
    double max_replication_lag_ms = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = shard_windows_.find(shard_id);
        if (it == shard_windows_.end()) {
            return true;
        }
        window = it->second;
        max_replication_lag_ms = config_.targets.max_replication_lag_ms;
    }

    if (!window) {
        return true;
    }

    const double avg_lag = window->getAvgReplicationLag();
    return avg_lag <= max_replication_lag_ms;
}

/** @brief Return remaining error budget fraction for one shard. */
double SLOMonitor::getErrorBudget(const std::string& shard_id) const {
    std::shared_ptr<SLOWindow> window;
    double availability_target = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = shard_windows_.find(shard_id);
        if (it == shard_windows_.end()) {
            return 1.0;  // Full error budget
        }
        window = it->second;
        availability_target = config_.targets.availability_target;
    }

    if (!window) {
        return 1.0;
    }

    return window->getErrorBudget(availability_target);
}

/** @brief Return mean remaining error budget across all tracked shards. */
double SLOMonitor::getGlobalErrorBudget() const {
    std::vector<std::shared_ptr<SLOWindow>> windows;
    double availability_target = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shard_windows_.empty()) {
            return 1.0;
        }
        availability_target = config_.targets.availability_target;
        windows.reserve(shard_windows_.size());
        for (const auto& [_, window] : shard_windows_) {
            windows.push_back(window);
        }
    }

    if (windows.empty()) {
        return 1.0;
    }

    double total_budget = 0.0;
    for (const auto& window : windows) {
        total_budget += window->getErrorBudget(availability_target);
    }

    return total_budget / windows.size();
}

/** @brief Return true when shard's remaining budget crosses exhaustion threshold. */
bool SLOMonitor::isErrorBudgetExhausted(const std::string& shard_id) const {
    double alert_threshold = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        alert_threshold = config_.alert_threshold;
    }
    double budget = getErrorBudget(shard_id);
    return budget <= (1.0 - alert_threshold);
}

/** @brief Render human-readable SLO report text snapshot. */
std::string SLOMonitor::generateSLOReport() const {
    SLOTarget targets;
    std::chrono::seconds window_duration;
    std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows;
    std::map<std::string, std::shared_ptr<SLOWindow>> query_latency_windows;
    std::vector<std::string> active_alerts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        targets = config_.targets;
        window_duration = config_.window_duration;
        shard_windows = shard_windows_;
        query_latency_windows = query_latency_windows_;
        active_alerts = active_alerts_;
    }

    std::ostringstream oss = {};
    
    oss << "=== ThemisDB Sharding SLO Report ===\n\n";
    oss << "Report Time: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
    oss << "Window Duration: " << window_duration.count() << " seconds\n\n";
    
    // Availability SLO
    oss << "AVAILABILITY SLO (Target: " << (targets.availability_target * 100) << "%)\n";
    for (const auto& [shard_id, window] : shard_windows) {
        double availability = window->getAvailability();
        double error_budget = window->getErrorBudget(targets.availability_target);
        
        oss << "  Shard " << shard_id << ": "
            << std::fixed << std::setprecision(4) << (availability * 100) << "% "
            << "(Error Budget: " << std::setprecision(2) << (error_budget * 100) << "%)\n";
    }
    oss << "\n";
    
    // Latency SLO
    oss << "LATENCY SLO\n";
    for (const auto& [query_type, window] : query_latency_windows) {
        double p50 = window->getLatencyP50();
        double p99 = window->getLatencyP99();
        
        oss << "  " << query_type << ": "
            << "p50=" << std::setprecision(2) << p50 << "ms, "
            << "p99=" << p99 << "ms\n";
    }
    oss << "\n";
    
    // Consistency SLO
    oss << "CONSISTENCY SLO (Target: Replication Lag < " 
        << targets.max_replication_lag_ms << "ms)\n";
    for (const auto& [shard_id, window] : shard_windows) {
        double avg_lag = window->getAvgReplicationLag();
        
        oss << "  Shard " << shard_id << ": "
            << std::setprecision(2) << avg_lag << "ms avg lag\n";
    }
    oss << "\n";
    
    // Active Alerts
    if (!active_alerts.empty()) {
        oss << "ACTIVE ALERTS:\n";
        for (const auto& alert : active_alerts) {
            oss << "  - " << alert << "\n";
        }
    } else {
        oss << "No active alerts. All SLOs being met.\n";
    }
    
    return oss.str();
}

/** @brief Render structured JSON SLO report snapshot. */
std::string SLOMonitor::generateSLOReportJSON() const {
    SLOTarget targets;
    std::chrono::seconds window_duration;
    std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows;
    std::map<std::string, std::shared_ptr<SLOWindow>> query_latency_windows;
    std::vector<std::string> active_alerts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        targets = config_.targets;
        window_duration = config_.window_duration;
        shard_windows = shard_windows_;
        query_latency_windows = query_latency_windows_;
        active_alerts = active_alerts_;
    }

    nlohmann::json report;
    
    report["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    report["window_duration_seconds"] = window_duration.count();
    
    // Availability
    nlohmann::json availability_slos = nlohmann::json::array();
    for (const auto& [shard_id, window] : shard_windows) {
        nlohmann::json slo;
        slo["shard_id"] = shard_id;
        slo["availability"] = window->getAvailability();
        slo["error_budget"] = window->getErrorBudget(targets.availability_target);
        slo["target"] = targets.availability_target;
        slo["met"] = window->getAvailability() >= targets.availability_target;
        availability_slos.push_back(slo);
    }
    report["availability_slos"] = availability_slos;
    
    // Latency
    nlohmann::json latency_slos = nlohmann::json::array();
    for (const auto& [query_type, window] : query_latency_windows) {
        nlohmann::json slo;
        slo["query_type"] = query_type;
        slo["p50_ms"] = window->getLatencyP50();
        slo["p99_ms"] = window->getLatencyP99();
        latency_slos.push_back(slo);
    }
    report["latency_slos"] = latency_slos;
    
    // Consistency
    nlohmann::json consistency_slos = nlohmann::json::array();
    for (const auto& [shard_id, window] : shard_windows) {
        nlohmann::json slo;
        slo["shard_id"] = shard_id;
        slo["avg_replication_lag_ms"] = window->getAvgReplicationLag();
        slo["target_lag_ms"] = targets.max_replication_lag_ms;
        slo["met"] = window->getAvgReplicationLag() <= targets.max_replication_lag_ms;
        consistency_slos.push_back(slo);
    }
    report["consistency_slos"] = consistency_slos;
    
    // Alerts
    report["active_alerts"] = active_alerts;
    report["alert_count"] = active_alerts.size();
    
    return report.dump(2);
}

/** @brief Return aggregate compliance metrics map for dashboards/tests. */
std::map<std::string, double> SLOMonitor::getSLOCompliance() const {
    std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows;
    double availability_target = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shard_windows = shard_windows_;
        availability_target = config_.targets.availability_target;
    }
    std::map<std::string, double> compliance;
    
    // Calculate overall compliance rates
    if (!shard_windows.empty()) {
        double total_availability = 0.0;
        for (const auto& [_, window] : shard_windows) {
            total_availability += window->getAvailability();
        }
        compliance["availability"] = total_availability / shard_windows.size();

        double total_budget = 0.0;
        for (const auto& [_, window] : shard_windows) {
            total_budget += window->getErrorBudget(availability_target);
        }
        compliance["error_budget"] = total_budget / shard_windows.size();
    } else {
        compliance["error_budget"] = 1.0;
    }

    return compliance;
}

/** @brief Return current in-memory alert list. */
std::vector<std::string> SLOMonitor::getActiveAlerts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_alerts_;
}

/** @brief Replace monitor target thresholds at runtime. */
void SLOMonitor::updateTargets(const SLOTarget& targets) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.targets = targets;
}

/** @brief Upsert repair progress record by job id. */
void SLOMonitor::recordRepairProgress(const RepairProgress& progress) {
    std::lock_guard<std::mutex> lock(mutex_);
    repair_progress_[progress.job_id] = progress;
}

/** @brief Retrieve repair progress record for job id or empty placeholder. */
SLOMonitor::RepairProgress SLOMonitor::getRepairProgress(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = repair_progress_.find(job_id);
    if (it == repair_progress_.end()) {
        RepairProgress not_found;
        not_found.job_id = job_id;
        return not_found;
    }
    return it->second;
}

/** @brief List all repair jobs currently not marked completed. */
std::vector<SLOMonitor::RepairProgress> SLOMonitor::getActiveRepairJobs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RepairProgress> active = {};

    for (const auto& [id, prog] : repair_progress_) {
        if (!prog.completed) {
            active.push_back(prog);
        }
    }
    return active;
}

/** @brief Fetch or create per-shard SLO window. */
std::shared_ptr<SLOWindow> SLOMonitor::getOrCreateShardWindow(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_windows_.find(shard_id);
    if (it == shard_windows_.end()) {
        auto window = std::make_shared<SLOWindow>(config_.window_duration);
        shard_windows_[shard_id] = window;
        return window;
    }
    
    return it->second;
}

/** @brief Fetch or create per-query-type SLO window. */
std::shared_ptr<SLOWindow> SLOMonitor::getOrCreateQueryWindow(const std::string& query_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = query_latency_windows_.find(query_type);
    if (it == query_latency_windows_.end()) {
        auto window = std::make_shared<SLOWindow>(config_.window_duration);
        query_latency_windows_[query_type] = window;
        return window;
    }
    
    return it->second;
}

/** @brief Fetch or create per-transaction-type SLO window. */
std::shared_ptr<SLOWindow> SLOMonitor::getOrCreateTransactionWindow(const std::string& tx_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = transaction_latency_windows_.find(tx_type);
    if (it == transaction_latency_windows_.end()) {
        auto window = std::make_shared<SLOWindow>(config_.window_duration);
        transaction_latency_windows_[tx_type] = window;
        return window;
    }
    
    return it->second;
}

/** @brief Recompute active alert list from current window samples and thresholds. */
void SLOMonitor::checkAndGenerateAlerts() {
    Config config_snapshot;
    std::map<std::string, std::shared_ptr<SLOWindow>> shard_windows;
    std::map<std::string, std::shared_ptr<SLOWindow>> query_latency_windows;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_snapshot = config_;
        shard_windows = shard_windows_;
        query_latency_windows = query_latency_windows_;
    }

    if (!config_snapshot.enable_alerting) {
        return;
    }

    std::vector<std::string> active_alerts;

    // Check availability SLOs
    for (const auto& [shard_id, window] : shard_windows) {
        double availability = window->getAvailability();
        if (availability < config_snapshot.targets.availability_target) {
            active_alerts.push_back(
                formatSLOViolation("availability:" + shard_id, availability, config_snapshot.targets.availability_target)
            );
        }
        
        double error_budget = window->getErrorBudget(config_snapshot.targets.availability_target);
        if (error_budget <= (1.0 - config_snapshot.alert_threshold)) {
            active_alerts.push_back(
                "Error budget exhausted for shard " + shard_id + ": " + 
                std::to_string(error_budget * 100) + "% remaining"
            );
        }
    }
    
    // Check latency SLOs
    for (const auto& [query_type, window] : query_latency_windows) {
        double p99 = window->getLatencyP99();
        double target = (query_type.find("single") != std::string::npos) 
            ? config_snapshot.targets.single_shard_query_p99_ms 
            : config_snapshot.targets.cross_shard_query_p99_ms;
        
        if (p99 > target) {
            active_alerts.push_back(
                formatSLOViolation("latency_p99:" + query_type, p99, target)
            );
        }
    }
    
    // Check consistency SLOs
    for (const auto& [shard_id, window] : shard_windows) {
        double avg_lag = window->getAvgReplicationLag();
        if (avg_lag > config_snapshot.targets.max_replication_lag_ms) {
            active_alerts.push_back(
                formatSLOViolation("replication_lag:" + shard_id, avg_lag, config_snapshot.targets.max_replication_lag_ms)
            );
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_alerts_ = std::move(active_alerts);
    }
}

/** @brief Format one SLO violation message line for alert/report output. */
std::string SLOMonitor::formatSLOViolation(const std::string& slo_name, double actual, double target) const {
    std::ostringstream oss = {};
    oss << "SLO VIOLATION: " << slo_name 
        << " (actual: " << std::fixed << std::setprecision(4) << actual 
        << ", target: " << target << ")";
    return oss.str();
}

// ==================== SLOReporter Implementation ====================

/** @brief Construct SLO reporter with monitor reference and output configuration. */
SLOReporter::SLOReporter(SLOMonitor& monitor, const Config& config)
    : monitor_(monitor)
    , config_(config) {
}

/** @brief Stop reporter loop on destruction. */
SLOReporter::~SLOReporter() {
    stop();
}

/** @brief Start background report-generation thread if not already running. */
void SLOReporter::start() {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    
    reporter_thread_ = std::make_unique<std::thread>(&SLOReporter::reporterLoop, this);
}

/** @brief Stop background report-generation thread and join it. */
void SLOReporter::stop() {
    if (!running_.exchange(false)) {
        return;  // Not running
    }
    
    if (reporter_thread_ && reporter_thread_->joinable()) {
        reporter_thread_->join();
    }
}

/** @brief Generate one text report and optional JSON companion file. */
void SLOReporter::generateReport() {
    std::string report_text = monitor_.generateSLOReport();
    writeReport(report_text);
    
    if (config_.enable_json_export) {
        std::string report_json = monitor_.generateSLOReportJSON();
        std::string json_filename = generateReportFilename() + ".json";
        std::ofstream json_file(json_filename);
        if (json_file.is_open()) {
            json_file << report_json;
            json_file.close();
        }
    }
}

/** @brief Background reporting loop honoring configured frequency cadence. */
void SLOReporter::reporterLoop() {
    while (running_) {
        generateReport();
        
        // Sleep based on frequency
        std::chrono::seconds sleep_duration;
        switch (config_.frequency) {
            case ReportFrequency::HOURLY:
                sleep_duration = std::chrono::hours(1);
                break;
            case ReportFrequency::DAILY:
                sleep_duration = std::chrono::hours(24);
                break;
            case ReportFrequency::WEEKLY:
                sleep_duration = std::chrono::hours(24 * 7);
                break;
            case ReportFrequency::MONTHLY:
                sleep_duration = std::chrono::hours(24 * 30);
                break;
        }
        
        // Sleep in short intervals to allow for quick shutdown
        auto end_time = std::chrono::steady_clock::now() + sleep_duration;
        while (running_ && std::chrono::steady_clock::now() < end_time) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

/** @brief Build timestamped report filename for text output. */
std::string SLOReporter::generateReportFilename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // Use thread-safe localtime
    std::tm tm = {};
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    std::ostringstream oss = {};
    oss << config_.output_path << "slo_report_"
        << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << ".txt";
    
    return oss.str();
}

/** @brief Write report content to filesystem target path. */
void SLOReporter::writeReport(const std::string& content) {
    std::string filename = generateReportFilename();
    std::ofstream file(filename);
    
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

} // namespace sharding
} // namespace themis
