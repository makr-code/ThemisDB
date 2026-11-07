---
Author: "ThemisDB Contributors"
Created: "2026-07-06"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Related_Issue: "#7447"
Canonical_References:
  - "README.md"
  - "ARCHITECTURE.md"
  - "API.md"
  - "RESEARCH.md"
  - "FUTURE_ENHANCEMENTS.md"
  - "PERFORMANCE_EXPECTATIONS.md"
  - "CMakeLists.txt"
  - "include/lora_package.h"
  - "src/lora_package.cc"
  - "src/lora_package_config.cc"
  - "../../include/retrieval/retrieval.h"
  - "../../include/retrieval/dual_read_validator.h"
  - "dual_read_validator.cpp"
  - "../../tests/retrieval/CMakeLists.txt"
  - "../../tests/retrieval/README.md"
  - "../../benchmarks/retrieval/CMakeLists.txt"
  - "../../benchmarks/retrieval/README.md"
  - "../../ROADMAP.md"
  - "../../VERSIONING.md"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Module Roadmap

This is a module-level proposal for the requested v2.5.0 and v3.0.0 planning
targets, not a release commitment or production-readiness approval. Root
`ROADMAP.md`, `VERSION`, `RELEASE_STRATEGY.md`, and human release decisions
remain authoritative for release scope and status.

## Current Status

**Assessment as of 2026-11-07:** retrieval is a narrow artifact-governance
library plus an unconnected dual-read metric helper. It is not a production
vector, keyword, hybrid, reranking, embedding, or RAG retrieval engine. Overall
retrieval-engine maturity is **prototype / integration-incomplete**; the signed
LoRA artifact registry has implemented fail-closed checks, but lacks application
composition and operational ownership evidence.

| Area | Source-backed state | Evidence and limitation |
|---|---|---|
| Build/API boundary | `themis_retrieval` is a C++20 static library. Its CMake source list contains the LoRA package/configuration and RAG deployment-gate controller, not `dual_read_validator.cpp`. `include/retrieval/retrieval.h` is an empty umbrella header. | `src/retrieval/CMakeLists.txt`, `include/retrieval/retrieval.h` |
| Artifact lifecycle | Configured RocksDB storage validates manifest hashes, pinned Ed25519 signer IDs and signatures on production writes/imports and on reopen. Deployment requires separate quality, evidence, authorization, lifecycle and persisted-state integrations; there is no repository-backed application composition supplying them. | `src/retrieval/README.md`, `src/retrieval/SECURITY.md`, `src/retrieval/src/lora_package_config.cc`, `src/retrieval/src/lora_package.cc` |
| Dual-read validation | `DualReadValidator::ExecuteQuery()` returns an empty vector; validation therefore cannot compare live indexes. `ValidateQuery()` returns current metrics and result lists but no current-versus-previous delta. Its binary judgments and metric helpers are not a live index integration. | `include/retrieval/dual_read_validator.h`, `src/retrieval/dual_read_validator.cpp` |
| Tests | Five focused sources exist. The artifact tests exercise `themis_retrieval`; hybrid retrieval, shard routing/failover and trust-boundary tests use local mocks/stubs. The dual-read test intentionally asserts empty query results. | `tests/retrieval/CMakeLists.txt`, `tests/retrieval/README.md`, `tests/retrieval/test_dual_read_validator.cpp` |
| Benchmarks | The only benchmark exercises synthetic key hashing and CPU/GPU-labelled vector-summing stubs; it does not call this library, a production index, router, or GPU backend. | `benchmarks/retrieval/README.md`, `benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp` |

No production retrieval latency, throughput, quality, memory, or scaling
baseline is established. The module-specific research note leaves the real
query API, component ownership, workload, hardware, and retrieval-engine scope
unresolved; these remain blocking decisions rather than implicit assumptions.

The compiled Developer LLM Wiki status is dated 2026-11-02 and reports a large
source delta. This roadmap therefore uses the checked-in source and module
evidence above as primary evidence; refresh the wiki before relying on its
summaries.

## Research Basis and Applicability

Research supports measuring quality and cost together, against task-relevant
baselines; it does not establish a universally best index, fusion strategy, or
numeric SLO for ThemisDB.

| Finding | Roadmap implication | Limitation |
|---|---|---|
| BEIR evaluates heterogeneous retrieval tasks and reports ranking quality such as nDCG@10 alongside candidate recall. | Use representative query/workload slices, relevance judgments, a lexical baseline, nDCG@10 and Recall@100; publish per-slice results as well as aggregates. | Public benchmark results are not a substitute for ThemisDB workload judgments. |
| ANN-Benchmarks evaluates recall against query throughput and also records index build time and size. | Compare exact ground truth and ANN configurations on the same corpus, hardware and query set; retain latency/throughput and memory/index-size costs. | Its in-memory algorithm benchmark does not measure storage, concurrency, or full service behavior. |
| HNSW and ACORN describe graph ANN and filtered ANN approaches with workload-dependent trade-offs. | If ANN/filtering is approved, measure recall and latency across filter selectivities and query/filter correlations before selecting an approach. | Neither paper proves that its algorithm or reported results fit ThemisDB. |
| Reciprocal Rank Fusion (RRF) is a rank-based fusion method; BEIR reports quality/cost trade-offs for reranking. | Treat lexical, dense, fusion, and reranking as separately measured configurations; compare quality gains with added latency and resource use. | RRF is a baseline to test, not a promised quality improvement. |
| OWASP RAG guidance and the NIST Generative AI Profile identify retrieval permissions, data trust, provenance and lifecycle risk as governance concerns. | Carry authorization into retrieval, test tenant/permission boundaries and revocation, and keep retrieved text untrusted; record artifact and evaluation provenance. | These are security/risk references, not retrieval architecture specifications or certifications. |

## In Progress and Planned Features

All tasks below are proposed. Target labels use `TBD` because release dates,
feature ownership and acceptance thresholds have not been approved. The
person-week ranges are planning estimates, exclude upstream index/service work,
and must be recalibrated after owners and interfaces are confirmed.

### v2.5.0 — establish a safe, measurable current retrieval boundary

The v2.5.0 proposal closes the existing dual-read and evidence gaps before
claiming a usable retrieval runtime. It does not assume this module owns ANN,
search, RAG, embedding or shard execution.

#### Wave 1 — ownership, API contract, and measurement specification

- **Goal:** establish an owned query adapter, workload, metric definition and
  security boundary before implementation.
- **Tasks:**
  - [ ] Identify the owner and canonical API for current/previous index queries, plus the service owner for deployment authorization and key lifecycle (Target: TBD).
  - [ ] Record an ADR for module boundaries with `search`, `index`, `rag`, and deployment composition; define error, timeout, cancellation, top-k, score/rank and tenant-permission semantics (Target: TBD).
  - [ ] Approve a versioned query set, relevance judgments, data/license constraints, hardware profile, and quality/performance gates before benchmarking (Target: TBD).
- **Dependencies:** owners from the query/index and deployment-service integrations; no code/API assumption is approved yet.
- **Effort:** 1–2 person-weeks, excluding owner response and dataset creation.
- **Risks and trade-offs:** without an owner/API or lawful representative workload, implementation would create an orphan interface or misleading benchmark.
- **Success metrics:** every contract decision has an owner and source; 100% of benchmark cases have query, judgment, corpus/version and permission provenance; all release thresholds are written before runs begin.
- **Validation:** API/ADR review; dataset permission and tenant-boundary review; schema validation of the benchmark manifest; review against root release governance.

#### Wave 2 — live dual-read adapter and metric correctness

- **Goal:** make the existing validator query real current and previous indexes through the approved owner interface, and return meaningful comparisons.
- **Tasks:**
  - [ ] Replace the empty `ExecuteQuery()` behavior with the approved adapter; return explicit errors for unavailable, timed-out, unauthorized or malformed results rather than successful empty results (Target: TBD).
  - [ ] Return per-query current/previous metric values and deltas, preserve the caller's `top_k`, and document empty judgment and result semantics (Target: TBD).
  - [ ] Add deterministic adapter integration tests for result ordering, exact-match fixtures, empty and missing judgments, duplicate IDs, timeout/error, and index-version mismatch (Target: TBD).
- **Dependencies:** Wave 1 API, owner, and metric contract; real test index/provider.
- **Effort:** 3–6 person-weeks, depending on adapter availability.
- **Risks and trade-offs:** query fan-out can add load and tail latency; production dual reads must have bounded timeouts, cancellation, sampling and an explicit policy for a failed side.
- **Success metrics:** 100% of deterministic integration fixtures return the expected IDs and current/previous deltas; no path reports a successful comparison when either query execution failed; `top_k` is honored for all tested values.
- **Validation:** focused unit tests for metric edge cases; integration tests against the owning index API; timeout/failure injection; ThreadSanitizer for concurrent validation if shared state remains.

#### Wave 3 — representative retrieval quality and performance baseline

- **Goal:** produce the first valid baseline for any approved live retrieval path; retire synthetic routing measurements as retrieval-performance evidence.
- **Tasks:**
  - [ ] Add a production-path benchmark using the approved corpus/query set and exact-search oracle; report Recall@K, nDCG@10, MRR@10, p50/p95/p99 latency, throughput, memory and index-build/storage size (Target: TBD).
  - [ ] Record code revision, corpus and query-set hashes, judgment version, hardware, configuration, warm/cold state and repeated-run dispersion with every result (Target: TBD).
  - [ ] Ratify a quality non-regression threshold and service latency/resource SLO with the owning service before using results as a release gate (Target: TBD).
- **Dependencies:** Wave 1 workload and SLO owners; Wave 2 executable query integration.
- **Effort:** 2–4 person-weeks, excluding representative hardware and dataset access.
- **Risks and trade-offs:** benchmark improvements may be workload-specific; relevance quality can regress despite lower latency, and tail latency can worsen while means improve.
- **Success metrics:** all listed metrics are reported for each workload slice; at least five independent benchmark runs include a dispersion statistic; no release claim is made without owner-approved thresholds and reproducible artifacts. A proposed initial non-regression guardrail is no more than a 2 percentage-point loss in agreed ranking metrics and no more than 10% p95/p99 latency regression against the approved baseline; both values require Wave 1 owner ratification.
- **Validation:** benchmark smoke test in CI; full benchmark on recorded representative hardware; compare exact-vs-candidate recall and quality/cost curves; archive raw results and environment manifest.

#### Wave 4 — artifact trust, authorization, and operational readiness

- **Goal:** connect existing signed-artifact checks to service-owned authorization, audit and key lifecycle controls without conflating quality approval with authorization.
- **Tasks:**
  - [ ] Specify and integrate service-owned key provisioning, rotation/revocation, deployment authorization and auditable decision IDs; keep private key material out of retrieval logs and manifests (Target: TBD).
  - [ ] Test permission propagation and revocation at query time, cross-tenant isolation, malicious/instruction-bearing retrieved content handling, corrupt artifact reopen and unavailable authorization (Target: TBD).
  - [ ] Publish operator recovery, rollback, timeout and diagnostic procedures only after exercising the corresponding runtime paths (Target: TBD).
- **Dependencies:** deployment-service and security owners; Waves 1–2.
- **Effort:** 2–4 person-weeks plus service/security review.
- **Risks and trade-offs:** stale permission metadata or incomplete composition wiring could expose data; strict fail-closed behavior may reduce availability during authorization outages.
- **Success metrics:** zero unauthorized or cross-tenant results in the negative integration suite; 100% of missing, revoked, invalid-signature and denied-authorization cases fail closed; every deployment decision has a correlated audit record with no secret material.
- **Validation:** adversarial authorization and tenant tests; invalid/revoked key tests; fault injection for unavailable dependencies; security review and runbook exercise.

### v3.0.0 — strategic retrieval evolution, conditional on v2.5 evidence

These are strategic candidates, not current module capabilities or approved architecture. Proceed only through owner-approved ADRs; a capability may remain in `search`, `index`, or `rag` rather than move into `retrieval`.

#### Wave 5 — scalable index and filtered retrieval contract

- **Goal:** decide and validate the scaling boundary for approved ANN/filtered retrieval workloads.
- **Tasks:**
  - [ ] Select an owner and index interface through an ADR; compare exact and candidate ANN methods against an exact ground-truth oracle at approved corpus sizes, dimensions and concurrency (Target: TBD).
  - [ ] If metadata filtering is in scope, benchmark recall, p95/p99 and memory at each approved filter selectivity and query/filter correlation; test tenant predicates before and after candidate generation (Target: TBD).
  - [ ] Specify bounded resource limits, cancellation, overload behavior, index rebuild/restore and shard-loss semantics before production integration (Target: TBD).
- **Dependencies:** v2.5 Waves 1–4 accepted; index/sharding owners and representative data/hardware.
- **Effort:** 4–8 person-weeks, excluding index-engine development or distributed service changes.
- **Risks and trade-offs:** ANN may lose recall; filtering can change both result quality and latency; larger indexes can trade memory/build cost for query throughput.
- **Success metrics:** approved data-size and concurrency matrix is fully measured; each point reports recall@K, p95/p99, throughput and memory; all values meet owner-approved SLOs with zero cross-tenant results.
- **Validation:** exact-vs-ANN recall tests; filter-selectivity matrix; load and shard-failure tests; representative-hardware benchmark; index rebuild/restore drill.

#### Wave 6 — hybrid retrieval and reranking evaluation

- **Goal:** determine, rather than presume, whether lexical+dense candidate fusion or reranking improves target workloads enough to justify cost.
- **Tasks:**
  - [ ] Benchmark lexical and dense retrieval independently, then compare RRF and any approved reranker on the same judged query sets (Target: TBD).
  - [ ] Include heterogeneous task/domain slices and report nDCG@10, Recall@100, MRR@10, p95/p99 latency, throughput and compute/memory overhead (Target: TBD).
  - [ ] Adopt a fusion/reranking path only if quality thresholds and per-query latency/resource budgets pass owner-approved gates (Target: TBD).
- **Dependencies:** v2.5 quality set and baseline; Wave 5 query/index ownership; relevant search/RAG owners.
- **Effort:** 4–8 person-weeks, excluding model training/hosting.
- **Risks and trade-offs:** fusion scores may be poorly calibrated; reranking can improve ranking quality at the expense of latency and compute; aggregate gains may hide task regressions.
- **Success metrics:** every candidate is compared with both component baselines; aggregate and per-slice quality meet pre-registered non-regression gates; added p95/p99 and compute remain within the approved budget.
- **Validation:** offline benchmark on held-out judgments; paired per-query analysis; ablation of each retrieval leg; online/canary test only after privacy and authorization review.

#### Wave 7 — distributed operation and release-grade governance

- **Goal:** prove operational behavior for whichever v3 retrieval architecture is accepted, without treating mocks or synthetic gates as runtime evidence.
- **Tasks:**
  - [ ] Exercise production-backed shard failure, recovery, overload, timeout and rollback behavior with bounded retries and observable outcomes (Target: TBD).
  - [ ] Define retention/access controls for query, judgment and retrieval telemetry; keep sensitive query/document content out of default logs (Target: TBD).
  - [ ] Reconcile the module evidence with root `ROADMAP.md` and `MODULE_INDEX.md`, refresh representative-hardware evidence and obtain release-owner sign-off (Target: TBD).
- **Dependencies:** Waves 5–6 and production composition; operations, privacy and release owners.
- **Effort:** 3–6 person-weeks plus scheduled soak/release review.
- **Risks and trade-offs:** telemetry may leak sensitive data; failover can change ranking consistency; long-duration evidence is hardware- and workload-specific.
- **Success metrics:** all defined failure scenarios meet recovery/availability SLOs; no unauthorized results in the soak/fault matrix; zero unresolved critical/high security findings; all module performance claims link to current reproducible artifacts.
- **Validation:** integration chaos/fault suite; privacy/security review; soak test with resource and tail-latency monitoring; `release_critical` CI and release governance review.

## Implementation Phases

The six phases below apply to the v2.5.0 foundation and, after its gate, to the v3.0.0 candidates. Each phase is evidence-gated; phase completion does not by itself approve a release.

### Phase 1 — Design / API Contract
- [ ] Complete Wave 1 ownership, query contract, dataset provenance and acceptance-gate decisions before adding a runtime adapter (Target: TBD).

### Phase 2 — Core Implementation
- [ ] Implement only the approved live dual-read adapter and explicit comparison result contract from Wave 2 (Target: TBD).
- [ ] Implement a v3 index/filter/fusion capability only under a separate owner-approved ADR (Target: TBD).

### Phase 3 — Error Handling & Edge Cases
- [ ] Define and test empty judgments, duplicate results, invalid top-k, query errors, deadlines, cancellation, authorization failure and partial dual-read behavior (Target: TBD).

### Phase 4 — Tests
- [ ] Replace mock-only acceptance claims with focused integration tests against the owning API and deterministic exact ground truth (Target: TBD).
- [ ] Add tenant isolation, permission revocation, malformed artifact, concurrency and failure-injection coverage for each shipped path (Target: TBD).

### Phase 5 — Performance / Hardening
- [ ] Establish reproducible quality, p50/p95/p99, throughput, index-size and memory baselines before setting service targets; enforce only owner-approved thresholds (Target: TBD).
- [ ] Run representative hardware, load, filtered-search and recovery tests for the capabilities actually implemented (Target: TBD).

### Phase 6 — Documentation & Acceptance
- [ ] Synchronize API, security, operations, test and benchmark documentation to measured source behavior; reconcile root roadmap/index claims before release-status promotion (Target: TBD).
- [ ] Archive benchmark manifests, test/CI evidence, risks, owners and release approval in the canonical evidence location (Target: TBD).

## Production Readiness Checklist

- [ ] Owning query/index and deployment-service interfaces are approved and composed; no placeholder executor is counted as a production path (Target: TBD).
- [ ] Quality thresholds, workload, service SLOs, hardware and dataset provenance are approved and reproducibly measured (Target: TBD).
- [ ] Query-time authorization, tenant isolation, key lifecycle, audit and rollback controls pass integration/security checks (Target: TBD).
- [ ] Production-backed benchmarks and failure/soak results are archived; synthetic/mock tests are labelled as such (Target: TBD).
- [ ] Root roadmap, module index and release owners agree on implementation and performance status (Target: TBD).

## Known Issues & Limitations

- There is no live retrieval query implementation in this module; the dual-read executor returns empty results.
- The public umbrella header does not currently expose a retrieval API. Existing hybrid and shard tests do not exercise production implementations.
- Existing benchmark names and historical phase labels are not evidence of retrieval-engine performance, GPU execution, multi-shard routing, soak coverage, or production readiness.
- The checked-in retrieval YAML has no pinned keys; production artifact-store initialization requires administrator-provisioned pins. Key provisioning/rotation, owner/RBAC policy and application callback composition are outside this module today.
- No workload owner, relevance judgments, supported index contract, representative hardware, service SLO or approved release thresholds are identified in the module sources.
- Root `ROADMAP.md` currently contains a source-inventory row reporting retrieval as zero LOC/tests, while `MODULE_INDEX.md` claims p95/p99 and memory gates are locked. These claims conflict with the inspected module source and `PERFORMANCE_EXPECTATIONS.md`; this module roadmap does not silently promote or reconcile root release status. Reconcile those aggregates before using them as release evidence.
- The Developer LLM Wiki freshness/delta signal needs refresh; module sources are authoritative for this plan.

## Breaking Changes

- No API or compatibility change is approved by this roadmap.
- A production query adapter will require an owner-approved public contract. Any incompatible API change, new retrieval ownership boundary, or v3 index contract must be reviewed and versioned under root release governance before implementation.

## Success Metrics and Validation Summary

| Dimension | Required measure | Gate |
|---|---|---|
| Retrieval quality | Recall@K, nDCG@10, MRR@10; per workload/domain/filter slice | Exact-oracle comparison; thresholds fixed before evaluation. The existing 2pp helper default is not an approved service SLO. |
| Performance and scale | p50/p95/p99 latency, throughput, memory, index size/build time | Same query set, hardware and configuration; publish repeated-run dispersion and compare with approved SLO/baseline. |
| Reliability | Error/timeout/cancel behavior, bounded retries, recovery time | Failure and overload tests pass without reporting failed reads as successful empty comparisons. |
| Security and governance | Unauthorized-result count, permission-revocation result, audit coverage, provenance completeness | Zero cross-tenant/unauthorized results; all denied or invalid states fail closed; evidence is traceable. |
| Operational readiness | Reproducible benchmark/test artifact, runbook, rollback and owner sign-off | Required evidence is current and linked before any release-readiness claim. |

## References

1. N. Thakur et al., “BEIR: A Heterogeneous Benchmark for Zero-shot Evaluation of Information Retrieval Models,” 2021. [arXiv:2104.08663](https://arxiv.org/abs/2104.08663). Supports diverse task evaluation, ranking metrics and retrieval/reranking quality-cost comparisons.
2. M. Aumüller, E. Bernhardsson, A. Faithfull, “ANN-Benchmarks: A Benchmarking Tool for Approximate Nearest Neighbor Algorithms,” 2018. [arXiv:1807.05614](https://arxiv.org/abs/1807.05614). Supports recall/throughput trade-off measurement and recording index build/size.
3. Y. Malkov and D. Yashunin, “Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs,” 2016. [arXiv:1603.09320](https://arxiv.org/abs/1603.09320). Foundational ANN method; does not establish a universally best index.
4. G. S. M. Cormack, C. L. A. Clarke, S. Büttcher, “Reciprocal Rank Fusion outperforms Condorcet and individual Rank Learning Methods,” SIGIR 2009. [DOI:10.1145/1571941.1572114](https://doi.org/10.1145/1571941.1572114). Candidate rank-fusion baseline, not a guaranteed local quality gain.
5. G. Patel et al., “ACORN: Performant and Predicate-Agnostic Search Over Vector Embeddings and Structured Data,” SIGMOD 2024. [DOI:10.1145/3654923](https://doi.org/10.1145/3654923), [preprint](https://arxiv.org/abs/2403.04871). Motivates filtered ANN/selectivity experiments; reported results are method- and setup-specific.
6. OWASP, [RAG Security Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/RAG_Security_Cheat_Sheet.html). Operational security guidance for permission propagation, retrieved-content trust and RAG attack surfaces.
7. NIST, [Artificial Intelligence Risk Management Framework: Generative Artificial Intelligence Profile, AI 600-1](https://doi.org/10.6028/NIST.AI.600-1). Broad risk-management context, not a retrieval architecture specification.
