/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_repair_engine.cpp                            ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:20:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     715                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 35b0161a0f  2026-03-13  fix(sharding): wire IOPS throttle and GPU flag into Shard... ║
    • f205c3d0d1  2026-03-13  fix(sharding): address code review feedback - capture sca... ║
    • 096960f501  2026-03-13  feat(sharding): implement Reed-Solomon repair engine para... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Shard Repair / Anti-Entropy Engine – Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sharding/shard_repair_engine.h"
#include "sharding/shard_resource_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <future>
#include <sstream>
#include <iomanip>

namespace themis {
namespace sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

ShardRepairEngine::ShardRepairEngine(
    const RepairConfig& config,
    RedundancyStrategy& strategy,
    ConsistentHashRing& ring,
    ShardTopology& topology,
    RedundancyStrategy::ReadHandler read_handler,
    RedundancyStrategy::WriteHandler write_handler)
    : config_(config),
      strategy_(strategy),
      ring_(ring),
      topology_(topology),
      read_handler_(std::move(read_handler)),
      write_handler_(std::move(write_handler)) {}

ShardRepairEngine::~ShardRepairEngine() {
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void ShardRepairEngine::start() {
    if (running_.exchange(true)) {
        return;  // already running
    }

    if (config_.enable_periodic_scan) {
        scan_thread_ = std::thread([this]() { scanLoop(); });
    }

    if (config_.enable_auto_repair) {
        repair_thread_ = std::thread([this]() { repairLoop(); });
    }

    spdlog::info("ShardRepairEngine started (scan_interval={}s, auto_repair={})",
                 config_.scan_interval.count(), config_.enable_auto_repair);
}

void ShardRepairEngine::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    // Wake up the repair worker so it can exit
    repair_cv_.notify_all();

    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }
    if (repair_thread_.joinable()) {
        repair_thread_.join();
    }

    spdlog::info("ShardRepairEngine stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Provider injection
// ─────────────────────────────────────────────────────────────────────────────

void ShardRepairEngine::setDocumentListProvider(DocumentListProvider provider) {
    doc_list_provider_ = std::move(provider);
}

void ShardRepairEngine::setPrometheusMetrics(std::shared_ptr<PrometheusMetrics> prom_metrics) {
    prom_metrics_ = std::move(prom_metrics);
}

void ShardRepairEngine::setSLOMonitor(std::shared_ptr<SLOMonitor> slo_monitor) {
    slo_monitor_ = std::move(slo_monitor);
}

void ShardRepairEngine::setResourceManager(std::shared_ptr<ShardResourceManager> resource_manager) {
    resource_manager_ = std::move(resource_manager);
}

// ─────────────────────────────────────────────────────────────────────────────
// On-demand triggers
// ─────────────────────────────────────────────────────────────────────────────

std::string ShardRepairEngine::generateJobId() const {
    uint64_t counter = job_counter_.fetch_add(1);
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss << "repair-" << now << "-" << counter;
    return oss.str();
}

std::string ShardRepairEngine::triggerRepair(const std::string& shard_id) {
    RepairJob job;
    job.job_id = generateJobId();
    job.shard_id = shard_id;
    job.collection = config_.default_collection;
    job.is_full_scan = shard_id.empty();
    job.submitted_at = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_[job.job_id] = job;
        job_queue_.push(job.job_id);
    }
    repair_cv_.notify_one();

    spdlog::info("ShardRepairEngine: enqueued repair job {} (shard='{}')",
                 job.job_id, shard_id.empty() ? "<all>" : shard_id);
    return job.job_id;
}

std::string ShardRepairEngine::triggerFullScan() {
    RepairJob job;
    job.job_id = generateJobId();
    job.is_full_scan = true;
    job.collection = config_.default_collection;
    job.submitted_at = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_[job.job_id] = job;
        job_queue_.push(job.job_id);
    }
    repair_cv_.notify_one();

    spdlog::info("ShardRepairEngine: enqueued full-scan job {}", job.job_id);
    return job.job_id;
}

std::string ShardRepairEngine::triggerDocumentRepair(const std::string& document_id,
                                                      const std::string& collection) {
    RepairJob job;
    job.job_id = generateJobId();
    job.document_id = document_id;
    job.collection = collection.empty() ? config_.default_collection : collection;
    job.submitted_at = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        jobs_[job.job_id] = job;
        job_queue_.push(job.job_id);
    }
    repair_cv_.notify_one();

    spdlog::info("ShardRepairEngine: enqueued document repair job {} (doc='{}')",
                 job.job_id, document_id);
    return job.job_id;
}

// ─────────────────────────────────────────────────────────────────────────────
// Status / reporting
// ─────────────────────────────────────────────────────────────────────────────

RepairJob ShardRepairEngine::getJobStatus(const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        RepairJob not_found;
        not_found.job_id = job_id;
        not_found.completed = true;
        not_found.success = false;
        not_found.error_message = "Job not found";
        return not_found;
    }
    return it->second;
}

std::vector<RepairJob> ShardRepairEngine::getActiveJobs() const {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    std::vector<RepairJob> active;
    for (const auto& [id, job] : jobs_) {
        if (!job.completed) {
            active.push_back(job);
        }
    }
    return active;
}

std::vector<ShardHealthReport> ShardRepairEngine::getShardHealthReports() const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    std::vector<ShardHealthReport> reports;
    reports.reserve(shard_health_.size());
    for (const auto& [id, report] : shard_health_) {
        reports.push_back(report);
    }
    return reports;
}

RepairMetrics ShardRepairEngine::getRepairMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

std::string ShardRepairEngine::exportPrometheusMetrics() const {
    RepairMetrics m = getRepairMetrics();
    auto reports = getShardHealthReports();

    std::ostringstream oss;

    oss << "# HELP themis_shard_repair_scans_total Total anti-entropy scans performed\n"
        << "# TYPE themis_shard_repair_scans_total counter\n"
        << "themis_shard_repair_scans_total " << m.total_scans << "\n\n";

    oss << "# HELP themis_shard_repair_attempts_total Total repair attempts\n"
        << "# TYPE themis_shard_repair_attempts_total counter\n"
        << "themis_shard_repair_attempts_total " << m.total_repairs_attempted << "\n\n";

    oss << "# HELP themis_shard_repair_successes_total Successful repairs\n"
        << "# TYPE themis_shard_repair_successes_total counter\n"
        << "themis_shard_repair_successes_total " << m.total_repairs_successful << "\n\n";

    oss << "# HELP themis_shard_repair_failures_total Failed repair attempts\n"
        << "# TYPE themis_shard_repair_failures_total counter\n"
        << "themis_shard_repair_failures_total " << m.total_repairs_failed << "\n\n";

    oss << "# HELP themis_shard_repair_documents_scanned_total Documents scanned for health\n"
        << "# TYPE themis_shard_repair_documents_scanned_total counter\n"
        << "themis_shard_repair_documents_scanned_total " << m.total_documents_scanned << "\n\n";

    oss << "# HELP themis_shard_repair_avg_duration_ms Average repair duration (ms)\n"
        << "# TYPE themis_shard_repair_avg_duration_ms gauge\n"
        << "themis_shard_repair_avg_duration_ms " << m.avg_repair_time_ms.count() << "\n\n";

    // Per-shard health
    oss << "# HELP themis_shard_health Shard health status (0=healthy,1=degraded,2=failed,3=rebuilding)\n"
        << "# TYPE themis_shard_health gauge\n";
    for (const auto& r : reports) {
        int status_val = static_cast<int>(r.status);
        oss << "themis_shard_health{shard=\"" << r.shard_id << "\"} " << status_val << "\n";
    }
    oss << "\n";

    oss << "# HELP themis_shard_degraded_documents Number of degraded documents per shard\n"
        << "# TYPE themis_shard_degraded_documents gauge\n";
    for (const auto& r : reports) {
        oss << "themis_shard_degraded_documents{shard=\"" << r.shard_id << "\"} "
            << r.documents_degraded << "\n";
    }
    oss << "\n";

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Background threads
// ─────────────────────────────────────────────────────────────────────────────

void ShardRepairEngine::scanLoop() {
    while (running_.load()) {
        try {
            performAntiEntropyScan();
        } catch (const std::exception& e) {
            spdlog::error("ShardRepairEngine scanLoop exception: {}", e.what());
        }

        // Sleep in small increments so we can react to stop() promptly
        auto remaining = config_.scan_interval;
        const auto step = std::chrono::seconds(1);
        while (remaining > std::chrono::seconds(0) && running_.load()) {
            std::this_thread::sleep_for(step);
            remaining -= step;
        }
    }
}

void ShardRepairEngine::repairLoop() {
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(jobs_mutex_);
        repair_cv_.wait_for(lock, config_.repair_poll_interval, [this]() {
            return !job_queue_.empty() || !running_.load();
        });

        if (!running_.load()) break;

        // Drain up to repair_batch_size jobs
        uint32_t processed = 0;
        while (!job_queue_.empty() && processed < config_.repair_batch_size) {
            std::string job_id = job_queue_.front();
            job_queue_.pop();

            auto it = jobs_.find(job_id);
            if (it == jobs_.end()) continue;

            RepairJob& job = it->second;
            lock.unlock();

            try {
                executeRepairJob(job);
            } catch (const std::exception& e) {
                job.completed = true;
                job.success = false;
                job.error_message = e.what();
                job.completed_at = std::chrono::system_clock::now();
                spdlog::error("ShardRepairEngine: job {} failed: {}", job_id, e.what());
            }

            ++processed;
            lock.lock();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal operations
// ─────────────────────────────────────────────────────────────────────────────

void ShardRepairEngine::performAntiEntropyScan() {
    spdlog::info("ShardRepairEngine: starting parallel anti-entropy scan");

    // Log the active erasure-coding path so operators can see which backend is in use.
    if (resource_manager_) {
        bool gpu_enabled = resource_manager_->isGPUErasureCodingEnabled();
        spdlog::info("ShardRepairEngine: erasure-coding path = {}",
                     gpu_enabled ? "GPU (CUDA)" : "CPU/OpenCL");
    }

    auto all_shards = topology_.getAllShards();
    const uint64_t total_shards = all_shards.size();

    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        ++metrics_.total_scans;
        metrics_.last_scan_time = std::chrono::system_clock::now();
    }

    // Forward scan event to the centralized PrometheusMetrics registry.
    if (prom_metrics_) {
        prom_metrics_->recordRepairScan();
    }

    if (total_shards == 0) {
        spdlog::info("ShardRepairEngine: no shards to scan");
        return;
    }

    // Determine number of parallel workers (0 → use hardware_concurrency)
    uint32_t num_workers = config_.num_parallel_workers;
    if (num_workers == 0) {
        num_workers = std::max(1u, std::thread::hardware_concurrency());
    }
    num_workers = std::min(num_workers, static_cast<uint32_t>(total_shards));

    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.last_scan_workers = num_workers;
    }

    // Generate a synthetic job-id for SLO progress tracking
    const std::string scan_job_id = "scan-" + generateJobId();

    // Reset per-scan progress counters
    scan_shards_done_.store(0, std::memory_order_relaxed);
    scan_shards_total_.store(total_shards, std::memory_order_relaxed);

    if (slo_monitor_) {
        SLOMonitor::RepairProgress prog;
        prog.job_id = scan_job_id;
        prog.documents_total = total_shards;
        prog.percent_complete = 0.0;
        prog.started_at = std::chrono::system_clock::now();
        prog.updated_at = prog.started_at;
        slo_monitor_->recordRepairProgress(prog);
    }

    // Partition shards into bands – one per worker thread
    std::vector<std::vector<ShardInfo>> bands(num_workers);
    for (size_t i = 0; i < all_shards.size(); ++i) {
        bands[i % num_workers].push_back(all_shards[i]);
    }

    // Launch one task per band using std::future for wait-semantics
    std::vector<std::future<void>> band_futures;
    band_futures.reserve(num_workers);

    auto& pool_mgr = themis::utils::getThreadPoolManager();

    for (uint32_t w = 0; w < num_workers; ++w) {
        if (!running_.load()) break;
        if (bands[w].empty()) continue;

        auto promise = std::make_shared<std::promise<void>>();
        band_futures.push_back(promise->get_future());

        bool submitted = pool_mgr.submitTask(
            themis::utils::ThreadPoolManager::PoolType::CPU,
            [this, band = bands[w], scan_job_id, total_shards, promise]() {
                try {
                    scanShardBand(band, scan_job_id, total_shards);
                } catch (const std::exception& e) {
                    spdlog::error("ShardRepairEngine: scan band exception: {}", e.what());
                }
                promise->set_value();
            },
            "repair-scan-band-" + std::to_string(w),
            themis::utils::Task::Priority::NORMAL
        );

        if (!submitted) {
            // Pool rejected the task – run inline and fulfill the promise directly
            spdlog::warn("ShardRepairEngine: thread pool rejected scan task for band {}, running inline", w);
            try {
                scanShardBand(bands[w], scan_job_id, total_shards);
            } catch (const std::exception& e) {
                spdlog::error("ShardRepairEngine: inline scan band exception: {}", e.what());
            }
            promise->set_value();
        }
    }

    // Wait for all bands to complete
    for (auto& f : band_futures) {
        if (f.valid()) {
            f.wait();
        }
    }

    // Mark scan as complete in SLO monitor
    if (slo_monitor_) {
        SLOMonitor::RepairProgress prog;
        prog.job_id = scan_job_id;
        prog.documents_scanned = total_shards;
        prog.documents_total = total_shards;
        prog.percent_complete = 100.0;
        prog.updated_at = std::chrono::system_clock::now();
        prog.completed = true;
        slo_monitor_->recordRepairProgress(prog);
    }

    spdlog::info("ShardRepairEngine: parallel anti-entropy scan complete ({} shards, {} workers)",
                 total_shards, num_workers);
}

void ShardRepairEngine::scanShardBand(const std::vector<ShardInfo>& band,
                                       const std::string& scan_job_id,
                                       uint64_t total_shards) {
    for (const auto& shard_info : band) {
        if (!running_.load()) break;

        const std::string& shard_id = shard_info.shard_id;

        ShardHealthReport report;
        report.shard_id = shard_id;
        report.last_scan = std::chrono::system_clock::now();

        // Obtain document list for this shard (if provider is set)
        std::vector<std::string> doc_ids;
        if (doc_list_provider_) {
            try {
                doc_ids = doc_list_provider_(shard_id);
            } catch (const std::exception& e) {
                spdlog::warn("ShardRepairEngine: doc list failed for shard {}: {}",
                             shard_id, e.what());
            }
        }

        for (const auto& doc_id : doc_ids) {
            if (!running_.load()) break;

            // Enforce the IOPS budget – back-off if the token bucket is empty.
            if (resource_manager_) {
                while (!resource_manager_->acquireRepairIOToken() && running_.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            ++report.documents_scanned;
            {
                std::lock_guard<std::mutex> lock(metrics_mutex_);
                ++metrics_.total_documents_scanned;
            }

            auto health = strategy_.checkDocumentHealth(
                doc_id, report.shard_id, ring_, topology_, read_handler_);

            if (health.is_healthy) {
                ++report.documents_healthy;
            } else if (health.can_recover) {
                ++report.documents_degraded;
                // Auto-enqueue repair if enabled
                if (config_.enable_auto_repair) {
                    triggerDocumentRepair(doc_id, config_.default_collection);
                }
            } else {
                ++report.documents_unrecoverable;
            }
        }

        // Determine shard status from scan results
        if (report.documents_scanned == 0) {
            report.status = ShardRepairStatus::HEALTHY;
        } else if (report.documents_unrecoverable > 0) {
            report.status = ShardRepairStatus::FAILED;
        } else if (report.documents_degraded > 0) {
            report.status = ShardRepairStatus::DEGRADED;
        } else {
            report.status = ShardRepairStatus::HEALTHY;
        }

        {
            std::lock_guard<std::mutex> lock(health_mutex_);
            shard_health_[shard_id] = report;
        }

        // Forward per-shard health to the centralized PrometheusMetrics registry.
        if (prom_metrics_) {
            using S = PrometheusMetrics::RepairShardStatus;
            const char* status_str = S::HEALTHY;
            switch (report.status) {
                case ShardRepairStatus::DEGRADED:   status_str = S::DEGRADED;   break;
                case ShardRepairStatus::FAILED:     status_str = S::FAILED;     break;
                case ShardRepairStatus::REBUILDING: status_str = S::REBUILDING; break;
                default: break;
            }
            prom_metrics_->recordRepairShardStatus(shard_id, status_str);
        }

        // Update SLO progress after each shard
        uint64_t done = scan_shards_done_.fetch_add(1, std::memory_order_relaxed) + 1;
        if (slo_monitor_ && total_shards > 0) {
            SLOMonitor::RepairProgress prog;
            prog.job_id = scan_job_id;
            prog.documents_scanned = done;
            prog.documents_total = total_shards;
            prog.percent_complete = static_cast<double>(done) /
                                    static_cast<double>(total_shards) * 100.0;
            prog.updated_at = std::chrono::system_clock::now();
            slo_monitor_->recordRepairProgress(prog);
        }

        spdlog::debug("ShardRepairEngine: shard {} – scanned={} healthy={} degraded={} "
                      "unrecoverable={}",
                      shard_id, report.documents_scanned, report.documents_healthy,
                      report.documents_degraded, report.documents_unrecoverable);
    }
}

void ShardRepairEngine::executeRepairJob(RepairJob& job) {
    spdlog::info("ShardRepairEngine: executing job {} (shard='{}', doc='{}', full={})",
                 job.job_id,
                 job.shard_id.empty() ? "<all>" : job.shard_id,
                 job.document_id.empty() ? "<all>" : job.document_id,
                 job.is_full_scan);

    // Mark shard as REBUILDING while job runs
    if (!job.shard_id.empty()) {
        std::lock_guard<std::mutex> lock(health_mutex_);
        shard_health_[job.shard_id].status = ShardRepairStatus::REBUILDING;
    }

    // Case 1: Single-document repair
    if (!job.document_id.empty()) {
        ++job.documents_scanned;
        bool ok = repairDocument(job.document_id, job.collection);
        if (ok) {
            ++job.documents_repaired;
        } else {
            ++job.documents_failed;
        }
        job.completed = true;
        job.success = (job.documents_failed == 0);
        job.completed_at = std::chrono::system_clock::now();
        return;
    }

    // Case 2: Shard-level or full-cluster repair
    auto shards_to_repair = topology_.getAllShards();
    if (!job.shard_id.empty()) {
        // Filter to the requested shard
        shards_to_repair.erase(
            std::remove_if(shards_to_repair.begin(), shards_to_repair.end(),
                           [&](const ShardInfo& s) { return s.shard_id != job.shard_id; }),
            shards_to_repair.end());
    }

    for (const auto& shard_info : shards_to_repair) {
        if (!running_.load()) break;

        std::vector<std::string> doc_ids;
        if (doc_list_provider_) {
            try {
                doc_ids = doc_list_provider_(shard_info.shard_id);
            } catch (...) {}
        }

        for (const auto& doc_id : doc_ids) {
            ++job.documents_scanned;
            auto start = std::chrono::steady_clock::now();

            auto health = strategy_.checkDocumentHealth(
                doc_id, job.collection, ring_, topology_, read_handler_);

            if (!health.is_healthy && health.can_recover) {
                bool ok = repairDocument(doc_id, job.collection);
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
                updateMetricsAfterRepair(ok, elapsed);
                if (ok) {
                    ++job.documents_repaired;
                } else {
                    ++job.documents_failed;
                }
            }
        }
    }

    job.completed = true;
    job.success = (job.documents_failed == 0);
    job.completed_at = std::chrono::system_clock::now();

    // Update shard health after repair
    if (!job.shard_id.empty()) {
        std::lock_guard<std::mutex> lock(health_mutex_);
        auto& report = shard_health_[job.shard_id];
        report.last_repair = job.completed_at;
        report.status = (job.documents_failed == 0) ? ShardRepairStatus::HEALTHY
                                                     : ShardRepairStatus::DEGRADED;
    }

    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_.last_repair_time = job.completed_at;
    }

    spdlog::info("ShardRepairEngine: job {} done – scanned={} repaired={} failed={}",
                 job.job_id, job.documents_scanned, job.documents_repaired, job.documents_failed);
}

bool ShardRepairEngine::repairDocument(const std::string& doc_id,
                                        const std::string& collection) {
    // Enforce the IOPS budget before executing the repair write.
    if (resource_manager_) {
        while (!resource_manager_->acquireRepairIOToken() && running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    try {
        return strategy_.recoverDocument(doc_id, collection, ring_, topology_,
                                         read_handler_, write_handler_);
    } catch (const std::exception& e) {
        spdlog::warn("ShardRepairEngine: recoverDocument({}) failed: {}", doc_id, e.what());
        return false;
    }
}

void ShardRepairEngine::updateMetricsAfterRepair(bool success,
                                                  std::chrono::milliseconds duration) {
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        ++metrics_.total_repairs_attempted;
        if (success) {
            ++metrics_.total_repairs_successful;
        } else {
            ++metrics_.total_repairs_failed;
        }
        // Cumulative moving average: avg_n = avg_{n-1} + (x_n - avg_{n-1}) / n
        // Avoids the overflow that would occur when computing avg * (n-1).
        int64_t delta = duration.count() - metrics_.avg_repair_time_ms.count();
        metrics_.avg_repair_time_ms += std::chrono::milliseconds(
            delta / static_cast<int64_t>(metrics_.total_repairs_attempted));
    }

    // Forward operation result to the centralized PrometheusMetrics registry.
    if (prom_metrics_) {
        prom_metrics_->recordRepairOperation(success,
                                             static_cast<double>(duration.count()));
    }
}

}  // namespace sharding
}  // namespace themis
