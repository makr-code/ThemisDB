# Unified Recovery Semantics Across Sharding, Replication, Failover, and AI Workloads

**Status**: REVIEW_CANDIDATE  
**Version**: 0.2  
**Last Updated**: 2026-08-10  
**Target Venue**: SIGMOD 2027

---

## Metadata

- **Scientific Delta**: Unify recovery semantics across transaction coordination, sharding, replication, failover, and AI/RAG execution paths instead of treating them as disconnected subsystem recoveries.
- **Canonical Evidence Sources**: `src/sharding/ROADMAP.md`, `docs/architecture/transaction_coordinators.md`, `src/replication/README.md`, `src/failover/ROADMAP.md`, `src/rag/README.md`.
- **Required Experiments**: cross-shard in-doubt recovery under failover, coordinator takeover while RAG requests are active, replication lag and recovery drift under mixed workloads.
- **Open Risks / Claim Boundaries**: subsystem evidence is strong, but a single end-to-end cross-module recovery evaluation package still needs to be assembled.
- **Overlap / Successor / Predecessor**: bridges flagship distributed ACID work with failover- and replication-specific manuscripts.

## Abstract

ThemisDB already documents mature recovery-related behavior in sharding, transaction coordination, replication, and failover. However, these guarantees are presently distributed across module-level artefacts. This manuscript proposes a unifying research narrative: recovery should be modeled as a cross-subsystem contract spanning in-doubt distributed transactions, coordinator failover, replication lag, and AI/RAG request continuity. The repository contains strong module-local evidence for these building blocks, but not yet a consolidated end-to-end experiment package. The paper should therefore emphasize semantic unification and workload design, backed by current subsystem evidence and a clearly staged experimental gap.

## I. Introduction

Recovery semantics become difficult to reason about when transaction, replication, failover, and AI-serving paths each expose their own local notion of correctness. For AI-native databases this is especially problematic because retrieval and inference requests may observe or amplify inconsistent intermediate states. ThemisDB provides the pieces of a unified story, but they are not yet published as a single resilience argument.

### Contributions

1. A unified recovery contract spanning sharding, replication, failover, and AI-facing workloads.
2. A taxonomy of in-doubt states, coordinator takeover paths, and observable recovery outcomes.
3. An evaluation plan tying distributed recovery semantics to user-visible RAG/AI behavior.

## II. Related Work

- 2PC/3PC/SAGA recovery semantics
- consensus and failover literature
- replication conflict-resolution and lag observability
- novelty delta: integrate these concerns with AI/RAG-visible continuity constraints

## III. System Model / Repository Scope

- distributed transaction coordination: `docs/architecture/transaction_coordinators.md`
- sharding protocol evidence: `src/sharding/ROADMAP.md`
- replication runtime: `src/replication/README.md`
- failover runtime: `src/failover/ROADMAP.md`
- AI-visible request layer: `src/rag/README.md`

## IV. Method / Design

### A. Unified Recovery Invariant Taxonomy

The paper proposes a three-layer taxonomy for recovery semantics across ThemisDB's subsystems:

1. **Transaction-layer recovery** — in-doubt distributed transactions, coordinator failover, 2PC/3PC/SAGA abort paths
2. **Infrastructure-layer recovery** — sharding node loss, replication lag absorption, failover state transition
3. **AI-layer recovery** — RAG retrieval context stability, inference request retry visibility, degraded-response policy

Recovery is well-behaved only when each layer upholds a consistent handoff contract: layer N+1 cannot expose a user-visible inconsistency from a failure that layer N has already absorbed.

### B. Rollback Isolation Model (Updates Module)

The `src/updates/` module provides a concrete instantiation of layer-1 recovery. Key properties:

- **Partial rollback via checkpoints**: rollback stops at the last committed checkpoint rather than unwinding the full operation sequence; fallback strategies prevent cascade failures
- **Reverse-sequence coordination**: in multi-participant updates, rollback proceeds in reverse order (leader last), matching the commit sequence to prevent partial visibility
- **Listener-based DiagnosticEmitter**: thread-safe broadcast via registered listener interface; observers receive structured `ErrorContext` (JSON-serializable) for each recovery event
- **Error code taxonomy**: all Updates errors in range [7400–7499]; structured taxonomy distinguishes isolation failures, coordination failures, and checkpoint failures

These properties map directly to the paper's layer-1 recovery invariants.

### C. Failover-Replication Coordination

Failover (`src/failover/`) and replication (`src/replication/`) interact at layer-2 recovery:

- split-brain prevention is fail-closed when quorum is unavailable (never guesses)
- replication lag observability surfaces as a bounded staleness metric during coordinator takeover
- DR concurrency guard ensures exactly one recovery plan executes at any time

### D. AI-Workload Observation Points

RAG and inference requests create three observable recovery touch points:

1. **Retrieval context stability**: snapshot isolation or MVCC at retrieval time determines whether a recovering shard exposes partial updates
2. **Request retry visibility**: retry policies interact with failover timing; a well-defined retry window prevents duplicate side effects
3. **Degraded-response semantics**: when recovery is in progress, the serving layer must distinguish "not yet available" from "permanently lost"

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `src/sharding/ROADMAP.md` | Phase 6 sections | 2PC/3PC/Calvin/SAGA recovery and fault-injection evidence | ready |
| E2 | `docs/architecture/transaction_coordinators.md` | architecture spec | distributed coordinator roles and recovery responsibilities | ready |
| E3 | `src/replication/README.md` | runtime behavior | promotion, conflict resolution, lag observability, CDC bridge | ready |
| E4 | `src/failover/ROADMAP.md` | Phases 2–6 | failover and DR execution semantics | ready |
| E5 | `src/rag/README.md` | module purpose / limits | AI/RAG workload surfaces affected by recovery semantics | ready |
| E6 | `src/updates/ROADMAP.md` | Q3 Items 1–3 | rollback isolation, partial rollback checkpoints, reverse-sequence coordination | ready |
| E7 | `tests/updates/test_updates_rollback_hardening_focused.cpp` | 20 tests | rollback isolation coverage: cascade prevention, checkpoint fallback, coord sequence | ready |
| E8 | `tests/updates/test_updates_diagnostics_focused.cpp` | 15 tests | DiagnosticEmitter listener pattern, ErrorContext JSON serialization | ready |
| E9 | `include/updates/updates_diagnostics.h` | error taxonomy | error code range [7400–7499], structured taxonomy | ready |
| E10 | `include/updates/updates_diagnostic_emitter.h` | emitter interface | listener pattern, thread-safe broadcast | ready |

### E6–E10 Detail — Updates Module Recovery Evidence

The Updates module (35 focused tests, error codes 7400–7499) provides layer-1 instantiation evidence:

- `test_updates_rollback_hardening_focused.cpp` (20 tests): validates that partial rollback stops at checkpoints, does not cascade across isolation boundaries, and executes in reverse coordination sequence
- `test_updates_diagnostics_focused.cpp` (15 tests): validates that DiagnosticEmitter broadcasts structured `ErrorContext` to all registered listeners thread-safely
- `updates_diagnostics.h`: defines the structured error taxonomy — isolation failures (74xx), coordination failures (74xx), checkpoint failures (74xx) — enabling precise recovery-event attribution

## VI. Experimental Methodology

### A. Setup
- mixed distributed + AI workload testbed
- controlled coordinator failure injection
- reproducible seeds and replayable failure schedules

### B. Workloads
- W1: cross-shard commit with coordinator crash
- W2: replication lag plus failover during active RAG retrieval
- W3: recovery replay with concurrent AI and transactional load

### C. Metrics
- commit latency and abort rate
- in-doubt recovery duration
- failover takeover time
- replication lag / stale-read exposure
- retrieval continuity and answer stability under recovery

## VII. Results

### A. Primary Results
- subsystem-local recovery claims already have direct evidence
- cross-module experimental package remains pending

### B. Ablations / Sensitivity
- compare failover with and without active AI requests
- compare protocol families under the same fault schedule

### C. Negative Results
- no single consolidated end-to-end benchmark bundle exists yet

## VIII. Discussion

This paper is scientifically strong because it turns several mature subsystem results into a unifying correctness and resilience narrative. The risk is overclaiming global behavior before the end-to-end experiments are bundled.

### Supported claims
- sharding, failover, and replication all expose concrete recovery primitives (`E1`, `E3`, `E4`)
- AI-facing workloads define user-visible surfaces that recovery can perturb (`E5`)

### Deferred claims
- end-to-end recovery transparency for all mixed AI/transaction workloads
- quantitative continuity guarantees under cluster-scale churn

## IX. Reproducibility & Artifact

- existing evidence resides in module roadmaps, module READMEs, and recovery benchmarks/tests
- next step: dedicated experiment protocol under `research/experiments/`

## X. Limitations, Risk, Ethics

- mixed-workload recovery semantics are workload-dependent
- not yet a full formal proof or exhaustive topology study

## XI. Conclusion

ThemisDB has enough subsystem evidence to justify a unified recovery manuscript now. The main remaining task is evidence consolidation across modules, not invention of new mechanisms.
