---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Vector Search Module Roadmap

## Current Status

- The module directory is a documentation boundary; vector-search runtime
  ownership is in `src/index/` and `include/index/`. There is no
  `include/vector_search/` directory in this checkout.
- The shared runtime includes `VectorIndexManager`, `MultiVectorSearch`,
  optional ANN/advanced backends, and an in-process distributed-index class.
- The module-specific stress/soak fixtures and dedicated gate benchmark use
  in-process flat-index implementations and do not establish production runtime
  performance or readiness. A separate benchmark in `benchmarks/ann/` calls
  `VectorIndexManager`, but the repository does not yet retain complete,
  repeatable quality/latency/resource evidence for release claims.
- **Maturity assessment:** substantial shared runtime and optional ANN paths,
  with useful persistence, reindexing, fusion, and focused index tests; evidence
  and contract gaps prevent a release-grade claim for ANN quality, supported
  backends, filtered/multi-field behavior, or distributed availability.
- This roadmap tracks documentation and evidence; it does not claim a
  maintainer release approval.

## Research Basis and Code Gap Analysis

The research points below guide evaluation methods and trade-offs; they do not
prove that a cited algorithm or performance result is implemented or supported
by ThemisDB.

| Research / technical finding | Current code and evidence | Gap and roadmap implication |
|---|---|---|
| HNSW uses a hierarchy of proximity graphs and trades memory/build work against search quality and latency; parameter sweeps need a quality measure as well as time. | `VectorIndexManager` exposes HNSW `M`, `efConstruction`, `efSearch`, and optional advanced indexes in `include/index/vector_index.h`. `benchmarks/ann/bench_vector_search.cpp` sweeps `efSearch`, but does not calculate recall against an exact oracle. | For v2.5, benchmark recall@k against exact search alongside p50/p95/p99, throughput, build time, and memory; do not promote existing flat-fixture counters as HNSW results. |
| DiskANN demonstrates an SSD-resident graph as a capacity/memory alternative, with outcomes dependent on the dataset, SSD, RAM, and hardware. | The API exposes optional DiskANN configuration, but module docs report that availability is build-dependent and unavailable support may fall back. The `DistributedVectorIndex` remains in-process, not an SSD-backed distributed service. | For v3, evaluate storage-aware alternatives with matched workloads and resource budgets before choosing a backend; do not infer scale or deployment support from the configuration enum. |
| ANN-Benchmarks emphasizes comparable recall/quality and performance measurements over documented datasets and common interfaces. | The dedicated `benchmarks/vector_search/` suite uses a synthetic flat index and exports threshold counters; `tests/vector_search/` uses in-process flat-index fixtures. | Preserve fixture-level tests, add or identify runtime-backed lifecycle/search tests, and archive benchmark manifests that include dataset, metric, index settings, backend, hardware, sample count, and measured percentiles. |
| Filtered ANN results vary with filter selectivity, predicate/data correlation, and strategy; a single unfiltered KNN result does not characterize filtered retrieval. | `VectorIndexManager` offers filtered search. The module-specific tests and dedicated gate benchmark do not cover filter selectivity/correlation, and `searchMultiField()` does not perform field-specific index routing. | Add a selectivity/correlation matrix and exact filtered baseline before changing filter routing or promising recall/latency behavior. Treat the 2026 filtered-ANN study as a preprint, not a normative SLO. |
| Embedding research shows vectors can leak information about inputs, attributes, and membership; vector data must be treated as potentially sensitive. | The manager exposes optional encryption and audit hooks, but this module does not supply authentication/authorization or prove encryption, key-management, or audit-retention deployment settings. | Keep authorization at the integrating service boundary; document data classification, key handling, retention, and audit evidence. Do not claim that encryption alone prevents embedding leakage. |

### Research References

1. Yu. A. Malkov and D. A. Yashunin, “Efficient and Robust Approximate
   Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs,”
   *IEEE Transactions on Pattern Analysis and Machine Intelligence*, 42(4),
   2020. [arXiv:1603.09320](https://arxiv.org/abs/1603.09320).
2. S. J. Subramanya et al., “DiskANN: Fast Accurate Billion-point Nearest
   Neighbor Search on a Single Node,” *NeurIPS*, 2019.
   [Microsoft Research](https://www.microsoft.com/en-us/research/publication/diskann-fast-accurate-billion-point-nearest-neighbor-search-on-a-single-node/).
3. M. Aumüller, E. Bernhardsson, and A. Faithfull, “ANN-Benchmarks: A
   benchmarking tool for approximate nearest neighbor algorithms,”
   *Information Systems*, 87, 2020.
   [Publisher record](https://www.sciencedirect.com/science/article/pii/S0306437918303685).
4. A. Amanbayev et al., “Filtered Approximate Nearest Neighbor Search in Vector
   Databases: System Design and Performance Analysis,” arXiv preprint
   arXiv:2602.11443, 2026.
   [arXiv](https://arxiv.org/abs/2602.11443). Findings are provisional until
   peer-reviewed or independently reproduced.
5. C. Song and A. Raghunathan, “Information Leakage in Embedding Models,”
   *ACM CCS*, 2020. [arXiv:2004.00053](https://arxiv.org/abs/2004.00053).

## Release Interpretation and Success KPIs

- **v2.5.0** is the next planned minor version in `VERSIONING.md`; this module
  plan is proposed work and neither commits a release date nor overrides root
  release gates.
- **v3.0.0** is a strategic, conditional planning horizon only. The current
  `VERSIONING.md` and `RELEASE_STRATEGY.md` do not establish a v3 release
  commitment, date, approved breaking API, or migration policy. The roadmap is
  not authorization to break compatibility.
- Report each performance result with dataset/corpus, metric, query count, `k`,
  filters/selectivity where applicable, index parameters, backend/build flags,
  hardware, sample count, and baseline commit. Report measured p50/p95/p99
  latency, throughput, recall@k, build/rebuild duration, and memory where
  measurable. Compare only like-for-like runs.
- The existing VS-BM thresholds remain documented targets, not verified
  runtime SLOs. Establish a production-runtime baseline first; do not claim a
  performance pass from static counters or synthetic fixtures.

## v2.5.0 Plan — Reliability and Evidence

Estimates are engineering person-weeks for one engineer familiar with the
shared index runtime. They exclude hardware/CI provisioning and release
approval; they are planning ranges, not schedule commitments.

### Wave 2.5-A — Input and API Contract Correctness (Priority: P0)

- **Goal:** make vector validation and multi-field/configuration behavior
  explicit and consistent before publishing performance claims.
- **Tasks:**
  - [ ] Apply and test dimension validation consistently across direct-commit,
    batch, and MVCC entity-write overloads (Target: v2.5.0).
  - [ ] Verify and test the policy for NaN/Inf vectors and search results,
    malformed vectors, invalid `k`, and dimension mismatch; fail with the
    existing status/result contracts (Target: v2.5.0).
  - [ ] Decide and document whether `searchMultiField()` performs actual
    field-specific routing; implement and test the selected contract, including
    `require_all_fields`, `index_name`, and `ef_search`, or narrow the API
    contract rather than leaving advertised options ambiguous (Target: v2.5.0).
- **Dependencies:** review of `include/index/vector_index.h` and
  `include/index/multi_vector_search.h`; API changes require normal compatibility
  review.
- **Risk / trade-off:** stricter validation can reject inputs accepted
  previously; implementing real field routing may require distinct index and
  schema contracts. Do not introduce undocumented compatibility paths.
- **Effort:** 2–4 person-weeks.
- **Success metrics:** every write overload rejects wrong dimensions; finite
  value behavior is covered by explicit tests; every documented configuration
  field has a tested effect or is no longer represented as active behavior.
- **Validation:** focused unit tests for each mutation overload, numeric edge
  cases, field routing/fusion and configuration; run the registered shared
  index tests and API compatibility review.

### Wave 2.5-B — Runtime-Backed Quality and Performance Baseline (Priority: P0)

- **Goal:** produce reproducible evidence for supported runtime paths, rather
  than interpreting fixture thresholds as production measurements.
- **Tasks:**
  - [ ] Add or identify RocksDB-backed `VectorIndexManager` tests for init,
    insert, exact/ANN search, filtered search, save/load, rebuild, and recovery
    (Target: v2.5.0).
  - [ ] Extend the runtime benchmark in `benchmarks/ann/bench_vector_search.cpp`
    or add a registered runtime-backed case for recall@k against an exact
    baseline, filtered workloads, insert/search mix, and rebuild (Target:
    v2.5.0).
  - [ ] Emit a benchmark manifest with all workload/backend/hardware/sample
    fields above; distinguish setup, build, and query time and calculate
    percentiles from measured samples (Target: v2.5.0).
  - [ ] Reconcile CMake/CTest registration and retain the focused run and
    benchmark artifacts; label unavailable hardware “not run,” never PASS
    (Target: v2.5.0).
- **Dependencies:** Wave 2.5-A contracts; deterministic dataset and exact-search
  oracle; test-enabled supported configure preset; benchmark target and CI
  artifact availability.
- **Risk / trade-off:** synthetic data may hide corpus skew; persistent storage
  setup can dominate timings; hardware/backend differences limit comparisons.
  Keep setup out of search timing and label synthetic results.
- **Effort:** 3–5 person-weeks.
- **Success metrics:** 100% of release-claimed benchmark cases emit the required
  manifest and measured percentiles; recall@k is reported against an exact
  oracle; each KPI maps to an executed named test or benchmark; performance
  regressions are compared to an approved same-host baseline (initially, no
  unexplained regression greater than 10%).
- **Validation:** run the focused vector-search CTest selection and shared
  index integration tests; repeat each benchmark at least three times with the
  same build/backend/host; retain logs and raw measurements. Hardware-specific
  results remain pending until run on representative devices.

### Wave 2.5-C — Security, Recovery, and Operational Readiness (Priority: P1)

- **Goal:** define verifiable failure and data-handling expectations for the
  current shared runtime without claiming distributed high availability.
- **Tasks:**
  - [ ] Test persistence reload, corrupt/missing index files, interrupted
  rebuild, storage errors, encryption setup errors, and resource exhaustion;
  document whether the prior usable index remains available (Target: v2.5.0).
  - [ ] Document the caller's authorization responsibility and deployment
  evidence for encryption/key management, audit retention, backup, and
  restore; keep vector payloads and secrets out of logs (Target: v2.5.0).
  - [ ] Exercise sustained insert/search and recovery under supported
  sanitizers; keep CPU/fixture evidence distinct from GPU or production
  backend coverage (Target: v2.5.0).
  - [ ] State explicitly that `DistributedVectorIndex` is in-process; do not
  advertise replication or multi-node availability without separate evidence
  (Target: v2.5.0).
- **Dependencies:** Waves 2.5-A and 2.5-B; storage recovery contract; available
  sanitizer and representative runtime environments.
- **Risk / trade-off:** fail-closed behavior or stronger validation can reduce
  availability; security evidence is deployment-specific and must not be
  inferred from API hooks.
- **Effort:** 2–4 person-weeks, plus environment and security review.
- **Success metrics:** every persistence/security failure scenario has an
  expected status and test; no test loses the last committed usable index
  without an explicit unavailable state; zero untriaged sanitizer findings;
  authorization, encryption, and audit boundaries are documented without
  claiming unsupported guarantees.
- **Validation:** storage-backed fault-injection tests, restart/recovery
  integration test, supported sanitizer/soak run, security review, and operator
  runbook review.

## v3.0.0 Strategic Plan — Conditional Architecture Evolution

All waves below require an explicitly approved v3 scope. Any public API,
serialized-format, or migration-breaking change requires a breaking-change
issue, compatibility review, migration guide, and changelog entry under
`VERSIONING.md`. No compatibility shim or distributed-service commitment is
approved by this roadmap.

### Wave 3.0-A — Backend Capability and Query Contract (Priority: P0, conditional)

- **Goal:** let routing and callers reason about backend capabilities rather
  than infer behavior from a strategy name or build flag.
- **Tasks:**
  - [ ] Specify a versioned capability contract for metrics, filtering,
  persistence, updates, resource limits, and supported query modes (Target:
  v3.0.0, pending approval).
  - [ ] Define observable route selection, fallback, partial-result, and
  unsupported-capability outcomes; design migration from the current shared
  manager APIs (Target: v3.0.0, pending approval).
  - [ ] Specify compatibility behavior for index metadata and public APIs before
  implementation (Target: v3.0.0, pending approval).
- **Dependencies:** v2.5.0 contract and benchmark evidence; approved API/ABI
  scope; maintainer and consumer review.
- **Risk / trade-off:** additional capability negotiation increases API
  complexity and may require incompatible changes; keep the design to
  demonstrated caller needs.
- **Effort:** 2–4 person-weeks for design and prototype; implementation estimate
  requires approved scope.
- **Success metrics:** 100% of supported routes are tied to tested advertised
  capabilities; unsupported operations fail predictably; migration fixtures
  have deterministic outcomes.
- **Validation:** API design review, conformance tests for each selected
  backend, public-header compatibility checks, and migration/recovery fixtures.

### Wave 3.0-B — Selectivity- and Resource-Aware Retrieval (Priority: P1, conditional)

- **Goal:** select in-memory, compressed, or storage-aware retrieval according
  to measured quality, latency, memory, and filter workload trade-offs.
- **Tasks:**
  - [ ] Benchmark HNSW and only build-supported alternatives (for example
  IVF-PQ or DiskANN) against the same exact oracle, corpus, filters, update
  pattern, memory/SSD budget, and hardware (Target: v3.0.0, pending approval).
  - [ ] Evaluate filtered search over low/medium/high selectivity and
  predicate/vector correlation; compare exact fallback and ANN recall/latency
  (Target: v3.0.0, pending approval).
  - [ ] Approve an architecture decision record before implementing new
  storage/quantization or automatic routing policies (Target: v3.0.0, pending
  approval).
- **Dependencies:** Waves 2.5-A/B; capability contract from Wave 3.0-A; dataset
  governance and representative storage/hardware access.
- **Risk / trade-off:** compression can reduce recall; SSD I/O can increase tail
  latency; filtered recall depends on predicate correlation. Avoid adopting a
  backend based on vendor claims or a single corpus.
- **Effort:** 4–8 person-weeks for evaluation and selected prototype, excluding
  hardware provisioning.
- **Success metrics:** decision has repeatable recall@k, p50/p95/p99, throughput,
  build/rebuild, and memory/SSD measurements for every selected workload; the
  selected path meets approved recall and latency budgets with no unexplained
  >10% regression against its matched baseline.
- **Validation:** ANN-Benchmarks-style comparison with exact ground truth,
  filtered selectivity/correlation matrix, repeated representative-hardware
  runs, and independent review of the decision record.

### Wave 3.0-C — Durable Distributed Search (Priority: P1, conditional)

- **Goal:** make a deliberate, evidence-backed choice between keeping the
  current in-process sharder and supporting durable multi-node search.
- **Tasks:**
  - [ ] Define node membership, partitioning, replica consistency, write
  acknowledgement, partial-result, timeout, rebalance, and recovery semantics
  before introducing a deployment contract (Target: v3.0.0, pending approval).
  - [ ] Prototype only after an ADR confirms a multi-node requirement; test
  restart, node loss, partition, rejoin, and data repair (Target: v3.0.0,
  pending approval).
  - [ ] Define persistence compatibility and migration/rollback for any
  distributed index metadata (Target: v3.0.0, pending approval).
- **Dependencies:** approved deployment use cases; Waves 3.0-A/B; storage,
  sharding, and transaction contracts; fault-injection infrastructure.
- **Risk / trade-off:** replication and consistency add write latency, storage
  overhead, and operational complexity. If the required availability cannot be
  demonstrated, retain the in-process implementation and explicitly exclude
  multi-node HA.
- **Effort:** 8–16 person-weeks for a narrowly scoped prototype; production
  estimate only after design and deployment requirements are approved.
- **Success metrics:** 100% of declared failure cases meet the approved
  consistency/availability contract; recovery loses no acknowledged writes
  within the specified failure model; search quality and p99 latency stay
  within approved budgets during rebalance and partial failure.
- **Validation:** deterministic fault-injection/chaos tests, restart and
  recovery integration suite, sustained multi-node load, consistency checks,
  recovery-time and data-loss measurements, and security/operations sign-off.

### Wave 3.0-D — Privacy, Governance, and Lifecycle (Priority: P1, conditional)

- **Goal:** make the vector data lifecycle and operational trust boundaries
  explicit for sensitive embeddings and index migrations.
- **Tasks:**
  - [ ] Define data classification, tenant isolation responsibility, retention
  and deletion behavior, encryption/key rotation, audit events, backup, and
  restore evidence with owning service teams (Target: v3.0.0, pending approval).
  - [ ] Assess embedding inversion/membership risks and document mitigations and
  residual leakage; do not claim that encrypted-at-rest vectors remove
  application-level or access-pattern risks (Target: v3.0.0, pending approval).
  - [ ] Version persisted index formats and specify validation, migration,
  rollback, and retention before a breaking format change (Target: v3.0.0,
  pending approval).
- **Dependencies:** security/privacy review, Wave 3.0-A capability contract,
  selected storage architecture, and release governance approval.
- **Risk / trade-off:** stronger privacy controls may increase compute/storage
  cost or reduce retrieval quality; obligations vary by deployment and
  jurisdiction.
- **Effort:** 3–6 person-weeks, excluding external audit and deployment-specific
  compliance approval.
- **Success metrics:** all documented lifecycle/security controls map to
  owner, configuration, and retained evidence; migration tests cover every
  supported prior format; no unresolved high-severity security finding remains
  for the approved release scope.
- **Validation:** threat-model review, access-control and key-rotation tests at
  the integrating service boundary, storage migration/recovery tests, audit
  evidence review, and human governance sign-off.

## Open Assumptions and Knowledge Gaps

- The issue scope names `include/vector_search/`, but this checkout has no such
  directory; canonical public interfaces are under `include/index/`.
- The existence of a benchmark source is not proof it is registered, executed,
  repeatable, or run with the intended production backend. The module-specific
  dedicated gate and stress fixtures are synthetic; the runtime benchmark in
  `benchmarks/ann/` needs retained execution evidence before release claims.
- Supported backends, build flags, representative deployment hardware, dataset
  license/representativeness, target corpus size, workload mix, and approved
  recall/latency/memory SLOs remain to be confirmed by maintainers.
- The filtered-ANN 2026 source is a preprint; treat its results as a hypothesis
  to test, not an implementation requirement or production guarantee.
- The scope and timing of v3.0.0, any breaking API or stored-format changes, and
  whether durable multi-node search is required are unapproved.

## In Progress

- [~] Replace synthetic dedicated-gate measurements with reproducible
  production-runtime benchmark evidence (Target: Q4 2026).
- [~] Record a supported-preset CTest run for the focused vector-search test
  selection (Target: Q4 2026).
- [~] Define the deployment and recovery contract for distributed indexing
  separately from the current in-process sharding implementation (Target: Q1 2027).
- [~] Resolve the direct-commit `addEntity()` dimension-validation gap and
  define actual multi-field routing and `SearchConfig` field behavior (Target: Q4 2026).

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Map `VectorIndexManager` and `MultiVectorSearch` to their canonical
  headers under `include/index/`.
- [x] Document the module-to-index ownership boundary and optional backend
  capability checks.

### Phase 2: Core Implementation

- [x] Use the existing shared index runtime; do not create a duplicate
  implementation under `src/vector_search/`.
- [x] Record that entity persistence/reindex APIs exist in the shared manager.
- [ ] Establish production-readiness evidence for the in-process distributed
  index before presenting it as a supported deployment capability.

### Phase 3: Error Handling & Edge Cases

- [x] Document initialization, dimension, storage, and optional-backend failure
  paths using the public `Status` contract.
- [ ] Verify and document numeric-value validation behavior from the actual
  runtime before making finite-value guarantees.
- [ ] Resolve the direct-commit `addEntity()` dimension-validation gap and
  define actual behavior for multi-field routing and currently unused
  `SearchConfig` fields.
- [ ] Document recovery behavior for persisted index load failures and
  encryption configuration failures.

### Phase 4: Tests

- [x] Identify the high-cardinality and soak fixtures as in-process flat-index
  tests, not production-runtime tests.
- [ ] Run the focused `vector_search` CTest selection from a configured
  repository-supported preset and attach the result to the issue.
- [ ] Add or identify runtime-backed tests for production index lifecycle,
  search, persistence, and advanced-backend behavior.
- [ ] Add focused tests for insertion-overload dimension checks, finite-value
  handling, and field-specific routing/configuration behavior.

### Phase 5: Performance / Hardening

- [ ] Benchmark the production `VectorIndexManager` path for insert and query
  workloads; report sample size, hardware, percentile calculation, and backend.
- [ ] Compare measured results with the documented VS-BM thresholds without
  treating static benchmark counters as measurements.
- [ ] Validate distributed-index failure, balancing, and recovery behavior
  before setting production targets.

### Phase 6: Documentation & Acceptance

- [x] Reconcile the module's primary docs and governance claims with the shared
  index implementation and current test fixtures.
- [x] Remove redundant secondary narratives that duplicated primary docs.
- [ ] Regenerate current module evidence and complete maintainer review.

## Production Readiness Checklist

- [x] Implementation ownership and public API locations are explicit.
- [x] Known differences between runtime code and synthetic test/benchmark
  fixtures are explicit.
- [x] Unsupported historical performance and readiness claims are removed.
- [ ] Focused vector-search CTest evidence is attached from a configured preset.
- [ ] Production-runtime benchmark evidence meets agreed release gates.
- [ ] Maintainer release sign-off is recorded.
- [ ] Direct-commit input validation and multi-field routing/configuration
  behavior are verified by source and focused tests.

## Known Issues & Limitations

- `src/vector_search/` contains no runtime source; behavior is owned by the
  shared index subsystem.
- Dedicated gate benchmark and named high-cardinality/soak fixtures are
  synthetic and cannot establish runtime performance or readiness.
- `DistributedVectorIndex` is currently an in-process implementation; its
  `RANGE` strategy is hash-based and replication is future work.
- `ai_working/sourcecode_graph.json` is absent in this checkout. The generated
  compliance report at `ai_context/developer_llm_wiki/SOLL_IST_GAP_REPORT.json`
  has a stale module-doc inventory.
- The direct-commit `addEntity()` overload does not check vector dimension;
  finite-value validation is unverified.
- `searchMultiField()` does not perform field-specific routing, and the
  `SearchConfig::index_name` and `ef_search` fields are unused.
- The `windows-release` configure preset disables tests in this checkout;
  focused tests require a test-enabled preset and configured CTest metadata.

## Breaking Changes

No runtime or public API change is made by this documentation update.
