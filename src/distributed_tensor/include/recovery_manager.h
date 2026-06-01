/**
 * @file recovery_manager.h
 * @brief Recovery, rebuild, and erasure-coding strategy for distributed tensor artifacts.
 *
 * Orchestrates stripe reconstruction after node failures using Reed-Solomon
 * erasure coding or simpler replication fallback.
 *
 * Planned in: docs/EPIC3_RECOVERY_STRATEGY.md
 * Sub-issue:   #5433
 */

#pragma once

#include "artifact_manifest.h"
#include "shard_placement.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::distributed_tensor {

/// Erasure strategy for a recovery operation.
enum class RecoveryStrategy {
    ReedSolomon,  ///< Reed-Solomon reconstruction from parity stripes
    Replication,  ///< Re-fetch from replica (fallback when RS not feasible)
    Manual,       ///< Operator-driven recovery; not automated
};

/// Status of a recovery job.
enum class RecoveryStatus {
    Queued,
    InProgress,
    Completed,
    Failed,
};

/// A recovery job record.
struct RecoveryJob {
    std::string     job_id;
    std::string     artifact_id;
    RecoveryStrategy strategy;
    RecoveryStatus  status = RecoveryStatus::Queued;
    std::vector<std::string> missing_shard_keys;
    std::vector<std::string> available_shard_keys;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point completed_at;
    std::string     failure_reason;
};

/**
 * @brief Recovery manager interface.
 *
 * Detects stripe losses via the manifest, plans and executes reconstruction,
 * and updates the manifest upon completion.
 */
class IRecoveryManager {
public:
    virtual ~IRecoveryManager() = default;

    /// Enqueue a recovery job for an artifact with partial loss.
    virtual std::string enqueue(const std::string& artifact_id,
                                 RecoveryStrategy strategy) = 0;

    /// Return the status of a recovery job.
    virtual std::optional<RecoveryJob> statusOf(
        const std::string& job_id) const = 0;

    /// Execute the next pending job synchronously (for testing / draining).
    virtual RecoveryJob executeNext() = 0;

    /// List all job IDs with a given status.
    virtual std::vector<std::string> listJobs(
        RecoveryStatus status) const = 0;

    /// Register a callback invoked when a job changes status.
    using StatusCallback = std::function<void(const RecoveryJob&)>;
    virtual void onStatusChange(StatusCallback cb) = 0;
};

/// Factory: create a recovery manager backed by the given manifest store.
std::unique_ptr<IRecoveryManager> makeRecoveryManager(
    std::shared_ptr<IManifestStore> manifest);

} // namespace themis::distributed_tensor
