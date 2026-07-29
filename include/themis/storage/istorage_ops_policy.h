/**
 * @file istorage_ops_policy.h
 * @brief Abstract storage-operations policy for background jobs, compaction,
 *        and snapshots in ThemisDB.
 *
 * Governs three storage-operation axes: concurrent background maintenance
 * jobs, compaction I/O throughput, and simultaneous snapshot operations.
 * Sits in Tier 2 of the four-tier resource-governance chain:
 *
 * @code
 *   compile-time constexpr (edition.h)    ← absolute ceiling, never overridable
 *   RuntimeLicenseGate                    ← edition-tier ceiling
 *   IStorageOpsPolicy   (this file)       ← signed-plugin fine-tuning
 *   StorageOpsConfig                      ← per-deployment operational tuning
 * @endcode
 *
 * All implementations must be individually thread-safe.
 *
 * @note This interface is part of the edition-policy plugin contract
 *       (IEditionPolicyPlugin::createStorageOpsPolicy).  Claimed limits are
 *       validated against the compile-time ceilings in edition.h
 *       (STORAGE_MAX_BACKGROUND_JOBS, STORAGE_MAX_COMPACTION_BYTES_PER_SEC,
 *        STORAGE_MAX_CONCURRENT_SNAPSHOTS) before EditionManager accepts the policy.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace storage {

/**
 * @brief Abstract storage-operations limit policy.
 *
 * Controls three storage-maintenance axes:
 *  - **Background jobs** — maximum concurrently running maintenance jobs
 *    (index rebuild, GC, WAL archival, etc.).
 *  - **Compaction rate** — maximum bytes-per-second fed to the compaction engine.
 *  - **Snapshots** — maximum concurrently active snapshot operations.
 *
 * Implementations are installed into EditionManager via
 * `EditionManager::installStorageOpsPolicy()` and consulted by the storage
 * engine before scheduling any background operation.
 */
class IStorageOpsPolicy {
public:
    virtual ~IStorageOpsPolicy() = default;

    // Non-copyable, non-movable by default.
    IStorageOpsPolicy(const IStorageOpsPolicy&)            = delete;
    IStorageOpsPolicy& operator=(const IStorageOpsPolicy&) = delete;

    // -------------------------------------------------------------------------
    // Background jobs
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff scheduling one more background job is permitted.
     *
     * Does not modify accounting state — call onJobStarted() only after the
     * job has been submitted to the job scheduler.
     *
     * @return true when another background job slot is available.
     */
    [[nodiscard]] virtual bool canScheduleJob() const = 0;

    /**
     * @brief Notify the policy that a background job has started.
     *
     * Updates concurrent-job accounting.  Thread-safe.
     *
     * @param job_id  Opaque identifier for the job (non-empty string).
     */
    virtual void onJobStarted(const std::string& job_id) = 0;

    /**
     * @brief Notify the policy that a background job has finished.
     *
     * Releases the job slot.  Implementations must clamp to zero on
     * mismatched calls.  Thread-safe.
     *
     * @param job_id  Job identifier previously passed to onJobStarted().
     */
    virtual void onJobFinished(const std::string& job_id) = 0;

    /**
     * @brief Return the number of background jobs currently tracked as running.
     */
    [[nodiscard]] virtual int32_t activeJobCount() const = 0;

    /**
     * @brief Maximum concurrent background jobs; -1 = unlimited.
     */
    [[nodiscard]] virtual int32_t maxBackgroundJobs() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Compaction rate
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff writing @p bytes to the compaction engine right
     *        now is within the allowed throughput budget.
     *
     * Uses token-bucket semantics.  May consume tokens — callers must
     * throttle when this returns false.
     *
     * @param bytes  Bytes the compaction engine intends to write in this step.
     * @return true when the write is within the allowed rate.
     */
    [[nodiscard]] virtual bool allowCompactionBytes(uint64_t bytes) = 0;

    /**
     * @brief Maximum compaction I/O throughput in bytes per second; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t maxCompactionBytesPerSec() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Snapshot operations
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff starting one more snapshot operation is permitted.
     *
     * Does not modify accounting state — call onSnapshotStarted() only after
     * the snapshot has been initiated.
     *
     * @return true when another snapshot slot is available.
     */
    [[nodiscard]] virtual bool canStartSnapshot() const = 0;

    /**
     * @brief Notify the policy that a snapshot operation has started.
     *
     * Updates snapshot accounting.  Thread-safe.
     *
     * @param snapshot_id  Opaque snapshot identifier (non-empty string).
     */
    virtual void onSnapshotStarted(const std::string& snapshot_id) = 0;

    /**
     * @brief Notify the policy that a snapshot operation has completed or failed.
     *
     * Releases the snapshot slot.  Implementations must clamp to zero on
     * mismatched calls.  Thread-safe.
     *
     * @param snapshot_id  Snapshot identifier previously passed to onSnapshotStarted().
     */
    virtual void onSnapshotFinished(const std::string& snapshot_id) = 0;

    /**
     * @brief Maximum concurrent snapshot operations; -1 = unlimited.
     */
    [[nodiscard]] virtual int32_t maxConcurrentSnapshots() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Status
    // -------------------------------------------------------------------------

    /**
     * @brief Return true when any storage-ops limit enforcement is active.
     *
     * Implementations should return false when all limits are unlimited
     * (-1 or 0) so that callers may bypass the check on hot paths.
     */
    [[nodiscard]] virtual bool isEnforced() const noexcept = 0;

protected:
    IStorageOpsPolicy() = default;
    IStorageOpsPolicy(IStorageOpsPolicy&&) = default;
    IStorageOpsPolicy& operator=(IStorageOpsPolicy&&) = default;
};

} // namespace storage
} // namespace themis
