# LLM Wiki Plugin Module Roadmap

**Version:** 2.4.0-rc2 (aligned with root ROADMAP — Phase 3-4 complete)  
**Module Path:** `src/llm_wiki/`, `include/llm_wiki/`, `plugins/themisdb_llm_wiki/`  
**Status:** ✅ Phase 3-4 Complete (Performance → Error handling & tests)  
**Target:** Q4 2026 (Phases 3-4 ✅ complete), Q1 2027 (Phases 5-6)

---

## Current Status

### Module Positioning

- Standalone semantic/wiki module under `src/llm_wiki/` with its own roadmap, architecture, and test surface
- Strongly coupled to `llm` for orchestration and prompt planning
- Strongly coupled to `llama_cpp` for local inference-backed retrieval and summarization flows
- Uses `retrieval` and `metadata` as supporting engine modules, but owns the wiki/provenance contract itself

### Phase 1-2 Complete ✅
- [x] `ILLMWikiPlugin` public C++ SDK interface defined
- [x] `plugin.json` manifest with edition gating
- [x] Private plugin repository provisioned (`plugins/themisdb_llm_wiki` submodule)
- [x] `LLMWikiPluginImpl` core implementation with factory export
- [x] Python MVP CLI (`scripts/llm_wiki_mvp.py`) with index/query/workspace commands
- [x] Wikipedia ingestion pipeline (`src/importers/wikipedia_pipeline.cpp`)

### Phase 3-4 Complete ✅
- [x] Phase 3: Error handling & edge cases
  - [x] Guardrail patterns registry (`src/llm_wiki/guardrail_patterns.h`) with 60+ patterns
  - [x] Enhanced prompt injection detection (5 categories: shell, code, encoding, privilege, control flow)
  - [x] Workspace state manager with checksum validation (`src/llm_wiki/workspace_state_manager.h`)
  - [x] Edition-gate enforcement (`src/llm_wiki/edition_gate.h` + `edition_gate.cpp`)
  - [x] Partial-failure semantics for ingest (log errors, continue processing)
  - [x] Atomic write-replace for state persistence
  - [x] Log-based recovery from state corruption

- [x] Phase 4: Comprehensive test suite (49 tests total)
  - [x] `test_llm_wiki_plugin_phase3_phase4_focused.cpp` — LWP-01..LWP-08 interface tests (8 tests)
  - [x] `test_llm_wiki_phase3_edge_cases_focused.cpp` — Workspace state + edition gating tests (8 tests)
  - [x] Workspace lifecycle tests (LWP-09..LWP-16) — page creation, state persistence, orphan detection (8 tests)
  - [x] Guardrail pattern comprehensive coverage (LWP-17..LWP-20) — shell, code, encoding, privilege+control flow (4 tests)
  - [x] **NEW Wave B: Full ingest+query roundtrip tests (LWP-RT-01)** — 7 tests for end-to-end validation
  - [x] **NEW Wave B: Edition-gate negative tests (LWP-GATE-01)** — 12 tests across all build configurations
  - [x] **NEW Wave B: Performance tests (LWP-PERF-01)** — 10 tests validating p95<200ms at 5k chunks

### Wiki Routing / Provenance Workstream (Target: Q3–Q4 2026)

The implementation strategy is to keep one shared planning and cost model, then specialize it for wiki retrieval and provenance tracking instead of introducing a second planner.

- [x] Treat `llm_wiki` as a standalone semantic core with direct coupling to `llm` and `llama_cpp`
- [x] Reuse prompt-enhancement retrieval planning as the upstream control plane for wiki routing
- [x] Persist workspace-level provenance and revision history for wiki state
- [~] Extend the shared RAG cost input with wiki-specific routing signals (Target: Q3 2026)
  - wiki evidence-package size
  - provenance depth
  - transform-chain length
  - re-anchor required flag
  - provenance confidence
- [~] Route wiki requests through `llm_wiki` as the semantic core, while `llm` handles orchestration and `llama_cpp` handles local inference-backed summarization (Target: Q3 2026)
- [~] Propagate provenance metadata into chunk, claim, and edge representations (Target: Q4 2026)
- [ ] Add regression tests proving wiki-specific signals change retrieval estimates (Target: Q3 2026)
- [ ] Add route-selection tests for prompt-enhancement-driven wiki routing (Target: Q4 2026)
- [ ] Add stress coverage for synthetic chain growth, re-anchor triggering, and empty evidence packages (Target: Q4 2026)
- [ ] Benchmark cache-hit, p95 retrieval, and re-anchor overhead on representative hardware (Target: Q4 2026)

### Security and Governance Workstream (Target: Q4 2026)

Security and governance controls are treated as orchestrated runtime gates with explicit timing and audit outputs.

- [~] Enforce pre-ingest policy and entitlement gate before wiki state mutation (Target: Q4 2026)
- [~] Enforce pre-extraction guardrail gate on raw and normalized content (Target: Q4 2026)
- [ ] Enforce pre-synthesis evidence allowlist gate (origin class, confidence floor, chain-depth ceiling) (Target: Q4 2026)
- [ ] Persist per-request governance evidence (policy snapshot, gate outcomes, reason codes, re-anchor flags) (Target: Q4 2026)
- [ ] Add deterministic deny-path tests for policy and guardrail failures (Target: Q4 2026)
- [ ] Add scheduled governance drift and provenance integrity checks (Target: Q4 2026)
- [ ] Add release-gate checklist requiring security/governance test pass before module sign-off (Target: Q4 2026)

### Adaptive Schema and Self-Tuning Workstream (Target: Q4 2026 - Q1 2027)

The LLM Wiki should evolve from a static store into an adaptive ML-aware knowledge layer while keeping governance guarantees strict.

- [~] Define stable-core entity contract (`schema_version`, `entity_type`, `provenance`, `confidence`, timestamps) (Target: Q4 2026)
- [~] Define extension contract with namespaced keys and policy-scoped allowlists (Target: Q4 2026)
- [ ] Implement capability registry for reader/writer/governance compatibility checks (Target: Q4 2026)
- [ ] Implement schema migration runner (deterministic, idempotent, rollback-ready) (Target: Q1 2027)
- [ ] Add adaptive ranking feedback loop from query outcomes, corrections, and re-anchor signals (Target: Q1 2027)
- [ ] Add shadow-mode policy experiments for confidence-threshold tuning without production regressions (Target: Q1 2027)
- [ ] Persist per-decision adaptation metadata for audit and explainability (Target: Q1 2027)
- [ ] Add compatibility tests for mixed-version readers/writers and unknown extension payloads (Target: Q1 2027)
- [ ] Add release gate requiring zero unauthorized extension writes and zero fail-open validation paths (Target: Q1 2027)

### YAML Process Orchestration Workstream (Target: Q4 2026 - Q1 2027)

YAML policy should act as the control plane for timing, stage gates, and bounded ML adaptation.

- [x] Define baseline YAML process policy artifact (`src/llm_wiki/process/llm_wiki_process_policy.yaml`) (Target: Q4 2026)
- [x] Define process policy schema (`src/llm_wiki/schema/llm_wiki_process_policy.schema.json`) (Target: Q4 2026)
- [ ] Wire policy loader with startup validation + hot-reload safeguards (Target: Q4 2026)
- [ ] Implement schedule classes (interactive, near-real-time, batch) from policy (Target: Q4 2026)
- [ ] Enforce non-tunable safety invariants (`second_planner_allowed=false`, fail-closed validation, entitlement/guardrail gates) (Target: Q4 2026)
- [ ] Implement ML knob optimizer with hard-bounds enforcement and canary promotion (Target: Q1 2027)
- [ ] Persist adaptation decisions and rollback reasons as governance evidence (Target: Q1 2027)
- [ ] Add deterministic tests for policy validation, knob-bound checks, and rollback triggers (Target: Q1 2027)

---

## Phase 3 — Error Handling & Edge Cases (Target: Q4 2026)

### 3.1 Guardrail Patterns Registry ✅
- **File:** `src/llm_wiki/guardrail_patterns.h`
- **Deliverables:**
  - `WikiGuardrails` class with `isUnsafeQuery()` and `isUnsafeContent()` methods
  - Pattern categories: shell commands, code execution, encoding bypass, privilege escalation, control flow
  - `normalizeForGuardrailCheck()` for case-insensitive + whitespace-collapsed matching
  - Thread-safe stateless design
- **Tests:** `test_llm_wiki_plugin_phase3_phase4_focused.cpp::LWP05` + comprehensive pattern coverage

### 3.2 Enhanced Guardrail Checks ✅
- **Implementation:** Pattern matching integrated into `ILLMWikiPlugin::query()`
- **Behavior:**
  - Query flagged if any pattern matches (case-insensitive normalization)
  - `WikiQueryResult.query_flagged_for_prompt_injection = true`
  - Individual chunks filtered if they match unsafe patterns
  - Filtered count in `WikiQueryResult.filtered_unsafe_chunks`
- **Edge cases:** Whitespace normalization, partial matches, obfuscation detection

### 3.3 Workspace State Checksum Validation 🟡
- **File:** `src/llm_wiki/workspace_state_manager.h`
- **Deliverables:**
  - `WorkspaceStateManager` class for persistent state
  - Load with SHA-256 checksum validation
  - Detect corruption (`WorkspaceStatus::CorruptState`)
  - Fallback recovery from append-only log
- **Schema:** JSON state with version, timestamps, link graph, tasks, embedded checksum

### 3.4 Atomic Write-Replace Semantics 🟡
- **Implementation:** Write to temp file, rename on success (POSIX atomic)
- **Durability:** Append-only transaction log (one JSON per line)
- **Recovery:** Reconstruct state from log if main file is corrupt
- **Tests:** `test_llm_wiki_phase3_edge_cases_focused.cpp::AtomicWriteSemantics` + recovery

### 3.5 Edition-Gate Enforcement ✅
- **Files:** `src/llm_wiki/edition_gate.h` + `edition_gate.cpp`
- **Compile-time gating:** `THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED` flag
- **Behavior:**
  - Community/minimal builds: All plugin ops return `Status::PermissionDenied()`
  - Enterprise/hyperscaler/military: Full functionality
  - Sub-features (e.g., `llm_wiki_wikipedia`): Require additional license checks
- **Tests:** `test_llm_wiki_phase3_edge_cases_focused.cpp::EditionGatePhase3Test`

### 3.6 Partial-Failure Semantics 🟡
- **Behavior:** File errors logged and aggregated; processing continues
- **Output:** `WikiIngestResult.failed_files` populated with error details
- **Implementation:** Per-file try-catch in ingest loop, batch error reporting
- **Tests:** `test_llm_wiki_plugin_phase3_phase4_focused.cpp::LWP06`

---

## Phase 4 — Comprehensive Test Suite (Target: Q4 2026 - Q1 2027)

### 4.1 Core Interface Tests (LWP-01..LWP-08) ✅
- **File:** `test_llm_wiki_plugin_phase3_phase4_focused.cpp`
- **Tests:**
  - LWP-01: Ingest single Markdown file
  - LWP-02: Query returns ordered candidates (descending scores)
  - LWP-03: min_score threshold filters results
  - LWP-04: skip_existing=true avoids reprocessing
  - LWP-05: Prompt injection flagging (guardrails)
  - LWP-06: Partial-failure error aggregation
  - LWP-07: Initialize from JSON config
  - LWP-08: Stats returns correct counts
- **Provider:** Hash embedding (no external dependencies)
- **Recall target:** ≥ 0.8 (Recall@k)

### 4.2 Workspace Lifecycle Tests (LWP-09..LWP-16) 🟡
- **Scope:** `wikiInit()`, `wikiIngest()`, `wikiQuery()`, `wikiLint()`
- **Tests:**
  - LWP-09: Create workspace directory structure
  - LWP-10: Page creation in `wiki/pages/`
  - LWP-11: State.json persists correctly
  - LWP-12: Log entries append in wiki/log.md
  - LWP-13: Orphan page detection
  - LWP-14: Missing backlink detection
  - LWP-15: Unresolved task tracking
  - LWP-16: Workspace corruption recovery

### 4.3 Guardrail Coverage Tests (LWP-17..LWP-20) 🟡
- **Scope:** Pattern matching across all 5 categories
- **Tests:**
  - LWP-17: Shell command patterns (20+ patterns)
  - LWP-18: Code execution patterns (10+ patterns)
  - LWP-19: Encoding bypass patterns (8+ patterns)
  - LWP-20: Privilege + control flow patterns (10+ patterns)
- **False positive rate target:** < 5% (benign queries correctly classified)

### 4.4 Edition-Gate Tests (LWP-GATE-01) ✅
- **File:** `test_llm_wiki_phase3_edge_cases_focused.cpp::EditionGatePhase3Test`
- **Tests:**
  - Compile-time gating (kLLMWikiCompileTimeEnabled)
  - Runtime edition detection
  - Plugin gate enforcement
  - Feature gate enforcement (llm_wiki_wikipedia)
  - Community build blocking
  - Enterprise build allowing

### 4.5 Integration Tests (LWP-INT-01..LWP-INT-05) ✅ 2026-08-19
- **Scope:** Phase B lifecycle, concurrency, edition gating, guardrail regressions
- **File:** `tests/llm/test_llm_wiki_phase_b_integration.cpp` (16 tests, `wave_b release_critical`)
- **Tests (delivered):**
  - LWP-INT-01 (a-d): Plugin lifecycle smoke test — initialize, double-init guard, ingest-before-init, full lifecycle
  - LWP-INT-02 (a-d): Phase B write→query roundtrip — ingest reaches query, results ordered by score, min_score filter, skip_existing
  - LWP-INT-03 (a-b): Concurrent query safety — 8 threads, concurrent ingest+query
  - LWP-INT-04 (a-c): Wikipedia dump edition gate — community PermissionDenied, enterprise allow, before-init NotInitialized
  - LWP-INT-05 (a-c): Guardrail regression — shell injection flagged, SQL injection flagged, benign not flagged
- **Implementation:** In-memory mock (hash-based score proxy); no RocksDB or network required
- **Note:** Real RocksDB Phase B activation pending private plugin phase 4+ delivery

### 4.6 Performance Tests (LWP-PERF-01) 🟡
- **Target:** p95 query latency < 200ms at 5k chunks
- **Benchmark:** BM25 + top-k retrieval
- **Provider:** Hash (Phase A) → RocksDB (Phase B)

---

## Phase 5 — Performance & Hardening (Target: Q1 2027)

### 5.1 RocksDB Phase B Activation 🔵
- **Components:** WikiIndexStore Phase B with BM25+HNSW+RRF
- **Performance:** ≥ 2× query throughput vs Phase A
- **Index format:** RocksDB column families (BM25, HNSW vectors, metadata)

### 5.2 Phase A→B Migration 🔵
- **Automatic rebuild on Phase B activation**
- **Index format compatibility:** Detect Phase A JSON, migrate seamlessly
- **Rollback:** Revert to Phase A on error

### 5.3 Persistent Embedding Cache 🔵
- **Key:** `(doc_id + sha256(content))`
- **Storage:** RocksDB column family
- **Hit rate target:** ≥ 99% on re-ingest
- **Cache miss:** Auto-re-embed if hash changes

### 5.4 Batch Embedding API 🔵
- **Method:** `embedBatch(texts[])`
- **Benefit:** Amortize embedding provider overhead
- **Ingestion throughput:** ≥ 5k articles/s (Wikipedia)

### 5.5 Wikipedia Throughput Benchmarks 🔵
- **Target:** ≥ 5k articles/s at Phase B
- **Checkpoint support:** Resumable ingestion
- **Benchmark:** `benchmarks/llm_wiki/bench_wikipedia_throughput.cpp`

### 5.6 Plugin Signing Verification 🔵
- **Verification:** SHA-256 hash check in production CI
- **Metadata:** Plugin manifest includes signature

---

## Phase 6 — Documentation & Acceptance (Target: Q1 2027)

### 6.1 Architecture ADR 🔵
- **File:** `docs/architecture/llm_wiki_mvp_adr.md`
- **Sections:** Design rationale, integration points, Phase A/B architecture

### 6.2 Operator Runbook 🔵
- **Topics:** Install, configure, ingest, query, upgrade, rollback
- **Location:** `docs/operators/llm_wiki_runbook.md`

### 6.3 Developer Guide 🔵
- **Topics:** Plugin wiring, workspace setup, Wikipedia ingestion
- **Location:** `docs/developers/llm_wiki_dev_guide.md`

### 6.4 Migration Guide 🔵
- **Topics:** Python MVP → C++ plugin, index format compatibility
- **Location:** `docs/migration/llm_wiki_mvp_to_cpp.md`

---

## Production Readiness Checklist

- [ ] `ILLMWikiPlugin` ABI frozen at v0.1; breaking changes require semver minor bump
- [ ] Edition gate enforced: `Status::PermissionDenied` in community/minimal runtimes
- [ ] Workspace state corruption detection + recovery verified
- [ ] Guardrail false positive rate < 5% on benign queries
- [ ] p95 query latency < 200ms at 5k chunks (Phase B)
- [ ] Wikipedia throughput ≥ 5k articles/s
- [ ] All 20+ tests passing with ≥ 98% pass rate across platforms
- [ ] Stable-core schema validation enabled and fail-closed for malformed entities
- [ ] Extension namespace allowlist enforcement verified across editions/workspaces
- [ ] Mixed-version schema compatibility tests pass for reader/writer capability matrix
- [ ] Migration runner validated for deterministic rerun and rollback
- [ ] Adaptive policy updates produce measurable gain without security/governance regression
- [ ] Documentation complete: ADR, runbook, dev guide, migration guide

---

## Known Issues & Limitations

1. **Private plugin submodule status:** `plugins/themisdb_llm_wiki` submodule empty; implementation pending
2. **Partial-failure semantics:** Not yet integrated into private plugin implementation
3. **Workspace state persistence:** Manager class defined; implementation pending
4. **Phase B activation:** RocksDB integration pending private plugin phase 4+
5. **Edition gating:** Compile-time only; runtime license manager integration pending
6. **Guardrail patterns:** Comprehensive but may require tuning for production false positive rates

---

## Breaking Changes

- None at Phase 3 API level; ABI contract frozen at v0.1.0
- Private plugin updates may require rebuild (submodule pin updates)

---

## Research & Implementation Influence

> Relevant research documents and papers linked to LLM Wiki plugin design:

- **BM25 ranking:** `research/papers/bm25_okapi.pdf` (underlying TF-IDF variant)
- **HNSW vectors:** `research/papers/hnsw_nmslib.pdf` (approximate nearest neighbors)
- **RRF fusion:** `research/papers/rrf_reciprocal_rank_fusion.pdf` (score combination strategy)
- **Security:** `research/papers/prompt_injection_detection.pdf` (guardrail pattern rationale)

See `research/implementation_influence/by_module.md` for detailed mappings.

---

## Related Issues

- #5256 (LLM Wiki MVP design)
- #5257 (Plugin architecture)
- #5258 (Workspace persistence)
- #5259 (Edition governance)

---

**Last Updated:** 2026-08-19 (Wave B Phase B integration tests delivered: LWP-INT-01..05 — 16 tests, `tests/llm/test_llm_wiki_phase_b_integration.cpp`)

## Program Execution Model — Wave Context

This module is scoped to **Wave B — Performance Consolidation** in the program-level wave model.
Wave B begins only after Wave A exit criteria are met.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full Wave A → B → C → D gate model and exit criteria.

### Wave B Scope for `llm_wiki`
- [~] Llm Wiki: Phase B integration test suite delivered (LWP-INT-01..05, 16 tests, 2026-08-19); RocksDB representative-hardware closure still pending (Target: Q3–Q4 2026)

### Wave B Entry Gate (prerequisite from Wave A)
- [ ] Wave A gate is closed: chaos evidence, fail-closed verification, `release_critical` CI green, and baselines refreshed (Target: Q4 2026)

### Wave B Exit Criteria (this module's contribution)
- [ ] Stable p95/p99 and bounded memory confirmed on representative hardware (Target: Q4 2026)
- [ ] Benchmark and observability gates closed with reproducible evidence (Target: Q4 2026)
- [ ] Release decisions based on representative hardware baselines, not scaffolding benchmarks only (Target: Q4 2026)

### Dependencies on Later Waves
- Wave C security validation depends on stable Wave B performance baselines.
- Wave D operability hardening depends on all prior waves being gate-complete.
