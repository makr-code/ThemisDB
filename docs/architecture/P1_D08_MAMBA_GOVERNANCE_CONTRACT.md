/**
 * @file P1_D08_MAMBA_GOVERNANCE_CONTRACT.md
 * @brief Mamba SSM State Lifecycle: Security and Governance Boundaries (Phase 1)
 * @version 0.1.0-alpha
 * @note Maturity: GOVERNANCE (Non-Production)
 * @note Status: Phase 1 Contract Definition
 * @note Issue: Part of Phase 1 (P1-D08) deliverable; binds P1-GATE-07 acceptance criteria
 */

# Mamba Governance Contract — Phase 1

**Version:** 0.1.0-alpha  
**Status:** Phase 1 PoC Contract  
**Scope:** SSM hidden-state lifecycle in ThemisDB  
**Approved:** [PENDING HUMAN SIGN-OFF]  
**Binding Gate:** P1-GATE-07 (Mamba Governance Contract approved)  

---

## 1. Core Principle: System-of-Record Governance

### 1.1 ThemisDB as Authoritative Data Owner

**Binding Rule (mandatory):**
> **ThemisDB LLMQueryContext + LLMPluginManager remain the authoritative System-of-Record (SoR) for all SSM state.**

This means:

- ✅ **SSM state belongs to ThemisDB**, not to any backend storage system
- ✅ **LLMPluginManager controls lifecycle** (init → checkpoint → recovery → invalidation)
- ✅ **All state transitions logged/audited** through ThemisDB's audit trail
- ✅ **RocksDB is an internal cache layer only**, not a sovereign datastore
- ❌ **NOT allowed:** Direct RocksDB modifications bypassing LLMPluginManager
- ❌ **NOT allowed:** External agents reading/writing SSM state without audit context

### 1.2 RocksDB as Internal Persistence Layer (Phase 2+)

**Binding Rule (Phase 2 forward):**

When RocksDB-backed SSMStateStore is introduced (Phase 2, P2-D04):

- ✅ RocksDB stores **serialized SSM state snapshots only** (opaque binary data)
- ✅ RocksDB keys are **HLC-timestamped** for MVCC isolation (no free-form keys)
- ✅ RocksDB is **NOT directly queryable** (no SQL-like access to SSM state)
- ✅ RocksDB access is **read-only to external callers** (only LLMPluginManager writes)
- ✅ RocksDB is **session-scoped** (each session has isolated state snapshots)
- ❌ **NOT allowed:** Using RocksDB as a shared state bus between sessions
- ❌ **NOT allowed:** Exposing RocksDB keys/structure to user-facing APIs

**Rationale:**
- Prevents privilege escalation via direct storage access
- Maintains session isolation (multi-tenant safety)
- Enables future backend swaps (RocksDB → distributed store → cold storage)

---

## 2. Security Boundaries

### 2.1 Tenant Isolation (Multi-Tenant Contract)

**Binding Rule (mandatory for L5 deployment):**

Each SSM state checkpoint MUST be tagged with:

1. **Session ID** — ties state to specific user/agent session (primary isolator)
2. **Tenant ID** — organizes sessions by tenant (enterprise deployments)
3. **HLC Timestamp** — establishes causality and prevents timestamp-based confusion attacks

**Enforcement:**

```cpp
// In ISSMStateStore::checkpoint()
// Before storing: verify session_id ∈ tenant_sessions[tenant_id]
// Before retrieving: check caller's tenant_id matches snapshot's tenant_id
```

**Failure Modes (strict validation):**
- ❌ Attempt to resume state with wrong tenant_id → **RuntimeException**
- ❌ Attempt to resume state with wrong session_id → **RuntimeException**
- ❌ Attempt to compact/delete state without correct auth → **AccessDenied**

### 2.2 Audit Trail Integration

**Binding Rule (mandatory):**

Every SSM state lifecycle event MUST log:

```
Timestamp | Operation    | Session ID | Tenant ID | HLC_TS   | Outcome  | Actor
-----------------------------------------------------------------------------------
T1        | CHECKPOINT   | sess-123   | tenant-A  | hlc-500  | SUCCESS  | llm-plugin-mgr
T2        | RESUME       | sess-123   | tenant-A  | hlc-500  | SUCCESS  | query-context
T3        | INVALIDATE   | sess-123   | tenant-A  | -        | SUCCESS  | session-cleanup
T4        | COMPACT      | -          | tenant-A  | -        | 5-deleted| maintenance
```

**Integration Point:**
- Audit logger: `ThemisDB::AuditLog` (or equivalent)
- Event category: `ssm_state_lifecycle`
- Severity: Varies (normal ops = INFO, errors = WARN/ERROR)

### 2.3 Cross-Shard State Loss Semantics (Fail-Closed Contract)

**Binding Rule (Phase 1 default; can be overridden by deployment policy):**

When SSM state loss is detected (e.g., cross-shard transfer failure, RocksDB corruption):

- **Fail-Closed (Default, Phase 1):** Reject inference request; return error to client
  - Rationale: Prevents silent hallucinations due to state corruption
  - Fallback: Client can retry with fresh session or RAG-only mode
  
- **Fail-Open (Optional, Phase 2+, requires explicit approval):** Continue with degraded state
  - Requires feature flag: `THEMIS_SSM_FAILOPEN_MODE=1`
  - Requires explicit logging: "SSM state loss detected; continuing with failover"
  - Rationale: For high-availability deployments willing to trade coherence for availability

**Acceptance Criteria (P1-GATE-07):**
- [ ] Fail-Closed semantics implemented and tested
- [ ] Cross-shard loss scenarios covered in P1-D07 integration tests
- [ ] Audit event logged on all state-loss detection

---

## 3. Governance Constraints by Phase

### 3.1 Phase 1 (PoC) Constraints

| Constraint | Binding Level | Rationale |
|-----------|---------------|-----------|
| In-Memory Only | **MANDATORY** | No persistence risk; state lost on restart (acceptable for PoC) |
| Single Node | **MANDATORY** | No replication; no cross-shard concerns yet |
| Synchronous Checkpoint | **MANDATORY** | No queueing; immediate durability semantics |
| No Encryption | **ALLOWED** | State is not sensitive; Seed=42 synthetic data anyway |
| Session-Scoped | **MANDATORY** | No long-term state sharing across sessions |

**Security Posture (Phase 1):**
- ✅ Suitable for **development/testing** environments
- ✅ Suitable for **internal demo environments** (single user)
- ❌ **NOT suitable** for production multi-tenant deployments
- ❌ **NOT suitable** for HIPAA/SOC-2/PCI environments

### 3.2 Phase 2 (Beta) Constraints (Proposed)

| Constraint | Change | Rationale |
|-----------|--------|-----------|
| RocksDB Persistence | **ADDED** | Enable session recovery across restarts |
| HLC-Keyed Snapshots | **ADDED** | MVCC isolation for concurrent sessions |
| Encryption-at-Rest | **ADDED** | State contains user interaction history |
| Tenant-Scoped Compaction | **ADDED** | Multi-tenant data cleanup compliance |
| Cross-Shard Replication | **PROPOSED** | High-availability deployments (design TBD) |

**Security Posture (Phase 2 target):**
- ✅ Suitable for **production single-tenant deployments**
- ✅ Suitable for **regulated environments** (with audit trail proof)
- ⚠️ Multi-tenant deployments require additional isolation review

---

## 4. Integration Requirements (Binding Criteria)

### 4.1 LLMPluginManager Integration

**Binding Requirement:**

LLMPluginManager MUST provide:

```cpp
// In include/llm/llm_plugin_manager.h

class LLMPluginManager {
  public:
    // Phase 1: Initialize SSM state store (in-memory)
    bool initializeSSMStateStore(const SSMStateStoreConfig& config);
    
    // Phase 1: Checkpoint SSM state for session
    bool checkpointSSMState(const std::string& session_id,
                           const ISSMPlugin* plugin);
    
    // Phase 1: Resume SSM state for session
    bool resumeSSMState(const std::string& session_id,
                       ISSMPlugin* plugin);
    
    // Phase 1: Invalidate SSM state on session cleanup
    bool invalidateSSMState(const std::string& session_id);
    
    // Phase 2+: Compact old snapshots
    uint64_t compactSSMStateStore(uint64_t retention_window_ms);
    
    // Phase 1+: Get state store statistics (for monitoring)
    std::string getSSMStateStoreStats() const;
};
```

**Acceptance Gate (P1-GATE-01):**
- [ ] All 5 methods implemented in LLMPluginManager
- [ ] All methods properly delegate to ISSMStateStore
- [ ] Unit test: `tests/llm/test_ssm_plugin_interface.cpp` verifies integration

### 4.2 Audit Log Integration

**Binding Requirement:**

Audit logger MUST track:

```json
{
  "timestamp": "2026-07-22T17:00:00Z",
  "event_type": "ssm_state_lifecycle",
  "operation": "checkpoint",
  "session_id": "session-abc123",
  "tenant_id": "tenant-xyz789",
  "hlc_timestamp": {"wall_clock_ms": 123456, "logical_clock": 42},
  "outcome": "success",
  "actor": "llm_plugin_manager",
  "details": {
    "state_size_bytes": 2048,
    "snapshot_count": 1
  }
}
```

**Acceptance Gate (P1-GATE-07):**
- [ ] Audit events logged for checkpoint/resume/invalidate operations
- [ ] Audit log entries include session_id, tenant_id, HLC timestamp
- [ ] Integration test verifies audit trail completeness

---

## 5. Compliance and Certification Path

### 5.1 Phase 1 Certification (Development Only)

**Cannot certify for:**
- ❌ Production deployments
- ❌ Multi-tenant deployments
- ❌ Regulated industries (HIPAA/SOC2/PCI)

**Can certify for:**
- ✅ Development/testing environments
- ✅ Single-user demo environments
- ✅ Internal research (non-sensitive data)

### 5.2 Phase 2 Certification Path (Proposed)

**Prerequisites for Production Certification:**
1. ✅ P1-GATE-07 passed (governance contract approved)
2. ✅ P2-D04 passed (RocksDB backend with HLC keys)
3. ✅ Encryption-at-Rest implemented (`libsodium` or similar)
4. ✅ Tenant isolation tests (multi-tenant scenarios in test suite)
5. ✅ Audit trail compliance verified (SOC-2 evidence)
6. ✅ Cross-shard replication strategy documented and tested
7. ✅ Penetration testing completed (state exfiltration scenarios)

**Certification Level (Phase 2):**
- Suitable for **production single-tenant deployments**
- Suitable for **HIPAA-lite** environments (with additional review)
- Not yet suitable for **mission-critical financial** or **national security** deployments

---

## 6. Sign-Off Checklist (P1-GATE-07)

**This contract is approved when ALL of the following are signed off:**

- [ ] **Architecture Lead:** Governance model aligns with ThemisDB architecture
- [ ] **Security Lead:** Fail-closed semantics, tenant isolation, audit trail requirements approved
- [ ] **Operations Lead:** Audit logging integration, compaction retention window, monitoring strategy approved
- [ ] **Compliance Lead:** Phase 1 PoC certification level ("dev/test only") documented
- [ ] **Test Lead:** P1-D07 integration tests cover all binding requirements

**Approvals:**
```
Architecture: ________________________  Date: ___________
Security:    ________________________  Date: ___________
Operations:  ________________________  Date: ___________
Compliance:  ________________________  Date: ___________
Testing:     ________________________  Date: ___________
```

---

## 7. Related Documents

- **Phase 1 Architecture:** `ssm-hybrid-rollout-plan.md` §3 (Phase 1 deliverables)
- **Design Review:** `P1_D01_ISSMPLUGIN_DESIGN_REVIEW.md` (interface shape decisions)
- **Test Suite:** `tests/llm/test_ssm_plugin_interface.cpp` (P1-D07)
- **State Store:** `include/llm/ssm_state_store.h` (P1-D02 interface)
- **LLMPluginManager:** `include/llm/llm_plugin_manager.h` (integration point)

---

## 8. Revision History

| Version | Date | Author | Summary |
|---------|------|--------|---------|
| 0.1.0-alpha | 2026-07-22 | Copilot | Initial Phase 1 contract definition |

---

**Document Status:** PENDING HUMAN SIGN-OFF  
**Blocking Gate:** P1-GATE-07 (Mamba Governance Contract approved)  
**Next Phase Dependency:** P2-D04 (RocksDB backend) requires this contract to be approved
