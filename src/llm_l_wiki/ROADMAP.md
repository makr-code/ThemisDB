---
Author: "ThemisDB Team"
Created: "2026-11-06"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.
> Project release status remains governed by the root [ROADMAP.md](../../ROADMAP.md); this plan does not assert that `llm_l_wiki` is implemented or release-ready.

# LLM-L-Wiki Module Roadmap

## Current Status

**Assessment: documentation/planning only; no source-verified runtime maturity.** In this checkout, `src/llm_l_wiki/wikipedia/wiki_workspace_orchestrator.cpp` is zero bytes; no module CMake target or public header was found, and `tests/TEST_DENSITY_MATRIX.md` records no module-owned tests or benchmarks. There is consequently no local retrieval, indexing, model, persistence, security, latency, or operational result to report. See the [implementation inventory](IMPLEMENTATION.md), [architecture boundary](ARCHITECTURE.md), [research record](RESEARCH.md), and [test-density record](../../tests/TEST_DENSITY_MATRIX.md).

`llm_l_wiki` is distinct from the implemented-but-separately-owned `src/llm_wiki/` module and its plugin. Their code, contracts, test results, and benchmarks are not evidence for this roadmap.

The root roadmap identifies v2.4.0-alpha as the current project version. The v2.5.0 and v3.0.0 entries below are proposed module targets, not release dates or a change to the project release plan. Estimated effort is person-weeks (PW) for one experienced engineer, with a ±50% planning range; it excludes shared infrastructure and delayed owner/data approvals.

## Planned Features

### v2.5.0 — establish a bounded, testable first implementation

- [ ] Approve the module owner, user workflows, scope, and boundary relative to `llm_wiki` and the private plugin before adding runtime behavior (Target: v2.5.0).
- [ ] Define source identity/version, workspace authorization, provenance, API/error semantics, data retention, and any model/index dependencies before implementation (Target: v2.5.0).
- [ ] Only after the scope gate, add a real build target and the smallest end-to-end implementation required by the approved workflows; do not register an empty translation unit as a feature (Target: v2.5.0).
- [ ] Establish a versioned, human-reviewed evaluation corpus and query set, then report retrieval quality separately from answer correctness and citation support (Target: v2.5.0).

### v3.0.0 — scale and production-operate only an accepted v2.5.0 capability

- [ ] Add idempotent incremental create/update/delete processing, explicit stale-data handling, and safe replay or rebuild behavior for the selected index/storage design (Target: v3.0.0).
- [ ] Demonstrate authorization at retrieval time, provenance-preserving answers, and resilience to malformed, conflicting, stale, and adversarial retrieved content (Target: v3.0.0).
- [ ] Set workload-derived SLOs and validate capacity, recovery, observability, and migration/rollback on the supported deployment profiles (Target: v3.0.0).
- [ ] Treat public API, persisted-format, and index-version changes as explicit compatibility decisions; do not add an unapproved legacy fallback or migration shim (Target: v3.0.0).

## Implementation Phases

### Phase 1 — Design / API Contract (Wave 1, v2.5.0)

- **Goal:** Resolve whether this path is an intended product module and record a sourceable contract before coding.
- **Tasks:**
  - [ ] Obtain maintainer approval for owner, personas/use cases, module boundary, source allowlist, and non-goals (Target: v2.5.0).
  - [ ] Decide whether retrieval, ranking, synthesis, and persistence belong here or to existing services (Target: v2.5.0).
  - [ ] Specify inputs, outputs, error/timeout/cancellation behavior, provenance fields, authorization, retention, and dependency failure semantics (Target: v2.5.0).
- **Dependencies:** Maintainer decision and representative data owners; no downstream phase may assume these decisions.
- **Effort:** 2–3 PW.
- **Risk:** An unapproved or overlapping scope can create duplicate APIs and confuse `llm_l_wiki` with `llm_wiki`.
- **Success measure / validation:** A reviewed contract answers all five open questions in [RESEARCH.md](RESEARCH.md); every proposed source and persisted field has an owner and access rule; unresolved decisions are explicitly blocking, not silently guessed.

### Phase 2 — Core Implementation (Wave 2, v2.5.0)

- **Goal:** Deliver only the owner-approved minimum workflow as real, build-integrated behavior.
- **Tasks:**
  - [ ] Add a non-empty implementation, build target, documented interface, and direct module test target for the approved workflow (Target: v2.5.0).
  - [ ] Preserve source IDs and revisions through ingestion and retrieval (Target: v2.5.0).
  - [ ] Enforce input bounds and workspace authorization before data enters a model context (Target: v2.5.0).
  - [ ] Expose explicit partial and dependency-failure outcomes (Target: v2.5.0).
- **Dependencies:** Wave 1 approval, agreed storage/index owner, and available supported build dependencies.
- **Effort:** 4–6 PW.
- **Risk:** Existing shared LLM/storage interfaces may not provide the selected guarantees; do not substitute another module’s implementation as a hidden fallback.
- **Success measure / validation:** The intended build discovers the target; all direct contract tests pass; create/query/update or the approved workflow is exercised end to end using the real implementation; no test result is inferred from `llm_wiki`.

### Phase 3 — Error Handling & Edge Cases (Wave 3, v2.5.0)

- **Goal:** Make incomplete, unsafe, or unavailable inputs fail explicitly and without leaking or corrupting workspace state.
- **Tasks:**
  - [ ] Test empty, malformed, oversized, duplicate, stale, and contradictory inputs (Target: v2.5.0).
  - [ ] Test dependency timeout/unavailability, partial writes, and cancellation if supported by the approved contract (Target: v2.5.0).
  - [ ] Test unauthorized access and cross-workspace isolation (Target: v2.5.0).
  - [ ] Preserve or roll back state consistently when updates fail (Target: v2.5.0).
- **Dependencies:** Wave 2 API, storage semantics, and threat-boundary decisions.
- **Effort:** 2–4 PW.
- **Risk:** Fail-open behavior can turn missing policy or provenance into unauthorized or unsupported output.
- **Success measure / validation:** Every contract-defined failure has a deterministic status and regression test; zero unauthorized reads/writes in the negative test suite; incomplete evidence is visibly marked or causes abstention according to the approved contract.

### Phase 4 — Tests (Wave 4, v2.5.0)

- **Goal:** Establish repeatable quality evidence before calling the initial capability usable.
- **Tasks:**
  - [ ] Check in an owner-approved, versioned query set with relevance judgments and a held-out split (Target: v2.5.0).
  - [ ] Report Recall@k/MRR separately from answer correctness and claim-level citation support (Target: v2.5.0).
  - [ ] Record corpus/version, configuration, model/index revisions, hardware, concurrency, and exact commands for each result (Target: v2.5.0).
  - [ ] Add deterministic, restart/replay, and regression tests for the approved workflow (Target: v2.5.0).
- **Dependencies:** Waves 1–3 and approved corpus handling/retention.
- **Effort:** 3–4 PW, excluding corpus labeling by domain experts.
- **Risk:** A small or leaked evaluation set can overstate quality; automatic citation metrics can miss semantic support errors.
- **Proposed acceptance measures (not existing results):** At least 100 human-reviewed queries with a held-out split; suggested initial gates are Recall@10 ≥0.80, ≥95% of answer claims supported by cited evidence, and zero unsupported critical claims in the held-out set. The module owner must confirm or replace these workload-dependent thresholds in Wave 1, before tuning. Validate citations by human review on a representative sample as well as automated checks.

### Phase 5 — Performance / Hardening (Wave 5, v3.0.0)

- **Goal:** Support evolving knowledge and representative scale without sacrificing retrieval quality or bounded resource use.
- **Tasks:**
  - [ ] Implement incremental, idempotent create/update/delete processing and replay (Target: v3.0.0).
  - [ ] Test index versioning, rebuild, migration, and rollback (Target: v3.0.0).
  - [ ] Benchmark ingestion and queries over declared corpus sizes and concurrency levels (Target: v3.0.0).
  - [ ] Measure p50/p95/p99 latency, throughput, memory, freshness, and retrieval quality together (Target: v3.0.0).
  - [ ] Set resource limits and backpressure from observed workloads (Target: v3.0.0).
- **Dependencies:** v2.5.0 acceptance, representative workload/hardware, and approved data lifecycle.
- **Effort:** 6–8 PW.
- **Risk:** Incremental state can become stale or inconsistent; batching/caching may improve latency while reducing freshness or retrieval quality.
- **Success measure / validation:** On the declared workload, updates are idempotent and replayable, deletes meet the approved freshness objective, memory stays within the configured cap, and performance/quality results are reproducible. Compare against the v2.5.0 baseline; do not claim a speedup without measurements.

### Phase 6 — Documentation & Acceptance (Wave 6, v3.0.0)

- **Goal:** Prove that the selected capability can be operated, audited, and safely upgraded.
- **Tasks:**
  - [ ] Threat-model prompt injection, poisoned/conflicting sources, data disclosure, and workspace isolation (Target: v3.0.0).
  - [ ] Run security, dependency-failure, and recovery tests on the supported deployment profile (Target: v3.0.0).
  - [ ] Define query and ingestion SLIs/SLOs, dashboards/alerts, incident steps, backup/recovery, and rollback (Target: v3.0.0).
  - [ ] Document API/storage/index compatibility and obtain maintainer sign-off (Target: v3.0.0).
- **Dependencies:** Stable v3.0.0 behavior and representative deployment profile from Wave 5.
- **Effort:** 4–6 PW, excluding external audit and deployment-specific operations work.
- **Risk:** Security controls and nominal SLOs are not evidence of resistance to unknown attacks or production availability.
- **Success measure / validation:** Zero cross-workspace disclosures in the defined adversarial suite; recovery/replay and rollback exercises meet owner-approved RPO/RTO; operational SLOs are measured over an agreed soak window; security, operations, test, and documentation reviews have recorded owners and results.

## Research-to-Plan Mapping

| Finding | Roadmap consequence | Evidence / limit |
|---|---|---|
| Citation presence is not equivalent to claim support; ALCE evaluates correctness and citation quality separately. | Wave 4 measures claim-level citation support and answer correctness separately; unsupported critical claims block acceptance. | Gao et al. (2023), “Enabling Large Language Models to Generate Text with Citations,” [ALCE / DOI](https://doi.org/10.18653/v1/2023.emnlp-main.398). Its reported benchmark outcomes are not ThemisDB targets or results. |
| RAG comparisons are difficult to reproduce when datasets, metrics, and configurations differ. | Wave 4 versions the corpus/query split, judgments, configuration, and commands; Wave 5 repeats this protocol for performance. | Rau et al. (2024), “BERGEN: A Benchmarking Library for Retrieval-Augmented Generation,” Findings of EMNLP 2024, [DOI](https://doi.org/10.18653/v1/2024.findings-emnlp.449). |
| Evolving knowledge motivates continual index maintenance rather than assuming a static index remains current. | Wave 5 tests versioned, idempotent updates, deletes, replay, freshness, and quality drift. | Saravanan et al. (2026), “AURORA: Neuro-Symbolic Continual Indexing for Evolving RAG Systems,” Findings of ACL 2026, [paper](https://aclanthology.org/2026.findings-acl.495/). Reported results are from that paper's setup only. |
| Retrieved content can contain adversarial or conflicting material; RAG does not itself establish safety. | Waves 3 and 6 test retrieved-content attacks, authorization, and fail-closed behavior across the end-to-end flow. | Liang et al. (2025), “SafeRAG: Benchmarking Security in Retrieval-Augmented Generation of Large Language Model,” ACL 2025, [DOI](https://doi.org/10.18653/v1/2025.acl-long.230); [OWASP Top 10 for LLM Applications](https://owasp.org/www-project-top-10-for-large-language-model-applications/). A test suite cannot establish immunity to unknown attacks. |
| Risk governance, provenance, and explicit resource authorization are lifecycle concerns. | Waves 1, 3, and 6 define provenance/access controls, auditability, and accountable risk acceptance before enablement. | [NIST AI 600-1](https://doi.org/10.6028/NIST.AI.600-1); [NIST SP 800-207](https://doi.org/10.6028/NIST.SP.800-207). |
| Operational targets should be expressed as indicators and service objectives and measured against actual use. | Waves 5–6 set workload-derived SLOs and validate them with repeatable load, recovery, and soak tests. | [Google SRE Book: Service Level Objectives](https://sre.google/sre-book/service-level-objectives/). It does not prescribe ThemisDB-specific thresholds. |

## Production Readiness Checklist

- [ ] Maintainer-approved scope, owner, supported workflows, and boundary from `llm_wiki`.
- [ ] Real target, documented interface, configuration/dependency contract, and non-empty implementation.
- [ ] Direct tests cover the supported contract, errors, authorization, lifecycle, and recovery.
- [ ] Versioned and held-out evaluation evidence reports retrieval, answer, and citation metrics separately.
- [ ] Threat model and negative security tests cover untrusted retrieved content and workspace isolation.
- [ ] Reproducible representative-load benchmarks and workload-derived resource limits/SLOs are recorded.
- [ ] Persistence/index migration, restart/replay, backup, and rollback are tested.
- [ ] Operator runbook, observability, data retention, audit trail, and maintainer acceptance are complete.

## Known Issues & Limitations

- No implementation, public API, module build target, direct tests, benchmark, corpus, or measured runtime behavior is currently evidenced in this path.
- Module ownership, user scenarios, corpus, authorization model, index/storage design, and supported model/provider are open assumptions. Wave 1 is a stop/go gate.
- Proposed numerical Wave 4 thresholds are hypotheses for owner review, not research-derived guarantees; no latency or scale threshold is asserted until representative hardware and workloads are named.
- The developer LLM wiki status snapshot is dated 2026-11-02 and has a large reported source delta (1,450 removals, 26 changes); its summaries are context only. Source and current test/build inventories remain authoritative. No curated `llm_l_wiki` page appears in the current developer-wiki module index.

## Breaking Changes

- No current public API or persisted format is established. Do not claim compatibility or add a compatibility shim before a supported contract exists.
- If v3.0.0 introduces an incompatible API, index, or persisted-format change, document the migration, validation, rollback, and versioning decision before release; never silently mix incompatible index/model versions.

## References

- [Implementation inventory](IMPLEMENTATION.md)
- [Architecture boundary](ARCHITECTURE.md)
- [API status](API.md)
- [Research and evaluation record](RESEARCH.md)
- [Embedding evaluation requirements](EMBEDDING.md)
- [Security boundary](SECURITY.md)
- [Trust and provenance boundary](TRUST.md)
- [Operations status](OPERATIONS.md)
- [Runbook](RUNBOOK.md)
- [Repository test-density matrix](../../tests/TEST_DENSITY_MATRIX.md)
- [Project roadmap](../../ROADMAP.md)
- [Versioning policy](../../VERSIONING.md)
