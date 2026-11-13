---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-13
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: include/process/*.h; src/process/*.cpp; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; src/process/ROADMAP.md; DOCUMENTATION_GOVERNANCE.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Future Enhancements

**AI provenance:** Exact model backend not verifiable from final document text alone.

This is a candidate-work index, not a commitment or delivery-status report. Earlier drafts presented fixed completion dates, test totals, benchmark pass states, and guarantees that are not established by the current source tree. No such status is claimed here.

## process

### Scope

Track implementation and verification work that is not proven complete by current source presence or historical reports.

### Design Constraints

- Do not document header-declared behavior as implemented unless the current implementation and tests support it.
- Keep orchestration profile state transitions and process-model lifecycle as separate contracts unless a transactional design is implemented.
- Preserve the internal-RAG profile invariant and avoid implying default embedding or telemetry backends.

### Required Interfaces

- `include/process/process_light_retriever.h` — retrieval modes and declared resource budgets.
- `include/process/process_orchestration_registry.h` — external profile lifecycle and prediction output.
- `include/process/process_audit_logger.h` and `include/process/process_telemetry_contract.h` — audit and telemetry contracts.
- Federation contracts under `include/process/` — verify against their matching implementations before relying on guarantees.

### Implementation Notes

1. **Retrieval budgets:** `ProcessLightRetriever::retrieve()` now caps returned context bytes, GLOBAL report count, LOCAL prompt lines, and LOCAL traversal depth and reports elapsed time/returned bytes. The synchronous timeout check is post-operation and cannot cancel dependencies. Run the new production-class budget tests in a configured build before closing this item.
2. **Orchestration state safety:** Decide whether activation/rollback need transactional semantics; current registry writes profile and active-domain state separately.
3. **Audit correctness and durability:** Replace or verify simplified in-memory persistence and placeholder delta encoding before relying on audit history, integrity, or temporal reconstruction.
4. **Telemetry interoperability:** Verify trace-context parsing/serialization and collector transport. Current export is simplified and logs the endpoint rather than demonstrating a real exporter.
5. **Federation maturity:** Reconcile contract-level quorum/failure guarantees with alpha/core implementation behavior; build tests and failure-injection evidence before making safety claims.
6. **Benchmark evidence:** Verify registered benchmark behavior, thresholds, run configuration, and raw result provenance before recording a baseline or pass status.
7. **Security/deployment integration:** Document authorization, data minimization, audit retention, authentication, and transport controls at the owning server/deployment layer.
8. **Hybrid coordination and framework persistence:** Preserve bounded native
   retrieval/tool/model-selection functions in the LLM/RAG path. Separately
   specify optional long-lived workflows/approval and distinct framework
   checkpoint/store/history adapters; do not require a new general graph
   runtime merely to integrate LangGraph. Any native persistent execution needs
   its own approved state/recovery contract. Target Q2 2027 is provisional,
   not a release commitment.

### Proposed persistence contracts

- **Checkpointer:** tenant/application/thread/namespace/checkpoint identity,
  parent links, pending task writes, exact/latest lookup, ordered pagination,
  versioned serialization, concurrency/conflict handling and scoped deletion.
  Checkpoint and write visibility must follow the explicitly selected
  transaction scope, not an assumed global distributed transaction.
- **Long-term store:** namespaced records, CRUD, prefix namespace search,
  pagination and optional semantic lookup; distinct from execution checkpoints.
- **History provider:** ordered session messages, append identity/idempotency,
  retention and load bounds. A thread/session ID is not authorization.
- **Optional native recovery/approval:** release DB locks before inference or
  approval waits, persist the intended transition, and reauthorize on resume.
  Revalidate stale source/model versions and separately approve external
  effects; no exactly-once guarantee follows from a checkpoint.
- Use the framework's pinned serialization contract; the DB can store typed
  opaque payloads without executing untrusted deserialization. Retention and
  deletion must cover pending writes and any required ancestor dependencies.

### Test Strategy

- Use `tests/process/test_process_light_retriever.cpp` for mode/fallback behavior, byte/result/depth caps, metrics, and best-effort timeout reporting; retain the configured test result.
- Use `tests/process/test_process_orchestration_registry.cpp` for profile transitions and invariants; add failure-injection coverage if atomic/recovery behavior changes.
- Confirm each test target and actual result from the configured build; registration is not a pass.
- Test persistent restart, exact/latest checkpoint lookup, pending-write
  round trips, parent pagination, competing writers, namespace/tenant isolation,
  full-session deletion, schema mismatch and expired/revoked approval.
- Verify any optional native resume separately from framework replay and
  test duplicate external-effect attempts using explicit idempotency fixtures.

### Performance Targets

The performance comparator currently has zero Process expectation rows. Four numeric PM-BM targets appear in source comments, but they are not mapped to comparator inputs or enforced by the benchmark implementations. The separate release coverage audit reports 14/14 Process mappings, which is not a performance result. A current performance baseline or pass status is not established; see [performance evidence](PERFORMANCE_EXPECTATIONS.md). Future work should define a threshold-to-case mapping and capture reproducible output before claiming a performance gate or baseline.

### Security / Reliability

Do not treat the current simplified audit persistence, telemetry export, federation implementations, or retrieval timeout as guarantees of durability, external delivery, federation safety, or hard return latency. Verify authorization, privacy, transport security, and recovery in the owning deployment components.

Do not treat process profiles or training checkpoints as durable agent
execution. Bounded native loops may remain inside the hybrid runtime; neither
native placement nor an external framework bypasses database policy, resource
budgets, or the need for an explicit storage/recovery contract.

Research basis (reviewed 2026-11-13):
[LangGraph checkpointers](https://docs.langchain.com/oss/python/langgraph/checkpointers)
and [stores](https://docs.langchain.com/oss/python/langgraph/stores) define
different persistence roles;
[Microsoft history storage](https://learn.microsoft.com/en-us/agent-framework/concepts/agents/conversations/storage)
separates history ownership and session state. Pin actual package versions
before implementing an adapter; these sources do not prove ThemisDB support.

## Source inventory note

Federation managers, conflict resolver, audit logger, diagnostics and telemetry sources are present in the current tree. Some corresponding public contracts and implementation files describe design targets or simplified/alpha behavior; presence is not proof of completion or production readiness. See [architecture](ARCHITECTURE.md), [implementation notes](IMPLEMENTATION.md), and [source audit notes](AUDIT.md).

## Validation references

- [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt) — focused test registration.
- [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt) — conditional benchmark registration.
- [Module roadmap](ROADMAP.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md)
