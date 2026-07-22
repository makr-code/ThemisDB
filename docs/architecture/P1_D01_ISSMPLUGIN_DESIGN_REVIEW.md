# P1-D01: ISSMPlugin Interface — Design Review and Sign-Off

**Status:** Awaiting Human Architect Approval  
**Date:** 2026-07-22  
**Target Gate:** P1-GATE-01 (SSM-Stub via LLMPluginManager registerable)  
**Reference Files:**
- Interface: `include/llm/i_ssm_plugin.h`
- Base: `include/llm/llm_plugin_interface.h`
- Stub (Phase 1): `src/llm/ssm_stub_plugin.cpp` (to be implemented)
- State Store (Phase 1): `include/llm/ssm_state_store.h` (to be designed)

---

## 1. Design Summary

The `ISSMPlugin` interface extends `ILLMPlugin` to add stateful SSM-specific operations:

```cpp
struct ISSMPlugin : public ILLMPlugin {
    virtual bool updateState(const std::vector<int32_t>& tokens) = 0;
    virtual SSMStateSnapshot getStateSnapshot(core::HLCTimestamp snapshot_ts) = 0;
    virtual bool restoreState(const SSMStateSnapshot& snapshot) = 0;
    virtual void resetState() = 0;
    virtual double getStateRetentionScore() const = 0;
    virtual std::string getStateFingerprint() const = 0;
};
```

**Key Design Decisions Already Made (locked):**
- ✅ Extends `ILLMPlugin` (no changes to base class needed)
- ✅ HLC-binding in `SSMStateSnapshot` for MVCC consistency
- ✅ Fingerprint validation for architecture mismatch detection
- ✅ Thread-safe contract (mandatory per spec)
- ✅ Fallback semantics: updateState() error → KnowledgeGapDetector triggers RAG

---

## 2. Open Design Questions (Requiring Human Architect Sign-Off)

### 2.1 SSMStateStore Distribution Strategy

**Question:** How should SSMStateSnapshot be stored and replicated across ThemisDB's 5-shard cluster?

**Options:**

#### Option A: Replication (Consensus-Driven)
- Every shard holds a replica of the SSM state
- Consensus write via Raft + HLC serialization point
- Pros: Single-reader/multi-writer safe, consistent across shards
- Cons: Write amplification (5× writes per state update), GC complexity
- Recommendation: **Best for correctness, worst for scale**

#### Option B: Shard-Partitioned (Sticky Session)
- Each shard owns a subset of SSM session states (hash-based partitioning)
- Session affinity: all tokens from a session route to same shard
- Cross-shard hand-off: explicit snapshot transfer on session migration
- Pros: No write amplification, session-local consistency
- Cons: Requires session routing logic, cross-shard RPC for migration
- Recommendation: **Best for scale, needs routing governance**

#### Option C: Hybrid (Lazy Replica + Checksum Cache)
- Primary shard holds SSM state, others cache checksum only
- State sync on read miss, lazy persistence on ttl
- Pros: Balance between replication and scale
- Cons: Complex eventual consistency semantics, possible brief divergence
- Recommendation: **For future optimization, too complex for Phase 1**

**Approval Needed:**
- [ ] Select Option A, B, or C
- [ ] Confirm session routing tier can enforce sticky-session if Option B
- [ ] Document failure-mode semantics (see §2.2 below)

---

### 2.2 Failure Semantics on Cross-Shard State Loss

**Question:** What should happen if an SSM state is lost on one shard during a cross-shard hand-off?

**Scenario:**
- Session S is on Shard 1, state snapshot taken at HLC timestamp T1
- Shard 1 fails or network partitions
- Session S arrives at Shard 2 with stale snapshot from T0 < T1
- Data loss: tokens processed between T0 and T1 are lost

**Options:**

#### Fail-Closed (Conservative)
- Reject session recovery; force re-initialization with resetState()
- User sees: "Session recovered, but context reset. Re-fetch context from retrieval system."
- Impact: Acceptable UX hit for consistency guarantee
- Recommendation: **Choose this for Phase 1 POC**

#### Fail-Open (Optimistic)
- Accept stale snapshot, continue from T0
- Mark session with degraded_context_flag; trigger preemptive RAG refresh
- User sees: Seamless recovery, but KnowledgeGapDetector forces verification
- Impact: Risk of stale context serving; mitigated by downstream RAG
- Recommendation: **Defer to Phase 3 (requires confidence in RAG verification)**

#### Hybrid (TTL-Gated)
- Fail-Closed if snapshot age > ttl_threshold (e.g., 1 hour)
- Fail-Open if snapshot age ≤ ttl_threshold
- Pros: Correctness guarantee for fresh snapshots, graceful degrade for stale
- Cons: More complex state machine
- Recommendation: **Phase 2+ after Phase 1 POC validates correctness**

**Approval Needed:**
- [ ] Select Fail-Closed, Fail-Open, or Hybrid
- [ ] If Fail-Open or Hybrid: confirm KnowledgeGapDetector confidence thresholds
- [ ] Document SLA impact (max acceptable context reset frequency)

---

### 2.3 SSMStateStore Backend and Migration Path

**Question:** Where should SSMStateSnapshot be persisted long-term, and how does it migrate from Phase 1 to Phase 2+?

**Phase 1 (POC):**
- In-Memory `SSMStateStore` (ephemeral, session-local)
- Loss on shard restart is acceptable for POC
- HLC timestamp still tracked for consistency checking

**Phase 2 (Beta):**
- RocksDB-backed `SSMStateStore` (durable, shard-local or replicated)
- Persists to local RocksDB partition (analogous to `KVPrefixTransferManager`)
- Optional cross-shard replication via raft-wal

**Phase 3+ (GA):**
- Optional: Distributed store (e.g., separate SSM snapshot service)
- Optional: Archive-tier (cold storage for historical session snapshots)

**Ownership:**
- Phase 1 Owner: Copilot (proof-of-concept)
- Phase 2+ Owner: **To be assigned** (human architect discretion)
- Migration SLA: Zero downtime via feature flag toggle (default off)

**Approval Needed:**
- [ ] Confirm Phase 1 in-memory design is acceptable
- [ ] Assign Phase 2+ owner (likely system architecture team)
- [ ] Confirm no breaking changes in Phase 1 that would block Phase 2 upgrade

---

## 3. Acceptance Criteria for P1-D01 Design Review

| Criterion | Status | Approval |
|-----------|--------|----------|
| Distribution strategy decided (A/B/C) | ⏳ Pending | [ ] Architect |
| Failure semantics decided (Closed/Open/Hybrid) | ⏳ Pending | [ ] Architect |
| Backend migration path documented | ⏳ Pending | [ ] Architect |
| Session routing tier can support sticky sessions (if Option B) | ⏳ Pending | [ ] Routing Lead |
| RAG confidence thresholds confirmed (if Fail-Open) | ⏳ Pending | [ ] RAG Lead |
| Phase 2+ owner assigned | ⏳ Pending | [ ] Architecture Lead |

---

## 4. Proposed Implementation Plan (Post-Approval)

Once decisions are locked, Phase 1 will deliver:

1. **P1-D02:** `SSMStateStore` interface + in-memory skeleton
   - `checkpoint()`, `resume()`, `invalidate()`, `compact()` methods
   - Replicas/sharding logic per approved strategy

2. **P1-D03:** `ISSMPlugin` stub implementation
   - Fixed random state (Seed=42)
   - Proper state snapshots with HLC binding
   - Error handling per approved failure semantics

3. **P1-D07:** Integration tests
   - Plugin registration flow
   - State snapshot roundtrip (store → retrieve → restore)
   - Cross-shard hand-off (if Option B)
   - Failure recovery (per approved semantics)

---

## 5. Rollback Plan

If issues arise post-Phase 1, the rollback is feature-flag only:

```cpp
// In LLMPluginManager::infer()
if (config.enable_ssm_plugin) {
    // Use SSMPlugin path
} else {
    // Fallback to Transformer (default, no code revert needed)
}
```

Default: `enable_ssm_plugin = false` (safe fallback).

---

## 6. Security & Governance Notes

- **ThemisDB Authority:** ThemisDB remains the system-of-record for SSM state. RocksDB/store backend is an internal implementation detail.
- **Tenant Isolation:** SSM state must be isolated per session/tenant. Cross-tenant contamination is a blocker.
- **Audit Trail:** State changes (init/checkpoint/restore/reset) must emit audit events for compliance.
- **Access Control:** Only the owning session/request can read/restore its SSM state.

---

## 7. Sign-Off Template

**Approved By:** [Human Architect Name]  
**Date:** ________________  
**Decisions Made:**

- [ ] Distribution Strategy: **Option ___ (A/B/C)**
  - Rationale: _____________________
  
- [ ] Failure Semantics: **Option ___ (Closed/Open/Hybrid)**
  - Rationale: _____________________
  
- [ ] Backend Migration Path: **Confirmed**
  - Phase 2+ Owner: _____________________
  - Constraints/Notes: _____________________

- [ ] Additional Constraints/Notes:
  _____________________________________

**Approved for Phase 1 Implementation:** ✅ YES / ❌ NO

---

## 8. Related Documents

- `docs/architecture/ssm-hybrid-analysis.md` — Technical deep-dive
- `docs/architecture/ssm-hybrid-rollout-plan.md` — Phased timeline
- `include/llm/i_ssm_plugin.h` — Interface header
- `include/llm/llm_plugin_interface.h` — Base class contract
- `ai_working/P0_P1_P2_IMPLEMENTATION_STATUS.md` — Current status

