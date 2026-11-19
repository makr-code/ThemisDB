---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: evidence reconciliation | reviewed: 2026-11-19; no local build or tests run -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The source set provides in-process capability callbacks, federated aggregation,
merge of already-fetched shard responses, feedback synchronization callbacks,
and distillation callbacks. It does not implement a graph store or graph-sync
protocol, peer transport/authentication, durable state, retry loops, or
restart recovery. `GossipAdapterPublisher` rejects unknown domain strings and
requires an explicitly configured inbound trust policy before dispatch.
`federated_distillation_coordinator.cpp` is included in the production library;
production linkage still requires a successful build/test run.

Evidence reconciliation (2026-11-19):
- The module has five implementation surfaces in the library target; no live graph synchronization implementation is present.
- Current test-source inventory: 110 GoogleTest declarations across 9 translation units; this is static source evidence, not proof that tests were built or run.
- Test targets follow module test pattern: `module_distributed_knowledge_test_<base>_focused`
  (filename `test_<base>_focused.cpp` → stem stripped of `_focused` → target appends `_focused`)
- Component-focused suites exercise local APIs; contract-hardening and high-cardinality stress sources include mocks/stubs. None establish real shard-topology behavior.
- No benchmark run artifact or representative-hardware p95/p99 baseline is available in this audit.

## In Progress

- [~] hardening of timeout, partial-failure, and policy-edge semantics across federation paths (Target: Q3 2026)
  - Test coverage added: ACA-01..ACA-10, LFC-01..LFC-12, FRM-01..FRM-12, CSS-01..CSS-12, FDC-01..FDC-12
  - Implementation surfaces: adapter_capability_announcement, lora_federation_coordinator, federated_rag_merger, cross_shard_feedback_sync, federated_distillation_coordinator
- [~] benchmark stabilization for aggregation, merge, and feedback sync hot paths (Target: Q3 2026)
  - Focused test infrastructure in place for regression tracking
- [~] diagnostics consistency improvements for federation incidents and degraded shard responses (Target: Q3 2026)
  - Test coverage includes failure scenarios, partial failures, timeouts

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic merge behavior under multi-shard timeout permutations (Target: Q4 2026)
- [ ] expand regression coverage for replay/dedup and policy-gated sync scenarios (Target: Q4 2026)
- [ ] refine operator-facing visibility for federation rounds and rollback events (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for aggregation and merge under sustained load (Target: Q1 2027)
- [ ] extend benchmark depth for distillation and risk-gated federation workflows (Target: Q1 2027)
- [ ] harden long-running stability across mixed shard capability/topology states (Target: Q1 2027)

## Implementation Phases

**Note:** Implementation phases (Phase 1-6) are orthogonal to test scenario layers (Layer A-D):
- **Phases 1-6** describe module development phases: design, implementation, error handling, testing, performance, documentation.
- **Layers A-D** (in test files) describe federation workflow scenarios: domain routing (A), LoRA (B), RAG (C), RLAIF feedback (D).

### Phase 1: Design / API Contract
- [x] freeze capability/federation/merge/sync contracts for active major line (Delivered: Q3 2026)
- [x] define explicit distributed failure taxonomy for timeout/policy/privacy classes (Delivered: Q3 2026)
  - Contract header: `include/distributed_knowledge/distributed_knowledge_api_contract.h`
  - Error codes: ENTITY_NOT_FOUND, FEDERATION_TIMEOUT, CONFLICT_UNRESOLVABLE, GRAPH_CORRUPTED,
    RETRIEVAL_LIMIT_EXCEEDED, TOMBSTONE_PROPAGATION_FAILED, CRDT_MERGE_TYPE_MISMATCH, INTERNAL_ERROR
  - LWW resolveLww() helper with tie-break semantics

### Phase 2: Core Implementation
- [x] complete explicit documentation of merge conflict resolution strategies (Delivered: Q3 2026)
  - Merge strategies documented: RRF, score-weighted, round-robin
  - Conflict resolution semantics: duplicate doc_id handling, tie-breaking
  - Timeout/failure handling: DK-OR-T contract (all shards timeout, partial failure)
  - Deduplication contract: preserving order, handling duplicates
  - Configuration validation: required fields and constraints
  - API contract: § 8 "Merge Strategy and Conflict Resolution Contract"
  - Reference: `include/distributed_knowledge/federated_rag_merger.h` class/method docs
  - Reference: `src/distributed_knowledge/federated_rag_merger.cpp` implementation comments
  - Reference: `include/distributed_knowledge/distributed_knowledge_api_contract.h` § 8
- [x] Batch 3: Consistency and Version Tracking Documentation (Delivered: Q3 2026)
  - Consistency levels documented (strong, causal, eventual) for all operations
  - Version ordering documented for round-based aggregation (gradients, distillation)
  - Replication lag assumptions documented (federation, RAG, feedback)
  - Stale-read intentionality documented with correctness justification
  - API contract § 9 added: "Consistency and Version Semantics"
  - Reference: `include/distributed_knowledge/distributed_knowledge_api_contract.h` § 9
  - Reference: Updated method docs in lora_federation_coordinator.h/cpp
  - Reference: Updated method docs in federated_distillation_coordinator.h/cpp
  - Reference: Updated method docs in federated_rag_merger.h/cpp
  - Reference: Updated method docs in cross_shard_feedback_sync.h/cpp
- [x] complete hardening for aggregation, merge, and sync coordinator internals (Target: Q4 2026) — `MergeHardeningPolicy` + `DistillationBoundedPolicy` added to API contract; trust-gate ACA-TRUST-01..05 tests added 2026-08-24
- [x] align distillation and feedback behavior to bounded runtime contracts (Target: Q4 2026) — `DistillationBoundedPolicy` with FAIL_CLOSED enforcement wired into `FederatedDistillationCoordinator` via `setBoundedPolicy()` 2026-08-24; enforced in `broadcastToStudents()` before any broadcast
- [x] include `federated_distillation_coordinator.cpp` in `themis_distributed_knowledge` (Target: v2.5.0) — production library source list and focused-test fallback now agree

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for unsafe federation and trust-gate violations (Target: Q4 2026) — `IFederationTrustPolicy` + `AlwaysPermitTrustPolicy` + `DKErrorCode::TRUST_GATE_REJECTED` added 2026-08-24
- [x] enforce configured trust policy before inbound capability dispatch and reject unsupported domain values (Target: v2.5.0) — missing/rejecting policy throws before callback; unknown domains no longer map silently to `GENERAL`
- [x] unify diagnostics for timeout, dedup, and partial-shard merge failures (Target: Q4 2026) — `DistributedKnowledgeDiagnosticEmitter` + `DKDiagnosticEvent` model added; `test_dk_diagnostics_focused.cpp` (DKD-01..14) added 2026-08-24

### Phase 4: Tests
- [x] expand focused regressions for capability/routing and federation edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic fixture coverage for shard-response permutation matrixes (Delivered: Q3 2026)
  - Test file: `tests/distributed_knowledge/test_dk_contract_hardening_focused.cpp`
  - Test cases: DKC-01..DKC-16 (entity lifecycle, federation, retrieval, conflict resolution)
  - kDKContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [~] benchmark sources and declared thresholds exist for component operations and DKRG-01..DKRG-06; this documentation audit found no run output or baseline comparison that verifies achieved p95/p99 or throughput.
  - Component benchmarks: `benchmarks/distributed_knowledge/bench_distributed_knowledge.cpp` and `benchmarks/distributed_knowledge/bench_distributed_knowledge_or.cpp`.
  - `bench_dk_release_gates.cpp` uses in-memory/mock entity, graph, federation-union, and serialization helpers; its thresholds do not by themselves establish production-path performance.
  - `kDKCanonicalSeed = 42`; the release-gate source declares 5 repetitions. Benchmark execution and release-baseline evidence remain to be confirmed.

### Phase 6: Documentation and Acceptance
- [x] core distributed_knowledge module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] focused test infrastructure created (current source inventory: 110 test declarations across 9 translation units; execution status not established by this source audit)
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (DKC-01..16), 6 release-gate benchmarks (DKRG-01..06)

## Production Readiness Checklist

- [x] core distributed_knowledge surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] focused test suite created for all module surfaces (ACA, LFC, FRM, CSS, FDC test families)
- [x] inbound capability dispatch fails closed for missing/rejecting trust policy and unsupported domain values
- [ ] execute the complete focused suite against the production library and preserve reproducible test artifacts
- [ ] validate real three-shard partial/all-timeout behavior and cancellation-aware timeout semantics
- [ ] establish representative-hardware p95/p99 baselines with no missing mapped benchmark cases

## Test Evidence (Q3 2026 Deliverable)

**Focused Test Sources:** Created starting 2026-07-28; current source inventory checked 2026-11-19

Test source location: `tests/distributed_knowledge/test_*.cpp`

Test coverage by module surface:

| Surface | Test File | Tests | Coverage |
|---------|-----------|-------|----------|
| Adapter Capability Announcement | test_adapter_capability_announcement_focused.cpp | 19 declarations (ACA-01..ACA-10, inbound dispatch, 5 trust-policy cases, and 3 fail-closed cases) | Gossip publish, domain types, inbound dispatch, trust-policy behavior |
| LoRA Federation Coordinator | test_lora_federation_coordinator_focused.cpp | LFC-01..LFC-12 (12) | Aggregation requests, state machine, timeout handling |
| Federated RAG Merger | test_federated_rag_merger_focused.cpp | FRM-01..FRM-12 (12) | Merge requests, dedup, partial failure, top-K truncation |
| Cross-Shard Feedback Sync | test_cross_shard_feedback_sync_focused.cpp | CSS-01..CSS-12 (12) | Feedback events, batching, dedup, replay detection, privacy filtering |
| Federated Distillation Coordinator | test_federated_distillation_coordinator_focused.cpp | 15 declarations (FDC-01..FDC-12 plus 3 bounded-policy cases) | Distillation requests, DP parameters, policy gates, privacy budgets |
| Contract hardening | test_dk_contract_hardening_focused.cpp | 16 | Contract checks; several cases use local mock/simulated behavior rather than module runtime calls |
| Diagnostics and hardening policy | test_dk_diagnostics_focused.cpp | 14 | Diagnostic emitter dispatch and `MergeHardeningPolicy` semantics |
| LLM/RAG context | test_distributed_knowledge_llm_focused.cpp | 7 | RAG merger configuration, merge, context formatting, and timeout path |
| High-cardinality stress | test_distributed_knowledge_highcardinality_stress.cpp | 3 | In-process stub/simulation stress only; not real shard or module-runtime validation |

**Total source declarations: 110 across 9 files.** This is a static count from the test sources, not a passing-test result.

Test registration pattern: `module_distributed_knowledge_<stem>_focused`
CTest tier: unit
Timeout: 120s
Labels: distributed_knowledge

Target registration is configured through the `test_*.cpp` glob in `tests/distributed_knowledge/CMakeLists.txt`; this audit did not build or execute the targets.

## Known Issues and Limitations

- No graph synchronization, peer transport, durable federation state, automatic retry, or restart recovery exists in this module.
- `FederatedRAGMerger` consumes already-fetched results; positive `shard_timeout_ms` does not enforce per-shard deadlines. The LoRA timeout overload cannot cancel unfinished work and is not a guaranteed wall-clock deadline.
- LoRA silently ignores stale/future/duplicate submissions; feedback policy rejection drops the item, and callback delivery is best effort. Feedback dedup and counters are process-local.
- Distillation callback failure may leave partial delivery after privacy budget is charged; no automatic retry/reconciliation contract exists.
- Mock-only contract/stress tests and local-helper benchmarks are not real topology or production-performance evidence.

## Breaking Changes

Inbound capability dispatch now fails closed without an explicit trust policy; callers that previously dispatched directly must configure one. Unknown domain strings now throw instead of being silently coerced to `GENERAL`. The `AlwaysPermitTrustPolicy` must be explicitly installed and is not production authorization.

## v2.5.0 -> v3.0.0 Coordination Gates

The v2.5.0 baseline is limited to the in-process component behavior above.
v3.0.0 sign-off requires all gates in
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md): linked
production-library tests, fail-closed capability trust and invalid-input
tests, real partial/all-timeout shard integration, operator-visible degraded
outcomes, and pinned same-hardware p95/p99 evidence. Graph synchronization,
bounded cancellation, and durable recovery remain release blockers until
implemented and tested; documentation alone does not close those gates.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `distributed_knowledge`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
