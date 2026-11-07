---
Author: "ThemisDB Team"
Created: "2026-08-06"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Projects Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-07 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · SECURITY.md · PERFORMANCE_EXPECTATIONS.md -->

## Current Status

**Assessment: implemented feature base; hardening and evidence incomplete.** The module implements project lifecycle transitions, document-metadata snapshots with SHA-256 integrity checks, structural diff/merge, template instantiation, sharing/permissions, in-process object locks, change notifications, metrics, and an in-memory audit-log implementation. This is a source inventory, not production-readiness approval.

The scoped implementation is in `src/projects/` and its public contracts in `include/projects/`. Tests in `tests/projects/` exercise the principal APIs and selected invalid-input, snapshot-corruption, lock-contention, and merge cases. The registered `bench_projects_release_gates` target in `benchmarks/projects/` measures only `ProjError` enum operations; it provides no evidence for project-runtime performance. No tests or benchmarks were executed for this roadmap update.

### Source and evidence baseline

| Area | Verified implementation / contract | Current evidence and gap |
|---|---|---|
| Lifecycle | `src/projects/project_lifecycle.cpp`, `include/projects/project_lifecycle.h`; validation-gated transitions persisted through RocksDB | `tests/projects/test_projects.cpp`, `test_projects_phase4_hardening_focused.cpp`; failed transition-audit writes are currently best-effort after the state write, so state and audit are not atomic |
| Snapshot/versioning | `src/projects/project_versioning.cpp`, `include/projects/project_versioning.h`; scans project document keys, serializes metadata, stores snapshot data and SHA-256 checksum, verifies before restore | Focused tests cover snapshot creation and corruption rejection; no scale, crash-recovery, or runtime benchmark evidence |
| Diff/merge | `src/projects/project_diff.cpp`, `include/projects/project_diff.h`; recursive JSON comparison and three-way conflict reporting | Focused tests cover nested changes and merge conflicts; unordered-map traversal means output ordering is not a documented deterministic contract, and benchmarks do not measure it |
| Collaboration | `src/projects/collaboration_manager.cpp`, `include/projects/collaboration_manager.h`; RocksDB-backed share/change records, in-process locks, bounded in-memory feed, synchronous subscriber callbacks | Tests cover permissions, locks, callbacks, and contention; no cross-process lock semantics or sustained-load evidence |
| Audit | `src/projects/in_memory_project_audit_log.cpp`, `include/projects/project_audit_log.h`; thread-safe in-memory log with bounded capacity and eviction | Query filters the in-memory collection and sorts results; this implementation is not durable audit retention or tamper evidence |
| API/use cases | `include/projects/README.md`, `include/projects/ARCHITECTURE.md`, `include/projects/ROADMAP.md` | `IProjectBundleManager` is an interface only; the module has no general authentication/authorization middleware, as documented in `src/projects/SECURITY.md` |
| Benchmarks | `benchmarks/projects/CMakeLists.txt`, `bench_projects_release_gates.cpp` | Only enum cast, dispatch, range, and batch-cast cases are registered; snapshot, diff/merge, template, collaboration, and audit runtime paths are unmeasured |

## In Progress

- [ ] Establish project-runtime benchmark baselines and repeatable release gates (Target: v2.5.0 / Wave 1)
- [ ] Define deterministic behavior and storage-failure outcomes for existing operations (Target: v2.5.0 / Wave 2)

## Planned Features

- [ ] Validate an authenticated caller boundary, bounded-resource behavior, and soak evidence (Target: v2.5.0 / Wave 3)
- [ ] Specify and, if approved, deliver durable project events and cross-process collaboration semantics (Target: v3.0.0 / Waves 4–6)

## Research Findings and Gap Analysis

Research below informs the proposed methods; it does not establish that the module meets a target or that a feature is required by users.

| Evidence | Finding relevant to `projects` | Implication |
|---|---|---|
| YCSB workload methodology [1] | Serving-system comparisons need representative operation mixes rather than a single best-case operation | Benchmark snapshots, diffs, templates, collaboration, and audit queries with documented data size, read/write mix, contention, and skew |
| Google Benchmark guidance [2] | Warm-up, repeated runs, variance-aware comparison, and machine context matter for interpretable measurements | Keep raw run results and environment metadata; instrument operation-level latency percentiles instead of treating a benchmark average as p95/p99 |
| NIST SP 800-92 [3] | Log management includes collection, protection, retention, review, and operational process—not only recording events | Distinguish the bounded in-memory audit helper from a durable, access-controlled, retention-managed audit service; do not claim regulatory compliance from this roadmap |
| Shapiro et al. on CRDTs [4] | Convergent replicated data types require operation/data semantics that preserve convergence under concurrent updates | Evaluate CRDTs only if disconnected multi-writer editing is a validated requirement; the current snapshot-based three-way merge is not a CRDT and no CRDT migration is presumed |

### Research-to-code conclusions

- **Performance and scale:** snapshot creation collects and serializes all project document metadata; diff loads snapshot arrays into maps; audit queries scan and sort in-memory entries. The benchmark gap is direct and measurable. No numeric service-level target can be responsibly claimed before workload and deployment requirements are supplied and a repeatable baseline is captured.
- **Usefulness and scenarios:** source-backed scenarios are lifecycle tracking, metadata snapshots, template bootstrapping, structured change comparison, and sharing/locking. End-user workflow, offline editing, project sizes, concurrency, and retention requirements are not defined by the scoped code, so these remain assumptions to validate.
- **Security and resilience:** checksum verification detects content/checksum mismatch but is not an authenticity proof. Callers must enforce access control; locks are manager-local; the provided audit implementation evicts old in-memory records. Lifecycle audit persistence is best-effort after state persistence. These are explicit design boundaries, not claims that no controls exist.
- **Governance and maintainability:** existing tests exercise behavior but do not substitute for executed CI, fault-injection, soak, or benchmark evidence. Public API and data-format changes must be versioned and migration-tested. Module `project_bundle.h` does not prove a working import/export feature.
- **Architecture and operations:** storage, authorization, metrics/telemetry, deployment, and operator ownership are external dependencies or gaps; the module has no standalone deployment configuration object or operator runbook verified in the scoped paths.

## Release Position

The following is a **proposed module plan**, not a project release commitment. The root `ROADMAP.md` identifies `v2.4.0-alpha` as the current repository version; root release governance and `release_critical` CI on `develop` remain authoritative. Owners, exact release dates, customer workload requirements, and hardware profiles for these module waves are unassigned.

### v2.5.0 — compatible stabilization and evidence

No public API or persisted-format break is planned for this release. Any newly discovered compatibility requirement must be reviewed against `VERSIONING.md` before implementation.

#### Wave 1 — Runtime benchmark baseline

- **Goal:** establish reproducible performance evidence for real projects runtime operations.
- **Tasks:**
  - [ ] Register runtime benchmark cases for snapshot create/list/restore, diff/merge, template instantiation, collaboration change/lock/query, and audit query (Target: v2.5.0 / Wave 1).
  - [ ] Cover 100/1,000/10,000 document metadata records where applicable, 1/4/16 concurrent actors, and merge conflict mixes of 0%, 1%, and 50%, with fixed seeds and correctness checks (Target: v2.5.0 / Wave 1).
  - [ ] Record compiler, build type, RocksDB configuration, CPU, OS, dataset, warm-up, repetitions, throughput, memory, and operation-level p50/p95/p99 in machine-readable output (Target: v2.5.0 / Wave 1).
- **Estimate:** 2–3 engineer-weeks.
- **Dependencies:** benchmark registration and a declared release-profile runner; benchmark outputs must be retained as artifacts.
- **Risk / trade-off:** large data sets and synchronization may make runs slow or noisy; start with bounded cases and reject unrepeatable measurements rather than publishing misleading thresholds.
- **Success metric:** every runtime surface above has at least one valid benchmark case and a checked-in or CI-retained baseline artifact with all required workload/environment fields; p50/p95/p99 and throughput are reported for each supported case.
- **Validation:** build `bench_projects_release_gates`; run the project benchmark suite in release mode on the declared runner; verify benchmark setup/teardown, result correctness, complete JSON output, and repeatability across at least five independent process runs. Calibrate regression budgets from measured variance before enabling a blocking threshold.

#### Wave 2 — Determinism and storage-failure hardening

- **Goal:** make replayable project operations and partial failures testable without changing the existing public contract.
- **Tasks:**
  - [ ] Define and test stable ordering for diff/merge results (Target: v2.5.0 / Wave 2).
  - [ ] Add deterministic seeded stress tests for snapshot churn, conflicting merges, and concurrent collaboration calls (Target: v2.5.0 / Wave 2).
  - [ ] Add storage-failure injection around snapshot metadata/content/index writes and lifecycle state/audit writes (Target: v2.5.0 / Wave 2).
  - [ ] Specify which partial states are rolled back or surfaced and ensure diagnostics identify the failed stage (Target: v2.5.0 / Wave 2).
- **Estimate:** 3–5 engineer-weeks.
- **Dependencies:** Wave 1 fixtures and agreed deterministic output contract; storage fault injection must not introduce test-only production behavior.
- **Risk / trade-off:** ordering guarantees can add sorting cost; stronger write atomicity may require RocksDB batch support or an API-visible failure semantic change, which must be reviewed before adoption.
- **Success metric:** 100 repeated runs with the same seed produce identical ordered diff/merge outputs; each injected write failure has an asserted recovery outcome and leaves no unaccounted partial snapshot; all existing focused project tests remain green.
- **Validation:** focused unit tests for ordering and every write-failure boundary; seeded stress runs at 1/4/16 actors; compare benchmark results with Wave 1 to quantify any latency/memory cost.

#### Wave 3 — Security, soak, and v2.5.0 handoff

- **Goal:** prove the module’s access-control boundary, bounded-resource behavior, and release evidence.
- **Tasks:**
  - [ ] Map every project mutation entry point to its owning authenticated/authorized caller and add integration tests showing unauthorized operations fail closed at that boundary (Target: v2.5.0 / Wave 3).
  - [ ] Exercise malformed templates, snapshot metadata, and collaboration input with fuzz/property tests (Target: v2.5.0 / Wave 3).
  - [ ] Document audit eviction/retention behavior and run a 24-hour project-operation soak (Target: v2.5.0 / Wave 3).
  - [ ] Assign recovery/runbook ownership for storage failure, conflict storms, and callback stalls (Target: v2.5.0 / Wave 3).
- **Estimate:** 2–4 engineer-weeks, excluding external security review and runner provisioning.
- **Dependencies:** Wave 2 failure semantics, caller/auth owners, representative test deployment, and a named operations reviewer.
- **Risk / trade-off:** module-level controls alone cannot prove deployment authorization; soak time and storage configuration may differ materially from production.
- **Success metric:** zero unauthorized mutation succeeds in the integration matrix; no crash, unbounded feed growth, or unreconciled acknowledged write occurs during the soak; release evidence links tests, benchmark artifacts, and the threat-boundary review.
- **Validation:** focused project tests, caller integration tests, fuzz/property tests, a 24-hour soak with memory/feed bounds checked, current `release_critical` CI on `develop`, and human security/operations sign-off. A green build alone is not sign-off.

### v3.0.0 — explicit durability and collaboration architecture

These are strategic design proposals. Persisted schema changes, new concurrency semantics, or public API breaks require a reviewed design and migration plan before implementation.

#### Wave 4 — Requirements and contract decision

- **Goal:** choose a supported project workload and specify durable collaboration/audit semantics before changing architecture.
- **Tasks:**
  - [ ] Obtain representative project/document counts, operation mixes, actor counts, retention and recovery objectives, identity-provider boundary, and offline/multi-writer needs (Target: v3.0.0 / Wave 4).
  - [ ] Compare current three-way merge with optimistic concurrency and CRDT approaches against the validated scenarios (Target: v3.0.0 / Wave 4).
  - [ ] Specify event identity/order, authorization, durability/acknowledgement, replay/pagination, retention/deletion, schema versioning, migration, and callback delivery guarantees (Target: v3.0.0 / Wave 4).
  - [ ] Decide whether bundle import/export is an actual product requirement (Target: v3.0.0 / Wave 4).
- **Estimate:** 2–3 engineer-weeks plus product/security/operations review.
- **Dependencies:** named product owner, auth/storage/observability owners, and v2.5.0 baseline evidence.
- **Risk / trade-off:** selecting distributed merge semantics without a real offline collaboration requirement can increase API and operational complexity without user value.
- **Success metric:** approved design records explicit non-goals, compatibility decision, workload envelope, recovery/retention requirements, failure matrix, and numeric latency/throughput/storage budgets for every in-scope operation.
- **Validation:** architecture and threat-model review; migration rehearsal using representative persisted data; recorded product acceptance of the selected scenarios and budgets.

#### Wave 5 — Durable, bounded project events

- **Goal:** implement only the Wave 4-approved durable project event and collaboration contract.
- **Tasks:**
  - [ ] Version persisted records and assign stable event IDs and an ordering/replay cursor (Target: v3.0.0 / Wave 5).
  - [ ] Implement bounded, paginated durable event/audit queries and explicit retention/purge behavior (Target: v3.0.0 / Wave 5).
  - [ ] Make acknowledged state and audit/event updates atomic or provide a recoverable outbox protocol (Target: v3.0.0 / Wave 5).
  - [ ] Replace manager-local locks with the Wave 4-selected cross-process concurrency/lease mechanism if multi-instance support is required (Target: v3.0.0 / Wave 5).
  - [ ] Bound subscriber buffering and define retry, duplicate, and slow-consumer behavior; enforce authorization at the owning API/service boundary and preserve actor/request provenance (Target: v3.0.0 / Wave 5).
- **Estimate:** 6–10 engineer-weeks (range depends on storage primitives, authorization integration, and migration scope).
- **Dependencies:** Wave 4 decision, storage transaction/batch APIs, authentication/authorization integration, and observability hooks.
- **Risk / trade-off:** durability and ordered delivery add write amplification, storage/retention cost, and operational failure modes; asynchronous callbacks can change timing and delivery semantics. Do not silently change existing behavior.
- **Success metric:** zero acknowledged events lost across the defined crash/restart test matrix; duplicate delivery follows the approved idempotency contract; stored data and event/feed sizes remain within configured bounds; p99 query/write latency and throughput meet the Wave 4 budgets on representative hardware.
- **Validation:** migration and rollback tests; process-restart and fault-injection tests at each acknowledgement boundary; multi-instance contention tests; authorization matrix; bounded-queue/backpressure tests; benchmark comparison against the v2.5.0 artifact.

#### Wave 6 — Scale, resilience, and v3.0.0 acceptance

- **Goal:** demonstrate correctness, performance, security, and operability under the approved workload envelope.
- **Tasks:**
  - [ ] Run mixed-workload load and long-duration soak tests and inject storage, subscriber, process, and recovery faults (Target: v3.0.0 / Wave 6).
  - [ ] Fuzz versioned event/snapshot payloads and migration inputs; verify integrity and retention controls (Target: v3.0.0 / Wave 6).
  - [ ] Publish dashboards/alerts and exercise an operator runbook (Target: v3.0.0 / Wave 6).
  - [ ] Complete API compatibility and migration notes, then close only evidence-backed roadmap gates (Target: v3.0.0 / Wave 6).
- **Estimate:** 4–6 engineer-weeks, excluding hardware availability and independent penetration testing.
- **Dependencies:** Wave 5 implementation, representative hardware, test data, and designated operations/security reviewers.
- **Risk / trade-off:** passing synthetic load tests cannot prove unknown customer workloads or regulatory compliance; retain stated envelope limits and unresolved risks.
- **Success metric:** all approved budgets pass on declared hardware; zero acknowledged-write loss in crash/recovery scenarios; no authorization bypass; no unbounded memory/feed growth during the soak; all schema migrations and rollback cases pass.
- **Validation:** release-profile benchmark artifacts, fault/chaos and soak reports, focused and integration tests, security review, runbook exercise, current `release_critical` CI, and human release sign-off.

## Prioritized Measures and KPIs

| Priority | Measure | Target / gate |
|---|---|---|
| P0 | Benchmark coverage and reproducibility | 100% of in-scope runtime surfaces represented; five independent baseline runs with workload and machine context captured |
| P0 | Runtime performance | Report operation-level p50/p95/p99, throughput, and memory; Wave 1 sets budgets only after variance is measured; v3 budgets must be approved in Wave 4 |
| P0 | Data/audit recovery | Zero unaccounted acknowledged writes in injected-failure and restart tests |
| P0 | Access control | Zero unauthorized mutation succeeds in the caller/API integration matrix |
| P1 | Determinism | Identical ordered diff/merge output for 100 same-seed repetitions |
| P1 | Bounded operation | Feed, audit memory, and subscriber queue remain within configured bounds under stress; eviction/drop/backpressure is observable and documented |
| P1 | Release governance | `release_critical` green on `develop`; no v3 release claim without attached test, benchmark, security, and operations evidence |

## Risk and Trade-off Register

- **Performance vs. correctness:** sorted deterministic deltas, integrity validation, durable audit, and stable event ordering can increase CPU, write amplification, and storage. Measure changes against the Wave 1 baseline; do not waive correctness for throughput.
- **Bounded memory vs. audit completeness:** current in-memory log eviction is a bounded-resource behavior, not durable retention. Durable compliance needs an explicitly owned storage, retention, access, and export policy.
- **Collaboration simplicity vs. distributed semantics:** manager-local locks and synchronous callbacks are simple but do not imply cross-process coordination or non-blocking delivery. Distributed locks, leases, async events, and CRDTs each add consistency and operational obligations.
- **Compatibility vs. redesign:** preserve v2.5.0 behavior where possible; use v3.0.0 for any approved schema/API/concurrency break only with versioned migration, rollback, and compatibility tests.
- **Evidence vs. assertion:** source presence and benchmark registration do not establish executed test results, production behavior, or release readiness.

## Open Assumptions and Knowledge Gaps

- No customer workload traces, target deployment hardware, project-size distribution, SLOs, retention policy, recovery objectives, or operations owner were found in the scoped module evidence.
- It is unknown whether projects must support disconnected/offline multi-writer editing; no CRDT is selected absent this requirement.
- The authentication/authorization service boundary must be named by the owning API/service teams; the projects module does not provide general auth middleware.
- It is not yet decided whether audit retention must be durable, tamper-evident, exportable, or compliance-certified. NIST guidance is a design reference, not a compliance determination.
- Product and release owners must confirm whether these v2.5.0/v3.0.0 proposals fit the release scope and assign dates/owners. Root `ROADMAP.md`, `VERSIONING.md`, and `RELEASE_STRATEGY.md` take precedence.
- The Developer LLM Wiki index/status was last generated on 2026-11-02 and has no curated projects-module page; this roadmap therefore relies on direct source and module documentation rather than a wiki synthesis.

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] Approve the project workload envelope, v2.5 compatibility boundary, and v3 persisted/API change policy (Target: Wave 4)
- [ ] Specify deterministic delta ordering and failure/acknowledgement semantics (Target: Waves 2 and 4)

### Phase 2: Core Implementation
- [ ] Register runtime benchmark cases for snapshot, diff/merge, template, collaboration, and audit paths (Target: Wave 1)
- [ ] Implement only the durable event/concurrency design approved in Wave 4 (Target: Wave 5)

### Phase 3: Error Handling and Edge Cases
- [ ] Exercise partial snapshot/lifecycle writes, malformed payloads, and subscriber failures with explicit expected outcomes (Target: Waves 2 and 5)
- [ ] Validate bounded feed, audit, and queue behavior under overflow and recovery (Target: Waves 3 and 6)

### Phase 4: Tests
- [ ] Add deterministic seeded stress, storage-failure injection, and caller authorization integration tests (Target: Waves 2 and 3)
- [ ] Add migration, multi-instance, crash-recovery, and property/fuzz tests for approved v3 semantics (Target: Waves 5 and 6)

### Phase 5: Performance and Hardening
- [ ] Capture repeatable runtime baselines and calibrate regression budgets from measured run variance (Target: Wave 1)
- [ ] Verify approved latency/throughput/storage budgets on representative hardware under mixed load (Target: Waves 5 and 6)

### Phase 6: Documentation and Acceptance
- [ ] Publish benchmark, test, security, and soak evidence with limits and unresolved assumptions (Target: Waves 3 and 6)
- [ ] Obtain human product, security, and operations acceptance before marking either release scope complete (Target: v2.5.0 / v3.0.0)

## Production Readiness Checklist

- [ ] Runtime benchmark coverage, environment metadata, and repeatable baselines are available
- [ ] Failure injection and recovery outcomes are tested for every multi-write project operation
- [ ] The actual caller authorization boundary is integration-tested and fails closed
- [ ] Audit durability, retention, integrity, and deletion semantics are explicit for the deployment
- [ ] Bounded resources, overload behavior, and callback delivery guarantees are measurable and documented
- [ ] Representative workload, security, soak, operational, and `release_critical` evidence is reviewed by owners

## Known Issues and Limitations

- Project-runtime latency and throughput are not measured by the current `projects` benchmark.
- Snapshot cost grows with project metadata volume; no project-size or retention envelope is established.
- Diff result ordering is not currently a documented deterministic guarantee.
- Locks and provided audit storage are manager-local/in-memory; the in-memory audit implementation evicts entries when its capacity is exceeded.
- Collaboration subscribers execute synchronously; callback stalls can delay notification processing.
- Lifecycle state persistence and its audit entry are not atomic when the audit write fails.
- The module does not supply general authentication/authorization middleware or a concrete project-bundle implementation.

## Breaking Changes

No breaking change is planned for v2.5.0. v3.0.0 may introduce versioned persisted records and revised concurrency/event APIs only after Wave 4 design approval. Any accepted break requires a migration and rollback plan, compatibility tests, and release notes under `VERSIONING.md`.

## Research References

1. Brian F. Cooper et al., “Benchmarking Cloud Serving Systems with YCSB,” ACM Symposium on Cloud Computing (SoCC), 2010. [DOI: 10.1145/1807128.1807152](https://doi.org/10.1145/1807128.1807152). Workload-mix and extensible serving benchmark methodology.
2. Google Benchmark, [User Guide](https://google.github.io/benchmark/user_guide.html), sections on warm-up, repetitions, statistics, and reducing variance. Used as instrumentation guidance, not as evidence of module performance.
3. Karen Kent and Murugiah Souppaya, NIST SP 800-92, [Guide to Computer Security Log Management](https://csrc.nist.gov/pubs/sp/800/92/final), 2006. General log-management guidance; it does not certify this module or deployment.
4. Marc Shapiro et al., “Conflict-Free Replicated Data Types,” SSS 2011, pp. 386–400. [DOI: 10.1007/978-3-642-24550-3_29](https://doi.org/10.1007/978-3-642-24550-3_29). Used to frame the conditional multi-writer design evaluation, not to recommend unconditional CRDT adoption.

## Program and Source-of-Truth Alignment

- This roadmap is the canonical module implementation plan. Public-header-only follow-ups remain in [`../../include/projects/ROADMAP.md`](../../include/projects/ROADMAP.md), which points back here.
- Root `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`, plus `VERSIONING.md` and `RELEASE_STRATEGY.md`, remain authoritative for project release status and scope.
- `release_critical` CI must remain green on `develop`; the projects plan does not override root wave order or release gates.
- Sources checked for this update: `src/projects/{project_lifecycle,project_versioning,project_diff,project_template,collaboration_manager,project_metrics,in_memory_project_audit_log}.cpp`; `include/projects/`; `tests/projects/`; `benchmarks/projects/`; module README, architecture, security, and performance expectations.
