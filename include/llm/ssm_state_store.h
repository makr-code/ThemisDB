/**
 * @file ssm_state_store.h
 * @brief SSM state persistence interface for ThemisDB LLM stack.
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL (Phase 1 PoC)
 * @note Gap Summary: Interface draft; in-memory impl for Phase 1, RocksDB for Phase 2
 * @note Status: In-memory implementation only
 * @note This file is auto-generated and will be updated per Phase 1 gates.
 */

#pragma once

#include "llm/i_ssm_plugin.h"
#include "storage/hlc.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace themis::llm {

/**
 * @brief SSM state persistence storage interface.
 *
 * Responsible for checkpoint/resume of SSM hidden state across session
 * boundaries, cross-shard transfers, and long-term persistence (Phase 2+).
 *
 * **Pattern:** Analog to `IKVStateSerializer` (include/llm/kv_prefix_transfer_manager.h)
 * **HLC Binding:** All snapshots tied to `HLCTimestamp` for MVCC isolation
 * **Governance:** ThemisDB remains System-of-Record; storage is internal
 *
 * **Phase 1 Implementation:** In-memory only (SessionID → snapshots map)
 * **Phase 2 Implementation:** RocksDB backend with HLC-stamped keys
 */
struct ISSMStateStore {
    virtual ~ISSMStateStore() = default;

    /**
     * @brief Checkpoint SSM state for a session.
     *
     * Stores the provided snapshot under the given session ID and HLC timestamp.
     * Multiple snapshots per session are allowed (for session history/rollback).
     *
     * @param session_id Unique session identifier
     * @param snapshot State snapshot to checkpoint
     * @return true if checkpoint succeeded, false if storage error
     *
     * **Error Cases (false return):**
     * - Storage backend unavailable
     * - Duplicate checkpoint (session_id + snapshot_ts already exists)
     * - Corrupt snapshot data
     *
     * **Thread Safety:** Safe for concurrent calls with different session_ids
     *
     * @throws std::runtime_error if implementation detects fatal storage issue
     */
    virtual bool checkpoint(const std::string& session_id,
                            const SSMStateSnapshot& snapshot) = 0;

    /**
     * @brief Resume SSM state for a session.
     *
     * Retrieves the most recent (or specified) snapshot for a session.
     * Returns empty optional if no checkpoint exists.
     *
     * @param session_id Unique session identifier
     * @param snapshot_ts Optional: specific HLC timestamp to retrieve.
     *                    If not provided, returns most recent snapshot.
     * @return SSMStateSnapshot if found, empty optional if not found
     *
     * @throws std::runtime_error if storage backend fails
     */
    virtual std::optional<SSMStateSnapshot> resume(
        const std::string& session_id,
        const std::optional<HLCTimestamp>& snapshot_ts = std::nullopt) = 0;

    /**
     * @brief Invalidate SSM state for a session.
     *
     * Clears all snapshots for the given session ID (used on error recovery
     * or session cleanup).
     *
     * @param session_id Unique session identifier
     * @return true if invalidation succeeded, false if already empty/missing
     *
     * @throws std::runtime_error if storage backend fails
     */
    virtual bool invalidate(const std::string& session_id) = 0;

    /**
     * @brief Compact old snapshots.
     *
     * Removes old snapshots outside retention window (used for storage cleanup).
     *
     * **Phase 1:** No-op (in-memory store)
     * **Phase 2:** RocksDB cleanup using HLC timestamp range
     *
     * @param retention_window_ms Time window (ms) to retain snapshots
     * @return Number of snapshots deleted
     *
     * @throws std::runtime_error if storage backend fails
     */
    virtual uint64_t compact(uint64_t retention_window_ms) = 0;

    /**
     * @brief Get storage statistics (for observability).
     *
     * @return JSON string with stats (session_count, total_snapshots, size_mb, etc.)
     */
    virtual std::string getStats() const = 0;
};

/**
 * @brief In-memory SSM state store (Phase 1 implementation).
 *
 * **STUB/SIMULATION NOTE:**
 * Purpose: Validate SSM state persistence dataflow without RocksDB dependency
 * Activation: Always active in Phase 1 (build flag THEMIS_ENABLE_SSM_PLUGIN=1)
 * Production Delta: Uses in-memory map; lost on process restart
 * Removal Plan: Replace with RocksDB backend in Phase 2 (P2-D04)
 */
class InMemorySSMStateStore : public ISSMStateStore {
public:
    explicit InMemorySSMStateStore(size_t max_snapshots_per_session = 10);

    bool checkpoint(const std::string& session_id,
                    const SSMStateSnapshot& snapshot) override;

    std::optional<SSMStateSnapshot> resume(
        const std::string& session_id,
        const std::optional<HLCTimestamp>& snapshot_ts =
            std::nullopt) override;

    bool invalidate(const std::string& session_id) override;

    uint64_t compact(uint64_t retention_window_ms) override;

    std::string getStats() const override;

private:
    /// Map: session_id → list of (HLC timestamp, snapshot)
    std::unordered_map<std::string, std::vector<SSMStateSnapshot>>
        state_by_session_;

    size_t max_snapshots_per_session_;

    mutable std::mutex mu_;
};

}  // namespace themis::llm

