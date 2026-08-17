/**
 * @file tnsr_task.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_network_storage_engine.h"
#include "storage/tensor_train_decomposer.h"
#include "tensor/hiss_structural_search.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// TNSRConfig — tuning knobs for one TNSR run
// ============================================================================

struct TNSRConfig {
    /**
     * @brief Reconstruction error tolerance used during recompression.
     *
     * Passed verbatim to `TensorTrainDecomposer::recompress()`.
     * Default: 0.01 (1% error — matches TensorCompactionFilter default).
     */
    double epsilon = 0.01;

    /**
     * @brief Maximum number of topology changes (rerouteEdge calls) per run.
     *
     * Caps the amount of structural mutation per TNSR invocation to bound
     * overhead.  Default: 4.
     */
    std::size_t max_topology_changes_per_run = 4;

    /**
     * @brief Minimum byte savings required before writing the result back.
     *
     * If the recompressed form is less than this many bytes smaller than the
     * stored form, the write is skipped (avoids write amplification for
     * marginal gains).  Default: 64 bytes.
     */
    std::size_t min_bytes_saved_to_commit = 64;

    /**
     * @brief Minimum hours between successive TNSR runs per index.
     *
     * Callers are responsible for enforcing this via a schedule; TNSRTask
     * itself does not track last-run timestamps.  Default: 6.
     */
    std::size_t run_frequency_hours = 6;

    /**
     * @brief Hiss config forwarded to HissStructuralSearchEngine::search().
     *
     * Controls entropy-guided skip-edge sampling used for topology
     * analysis.  Default values are appropriate for most workloads.
     */
    HissConfig hiss_config = {};
};

// ============================================================================
// TNSRReport — result of one TNSR run
// ============================================================================

struct TNSRReport {
    /// Total bytes saved by recompression (before – after serialised sizes).
    std::size_t bytes_saved = 0;

    /// Sum of (old_max_rank - new_max_rank) across all keys that were rewritten.
    /// Positive values indicate rank reduction (storage savings); negative values
    /// indicate rank increase (unusual — possible only when recompression with a
    /// looser epsilon yields a higher-rank intermediate train, which is rare).
    std::int64_t rank_delta = 0;

    /// Number of rerouteEdge calls applied across all keys.
    std::size_t topology_changes = 0;

    /// Number of keys where topology-search was skipped by fast-path.
    ///
    /// Fast-path applies when the recompressed train is structurally trivial
    /// (`cores.size() < 3` or `maxRank() < 2`), where HISS topology analysis
    /// cannot produce useful non-chain mutations.
    std::size_t topology_search_skipped_keys = 0;

    /// Number of keys inspected (including skipped ones).
    std::size_t keys_processed = 0;

    /// Number of keys actually rewritten (saved ≥ min_bytes_saved_to_commit).
    std::size_t keys_rewritten = 0;

    /// Wall-clock duration of the run in milliseconds.
    double duration_ms = 0.0;

    /**
     * @brief Whether the run produced any error (e.g. deserialisation failure).
     *
     * Individual key errors are counted in `error_count` but do not abort
     * the run; the task continues with remaining keys.
     */
    std::size_t error_count = 0;
};

// ============================================================================
// TNSRTask — background structural rounding
// ============================================================================

/**
 * @brief Background task that performs TensorNetworkStructuralRounding.
 *
 * ### STUB/SIMULATION NOTE (STUB #252):
 * Purpose: Phase-6 TNSR skeleton — rank reduction (recompress) is
 *          production-quality; topology mutation via rerouteEdge is a
 *          demonstration path that rebuilds the in-memory TensorNetworkGraph
 *          but does NOT yet re-serialise the mutated topology back to
 *          storage.  The on-disk TT representation is unchanged by topology
 *          mutation in this release.
 * Activation: Always when run() is called and no RerouteSerializeFn is set.
 * Production Delta: Topology changes are counted but not persisted; only
 *                   bond-dimension reductions (rank_delta > 0) are durable.
 * Removal Plan: Q3 2028 — implement topology-guided TT re-serialisation
 *               that maps rerouteEdge suggestions to re-contraction + storage.
 *
 * ### Bridge Injection (STUB #252)
 *
 * Call `setRerouteSerializeFn(fn)` to install a callable that receives the
 * mutated `TensorNetworkGraph` after all `rerouteEdge()` calls for one key
 * and must persist the changed topology to storage.  The callable signature:
 *
 * ```cpp
 * bool fn(storage::TensorNetworkStorageEngine& engine,
 *         const storage::TensorFieldKey&       field_key,
 *         const TensorNetworkGraph&            tng,
 *         const storage::TTTrain&              train);
 * ```
 *
     * Returning `true` indicates success (the key is counted in `keys_rewritten`
     * for topology changes); returning `false` increments `error_count`.
     *
     * ### Thread safety
 * `run()` takes exclusive ownership of the engine for the duration of the
 * run (holds the engine's internal write lock during each `put()`).  Do not
 * call `run()` from multiple threads concurrently on the same engine.
 */
class TNSRTask {
public:
    // -------------------------------------------------------------------------
    // STUB #252 bridge — topology re-serialization
    // -------------------------------------------------------------------------

    /**
     * @brief Callable type for topology-guided re-serialization.
     *
     * @param engine    Storage engine to write to.
     * @param field_key Key of the tensor being processed.
     * @param tng       Mutated TensorNetworkGraph (after rerouteEdge calls).
     * @param train     Recompressed TTTrain to persist.
     * @return `true` on success, `false` on failure.
     */
    using RerouteSerializeFn =
        std::function<bool(storage::TensorNetworkStorageEngine& engine,
                           const storage::TensorFieldKey&       field_key,
                           const TensorNetworkGraph&            tng,
                           const storage::TTTrain&              train)>;

    /// Install a topology re-serialization backend.  Thread-safe.
    static void setRerouteSerializeFn(RerouteSerializeFn fn);

    /// Remove the topology re-serialization backend (fallback: count-only).
    static void clearRerouteSerializeFn();

    /**
     * @brief Construct with an engine and optional decomposer.
     *
     * @param engine      The tensor storage engine to process.
     * @param decomposer  Decomposer used for `recompress()`.  Defaults to a
     *                    freshly constructed (stateless) instance.
     */
    explicit TNSRTask(
        std::shared_ptr<storage::TensorNetworkStorageEngine> engine,
        storage::TensorTrainDecomposer decomposer = {});

    ~TNSRTask() = default;

    // Non-copyable, movable
    TNSRTask(const TNSRTask&) = delete;
    TNSRTask& operator=(const TNSRTask&) = delete;
    TNSRTask(TNSRTask&&) noexcept = default;
    TNSRTask& operator=(TNSRTask&&) noexcept = default;

    /**
     * @brief Run TNSR on a set of storage keys.
     *
     * Iterates over `index_key_range`, recompresses each TTTrain with the
     * configured epsilon, analyses topology with `HissStructuralSearchEngine`,
     * and writes back any train whose serialised size decreases by at least
     * `cfg.min_bytes_saved_to_commit` bytes.
     *
     * @param index_key_range  Storage keys to process (logical field addresses).
     * @param cfg              Task configuration.
     * @return                 TNSRReport summarising the run.
     */
    TNSRReport run(
        const std::vector<storage::TensorFieldKey>& index_key_range,
        const TNSRConfig& cfg = {});

    /**
     * @brief Cancel a running `run()` call from another thread.
     *
     * Sets an atomic flag that `run()` checks between keys.  The current key
     * is completed before the run exits.  Safe to call concurrently.
     */
    void requestCancel() noexcept { cancel_requested_.store(true, std::memory_order_release); }

    /**
     * @brief Clear the cancel flag (call before a new run after cancellation).
     */
    void clearCancel() noexcept { cancel_requested_.store(false, std::memory_order_release); }

    /**
     * @brief Return true if a cancel has been requested.
     */
    bool isCancelRequested() const noexcept {
        return cancel_requested_.load(std::memory_order_acquire);
    }

    [[nodiscard]] static bool hasRerouteSerializeFn();
    [[nodiscard]] static RerouteSerializeFn getRerouteSerializeFn();

private:
    std::shared_ptr<storage::TensorNetworkStorageEngine> engine_;
    storage::TensorTrainDecomposer                       decomposer_;
    HissStructuralSearchEngine                           hiss_engine_;
    std::atomic<bool>                                    cancel_requested_{false};
};

} // namespace tensor
} // namespace themis
