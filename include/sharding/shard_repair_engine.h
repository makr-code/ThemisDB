/**
 * @file shard_repair_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Shard Repair / Anti-Entropy Engine
 *
 * Implements automated repair and anti-entropy mechanisms for Parity
 * (RAID-5/6) and Mirrored shard setups:
 *
 *  - Periodic background consistency checks between shards
 *  - Automatic detection and queuing of degraded/failed shards
 *  - On-demand rebuild / health-check triggers (API & CLI)
 *  - Prometheus-compatible metrics and repair reporting
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/slo_monitor.h"
#include "utils/thread_pool_manager.h"
#include "utils/expected.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace sharding {

// Forward declaration – avoid pulling in the heavy gossip/gRPC headers from
// shard_resource_manager.h into every translation unit that includes this header.
class ShardResourceManager;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Runtime tuning knobs for anti-entropy scan and repair execution. */
struct RepairConfig {
    /// Interval between periodic anti-entropy scans.
    std::chrono::seconds scan_interval{300};
    /// Interval at which the repair worker polls the job queue.
    std::chrono::seconds repair_poll_interval{30};
    /// Maximum number of documents repaired concurrently.
    uint32_t max_concurrent_repairs = 4;
    /// Maximum documents processed per repair queue drain.
    uint32_t repair_batch_size = 100;
    /// Enable automatic background scanning and repair.
    bool enable_auto_repair = true;
    /// Enable periodic anti-entropy scans.
    bool enable_periodic_scan = true;
    /// Collection name used when scanning (empty = default).
    std::string default_collection;
    /// Number of parallel worker threads for the anti-entropy scan.
    /// Shards are partitioned into scan bands, one per worker.
    /// Set to 0 to use std::thread::hardware_concurrency() at runtime.
    uint32_t num_parallel_workers = 8;
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-shard health status
// ─────────────────────────────────────────────────────────────────────────────

enum class ShardRepairStatus {
    HEALTHY,     ///< All monitored documents are intact.
    DEGRADED,    ///< Some documents are missing replicas / chunks.
    FAILED,      ///< Shard is unreachable or majority of docs unavailable.
    REBUILDING   ///< Active repair is running on this shard.
};

/** @brief Latest per-shard anti-entropy health report. */
struct ShardHealthReport {
    std::string shard_id;
    ShardRepairStatus status = ShardRepairStatus::HEALTHY;
    uint64_t documents_scanned = 0;
    uint64_t documents_healthy = 0;
    uint64_t documents_degraded = 0;
    uint64_t documents_unrecoverable = 0;
    std::chrono::system_clock::time_point last_scan;
    std::chrono::system_clock::time_point last_repair;
    std::string last_error;
};

// ─────────────────────────────────────────────────────────────────────────────
// Repair job
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Queued or completed repair job execution record. */
struct RepairJob {
    std::string job_id;
    /// Target shard (empty = all shards / full cluster scan).
    std::string shard_id;
    /// Target document (empty = all documents in shard).
    std::string document_id;
    /// Collection context.
    std::string collection;
    bool is_full_scan = false;

    std::chrono::system_clock::time_point submitted_at;
    std::chrono::system_clock::time_point completed_at;

    uint64_t documents_scanned = 0;
    uint64_t documents_repaired = 0;
    uint64_t documents_failed = 0;

    bool completed = false;
    bool success = false;
    std::string error_message;
};

// ─────────────────────────────────────────────────────────────────────────────
// Engine metrics
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Aggregate counters/timings for repair-engine activity. */
struct RepairMetrics {
    uint64_t total_scans = 0;
    uint64_t total_repairs_attempted = 0;
    uint64_t total_repairs_successful = 0;
    uint64_t total_repairs_failed = 0;
    uint64_t total_documents_scanned = 0;
    std::chrono::milliseconds avg_repair_time_ms{0};
    std::chrono::system_clock::time_point last_scan_time;
    std::chrono::system_clock::time_point last_repair_time;
    /// Number of worker threads used in the last parallel scan.
    uint32_t last_scan_workers = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ShardRepairEngine
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Anti-Entropy and Shard Repair Engine.
 *
 * Lifecycle:
 *   1. Construct with a RepairConfig and references to the existing
 *      RedundancyStrategy, ConsistentHashRing, and ShardTopology.
 *   2. Provide read / write handlers (lambdas matching the types used by
 *      RedundancyStrategy) and optionally a document-list provider.
 *   3. Call start() to launch background threads.
 *   4. Call triggerRepair() / triggerFullScan() for on-demand operations.
 *   5. Query getRepairMetrics() / exportPrometheusMetrics() for observability.
 *   6. Call stop() on shutdown.
 */
class ShardRepairEngine {
public:
    /// Callback that returns the list of document IDs stored on a shard.
    using DocumentListProvider =
        std::function<std::vector<std::string>(const std::string& shard_id)>;

    /** @brief Construct repair engine with explicit dependencies and config. */
    ShardRepairEngine(const RepairConfig& config,
                      RedundancyStrategy& strategy,
                      ConsistentHashRing& ring,
                      ShardTopology& topology,
                      RedundancyStrategy::ReadHandler read_handler,
                      RedundancyStrategy::WriteHandler write_handler);

    /** @brief Stop background workers and release engine resources. */
    ~ShardRepairEngine();

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /** @brief Start periodic scan and repair worker loops according to config. */
    void start();
    /** @brief Stop scan/repair loops and join worker threads. */
    void stop();
    bool isRunning() const { return running_.load(); }

    // ── Providers ────────────────────────────────────────────────────────────

    /// Inject a function that lists document IDs for a given shard.
    /** @brief Install callback that enumerates document ids for a shard. */
    void setDocumentListProvider(DocumentListProvider provider);

    /**
     * Attach a shared PrometheusMetrics instance so that the engine
     * forwards repair events (scans, operation results, shard health) into
     * the centralized metrics registry.  Optional — if not set, repair events
     * are only visible through exportPrometheusMetrics().
     */
    void setPrometheusMetrics(std::shared_ptr<PrometheusMetrics> prom_metrics);

    /**
     * Attach an SLOMonitor so that scan and repair progress is reported for
     * operator-visible time-to-full-repair tracking.  Optional — if not set,
     * progress is only available through getRepairMetrics().
     */
    void setSLOMonitor(std::shared_ptr<SLOMonitor> slo_monitor);

    /**
     * Attach a ShardResourceManager so that the repair engine can enforce the
     * configured IOPS budget (`acquireRepairIOToken`) and consult the GPU
     * erasure-coding feature flag (`isGPUErasureCodingEnabled`).  Optional —
     * if not set, the engine runs without I/O throttling and always uses the
     * CPU erasure-coding path.
     */
    void setResourceManager(std::shared_ptr<ShardResourceManager> resource_manager);

    // ── On-demand triggers (API / CLI) ────────────────────────────────────────

    /**
     * Enqueue a repair job for a specific shard (or all shards if empty).
     * Returns the job ID that can be used with getJobStatus().
     */
    std::string triggerRepair(const std::string& shard_id = "");

    /**
     * Enqueue a full cluster-wide anti-entropy scan + repair.
     * Returns the job ID.
     */
    std::string triggerFullScan();

    /**
     * Enqueue repair of a single document.
     * @param document_id Document ID to repair (non-empty required)
     * @param collection Collection name; if empty, uses default collection
     * @return Job ID if enqueued successfully, empty string if document_id is empty (fail-closed)
     * @note Rejects empty document_id fail-closed to prevent silent repair failures
     */
    std::string triggerDocumentRepair(const std::string& document_id,
                                      const std::string& collection = "");

    // ── Status / reporting ────────────────────────────────────────────────────

    /** @brief Return current status for one repair job id. */
    RepairJob getJobStatus(const std::string& job_id) const;
    /** @brief Return all currently active (not yet completed) repair jobs. */
    std::vector<RepairJob> getActiveJobs() const;
    /** @brief Return latest cached shard health reports. */
    std::vector<ShardHealthReport> getShardHealthReports() const;

    /** @brief Return aggregate repair engine counters and timing metrics. */
    RepairMetrics getRepairMetrics() const;

    /// Export current metrics in Prometheus text exposition format.
    std::string exportPrometheusMetrics() const;

    /**
     * @brief Run a synchronous replica consistency check across all shards.
     *
     * Iterates the last-known per-shard health reports (populated by the
     * background scan loop or the most recent triggerFullScan() call) and
     * returns a human-readable summary string.  The summary format is:
     *
     *   "Replica validation OK: <n> shard(s) healthy"          — all healthy
     *   "Replica validation: <n> degraded / <m> unrecoverable shard(s) found" — issues found
     *
     * This method is intentionally lightweight and non-blocking: it reads
     * cached state rather than triggering a new network scan.  Use
     * triggerFullScan() before calling this if fresh data is required.
     *
     * Designed to be passed as the `check_fn` of a
     * `ReplicaValidationHandler` registered with the
     * `DatabaseMaintenanceOrchestrator`:
     * @code
     *   auto handler = std::make_shared<ReplicaValidationHandler>(
     *       [engine]() { return engine->runConsistencyCheck(); });
     *   orchestrator.registerTaskHandler(
     *       MaintenanceTaskType::REPLICA_VALIDATION,
     *       std::move(handler));
     * @endcode
     *
     * @return Ok<std::string> with the summary message.
     * @return Err<Error>      if reports indicate unrecoverable shard(s)
     *                         (ErrorCode::ERR_STORAGE_TRANSACTION_FAILED).
     */
    themis::Result<std::string> runConsistencyCheck() const;

private:
    // ── Background threads ────────────────────────────────────────────────────

    /** @brief Background periodic anti-entropy scan loop body. */
    void scanLoop();
    /** @brief Background repair job execution loop body. */
    void repairLoop();

    // ── Internal operations ───────────────────────────────────────────────────

    /** @brief Run one anti-entropy scan pass over configured shard set. */
    void performAntiEntropyScan();
    /** @brief Scan one shard band (worker partition) and update reports/progress. */
    void scanShardBand(const std::vector<ShardInfo>& band,
                       const std::string& scan_job_id,
                       uint64_t total_shards);
    /** @brief Execute one queued repair job and update job/report/metric state. */
    void executeRepairJob(RepairJob& job);
    /** @brief Attempt repair of one document in target collection. */
    bool repairDocument(const std::string& doc_id, const std::string& collection);

    // ── Helpers ───────────────────────────────────────────────────────────────

    /** @brief Generate unique repair job identifier string. */
    std::string generateJobId() const;
    /** @brief Update aggregate repair metrics after one repair attempt. */
    void updateMetricsAfterRepair(bool success, std::chrono::milliseconds duration);

    // ── Members ───────────────────────────────────────────────────────────────

    RepairConfig config_;
    RedundancyStrategy& strategy_;
    ConsistentHashRing& ring_;
    ShardTopology& topology_;
    RedundancyStrategy::ReadHandler read_handler_;
    RedundancyStrategy::WriteHandler write_handler_;
    DocumentListProvider doc_list_provider_;
    /// Optional centralized metrics registry (set via setPrometheusMetrics).
    std::shared_ptr<PrometheusMetrics> prom_metrics_;
    /// Optional SLO monitor for repair-progress tracking.
    std::shared_ptr<SLOMonitor> slo_monitor_;
    /// Optional resource manager for IOPS throttle and GPU feature-flag queries.
    std::shared_ptr<ShardResourceManager> resource_manager_;

    std::atomic<bool> running_{false};
    std::thread scan_thread_;
    std::thread repair_thread_;

    // Job registry
    mutable std::timed_mutex jobs_mutex_;
    std::map<std::string, RepairJob> jobs_;
    std::queue<std::string> job_queue_;  // job IDs pending execution
    std::condition_variable_any repair_cv_;

    // Per-shard health reports
    mutable std::mutex health_mutex_;
    std::map<std::string, ShardHealthReport> shard_health_;

    // Aggregated metrics
    mutable std::mutex metrics_mutex_;
    RepairMetrics metrics_;

    // Monotonic counter for job IDs
    mutable std::atomic<uint64_t> job_counter_{0};

    // Scan-progress counters shared across worker threads (atomic)
    std::atomic<uint64_t> scan_shards_done_{0};
    std::atomic<uint64_t> scan_shards_total_{0};
};

}  // namespace sharding
}  // namespace themis

// Backward-compatibility alias
namespace themisdb {
namespace sharding {
using themis::sharding::RepairConfig;
using themis::sharding::RepairJob;
using themis::sharding::RepairMetrics;
using themis::sharding::ShardHealthReport;
using themis::sharding::ShardRepairEngine;
using themis::sharding::ShardRepairStatus;
}  // namespace sharding
}  // namespace themisdb
