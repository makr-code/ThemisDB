> **Roadmap-Hinweis:** Format per Governance: `- [ ] <Task> (Target: <Q/Jahr>)`.
> Checkbox-Status: `[ ]` offen · `[~]` in Bearbeitung · `[x]` erledigt · `[?]` blockiert · `[!]` zu prüfen

# ROADMAP — themisdb_llm_wiki

## Module Evidence Summary (Public API / C++ core — maintained in ThemisDB monorepo)

| Component | Status | Location |
|---|---|---|
| `wiki_index_store.h/cpp` | 🟢 PRODUCTION-READY | `include/llm/` + `src/llm/` |
| `wiki_chunk_splitter.h/cpp` | 🟢 PRODUCTION-READY | `include/llm/` + `src/llm/` |
| `wiki_rag_source.h/cpp` | 🟢 PRODUCTION-READY | `include/llm/` + `src/llm/` |
| `wikipedia_*.cpp` (ingestion pipeline) | 🟢 PRODUCTION-READY | `src/importers/` |
| `llm_wiki_plugin_interface.h` | 🟡 BETA (Phase 1) | `include/llm_wiki/` |
| Python MVP CLI (`llm_wiki_mvp.py`) | 🟢 MVP-COMPLETE | `scripts/llm_wiki_mvp.py` |
| Plugin implementation (`themisdb_llm_wiki_cpp`) | 🟡 BETA (Phase 2) | `src/wikipedia/` in this repo |
| Plugin manifest (`plugin.json`) | 🟡 BETA | `plugin.json` |

---

## Current Status

- [x] Core C++ building blocks (`WikiIndexStore`, `WikiChunkSplitter`, `WikiRagSource`) are
  production-ready in the ThemisDB LLM module.
- [x] Python MVP CLI (`llm_wiki_mvp.py`) is MVP-complete: `index`, `query`, `wiki-init`,
  `wiki-ingest`, `wiki-query`, `wiki-lint` subcommands; guardrails; persistent workspace mode.
- [x] Wikipedia XML dump ingestion C++ pipeline exists in `src/importers/`.
- [x] Public C++ SDK interface (`llm_wiki_plugin_interface.h`) is defined (Phase 1 delivery).
  - `Status` struct added to interface (required for `initialize()` / `wikiInit()` return types).
- [~] Private plugin shared library (`themisdb_llm_wiki_cpp`) Phase 2 implementation delivered:
  - `LLMWikiPluginImpl` implements all `IThemisPlugin` + `ILLMWikiPlugin` pure virtuals.
  - `WikiWorkspaceOrchestrator` implements full workspace lifecycle (init/ingest/query/lint/stats).
  - Hash embedding (FNV-1a), in-memory BM25 fallback, prompt-injection + content guardrails.
  - Factory entry point (`themisdb_llm_wiki_create`) exported with C linkage.
  - Phase B (RocksDB) path guarded by `THEMISDB_WIKI_PHASE_B` compile flag; falls back to Phase A.
  - LWP-01..LWP-08 focused unit tests delivered in `tests/test_lwp_plugin_focused.cpp`.

## In Progress

- [x] Phase 1 — Design / API Contract: `llm_wiki_plugin_interface.h` delivered (Target: Q3 2026)
  - `Status` struct added; all lifecycle method signatures finalised.
- [~] Phase 2 — Core Implementation: private plugin `.so` wrapping existing C++ building
  blocks behind `ILLMWikiPlugin` (Target: Q3–Q4 2026)
  - [x] `LLMWikiPluginImpl` skeleton and all pure-virtual overrides
  - [x] `WikiWorkspaceOrchestrator` (init/ingest/query/lint/stats)
  - [x] Hash embedding (FNV-1a, deterministic, no external deps)
  - [x] In-memory BM25 Phase A fallback
  - [x] Prompt-injection + content guardrails (`UNSAFE_PATTERNS`)
  - [x] `themisdb_llm_wiki_create()` factory with C linkage
  - [x] CMakeLists.txt: monorepo detection, STATIC + SHARED targets
  - [x] LWP-01..LWP-08 focused unit tests
  - [ ] Phase B (RocksDB + `WikiIndexStore`) activation and integration tests

## Planned Features

- [ ] RocksDB-native persistence for wiki chunks (WikiIndexStore Phase B activation) (Target: Q4 2026)
- [ ] Persistent embedding cache backed by RocksDB (Target: Q4 2026)
- [ ] RBAC-aware multi-tenant wiki namespaces (Target: Q1 2027)
- [ ] Wikipedia dump ingestion wired through plugin interface (Target: Q4 2026)
- [ ] AQL query integration for graph-native traversal and contradiction resolution (Target: Q1 2027)
- [ ] Quality evaluation: Recall@k, MRR, p95 latency reporting (Target: Q1 2027)

---

## Implementation Phases

### Phase 1 — Design / API Contract
- [x] Define `ILLMWikiPlugin` public C++ SDK interface in `include/llm_wiki/llm_wiki_plugin_interface.h`
  - Inputs/outputs: `WikiIngestOptions`, `WikiQueryOptions`, `WikiIngestResult`, `WikiQueryResult`,
    `WikiWorkspaceStats`, `WikiLintResult`, `WikiDumpIngestOptions`
  - Edition gating: `enterprise` / `hyperscaler` / `military`; license feature `"llm_wiki_enterprise"`
  - Lifecycle: `initialize(config_json)` → use → `shutdown()`
  - Factory symbol: `themisdb_llm_wiki_create` (C linkage export)
- [x] Create `plugin.json` manifest with capability, edition, sub-feature, and dependency metadata
- [ ] Freeze plugin ABI version tag and document backward-compatibility policy (Target: Q3 2026)
  - Inputs: `kLLMWikiPluginId`, `kLLMWikiPluginFactorySymbol` constants
  - Constraint: ABI break → version bump + migration note in `BREAKING_CHANGES.md`
- [ ] Document `initialize()` JSON config schema with all required/optional fields (Target: Q3 2026)

### Phase 2 — Core Implementation
- [ ] Implement `LLMWikiPluginImpl : ILLMWikiPlugin` in private plugin repo (Target: Q3–Q4 2026)
  - [x] `ingest()`: `WikiChunkSplitter::split()` per file → hash embeddings → in-memory chunks_
  - [x] `query()`: guardrail filter → in-memory BM25 / `JsonWikiIndexReader` fallback
  - [x] `wikiInit()` / `wikiIngest()` / `wikiQuery()` / `wikiLint()`: delegate to `WikiWorkspaceOrchestrator`
  - [x] `ingestWikipediaDump()`: delegated to `WikipediaIngestionPipeline` (requires `llm_wiki_wikipedia` license)
  - [x] Error cases: invalid config JSON, missing source path, >50 MB files, uninitialised
  - [x] Fallback: Phase A JSON reader / in-memory BM25 when `rocksdb_dir` is empty
- [ ] Implement `WikiWorkspaceOrchestrator` for persistent workspace mode (Target: Q4 2026)
  - [x] Directory structure: `raw_sources/`, `wiki/pages/`, `wiki/index.md`, `wiki/log.md`,
    `wiki/schema.md`, `wiki/state.json`
  - [x] Append-only log entries per ingest/query/lint operation
  - [x] Concept link extraction (heading adjacency + doc→section links)
  - [x] Contradiction detection: CONTRADICTION_CUES → open review task in `state.json`
- [ ] Wire `WikiRagSource` into `ModularRAGPipeline` via plugin initialization hook (Target: Q4 2026)
- [ ] Export `themisdb_llm_wiki_create()` factory with C linkage (Target: Q3 2026)
  - [x] Implemented in `src/wikipedia/llm_wiki_plugin_factory.cpp`
  - Safety: verify `host_api_version` ≥ 2 before registering; return non-zero on ABI mismatch

### Phase 3 — Error Handling & Edge Cases
- [ ] Input validation: reject empty `query_text`, oversized source files (> 50 MB by default),
  malformed JSON config (Target: Q4 2026)
- [ ] Partial-failure semantics for `ingest()`: log per-file errors, continue, populate
  `WikiIngestResult::failed_files`; never abort the entire batch on a single file error (Target: Q4 2026)
- [ ] Guardrail hardening: extend `UNSAFE_PATTERNS` list; apply to both query and chunk content;
  ensure `query_flagged_for_prompt_injection` and `filtered_unsafe_chunks` are accurate (Target: Q4 2026)
- [ ] Workspace corruption detection: validate `state.json` schema on every `wikiInit()` /
  `wikiIngest()` open; recover from truncated log with partial state (Target: Q4 2026)
- [ ] Wikipedia dump errors: checkpoint on parse error; resume from last committed article batch;
  report `WikiIngestResult::errors` accurately (Target: Q4 2026)
- [ ] Edition-gate enforcement: plugin `initialize()` must return an error status when loaded
  in a community or minimal runtime; fail closed, never degrade silently (Target: Q4 2026)

### Phase 4 — Tests
- [ ] Unit tests: `LWP-01..LWP-08` — `ILLMWikiPlugin::ingest()` + `query()` with hash provider
  and fixture documents; verify chunk counts and Recall@k ≥ 0.8 (Target: Q4 2026)
  - Inputs: fixture Markdown docs in `tests/fixtures/`; deterministic hash embeddings
  - Expected output: correct chunk counts; top-k results include expected sources
- [ ] Unit tests: `LWP-09..LWP-16` — workspace init/ingest/query/lint lifecycle;
  validate log entries, page creation, orphan detection (Target: Q4 2026)
- [ ] Unit tests: `LWP-17..LWP-20` — guardrail tests; prompt-injection query flagged;
  unsafe chunk excluded from result set (Target: Q4 2026)
- [ ] Integration tests: `LWP-INT-01..LWP-INT-04` — live RocksDB fixture;
  Phase B store write → query roundtrip; verify RRF fusion score ordering (Target: Q1 2027)
- [ ] Wikipedia dump smoke test: `LWP-WIKI-01` — ingest 1k-article subset; verify
  article count, checkpoint resume after simulated interruption (Target: Q1 2027)
- [ ] Edition-gate negative test: `LWP-GATE-01` — plugin returns error status when
  loaded in community-edition runtime (Target: Q4 2026)
- [ ] Performance gate: `LWP-PERF-01` — p95 query latency < 200 ms for ≤ 5k chunks
  on reference hardware (Target: Q1 2027)

### Phase 5 — Performance / Hardening
- [ ] Activate WikiIndexStore Phase B (RocksDB-native BM25 + HNSW) when `rocksdb_dir`
  is configured; document Phase A → Phase B migration path and index rebuild procedure (Target: Q1 2027)
  - Perf target: ≥ 2× query throughput vs. Phase A JSON reader at 50k chunks
- [ ] Persistent embedding cache: store embedding vectors in RocksDB to avoid
  re-embedding unchanged documents on re-ingest (Target: Q1 2027)
  - Behaviour: cache keyed on `(file_path + sha256(content))`; evict stale entries on re-ingest
- [ ] Batch embedding API: replace per-chunk `embed()` calls with `embedBatch()` during
  ingestion to reduce model round-trips (Target: Q1 2027)
- [ ] Wikipedia ingestion throughput: benchmark `WikipediaPipeline` end-to-end at
  ≥ 5k articles/s on reference hardware; identify and fix top bottleneck (Target: Q2 2027)
- [ ] RBAC-aware namespacing: `WikiIndexStore` table scoped per tenant; queries restricted
  to caller's accessible namespaces (Target: Q2 2027)
- [ ] Signed plugin verification: enforce SHA-256 digest check via `AdapterSignature`
  before `themisdb_llm_wiki_create()` is invoked in production builds (Target: Q2 2027)

### Phase 6 — Documentation & Acceptance
- [ ] Update `docs/architecture/llm_wiki_mvp_adr.md` to reflect enterprise plugin
  architecture: Phase B backend, RBAC, Wikipedia sub-feature, edition gating (Target: Q4 2026)
- [ ] Write operator runbook: install, configure, ingest, query, upgrade, rollback (Target: Q1 2027)
- [ ] Write developer guide: how to wire `ILLMWikiPlugin` into an application,
  workspace setup, embedding provider selection, Wikipedia dump ingestion (Target: Q1 2027)
- [ ] Update `docs/use-cases/LLM_WIKI_MVP.md`: add enterprise mode section covering
  the plugin interface and differences from the Python MVP (Target: Q4 2026)
- [ ] Record migration path from Python MVP (`scripts/llm_wiki_mvp.py`) to the
  C++ plugin, including index format compatibility and workspace import (Target: Q1 2027)
- [ ] Module acceptance sign-off against Production Readiness Checklist below (Target: Q2 2027)

---

## Production Readiness Checklist

- [ ] `ILLMWikiPlugin` ABI frozen at v0.1; breaking changes require semver minor bump
- [ ] `initialize()` JSON config schema documented and schema-validated at plugin load time
- [ ] `ingest()` / `query()` meet p95 latency targets (< 200 ms @ 5k chunks)
- [ ] Edition-gate enforced: plugin returns `Status::Error` when loaded outside enterprise+
- [ ] All `LWP-01..LWP-20` focused tests pass with `TIMEOUT 120`
- [ ] Integration tests `LWP-INT-01..LWP-INT-04` pass with live RocksDB fixture
- [ ] Guardrail tests cover prompt-injection and unsafe-chunk filtering
- [ ] Wikipedia dump ingestion smoke test `LWP-WIKI-01` passes
- [ ] Signed plugin SHA-256 verification active in production CI
- [ ] Operator runbook complete: install, upgrade, rollback, disaster recovery
- [ ] Persistent embedding cache active to prevent redundant model calls on re-ingest
- [ ] RBAC-aware multi-tenant namespacing implemented and tested

---

## Known Issues & Limitations

- The private plugin shared library (`themisdb_llm_wiki_cpp`) does not yet exist.
  Phase 2 implementation work is the blocking item.
- WikiIndexStore Phase B (RocksDB-native) requires `rocksdb_dir` configuration and
  an initial index rebuild; Phase A JSON reader is the current production fallback.
- Persistent workspace mode is a Python MVP orchestration layer; not yet ported to C++.
- `ingestWikipediaDump()` delegates to the existing C++ pipeline but is not yet wired
  through the `ILLMWikiPlugin` interface; the pipeline exists but lacks the plugin ABI wrapper.
- Recall@k quality is bounded by the hash embedding provider; sentence-transformers or
  OpenAI embeddings yield significantly better retrieval quality but add external dependencies.
- Multi-tenant RBAC isolation is planned for Phase 5; current implementation is single-tenant.

## Breaking Changes

- `llm_wiki_plugin_interface.h` is at v0.1.0; interface is in BETA and may change until v1.0.0 is declared.
- `plugin.json` sub-feature key `"llm_wiki_wikipedia"` is reserved; loaders must treat
  missing sub-feature keys as disabled (backward-compatible default).
