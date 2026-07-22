# P1-D01: ISSMPlugin Interface Design Review

**Status:** PENDING HUMAN REVIEW (Design Gate Required)  
**Document Type:** Technical Design Specification  
**Target:** `develop`  
**Scope:** SSM backend plugin architecture for ThemisDB LLM stack  

---

## Purpose

Define the `ISSMPlugin` interface that allows SSM/Mamba backends to be plugged into ThemisDB's `ILLMPlugin` architecture without modifying the core inference pipeline.

---

## Design Goals

1. **Minimal Core Disruption:** All SSM-specific logic encapsulated in plugin
2. **State Persistence Contract:** Snapshots tied to HLC timestamps for MVCC consistency
3. **Graceful Fallback:** SSM failures automatically route to transformer pipeline
4. **Governance Compliance:** All state remains under ThemisDB control; RocksDB internal only

---

## Proposed Interface

```cpp
#pragma once

#include "llm/llm_plugin_interface.h"
#include "core/timestamp.h"  // HLCTimestamp
#include <vector>
#include <cstdint>
#include <memory>

namespace themis::llm {

/**
 * @brief SSM hidden state snapshot with HLC binding.
 * 
 * Carries serialized SSM state associated with a transaction snapshot timestamp.
 * Used for state checkpoint/restore across request boundaries.
 */
struct SSMStateSnapshot {
    /// Transaction-consistent timestamp (HLC) when snapshot was taken
    core::HLCTimestamp snapshot_ts;
    
    /// Serialized hidden state (opaque to caller)
    std::vector<uint8_t> state_data;
    
    /// Fingerprint for integrity check (model architecture hash)
    std::string state_fingerprint;
    
    /// Sequence counter (tokens processed up to this snapshot)
    uint64_t sequence_counter = 0;
};

/**
 * @brief SSM / Mamba backend plugin interface.
 * 
 * Extends ILLMPlugin to add stateful SSM-specific operations:
 * - State update with token batch
 * - Snapshot generation for persistence
 * - Restore from checkpoint
 * - State reset to clean slate
 * 
 * **Activation:** Registered via `LLMPluginManager::registerPlugin()` at runtime.
 * **Feature Flag:** Build: `THEMIS_ENABLE_SSM_PLUGIN`, Runtime: `ssm_plugin_enabled`
 * 
 * **Failure Mode:** Any unrecoverable error in state ops triggers automatic
 * fallback to transformer path via `KnowledgeGapDetector` + `AgenticRAG`.
 */
struct ISSMPlugin : public ILLMPlugin {
    virtual ~ISSMPlugin() = default;
    
    /**
     * @brief Update SSM hidden state with new token batch.
     * 
     * **Contract:** This method processes a sequence of tokens and updates the 
     * internal hidden state representation. Must be thread-safe for concurrent
     * calls within the same session boundary.
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
```

---

## Design Review Gates (MUST COMPLETE BEFORE IMPLEMENTATION)

### Gate 1: SSMStateStore Distribution Strategy
**Question:** How should SSM state be distributed across shards in a multi-shard deployment?

**Options:**
a) **Replication:** Each shard maintains a full copy of global SSM state (simple, high memory)
b) **Shard-Partitioned:** Different state layers partitioned across shards (complex routing, low memory)
c) **Single-Master:** One shard holds authoritative state, others query/replicate (hybrid)

**Decision Needed:** Recommend `__________` for Phase 1 PoC; can defer multi-shard strategy to Phase 3

### Gate 2: Failure Semantics on Cross-Shard State Loss
**Question:** If a shard loses SSM state mid-session, should the system:

**Options:**
a) **Fail-Closed:** Session immediately fails, user gets error (safe, poor UX)
b) **Fail-Open:** Automatically reset state and continue (graceful, may lose context)
c) **Fail-Replay:** Attempt to reconstruct state from transaction log (complex, eventual consistency)

**Decision Needed:** Recommend `__________` for Phase 1 PoC

### Gate 3: SSMStateStore Persistence Path
**Question:** Where should persistent SSM state eventually be stored?

**Options:**
a) **RocksDB (internal):** Alongside KV-cache in RocksDB (ThemisDB SoR preserved)
b) **Dedicated Vector Store:** Separate from transactional data (operational complexity)
c) **Distributed Journal:** Append-only log across shards (strong consistency, overhead)

**Decision Needed:** Recommend `__________` and capture ownership/migration path in Phase 1

### Gate 4: ISSMPlugin Registration Concurrency
**Question:** Can multiple SSM plugins be active simultaneously?

**Expected Answer:** No; `LLMPluginManager` enforces one active backend per session. Design assumes single plugin registration per inference context.

**Confirm:** ✅ YES / ❌ NO

---

## Integration Points

### With `LLMPluginManager`
```cpp
// Phase 1 usage (conceptual)
std::shared_ptr<ISSMPlugin> mamba_plugin = std::make_shared<SyntheticSSMStub>();
llm_manager->registerPlugin("ssm-mamba", mamba_plugin);

// Configuration controls activation
if (config.ssm_plugin_enabled) {
    plugin = llm_manager->getPlugin("ssm-mamba");
}
```

### With `ContextQualityBudget`
```cpp
// P1-D06: ContextQualityMetrics will use getStateRetentionScore()
struct ContextQualityMetrics {
    double state_retention_score = 0.0;  // from ISSMPlugin::getStateRetentionScore()
    double factual_drift_estimate = 0.0;  // computed from KnowledgeGapDetector
    uint64_t tokens_since_last_retrieval = 0;
};
```

### With `KnowledgeGapDetector`
```cpp
// P2-D05: Drift signal triggers RAG refresh
if (ssm_plugin && ssm_plugin->getStateRetentionScore() < drift_threshold) {
    trigger_agentic_rag_refresh();
}
```

---

## Feature Flag & Configuration

```cpp
// CMake flag
#define THEMIS_ENABLE_SSM_PLUGIN 0  // Default off for Phase 1 PoC

// Runtime config (JSON)
{
    "llm": {
        "ssm_plugin_enabled": false,
        "ssm_backend": "mamba-7b-gguf",
        "ssm_state_retention_threshold": 0.5
    }
}
```

---

## Stub Implementation Note (P1-D03)

Phase 1 will include `SyntheticSSMStub` (build flag `THEMIS_SSM_STUB_MODE=1`) for dataflow validation:

```cpp
// STUB/SIMULATION NOTE:
// Purpose: Validate SSM plugin dataflow without a real Mamba model
// Activation: Only when THEMIS_SSM_STUB_MODE=1 and ssm_plugin_enabled=true
// Production Delta: Uses fixed random state (Seed=42); no real token processing
// Removal Plan: Replace with real Mamba ISSMPlugin in Phase 2 when GGUF models available
```

---

## Success Criteria (P1-GATE-01 through P1-GATE-08)

1. ✅ SSM-Stub registerable via `LLMPluginManager`
2. ✅ `SSMStateSnapshot` round-trip lossless
3. ✅ HLC `snapshot_ts` in state snapshot
4. ✅ Infini-CPU matrix updates correct
5. ✅ Drift metric Prometheus export visible
6. ✅ Latency p99 ≤ +5% vs P0 baseline
7. ✅ Mamba governance contract approved
8. ✅ `ctest -L release_critical` green

---

## Next Steps

1. **Attendee Review:** Submit this document for human architect/PM approval
2. **Gate Decisions:** Capture answers to Gates 1–4 above
3. **Implementation:** Start P1-D01 header file creation once gates signed off
4. **Parallel:** P1-D02 (SSMStateStore interface), P1-D03 (stub), P1-D04 (Infini CPU)

