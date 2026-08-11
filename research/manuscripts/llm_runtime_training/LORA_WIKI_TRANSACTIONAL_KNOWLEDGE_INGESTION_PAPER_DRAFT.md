# Transactional Knowledge-Graph Ingestion for LLM Adapter Serving in ThemisDB

**Status**: ACTIVE_DRAFT  
**Version**: 0.1  
**Last Updated**: 2026-08-10  
**Target Venue**: VLDB 2027 / arXiv (cs.DB / cs.AI) — Q4 2026  
**Portfolio Cluster**: `research/manuscripts/llm_runtime_training/`

---

## Metadata

- **Scientific Delta**: Treat wiki-scale knowledge-graph ingestion as a first-class transactional database operation coupled to LLM adapter lifecycle management, rather than a batch-import preprocessing step.
- **Canonical Evidence Sources**: `include/llm_wiki/llm_wiki_plugin_interface.h`, `plugins/private/themisdb_llm_wiki/ROADMAP.md`, `tests/test_lwp_plugin_focused.cpp`.
- **Required Experiments**: ingestion throughput with ACID guarantees vs. batch import; adapter hot-swap latency with knowledge-graph update applied transactionally; freshness lag between wiki index and serving adapter.
- **Open Risks / Claim Boundaries**: Phase B (THEMISDB_WIKI_PHASE_B flag) is not yet enabled; only Phase A (JsonWikiIndexReader + FNV hash) is production-verified. Quantitative throughput and latency claims require Phase B completion.
- **Overlap / Successor / Predecessor**: complements the LLM serving optimization manuscript and the AdaLoRA-TT bridge paper; focuses on the ingestion-path novelty rather than adapter representation or serving mechanics.

---

## Abstract

Knowledge-augmented LLM adapters depend on two coupled operations: ingesting fresh knowledge and serving inference with up-to-date context. Systems that separate these operations into offline batch pipelines and online serving paths introduce a staleness window that degrades retrieval quality without observable latency cost. ThemisDB's LLM Wiki plugin provides the first database-native path that ties wiki-scale knowledge-graph indexing to the transactional lifecycle of LLM adapters. This paper formalizes the design: an enterprise plugin that exposes `themisdb_llm_wiki_create`, manages a `WikiStatus` surface for freshness tracking, provides a `JsonWikiIndexReader` for incremental indexed ingestion in Phase A, and defers full graph-level consistency semantics to Phase B. We describe the plugin architecture, ACID integration points, and the evaluation plan for a production-ready transactional ingestion path.

---

## I. Introduction

Production RAG systems routinely face a freshness problem: the knowledge corpus backing retrieval is updated on a different cadence than the LLM adapter it feeds. This creates an observable window where the adapter serves inference with stale context. The usual mitigation — reducing batch-import frequency — is a scheduling heuristic, not a correctness guarantee.

ThemisDB's LLM Wiki plugin offers a different framing: knowledge-graph updates and adapter lifecycle events are both first-class database operations. Their relative ordering, isolation level, and commit boundary can be reasoned about in exactly the same way as any other multi-table transaction. The paper targets this design question: what does transactional knowledge ingestion mean for an enterprise LLM adapter platform?

### Contributions

1. A formalization of knowledge-graph ingestion as a transactional adapter lifecycle event.
2. A plugin architecture (`ILLMWikiPlugin` + `WikiStatus`) that surfaces freshness as an observable, queryable database attribute.
3. An evaluation plan for ingestion throughput, freshness lag, and hot-swap latency under concurrent adapter serving.

---

## II. Related Work

- RAG knowledge-source management and corpus refresh strategies
- PEFT adapter lifecycle and storage: LoRA, AdaLoRA, S-LoRA
- Knowledge graph construction and update in enterprise settings
- novelty delta: couple knowledge ingestion directly to adapter transactional lifecycle rather than treating them as independent ETL and serving concerns

---

## III. System Model / Repository Scope

- Public SDK: `include/llm_wiki/llm_wiki_plugin_interface.h`
  - `ILLMWikiPlugin` — plugin lifecycle interface
  - `WikiStatus` — freshness, index size, phase flag, error state
  - Factory symbol: `themisdb_llm_wiki_create`
  - Allowed editions: enterprise / hyperscaler / military
- Phase A implementation: `plugins/private/themisdb_llm_wiki/src/wikipedia/`
  - `JsonWikiIndexReader` — incremental JSON wiki index ingestion
  - FNV-based hash indexing for deduplication
- Phase B: guarded by `THEMISDB_WIKI_PHASE_B` compile flag; full graph-level consistency semantics deferred
- Plugin manifest: `plugins/private/themisdb_llm_wiki/plugin.json`
- Tests: `tests/test_lwp_plugin_focused.cpp` — LWP-01..LWP-08

---

## IV. Method / Design

### A. Transactional Ingestion Model

Knowledge-graph updates enter ThemisDB via the wiki plugin's ingestion interface. Each ingestion batch is assigned a transaction context that determines:

1. **Isolation level**: ingested nodes and edges are not visible to serving paths until the ingestion transaction commits
2. **Ordering**: adapter hot-swaps that depend on new knowledge are serialized after the ingestion commit
3. **Rollback**: partial ingestion failures roll back to the last committed index state without corrupting the serving path

### B. `WikiStatus` Freshness Surface

The `WikiStatus` struct exposes five observable attributes:

| Field | Type | Purpose |
|---|---|---|
| `is_ready` | bool | plugin initialized and index ready for serving |
| `index_size` | uint64_t | number of indexed entries |
| `last_update_epoch` | int64_t | Unix epoch of last successful ingestion commit |
| `phase_b_enabled` | bool | full graph-consistency mode active |
| `error_message` | std::string | last ingestion error, empty on success |

These fields allow the serving layer to make explicit freshness decisions rather than assuming the index is always current.

### C. Phase A — JsonWikiIndexReader

Phase A implements incremental ingestion from JSON wiki index dumps using:
- streaming JSON parsing (no full-document load into memory)
- FNV-1a hash for entry deduplication across incremental updates
- append-only index updates committed atomically per ingestion batch

### D. Phase B — Full Graph Consistency (Planned)

Phase B, gated by `THEMISDB_WIKI_PHASE_B`, extends ingestion to full graph-level consistency:
- entity and relation disambiguation across ingestion batches
- cross-entity ACID constraints during multi-batch ingestion windows
- adapter serving isolation during active graph restructuring operations

---

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `include/llm_wiki/llm_wiki_plugin_interface.h` | public SDK | `ILLMWikiPlugin`, `WikiStatus`, factory symbol | ready |
| E2 | `plugins/private/themisdb_llm_wiki/plugin.json` | manifest | allowed editions, plugin ID, factory symbol registration | ready |
| E3 | `plugins/private/themisdb_llm_wiki/ROADMAP.md` | phases | Phase A complete, Phase B gated | ready |
| E4 | `tests/test_lwp_plugin_focused.cpp` | LWP-01..LWP-08 | plugin lifecycle, index reader, freshness surface, injection hooks | ready |
| E5 | `plugins/private/themisdb_llm_wiki/src/wikipedia/` | implementation | `JsonWikiIndexReader`, FNV hash, Phase A ingestion path | ready (private submodule) |

---

## VI. Experimental Methodology

### A. Setup
- Phase A only: JSON index ingestion with varying batch sizes (1K, 10K, 100K entries)
- Adapter serving running concurrently at fixed request rate
- ACID isolation: committed ingestion batches only visible to serving after commit
- Freshness measurement: `WikiStatus.last_update_epoch` delta vs. latest ingestion commit

### B. Workloads
- W1: baseline ingestion throughput (no concurrent serving)
- W2: ingestion under concurrent serving load (variable serving rate)
- W3: rollback under partial ingestion failure; verify serving path sees last committed state
- W4 (Phase B, deferred): full graph-consistency ingestion with concurrent structural updates

### C. Metrics
- ingestion throughput: entries/sec per batch size
- freshness lag: time from ingestion start to `WikiStatus.is_ready` + `last_update_epoch` update
- adapter hot-swap latency: p95/p99 delay from ingestion commit to serving path switch
- isolation correctness: zero serving-path reads from uncommitted ingestion transactions

---

## VII. Results

### A. Primary Results
- Phase A ingestion path and `WikiStatus` freshness surface are implemented and test-covered (LWP-01..LWP-08)
- Plugin lifecycle, factory registration, and edition gating are verified
- Quantitative throughput and freshness-lag measurements require Phase B completion or dedicated Phase A evaluation run

### B. Ablations / Sensitivity
- batch-size sensitivity: ingestion throughput and freshness lag vs. batch granularity
- concurrency sensitivity: serving rate impact on ingestion commit latency

### C. Negative Results
- Phase B (full graph consistency) is not yet enabled; cross-entity ACID constraints and graph restructuring isolation are deferred claims
- end-to-end freshness-lag benchmarks are not yet captured in `benchmarks/`

---

## VIII. Discussion

### Supported claims
- knowledge-graph ingestion as a transactional database operation is architecturally realized in ThemisDB's LLM Wiki plugin (`E1`, `E2`, `E3`)
- `WikiStatus` provides a queryable freshness surface usable by serving-layer freshness policies (`E1`, `E4`)

### Deferred claims
- quantitative throughput advantages over batch-import pipelines (requires Phase A benchmark run)
- Phase B graph-consistency semantics and their impact on serving-path isolation (gated on `THEMISDB_WIKI_PHASE_B`)

---

## IX. Reproducibility & Artifact

- public SDK: `include/llm_wiki/llm_wiki_plugin_interface.h`
- focused tests: `tests/test_lwp_plugin_focused.cpp` (LWP-01..LWP-08)
- experiment protocol: `research/experiments/llm_runtime_training/wiki_ingestion_protocol.md` (to be created)
- Phase A artifact: available in enterprise plugin submodule

---

## X. Limitations, Risk, Ethics

- private submodule for Phase A/B implementation limits full open-source reproducibility; public SDK and test files provide interface-level traceability
- FNV hash deduplication is fast but not collision-resistant at very large index sizes; a stronger hash or content-addressed ID scheme may be needed for Phase B
- knowledge-graph ingestion of public wiki content must respect upstream license and attribution requirements

---

## XI. Conclusion

ThemisDB's LLM Wiki plugin provides a concrete, repository-grounded instantiation of transactional knowledge-graph ingestion coupled to LLM adapter lifecycle management. The Phase A path is implemented and test-covered; the Phase B path is architecturally specified and gated. The next step is a reproducible Phase A throughput and freshness-lag evaluation to turn architecture evidence into quantitative paper claims.
