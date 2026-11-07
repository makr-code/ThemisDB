# Metadata Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: research-backed version and wave plan added 2026-11-07; release measurements not revalidated -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

**Author:** ThemisDB Contributors  
**Created:** 2026-10-31  
**Last Updated:** 2026-11-07
**Status:** review  
**AI_Used:** true  
**AI_Models:** unknown  
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator  
**Human_Review:** false  
**Human_Author:** ThemisDB Contributors  
**Source level:** level1 — module-local  
**SOT domain:** module-behavior  
**Canonical source references:** [metadata benchmark mappings](./PERFORMANCE_EXPECTATIONS.md), [benchmark sources](../../benchmarks/metadata/), [module runbook](./RUNBOOK.md)  
**Related issue / milestone:** [#7435](https://github.com/makr-code/ThemisDB/issues/7435) · v2.5.0 / v3.0.0 research and wave plan

> Exact model backend not verifiable from final document text alone.

## Current Status

Production metadata runtime exists across schema discovery, consistency tooling, lineage/export behavior, distributed metadata surfaces, and information-schema/statistics support.

## In Progress

- [~] re-baseline p95/p99 envelopes for metadata access and cache operations (Target: Q1 2027)
- [~] broaden benchmark depth beyond cache-centric metadata hot paths (Target: Q1 2027)
- [~] harden long-running reliability under sustained schema mutation pressure (Target: Q1 2027)

## Planned Features

### Mid-term (6-12 months)
- [~] expand deep concurrent-access stress coverage beyond focused unit tests (Target: Q2 2027) — `tests/metadata/test_metadata_highcardinality_stress.cpp` (HC-MD-03) delivered; Q2 2027 extended coverage in progress
- [?] confirm whether the deployment runbook meets the planned incident-triage scope (Target: Q2 2027) — a module developer runbook exists at `RUNBOOK.md`; deployment command coverage and completion status are not verified here

## Version Roadmap: v2.5.0 and v3.0.0

This plan is a module-level proposal based on the source evidence summarized in
[`RESEARCH.md`](./RESEARCH.md), the public contracts under
[`include/metadata/`](../../include/metadata/), focused tests under
[`tests/metadata/`](../../tests/metadata/), and benchmark sources under
[`benchmarks/metadata/`](../../benchmarks/metadata/). It distinguishes
implemented behavior from test/benchmark source presence and from executed
release evidence. `v2.5.0` follows the repository's next-minor release policy;
`v3.0.0` is a strategic horizon only and is not a committed release date or
scope.

### v2.5.0 — evidence, reliability, and compatible hardening

**Outcome:** retain the current public contract while establishing production-
representative evidence and closing integration/reliability gaps.

#### Wave 2.5-A — Workload baseline and contract inventory

- **Priority / estimate:** P0; S (1–2 person-weeks), excluding shared hardware/CI wait.
- **Goal:** produce a reproducible baseline and a source-to-test-to-benchmark map before selecting optimizations.
- **Tasks:**
  - [ ] Define representative metadata workloads and scale points for table count, entity count, schema churn, concurrent readers/writers, and export volume; record hardware, build profile, seed, and database state (Target: v2.5.0 alpha).
  - [ ] Extend the release benchmark manifest to cover actual `SchemaManager` cold/warm access, refresh/churn, consistency scan, lineage traversal, and exporter outcomes; label helper-only and stub-only cases as such (Target: v2.5.0 alpha).
  - [ ] Record repeated-run p50/p95/p99, throughput, and memory results against the same workload; set the absolute p99 gate only after maintainers approve a measured SLO (Target: v2.5.0 beta).
  - [ ] Inventory public metadata contracts and classify the stale `metadata_api_contract.h` as retained/frozen, corrected, or deprecated before any API change (Target: v2.5.0 beta).
- **Dependencies:** representative release hardware and benchmark runner; release-owner decision for workload and p99 SLO.
- **Risk / trade-off:** hardware and workload variability can produce misleading thresholds; extra benchmark coverage increases CI/runtime cost. Do not treat initial runs as universal limits.
- **Success metric / validation:** every required benchmark case appears in a machine-readable run manifest; at least five recorded repetitions per benchmark with p50/p95/p99 and memory context; run-to-run variance and environment metadata are reported; no performance pass is claimed until the approved threshold and baseline are present. Validate using the registered targets in [`benchmarks/metadata/CMakeLists.txt`](../../benchmarks/metadata/CMakeLists.txt) and compare results with [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

#### Wave 2.5-B — Runtime security and reliability evidence

- **Priority / estimate:** P0; M (3–5 person-weeks), dependent on Wave 2.5-A's test workload.
- **Goal:** verify security and failure behavior through real metadata paths rather than inferred interface presence.
- **Tasks:**
  - [ ] Add/extend tests using the real `SchemaManager` and RocksDB for concurrent read/mutation, cache invalidation, malformed schema, and restart/recovery behavior; keep test doubles explicitly separated (Target: v2.5.0 beta).
  - [ ] Trace metadata security-provider and export-policy use from public entry points to server/export callers; test denied and allowed export behavior at the integration boundary, or document the unimplemented enforcement boundary (Target: v2.5.0 beta).
  - [ ] Exercise exporter timeout, transport failure, partial response, and retry/idempotency behavior; assert typed outcomes and absence of sensitive payloads in diagnostics (Target: v2.5.0 beta).
  - [ ] Validate that consistency findings remain explicitly bounded and that callers do not interpret a partial 5,000-key scan as full reconciliation (Target: v2.5.0 RC).
- **Dependencies:** Wave 2.5-A workload and benchmark fixtures; server/export caller owners for integration tests.
- **Risk / trade-off:** integration tests can be slower and more environment-sensitive than focused tests; tightening an unenforced policy boundary may expose compatibility or ownership questions.
- **Success metric / validation:** zero lost updates or inconsistent cache observations in deterministic concurrency/restart tests; all tested denied exports fail closed; every injected exporter failure yields an explicit outcome; sanitizer/repeated stress runs report no data races or memory errors. Register tests in [`tests/metadata/CMakeLists.txt`](../../tests/metadata/CMakeLists.txt) or the owning integration suite, and retain test output as release evidence.

#### Wave 2.5-C — Release gates and operator acceptance

- **Priority / estimate:** P1; M (2–4 person-weeks), after Waves 2.5-A and 2.5-B.
- **Goal:** make the metadata release decision auditable and operable without introducing a breaking API.
- **Tasks:**
  - [ ] Re-run the complete metadata benchmark manifest on representative hardware and compare with Wave 2.5-A baseline; investigate every regression above the existing 10% relative guard (Target: v2.5.0 RC).
  - [ ] Agree and record the absolute p99 SLO, or explicitly leave the gate blocked rather than inventing a threshold (Target: v2.5.0 RC).
  - [ ] Verify deployment/runbook commands, symptoms, observability signals, rollback and recovery steps against actual server behavior; resolve the runbook status discrepancy noted in [`RESEARCH.md`](./RESEARCH.md) (Target: v2.5.0 RC).
  - [ ] Retain test, benchmark, security review, and operator sign-off evidence with the release record (Target: v2.5.0 stable).
- **Dependencies:** successful Wave 2.5-A and 2.5-B; representative hardware; release and operations sign-off.
- **Risk / trade-off:** an unapproved SLO or unavailable hardware can delay the release gate; accepting a source-only benchmark or runbook would overstate readiness.
- **Success metric / validation:** no mapped benchmark case missing; no unexplained regression greater than 10% against the recorded baseline; p99 is within the approved SLO; all operator procedures are executed successfully in a documented validation environment; security and release evidence is linked and reviewed.

### v3.0.0 — versioned governance and scale architecture

**Outcome:** consider deliberate major-version contract evolution only after v2.5.0
evidence confirms a product need. The items below are design candidates, not
commitments to add durable lineage, snapshots, or a new storage model.

#### Wave 3.0-A — Contract and governance design

- **Priority / estimate:** P1; M (3–6 person-weeks), beginning after the v2.5.0 contract inventory.
- **Goal:** establish one versioned, supportable metadata contract and decide which persistence/interoperability capabilities are required.
- **Tasks:**
  - [ ] Decide the lifecycle of the stale `metadata_api_contract.h` and define a versioned schema/metadata compatibility policy with upgrade and rollback rules (Target: v3.0.0 design freeze).
  - [ ] Decide, with consumers, whether lineage and metadata snapshots require durable storage, retention/deletion controls, or remain explicitly in-memory; document ownership, privacy, and recovery requirements before implementation (Target: v3.0.0 design freeze).
  - [ ] Specify export authorization, tenant/data minimization, auditability, and failure semantics at the actual caller boundary; map any lineage interoperability proposal to a versioned external schema such as OpenLineage facets (Target: v3.0.0 design freeze).
  - [ ] Publish compatibility fixtures and migration examples for every approved contract change (Target: v3.0.0 beta).
- **Dependencies:** v2.5.0 test/benchmark evidence; product, security, and API-owner decisions; no API-breaking implementation before human approval.
- **Risk / trade-off:** a unified contract may require a migration burden for consumers; adopting external conventions can create long-term compatibility obligations.
- **Success metric / validation:** all public metadata APIs have an identified owner, version policy, error/ownership/thread-safety contract, and compatibility test; every proposed breaking change has a migration and rollback case; security and data-retention requirements receive explicit review.

#### Wave 3.0-B — Scale, durability, and interoperability

- **Priority / estimate:** P1; L (6–12 person-weeks); estimate is conditional on Wave 3.0-A's approved scope.
- **Goal:** implement only the architecture choices supported by measured workloads and approved contracts.
- **Tasks:**
  - [ ] Benchmark candidate incremental discovery/indexing against the v2.5.0 scan baseline at workload sizes beyond the current 100-table benchmark ceiling; adopt it only if end-to-end benefit exceeds its write/space cost (Target: v3.0.0 beta).
  - [ ] If durable lineage or snapshots are approved, implement versioned persistence, bounded retention, restart/recovery, and backward-compatible migration tests; otherwise retain and document the in-memory limitation (Target: v3.0.0 beta).
  - [ ] Add end-to-end catalog/lineage interoperability tests for versioned payloads, authorization, retries, and schema evolution (Target: v3.0.0 RC).
  - [ ] Validate sustained concurrent mutation, restart recovery, and high-cardinality behavior using production code and representative hardware (Target: v3.0.0 RC).
- **Dependencies:** Wave 3.0-A design sign-off; Wave 2.5.0 baseline; storage and catalog integration owners.
- **Risk / trade-off:** incremental indexes and durable lineage add write amplification, storage/retention cost, and migration complexity; external catalog behavior may vary by server implementation.
- **Success metric / validation:** at the approved ≥10,000-table workload, p95 discovery latency improves by at least 2× over the recorded v2.5.0 scan baseline, with p99 at the 100-table control workload no more than 10% worse; if persistence is approved, all acknowledged records survive restart and retention/deletion tests pass; all versioned interop fixtures pass. These are proposed gates and must be confirmed against measured Wave 2.5-A hardware/workloads before v3.0.0 implementation commitment.

## Assumptions and Open Decisions

- Effort estimates are engineering person-week ranges, exclude shared release/hardware wait, and require team sizing before commitment.
- `v2.5.0` is the next planned minor version in [`VERSIONING.md`](../../VERSIONING.md). The repository's current canonical release is still v2.4.0-alpha; this module plan does not assert release readiness.
- No v3.0.0 date or scope is currently established by the root roadmap or version policy. Release governance must approve the strategic horizon.
- No current representative-hardware benchmark output or p95/p99 result was validated for this plan; all such gates remain pending evidence.
- The 2× discovery goal is a proposed decision threshold, contingent on an approved workload and repeatable v2.5.0 baseline, not a measured capability claim.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze schema/consistency/export contracts for active major line (Target: Q3 2026)
- [x] define explicit error taxonomy for schema/export/distributed metadata failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] complete hardening for metadata orchestration internals (Target: Q4 2026)
- [x] align consistency/lineage/export behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for malformed schema and export failure scenarios (Target: Q4 2026)
- [x] unify diagnostics across schema/consistency/export incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for metadata consistency and export edge scenarios (Target: Q4 2026)
- [x] extend deterministic stress fixtures for metadata cache and schema mutation operations (Target: Q4 2026)
- [x] Phase A: schema churn stress + SchemaVersionManager lock contract (MCH-S01..S08, MCH-L01..L04)
- [x] Phase B: consistency edge cases + lineage traversal + export failure paths (MCH-C01..C08, MCH-LN01..LN04, MCH-EX01..EX04)
- [x] Phase C: distributed catalog diagnostics + RBAC diagnostics (MCH-DC01..DC04, MCH-SEC01..SEC04)

### Phase 5: Performance and Hardening
- [~] maintain benchmark cases and gate targets for metadata hot paths (Target: Q4 2026) — case presence is documented; a passing release run is not established by this source review
- [?] validate p95/p99 and throughput behavior against recorded release baselines (Target: Q4 2026) — measurement output and baseline comparison not verified
- [x] Phase D: broaden benchmarks to consistency/lineage hot paths (GATE-MCL-01..04)
- [x] fix blocking_no_timeout gaps in schema_manager: removed unbounded shared-lock re-acquisition in getTable/getAllRelationships/getDatabaseMetadata; read under existing write lock instead (2026-08-04)
- [x] fix circular_lock_ordering / deadlock_risk in statistics_collector: release refresh_mutex_ before acquiring cache_mutex_ in refreshLoop_ (2026-08-04)

### Phase 6: Documentation and Acceptance
- [x] core metadata module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] benchmark README extended with full gate table (GATE-MET-01..04, GATE-MCL-01..04)
- [x] fix schema_consistency_checker: removed const_cast in runCheck() — results_mutex_ already declared mutable (2026-08-04)

## Production Readiness Checklist

- [x] core metadata surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Phase 2/3 hardening closed: ConsistencyIssue diagnostics, ColumnRef contracts, fail-safe edge paths (MCH-01..MCH-08)
- [x] Phase A–D tests and benchmarks delivered and registered (MCH-S/L/C/LN/EX/DC/SEC + GATE-MCL-01..04)
- [?] release benchmark stabilization — pending recorded representative-hardware results and baseline evidence
- [x] CRITICAL gap closure: blocking_no_timeout (schema_manager), circular_lock_ordering (statistics_collector), const_cast (schema_consistency_checker) — resolved 2026-08-04

## Known Issues and Limitations

- runtime behavior depends on schema scale, cache state, and configured integration/export paths.
- deep concurrent-access stress beyond focused unit level still expanding (Q2 2027).
- a module developer runbook exists at `RUNBOOK.md`; deployment-runbook scope and the conflicting status claims in this roadmap and `FUTURE_ENHANCEMENTS.md` require follow-up.
- the older gap-scanner severity/count snapshot is undated and not revalidated;
  rerun/review it before using its findings or severity labels to prioritize the
  Q2 2027 maintenance sweep (see [`MODULE_GAPS.md`](./MODULE_GAPS.md)).

## Breaking Changes

No breaking metadata contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `metadata`
- [~] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — high-cardinality test source is present; deployment-runbook coverage is not verified here
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — `tests/integration/test_metadata_store_soak.cpp` delivered (MD-SOAK-01..03); registered in Wave D foreach with TIMEOUT 120
- [?] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — verify deployment-runbook coverage and status before sign-off

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — partition inconsistency and index corruption paths tested in soak and stress suites
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027) — re-baseline in progress (Q1 2027); representative hardware sign-off pending
- [?] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — deployment evidence not reviewed for this update
