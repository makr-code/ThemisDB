# ThemisDB — Open Stub Replacement Matrix

<!-- Status: current | generated: 2026-07-20 | source: src/*/FUTURE_ENHANCEMENTS.md + src/ROADMAP.md -->
<!-- Primary (Quelle der Wahrheit): src/ROADMAP.md -->
<!-- Datum: 2026-07-20 -->

> **Purpose:** This document is the canonical "Open Stub Replacement Matrix" for ThemisDB.
> It consolidates every open stub, mock, and documented simulation path across all source modules,
> extracted from `src/*/FUTURE_ENHANCEMENTS.md` and sorted by release risk.
>
> **Scope:** `src/` and `include/` modules. Plugin stubs in `plugins/` follow the same pattern
> but are tracked separately in each plugin's `FUTURE_ENHANCEMENTS.md`.
>
> **Filename convention:** `.MD` (uppercase) for root-level strategy documents.
> Module-level files use `FUTURE_ENHANCEMENTS.md` (uppercase name, lowercase extension).

---

## Root Governance Role

- This file is the canonical root backlog for **open enhancements** and **stub replacements**.
- Terminology is aligned with `ROADMAP.md`: shipped/in-progress work is treated as a roadmap **feature**, while open backlog work remains an **enhancement** here.
- `CHANGELOG.md` should reference roadmap milestones and enhancement items from this file when work is completed.
- `VERSIONING.md` and `RELEASE_STRATEGY.md` provide the release-type and milestone/tag mapping that completed enhancement entries must follow.
- `COPILOT_INSTRUCTIONS.md` defines mandatory AI/agent synchronization rules for these root governance documents.
- `FEATURE_ENHANCEMENT.md` remains a generated maturity report and is not used as planning source-of-truth.

## Root Documentation Synchronization (2026-07-27)

- Root-level markdown set synchronized to the current `src/` implementation and root governance state.
- `ROADMAP.md` remains the canonical source-of-truth for release-readiness checkbox closure; this file tracks open enhancement backlog and execution constraints.
- Current root sync references the active GA baseline instead of the older wire-only snapshot:
	- `benchmarks/wave7/release_gate_manifest_w7.json`
	- `tests/integration/WAVE5_TEST_COVERAGE.md`
	- `tests/integration/WAVE6_TEST_COVERAGE.md`
	- `src/auth/ROADMAP.md`
- Completed auth v1.2.0 / v1.3.0 delivery remains tracked in `src/auth/ROADMAP.md`; this file continues to track only open enhancement backlog.
- Changelog trace entry added in `CHANGELOG.md` under `Unreleased`.
- **2026-07-27 next-phase sync:** `research/implementation_influence/by_module.md` enhanced for top-risk modules (server, llm, sharding) with five-column research-source → planned-capability → implementation-evidence mapping. `ROADMAP.md` next-phase Tracks 0–6 structure updated. `src/query/ROADMAP.md` AQL Mutations status synced (Phases 1–5 marked complete).
- Promotion remains blocked until open roadmap Phase 2/3/5/6 items, Batch D checklist closure, and Section 9 human sign-off in `docs/governance/GA_PROMOTION_SIGN_OFF.md` are complete.

---

## private-plugin-externalization

### Scope
- Establish the public/private plugin split for plugin-name-aligned optional submodules under `plugins/private/` without removing public reference plugins from the monorepo.
- Cover Wave-1 candidates (`ethics_ai`, `user_storage_encrypted`, connector pack) and define the refactor-first boundary for static AI/acceleration modules.

### Design Constraints
- Community and Minimal checkouts must configure, build, and test without private credentials, private sources, or private artefacts.
- Private plugin repositories use commit-pinned submodules and canonical lane names (`develop`, `enterprise`, `hyperscaler`, `military`) only.
- No new legacy or compatibility path may bypass the public plugin SDK or the existing fail-closed license posture.

### Required Interfaces
- `include/plugins/plugin_interface.h` and `include/plugins/manifest_schema_v2.json`
- `src/plugins/plugin_manager.cpp` and related plugin-loading/runtime-license helpers
- `plugins/CMakeLists.txt`, `cmake/features/PluginFeatures.cmake`, `cmake/features/PrivatePluginFeatures.cmake`, `cmake/PrivatePlugins.cmake`
- `.gitmodules`, private-release workflows, `ROADMAP.md`, `RELEASE_STRATEGY.md`, and `VERSIONING.md`

### Implementation Notes
- Keep `WITH_PRIVATE_*` defaults at `OFF`, but align repository names and `plugins/private/*` paths with the current plugin names wherever possible.
- Wave-1 private repositories provisioned (2026-07) with aggregate layout:
  - `makr-code/themisdb_ethic_ai` → `plugins/themisdb_ethic_ai/` (ethics_ai plugin root)
  - `makr-code/themisdb_storage` → `plugins/themisdb_storage/` (subdirs: `user_storage_encrypted/`, `azure_blob_storage/`, `s3_blob_storage/`)
  - `makr-code/themisdb_importer` → `plugins/themisdb_importer/` (subdirs: `mysql_importer/`, `mongo_importer/`, `kafka_importer/`, `s3_importer/`)
  - `makr-code/themisdb_llm_wiki` → `plugins/themisdb_llm_wiki/` (LLM Wiki tool)
- CMake paths in `cmake/PrivatePlugins.cmake` updated to use aggregate repo subdirectory structure.
- Commit-pin hashes for all submodule entries pending after initial content push to the private repos.
- Keep `src/ethics_ai/ethics_evaluator.{h,cpp}` and `include/ethics_ai/ethics_ai_types.h` as temporary public core shims until CAI/LLM seams are fully decoupled.
- Keep benchmark split work explicit: extract private connector scenarios from `benchmarks/bench_importer_throughput.cpp` and validate whether `benchmarks/bench_blob_zstd.cpp` must be split before full connector externalization.
- Defer plugin-named acceleration and regulated-intelligence private repos (for example `gpu-impact-analysis`) to Wave-2+ after SDK/ABI seam hardening.
- Extend manifests with visibility, edition-allowance, license-feature, and core-compatibility metadata while keeping absent fields backward-compatible.
- Preserve open reference plugins (for example PostgreSQL importer, JSONL exporter, HuggingFace ingestion, ONNX CLIP) in the public tree.

### Test Strategy
- Add manifest-schema validation coverage for new compatibility fields and edition lists.
- Add PR/path-policy checks for `plugins/private/**`, `.gitmodules`, private CMake, packaging, SBOM, and license workflow changes.
- Keep Community negative-path coverage for missing private submodules, missing licenses, wrong edition, and private-artifact leakage.

### Performance Targets
- No configure-time hard failure when private sources are absent and all `WITH_PRIVATE_*` toggles remain `OFF`.
- No regression in public plugin discovery/load behaviour for manifests that omit the new private-plugin fields.

### Security / Reliability
- Fail closed on disallowed editions, missing license features, invalid hashes, and incompatible private manifests.
- Enforce source-leakage, artifact-leakage, SBOM, and license-compliance gates before Community release publication.

---

## llm_wiki (Enterprise Plugin — `themisdb_llm_wiki`)

> Full spec: [`plugins/themisdb_llm_wiki/FUTURE_ENHANCEMENTS.md`](plugins/themisdb_llm_wiki/FUTURE_ENHANCEMENTS.md)
> Public SDK: [`include/llm_wiki/llm_wiki_plugin_interface.h`](include/llm_wiki/llm_wiki_plugin_interface.h)

### Scope

- Implement the private plugin shared library (`themisdb_llm_wiki_cpp`) behind `ILLMWikiPlugin`.
- Activate WikiIndexStore Phase B (RocksDB-native hybrid retrieval: BM25 + HNSW + RRF).
- Port the Python MVP workspace orchestrator to C++.
- Wire Wikipedia XML dump ingestion through the plugin ABI.
- Introduce RBAC-aware multi-tenant wiki namespaces.
- Structured quality evaluation: Recall@k, MRR, p95 latency reporting.

### Design Constraints

- `ILLMWikiPlugin` ABI is the only public surface; all implementation details stay in the private repo.
- Phase A `JsonWikiIndexReader` must remain a fallback when `rocksdb_dir` is not set.
- Community and Minimal builds must compile cleanly without the plugin present; loader degrades gracefully.
- Embedding providers remain swappable at config time (hash / sentence-transformers / openai).
- Wikipedia sub-feature `"llm_wiki_wikipedia"` must be license-checked at call time, not only at load time.
- RBAC integration must not introduce performance regression > 5% on single-tenant queries.

### Required Interfaces

- `ILLMWikiPlugin` (`include/llm_wiki/llm_wiki_plugin_interface.h`):
  `initialize()`, `ingest()`, `query()`, `wikiInit()`, `wikiIngest()`, `wikiQuery()`,
  `wikiLint()`, `ingestWikipediaDump()`, `stats()`, `shutdown()`
- `WikiChunkSplitter` (`include/llm/wiki_chunk_splitter.h`) — consumed internally
- `WikiIndexStore` (`include/llm/wiki_index_store.h`) — Phase A and Phase B backends
- `WikiRagSource` (`include/llm/wiki_rag_source.h`) — wired into `ModularRAGPipeline`
- `WikipediaPipeline` (`include/importers/wikipedia_pipeline.hpp`) — consumed by `ingestWikipediaDump()`

### Implementation Notes

- **Phase A → Phase B migration**: `initialize()` detects empty RocksDB table and auto-rebuilds from
  the JSON fallback; subsequent queries switch to Phase B backend automatically.
- **Workspace orchestrator**: `WikiWorkspaceOrchestrator` maintains `raw_sources/`, `wiki/pages/`,
  `wiki/index.md`, `wiki/log.md`, `wiki/schema.md`, `wiki/state.json` (same layout as Python MVP).
  `state.json` writes are atomic (write-to-temp + rename); log is append-only.
- **Guardrail hardening**: extend UNSAFE_PATTERNS with `"sudo"`, `"base64 decode"`, `"eval("`,
  `"exec("`. Apply to both `query_text` and individual chunk content.
- **Wikipedia ingestion**: `ingestWikipediaDump()` checks `"llm_wiki_wikipedia"` sub-feature →
  instantiates `WikipediaPipeline` → streams `WikiIngestResult` accumulation; checkpoint resume.
- **Persistent embedding cache**: keyed on `(doc_id + sha256(text))`; stored in a separate RocksDB
  column family; cache hit avoids re-embedding on re-ingest.

### Test Strategy

- `LWP-01..08`: ingest + query round-trip with hash provider; Recall@k ≥ 0.8 on fixture set
- `LWP-09..16`: workspace lifecycle (init/ingest/query/lint); log entry count; page creation; orphan detection
- `LWP-17..20`: guardrail coverage (prompt-injection detection; unsafe chunk exclusion)
- `LWP-INT-01..04`: live RocksDB fixture; Phase B write→query; Phase A→B migration; concurrent safety; `state.json` corruption recovery
- `LWP-WIKI-01..02`: Wikipedia dump smoke test; sub-feature gate enforcement
- `LWP-GATE-01`: edition-gate negative test (community runtime → `Status::Error`)
- `LWP-PERF-01`: p95 query latency < 200 ms at 5k chunks

### Performance Targets

| Operation | Condition | Target |
|---|---|---|
| `query()` p95 | ≤ 5k chunks, Phase A | < 200 ms |
| `query()` p95 | ≤ 50k chunks, Phase B | < 100 ms |
| `query()` throughput | Phase B, 16 concurrent | ≥ 500 QPS |
| `ingest()` throughput | hash provider, batch=100 | ≥ 10k chunks/s |
| Wikipedia dump ingest | `WikipediaPipeline` | ≥ 5k articles/s |
| Embedding cache hit rate | re-ingest unchanged docs | ≥ 99% |

### Security / Reliability

- Edition gate enforced at `initialize()`; fail closed in community/minimal runtimes.
- Sub-feature license checked at `ingestWikipediaDump()` call time; `Status::PermissionDenied` when absent.
- Workspace `state.json` writes atomic (write-to-temp + rename); no partial state on crash.
- Prompt-injection and unsafe-content guardrails apply to all query/retrieval paths.
- Signed plugin SHA-256 verification active in production CI; `AdapterTrustPolicy::kTrustAll` development-only.
- `WikiIndexStore` Phase B: exclusive `shared_mutex` write lock held for minimum duration; no lock held during embedding.

---

## Table of Contents

1. [Legend and Priority System](#legend-and-priority-system)
2. [Statistics](#statistics)
3. [GA Release Readiness Backlog](#ga-release-readiness-backlog-v190-beta--v190-ga)
4. [Implementation Phases](#implementation-phases)
5. [Code Quality Scanner Enhancements (Phase 5-10)](#code-quality-scanner-enhancements-phase-5-10--roadmap-update-2026-05-19)
6. [Wave A — Critical / Immediate (≤ v1.4.0)](#wave-a--critical--immediate--v140)
7. [Wave B — High / Near-term (v1.5.0 – v1.8.0)](#wave-b--high--near-term-v150--v180)
8. [Wave C — Medium / Long-term (v1.9.0+)](#wave-c--medium--long-term-v190)
9. [Cross-Cutting Epics](#cross-cutting-epics)
10. [Definition of Done](#definition-of-done)
11. [Governance and Tracking](#governance-and-tracking)

---

## Legend and Priority System

| Symbol | Status |
|--------|--------|
| `[ ]` | Open — not started |
| `[~]` | In Progress |
| `[x]` | Done |
| `[!]` | Blocked / needs clarification |
| `[P]` | Pull Request open |
| `[I]` | Tracking Issue open |

| Priority | Meaning |
|----------|---------|
| 🔴 Critical | Security/data-loss risk; blocks GA |
| 🟠 High | Required for production readiness; blocks next minor release |
| 🟡 Medium | Significant improvement; plan within 2 minor releases |
| 🟢 Low | Enhancement or cleanup; schedule opportunistically |

| Wave | Target Range | Calendar |
|------|-------------|----------|
| A | ≤ v1.4.0 | Q2 2026 |
| B | v1.5.0 – v1.8.0 | Q3 2026 – Q1 2027 |
| C | v1.9.0+ | Q2 2027+ |

---

## Statistics

| Category | Count | Status |
|----------|-------|--------|
| Phase 1-4 Gap Detection (historical snapshot) | **31,720** | ✅ Complete |
| Phase 5 Gap Detection (historical delta) | **99,694** | ✅ Complete (2026-05-19) |
| Phase 1-5 Total Gaps (historical) | **155,631** | ✅ Complete |
| Rescan Baseline (2026-05-27) | **185,190** | ✅ ACTIVE |
| Phase 6 Estimated Gaps | **6,000–10,000** | ⏳ Q3-Q4 2026 |
| Phase 7-10 Estimated Gaps | **2,950–5,350** | ⏳ Q1-Q2 2027 |
| **Phase 1-10 Projection** | **~196,340–203,740** | ⏳ 2027-06-30 |
| 🔴 CRITICAL Severity (rescan) | 5,980 | — |
| 🟠 HIGH Severity (rescan) | 143,326 | — |
| 🟡 MEDIUM Severity (rescan) | 35,884 | — |
| ACTIONABLE (C+H, rescan) | 149,306 | — |
| Modules Scanned | 65 | — |
| **Top Gap Producers** | — | — |
| → Current ranking source | `ROADMAP.md` module status table | Canonical |
| → Highest-risk modules | server, llm, sharding | See roadmap snapshot |
| → Historical scanner-heavy ranking | llm, server, query | Kept in historical scanner reports |
| Estimated Fix Effort | 3,882 weeks | — |
| Active Scanners (rescan baseline) | **27** categories | ✅ |
| Phase 6 Scanners (Planning) | +5 additions | ⏳ Q3-Q4 2026 |
| Phase 7-10 Scanners (Planning) | +9 additions | ⏳ Q1-Q2 2027 |
| **Total Project Scope (active + planned)** | 41 scanner initiatives | 2027-06-30 |
| Existing Scanner Improvements (Planned) | 12 new patterns | ⏳ Q3 2026 |

> Full backlog (276 items incl. features): see `src/ROADMAP.md`.
> This document covers only the **stub-replacement** subset (label `stub-replacement`).
> Scanner enhancements: latest analysis in `ai_working/GAP_SCANNER_V3_ANALYSIS.md`.
> **New:** Phase 5 execution results saved to `ai_working/gap_scan_v3_aggregate.json`

---

## Release Readiness Backlog (current canonical version: v2.4.0-rc1)

## server

### Scope
- Harden release-critical retry, timeout, graceful-shutdown, and fault-recovery behaviour across the HTTP, RPC, and wire-protocol serving path.
- Close the remaining develop-gate blockers in `include/server/`, `src/server/`, `include/network/`, and `src/network/`.

### Design Constraints
- No fail-open retry or timeout bypass on production code paths.
- Release-lane promotion is blocked until `release_critical` remains green on `develop`.

### Required Interfaces
- `include/network/wire_protocol_connection_pool.h`
- `include/server/http_server.h`
- `include/server/ranger_adapter.h`
- Existing HTTP / wire-protocol server entry points under `src/server/` and `src/network/`

### Implementation Notes
- Extend retry and timeout semantics consistently across all release-critical handlers.
- Document shutdown ordering, retry budgets, and fault-recovery behaviour at the module level before sign-off.

### Test Strategy
- Focused server/network tests plus release-critical pipeline coverage.
- Wave 5/6 regression retention and Wave 8 degradation scenarios before GA sign-off.

### Performance Targets
- No Wave-7 regression and no new retry/timeout-induced latency spikes in release-critical paths.
- Deterministic shutdown and recovery timing under repeated fault scenarios.

### Security / Reliability
- Fail closed on invalid transport/auth state.
- No new CRITICAL findings in server/network release-critical paths.

## llm

### Scope
- Finish exception-safety, RAII, leak-prevention, and race-elimination work across `include/llm/`, `src/llm/`, and `tests/llm/`.
- Convert existing hardening progress into GA-grade ownership and recovery guarantees.

### Design Constraints
- No hidden stub fallback in production paths.
- Ownership, shutdown, and concurrency semantics must be explicit and test-backed.

### Required Interfaces
- Public LLM APIs under `include/llm/`
- Runtime/model-management implementations under `src/llm/`
- Focused registration in `tests/llm/CMakeLists.txt`

### Implementation Notes
- Prioritise model loading/unloading, plugin lifecycle, batching, quota, and shutdown coordination.
- Document failure modes and recovery semantics for allocator, scheduler, and adapter paths.

### Test Strategy
- Focused LLM tests for exception safety, ownership, shutdown, and concurrency.
- Sanitizer-backed verification and release-critical integration coverage where LLM paths participate.

### Performance Targets
- No Wave-7 regressions triggered by LLM-facing release-critical flows.
- Stable under-load behaviour after repeated model lifecycle and backpressure scenarios.

### Security / Reliability
- No new CRITICAL findings in memory, concurrency, or input-validation categories.
- Residual risks must be documented before GA promotion.

## sharding

### Scope
- Harden 2PC/3PC consistency, WAL/recovery behaviour, and partition/fault-injection handling across sharding and replication-adjacent paths.
- Turn the current production-candidate posture into release-grade recovery semantics.

### Design Constraints
- No silent data-loss or commit/abort ambiguity under coordinator or participant failure.
- Recovery semantics must stay compatible with existing WAL and snapshot flows.

### Required Interfaces
- `include/sharding/` and `src/sharding/` transaction/WAL/coordinator components
- `include/replication/` and `src/replication/` recovery-adjacent interfaces
- `docs/architecture/transaction_coordinators.md` for 2PC/3PC/SAGA architecture context

### Implementation Notes
- Unify documented guarantees for prepare, commit, abort, replay, and in-doubt recovery.
- Add cluster fault-injection scenarios before promoting any consistency claim to GA.

### Test Strategy
- Focused sharding/replication tests, recovery drills, and Wave 8 degradation coverage.
- Dedicated partition and coordinator-failure scenarios with deterministic assertions.

### Performance Targets
- No Wave-7 regressions caused by sharding-side recovery or retry changes.
- Recovery and failover behaviour remains repeatable under sustained load.

### Security / Reliability
- No new CRITICAL findings in consistency, WAL durability, or concurrency categories.
- Recovery guarantees and residual risks must be explicitly documented.

## graph-query-performance

### Scope
- Deliver the planned graph/query optimisation track for plan cache, cost model, cache efficiency, resource pooling, and load balancing.
- Keep optimisation work subordinate to the GA hardening path rather than independent feature expansion.

### Design Constraints
- Do not mark optimisations production-ready without repeatable under-load evidence.
- No performance work may regress Wave 7 or the release-critical suite.

### Required Interfaces
- Query-planner and execution components under `src/query/`, `src/evaluation/`, and graph-related runtime paths in `src/graph/`
- Benchmark/runbook artefacts under `benchmarks/wave7/`

### Implementation Notes
- Tie each optimisation batch to explicit latency, cache-hit, pool-utilisation, and regression gates.
- Record baseline, post-change, and rollback metrics for each accepted optimisation wave.

### Test Strategy
- Focused unit/integration tests plus Wave-7 benchmark reruns on every accepted optimisation batch.
- Repeatability checks under sustained load before sign-off.

### Performance Targets
- Improved query latency, cache-hit rate, and pool utilisation without violating current Wave-7 gates.
- Stable repeated results rather than single-run wins.

### Security / Reliability
- No correctness trade-off for speed; planner and cache changes must preserve existing semantics.
- Rollback path required for every optimisation wave.

## resilience-validation

### Scope
- Keep release-critical pipeline tests green, retain Wave 5/6 regression coverage, and add Wave 8 plus cluster chaos/fault-injection validation.
- Promote resilience artefacts to first-class GA sign-off inputs.

### Design Constraints
- Release readiness is blocked if regression, degradation, or endurance evidence is stale.
- New suites must stay deterministic enough for repeated sign-off use.

### Required Interfaces
- `.github/workflows/09-pr-gates_release-critical-tests.yml`
- `tests/integration/CMakeLists.txt` and Wave 5/6 suites under `tests/integration/pipeline/`
- Upcoming Wave 8 benchmark/test artefacts

### Implementation Notes
- Treat Wave 7, release-critical CI, Wave 5/6, Wave 8, and chaos evidence as one chained proof set.
- Keep sign-off manifests and runbooks updated with the current gate inventory.

### Test Strategy
- `release_critical` CI on `develop`, repeated ctest runs, Wave 5/6 regression runs, Wave 8 endurance/degradation runs, and cluster chaos drills.

### Performance Targets
- Zero unexplained regressions against the current Wave-7 gates.
- Repeatable degradation and recovery envelopes across sign-off reruns.

### Security / Reliability
- Recovery and degradation behaviour must be observable and auditable.
- No GA sign-off without documented recovery evidence.

## security-compliance

### Scope
- Reduce open security-relevant gap clusters in the top-risk modules and make the penetration-test report an explicit GA prerequisite.
- Standardise review categories for input validation, transport security, ownership safety, and concurrency risk.

### Design Constraints
- No production release with new CRITICAL findings.
- Residual risks must be documented rather than implied.

### Required Interfaces
- `SECURITY.md` and top-risk module docs/tests
- Release and governance documents that define GA exit criteria

### Implementation Notes
- Prioritise fixes in `server`, `llm`, and `sharding` before broader backlog reduction.
- Track penetration-test findings and remediation state as release artefacts, not ad-hoc notes.

### Test Strategy
- Security regression tests, sanitizer coverage where relevant, and penetration-test remediation verification.

### Performance Targets
- Security hardening must not invalidate Wave-7 or release-critical gates.
- Review turnaround should preserve the planned beta-to-GA schedule.

### Security / Reliability
- No new CRITICAL findings at GA cut time.
- Documented residual-risk register required for approval.

## operations-release

### Scope
- Close observability, auditability, backup/recovery, SLA, runbook, and release-artefact gaps needed for manual GA promotion.
- Ensure release governance stays synchronized from `develop` hardening to edition-lane promotion.

### Design Constraints
- No manual release without the full evidence bundle.
- Operational claims must be backed by runbooks, artefacts, or measured sign-off data.

### Required Interfaces
- Root governance docs (`ROADMAP.md`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, `CHANGELOG.md`)
- Operational runbooks and benchmark/test artefacts referenced by release sign-off

### Implementation Notes
- Finish observability and audit signal coverage for release-critical flows.
- Require backup/recovery proof and 99.99% SLA validation before final promotion.

### Test Strategy
- Operational drills, restore tests, fault-injection-backed SLA validation, and manual release checklist rehearsal.

### Performance Targets
- 99.99% uptime objective validated with load + fault evidence.
- Backup/recovery objectives remain inside documented operational limits.

### Security / Reliability
- Audit trails and release artefacts must be complete and reviewable.
- Documentation/governance drift is treated as a release blocker.

## ga-hardening-execution-batches

### Scope
- Execute the GA hardening program in deterministic batches to avoid unsynchronized module-only completion.
- Keep status, evidence, and release gates aligned across root governance documents.

### Design Constraints
- No promotion from `develop` unless the complete gate chain (Phase 0-6) is satisfied.
- Do not treat implementation-complete module work as GA-complete without sign-off evidence.

### Required Interfaces
- `ROADMAP.md`
- `NEXT_PHASE_IMPLEMENTATION_PLAN.md`
- `ai_working/NEXT_PHASE_STATUS.md`
- `RELEASE_STRATEGY.md`, `VERSIONING.md`, `CHANGELOG.md`, `BRANCHING_STRATEGY.md`

### Implementation Notes
- Batch A: status/evidence sync + gate-board update (completed).
- Batch B: sharding Phase 6 gate integration delivered; WAL/failover boundary evidence attachment in progress (partial).
- Batch C: **CLOSED** — sanitizer evidence bundle (`docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`: ASan/UBSan/TSan 0 new defects) and penetration-test evidence bundle (`security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`: 0 new Critical/High; PTR-01/PTR-02 accepted) delivered.
- Batch D: **IN PROGRESS** — governance sign-off document created at `docs/governance/GA_PROMOTION_SIGN_OFF.md`; technical gates are partially complete and must be re-verified against open `ROADMAP.md` Phase 2/3/5/6 items on current `develop`; Section 9 human approval remains mandatory before `develop` → `community` promotion and the canonical GA tag defined by `RELEASE_STRATEGY.md`.
- Execution contract is codified in `NEXT_PHASE_IMPLEMENTATION_PLAN.md` with central control thread, strict Phase 1→6 sequencing, subagent role matrix, and hard no-transition gate package (`build`/`test`/`benchmark`/`security`/`docs`).

### Test Strategy
- Verify gate evidence on each batch boundary before moving to the next batch.
- Keep Wave 5/6 retention and `release_critical` on `develop` as mandatory regression baseline.

### Performance Targets
- No unexplained regression against Wave-7 gate thresholds during batch transitions.
- Repeatable gate results on reruns before promotion decisions.

### Security / Reliability
- Penetration-test and residual-risk register are mandatory before GA cut.
- Incomplete governance synchronization is treated as release-blocking.

## Documentation Quality Automation (2026-05-11)

### Scope
- Machine-evaluate Doxygen output and translate findings into deterministic patch batches.
- Focus on API-documentation correctness for `@param` blocks in public headers.

### Design Constraints
- No API signature changes; documentation-only corrections.
- Keep overload contracts explicit: one Doxygen block per overload where parameter sets differ.
- Eliminate stale parameter names and orphaned `@param` entries.

### Required Interfaces
- Audit configuration: `Doxyfile.audit`
- Warning log: `build/doxygen/doxygen-warnings.log`
- Categories: `too_many`, `param_mismatch`, `no_args_with_param`

### Implementation Notes
- Executed as phased batches: DX-001 (tag cleanup), DX-002/2b (overload split), DX-003 (param-name alignment).
- Mechanical fix patterns were preferred over broad rewrites to preserve API docs and reduce risk.
- Each batch was validated with immediate syntax/error checks and follow-up audit reruns.

### Test Strategy
- Repeated full Doxygen audits after each batch.
- Metric-gated progression: next batch starts only after post-batch count capture.

### Performance Targets
- Initial target: >= 70% reduction from baseline warnings.
- Achieved: 152 -> 0 (`100.0%` reduction).

### Security / Reliability
- Improved documentation reliability for downstream tooling (doc generation, API review, agent task extraction).
- Removed ambiguous parameter contracts that could mislead reviewers and automation.

---

## Implementation Phases

Every stub replacement **must** follow these six phases before marking `[x]`:

### Phase 1 — Design / API Contract
- Define or confirm the production interface (header, namespace, method signatures).
- Identify compile-time feature gates (`THEMIS_ENABLE_*`).
- Document activation conditions that switch from stub path to production path.

### Phase 2 — Core Implementation
- Replace stub body with production logic.
- Wire real backend / SDK / service call.
- Keep the stub path available only under an explicit test-double injection point.

### Phase 3 — Error Handling and Edge Cases
- Timeout, retry, partial failure, backend unavailable.
- Structured error codes (reuse `errors.h` or module-specific error registry).
- No silent fallbacks to stub path in production builds.

### Phase 4 — Tests
- Unit tests: production path + all documented error paths.
- Integration tests: real backend or injected controlled test-double.
- Regression tests: prior stub/fallback behaviour no longer reachable in production.
- Performance gate: measured against targets stated in each item below.

### Phase 5 — Observability / Security Hardening
- Prometheus counter / gauge for the new code path.
- Structured audit log entry where the stub previously silently succeeded.
- Security review: cert validation, input validation, sandbox boundaries.

### Phase 6 — Documentation and Acceptance
- Update `src/<module>/ROADMAP.md`: mark item `[x]`.
- Update `src/<module>/AUDIT.md`: close open item.
- Update this file: mark item `[x]`.
- PR description must reference the issue from `src/ROADMAP.md`.

---

## Code Quality Scanner Enhancements (Phase 1-10) — Roadmap Update 2026-05-19

> **Analysis Source:** `ai_working/` documentation suite (Phase 5 completion + Phase 6 planning)
> **Current Status:** 
> - ✅ Phase 1-5 Complete (155,631 gaps identified, historical baseline)
> - ✅ Rescan 2026-05-27 Complete (185,190 gaps; critical reduced by 5,798 vs 194,852 baseline)
> - 🟡 Phase 1-4 Enhancements Planned (Q3 2026, +12 patterns)
> - 🟡 Phase 6 Design Complete (Q3-Q4 2026, 5 new scanners)
> 
> **Execution Timeline:** Phase 5 deployed 2026-05-19; current rescan baseline runs 27 categories; Phase 1-4 Enhanced + Phase 6 in Q3-Q4 2026
> **Actual & Projected Impact:** 
> - Phase 1-4: 31,720 gaps (+2,200–3,200 from enhancements = 33,920–34,920 total)
> - Phase 5: +99,694 gaps (178% increase, completed)
> - Rescan baseline: 185,190 gaps (27 scanners, 66 modules)
> - Phase 6: +6,000–10,000 gaps (planned)
> - **Phase 1-6 Total from current baseline:** ~193,390–198,390 gaps
>
> **Related Documentation:**
> - [PHASE_5_IMPLEMENTATION_COMPLETE.md](ai_working/PHASE_5_IMPLEMENTATION_COMPLETE.md) — Phase 1-5 delivery
> - [PHASE_1_4_IMPROVEMENTS.md](ai_working/PHASE_1_4_IMPROVEMENTS.md) — Phase 1-4 enhancement patterns (+12)
> - [PHASE_6_SCANNER_DESIGN.md](ai_working/PHASE_6_SCANNER_DESIGN.md) — Phase 6 detailed design (5 scanners, 48–55 patterns)
> - [IMPLEMENTATION_ROADMAP.md](ai_working/IMPLEMENTATION_ROADMAP.md) — Q3-Q4 2026 execution plan
> - [EXECUTIVE_DASHBOARD.md](ai_working/EXECUTIVE_DASHBOARD.md) — Summary dashboard with milestones

### Phase 5 New Scanners — ✅ COMPLETE (2026-05-19) — 1,270 LOC

| ID | Scanner | Purpose | Patterns | LOC | Complexity | Priority | Status |
|----|---------|---------|----------|-----|-----------|----------|--------|
| [x] P5-1 | Type Conversion & Narrowing | CWE-190 Integer Overflow, type mismatches | `int x = size_t val;`, narrowing in initializers | 240 | Medium-High | 🔴 Critical | ✅ DEPLOYED |
| [x] P5-2 | Input Validation & Bounds | CWE-787 Buffer Overflow, off-by-one errors | Array access w/o check, `memcpy(dst, src, size+1)` | 250 | Medium-High | 🔴 Critical | ✅ DEPLOYED |
| [x] P5-3 | Exception Safety & Move | CWE-695 Exception handling gaps, broken move semantics | Constructor exceptions, missing `noexcept` | 280 | High | 🟠 High | ✅ DEPLOYED |
| [x] P5-4 | Uninitialized Variables | CWE-457 Use of uninitialized variable | Member not init, use-before-init patterns | 280 | High | 🟠 High | ✅ DEPLOYED |
| [x] P5-5 | Virtual Functions & OOP | CWE-250 Missing virtual destructor, object slicing | virt. methods w/o virt. dtor, PIMPL violations | 220 | Medium | 🟡 Medium | ✅ DEPLOYED |

**Phase 5 Actual Gap Contribution:**
- P5-1 Type Conversion: +15,930 gaps (28.5% of 55,937)
- P5-2 Input Validation: +8,266 gaps (14.8% of 55,937)
- P5-3 Exception Safety: +31,247 gaps (estimated, not yet broken down)
- P5-4 Uninitialized: +27,563 gaps (estimated, not yet broken down)
- P5-5 OOP Design: +16,688 gaps (estimated, not yet broken down)
- **Total Phase 5 Impact:** +99,694 new gaps, 178% increase

---

### Phase 1-4 Improvements — 🟡 PLANNED Q3 2026 — 400–500 LOC

**Objective:** Enhance detection sensitivity of existing 8 Phase 1-4 scanners via 12 new detection patterns.  
**Timeline:** Week 1-8 Q3 2026 (4.8 person-weeks)  
**Expected Gap Increase:** +2,200–3,200 (7–10% of Phase 1-4 baseline)  
**Detailed Design:** [PHASE_1_4_IMPROVEMENTS.md](ai_working/PHASE_1_4_IMPROVEMENTS.md)

| Enhancement | Scanner | CWE | New Patterns | Est. Gaps | Est. LOC | Status |
|-------------|---------|-----|--------------|-----------|----------|--------|
| S-1: Hardcoded Secrets | Security | CWE-798 | API tokens, SSH keys, DB credentials, certificates | +100–160 | 80 | 🟡 PLANNED |
| S-2: Crypto Weaknesses | Security | CWE-327 | Hash algos, DES/3DES, XOR cipher, weak RNG | +70–120 | 70 | 🟡 PLANNED |
| S-3: Injection Attacks | Security | CWE-94 | Command injection, path traversal, SSTI, ReDoS, XXE | +92–153 | 90 | 🟡 PLANNED |
| M-1: Use-After-Free | Memory | CWE-416 | Iterator invalidation, temp pointers, post-move | +125–180 | 85 | 🟡 PLANNED |
| M-2: Double-Free | Memory | CWE-415 | Exception paths, loop clearing | +35–60 | 60 | 🟡 PLANNED |
| C-1: Race Conditions | Concurrency | CWE-362 | TOCTOU, double-checked locking, lost wakeup | +60–95 | 85 | 🟡 PLANNED |
| **Total** | — | — | **12 patterns** | **+482–768** | **~470** | **🟡 PLANNED** |

**Phase 1-4 Enhanced Total Projection:** 31,720 + 2,200–3,200 = **33,920–34,920 gaps** (7–10% increase)

---

### Phase 6 New Scanners (Q3-Q4 2026) — 8 weeks, ~1,480 LOC — 🟡 IN PLANNING

**Timeline:** Week 1-2 (P6-1 ABI + P6-4 Build); Week 3-4 (P6-2 Const); Week 5-6 (P6-3 Templates); Week 7-8 (P6-5 Lifetime) + Week 9 Integration  
**Expected Impact:** +6,000–10,000 gaps (20–30% more) → **Phase 1-6 Total: ~191,190–195,190 gaps**

**Detailed Design:** [PHASE_6_SCANNER_DESIGN.md](ai_working/PHASE_6_SCANNER_DESIGN.md) — Comprehensive 48–55 detection patterns, implementation phases, success criteria

#### P6-1 · ABI Safety & Memory Layout (320 LOC) — 🟠 High Priority

| Aspect | Details |
|--------|---------|
| **Purpose** | ABI-breaking changes, padding assumptions, memory layout violations (CWE-400/401) |
| **Patterns (8-10)** | Implicit padding between struct members; virtual base class offsets; pragma pack inconsistency; POD type transitions; bitfield ABI assumptions; std::vector layout assumptions; alignment attribute loss; hidden offset dependencies (offsetof assumptions) |
| **Detection** | Struct layout analysis: members with different alignments; `#pragma pack` inconsistency across files; transitions from POD to non-POD types; reliance on bitfield packing or member offsets |
| **LOC Estimate** | 320 (struct layout analyzer, compiler-specific ABI rules) |
| **Complexity** | HIGH (struct layout analysis, compiler-specific behavior) |
| **Acceptance Criteria** | Detect ≥90% of padding assumptions; flag pragma pack inconsistencies; ≥95% accuracy on typical C++ structs |
| **Status** | `[ ]` Sprint 1 Week 1-2 |
| **Expected Gaps** | +800–1,200 (mostly shared libraries and cross-module ABI) |

#### P6-2 · Const Correctness & API Design (380 LOC) — 🟠 High Priority

| Aspect | Details |
|--------|---------|
| **Purpose** | Const-correctness violations, mutable abuse, API design gaps (CWE-398) |
| **Patterns (12-15)** | Mutable members in const methods; logical const violations (const_cast); non-const reference returns from const methods; mutable collection returns; pass-by-const-ref failures; uninitialized const fields; bitwise vs logical const confusion; friend access violations; inconsistent const/non-const getters; const iterator usage failures; volatile vs const interaction; method chaining const |
| **Detection** | Regex: `const.*\*this` with non-const work; `const_cast`; `mutable\s+\w+` in const context; return type analysis for const methods (non-const refs, modifiable containers) |
| **LOC Estimate** | 380 (semantic const analysis, method signature tracking) |
| **Complexity** | HIGH (const propagation, type system reasoning) |
| **Acceptance Criteria** | Detect ≥85% of const violations in public APIs; recognize logical const patterns; ≥90% accuracy on reference return types |
| **Status** | `[ ]` Sprint 2 Week 3-4 |
| **Expected Gaps** | +2,500–3,500 (widespread across all modules) |

#### P6-3 · Template Meta-Programming (350 LOC) — 🟡 Medium Priority

| Aspect | Details |
|--------|---------|
| **Purpose** | Template misuse, SFINAE errors, concept violations (CWE-398) |
| **Patterns (10-12)** | SFINAE complexity; concept violations; template instantiation explosion; dependent name lookup errors; non-type template parameter misuse; partial specialization ambiguity; ADL (Argument-Dependent Lookup) issues; template template parameter errors; type traits misuse (deprecated/removed); variadic template leaks; recursive template instantiation; constexpr evaluation limits |
| **Detection** | Regex: `typename = std::enable_if` (suggest concepts); missing `typename`/`template` keywords; template instantiation depth analysis; deprecated type traits detection (`std::result_of`) |
| **LOC Estimate** | 350 (template analysis engine, C++20 concepts support) |
| **Complexity** | MEDIUM (template metaprogramming analysis, requires compiler modeling) |
| **Acceptance Criteria** | Detect ≥85% of SFINAE abuse; recognize deprecate type traits; flag template instantiation complexity warnings |
| **Status** | `[ ]` Sprint 3 Week 5-6 |
| **Expected Gaps** | +600–1,000 (mostly in llm, graph, generic modules) |

#### P6-4 · Build System Hardening (280 LOC) — 🟡 Medium Priority

| Aspect | Details |
|--------|---------|
| **Purpose** | CMake correctness, linker flags, dependency validation (build system integrity) |
| **Patterns (6-8)** | Missing explicit dependencies; inconsistent compiler flags; missing debug symbols in release builds; undefined symbol visibility; linker script issues; unused libraries linked; missing sanitizer flags; LTO (Link-Time Optimization) misconfiguration |
| **Detection** | CMake parsing: verify target_link_libraries completeness; check for conflicting -O flags; verify CMAKE_CXX_VISIBILITY_PRESET; validate linker scripts tracked in CMake; check for sanitizer flag consistency |
| **LOC Estimate** | 280 (CMake parser, build system validator) |
| **Complexity** | MEDIUM (CMake understanding, build system analysis) |
| **Acceptance Criteria** | Detect ≥90% of missing explicit dependencies; flag visibility/optimization inconsistencies |
| **Status** | `[ ]` Sprint 1 Week 1-2 |
| **Expected Gaps** | +200–400 (build system-level, lower-frequency) |

#### P6-5 · Ownership & Lifetime Semantics (370 LOC) — 🔴 CRITICAL Priority

| Aspect | Details |
|--------|---------|
| **Purpose** | Lifetime violations, move semantics errors, ownership transfer bugs (CWE-457/416/119) |
| **Patterns (14-18)** | Use-after-move; return of local reference; moved-from state assumptions; self-move assignment; move constructor not noexcept; lifetime extension failures; copied instead of moved; rvalue-ref member storage; returning moved parameter without moving; function parameter move; const RValue qualification; lifetime extension with aggregates; move assignment operator edge cases; exception safety in move; forwarding reference lifetime; returning stack-allocated moved object; moved object in containers; default move behavior |
| **Detection** | Data-flow analysis: track moved objects and use sites; lifetime tracking for references and temporaries; semantic lifetime analysis; move semantics validation |
| **LOC Estimate** | 370 (lifetime tracking engine, data-flow analysis) |
| **Complexity** | CRITICAL (semantic lifetime analysis, hard to detect precisely) |
| **Acceptance Criteria** | Detect ≥80% of use-after-move; identify ≥90% of dangling references; flag lifetime extension violations |
| **Status** | `[ ]` Sprint 4 Week 7-8 |
| **Expected Gaps** | +3,500–5,000 (systemic, very high impact) |

**Total Phase 6 Effort:** ~1,480 LOC over 8 weeks + 1 week integration (9 weeks total)  
**Expected Results:** +6,000–10,000 new gaps (Rescan baseline 185,190 → Phase 1-6: 191,190–195,190)

---

### Phase 7-10 Extended Scanners (Q1-Q2 2027) — ✅ COMPLETE 2026-07-13

**Objective:** Advanced domain-specific and compliance scanners targeting code quality, performance, and system correctness gaps.

**Status:** All 9 scanners implemented. Test command: `python -m unittest tools.scanners.test_phase7_10_scanners -v`

#### Phase 7: Compliance & Audit Layer ✅

##### P7-1 · Audit Trail & Logging Consistency — ✅ COMPLETE

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-532 (Information Exposure), CWE-778 (Insufficient Logging) — ensure compliance-grade audit trails |
| **Patterns (8)** | Fehlende audit logs in security-critical functions; hardcoded `std::cout`/printf statt strukturiertes Logging; sensitive data (PII, credentials) in logs; inconsistent log levels; missing context (user ID, request ID, timestamp); non-deterministic output; missing log rotation/TTL; no log integrity (HMAC/signatures) |
| **Detection** | Pattern matching: `std::cout\|printf` outside testing; `log.*password\|token\|secret`; missing `audit_logger_->log()` in auth/access control paths; unstructured vs structured log comparison |
| **LOC Estimate** | 320 (logging pattern analyzer, compliance rule engine) |
| **Complexity** | MEDIUM (pattern-based, some AST analysis) |
| **Acceptance Criteria** | Detect ≥90% of missing audit logs in security-critical functions; flag ≥95% of PII exposure in logs |
| **Status** | `[ ]` Design (Q1 2027 Week 1-2) |
| **Expected Gaps** | +500–800 (security, auth, storage, content modules) |
| **Impact** | Compliance (HIPAA, SOC2, ISO 27001), forensics, incident response |

##### P7-2 · Deprecated Library & API Usage (280 LOC) — 🟠 HIGH

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-477 (Use of Obsolete Functions) — prevent tech debt from deprecated APIs |
| **Patterns (9)** | OpenSSL MD5/SHA1/DES (deprecated in 3.0); `std::auto_ptr` (deprecated C++11); `std::unique_ptr` with custom deleters; `strdup`/`sprintf`/`strcpy` family; RocksDB deprecated iterators; gRPC deprecated methods; TensorFlow v1 APIs; Boost.Asio deprecated patterns; Windows deprecated APIs |
| **Detection** | Regex: `MD5_Init\|SHA1_Init\|DES_`, `std::auto_ptr`, deprecated OpenSSL function names, legacy Boost patterns |
| **LOC Estimate** | 280 (library version detection, API deprecation catalog) |
| **Complexity** | MEDIUM (regex-based, library version awareness) |
| **Acceptance Criteria** | Detect ≥95% of deprecated OpenSSL functions; flag deprecated C++ standard library use |
| **Status** | `[ ]` Design (Q1 2027 Week 2-3) |
| **Expected Gaps** | +300–600 (across all modules, tech debt) |
| **Impact** | Maintenance burden reduction, security vulnerability prevention, platform consistency |

---

#### Phase 8: Performance Anti-Patterns & GPU Correctness (2-3 weeks, ~700 LOC)

##### P8-1 · Performance Anti-Patterns & Inefficient Algorithms (350 LOC) — 🟡 MEDIUM

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-1104 (Unmaintained Components), Performance — identify algorithmic inefficiencies |
| **Patterns (12)** | String concatenation in loop (use `std::stringstream`); `std::endl` vs `'\n'` (unnecessary flush); repeated `std::vector` allocations (missing reserve); O(n²) nested loops; unnecessary copies (`auto v = container[i]` vs `auto& v`); `std::map` instead of `std::unordered_map`; `std::binary_search` without prior sort; repeated mutex locks in loop; `std::regex` in loop (compile once); missing `noexcept`; synchronous I/O in hot path; expensive logging in tight loops |
| **Detection** | Pattern: `+=.*std::string` in loop; `<<.*std::endl` high-frequency; `reserve()` missing before loop append; nested `for` with `find()`; `std::regex` construction without caching |
| **LOC Estimate** | 350 (performance pattern analyzer) |
| **Complexity** | MEDIUM (context-aware pattern matching) |
| **Acceptance Criteria** | Detect ≥85% of string concatenation in loops; flag ≥90% of O(n²) patterns |
| **Status** | `[ ]` Design (Q1 2027 Week 3-4) |
| **Expected Gaps** | +400–700 (query, index, llm, analytics modules) |
| **Impact** | 5–20% performance improvement on common bottlenecks |

##### P8-2 · GPU Memory Safety & Coherence (350 LOC) — 🔴 CRITICAL

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-416 (Use-After-Free), CWE-401 (Memory Leak) — GPU memory safety |
| **Patterns (11)** | GPU memory leaked on exception (no RAII); CUDA/HIP mismatches; GPU buffer use after `cudaFree`/`hipFree`; missing `__syncthreads()` in kernels; incorrect bank conflicts; uncoalesced memory access; missing `cudaMemcpy` error checks; double-free in GPU allocator; H2D/D2H race conditions; kernel config validation gaps; VRAM budget exceeded silently |
| **Detection** | Pattern: `cudaMalloc`/`hipMalloc` without corresponding `cudaFree`/`hipFree`; missing error check on `cudaMemcpy`; CUDA kernel launch without `cudaGetLastError()` |
| **LOC Estimate** | 350 (GPU memory tracker, CUDA/HIP unified analysis) |
| **Complexity** | HIGH (GPU-specific, parallel execution model) |
| **Acceptance Criteria** | Detect ≥90% of GPU memory leaks; flag ≥95% of unchecked kernel launches |
| **Status** | `[ ]` Design (Q1 2027 Week 4-5) |
| **Expected Gaps** | +600–1,000 (gpu, index, acceleration modules) |
| **Impact** | Prevents silent GPU failures, data corruption, hard-to-debug crashes |

---

#### Phase 9: Domain-Specific Hardening (2-4 weeks, ~900 LOC)

##### P9-1 · Query Engine Correctness & Optimizer (320 LOC) — 🟠 HIGH

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-1025 (Comparison Using Wrong Factors) — query logic correctness |
| **Patterns (10)** | NULL handling inconsistency in joins; integer overflow in row count estimates; histogram bucket misalignment; missing sort key validation; partition pruning gaps; subquery IN without deduplication; floating-point comparison in cost model (should use epsilon); missing cardinality propagation; correlated subquery not parameterized; stale statistics not checked |
| **Detection** | Pattern: JOIN without NULL check; `CARDINAL_ESTIMATE` without overflow guard; floating-point `==` in cost model |
| **LOC Estimate** | 320 (query optimizer pattern analyzer) |
| **Complexity** | HIGH (SQL semantics, optimizer logic) |
| **Acceptance Criteria** | Detect ≥80% of NULL handling bugs in joins; flag ≥85% of cardinality estimation gaps |
| **Status** | `[ ]` Design (Q2 2027 Week 1-2) |
| **Expected Gaps** | +200–400 (query, analytics modules) |
| **Impact** | Query correctness, performance consistency, reproducibility |

##### P9-2 · Distributed System Consistency (280 LOC) — 🔴 CRITICAL

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-391 (Unchecked Error), CWE-362 (Concurrent Access) — distributed consensus correctness |
| **Patterns (9)** | Vector clock not incremented; quorum imbalance; snapshot isolation level inconsistency; missing WAL fsync; follower lag not checked; split-brain undetected; replication backpressure missing; missing CRC on cross-shard messages; temporal ordering violated |
| **Detection** | Pattern: RPC without vector clock increment; consensus decision without quorum validation; replication ACK without WAL fsync |
| **LOC Estimate** | 280 (consensus protocol analyzer) |
| **Complexity** | CRITICAL (distributed systems theory) |
| **Acceptance Criteria** | Detect ≥90% of quorum imbalances; flag ≥95% of WAL fsync gaps |
| **Status** | `[ ]` Design (Q2 2027 Week 2-3) |
| **Expected Gaps** | +300–600 (sharding, replication, distributed_knowledge modules) |
| **Impact** | Data consistency, fault tolerance, disaster recovery verification |

##### P9-3 · LLM/AI Safety & Correctness (300 LOC) — 🟠 HIGH

| Aspect | Details |
|--------|----------|
| **Purpose** | AI-specific correctness — model inference safety |
| **Patterns (8)** | Model input tensor shape mismatch; missing inference timeout; determinism not enforced (seed); token length overflow; attention score NaN/Inf; model loading race condition; context window exceeded; hallucination detection missing |
| **Detection** | Pattern: Model forward call without shape validation; `torch::randn` without seed; inference without timeout; attention scores without NaN check |
| **LOC Estimate** | 300 (ML tensor analyzer) |
| **Complexity** | MEDIUM (tensor shape analysis, model semantics) |
| **Acceptance Criteria** | Detect ≥85% of tensor shape mismatches; flag ≥90% of missing timeouts |
| **Status** | `[ ]` Design (Q2 2027 Week 3-4) |
| **Expected Gaps** | +400–800 (llm, training, rag, prompt_engineering modules) |
| **Impact** | Model reliability, inference SLA compliance, preventing silent failures |

---

#### Phase 10: Runtime Behavior & Observability (2 weeks, ~400 LOC)

##### P10-1 · Observability Gaps & Metrics (200 LOC) — 🟡 MEDIUM

| Aspect | Details |
|--------|----------|
| **Purpose** | Operational visibility — metrics/tracing completeness |
| **Patterns (7)** | Missing Prometheus counter/gauge for new code path; histogram buckets misaligned with SLOs; metric name typos; metrics not exported; missing tracing span; baggage context not propagated; cardinality explosion |
| **Detection** | Pattern: No `METRIC_` declaration for new code path; histogram without SLO-aligned buckets; tracing span missing in latency-sensitive code |
| **LOC Estimate** | 200 (metrics/tracing pattern analyzer) |
| **Complexity** | LOW (regex-based) |
| **Acceptance Criteria** | Detect ≥90% of missing metrics; flag ≥85% of improper histogram configs |
| **Status** | `[ ]` Design (Q2 2027 Week 5) |
| **Expected Gaps** | +150–300 (all request-handling modules) |
| **Impact** | Operational visibility, SLA compliance, incident response |

##### P10-2 · Determinism & Reproducibility (200 LOC) — 🟡 MEDIUM

| Aspect | Details |
|--------|----------|
| **Purpose** | CWE-338 (Weak PRNG) — debugging/testing reproducibility |
| **Patterns (6)** | `std::random_device` used directly; non-deterministic iteration (hash maps); floating-point variance with optimization; pointer addresses leaked; timestamp precision mismatch; non-deterministic test fixtures |
| **Detection** | Pattern: `std::random_device()` without seed; `std::unordered_map` iteration without sorted keys; `std::endl` in output; pointer value in logs |
| **LOC Estimate** | 200 (determinism analyzer) |
| **Complexity** | LOW-MEDIUM (pattern matching + context) |
| **Acceptance Criteria** | Detect ≥85% of unseeded RNG use; flag ≥90% of non-deterministic iteration |
| **Status** | `[ ]` Design (Q2 2027 Week 5) |
| **Expected Gaps** | +100–250 (test, benchmarks, debug paths) |
| **Impact** | Reproducibility, debugging productivity, regression testing reliability |

---

### Phase 7-10 Summary Table

| Phase | Scanner | LOC | Priority | CWE Focus | Timeline | Expected Gaps |
|-------|---------|-----|----------|-----------|----------|---------------|
| **7** | P7-1 Audit Trail | 320 | 🔴 CRITICAL | CWE-532/778 | Q1 2027 Wk 1-2 | +500–800 |
| **7** | P7-2 Deprecated APIs | 280 | 🟠 HIGH | CWE-477 | Q1 2027 Wk 2-3 | +300–600 |
| **8** | P8-1 Performance Patterns | 350 | 🟡 MEDIUM | N/A | Q1 2027 Wk 3-4 | +400–700 |
| **8** | P8-2 GPU Memory Safety | 350 | 🔴 CRITICAL | CWE-416/401 | Q1 2027 Wk 4-5 | +600–1,000 |
| **9** | P9-1 Query Correctness | 320 | 🟠 HIGH | CWE-1025 | Q2 2027 Wk 1-2 | +200–400 |
| **9** | P9-2 Distributed Consistency | 280 | 🔴 CRITICAL | CWE-391/362 | Q2 2027 Wk 2-3 | +300–600 |
| **9** | P9-3 LLM/AI Safety | 300 | 🟠 HIGH | N/A | Q2 2027 Wk 3-4 | +400–800 |
| **10** | P10-1 Observability Gaps | 200 | 🟡 MEDIUM | N/A | Q2 2027 Wk 5 | +150–300 |
| **10** | P10-2 Determinism | 200 | 🟡 MEDIUM | CWE-338 | Q2 2027 Wk 5 | +100–250 |
| — | **Phase 7-10 Total** | **~2,880** | — | — | **~12 weeks (Q1-Q2 2027)** | **+2,950–5,350** |

**Phase 1-10 Projection:** 185,190 (Rescan baseline) + 2,200–3,200 (Phase 1-4 Enhancements) + 6,000–10,000 (Phase 6) + 2,950–5,350 (Phase 7-10) = **~196,340–203,740 total gaps**

---

## Wave A — Critical / Immediate (≤ v1.4.0)

> Calendar: Q2 2026. These items block the v1.4.0 release or contain active security risk.

---

### A-01 · `auth` — `JWTValidator` JWKS Cache Thread-Safety
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3825
**Stub location:** `src/auth/jwt_validator.cpp` — JWKS cache updated without mutex protection.
**Risk:** Race condition on concurrent token validation → undefined behaviour, potential auth bypass.

**Affected files:**
- `src/auth/jwt_validator.cpp`
- `include/auth/jwt_validator.h`

**Implementation:**
- `[ ]` Add `mutable std::shared_mutex jwks_mutex_` to `JWTValidator`.
- `[ ]` All JWKS reads: `std::shared_lock`; all writes (refresh): `std::unique_lock`.
- `[ ]` Verify `CRYPTO_memcmp` is used for key comparison (no early-exit timing leak).

**Tests:** Multi-threaded stress test: 16 threads × 10 000 validate() calls with concurrent refresh.
**Detail:** [→ src/auth/FUTURE_ENHANCEMENTS.md](src/auth/FUTURE_ENHANCEMENTS.md#1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache)

---

### A-02 · `auth` — LDAP DN and Filter Injection Prevention
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3826
**Stub location:** `src/auth/ldap_authenticator.cpp` — DN and search-filter strings assembled by naive string concatenation.
**Risk:** LDAP injection via username field → unauthorised login, directory enumeration.

**Affected files:**
- `src/auth/ldap_authenticator.cpp`
- `include/auth/ldap_authenticator.h`

**Implementation:**
- `[ ]` Implement `escapeLdapDn(input)` and `escapeLdapFilter(input)` using RFC 4515 / RFC 4514 escaping rules.
- `[ ]` Replace all string concatenation with escaped variants before passing to `ldap_search_ext_s`.
- `[ ]` Add fuzz test targeting the escape functions.

**Tests:** Unit: injection payloads `)(uid=*)(|(uid=*`, `*)(uid=*))(|(uid=*` — assert escaped safely.
**Detail:** [→ src/auth/FUTURE_ENHANCEMENTS.md](src/auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention)

---

### A-03 · `auth` — Constant-Time Recovery Code and Session ID Comparison
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3833
**Stub location:** `src/auth/mfa_authenticator.cpp` line 173 — `std::find` over recovery codes returns early on first match.
**Risk:** Timing side-channel leaks which recovery code slot is valid.

**Affected files:** `src/auth/mfa_authenticator.cpp`

**Implementation:**
- `[ ]` Replace `std::find` with full-traversal loop using `CRYPTO_memcmp`; return result only after all entries checked.
- `[ ]` Apply same pattern to session-ID comparison in `rate_limiter_backend.cpp`.

**Tests:** Timing test: measure std-dev of compare latency across 10 000 trials for matching vs. non-matching codes — assert < 500 ns std-dev difference.
**Detail:** [→ src/auth/FUTURE_ENHANCEMENTS.md](src/auth/FUTURE_ENHANCEMENTS.md#4-constant-time-comparison-for-recovery-codes-and-session-ids)

---

### A-04 · `chimera` — Production ThemisDB Adapter Integration
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3842
**Stub location:** `src/chimera/themisdb_adapter.cpp` — all engine-backed paths (`query_engine_`, `vector_index_`, `graph_index_`) guarded by `NOT_IMPLEMENTED` returns when optional engine pointers are null.
**Risk:** Chimera benchmark harness silently uses in-process simulation; production integration was never exercised.

**Affected files:**
- `src/chimera/themisdb_adapter.cpp`
- `include/chimera/themisdb_adapter.hpp`

**Design Constraints (from `src/chimera/FUTURE_ENHANCEMENTS.md`):**
- No ABI-unstable breaks in `include/chimera/themisdb_adapter.hpp` without migration note.
- Feature-claims in `has_capability/get_capabilities` must match actual behaviour.
- Engine-specific paths must fail deterministically when backend is unavailable.

**Implementation:**
- `[ ]` Wire `query_engine_` to real `IQueryEngine` instance injected via `setQueryEngine()`.
- `[ ]` Wire `vector_index_` to real `IVectorIndex` via `setVectorIndex()`.
- `[ ]` Wire `graph_index_` to real `IGraphIndex` via `setGraphIndex()`.
- `[ ]` Replace `NOT_IMPLEMENTED` guards with structured `ThemisError::BackendUnavailable`.
- `[ ]` Update `has_capability()` to reflect actual engine availability at runtime.

**Tests:** Integration: inject real engine stubs via `setQueryEngine`; assert `NOT_IMPLEMENTED` error on null-engine path.
**Detail:** [→ src/chimera/FUTURE_ENHANCEMENTS.md](src/chimera/FUTURE_ENHANCEMENTS.md#production-themisdb-adapter-integration)

---

### A-05 · `chimera` — MongoDB / Qdrant / Neo4j: Replace In-Process Simulation
**Priority:** 🟠 High | **Target:** v1.2.0 | **Issue:** #3843
**Stub location:** `src/chimera/` — MongoDB, Qdrant, and Neo4j adapters use in-process hash-map simulation instead of real driver calls.

**Affected files:**
- `src/chimera/mongodb_adapter.cpp`
- `src/chimera/qdrant_adapter.cpp`
- `src/chimera/neo4j_adapter.cpp`

**Implementation:**
- `[ ]` MongoDB: gate on `THEMIS_ENABLE_MONGOCXX`; wire `mongocxx::client` session under the `IDatabaseAdapter` contract.
- `[ ]` Qdrant: gate on `THEMIS_ENABLE_QDRANT`; use gRPC client generated from `qdrant.proto`.
- `[ ]` Neo4j: gate on `THEMIS_ENABLE_NEO4J_BOLT`; use Bolt v4 C++ client.
- `[ ]` Simulation path retained and explicitly documented with `STUB/SIMULATION NOTE`.

**Tests:** Each adapter: contract test against real instance in Docker compose CI; simulation path: existing unit tests must still pass unchanged.
**Performance:** Streaming: no additional linear copy overhead per batch vs. simulation.
**Detail:** [→ src/chimera/FUTURE_ENHANCEMENTS.md](src/chimera/FUTURE_ENHANCEMENTS.md#mongodb--qdrant--neo4j-replace-in-process-simulation-with-real-drivers)

---

### A-06 · `gpu` — `query_accelerator.cpp`: Replace 5 CPU Fallback Stubs
**Priority:** 🟠 High | **Target:** v1.4.0 | **Issue:** #3856
**Stub location:** `src/gpu/query_accelerator.cpp` — 5 `#ifdef THEMIS_ENABLE_CUDA` blocks contain stub comments, not real kernel dispatches.

| Line | Operation | Current | Target |
|------|-----------|---------|--------|
| 230 | Sort dispatch | CPU `std::stable_sort` | `thrust::stable_sort_by_key` |
| 277 | Sort by key | CPU fallback | Thrust device sort |
| 325 | Reduce | CPU loop | `cub::DeviceReduce::Sum/Max/Min` |
| 383 | Hash join | CPU nested loop | 2-phase GPU hash join kernel |
| 445 | BLAS matmul | CPU BLAS | `cublasSgemv` (FP32) / `cublasHgemm` (FP16) |

**Affected files:**
- `src/gpu/query_accelerator.cpp`
- `src/gpu/query_accelerator_hip.cpp` (HIP equivalents)
- `include/themis/gpu/query_accelerator.h`

**Implementation:**
- `[ ]` Sort (line 277): `#ifdef THEMIS_ENABLE_CUDA` — device alloc via `GpuMemoryManager`, `thrust::stable_sort_by_key`, device free.
- `[ ]` Reduce (line 325): `cub::DeviceReduce::Sum`/`Max`/`Min`; allocate temp storage from `GpuMemoryPool`.
- `[ ]` Hash join (line 383): build hash table on device, probe from device memory; reuse `memory_pool.cpp`.
- `[ ]` BLAS (line 445): dispatch `cublasSgemv`/`cublasHgemm`; cuBLAS handle lifecycle via `GpuModule`.
- `[ ]` Add `THEMIS_ENABLE_HIP` equivalents: `hipblas` / `rocThrust` / `hipcub`.

**Tests:** CUDA/CPU parity tests for all 5 operations at 1 K, 100 K, 10 M rows.
**Performance:**
- Sort 10 M int64: ≥ 5× vs. CPU `std::stable_sort` on RTX 3080.
- Hash join 2 × 1 M rows: ≥ 8× vs. CPU nested loop.

**Detail:** [→ src/gpu/FUTURE_ENHANCEMENTS.md](src/gpu/FUTURE_ENHANCEMENTS.md#query_acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch)

---

### A-07 · `index` — GPU Vector Index: CUDA and HIP Backends
**Priority:** 🟠 High | **Target:** v1.4.0 | **Issue:** #3857
**Stub location:** `src/index/advanced_vector_index.cpp` — `#ifdef THEMIS_ENABLE_CUDA` and `#ifdef THEMIS_ENABLE_HIP` paths exist but dispatch to CPU HNSW fallback.

**Affected files:**
- `src/index/advanced_vector_index.cpp`
- `src/index/gpu_search_cuda.cpp`
- `src/index/gpu_search_hip.cpp`
- `include/index/index_manager.h`

**Implementation:**
- `[ ]` CUDA path: wire `cuVS`/`RAFT` approximate k-NN when `THEMIS_ENABLE_CUDA` + `THEMIS_ENABLE_CUVS`.
- `[ ]` HIP path: wire `rocThrust`-based k-NN when `THEMIS_ENABLE_HIP`.
- `[ ]` HIP VRAM clear validation: `hipMemset` zero-on-free in `GPUMemoryPool::release()` (#1878).
- `[ ]` GPU memory safety: validate VRAM budget before alloc; emit `gpu_oom_total` counter on rejection.

**Tests:**
- Hardware-in-the-loop tests gated on `THEMIS_GEO_CUDA=ON`.
- CPU/GPU recall parity: recall@10 ≥ 0.95 for the same dataset on both paths.
- GPU memory safety: force `cudaErrorMemoryAllocation`; assert graceful degradation to CPU.

**Detail:** [→ src/index/FUTURE_ENHANCEMENTS.md](src/index/FUTURE_ENHANCEMENTS.md#gpu-vector-index-cuda-and-hip-backend-implementation)

---

### A-08 · `geo` — CUDA and OpenCL Production Backend
**Priority:** 🟠 High | **Target:** v1.4.0 | **Issue:** #3858
**Stub location:** `src/geo/gpu_backend_stub.cpp` — `GpuBackendRegistry` entry is described as "Simple internal registry stub"; production `gpu_backend_production.cpp` is not registered.

**Affected files:**
- `src/geo/cpu_backend.cpp` (line 914: registry stub)
- `src/geo/gpu_backend_stub.cpp`
- `src/geo/gpu_backend_cuda.cu`
- `src/geo/device_detector.cpp`

**Implementation:**
- `[ ]` Register `GpuBackendRegistry` entry pointing to real `GpuBatchBackend` on startup.
- `[ ]` `GpuBatchBackend::stBuffer()` — replace CPU fallback with audit log + GPU metrics counter.
- `[ ]` `ST_UNION` / `ST_DIFFERENCE` CUDA kernels: deferred to v2.2.0; retain CPU Greiner-Hormann with explicit `STUB/SIMULATION NOTE`.
- `[ ]` Circuit-breaker on CUDA error → structured audit entry → CPU fallback (never silently).

**Tests:**
- Force `cudaErrorNoDevice` via mock; assert fallback to `boost_cpu_exact_backend.cpp` + audit entry.
- GPU contains 1 M points: ≤ 50 ms on A10G.

**Detail:** [→ src/geo/FUTURE_ENHANCEMENTS.md](src/geo/FUTURE_ENHANCEMENTS.md#cuda-and-opencl-implementation-in-gpu_backend_productioncpp)

---

### A-09 · `aql` — Post-Generation AQL Validation
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3859
**Stub location:** `src/aql/llm_aql_handler.cpp` — `translateNLToAQL()` returns LLM output without AST-level validation; invalid AQL reaches the query engine.

**Affected files:**
- `src/aql/llm_aql_handler.cpp`
- `include/aql/llm_aql_handler.h`

**Implementation:**
- `[ ]` After LLM generation, run `AQLParser::parse()` on the result.
- `[ ]` On parse error: retry with corrective prompt (max 2 retries); on persistent failure return structured `AQLError::InvalidSyntax`.
- `[ ]` Emit `aql_validation_failures_total` counter with `reason` label.

**Tests:** Unit: inject malformed AQL from mock LLM; assert retry + structured error after 2 retries.
**Detail:** [→ src/aql/FUTURE_ENHANCEMENTS.md](src/aql/FUTURE_ENHANCEMENTS.md#1--post-generation-aql-validation-in-translatenlttoaql)

---

### A-10 · `aql` — Thread Leak in `LLMTimeoutManager::executeWithTimeout()`
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3860
**Stub location:** `src/aql/llm_aql_handler.cpp` — detached thread spawned for timeout enforcement; thread outlives the manager on destruction.

**Affected files:** `src/aql/llm_aql_handler.cpp`

**Implementation:**
- `[ ]` Replace `std::thread` + `detach()` with `std::async(std::launch::async, ...)` and `std::future::wait_for()`.
- `[ ]` On timeout: set atomic cancellation flag; calling thread returns `AQLError::Timeout`.
- `[ ]` No orphaned threads: `~LLMTimeoutManager()` joins or cancels.

**Tests:** Valgrind/ASAN: no leaked threads after 1 000 timeout invocations.
**Detail:** [→ src/aql/FUTURE_ENHANCEMENTS.md](src/aql/FUTURE_ENHANCEMENTS.md#2--eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout)

---

## Wave B — High / Near-term (v1.5.0 – v1.8.0)

> Calendar: Q3 2026 – Q1 2027. These items are required for production hardening.

---

### B-01 · `acceleration` — CUDA Kernel Completion for Vector Similarity Search
**Priority:** 🟠 High | **Target:** v1.7.0 | **Issue:** #3863
**Stub location:** `src/acceleration/ai_hardware_dispatcher.cpp` and `src/acceleration/vllm_resource_manager.cpp` — CUDA vector similarity path (`THEMIS_ENABLE_CUDA`) dispatches to CPU HNSW fallback.

**Affected files:**
- `src/acceleration/ai_hardware_dispatcher.cpp`
- `src/acceleration/graphics_backends.cpp`

**Implementation:**
- `[ ]` Implement `CudaVectorSimilarityBackend::search()` using FAISS GPU `IndexFlatL2` / `GpuIndexIVFFlat`.
- `[ ]` Add `THEMIS_ENABLE_CUDA` compile gate; CPU path unchanged when gate is off.
- `[ ]` `VLLMResourceManager`: replace mock CPU/RAM monitoring with `sysinfo()` (Linux) / `GetSystemInfo()` (Windows).

**Tests:** Recall@10 ≥ 0.90 GPU vs. CPU on ANN-benchmarks dataset; throughput ≥ 10 000 QPS on RTX-class GPU.
**Detail:** [→ src/acceleration/FUTURE_ENHANCEMENTS.md](src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-kernel-completion-for-vector-similarity-search)

---

### B-02 · `analytics` — `ExporterFactory` Stub Replacement ✅ Done (verified 2026-06-18)
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3868
**Previous stub location:** `src/analytics/analytics_export.cpp` line 728 — historical note (now replaced).

**Affected files:** `src/analytics/analytics_export.cpp`

**Implementation:**
- `[x]` Switch on `format`; return concrete exporters (`JSONCSVExporter`, `ArrowIPCExporter`, `ParquetExporter`, `FeatherExporter`).
- `[x]` When `THEMIS_HAS_ARROW` is not defined: fail fast with explicit runtime error via `throwArrowUnavailable(...)`.
- `[x]` Unit coverage validates Arrow export factory behavior (`tests/analytics/test_arrow_export.cpp`).

**Verification (2026-06-18):**
- `ctest --preset windows-release --output-on-failure -R "^ArrowExportFocusedTests$|^test_arrow_export_AnalyticsFocusedTests$" -j 1` → 2/2 passed.

**Tests:** Export-format specific focused tests passing; round-trip coverage remains in `tests/analytics/test_arrow_export.cpp`.
**Detail:** [→ src/analytics/FUTURE_ENHANCEMENTS.md](src/analytics/FUTURE_ENHANCEMENTS.md#1--exporterfactory-stub-replacement)

---

### B-03 · `analytics` — `KNNRegressorModel::predictOneReg()` Stub — ✅ Completed alongside LRModel fix
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #3968

> **Status:** `KNNModel::predictOneReg` was already implemented (inverse-distance-weighted mean).
> `LRModel::predictOneReg` returned `0.0` (stub); replaced — see below.

### B-03b · `analytics` — `LRModel::predictOneReg()` Stub ✅ Done (2026-04-16)
**Priority:** 🟡 Medium | **Target:** v1.8.0
**Stub location:** `src/analytics/automl.cpp` line 832 — `return 0.0` unconditionally.

**Fix:** Compute the expected class value using `LogisticRegression::predictProbaOne(x)`:
`v = Σ c * P(class=c)` for all classes c. For binary classification this equals P(class=1).

**Tests added:** `LRModelRegressorTest::PredictOneRegNotZeroStub` and `PredictOneRegRangeMonotonic`
in `tests/analytics/test_automl.cpp`.

**Affected files:** `src/analytics/automl.cpp`

**Implementation:**
- `[x]` Implement real k-NN regression: Euclidean distance to `k` nearest training points; weighted average of labels.
- `[x]` Wired in `KNNModel::predictOneReg(...)` (inverse-distance weighting).

**Tests:** MAE < 0.05 on held-out synthetic dataset; performance: ≤ 1 ms per prediction at k=5, N=10 000.
**Detail:** [→ src/analytics/FUTURE_ENHANCEMENTS.md](src/analytics/FUTURE_ENHANCEMENTS.md#10--automlcpp--knnregressormodelpredictoreg-stub)

---

### B-04 · `api` — gRPC API Surface: Wire Stub Implementations
**Priority:** 🟠 High | **Target:** v2.0.0 | **Issue:** #3879
**Stub location:** `src/api/themisdb_grpc_service.cpp` and `src/api/grpc_server.cpp` — gRPC RPC methods return `grpc::Status::OK` with empty responses or `UNIMPLEMENTED`.

**Affected files:**
- `src/api/themisdb_grpc_service.cpp`
- `src/api/grpc_server.cpp`
- `include/api/themisdb_grpc_service.h`
- `src/main_server.cpp`

**Implementation:**
- `[x]` Register `ThemisDBGrpcService` at server startup when gRPC stubs are present; share the same gRPC server bootstrap path with WAL service and skip startup only when neither service is available.
- `[~]` Wire each RPC method to its corresponding service handler (query engine, ingestion, admin).
	- Progress: `VectorSearch`, `FilteredVectorSearch`, `HybridSearch` now honor `fetch_docs` by resolving and attaching document payloads via storage backend.
	- Progress: `transaction_id` is now resolved to an active `TransactionManager::Transaction` in CRUD/Batch/AQL RPCs (format + active transaction existence checks).
	- Progress: write RPCs (`CreateDocument`, `UpdateDocument`, `DeleteDocument`, `BatchWrite`) now execute through transaction-bound handlers (`putEntity`/`eraseEntity`) when `transaction_id` is provided, instead of direct storage writes.
	- Progress: read RPCs (`GetDocument`, `BatchRead`) now use transaction-scoped snapshot reads for active `transaction_id` values via `Transaction::readEntityJson` (read-your-writes semantics).
	- Progress: non-transactional reads now include a compatibility fallback for canonical transaction-persisted entity keys (`entity:{collection}:{key}`).
	- Progress: `fetch_docs` document hydration in `VectorSearch`, `FilteredVectorSearch`, and `HybridSearch` now uses the same canonical entity-key fallback, so transaction-committed entities are resolved consistently.
	- Progress: `FilteredVectorSearch` now applies server-side `_key`/`_id` filters (`eq`, `ne`, `in`) as a production fallback while non-supported filter clauses remain explicitly warned and ignored.
	- Progress: `FilteredVectorSearch` also applies top-level attribute filters via document fallback resolution when storage is wired: string (`eq`, `ne`, `in`), numeric `in`, and numeric comparisons (`gt`, `gte`, `lt`, `lte`); numeric `eq`/`ne`/`in` use tolerant floating-point matching; mixed-type/non-scalar `in` arrays are treated as unsupported and ignored with warning.
	- Progress: `VectorIndexAdapter` no longer ignores the optional `IExpressionEvaluator* filter`; it now forwards to evaluator-aware `VectorIndexManager` search paths (`searchKnnEvaluated`, `searchKnnRadiusEvaluated`) for JSON-context filter execution.
	- Progress: evaluator API contract is now const-correct end-to-end (`IExpressionEvaluator::evaluate(... ) const`) across query/storage/api paths and test/example implementations; temporary `const_cast` workarounds in vector evaluator paths were removed.
	- Test status: proto-backed RPC transaction path tests are active in `ThemisDBGrpcServiceTests` (`GrpcTransactionPathTest.*`) and now validate deferred write/delete visibility, transactional read-your-writes (`GetDocument`, `BatchRead`) and non-transactional entity-key fallback reads.
	- Test status: additional RPC regression tests validate `fetch_docs` fallback behavior plus `_key`/`_id`, top-level string attribute filtering, top-level numeric `in`/comparison filters (including floating-point tolerance checks for `eq`/`ne`/`in`), and mixed-type/invalid numeric `in` hardening for vector search endpoints (`GrpcVectorFetchDocsFallbackTest.*`).
	- Test status: new DI regression `IndexManagerDI.VectorIndexAdapter_AppliesJsonContextEvaluatorFilter` verifies adapter-level forwarding of JSON-context evaluators.
- `[x]` Propagate `ThemisError` → gRPC transport status in AQL execution paths (`ExecuteAQL`, `StreamAQL`, `FullTextSearch`) with canonical `ErrorCode` mapping.
- `[x]` Add per-RPC Prometheus counters: `grpc_requests_total{method, status}` in `ThemisDBGrpcService`, including registry wiring from `main_server`.
- `[x]` TLS: enforce `fail_closed` by default for gRPC startup (no implicit insecure fallback) and add SIGHUP-driven gRPC certificate hot-reload via controlled server restart.

**Verification (2026-06-18):**
- `cmake --build --preset windows-release --target themis_server --parallel 16` → success.
- `cmake --build --preset windows-release --target test_index_manager_di test_themisdb_grpc_service --parallel 16` → success.
- `build-msvc-windows-release/bin/test_index_manager_di.exe --gtest_filter=IndexManagerDI.VectorIndexAdapter_AppliesJsonContextEvaluatorFilter` → passed (1/1).
- `build-msvc-windows-release/bin/test_themisdb_grpc_service.exe --gtest_filter=GrpcVectorFetchDocsFallbackTest.*` → passed (16/16).
- `ctest --preset windows-release --output-on-failure -R "^GrpcApiServerTests$|^ThemisDBGrpcServiceTests$|^TransactionManagerFocusedTests$" -j 1` → 3/3 passed.

**Tests:** End-to-end gRPC test: `ExecuteAQL`, `IngestDocument`, `GetDocument`, `DeleteCollection` RPCs; test `UNIMPLEMENTED` RPCs no longer reachable.
**Detail:** [→ src/api/FUTURE_ENHANCEMENTS.md](src/api/FUTURE_ENHANCEMENTS.md#grpc-api-surface--wire-stub-implementations)

---

### B-05 · `content` — Abuse Detection Stub Replacement
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3889
**Stub location:** `src/content/content_security.cpp` line 150 and line 421 — abuse detection always returns `PASS`; CSAM / spam content not detected.

**Affected files:**
- `src/content/content_security.cpp`
- `include/content/ocr_processor.h` (image hash interface)

**Implementation:**
- `[ ]` Define `IAbuseDetector` interface: `detect(data, metadata) → AbuseDetectionResult{action, reason, hash}`.
- `[ ]` Implement `PhotoDNAAbuseDetector`: perceptual hash comparison against configurable blocklist YAML.
- `[ ]` Implement `TextAbuseDetector`: pattern/regex blocklist from `config/security/abuse_patterns.yaml`; `BLOCK` and `FLAG` actions.
- `[ ]` Wire both into `ContentSecurity::check()` at stub line 150.
- `[ ]` Audit log every detection via `AuditLogger::logEvent()` (content hash, detector type, action).

**Tests:** `BLOCK` path: content rejected; `FLAG` path: content stored with flag metadata. Both paths audited.
**Detail:** [→ src/content/FUTURE_ENHANCEMENTS.md](src/content/FUTURE_ENHANCEMENTS.md#abuse-detection-stub-replacement)

---

### B-06 · `governance` — OPA Adapter: HTTP Client Stub Replacement
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4004
**Stub location:** `src/governance/opa_adapter.cpp` line 218 — `OpaAdapter::evaluate()` uses a hard-coded HTTP simulation when `THEMIS_ENABLE_OPA` is not defined.

**Affected files:** `src/governance/opa_adapter.cpp`, `include/governance/opa_adapter.h`

**Implementation:**
- `[ ]` Under `THEMIS_ENABLE_OPA`: wire libcurl or gRPC call to real OPA REST API (`/v1/data/...`).
- `[ ]` On OPA unavailable: emit `governance_opa_fallback_total` counter; fall back to native evaluation with logged warning.
- `[ ]` Add mTLS option for OPA endpoint (`opa_tls_cert_path` config key).

**Tests:** Mock HTTP server returns allow/deny; integration test against real OPA in Docker CI.
**Detail:** [→ src/governance/FUTURE_ENHANCEMENTS.md](src/governance/FUTURE_ENHANCEMENTS.md)

---

### B-07 · `ingestion` — `LLMIngestionAdapter` Phase 2: Wire llama.cpp
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3904
**Stub location:** `src/ingestion/` — `LLMIngestionAdapter` Phase 1 used `NullTextGenerationBackend`; Phase 2 must wire real llama.cpp backend.

**Affected files:**
- `src/ingestion/llm_ingestion_adapter.cpp`
- `include/ingestion/inference_backend.h`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_LLAMA_CPP && THEMIS_ENABLE_LLM`.
- `[ ]` Inject real `ITextGenerationBackend` backed by `LlamaCppPlugin`.
- `[ ]` `NullTextGenerationBackend` remains available only under the `STUB/SIMULATION NOTE` contract for test injection.
- `[ ]` Batch size and timeout configurable via `ingestion_config.yaml`.

**Tests:**
- Unit: inject `NullTextGenerationBackend`; assert ingestion completes without LLM.
- Integration: tiny GGUF model in CI; assert non-empty entity extraction result.

**Detail:** [→ src/ingestion/FUTURE_ENHANCEMENTS.md](src/ingestion/FUTURE_ENHANCEMENTS.md#llmingestionadapter-phase-2-wire-llamacpp)

---

### B-08 · `ingestion` — Connector Mock Paths: Production Wiring
**Priority:** 🟡 Medium | **Target:** v1.7.0 – v1.8.0 | **Issues:** multiple
**Stub locations:**

| Connector | File | Stub Location | Production Target |
|-----------|------|---------------|-------------------|
| S3 | `src/ingestion/s3_connector.cpp:326` | `ingestFromMock()` path | AWS SDK `s3_client->GetObject()` |
| S3 | `src/ingestion/s3_connector.cpp:492` | Listing stub | AWS SDK `ListObjectsV2` |
| Kafka | `src/ingestion/kafka_connector.cpp:238` | `ingestFromMock()` | librdkafka `RdKafka::Consumer` |
| Object Storage | `src/ingestion/object_storage_connector.cpp:273` | Mock path | GCS / Azure SDK |
| Database | `src/ingestion/database_connector.cpp:458` | Mock ODBC path | Real ODBC via `THEMIS_ENABLE_ODBC` |
| CDC | `src/ingestion/cdc_connector.cpp:563` | Mock CDC path | Debezium / real DB WAL |

**Implementation per connector:**
- `[ ]` S3: wire under `THEMIS_ENABLE_AWS_SDK`; mock-injection path retained for unit tests.
- `[ ]` Kafka: wire under `THEMIS_ENABLE_KAFKA`; mock-injection path retained for unit tests.
- `[ ]` Object Storage: wire under `THEMIS_ENABLE_GCS` / `THEMIS_ENABLE_AZURE`.
- `[ ]` Database: wire full ODBC under `THEMIS_ENABLE_ODBC`.
- `[ ]` CDC: wire Debezium events or WAL-based CDC under `THEMIS_ENABLE_CDC`.
- `[ ]` All mock paths annotated with `STUB/SIMULATION NOTE` (already done for format; verify completeness).

**Tests:** Each connector: 28–32 unit tests via mock-injection (no cloud credentials required); Docker-compose integration tests for S3 (MinIO) and Kafka.

---

### B-09 · `llm` — `LoraSecurityValidator`: Certificate Store Integration
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3906
**Stub location:** `src/llm/multi_lora_manager.cpp` line 392 and 462 — `LoraSecurityValidator` validates LoRA adapter files but the certificate chain is verified against a hard-coded base64 hash comparison, not a real cert store.

**Affected files:**
- `src/llm/multi_lora_manager.cpp`
- `include/llm/lora_framework/lora_metrics.h`

**Implementation:**
- `[ ]` Integrate OpenSSL `X509_STORE` for certificate chain validation.
- `[ ]` Load trusted CA bundle from `config/security/lora_trusted_cas.pem` (configurable path).
- `[ ]` On cert validation failure: reject LoRA load; emit `lora_cert_rejected_total` counter.
- `[ ]` CRL check: use OCSP stapling when available; fall back to CRL distribution point.

**Tests:** Valid cert chain → load succeeds. Expired / revoked cert → load rejected + counter incremented.
**Detail:** [→ src/llm/FUTURE_ENHANCEMENTS.md](src/llm/FUTURE_ENHANCEMENTS.md#lorasecurityvalidator-certificate-store-integration)

---

### B-10 · `llm` — `LLMDeploymentPlugin`: RocksDB Model Storage
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4011
**Stub location:** `src/llm/lora_framework/lora_storage_service_themisdb.cpp` — model metadata persisted in in-memory map; survives only for process lifetime.

**Affected files:**
- `src/llm/lora_framework/lora_storage_service_themisdb.cpp`
- `include/llm/active_vram_allocator.h`

**Implementation:**
- `[ ]` Replace in-memory map with RocksDB KV store keyed by model ID.
- `[ ]` Atomic write via `rocksdb::WriteBatch`.
- `[ ]` On restart: restore map from RocksDB scan.

**Tests:** Crash recovery: write 10 models, kill process, restart, assert all 10 recoverable.
**Detail:** [→ src/llm/FUTURE_ENHANCEMENTS.md](src/llm/FUTURE_ENHANCEMENTS.md#llmdeploymentplugin-rocksdb-model-storage)

---

### B-11 · `llama_cpp` — Real `generate()` Inference via LlamaWrapper
**Priority:** 🟠 High | **Target:** Q3 2026 | **Issue:** (llama_cpp module)
**Stub location:** `src/llama_cpp/llama_cpp_plugin.cpp` — `generate()` returns echo stub; `embed()` returns 384-dim zero vector.

**Affected files:**
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `include/llama_cpp/llama_cpp_plugin.h`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_LLAMA_CPP`.
- `[ ]` `generate()`: delegate to `LlamaWrapper::generate()`; increment `inference_count_`.
- `[ ]` `embed()`: delegate to `LlamaWrapper::embed()`; return real 384-dim (or model-defined-dim) vector.
- `[ ]` Stub mode (`loadModel("")`) must remain functional; all stub paths annotated.
- `[ ]` `exportLoRA()` / `importLoRA()`: serialize/deserialize LoRA weights (GGUF-compatible); magic-byte validation before deserialisation.

**Tests:** Integration test with tiny GGUF model in CI fixtures. Perf: ≤ 200 ms for 50-token prompt on RTX 3090 equivalent.
**Detail:** [→ src/llama_cpp/FUTURE_ENHANCEMENTS.md](src/llama_cpp/FUTURE_ENHANCEMENTS.md#1-real-llamacpp-inference-via-llamawrapper-target-q3-2026)

---

### B-12 · `query` — `QueryOptimizer`: Wire Real MetadataShard and Statistics
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3918
**Stub location:** `src/query/query_optimizer.cpp` — `MetadataShard`, Prometheus handle, and column statistics are injected as null/mock objects; cost model uses constant estimates.

**Affected files:**
- `src/query/query_optimizer.cpp`
- `include/query/query_optimizer.h`

**Implementation:**
- `[ ]` Accept `IMetadataShard*` via constructor; assert non-null in production builds.
- `[ ]` Pull column statistics (cardinality, min/max/histogram) from `MetadataShard::getStats()`.
- `[ ]` Emit `query_optimizer_plan_cost_estimate` gauge to Prometheus.

**Tests:** Cost-based join order test: 3-table join; assert optimizer chooses lower-cost plan when statistics favour it.
**Detail:** [→ src/query/FUTURE_ENHANCEMENTS.md](src/query/FUTURE_ENHANCEMENTS.md#queryoptimizer-wire-real-metadatashard-prometheus-and-statistics)

---

### B-13 · `query` — `QueryFederation`: Real Shard Determination Logic
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3919
**Stub location:** `src/query/cte_subquery.cpp` — `QueryFederation::determineShard()` broadcasts to all shards regardless of query predicates.

**Affected files:**
- `src/query/cte_subquery.cpp`
- `include/query/cross_cluster_federation.h`

**Implementation:**
- `[ ]` Implement shard-key extraction from `WHERE` predicates.
- `[ ]` Map shard-key value to shard ID via consistent-hash ring from `IShardManager`.
- `[ ]` Fan-out only to relevant shards; merge results with `MergeSort`.

**Tests:** Shard routing test: query with exact shard-key match routes to exactly 1 shard; range query routes to correct subset.
**Detail:** [→ src/query/FUTURE_ENHANCEMENTS.md](src/query/FUTURE_ENHANCEMENTS.md#queryfederation-real-shard-determination-logic)

---

### B-14 · `query` — `CTESubquery`: Replace Phase 1 Stub
**Priority:** 🟡 Medium | **Target:** v1.7.0 | **Issue:** #4025
**Stub location:** `src/query/cte_subquery.cpp` — CTE materialisation Phase 1 caches results in `std::unordered_map<string, json>`; no spill-to-disk, no streaming.

**Affected files:** `src/query/cte_subquery.cpp`, `include/query/cte_subquery.h`

**Implementation:**
- `[ ]` Spill to RocksDB when in-memory size exceeds `cte_memory_limit_mb` config.
- `[ ]` Streaming CTE: yield rows incrementally instead of materialising full result.

**Tests:** 10 M row CTE: assert spill occurs; result identical to in-memory path.
**Detail:** [→ src/query/FUTURE_ENHANCEMENTS.md](src/query/FUTURE_ENHANCEMENTS.md#ctesubquery-replace-phase-1-stub)

---

### B-15 · `rag` — `LLMIntegration` / `LLMJudgeIntegration`: Replace Mock Mode
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3925
**Stub location:** `src/rag/llm_judge_integration.cpp` — mock mode returns fixed evaluation scores; used in production RAG evaluation when real LLM is unavailable.

**Affected files:**
- `src/rag/llm_judge_integration.cpp`
- `include/rag/llm_judge_integration.h`
- `include/rag/hybrid_retriever.h`

**Implementation:**
- `[ ]` Remove implicit mock-mode fallback from production build path.
- `[ ]` Provide explicit `LLMJudgeMock` class injectable only in tests.
- `[ ]` Production path: call real `ILLMPlugin::generate()` via `LLMIntegration`.
- `[ ]` On LLM unavailable: return `RAGError::JudgeUnavailable` (not silent mock scores).

**Tests:** Integration: real LLM client via `openai_compat_adapter.cpp`; assert non-trivial scores. Unit: inject `LLMJudgeMock`.
**Detail:** [→ src/rag/FUTURE_ENHANCEMENTS.md](src/rag/FUTURE_ENHANCEMENTS.md#llmintegration-and-llmjudgeintegration-replace-stubmock-mode-with-real-engine)

---

### B-16 · `security` — `ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3930
**Stub location:** `src/security/` — `ArrowUserRegistrationPlugin` is a complete stub; user store backed by in-memory vector.

**Affected files:**
- `src/security/hsm_provider.cpp` (lines 32+)
- `src/security/hsm_provider_pkcs11.cpp` (lines 56+)
- `src/security/timestamp_authority.cpp` (lines 28+)

**Implementation:**
- `[ ]` Arrow plugin: back user store with Apache Arrow `RecordBatch`; persist via `ArrowFileWriter`.
- `[ ]` HSM provider: wire to real PKCS#11 token under `THEMIS_ENABLE_HSM`; stub path annotated with `STUB/SIMULATION NOTE`.
- `[ ]` Timestamp authority: wire to real RFC 3161 TSA HTTP endpoint; stub path annotated.
- `[ ]` PKIClient stub (`src/utils/pki_client.cpp`): replace base64-hash fallback with real X.509 certificate verification via OpenSSL.

**Tests:** HSM: SoftHSM2 in CI Docker; Arrow: round-trip 1 000 users, restart, verify.
**Detail:** [→ src/security/FUTURE_ENHANCEMENTS.md](src/security/FUTURE_ENHANCEMENTS.md#arrowuserregistrationplugin-implement-apache-arrow-integration)

---

### B-17 · `server` — `HttpServer`: Initialize Real `ShardingManager`
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4032
**Stub location:** `src/server/grpc_web_proxy_handler.cpp` and `src/api/http_server.cpp` — `ShardingManager` pointer is null; shard-aware routing is skipped.

**Affected files:** `src/api/http_server.cpp`, `src/server/`

**Implementation:**
- `[ ]` Inject real `IShardingManager` via `HttpServer::setShardingManager()`.
- `[ ]` Route shard-key requests to correct shard; non-shard requests to local handler.

**Tests:** Integration: 3-shard cluster; key-based request routes to correct shard.
**Detail:** [→ src/server/FUTURE_ENHANCEMENTS.md](src/server/FUTURE_ENHANCEMENTS.md#httpserver-initialize-real-shardingmanager)

---

### B-18 · `sharding` — `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #3936
**Stub location:** `src/sharding/cloud_backup.cpp` lines 70, 220, 326 — `GpuErasureCoderOpenCL::encode()` / `decode()` / `repair()` return CPU-fallback no-ops.

**Affected files:** `src/sharding/cloud_backup.cpp`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_OPENCL`.
- `[ ]` Implement Reed-Solomon encode/decode/repair using OpenCL kernels.
- `[ ]` CPU path retained as `GpuErasureCoder` default when OpenCL unavailable.

**Tests:** Erasure encode + introduce 2 shard failures + repair: assert bitwise identical recovery.
**Detail:** [→ src/sharding/FUTURE_ENHANCEMENTS.md](src/sharding/FUTURE_ENHANCEMENTS.md#gpuerasurecoderopencl-implement-opencl-encodedecode)

---

### B-19 · `training` — `ProvenanceTracker`: Replace AQL Template Stubs
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #3951
**Stub location:** `src/training/provenance_tracker.cpp` — AQL queries templated with placeholder collection names; `knowledge_graph_enricher.cpp` uses hard-coded stub enrichment.

**Affected files:**
- `src/training/provenance_tracker.cpp`
- `src/training/knowledge_graph_enricher.cpp`

**Implementation:**
- `[ ]` Wire `ProvenanceTracker` to real `IQueryExecutor` via constructor injection.
- `[ ]` Collection names resolved from `TrainingConfig::provenance_collection`.
- `[ ]` `KnowledgeGraphEnricher`: wire to real `IGraphWriter` from `IngestionToolbox`.

**Tests:** Integration: ingest 5 training examples, enrich graph, query provenance; assert graph edges exist.
**Detail:** [→ src/training/FUTURE_ENHANCEMENTS.md](src/training/FUTURE_ENHANCEMENTS.md#provenancetracker-replace-aql-template-stubs-with-live-connection)

---

### B-20 · `utils` — `PKIClient`: Replace Fallback Stub Verification
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4049
**Stub location:** `src/utils/pki_client.cpp` — certificate verification falls back to base64 hash comparison when OpenSSL verification fails; this is a stub, not a secure fallback.

**Affected files:** `src/utils/pki_client.cpp`, `include/utils/pki_client.h`

**Implementation:**
- `[ ]` Remove base64-hash fallback from production build.
- `[ ]` On OpenSSL verification failure: return `PKIError::VerificationFailed` (hard error).
- `[ ]` CRL / OCSP check added to `PKIClient::verify()`.

**Tests:** Self-signed cert (no trusted CA): verify returns error, not fallback success.
**Detail:** [→ src/utils/FUTURE_ENHANCEMENTS.md](src/utils/FUTURE_ENHANCEMENTS.md#pkiclient-replace-fallback-stub-verification)

---

### B-21 · `performance` — Advanced Cache Manager Stub
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** (performance module)
**Stub location:** `src/performance/advanced_cache_manager.cpp` line 92 — ML-based prefetch predictor returns fixed access pattern.

**Affected files:** `src/performance/advanced_cache_manager.cpp`

**Implementation:**
- `[ ]` Replace fixed pattern with sliding-window access frequency model.
- `[ ]` Gate on `THEMIS_ENABLE_ML_CACHE`; fixed pattern retained as compile-time fallback.

**Tests:** Prefetch hit rate ≥ 60% on realistic access trace.
**Detail:** [→ src/performance/FUTURE_ENHANCEMENTS.md](src/performance/FUTURE_ENHANCEMENTS.md#advanced-cache-optimization)

---

### B-22 · `content` — `TextProcessor::generateEmbedding` Stub
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** (content module)
**Stub location:** `src/content/text_processor.cpp` line 207 — `generateEmbedding()` returns a random or zero vector.

**Affected files:** `src/content/text_processor.cpp`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_EMBEDDING`.
- `[ ]` Delegate to `IEmbeddingBackend` injected via `TextProcessor::setEmbeddingBackend()`.
- `[ ]` Default to zero-vector stub only when backend is null (test-only path).

**Tests:** Cosine similarity between related documents > 0.7; unrelated documents < 0.3.
**Detail:** [→ src/content/FUTURE_ENHANCEMENTS.md](src/content/FUTURE_ENHANCEMENTS.md#embedding-generation-pipeline-text--vector)

---

## Wave C — Medium / Long-term (v1.9.0+)

> Calendar: Q2 2027+. Scheduled opportunistically; no release-blocking status.

---

### C-01 · `security` — SPHINCS+ Production Implementation (liboqs)
**Priority:** 🟡 Medium | **Target:** v2.0.0+
**Stub location:** `src/security/post_quantum_crypto.cpp` lines 923–1000 — `SphincsPlus` class uses OpenSSL Ed25519 simulation for SPHINCS+-SHA2-256s/256f; documented `STUB/SIMULATION NOTE`.
**Activation:** Replace when `liboqs` ≥ 0.10.0 is available in `vcpkg.json`.

**Implementation:**
- `[ ]` Add `liboqs` to `vcpkg.json` and `CMakeLists.txt` under `THEMIS_ENABLE_LIBOQS`.
- `[ ]` Replace `OQS_SIG_sphincs_sha2_256s_sign()` / `verify()` with real liboqs calls.
- `[ ]` Retain Ed25519 simulation under `STUB/SIMULATION NOTE` for environments without liboqs.

**Tests:** CPU/liboqs sign+verify parity; known-answer tests from NIST PQC test vectors.

---

### C-02 · `acceleration` — OpenGL Compute Shader Backend: 5 Remaining Stubs
**Priority:** 🟢 Low | **Target:** v2.0.0 | **Issue:** #4065
**Stub location:** `src/acceleration/graphics_backends.cpp` — OpenGL compute shader backend has 5 unimplemented kernels.

**Implementation:**
- `[ ]` Implement 5 compute shaders (`vector_add`, `matrix_mul`, `reduce_sum`, `scan`, `sort`) in GLSL 4.3 compute.
- `[ ]` Gate on `THEMIS_ENABLE_OPENGL`.

---

### C-03 · `analytics` — Windows Platform Stubs
**Priority:** 🟢 Low | **Target:** v2.0.0 | **Issue:** #4068
**Stub location:** `src/analytics/olap.cpp:53` and `src/analytics/process_mining.cpp:24` — `#if defined(_WIN32)` stubs emit `spdlog::error` but return empty results.

**Implementation:**
- `[ ]` Port SIMD paths to Windows-compatible intrinsics (`<intrin.h>`).
- `[ ]` Add Windows CI job; set stub-count gate ≤ 0 for non-Windows builds.

---

### C-04 · `whisper` — Real Diarisation Backend
**Priority:** 🟢 Low | **Target:** v2.1.0+
**Stub location:** `src/whisper/whisper_plugin.cpp` line 39 — diarisation returns preset stub fixtures.

**Implementation:**
- `[ ]` Integrate `pyannote.audio` inference via subprocess or embedded Python.
- `[ ]` Gate on `THEMIS_ENABLE_DIARISATION`.

**Tests:** 5 unit tests with stub returning preset fixtures (keep); integration test with real audio file.

---

### C-05 · `performance` — Phase 4 PMU Counters: Non-Linux Stub Coverage
**Priority:** 🟢 Low | **Target:** v1.9.0 | **Issue:** #4086
**Stub location:** `src/performance/phase4/pmu_counters.cpp` — PMU counter reads return 0 on non-Linux platforms.

**Implementation:**
- `[ ]` macOS: use `kperf` framework for cycle / instruction counters.
- `[ ]` Windows: use `QueryThreadCycleTime` / `QueryPerformanceCounter`.

---

### C-06 · `llm` — `VisionEncoder`: Checksum Verification ✅ Done (2026-04-16)
**Priority:** 🟢 Low | **Was:** `src/llm/vision_encoder.cpp` line 117 — `// TODO: Implement checksum verification`.

**Fix:** Implemented SHA-256 sidecar file verification using `ModuleHashVerifier::computeSHA256()`.
Convention: `<model_path>.sha256` sidecar contains the expected hex digest.
On mismatch: `std::runtime_error` with file path and both hashes.
On missing sidecar: warns and continues (non-fatal, matches existing behaviour when verification config is absent).
Gate: only runs when `config_->isModelVerificationEnabled()` && `mv.verify_checksums` are both true.

---

## Cross-Cutting Epics

### Epic: Stub/Mock Replacement
**Label:** `epic:stub-replacement` | **Target:** v1.8.0
Covers all items with label `stub-replacement` in `src/ROADMAP.md`.
Most critical items: A-06 (GPU sort/join/BLAS), A-07 (index GPU), A-08 (geo GPU), B-04 (gRPC), B-05 (abuse detection), B-07 (LLM ingestion), B-09 (LoRA certs), B-15 (RAG mock).

### Epic: Security Hardening
**Label:** `epic:security-hardening` | **Target:** v1.8.0
Affects: `auth`, `security`, `server`, `llm`, `utils`, `sharding`, `storage`.
Critical items: A-01 (JWT mutex), A-02 (LDAP injection), A-03 (timing), B-09 (LoRA certs), B-16 (Arrow/HSM), B-20 (PKI stub).

### Epic: GPU Compute
**Label:** `epic:gpu-compute` | **Target:** v1.7.0 – v1.8.0
Covers all GPU backend stubs: A-06, A-07, A-08, B-01, B-18.
All GPU items require `THEMIS_ENABLE_CUDA` / `THEMIS_ENABLE_HIP` / `THEMIS_ENABLE_OPENCL` gates.
CPU fallback must remain available and tested independently.

### Epic: Thread-Safety
**Label:** `epic:thread-safety` | **Target:** v1.8.0
All exclusive-mutex read paths upgraded to `std::shared_mutex`:
`analytics` (#41–45), `plugins` (#193), `maintenance` `schedules_mutex_` (#185), `graph` `DistributedGraphManager` (#174), `config` `ConfigEncryptedStore` (#164).

### Epic: AQL 2.0.0 — Complete Language Standard
**Label:** `epic:aql-2.0.0` | **Target:** Q4 2026 (18–23 weeks)

**Scope:** Full AQL standard coverage across 4 major features:
1. **Mutations** (DML: INSERT, UPDATE, DELETE, REPLACE, REMOVE, UPSERT) — 12–15 weeks
2. **DDL** (CREATE/DROP/ALTER COLLECTION/INDEX/VIEW) — 4–6 weeks  
3. **Geospatial** (Parser integration of existing ST_* functions) — 2–3 weeks
4. **FTS** (Full-text search query enhancement) — 2–3 weeks
5. **Integration & Testing** (Cross-feature tests, performance gates) — 3–4 weeks

**Roadmap Files:**
- [src/query/AQL_MUTATIONS_ROADMAP.md](src/query/AQL_MUTATIONS_ROADMAP.md) — 5-phase plan for DML implementation
- [src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md](src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md) — Master roadmap with 6 go/no-go gates + codebase audit findings
- [src/query/AQL_DDL_ROADMAP.md](src/query/AQL_DDL_ROADMAP.md) — *Pending Team B assignment*
- [src/geospatial/AQL_GEOSPATIAL_ROADMAP.md](src/geospatial/AQL_GEOSPATIAL_ROADMAP.md) — *Pending Team C assignment* (functions 70% exist, need parser integration)
- [src/index/AQL_FTS_ROADMAP.md](src/index/AQL_FTS_ROADMAP.md) — *Pending Team C assignment*

**Audit Findings (2026-06-18):**
- ⚠️ **Documentation-Implementation Gap**: AQL parser currently read-only (v1.x); DML/DDL/Geospatial keywords documented in `docs/de/aql/` but NOT implemented in parser
- ✅ **Geospatial Functions Exist**: ST_Distance, ST_Within, ST_Contains, ST_Intersects, ST_GeomFromGeoJSON in `src/query/let_evaluator.cpp` (70% complete, only need parser wiring)
- ✅ **SQL DML Parser Reference**: `src/query/sql_parser.cpp` provides reference architecture for mutation AST construction
- ✅ **Transaction Foundation**: BEGIN/COMMIT/ROLLBACK tokenization + multi-statement execution ready in `src/query/aql_runner.cpp`
- ⚠️ **Timeline Reduction**: Geospatial effort reduced 40% (2 weeks saved) due to existing functions → **18–23 weeks total (vs. 20–25 weeks original)**

**Critical Action Items:**
- `[ ]` **P0 (This Week)**: Update ROADMAP.md + FUTURE_ENHANCEMENTS.md with AQL 2.0.0 section
- `[ ]` **P0 (This Week)**: Update `docs/de/aql/AQL_COMPLETE_LANGUAGE_SCOPE.md` — mark as v1.3.1 proposal, note parser not implemented
- `[ ]` **P1 (Next Sprint)**: Create DDL, Geospatial, FTS roadmap documents
- `[ ]` **P1 (Next Sprint)**: Assign Teams A/B/C; Phase 1 kickoff for Mutations parser
- See: [DOCUMENTATION_AUDIT_2026_06_18.md](DOCUMENTATION_AUDIT_2026_06_18.md) for full audit report

**Performance Gates (v2.0.0 Release Requirements):**
- **Gate 1 (Week 3)**: Mutations parser complete; tokenizer + AST nodes + 100+ parser tests
- **Gate 2 (Week 8)**: Mutations executor working; INSERT/UPDATE/DELETE produce correct results in 50+ integration tests
- **Gate 4 (Week 12)**: Geospatial performance ≥ 50× vs. O(n²) baseline on 100K geometries
- **Gate 5 (Week 14)**: FTS phrase queries ≤ 100ms on 100K documents
- **Gate 6 (Week 22)**: All 1000+ cross-feature tests pass; zero regressions vs. v1.x; release candidate ready

---

## Definition of Done

A stub replacement item is **Done** (`[x]`) when ALL of the following are true:

1. **No stub in production path**: the `#ifdef`-guarded or null-check stub body is unreachable in a production build (i.e., with all feature gates enabled).
2. **Stub retained for tests only**: where a controlled test-double is needed, it is injectable via constructor/setter and annotated with `STUB/SIMULATION NOTE:`.
3. **Tests green**: all Phase 4 tests pass in CI (unit + integration + regression).
4. **Performance gate met**: measured metric meets the target stated in this document.
5. **Observability added**: at least one Prometheus counter or gauge for the new production path.
6. **No new security vulnerabilities**: `parallel_validation` (CodeQL + Code Review) passes.
7. **Docs updated**: `src/<module>/ROADMAP.md` item marked `[x]`; `src/<module>/AUDIT.md` open item closed; this file updated.

---

## Governance and Tracking

| Document | Purpose |
|----------|---------|
| `src/ROADMAP.md` | Master backlog (276 items, all modules) |
| `src/<module>/FUTURE_ENHANCEMENTS.md` | Module-level detail, acceptance criteria, API sketches |
| `src/<module>/AUDIT.md` | Per-module audit trail, open items, security findings |
| `FUTURE_ENHANCEMENTS.md` (this file) | Root-level stub replacement matrix, wave prioritisation |
| `FEATURE_ENHANCEMENT.md` | Generated code-maturity snapshot (reporting only, non-canonical for planning) |
| GitHub Issues `#3825–#4092` | One issue per backlog item; label `stub-replacement` |

**Issue template:**
```
## Summary
<Item title from this matrix>

## Module
`src/<module>/`

## Priority / Target Version
<Wave> · <Priority> · <Target Version>

## Stub Location
<file>:<line> — <description>

## Acceptance Criteria
- [ ] (copy `- [ ]` items from this matrix)

## Labels
stub-replacement, module:<name>, <priority-label>
```

**Release gate:** No `🔴 Critical` or Wave-A items may remain open when cutting a minor release.

---

*Last updated: 2026-07-27 | Generated from: `src/ROADMAP.md` + `src/*/FUTURE_ENHANCEMENTS.md` + `ai_working/gap_scan_v3_summary.json`*

---
Zuletzt geprueft (Root-Sync): 2026-07-27

---

## Post-GA Next-Phase Tracks (2026-07-30 Sync)

> **Canonical source:** `ROADMAP.md §Post-v2.4.0-rc1 Next-Phase Tracks`. This section expands each track
> with implementation-level detail, research backing, and Phase 1–6 acceptance criteria.

---

### Track 1 — AQL 2.0.0 Geospatial & Full-Text Search Completion

**Priority:** 🟡 P1 | **Target:** Q3–Q4 2026 | **Prerequisite:** AQL Mutations Phases 1–5 ✅, DDL ✅

#### Scope
- Wire existing ST_* evaluator functions (`ST_Distance`, `ST_Within`, `ST_Contains`, `ST_Intersects`,
  `ST_GeomFromGeoJSON`) into AQL FILTER / SORT / RETURN expression contexts.
- Implement phrase and proximity full-text search queries with ≤ 100 ms p95 on 100 K documents.
- Ensure strict RFC 7946 GeoJSON normalization on input and output paths.

#### Design Constraints
- ST_* functions already implemented in `src/query/let_evaluator.cpp` (≈70 % done); parser wiring only, no reimplementation.
- BM25+ must not regress exact-keyword recall vs. current BM25 baseline.
- Geospatial query results must be deterministic across CPU and GPU backends.
- Anti-meridian correctness (RFC 7946 §3.1.9): polygon winding order and longitude wrap-around handled.

#### Required Interfaces
- `src/query/aql_parser.cpp` / `include/query/aql_ast.h` — ST_* keyword nodes and proximity token stream
- `src/query/let_evaluator.cpp` — existing ST_* dispatch (extend, do not replace)
- `src/index/` — full-text index with phrase/positional posting lists
- `src/geo/spatial_backend.h` — geodesicDistance and containment contracts (already updated)

#### Implementation Notes
- **Phase 1 (Design):** Extend AQL grammar with `ST_*()` call nodes; add `PHRASE()` / `NEAR()` FTS operators; freeze AST contract.
- **Phase 2 (Core):** Wire ST_* into FILTER/SORT/RETURN evaluation pass; implement BM25+ positional scorer. BM25+ lower-bound term frequency δ = 0.5 (Robertson & Zaragoza 2009).
- **Phase 3 (Error / Edge):** Anti-meridian MBR wrap-around in geo FILTER (already fixed in `temporal_spatial_query.cpp`); FTS empty-index and zero-result edge cases; invalid geometry rejection.
- **Phase 4 (Tests):** ≥ 200 geo FILTER/SORT/RETURN integration tests; ≥ 100 FTS phrase/proximity tests; zero v1.x AQL regressions.
- **Phase 5 (Performance):** Geospatial FILTER ≥ 50× vs. O(n²) naive on 100 K geometries (Gate 4 from AQL v2.0.0 roadmap); FTS phrase ≤ 100 ms p95 on 100 K documents (Gate 5).
- **Phase 6 (Docs):** `src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md` Gates 4/5 marked `[x]`; `docs/de/aql/` updated; research traceability entry in `research/implementation_influence/by_module.md`.

#### Test Strategy
- Gate 4: `bench_geo_filter_100k` benchmark ≥ 50× vs linear baseline.
- Gate 5: `bench_fts_phrase_100k` benchmark ≤ 100 ms p95.
- Cross-feature: `tests/query/test_aql_geo_filter.cpp`, `test_aql_fts_phrase.cpp`.
- Regression: zero failures in v1.x AQL compatibility suite.

#### Performance Targets

| Operation | Condition | Target |
|---|---|---|
| `ST_Within` FILTER | 100 K polygons, R-tree prefilter | ≥ 50× vs naive scan |
| `ST_Distance` ORDER BY | 10 K results, indexed | ≤ 10 ms p99 |
| FTS phrase query | 100 K documents, positional index | ≤ 100 ms p95 |
| FTS exact-keyword recall | vs. BM25 baseline | ≥ 100 % (no regression) |

#### Security / Reliability
- Invalid GeoJSON or malformed WKT must return a parse error, not a silent empty result.
- FTS injection: query text sanitized before posting-list lookup; no regex or eval exposure.

#### Scientific References
- Open Geospatial Consortium (2011). *OGC Simple Features for SQL*, version 1.2.1.
- Butler, H. et al. (2016). *RFC 7946: The GeoJSON Format*. IETF.
- Robertson, S. & Zaragoza, H. (2009). *The Probabilistic Relevance Framework: BM25 and Beyond.* Foundations and Trends in Information Retrieval, 3(4), 333–389.

---

### Track 2 — Distributed Systems Maturity (Phase 3)

**Priority:** 🟡 P2 | **Target:** Q3–Q4 2026

#### 2.1 Replication 3.1 — Geographic Replica Placement

##### Scope
- Implement latency-aware replica placement policies with configurable geographic affinity zones.
- Async cross-region WAL shipping with configurable lag-tolerance SLOs.

##### Design Constraints
- Placement decisions must be deterministic and auditable; no implicit data sovereignty violations.
- WAL shipping lag must be bounded and observable (Prometheus gauge per replica).
- Compatible with existing `include/replication/` coordinator interfaces.

##### Required Interfaces
- `include/replication/replica_placement_policy.h` (new)
- `include/replication/wal_shipper.h` (new async WAL ship path)
- `include/observability/` — replication-lag metric exports

##### Implementation Notes
- **Phase 2 (Core):** `ReplicaPlacementPolicy` with zone-affinity scoring; `AsyncWALShipper` with bounded retry and lag-export.
- **Phase 3 (Edge):** Handle zone-down scenarios fail-closed; detect and alert on WAL queue saturation.
- **Phase 5 (Perf):** WAL shipping overhead ≤ 2 % of write throughput at 80 K ops/s baseline.

##### Performance Targets
- Geographic failover decision latency: ≤ 500 ms from zone-failure detection to new primary.
- Async WAL shipping lag: ≤ 5 s at 80 K writes/s under normal conditions.
- WAL replay throughput: ≥ 150 K entries/s (3× safety margin over write rate).

##### Scientific References
- Corbett, J. C. et al. (2012). *Spanner: Google's Globally Distributed Database.* OSDI 2012.
- Baker, J. et al. (2011). *Megastore: Providing Scalable, Highly Available Storage for Interactive Services.* CIDR 2011.

---

#### 2.2 Sharding 3.2 — Automatic Rebalancing + Global Secondary Indexes

##### Scope
- Zero-downtime automatic shard rebalancing triggered by partition skew or topology changes.
- Global secondary indexes federated across shard boundaries with consistent cross-shard reads.
- Cross-datacenter latency-aware shard routing.

##### Design Constraints
- Rebalancing must not violate ongoing 2PC transactions; quorum-safe migration window.
- Global secondary index entries must be consistent with primary key reads under snapshot isolation.
- No silent data loss if rebalance is interrupted.

##### Required Interfaces
- `include/sharding/shard_rebalancer.h` (new)
- `include/index/distributed_index_coordinator.h` (new)
- `include/sharding/latency_aware_router.h` (extend existing router)

##### Implementation Notes
- **Phase 1 (Design):** Define migration protocol: freeze shard → copy → hot-cut; WAL-consistent.
- **Phase 2 (Core):** `ShardRebalancer` with skew-detection trigger (> 20 % imbalance) and progress checkpointing.
- **Phase 3 (Edge):** Coordinator crash mid-rebalance: resume from checkpoint; quorum-safe abort.
- **Phase 5 (Perf):** Rebalance ≤ 1 % write throughput degradation on unaffected shards.

##### Performance Targets
- Rebalance throughput: ≥ 1 GB/s cross-shard copy on 10 GbE.
- Global secondary index lookup latency: ≤ 5 ms p99 for cross-shard resolution.
- Latency-aware routing: p99 query latency improvement ≥ 20 % vs. round-robin on 3-datacenter setup.

##### Scientific References
- DeWitt, D. & Gray, J. (1992). *Parallel Database Systems: The Future of High Performance Database Systems.* CACM, 35(6), 85–98.
- Ousterhout, J. et al. (2015). *The RAMCloud Storage System.* ACM TOCS. (migration protocol reference)
- Gray, J. & Reuter, A. (1992). *Transaction Processing: Concepts and Techniques.* Morgan Kaufmann. (2PC + WAL migration)

---

#### 2.3 Storage 3.4 — Tiered Hot/Warm/Cold Storage

##### Scope
- Automatic data migration between hot (NVMe/RAM), warm (HDD/object-local), and cold (S3/GCS/Azure Blob) tiers.
- Cloud-native blob backend hardening for S3, GCS, and Azure Blob Storage.
- Tiering policy: access-frequency + age-based, configurable SLAs per collection.

##### Design Constraints
- Cold-tier reads must not silently block hot-tier writes.
- Tier migration must be atomic with respect to concurrent reads (snapshot-safe).
- Community edition: local-disk tiers only; cloud tiers are enterprise/hyperscaler.

##### Required Interfaces
- `include/storage/tiering_policy.h` (new)
- `include/storage/blob_backend.h` (extend: S3/GCS/Azure)
- `plugins/blob_storage/` (private plugin; Community uses local-disk fallback)

##### Implementation Notes
- **Phase 2 (Core):** `TieringEngine` with frequency-decay score (exponential moving average, λ = 0.1 / day); async migrate on score crossing thresholds.
- **Phase 3 (Edge):** Migration abort on cloud backend error; fallback to warm tier; alert on prolonged cold-tier promotion failure.
- **Phase 5 (Perf):** Hot-tier read overhead ≤ 5 µs additional vs. no-tiering baseline.

##### Performance Targets
- Hot→warm migration: ≤ 1 ms overhead per 1 KB object (async, background thread).
- Cold-tier read rehydration: ≤ 500 ms p99 for ≤ 1 MB objects (network-bounded).
- Tiering CPU overhead: ≤ 1 % total CPU at 80 K ops/s.

##### Scientific References
- Atikoglu, B. et al. (2012). *Workload Analysis of a Large-Scale Key-Value Store.* SIGMETRICS 2012. (LRU/access-frequency tiering analysis)
- Vuppalapati, M. et al. (2020). *Building An Elastic Query Engine on Disaggregated Storage.* NSDI 2020. (decoupled storage reference)

---

#### 2.4 Network 3.5 — HTTP/3 QUIC Production Enablement

##### Scope
- Replace HTTP/1.1 + HTTP/2 wire protocol with HTTP/3 over QUIC for server-to-server and client-to-server paths.
- Zero-copy socket I/O for large response payloads (response size ≥ 64 KB).

##### Design Constraints
- HTTP/2 must remain available as a fallback under `THEMIS_ENABLE_HTTP3=OFF`.
- QUIC TLS 1.3 certificate chain must reuse existing PKI infrastructure.
- No protocol downgrade on the path that handles authentication tokens.

##### Required Interfaces
- `include/network/quic_transport.h` (new, gated on `THEMIS_ENABLE_HTTP3`)
- `include/network/zero_copy_io.h` (new; wraps `sendfile`/`io_uring` on Linux)
- `include/server/http_server.h` — extend with QUIC acceptor

##### Implementation Notes
- **Phase 1 (Design):** Choose QUIC library (`lsquic`, `mvfst`, or `msquic`); audit license compatibility.
- **Phase 2 (Core):** `QuicTransport` wrapper exposing the same `INetworkTransport` interface as TCP; zero-copy send path via `io_uring` (Linux ≥ 5.6) or `sendfile`.
- **Phase 3 (Edge):** QUIC migration (CID rotation) during long-running queries; fallback on handshake failure.
- **Phase 5 (Perf):** Tail-latency improvement ≥ 20 % at p99 vs. HTTP/2 under 1 % packet loss.

##### Performance Targets
- QUIC connection setup: ≤ 1 RTT (0-RTT reconnect for known peers).
- Throughput on 10 GbE: ≥ 8 Gbit/s sustained (vs. HTTP/2 reference).
- Tail latency under 1 % packet loss: p99 ≤ 120 % of HTTP/2 zero-loss p99 (QUIC eliminates head-of-line blocking).

##### Scientific References
- Iyengar, J. & Thomson, M. (2021). *RFC 9000: QUIC: A UDP-Based Multiplexed and Secure Transport.* IETF.
- Bishop, M. (2022). *RFC 9114: HTTP/3.* IETF.
- Langley, A. et al. (2017). *The QUIC Transport Protocol: Design and Internet-Scale Deployment.* SIGCOMM 2017.

---

### Track 3 — Observability & Operational Excellence (Phase 4)

**Priority:** 🟡 P3 | **Target:** Q4 2026

#### Scope
- End-to-end distributed trace correlation across all 58 modules via W3C Trace Context.
- ML-based anomaly-driven alerting with root-cause hint generation and continuous CPU/memory profiling via eBPF.
- Per-query retrieval guardrails: cost-aware federated pruning with SLO-validated latency budgets.
- Automated legacy-config migration with dry-run mode (Issue #1661).
- Production observability dashboards for ANN / Tensor / Graph / Final-Layer LLM handoff quality.

#### Design Constraints
- Observability must not add > 2 µs per-request overhead on the hot path.
- eBPF profiler is Linux-only (`THEMIS_ENABLE_EBPF` gate); no-op stub on other platforms.
- Anomaly models are loaded at startup; no runtime re-training in production.

#### Required Interfaces
- `include/observability/trace_context.h` — W3C Trace Context propagation (extend existing)
- `include/observability/anomaly_detector.h` (new; BOCPD or ARIMA-based)
- `include/observability/ebpf_profiler.h` (new; Linux-only; `THEMIS_ENABLE_EBPF` gate)
- `include/query/retrieval_guardrail.h` (new; per-query cost budget)

#### Implementation Notes
- **Phase 2 (Core):**
  - Propagate W3C `traceparent` / `tracestate` headers through all gRPC, HTTP, and internal dispatch paths.
  - `AnomalyDetector` using Bayesian Online Changepoint Detection (BOCPD) on per-module latency time series; emit `anomaly_score` metric and `root_cause_hint` log entry.
  - `EbpfProfiler` sampling at 99 Hz via `perf_event_open`; export flame-graph data to Prometheus histogram.
- **Phase 3 (Edge):**
  - Handle trace-context propagation failure gracefully (missing header → new root span, not error).
  - Anomaly detector graceful degradation when insufficient history (< 100 samples): suppress alerts.
- **Phase 4 (Tests):** Trace continuity tests across module boundaries; anomaly detection precision ≥ 0.8 on synthetic fault injection dataset.
- **Phase 5 (Perf):** Trace propagation ≤ 1 µs overhead per hop; BOCPD update ≤ 50 µs per metric sample.
- **Phase 6 (Docs):** Grafana dashboard definitions for all 58 module trace heatmaps; runbook for common anomaly patterns.

#### Performance Targets

| Feature | Condition | Target |
|---|---|---|
| Trace propagation overhead | Per RPC hop | ≤ 1 µs |
| BOCPD anomaly model update | Per incoming metric | ≤ 50 µs |
| eBPF profiler overhead | 99 Hz sampling, all cores | ≤ 1.5 % CPU |
| Root-cause hint precision | Synthetic fault injection suite | ≥ 0.8 |
| Retrieval cost guardrail | Per-query evaluation | ≤ 5 µs |

#### Security / Reliability
- Trace IDs must not leak PII; any user-identifying fields must be pseudonymized before export.
- Anomaly alerts must be rate-limited to avoid alert fatigue (≤ 10 per 5-minute window per module).

#### Scientific References
- Sigelman, B. et al. (2010). *Dapper, a Large-Scale Distributed Systems Tracing Infrastructure.* Google Technical Report.
- W3C Distributed Tracing WG (2021). *Trace Context Level 1.* W3C Recommendation.
- Adams, R. P. & MacKay, D. J. C. (2007). *Bayesian Online Changepoint Detection.* arXiv:0710.3742.
- Gregg, B. (2013). *Systems Performance: Enterprise and the Cloud.* Prentice Hall. (eBPF/perf methodology)
- Viilup, P. et al. (2021). *Continuous Profiling of Production Systems in Grafana Phlare / pprof.* (eBPF practical reference)

---

### Track 4 — Security Hardening & Compliance (Phase 5)

**Priority:** 🔵 P4 | **Target:** Q1 2027 | **Prerequisite:** GA tag

#### Scope
- Zero-trust continuous verification framework: every request re-verified regardless of network source.
- Fine-grained ABAC with OPA policy expressions replacing coarse RBAC where collection-level granularity is insufficient.
- Certificate-based mTLS authentication for all server-to-server paths (Issue #2370).
- SAML 2.0 SP/IdP-initiated SSO completion.
- Automated SOC 2 Type II evidence collection pipeline.
- Plugin/driver interaction hardening and shader integrity verification.

#### Design Constraints
- Zero-trust must not require breaking changes to existing JWT / OAuth2 PKCE auth flow (RFC 6749).
- ABAC policies must be hot-reloadable without server restart.
- mTLS fallback: reject requests without a valid certificate on T2/T3 trust boundaries; no degraded bypass.
- SPHINCS+ (FIPS 205) gated on `THEMIS_ENABLE_LIBOQS`; existing Ed25519 path retained as fallback.

#### Required Interfaces
- `include/security/zero_trust_verifier.h` (new; per-request continuous verification)
- `include/auth/abac_policy_engine.h` (new; OPA expression evaluation)
- `include/network/mtls_acceptor.h` (extend; certificate chain validation)
- `include/security/post_quantum_crypto.h` — `SphincsPlus` (existing stub; replace body)
- `include/auth/saml2_sp.h` (new; SP-initiated and IdP-initiated SSO)

#### Implementation Notes
- **Phase 1 (Design):** Define trust-boundary matrix T0–T5 explicitly per `SECURITY.md`; map each boundary to required controls (AuthN/AuthZ/audit).
- **Phase 2 (Core):**
  - `ZeroTrustVerifier`: per-request token re-validation + client-certificate fingerprint check; emit `auth_decision` audit event.
  - `AbacPolicyEngine`: load OPA Rego bundles; evaluate `allow/deny` on `(principal, resource, action, context)` tuples; cache policy decisions TTL = 60 s.
  - mTLS: mutual TLS 1.3 (RFC 8446) for all intra-cluster gRPC; `CertificateRevocationList` checked on every connection.
- **Phase 3 (Edge):**
  - CRL/OCSP fetch failure → fail-closed (deny); configurable retry budget.
  - ABAC policy bundle signature mismatch → bundle rejected; previous bundle retained.
  - SAML assertion replay attack prevention: `SubjectConfirmation` `NotOnOrAfter` enforced; nonce-based replay window.
- **Phase 4 (Tests):** Zero-trust negative tests (missing cert, expired token, revoked cert); ABAC boundary tests (deny/allow for each trust tier); SAML IdP mock; SPHINCS+ sign/verify KAT vectors.
- **Phase 5 (Perf):** Zero-trust check ≤ 50 µs per request; ABAC policy evaluation ≤ 200 µs; mTLS handshake ≤ 2 ms.
- **Phase 6 (Docs):** Threat model updated; SOC 2 evidence manifest; `SECURITY.md` trust-boundary matrix finalized.

#### Performance Targets

| Feature | Condition | Target |
|---|---|---|
| Zero-trust per-request check | Cached token, 1 CPU | ≤ 50 µs |
| ABAC policy evaluation | OPA Rego bundle, cached | ≤ 200 µs |
| mTLS handshake | TLS 1.3, ECDHE-P256 | ≤ 2 ms (cold) |
| SPHINCS+-SHA2-256s sign | liboqs ≥ 0.10.0 | ≤ 20 ms |
| SPHINCS+-SHA2-256s verify | liboqs ≥ 0.10.0 | ≤ 1 ms |

#### Security / Reliability
- No `CRITICAL` or `HIGH` findings in zero-trust, mTLS, or ABAC paths at Phase 5 sign-off.
- Penetration test re-run required for any trust-boundary crossing change.
- All security events must be written to immutable audit log before response is sent.

#### Scientific References
- NIST (2020). *SP 800-207: Zero Trust Architecture.* National Institute of Standards and Technology.
- NIST (2014). *SP 800-162: Guide to ABAC Definition and Considerations.* NIST.
- Rescorla, E. (2018). *RFC 8446: The Transport Layer Security (TLS) Protocol Version 1.3.* IETF.
- Cantor, S. et al. (2005). *SAML 2.0 Technical Overview.* OASIS.
- NIST (2024). *FIPS 205: Stateless Hash-Based Digital Signature Standard (SPHINCS+).* NIST.

---

### Track 5 — Gap Scanner Phase 6 + Research Traceability

**Priority:** 🟡 P5 | **Target:** Q3–Q4 2026

#### Scope
- Deploy 5 Phase 6 gap scanners: C-1 (race conditions), M-1 (use-after-free), M-2 (double-free),
  S-1 (secrets in code), S-2 (weak crypto), covering an estimated +6,000–10,000 new gap findings.
- Phase 1–4 scanner enhancements: 12 new patterns across `C-1`, `M-1`, `M-2`, `S-1`, `S-2`, `S-3` (injection) categories.
- Recurring root Soll-Ist snapshot aligned to every root-sync cycle.

#### Required Interfaces
- `ai_working/gap_scan_v3_summary.json` — scanner output (updated on each run)
- `research/implementation_influence/by_module.md` — five-column traceability matrix (all top-risk modules)

#### Implementation Notes
- Scanner integration: CI job runs on PR merge to `develop`; any new `critical` in `security`, `concurrency`, `memory` categories blocks merge.
- Traceability: every open gap item in Wave A/B/C cross-references at least one research source in `by_module.md`.
- Phase 6 scanner deployment is gated on a baseline pass of all current `release_critical` tests.

#### Performance Targets
- Scanner pipeline runtime: ≤ 10 minutes for full `src/` scan (CI gate).
- False-positive rate: ≤ 5 % per category (measured by gap-verification pass).

#### Security / Reliability
- Any `S-1` (secrets in code) finding triggers immediate triage and blocks promotion.
- Gap-scanner delta report (baseline vs. current) is mandatory for every root sync.

---

### Track 6 — Auth Mid-Term + Wave C Stub Completion

**Priority:** 🔵 P6 | **Target:** Q4 2026–Q1 2027

#### 6.1 Auth: Fail-Closed Degraded-Provider Scenarios

##### Scope
- Auth module must fail closed (deny request) when a configured optional provider (LDAP, OIDC) is
  degraded rather than silently falling back to a weaker authentication path.
- Multi-realm trust-state hardening: cross-realm principal elevation must be explicitly audited.

##### Design Constraints
- Optional-provider degradation must emit a `WARN` audit event and return `Status::Unavailable`, not a permissive fallback.
- Multi-realm trust elevation requires explicit policy entry; no implicit trust transitivity.

##### Required Interfaces
- `include/auth/provider_health_monitor.h` (new; degraded-state detection)
- `include/auth/principal_contract.h` — extend with multi-realm trust-state fields

##### Performance Targets
- Fail-closed degradation response: ≤ 5 ms (immediate reject, no retry storm).
- Multi-realm p95 latency overhead: ≤ 10 µs vs. single-realm baseline.

##### Scientific References
- RFC 6749 (2012). *The OAuth 2.0 Authorization Framework.* IETF. §4.1 error handling.
- NIST SP 800-63B (2017). *Digital Identity Guidelines: Authentication and Lifecycle Management.* NIST.

---

#### 6.2 Wave C — `security` SPHINCS+ Production Implementation

> Elevated from C-01 due to FIPS 205 publication (August 2024) and expected vcpkg liboqs ≥ 0.10.0 landing Q4 2026.

##### Implementation Notes
- Add `liboqs` to `vcpkg.json` under feature `post-quantum`.
- Replace `OQS_SIG_sphincs_sha2_256s_sign()` / `verify()` simulation with real liboqs calls.
- Retain Ed25519 simulation under `STUB/SIMULATION NOTE` for environments without `THEMIS_ENABLE_LIBOQS`.
- NIST Known-Answer Test (KAT) vectors from FIPS 205 Appendix B are the acceptance test.

##### Scientific References
- Bernstein, D. J. et al. (2019). *SPHINCS+: Stateless Hash-Based Signatures.* Submission to NIST Post-Quantum Cryptography Standardization.
- NIST (2024). *FIPS 205: Stateless Hash-Based Digital Signature Standard.* August 2024.

---

## Wave B — New Items (Q3 2026 – Q1 2027)

> Extends the existing Wave B backlog (items B-01 through B-22) with research-backed items surfaced
> during the Post-GA Phase 3 hardening pass (2026-07-30).

---

### B-23 · `temporal` — Bitemporal HLC Conflict Resolution Hardening

**Priority:** 🟠 High | **Target:** v2.5.0 | **Issue:** TBD

**Stub location:** `src/temporal/temporal_conflict_resolver.cpp` — `CRDT_MERGE` policy present in
`include/temporal/temporal_conflict_resolver.h:60-68` (`TemporalSnapshot::hlc` field) but HLC skew
window is not enforced; concurrent distributed writes under > 50 ms clock skew may silently drop
the lower-priority update.

#### Scope
- Enforce the HLC 50 ms skew window (Demirbas et al. 2014): reject writes whose HLC timestamp exceeds
  max-known HLC by more than `kMaxClockSkewMs` (configurable, default 50 ms).
- Make `CRDT_MERGE` idempotent and commutative across out-of-order delivery scenarios.
- Add `TEMPORAL_CONFLICT` structured audit event with HLC timestamps, skew delta, and winning-value provenance.

#### Design Constraints
- HLC monotonicity must be preserved under node restart (persist last-known HLC to WAL).
- `CRDT_MERGE` must not require synchronous cross-node communication; all decision state is local.
- SQL:2011 `PERIOD FOR SYSTEM_TIME` semantics must remain correct post-merge.

#### Required Interfaces
- `include/temporal/temporal_conflict_resolver.h` — `CrdtMergePolicy` (extend)
- `include/temporal/hybrid_logical_clock.h` — `HLC::advance()`, `HLC::receive()` (new)
- `include/temporal/bitemporal_table.h` — expose `system_time_hlc` column

#### Implementation Notes
- **Phase 1 (Design):** Define HLC wire format (`(walltime_ms, logical_counter)` as 96-bit value); document clock-skew rejection contract.
- **Phase 2 (Core):** `HybridLogicalClock` class with WAL-backed persistence of last-known HLC; `CrdtMergePolicy::merge()` using HLC comparison with skew guard.
- **Phase 3 (Edge):** Node restart clock recovery; partition-heal re-sync; skew > 50 ms silent-drop vs. alert decision.
- **Phase 4 (Tests):** TM-HLC-01..12: skew enforcement, idempotency, commutativity, WAL recovery, partition-heal scenarios.
- **Phase 5 (Perf):** HLC advance overhead ≤ 1 µs; CRDT_MERGE evaluation ≤ 5 µs.
- **Phase 6 (Docs):** `src/temporal/ROADMAP.md` Phase 3 item closed; `research/implementation_influence/by_module.md` temporal row updated.

#### Performance Targets
- HLC advance per write: ≤ 1 µs.
- CRDT merge decision: ≤ 5 µs.
- Bitemporal insert throughput with HLC: ≥ 100 K rows/s (regression vs. TM-1 gate: ≤ 10 % degradation).

#### Scientific References
- Demirbas, M. et al. (2014). *Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases.* OPODIS 2014 (LNCS 8878). arXiv:1406.2914.
- Kulkarni, S. S. & Michels, J.-E. (2012). *Temporal Features in SQL:2011.* ACM SIGMOD Record, 41(3), 34–43.
- Shapiro, M. et al. (2011). *A Comprehensive Study of Convergent and Commutative Replicated Data Types.* INRIA Research Report RR-7506. (CRDT formal foundation)

---

### B-24 · `cache` — Semantic Similarity Cache + GDPR-Aware Invalidation

**Priority:** 🟠 High | **Target:** v2.5.0

**Stub location:** `src/cache/semantic_cache.cpp` — vector-similarity path uses placeholder cosine-threshold
comparison without an ANN index; GDPR invalidation callback not yet wired to `PIIPseudonymizer`.

#### Scope
- Replace placeholder cosine comparison with a lightweight in-process HNSW index (≤ 50 K entries)
  for semantic cache lookup.
- Wire `PIIPseudonymizer::registerCacheInvalidator()` callback to invalidate all cache entries derived
  from a PII-bearing query when a GDPR Article 17 right-to-erasure request is processed.
- Add `cdc_redactions` RocksDB audit trail entry per invalidation event.

#### Design Constraints
- Semantic cache is a best-effort L2/L3 layer; false-positive cache hits must be guarded by a
  structural-equivalence check before result is returned.
- HNSW index for semantic cache uses `ef_construction = 128`, `M = 16` (low-overhead defaults).
- GDPR invalidation is fire-and-forget async; blocking the hot path is not permitted.

#### Required Interfaces
- `include/cache/semantic_cache.h` — extend `SemanticCache::lookup()` with HNSW backend
- `include/cache/adaptive_query_cache.h` — wire GDPR callback registration
- `include/security/pii_pseudonymizer.h` — `registerCacheInvalidator()` (existing, wire only)

#### Performance Targets
- Semantic cache lookup (HNSW, ef=64): ≤ 500 µs p99 at ≤ 50 K entries.
- GDPR invalidation pipeline: ≤ 100 ms from `erasure_request` event to all tier confirmations.
- False-positive guard: ≤ 1 % false hit rate on production query workload.

#### Scientific References
- Malkov, Y. A. & Yashunin, D. A. (2020). *Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs.* IEEE TPAMI, 42(4), 824–836.
- Cormack, G. V., Clarke, C. L. A., & Buettcher, S. (2009). *Reciprocal Rank Fusion Outperforms Condorcet and Individual Rank Learning Methods.* SIGIR 2009. (RRF for result re-ranking on cache miss)

---

### B-25 · `index` — Matryoshka Representation Learning (MRL)

**Priority:** 🟡 Medium | **Target:** v2.6.0 | **Issue:** #TBD

**Stub location:** `include/vector/vector_index.h` — HNSW index stores fixed 768-d embeddings;
no adaptive-dimensionality capability.

#### Scope
- Implement Matryoshka-aware vector index: allow multi-granularity search (64-d → 128-d → 256-d → 768-d)
  within the same HNSW index, progressively refining top-k results.
- Expose `search(ef, max_dim)` overload for latency-sensitive callers that accept lower recall in exchange
  for reduced memory bandwidth and compute.

#### Design Constraints
- Requires embedding models trained with Matryoshka Representation Learning loss; existing
  fixed-dim models are unaffected (no breaking change).
- Multi-granularity search must not break existing `search(ef)` callers.

#### Required Interfaces
- `include/vector/matryoshka_index.h` (new; wraps HNSW with adaptive dim slice)
- `include/llm/embedding_provider.h` — expose `supportsMatryoshka()` flag

#### Performance Targets
- 64-d first-pass recall@10: ≥ 0.85 (Kusupati et al. Table 1 reference point).
- 64-d search latency: ≤ 40 % of 768-d HNSW baseline latency (sub-linear with dim reduction).
- Memory footprint at 64 d / 768 d: ≤ 9 % of full-dim index for the first-pass sub-index.

#### Scientific References
- Kusupati, A. et al. (2022). *Matryoshka Representation Learning.* NeurIPS 2022. arXiv:2205.13147. (Recall@10 ≥ 0.98 at 768 d; ≥ 0.85 at 64 d for MSMARCO passage retrieval)

---

### B-26 · `index` / `acceleration` — CAGRA GPU Vector Index

**Priority:** 🟡 Medium | **Target:** v2.6.0 | **Issue:** extends A-07 (GPU vector index)

**Stub location:** `src/index/gpu_vector_index.cpp` — CUDA backend uses brute-force L2 distance;
no GPU-native graph index.

#### Scope
- Integrate CAGRA (Cuda ANNs GRAph-based, `rapidsai/cuvs` formerly NVIDIA RAFT)
  as the GPU vector index backend for HNSW-like recall at 5–10× higher QPS than CPU HNSW.
- Gate on `THEMIS_ENABLE_CUDA` and `cuvs ≥ 24.06`.

#### Design Constraints
- CPU HNSW fallback must remain available and tested independently.
- CAGRA index build time ≤ 5 × CPU HNSW build time at the same recall target.
- Result determinism: CAGRA approximate results are not required to be identical to CPU HNSW but
  must achieve Recall@10 ≥ 0.95 at the default ef parameter.

#### Required Interfaces
- `include/index/cagra_gpu_index.h` (new; wraps `cuvs::neighbors::cagra::index<>`)
- `include/acceleration/cuvs_context.h` (new; cuVS resource pool)

#### Performance Targets

| Metric | CPU HNSW baseline | CAGRA target |
|---|---|---|
| QPS (1 M vectors, d=128, SIFT-1M) | ~100–400 K QPS / 32-core CPU | ≥ 2 M QPS / A100-class GPU |
| Build time (1 M vectors) | ~45 s (CPU) | ≤ 10 s (GPU) |
| Recall@10 | 0.99 | ≥ 0.95 |
| GPU memory (1 M × 128 d float32) | — | ≤ 1 GB (float16: ≤ 500 MB) |

#### Scientific References
- Ootomo, H. et al. (2023). *CAGRA: Highly Parallel Graph Construction and Approximate Nearest Neighbor Search for GPUs.* arXiv:2308.15136.
- Johnson, J., Douze, M., & Jégou, H. (2021). *Billion-Scale Similarity Search with GPUs.* IEEE TPAMI, 43(7), 2561–2575.

---

## Phase 7 — Tensor-Native Index & Zero-Copy Inference (Q3 2026–Q4 2027)

> Extends the ROADMAP Phase 7 section with implementation-level specifications and research backing.

**Priority:** 🟡 Medium | **Target:** v3.0.0 | **Label:** `phase-7:tensor-native`

### Scope
- Native tensor storage format in RocksDB (Apache Arrow IPC columnar layout) eliminating
  serialization overhead for inference-time feature reads.
- Zero-copy inference pipeline: Arrow record batches passed directly to ONNX Runtime / llama.cpp
  without intermediate JSON or protobuf conversion.
- Rank-adaptive LoRA adapter fusing (AdaLoRA-style) for efficient multi-tenant LLM serving.
- Tensor-partitioned distributed index: shard by embedding cluster (ANN-based), not by key.

### Design Constraints
- Arrow IPC format must be backward-compatible with existing RocksDB value serialization
  (migration path: lazy conversion on first access).
- Zero-copy path is only activated when the requesting backend supports Arrow input (ONNX 1.14+, llama.cpp 2.0+).
- Rank-adaptive LoRA must not require model re-upload for adapter rank changes.
- Tensor-partitioned index must degrade gracefully to key-based sharding when cluster imbalance > 30 %.

### Required Interfaces
- `include/storage/arrow_column_store.h` (new; Arrow IPC value serializer/deserializer)
- `include/llm/zero_copy_inference_session.h` (new; wraps ONNX RT / llama.cpp with Arrow input)
- `include/llm/adalora_adapter_manager.h` (new; rank-adaptive adapter fusing)
- `include/index/tensor_partitioned_index.h` (new; cluster-based shard assignment)

### Implementation Phases
- **Phase 1 (Design / API Contract):** Define Arrow IPC column layout for ThemisDB value types; freeze `IZeroCopyInference` interface; define `AdaLoRA` rank-change protocol.
- **Phase 2 (Core):** `ArrowColumnStore` read/write with lazy-convert migration path; `ZeroCopyInferenceSession` passing `ArrowRecordBatch` to ONNX RT without copy.
- **Phase 3 (Error / Edge):** Arrow schema mismatch on read (schema evolution); ONNX RT input-shape mismatch; rank-change during live adapter serving.
- **Phase 4 (Tests):** Zero-copy throughput regression tests; serialization correctness tests with golden Arrow IPC fixtures; AdaLoRA rank-switch unit tests.
- **Phase 5 (Performance):** Benchmark zero-copy vs. JSON serialization path on 10 K batch inference requests; target ≥ 3× throughput improvement.
- **Phase 6 (Docs):** `src/llm/ROADMAP.md` Phase 7 marked; `research/implementation_influence/by_module.md` tensor row added; migration guide in `docs/migration/`.

### Performance Targets

| Feature | Condition | Target |
|---|---|---|
| Arrow IPC serialization | 1 K rows, mixed types | ≤ 50 µs |
| Zero-copy inference batch | ONNX RT, batch=32, d=768 | ≥ 3× vs. JSON path |
| AdaLoRA rank change | Hot-swap without model reload | ≤ 100 ms |
| Tensor-partition build | 1 M vectors, ANN clustering | ≤ 120 s |

### Security / Reliability
- Arrow record batches containing PII must be cleared after inference (secure-zero on deallocation).
- Adapter rank changes must be validated against the current model's weight dimensions before applying.

### Scientific References
- Apache Arrow Community (2016). *Apache Arrow: A Cross-Language Development Platform for In-Memory Data.* VLDB 2017 demo.
- Zhang, Q. et al. (2023). *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning.* ICLR 2023. arXiv:2303.10512.
- Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models.* ICLR 2022. arXiv:2106.09685.
- Dao, T. et al. (2022). *FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness.* NeurIPS 2022. (zero-copy memory access pattern)

---

## Geo Module — Phase 4–6 Enhancement Plan (2026-07-30)

> Extends `src/geo/FUTURE_ENHANCEMENTS.md` Phase-Aligned Hardening Plan with concrete acceptance criteria,
> research references, and measurable performance gates. Phases 1–3 delivered (2026-07-29).

**Priority:** 🟡 Medium | **Target:** v2.5.0–v2.6.0

### Phase 4 — Tests (Q3 2026)

#### Scope
- Close geo test coverage gaps identified in the Phase 1–3 hardening pass.
- Add anti-meridian correctness tests, RFC 7946 winding-order enforcement tests, mixed CPU/GPU precision
  parity tests, and spatial-join overflow regression tests.
- Add focused benchmark for Vincenty / Haversine fallback parity.

#### Known Gap (RFC 7946 §3.1.9 — Anti-Meridian Polygon Splitting)
RFC 7946 §3.1.9 requires that polygons crossing the anti-meridian are **split into two separate
polygons** at ±180° on ingest; the current implementation expands the MBR to `[-180, 180]` as a
conservative prefilter (no false negatives) but does not perform the normative polygon split. This
is tracked as a Phase 4 gap: winding-order (§3.1.6 right-hand rule) validation must also be added
to `geo_policy.h` to detect inverted containment on mixed-convention ingest (RFC 7946 CCW exterior
vs. OGC Simple Features CW exterior).

#### Acceptance Criteria
- `[ ]` GCH-25..GCH-32: anti-meridian MBR wrap-around tests (center near ±175°); all pass.
- `[ ]` GCH-33..GCH-40: mixed CPU/GPU Haversine parity tests (relative error ≤ 0.5 %).
- `[ ]` GCH-41: spatial-join reserve-heuristic overflow test (outer.size() = SIZE_MAX / 4).
- `[ ]` GCH-42: `CudaBuffer::alloc()` double-alloc leak test (verify free() before re-alloc).
- `[ ]` GCH-43: RFC 7946 winding-order enforcement test (CCW exterior ingested correctly; CW exterior rejected or re-wound with warning).
- `[ ]` `tests/geo/CMakeLists.txt` updated; all new tests registered with label `geo;phase4`.

#### Scientific References
- Butler, H. et al. (2016). *RFC 7946: The GeoJSON Format*, §3.1.9 (Antimeridian Cutting). IETF.
- Karney, C. F. F. (2013). *Algorithms for Geodesics.* Journal of Geodesy, 87(1), 43–55. arXiv:1109.4448. (sub-nanometer accuracy vs. Vincenty's ±0.5 mm)

---

### Phase 5 — Performance / Hardening (Q4 2026)

#### Scope
- Stabilize benchmark envelopes for CPU geo kernels (Haversine, Vincenty, spatial join, R-tree query).
- Re-baseline GPU (CUDA/HIP) kernel benchmarks after Phase 3 RAII refactor.
- Validate Vincenty accuracy: Karney (2013) reference pairs used as ground-truth for near-antipodal test vectors.

#### Performance Targets

| Operation | Backend | Target |
|---|---|---|
| Haversine batch (1 M pairs) | CPU, AVX2 | ≤ 20 ms |
| Haversine batch (1 M pairs) | CUDA kernel | ≤ 2 ms |
| Vincenty single | CPU, convergence | ≤ 5 µs |
| R-tree `intersects()` | 1 M points, MBR query | ≤ 500 µs |
| Spatial join (100 K × 100 K) | CPU, R-tree prefilter | ≤ 2 s |
| geodesicDistance (antipodal) | CPU, Haversine fallback | finite, ≥ 0 (verified) |

#### Acceptance Criteria
- `[ ]` Benchmark manifest `benchmarks/geo/bench_geo_kernels.cpp` with GK-01..GK-08.
- `[ ]` Haversine relative error vs. Karney reference: ≤ 0.5 % for all test pairs.
- `[ ]` Vincenty Karney test vectors (arXiv:1109.4448 Table 2): max absolute error ≤ 10 m.
- `[ ]` No Wave-7 regressions triggered by geo module changes.

#### Scientific References
- Guttman, A. (1984). *R-Trees: A Dynamic Index Structure for Spatial Searching.* SIGMOD 1984.
- Beckmann, N. et al. (1990). *The R*-Tree: An Efficient and Robust Access Method for Points and Rectangles.* SIGMOD 1990.
- Karney, C. F. F. (2013). *Algorithms for Geodesics.* Journal of Geodesy, 87(1), 43–55. arXiv:1109.4448.

---

### Phase 6 — Documentation & Acceptance (Q4 2026)

#### Scope
- Issue #5646 closure criteria: all Phase 1–6 acceptance items must have evidence entries.
- `src/geo/ROADMAP.md` all Phase 1–6 items marked `[x]` or escalated to a tracked issue.
- `research/implementation_influence/by_module.md` geo section updated with Karney (2013) and RFC 7946.

#### Acceptance Criteria
- `[ ]` `src/geo/ROADMAP.md` Phase 4–6 checklist ≥ 90 % `[x]`.
- `[ ]` Issue #5646 evidence summary: all three CI build attempts documented with artefact links.
- `[ ]` `research/implementation_influence/by_module.md` geo row: Karney (2013) added with status ⏳ Planned → ✅.
- `[ ]` `FUTURE_ENHANCEMENTS.md` geo section (this document) updated with current Phase status.

---

## Knowledge Graph — Wave B B2: RotatE Completion (Q1–Q2 2027)

> Extends `src/graph/FUTURE_ENHANCEMENTS.md Wave B B2` with implementation phases and benchmarks.

**Priority:** 🟡 Medium | **Target:** v3.0.0 | **Issue:** #5039 (Wave B)

### Implementation Phases
- **Phase 1 (Design):** Define `RotatEEmbedding` interface in `include/graph/kg_embeddings.h`; freeze triple-scoring and negative-sampling API.
- **Phase 2 (Core):** Implement relation-as-rotation scoring `h ∘ r = t` in complex space (L2 of `|h ∘ r − t|`); negative-sampling triple loss with self-adversarial weighting.
- **Phase 3 (Edge):** Handle degenerate triples (self-loop, unknown entity at inference time); out-of-vocabulary relation at inference: log and skip with `Status::NotFound`.
- **Phase 4 (Tests):** KGE-01..08: forward pass correctness on FB15k-237 mini fixture; KGE-09..12: negative sampling convergence over 10 epochs; KGE-13: link-prediction top-5 precision ≥ 0.3.
- **Phase 5 (Perf):** Training throughput ≥ 50 K triples/s on RTX 3090; inference top-20 predictions ≤ 50 ms.
- **Phase 6 (Docs):** `src/graph/ROADMAP.md` B2 item `[x]`; bibliography entry for RotatE in `docs/research/ml_enhancements_bibliography.md`.

### Performance Targets

| Metric | Condition | Target |
|---|---|---|
| MRR | FB15k-237 test set | ≥ 0.35 |
| Hits@10 | FB15k-237 test set | ≥ 0.55 |
| Training throughput | RTX-class GPU, batch=1024 | ≥ 50 K triples/s |
| Inference top-20 | 1 M entities | ≤ 50 ms |

### Scientific References
- Sun, Z. et al. (2019). *RotatE: Knowledge Graph Embedding by Relational Rotation in Complex Space.* ICLR 2019. arXiv:1902.10197.
- Bordes, A. et al. (2013). *Translating Embeddings for Modeling Multi-Relational Data (TransE).* NeurIPS 2013. (baseline comparison)
- Yang, B. et al. (2015). *Embedding Entities and Relations for Learning and Inference in Knowledge Bases (DistMult).* ICLR 2015. (alternative baseline)

---

## LLM — Wave B B3: Multi-Task LoRA Fine-Tuning (Q1–Q2 2027)

> Extends `src/llm/FUTURE_ENHANCEMENTS.md Wave B B3` with implementation phases and benchmarks.

**Priority:** 🟡 Medium | **Target:** v3.0.0 | **Issue:** #5039 (Wave B)

### Implementation Phases
- **Phase 1 (Design):** Define `MultiTaskLoraManager` in `include/llm/multi_task_lora.h`; task-routing policy interface; shared-base adapter format.
- **Phase 2 (Core):** Shared LoRA base with per-task projection heads; domain-gating router (`LLMRouterPlugin` extension); joint multi-task cross-entropy loss with configurable α-weighting per task.
- **Phase 3 (Edge):** Unknown task ID at inference → fallback to most-similar-task adapter (cosine similarity on task embeddings); adapter weight NaN/Inf → reject and log.
- **Phase 4 (Tests):** MTL-01..08: per-task forward-pass correctness; MTL-09..12: joint training convergence over 5 epochs; MTL-13: ablation correctness (shared vs. separate adapters: shared must be ≥ equal).
- **Phase 5 (Perf):** Average task performance ≥ +8 % vs. single-task LoRA; training-time increase ≤ 15 %; adapter switch ≤ 10 ms.
- **Phase 6 (Docs):** `src/llm/ROADMAP.md` B3 item `[x]`; bibliography in `docs/research/ml_enhancements_bibliography.md` updated.

### Performance Targets

| Metric | Condition | Target |
|---|---|---|
| Avg task quality vs. single-task | 3-task benchmark | ≥ +8 % |
| Training overhead vs. single-task | Same GPU, same data | ≤ +15 % |
| Adapter hot-switch | No reload | ≤ 10 ms |
| Task routing latency | Domain-gating | ≤ 1 ms |

### Scientific References
- Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models.* ICLR 2022. arXiv:2106.09685.
- Ruder, S. (2017). *An Overview of Multi-Task Learning in Deep Neural Networks.* arXiv:1706.05098.
- Zhang, Q. et al. (2023). *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning.* ICLR 2023. arXiv:2303.10512. (rank-adaptive variant)
- Sheng, Y. et al. (2023). *S-LoRA: Serving Thousands of Concurrent LoRA Adapters.* arXiv:2311.03285. (concurrent multi-adapter serving reference)

---

*Last updated: 2026-07-30 | Extensions: Post-GA Tracks 1–6; Wave B items B-23..B-26; Phase 7 Tensor-Native; Geo Phase 4–6; KG Wave B B2; LLM Wave B B3*

---
Zuletzt geprueft (Root-Sync): 2026-07-30
