/**
 * @file mamba_security_governance.md
 * @brief SSM/Mamba security and governance boundaries (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: POLICY
 */

# SSM/Mamba Security & Governance Contract

**Phase:** 1 PoC (Q3 2026)  
**Status:** Pending human architecture/security review  
**Owner:** LLM Module Maintainers

---

## 1. System-of-Record Principle

**Binding Constraint:** ThemisDB remains the authoritative source for all model state, configuration, and session data.

- **Mamba/SSM state storage:** RocksDB internal persistence only (see §2)
- **No external state database:** Mamba state is NOT synced to external backends (e.g., Elasticsearch, Redis, distributed journal)
- **No sidecar state server:** State is NOT delegated to a separate service
- **Fallback contract:** If SSM plugin fails, transformer fallback via KnowledgeGapDetector + AgenticRAG is activated

---

## 2. State Persistence Boundaries

### Allowed
- **RocksDB internal store:** SSM state checkpoints with HLC timestamps (in-memory Phase 1; Phase 2 RocksDB)
- **Session-scoped storage:** State isolated by `session_id` (no cross-session leakage)
- **Serialized vectors:** State data serialized as opaque byte vectors (`std::vector<u8>`)

### Forbidden
- **External model server state:** No state pulled from llama.cpp or external inference servers
- **Distributed consensus:** No RAFT/Paxos for state replication in Phase 1 (Phase 3+: explicit approval required)
- **Third-party state stores:** No Cassandra, PostgreSQL, or cloud object storage for state persistence

---

## 3. Tenant & Data Isolation

### Multi-Tenant Boundary
- Each session (`session_id`) carries its own SSM state
- State snapshots are NOT shared across sessions
- No cross-tenant state leakage in checkpoint/resume operations

### Access Control
- SSM state accessible only via `ISSMPlugin` interface (no direct RocksDB access)
- Session authentication enforced at LLMAQLHandler layer (existing contract)
- Audit events logged for state checkpoint/restore (future: audit trail in P2+)

---

## 4. Cryptographic Commitment

### Fingerprint Validation
- Each `SSMStateSnapshot` includes `state_fingerprint` (model hash + version)
- Fingerprint mismatch on restore triggers error + fallback (no silent corruption)
- Fingerprint derived from model architecture (hidden_dim, num_layers) — immutable

### HLC Timestamp Binding
- All state snapshots carry HLC timestamp for MVCC consistency
- HLC clock managed by existing `HLCManager` (ThemisDB coordination layer)
- Cross-shard state consistency enforced via HLC ordering

---

## 5. Plugin Failure & Fallback Semantics

### Error Cases
| Condition | Action | Fallback |
|-----------|--------|----------|
| updateState() fails | Log error + increment counter | Transformer path + RAG refresh |
| restoreState() fingerprint mismatch | Log error + reject restore | Transformer path + empty context |
| getStateSnapshot() returns empty | Log error + clear session | Transformer path + new session |
| checkpointStore full | Log warning + delete oldest | Use in-memory (Phase 1 only) |

### Activation: Automatic via KnowledgeGapDetector
```cpp
if (!ssm_plugin->updateState(tokens)) {
    metrics::recordSSMPluginFailure();
    // Trigger RAG refresh + Transformer fallback
    use_transformer_path = true;
    agentic_rag.refresh(session_id);
}
```

---

## 6. Security Requirements (Human Review Gate)

### Required for Phase 1 Sign-Off
- [ ] No credential leakage in state serialization (code review)
- [ ] Fingerprint validation prevents model type confusion (unit tests)
- [ ] Cross-session isolation verified (state snapshot tests)
- [ ] Fallback activation tested under fault injection (W6 style tests)
- [ ] HLC timestamp binding validated (mvcc_test.cpp)

### Future (Phase 2+): Deferred
- [ ] State encryption-at-rest (RocksDB layer)
- [ ] Audit trail for compliance (separate PR)
- [ ] Multi-tenant resource quotas (separate PR)

---

## 7. Integration Checklist

- [ ] ISSMPlugin registers via LLMPluginManager (existing singleton)
- [ ] KnowledgeGapDetector consumes state_retention_score
- [ ] ContextQualityMetrics integrated into ContextWindowBudget
- [ ] Prometheus metrics exported (ssm_drift_metrics.cpp)
- [ ] Phase 1 unit tests: test_ssm_plugin_interface.cpp (P1-D07)
- [ ] Human security review approval (required before merge)

---

## 8. Review & Sign-Off

**Proposed Approvals Required:**

1. **Architecture:** SSMStateStore distribution model + failure semantics (P1-D01 design gate)
2. **Security:** No credential leakage, isolation contract (new)
3. **LLM Module Owner:** Plugin registration + fallback semantics (existing)
4. **Operations:** Monitoring/alerting for plugin failures (new)

**Once Approved:** Mark decision gates complete, proceed with Phase 1 implementation.

---

**Last Updated:** 2026-07-22  
**Next Review:** After P1-D07 unit tests complete (before Phase 1 acceptance)

