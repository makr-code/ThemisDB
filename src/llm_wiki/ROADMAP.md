---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Module Roadmap

> Root [`ROADMAP.md`](../../ROADMAP.md), release strategy, and maintainer sign-off
> remain canonical for project release dates and readiness. The v2.5.0 and
> v3.0.0 targets below are proposed module delivery scopes, not release
> commitments. Current root roadmap version is 2.4.0-alpha.

## Current Status

**Maturity: prototype / pre-production.** This is a qualitative assessment of
the inspected code and declared evidence, not a standardized score or a claim
that a configured CI lane has passed.

| Area | Source-grounded status | Consequence |
|---|---|---|
| Ingestion and use cases | The public plugin contract exposes document and Wikipedia ingestion. The plugin currently stores placeholder content, and Wikipedia ingestion returns fixed counts (`MODULE_GAPS.md`, LW-GAP-01/03). | End-to-end source ingestion is not ready for user-facing claims. |
| Retrieval and quality | Plugin `query()` does not use a production retrieval index or honor all `WikiQueryOptions` (LW-GAP-02). Evaluation counters and a deterministic token-hash embedding helper exist, but are not evidence of retrieval quality. | Define a labeled corpus and connect the request path before making quality or latency claims. |
| Performance | The two module benchmarks exercise stubs or synthetic article strings (`PERFORMANCE_EXPECTATIONS.md`). The root RAG roadmap separately sets a Phase B target of at least 2x query throughput at 50K chunks and p95 below 100 ms. | The Phase B numbers are targets, not a measured baseline for this plugin. |
| Security and resilience | Pattern screening is heuristic; `fail_open` can continue flagged input. Edition selection is compile-time, workspace authorization is an integration responsibility, and the workspace checksum is FNV-1a despite its `sha256:` prefix (`SECURITY.md`, LW-GAP-04/06/09). | Do not represent these as authorization, prompt-injection prevention, or cryptographic integrity controls. |
| Governance and operations | Policy and knowledge-sphere artifacts exist, but not all declared behaviors are runtime-integrated (LW-GAP-05/08). Workspace persistence and recovery have limited guarantees. | Complete and test the supported contract before promoting lifecycle or compliance claims. |
| Tests and benchmarks | Focused tests are present in `tests/llm_wiki/`; CMake discovers them as focused targets. Their presence does not establish that they were configured or passed. Benchmark registration exists, but current workloads are not production-path measurements. | Release evidence must include the exact build, discovered tests, run results, and reproducible benchmark artifacts. |

### Research basis and design implications

The research supports measuring retrieval quality on diverse, held-out tasks
rather than optimizing a single corpus: BEIR reports that BM25 is a robust
baseline and that reranking/late-interaction systems may improve average
zero-shot effectiveness at higher computational cost. RAGAS separates retrieval
context quality, answer faithfulness, and answer quality; its reference-free
scores are useful diagnostics, not substitutes for human relevance judgments.
OWASP's 2025 LLM risks explicitly cover prompt injection, vector/embedding
weaknesses, and unbounded consumption; RAG alone is not a security boundary.
NIST AI 600-1 provides a voluntary risk-management profile for generative AI.
These sources inform the evaluation and control plan; none is evidence that
`llm_wiki` currently meets a benchmark, security standard, or legal obligation.
Full references are listed under [Research references](#research-references).

## Planned Features

### v2.5.0 — Complete and verifiable module behavior

- [ ] Implement bounded source ingestion with accurate partial-failure results; implement Wikipedia processing or explicitly disable the unsupported operation. (Target: v2.5.0)
- [ ] Connect queries to the intended retrieval index and honor top-k, minimum-score, workspace, and save-page options. (Target: v2.5.0)
- [ ] Add a fixed, versioned evaluation corpus with human relevance labels; report Recall@k/MRR separately from latency and automatic diagnostics. (Target: v2.5.0)
- [ ] Close critical workspace integrity, policy failure, edition-gate, authorization-boundary, and resource-limit gaps, with explicit integration responsibilities. (Target: v2.5.0)
- [ ] Demonstrate configured focused-test execution and reproducible production-path performance on representative hardware. (Target: v2.5.0)

### v3.0.0 — Governed, evolvable knowledge and retrieval

- [ ] Integrate knowledge spheres as a versioned runtime contract with provenance, access scope, lifecycle, deterministic migration, and rollback. (Target: v3.0.0)
- [ ] Compare lexical, dense, hybrid/fusion, and optional reranking strategies without changing corpus or relevance judgments between runs. (Target: v3.0.0)
- [ ] Enforce workspace/tenant boundaries at the integration seam; make lifecycle, backup, recovery, resource, and policy rollout behavior observable. (Target: v3.0.0)
- [ ] Require governance and compatibility evidence for persisted-schema or public-contract changes; do not imply regulatory compliance from technical controls alone. (Target: v3.0.0)

## Implementation Phases

Effort is a planning estimate in **person-weeks** for module engineering only.
It excludes external service/identity work, hardware procurement, and release
calendar time. Re-estimate after Wave 1. Waves are gates: a later wave must not
mask a failed earlier acceptance criterion.

### Phase 1 — Design / API Contract

- [ ] Define supported operations, ownership, failure semantics, and thread-safety; publish the versioned evaluation corpus and query set (Wave 1). (Target: v2.5.0)

### Phase 2 — Core Implementation

- [ ] Replace placeholder ingestion, define partial-write/idempotency behavior, and integrate or disable Wikipedia ingestion (Wave 2). (Target: v2.5.0)
- [ ] Connect option-aware plugin queries to the supported retrieval index and preserve evidence attribution (Wave 3). (Target: v2.5.0)

### Phase 3 — Error Handling & Edge Cases

- [ ] Enforce input/resource boundaries, policy reload behavior, workspace integrity, and the agreed authorization seam (Wave 4). (Target: v2.5.0)
- [ ] Define fail-closed validation and rollback for versioned knowledge-sphere migrations (Wave 6). (Target: v3.0.0)

### Phase 4 — Tests

- [ ] Run focused tests for ingestion, retrieval options, persistence, concurrency, and security failures in configured build lanes (Waves 1–4). (Target: v2.5.0)
- [ ] Add schema, lifecycle, migration, rollback, and mixed-version coverage (Wave 6). (Target: v3.0.0)

### Phase 5 — Performance / Hardening

- [ ] Establish production-path latency, throughput, memory, and relevance baselines, then compare retrieval strategies under the same workload (Waves 3 and 7). (Target: v3.0.0)
- [ ] Verify resource limits, recovery, isolation, and degraded-provider behavior before promotion (Waves 4, 7, and 8). (Target: v3.0.0)

### Phase 6 — Documentation & Acceptance

- [ ] Publish v2.5.0 evidence for configured tests, representative benchmarks, security review, and known exceptions (Wave 5). (Target: v2.5.0)
- [ ] Document v3.0.0 lifecycle, governance, operator procedures, and compatibility decisions and obtain maintainer sign-off (Wave 8). (Target: v3.0.0)

## Wave Plan

### v2.5.0 — Core behavior and release evidence

#### Wave 1 — Contract, baseline, and test discovery (P0; 2–3 person-weeks)

- **Goal:** Fix the supported v2.5 behavior and create a reproducible baseline.
- **Tasks:**
  - [ ] Reconcile `include/llm_wiki/llm_wiki_plugin_interface.h` with the implementation and define supported inputs, failure/status semantics, workspace ownership, and thread-safety. (Target: v2.5.0)
  - [ ] Confirm candidate use cases (documentation-grounded Q&A, gated public-corpus import, and workspace-scoped retrieval), then version their corpus/query sets and human relevance judgments. (Target: v2.5.0)
  - [ ] Record which focused tests are registered and run in the supported build. (Target: v2.5.0)
- **Dependencies:** None.
- **Risk / trade-off:** Narrowing unsupported API behavior may require a
  compatibility decision; synthetic results can be mistaken for product
  evidence.
- **Success measures:** Every public operation has an explicit supported or
  disabled status; corpus, query-set, build, and hardware revisions are
  recorded; all chosen tests appear in CTest and have retained results.
- **Validation:** API/compatibility review; focused CMake/CTest discovery and
  execution; baseline Recall@1/3/5/10, MRR, latency, throughput, memory, and
  failure counts on the fixed workload.

#### Wave 2 — Content ingestion and persistence (P0; 4–6 person-weeks)

- **Goal:** Ingest real source content, preserve provenance, and report
  deterministic outcomes.
- **Tasks:**
  - [ ] Replace placeholder ingestion with bounded parsing/chunking and validate empty, oversized, malformed, missing, and unreadable inputs. (Target: v2.5.0)
  - [ ] Define idempotency and partial-write behavior; implement gated Wikipedia processing or explicitly disable the unsupported operation. (Target: v2.5.0)
  - [ ] Reconcile workspace persistence format and migration behavior. (Target: v2.5.0)
- **Dependencies:** Wave 1 contract and fixture corpus.
- **Risk / trade-off:** Parsing and persistence add I/O and storage costs;
  backward-compatible handling of existing placeholder records is unresolved.
- **Success measures:** 100% of accepted test fixtures produce content-derived
  chunks and source provenance; malformed/denied fixtures return documented
  errors; retries do not silently duplicate records; no fixed-result ingestion
  path remains enabled.
- **Validation:** Unit and integration tests for valid/invalid input, filters,
  partial failures, retries, persistence restart, and gated Wikipedia fixtures;
  inspect stored records against the declared schema.

#### Wave 3 — Option-aware retrieval and quality (P0; 4–6 person-weeks)

- **Goal:** Make plugin queries retrieve real indexed content and measure
  relevance independently of latency.
- **Tasks:**
  - [ ] Connect the plugin to the supported index reader and honor top-k, minimum score, workspace selection, and save-page behavior. (Target: v2.5.0)
  - [ ] Establish lexical retrieval as a measured baseline before adding embedding or reranking complexity. (Target: v2.5.0)
  - [ ] Retain source/evidence attribution in query results. (Target: v2.5.0)
- **Dependencies:** Waves 1–2; a configured index/backend and fixed judgments.
- **Risk / trade-off:** Dense retrieval or reranking may improve some queries
  while increasing latency, memory, and provider dependencies.
- **Success measures:** All public query options pass behavioral tests;
  Recall@k and MRR are reported on the held-out set. For the existing root
  LWP-01..08 acceptance set, target Recall@k >= 0.80; root Phase B remains the
  separate target of >=2x query throughput over Phase A at 50K chunks and p95
  <100 ms on representative hardware. Neither target is considered met without
  production-path results.
- **Validation:** Focused retrieval tests, held-out relevance evaluation,
  same-corpus backend comparison, and representative-hardware p50/p95/p99,
  throughput, and memory benchmarks. Use RAGAS-like measures only as
  complementary diagnostics, with human labels as the quality gate.

#### Wave 4 — Security, resilience, and operations (P0; 3–5 person-weeks)

- **Goal:** Bound failure and resource behavior and make security claims match
  enforced controls.
- **Tasks:**
  - [ ] Remove or strictly constrain fail-open screening where rejection is required; establish request, document, concurrency, timeout, memory, and disk limits or document enforceable integration limits. (Target: v2.5.0)
  - [ ] Rename the FNV-1a checksum or replace it with a keyed integrity mechanism appropriate to the threat model; test corruption and specify policy reload/recovery. (Target: v2.5.0)
  - [ ] Specify edition-gating and authorization responsibilities and align the thread-safety contract with implementation. (Target: v2.5.0)
- **Dependencies:** Waves 1–2; Wave 3 query boundary for request budgets.
- **Risk / trade-off:** Fail-closed handling can reduce availability; stronger
  integrity checks and logging add cost. The plugin cannot itself provide
  caller identity or tenant routing without an agreed integration contract.
- **Success measures:** Zero unauthorized cross-workspace results in the
  defined test matrix; every configured resource limit is enforced or assigned
  to a named integration boundary; workspace integrity failures are detected
  within the declared threat model; failed reload preserves the last-known-good
  policy; concurrent API tests pass under sanitizers where available.
- **Validation:** Security review against OWASP LLM risks and NIST AI RMF
  categories; negative tests for policy, gate, authorization, injection-like
  corpus/query inputs, corruption, and exhaustion; crash/restart and
  concurrent-load tests; verify operational instructions against observed
  behavior.

#### Wave 5 — v2.5.0 acceptance and release packet (P0; 1–2 person-weeks)

- **Goal:** Make v2.5.0 module readiness reviewable and repeatable.
- **Tasks:**
  - [ ] Close or explicitly defer each P0 gap with an owner and evidence. (Target: v2.5.0)
  - [ ] Update API, security, operations, and performance docs from final behavior. (Target: v2.5.0)
  - [ ] Attach source revision, configured test results, benchmark artifacts, and maintainer review. (Target: v2.5.0)
- **Dependencies:** Waves 1–4.
- **Risk / trade-off:** Unavailable representative hardware or external
  authorization integration can defer the release gate; do not substitute
  synthetic measurements.
- **Success measures:** All v2.5.0 production-readiness checklist items below
  have evidence or a documented release-blocking exception; no release
  readiness is inferred from this roadmap alone.
- **Validation:** Fresh supported build/test lane, reproducible benchmark
  rerun, security and governance sign-off, and link/source consistency check.

### v3.0.0 — Versioned knowledge and scalable retrieval

#### Wave 6 — Knowledge-sphere lifecycle and migrations (P1; 6–8 person-weeks)

- **Goal:** Turn existing sphere schema/configuration artifacts into an
  explicit, versioned runtime contract.
- **Tasks:**
  - [ ] Define registry, descriptor, validator, and migrator interfaces; enforce canonical, derived, ingested, and protected boundaries. (Target: v3.0.0)
  - [ ] Persist owner, source-of-truth, schema version, access scope, retention, lifecycle, and migration target. (Target: v3.0.0)
  - [ ] Implement deterministic, idempotent migration and rollback. (Target: v3.0.0)
- **Dependencies:** v2.5.0 persistence and provenance contract (Waves 1–2);
  architecture review before public API or persisted-format changes.
- **Risk / trade-off:** A richer registry and dual-version migration increase
  storage and compatibility complexity; do not silently promote the current
  design-only artifacts into runtime policy.
- **Success measures:** 100% of persisted spheres validate against a versioned
  schema and have required provenance/ownership; migration is idempotent;
  failed migration retains readable last-known-good data; protected content
  cannot enter a public scope in the test matrix.
- **Validation:** Schema/contract tests; old-to-new and rollback integration
  tests; mixed-version reader/writer tests; access-isolation tests; audit
  review of lifecycle transitions.

#### Wave 7 — Retrieval strategy and scale (P1; 6–10 person-weeks)

- **Goal:** Select retrieval complexity using quality, latency, and cost
  evidence rather than assumptions.
- **Tasks:**
  - [ ] Compare lexical, dense, hybrid/fusion, and optional reranking strategies using identical corpus revisions and labels. (Target: v3.0.0)
  - [ ] Keep providers replaceable and report fallback/degradation explicitly. (Target: v3.0.0)
  - [ ] Revisit chunking, cache, and index configuration only with benchmark evidence. (Target: v3.0.0)
- **Dependencies:** v2.5.0 production retrieval and evaluation set (Wave 3);
  Wave 6 provenance/scope contract for filtered retrieval.
- **Risk / trade-off:** Reranking can improve quality at added compute cost;
  embedding/model changes can invalidate caches or reduce out-of-domain
  effectiveness.
- **Success measures:** The selected strategy has a reproducible Recall@k/MRR
  and p95/throughput/cost profile across the fixed and held-out domain sets;
  it meets root Phase B latency/throughput targets or has an explicit,
  maintainer-approved alternative. No quality gain is accepted if it violates
  the agreed resource budget or scope isolation.
- **Validation:** Heterogeneous held-out evaluation informed by BEIR;
  relevance judgments plus complementary faithfulness/context diagnostics;
  representative 50K-chunk and larger-scale runs where supported; cold/warm
  cache, provider unavailable, and degraded-mode tests.

#### Wave 8 — Governance, tenancy, and operational maturity (P1; 4–7 person-weeks)

- **Goal:** Make lifecycle, isolation, recovery, and change control
  observable across supported deployments.
- **Tasks:**
  - [ ] Define the plugin-to-service identity/authorization seam and add actor-attributed lifecycle/audit events where identity is available. (Target: v3.0.0)
  - [ ] Specify backup, restore, retention, policy rollout, and rollback procedures. (Target: v3.0.0)
  - [ ] Map controls to a selected governance framework without claiming certification or legal compliance. (Target: v3.0.0)
- **Dependencies:** Waves 4 and 6; integration owners for identity,
  retention, and audit sinks.
- **Risk / trade-off:** Stronger audit and isolation controls can add storage
  and latency; external identity and compliance obligations vary by deployment.
- **Success measures:** Every supported deployment has an owned identity and
  workspace boundary, a tested backup/restore procedure, auditable lifecycle
  changes, and documented SLO/resource limits; unresolved control gaps are
  visible in the release evidence packet.
- **Validation:** Cross-workspace and failover tests; policy rollback and
  recovery drills; access review; security/governance sign-off; operator
  runbook exercise.

## Production Readiness Checklist

- [ ] Public API behavior, ownership, failure semantics, and thread-safety agree with implementation.
- [ ] Ingestion processes real content, preserves provenance, bounds resources, and reports partial failures.
- [ ] Query uses production retrieval and honors all supported options.
- [ ] Wikipedia ingestion processes input behind the intended feature gate or is explicitly unsupported.
- [ ] Workspace persistence has tested integrity, migration, recovery, and concurrency guarantees.
- [ ] Authorization and tenant/workspace isolation are enforced at a documented and tested boundary.
- [ ] Focused tests are configured, discovered, and pass in supported build lanes.
- [ ] Production-path quality and performance results are reproducible and meet accepted targets on representative hardware.
- [ ] Security, operational, and governance reviews are recorded; no technical control is overstated as legal compliance.

## Known Issues & Limitations

See [`MODULE_GAPS.md`](MODULE_GAPS.md), [`SECURITY.md`](SECURITY.md), and
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) for source-verified
detail. At planning time, placeholder ingestion, non-indexed plugin queries,
fixed Wikipedia results, compile-time-only edition selection, heuristic
screening, non-cryptographic workspace checksum, incomplete policy/sphere
runtime integration, and synthetic-only module benchmarks remain material
gaps. No module-specific production benchmark result or release test result is
claimed here.

## Breaking Changes

- Public request/result structures, plugin lifecycle, thread-safety guarantees,
  and workspace/persisted schema changes require API/ABI and data migration
  review before v2.5.0.
- v3.0.0 is the proposed point to adopt versioned knowledge-sphere and
  migration contracts. No compatibility alias or silent legacy path is planned;
  any required transition must have an approved scope, owner, and sunset.

## Research References

1. N. Thakur et al., “BEIR: A Heterogeneous Benchmark for Zero-shot Evaluation
   of Information Retrieval Models,” NeurIPS 2021,
   [arXiv:2104.08663](https://arxiv.org/abs/2104.08663). Supports heterogeneous
   retrieval evaluation and cost/quality comparisons; it is not an `llm_wiki`
   benchmark result.
2. S. Es et al., “Ragas: Automated Evaluation of Retrieval Augmented
   Generation,” 2023, [arXiv:2309.15217](https://arxiv.org/abs/2309.15217).
   Supports separating retrieval context, faithfulness, and generation
   diagnostics; automatic metrics remain proxies.
3. OWASP GenAI Security Project, [Top 10 for LLM Applications 2025](https://genai.owasp.org/llm-top-10/).
   Relevant risks include prompt injection, vector/embedding weaknesses, and
   unbounded consumption; retrieval augmentation is not a substitute for
   authorization or input/resource controls.
4. NIST, [AI 600-1: Artificial Intelligence Risk Management Framework:
   Generative Artificial Intelligence Profile](https://doi.org/10.6028/NIST.AI.600-1),
   July 2024. Used as a voluntary risk-management reference, not a compliance
   certification.

## Open Assumptions and Knowledge Gaps

- The root roadmap's 50K-chunk p95/throughput targets are project targets for
  WikiIndexStore Phase B; a production benchmark must confirm whether the
  `llm_wiki` plugin path can use that implementation and workload.
- No fixed production corpus, query set, labeled relevance judgments, hardware
  baseline, or passing module release-test report is recorded in this plan.
- External identity, tenant routing, retention, and audit ownership are
  service/deployment responsibilities until an integration contract assigns
  them to the module.
- Schedule and person-week estimates are provisional and must be revised after
  Wave 1.
- Product-owner confirmation is needed for the prioritized user groups and
  canonical use cases before the evaluation corpus is frozen.
- Developer wiki health is stale relative to this task: `WIKI_STATUS.json` was
  generated on 2026-11-02 and reports 8,066 sources with 1,450 removals and 26
  changes. It does not contain a curated `llm_wiki` module context; this
  roadmap relies on current source and module-local documents instead.

> Exact model backend not verifiable from final document text alone.
