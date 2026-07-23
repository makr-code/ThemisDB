/**
 * @file i_ssm_plugin.h
 * @brief SSM / Mamba backend plugin interface for ThemisDB LLM stack.
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL (Phase 1 PoC)
 * @note Gap Summary: Interface draft; implementation pending Phase 1 design review
 * @note Status: Pending human architect approval (P1-D01 design gate)
 * @note This file is auto-generated and will be updated per design review feedback.
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "core/timestamp.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::llm {

/**
 * @brief SSM hidden state snapshot with HLC binding.
 *
 * Carries serialized SSM state associated with a transaction snapshot timestamp.
 * Used for state checkpoint/restore across request boundaries and session persistence.
 */
struct SSMStateSnapshot {
    /// Transaction-consistent timestamp (HLC) when snapshot was taken
    core::HLCTimestamp snapshot_ts;

    /// Serialized hidden state (opaque to caller)
    std::vector<uint8_t> state_data;

    /// Fingerprint for integrity check (model architecture hash)
    std::string state_fingerprint;

    /// Sequence counter: tokens processed up to this snapshot
    uint64_t sequence_counter = 0;

    // Backwards-compatibility fields expected by older tests
    std::string session_id;
    std::string hidden_state; // string form of state_data for legacy tests
    std::string cell_state;
    std::string metadata; // JSON string metadata
};

/**
 * @brief SSM / Mamba backend plugin interface.
 *
 * Extends ILLMPlugin to add stateful SSM-specific operations:
 * - State update with token batch
 * - Snapshot generation for persistence
 * - Restore from checkpoint
 * - State reset
 *
 * **Activation:** Registered via `LLMPluginManager::registerPlugin()` at runtime.
 * **Feature Flag:** Build: `THEMIS_ENABLE_SSM_PLUGIN`, Runtime: `ssm_plugin_enabled`
 * **Thread Safety:** Must be thread-safe for concurrent calls within session boundary
 *
 * **Failure Mode:** Any unrecoverable error in state ops triggers automatic
 * fallback to transformer path via `KnowledgeGapDetector` + `AgenticRAG`.
 */
struct ISSMPlugin : public ILLMPlugin {
    virtual ~ISSMPlugin() = default;

    /**
     * @brief Update SSM hidden state with new token batch.
     *
     * **Contract:** Processes a sequence of tokens and updates the
     * internal hidden state representation.
     *
     * @param tokens Token IDs to process (typically [0..32k) range)
     * @return true if state update succeeded, false if error occurred
     *
     * **Error Cases (false return):**
     * - Invalid token ID
     * - State corruption detected
     * - Resource exhaustion (hidden state buffer full)
     * - Backend not initialized
     *
     * **Caller Response:** Return false → caller triggers `invalidate()` and
     * falls back to transformer path.
     *
     * @throws std::runtime_error if implementation detects fatal corruption
     */
    virtual bool updateState(const std::vector<int32_t>& tokens) = 0;

    /**
     * @brief Generate a checkpoint of current hidden state.
     *
     * **Contract:** Returns a serialized snapshot suitable for persistence or
     * cross-shard transfer. Snapshot is read-only and independent of future
     * state updates.
     *
     * @param snapshot_ts HLC timestamp for MVCC binding
     * @return SSMStateSnapshot (can be default/empty if error)
     *
     * @throws std::runtime_error if state is invalid or serialization fails
     *
     * **Usage:** Called before session boundary, before cross-shard hand-off,
     * before storage persistence (Phase 2).
     */
    virtual SSMStateSnapshot getStateSnapshot(core::HLCTimestamp snapshot_ts) = 0;

    /**
     * @brief Restore hidden state from a checkpoint.
     *
     * **Contract:** Loads serialized state back into the plugin. All subsequent
     * calls to updateState() continue from this restored point.
     *
     * @param snapshot Checkpoint to restore from
     * @return true if restore succeeded, false if snapshot is invalid/corrupted
     *
     * **Error Cases (false return):**
     * - Snapshot fingerprint mismatch (model architecture changed)
     * - Corrupted state data
     * - HLC timestamp out of valid range
     *
     * **Caller Response:** Return false → caller must re-init with resetState()
     * and restart from clean slate, triggering full context re-fetch from
     * retrieval system.
     *
     * @throws std::runtime_error if implementation detects fatal issues
     */
    virtual bool restoreState(const SSMStateSnapshot& snapshot) = 0;

    /**
     * @brief Reset state to uninitialized / clean slate.
     *
     * **Contract:** Clears all hidden state. Next updateState() call will
     * start from token sequence #0.
     *
     * **Usage:** Called on session init, after restore() error, on explicit
     * client session reset.
     *
     * @throws std::runtime_error if state reset fails (unusual)
     */
    virtual void resetState() = 0;

    /**
     * @brief Query current state retention score.
     *
     * **Contract:** Returns a numeric [0.0, 1.0] quality metric indicating
     * how much useful context remains in the hidden state vs. freshness decay.
     *
     * Used by `KnowledgeGapDetector` to decide whether RAG refresh is needed.
     * Higher value = more context retained.
     *
     * @return Score in [0.0, 1.0]
     *
     * **Semantics:**
     * - 1.0 = just initialized, no decay
     * - 0.8–0.9 = moderate context age, still fresh
     * - 0.5–0.8 = older context, retrieval recommended
     * - 0.0–0.5 = very stale, likely needs refresh
     * - 0.0 = empty or uninitialized
     */
    virtual double getStateRetentionScore() const = 0;

    /**
     * @brief Get the fingerprint of the current model/architecture.
     *
     * Used to validate snapshot compatibility on restore. If fingerprints
     * don't match, restore() should fail with a clear error.
     *
     * @return Model architecture hash (e.g., SHA256 of config + weights metadata)
     */
    virtual std::string getStateFingerprint() const = 0;
};

}  // namespace themis::llm

