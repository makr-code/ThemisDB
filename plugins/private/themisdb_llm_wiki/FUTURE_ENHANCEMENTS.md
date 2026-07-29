> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie
> mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# FUTURE_ENHANCEMENTS — themisdb_llm_wiki

## themisdb_llm_wiki (Enterprise Plugin)

### Scope

- Complete private plugin shared library implementing `ILLMWikiPlugin` behind the public SDK.
- Activate WikiIndexStore Phase B (RocksDB-native hybrid retrieval).
- Port the Python MVP workspace orchestrator to C++.
- Wire Wikipedia XML dump ingestion through the plugin ABI.
- Introduce RBAC-aware multi-tenant wiki namespaces.
- Structured quality evaluation pipeline (Recall@k, MRR, p95 latency).

### Design Constraints

- `ILLMWikiPlugin` ABI is the only public surface; all implementation details stay private.
- Phase A JSON reader must remain a fallback when `rocksdb_dir` is not set; no hard RocksDB
  dependency at plugin load time.
- Community and Minimal builds must compile and run cleanly without this plugin present; the
  plugin loader degrades gracefully when the `.so` is absent.
- Embedding providers remain swappable at config time; no provider-specific code in the core path.
- Wikipedia ingestion requires the `"llm_wiki_wikipedia"` sub-feature license; callers without it
  receive an explicit `Status::PermissionDenied` from `ingestWikipediaDump()`.
- RBAC integration must not introduce performance regression > 5% on single-tenant queries.

### Required Interfaces

- **`ILLMWikiPlugin`** (`include/llm_wiki/llm_wiki_plugin_interface.h`):
  - `initialize(config_json)` → `Status`
  - `ingest(source_path, WikiIngestOptions)` → `WikiIngestResult`
  - `query(query_text, WikiQueryOptions)` → `WikiQueryResult`
  - `wikiInit(workspace_root)` → `Status`
  - `wikiIngest(source_path, WikiIngestOptions)` → `WikiIngestResult`
  - `wikiQuery(query_text, WikiQueryOptions)` → `WikiQueryResult`
  - `wikiLint(workspace_root, max_staleness_days)` → `WikiLintResult`
  - `ingestWikipediaDump(dump_path, WikiDumpIngestOptions)` → `WikiIngestResult`
  - `stats(workspace_root)` → `WikiWorkspaceStats`
  - `shutdown()`
- **`WikiChunkSplitter`** (`include/llm/wiki_chunk_splitter.h`): consumed internally.
- **`WikiIndexStore`** (`include/llm/wiki_index_store.h`): Phase A (JSON) and Phase B (RocksDB)
  backends consumed internally.
- **`WikiRagSource`** (`include/llm/wiki_rag_source.h`): wired into `ModularRAGPipeline` on request.
- **`WikipediaPipeline`** (`include/importers/wikipedia_pipeline.hpp`): consumed by
  `ingestWikipediaDump()`.

### Implementation Notes

#### Phase A → Phase B migration

Phase A uses `JsonWikiIndexReader` (in-memory BM25 TF overlap + cosine KNN).
Phase B activates `WikiIndexStore` (BM25 fulltext secondary index + HNSW cosine vector index + RRF
fusion) backed by RocksDB when `rocksdb_dir` is set in `initialize()`.

Migration path:
1. Run `ingest()` against all sources to build the JSON index.
2. Set `rocksdb_dir` in plugin config and call `initialize()` again.
3. Plugin detects an empty RocksDB table and runs an automatic index rebuild from the JSON fallback.
4. Subsequent queries use the Phase B backend; Phase A JSON file is retained as a rollback snapshot.

#### Workspace orchestrator (C++ port)

The Python MVP workspace orchestrator must be re-implemented in C++ as `WikiWorkspaceOrchestrator`:
- Manages `raw_sources/`, `wiki/pages/`, `wiki/index.md`, `wiki/log.md`, `wiki/schema.md`,
  `wiki/state.json` (same layout as Python MVP for workspace compatibility).
- Log entries use ISO 8601 timestamps and append-only semantics (no truncation).
- `state.json` schema:
  ```json
  {
    "pages": { "<slug>": { "title": "...", "source": "...", "created": "...", "updated": "..." } },
    "links": [ { "from": "...", "to": "...", "type": "concept" } ],
    "assertions": [ { "id": "...", "text": "...", "source": "..." } ],
    "tasks": [ { "id": "...", "type": "contradiction_review", "status": "open", "refs": [...] } ]
  }
  ```
- Concept link extraction: heading adjacency heuristic + cross-document term co-occurrence.
- Contradiction detection: CONTRADICTION_CUES scan on ingested chunk text → creates `open` task
  in `state.json`; resolved when operator marks task `done` via `wikiLint()` annotation.

#### Guardrail hardening

Current UNSAFE_PATTERNS list (Python MVP):
- `"ignore previous instructions"`, `"ignore all previous instructions"`, `"system prompt"`,
  `"reveal secret"`, `"api key"`, `"password"`, `"private key"`

Extensions for the C++ plugin:
- Add `"sudo"`, `"base64 decode"`, `"eval("`, `"exec("` patterns.
- Guardrail check applies to: (a) incoming `query_text`, (b) each chunk's `text` before inclusion.
- `WikiQueryResult::query_flagged_for_prompt_injection` = `true` when query matched; result set
  is still returned with safe chunks only.
- `WikiQueryResult::filtered_unsafe_chunks` = count of excluded chunks.

#### Wikipedia dump ingestion

The existing `WikipediaPipeline` C++ implementation handles:
- Compressed XML stream parsing (`wikipedia_xml_parser.cpp`)
- Article transformation and validation (`wikipedia_transform.cpp`, `wikipedia_validator.cpp`)
- Multi-projection storage: vector (`wikipedia_project_vector.cpp`), graph
  (`wikipedia_project_graph.cpp`), timeseries (`wikipedia_project_timeseries.cpp`)
- Resumable checkpointing (`wikipedia_checkpoint.cpp`)

The plugin wraps this pipeline behind `ingestWikipediaDump()`:
1. Check sub-feature license `"llm_wiki_wikipedia"`; return `Status::PermissionDenied` if absent.
2. Instantiate `WikipediaPipeline` with `WikipediaConfig` derived from `WikiDumpIngestOptions`.
3. Run pipeline; stream `WikiIngestResult` accumulation (files → articles, chunks → chunks).
4. On interruption, persist checkpoint; resume from last committed batch on next call.

### Test Strategy

- **Unit (LWP-01..LWP-20)**: focused tests in `tests/` of this repo
  - `LWP-01..08`: `ingest()` + `query()` round-trip with hash provider; Recall@k ≥ 0.8 on fixture set
  - `LWP-09..16`: workspace lifecycle (init/ingest/query/lint); log entry count; page creation;
    orphan detection accuracy
  - `LWP-17..20`: guardrail coverage; prompt-injection detection; unsafe chunk exclusion
- **Integration (LWP-INT-01..04)**: live RocksDB fixture
  - `LWP-INT-01`: Phase B write → query round-trip; verify RRF score ordering
  - `LWP-INT-02`: Phase A → Phase B migration; verify no chunk loss
  - `LWP-INT-03`: concurrent ingest + query; no data races under sanitizer
  - `LWP-INT-04`: workspace state.json corruption recovery; plugin reports `Status::Error`
    and does not panic
- **Wikipedia dump (LWP-WIKI-01..02)**:
  - `LWP-WIKI-01`: ingest 1k-article subset; verify article count, chunk count, checkpointing
  - `LWP-WIKI-02`: sub-feature license absent → `ingestWikipediaDump()` returns `PermissionDenied`
- **Edition gate (LWP-GATE-01)**: plugin returns `Status::Error` when `initialize()` is called
  in a community-edition runtime
- **Performance gate (LWP-PERF-01)**: p95 query latency < 200 ms at 5k chunks (reference hardware)

### Performance Targets

| Operation | Condition | Target |
|---|---|---|
| `query()` p95 latency | ≤ 5k chunks, Phase A | < 200 ms |
| `query()` p95 latency | ≤ 50k chunks, Phase B | < 100 ms |
| `query()` throughput | Phase B, 16 concurrent | ≥ 500 QPS |
| `ingest()` throughput | hash provider, batch=100 | ≥ 10k chunks/s |
| Wikipedia dump ingest | `WikipediaPipeline`, local disk | ≥ 5k articles/s |
| Embedding cache hit rate | re-ingest unchanged docs | ≥ 99% |

### Security / Reliability

- Prompt-injection and unsafe-content guardrails applied to all query/retrieval paths.
- Plugin initialization validates edition gating before any I/O; fail closed.
- Wikipedia dump ingest validates XML structure and article size; rejects oversized articles
  (> 10 MB raw text) without aborting the batch.
- Sub-feature license checked at call time, not only at load time; avoids license-bypass through
  deferred initialization.
- Workspace `state.json` writes are atomic (write-to-temp + rename) to prevent corruption
  on unexpected process termination.
- Signed plugin verification enforced in production CI via SHA-256 digest;
  `AdapterTrustPolicy::kTrustAll` permitted only in development builds.
- Memory: `WikiIndexStore` Phase B writes hold an exclusive lock for the minimum duration;
  query reads use shared lock; no lock held during embedding computation.

---

## wiki_index_store (Phase B — RocksDB-native enhancements)

### Scope

Extend the existing `WikiIndexStore` (`include/llm/wiki_index_store.h`) from Phase A
(in-memory BM25) to a fully RocksDB-native hybrid store.

### Design Constraints

- Phase A `JsonWikiIndexReader` must remain functional as a fallback (no breaking API change).
- Phase B is activated only when `WikiIndexConfig::rocksdb_dir` is non-empty.
- The `query()` method signature does not change between phases.

### Required Interfaces

- `IWikiIndexWriter::writeBatch(chunks)` — unchanged API; Phase B writes to RocksDB tables.
- `IWikiIndexReader::query(text, top_k, min_score)` — unchanged API; Phase B uses BM25 + HNSW.
- `WikiIndexStore::rebuildFromJson(path)` — new method for Phase A → Phase B migration.

### Implementation Notes

- BM25 fulltext index via `SecondaryIndexManager` on `"content"` column (already wired).
- HNSW cosine vector index via `VectorIndexManager` on `"embedding"` column (already wired).
- RRF fusion via `HybridRetriever` with `rrf_k = 60.0` default (already wired).
- Missing piece: persistent embedding cache keyed on `(doc_id + sha256(text))` stored in
  a separate RocksDB column family to avoid re-embedding unchanged chunks.
- `rebuildFromJson()`: reads `index.json`, calls `writeBatch()` in batches of 1000 chunks.

### Test Strategy

- `WIS-B-01..16` exist (`tests/llm/test_wiki_index_store_phase_b.cpp`).
- Additional: `WIS-B-17` — embedding cache hit/miss verification;
  `WIS-B-18` — `rebuildFromJson()` round-trip integrity.

### Performance Targets

- Phase B `query()` p95 < 100 ms at 50k chunks.
- `writeBatch()` ≥ 10k chunks/s.
- Embedding cache hit rate ≥ 99% on re-ingest of unchanged docs.

### Security / Reliability

- Exclusive `shared_mutex` write lock during `writeBatch()`; held for minimum duration.
- Shared read lock during `query()`.
- Stale cache entries evicted on `writeBatch()` when chunk text has changed (SHA-256 mismatch).

---

## wiki_chunk_splitter (enhancements)

### Scope

Extend `WikiChunkSplitter` to support non-Markdown source formats.

### Design Constraints

- `split(file_path, content)` API remains unchanged.
- New formats detected by file extension or explicit format hint; Markdown is the default.

### Required Interfaces

- `WikiChunkSplitter::splitAsciiDoc(file_path, content)` — AsciiDoc heading-aware split.
- `WikiChunkSplitter::splitReStructured(file_path, content)` — reST heading underline detection.
- Optional: auto-detect by file extension in `split()`.

### Implementation Notes

- AsciiDoc: `=`, `==`, `===` heading markers.
- reST: underline-based headings (`=====`, `-----`, `~~~~~`, etc.).
- Sliding-window and overlap parameters are shared across formats.

### Test Strategy

- Unit tests: `WCS-EXT-01..04` — AsciiDoc and reST fixture documents; verify correct section splits.

### Performance Targets

- No measurable regression vs. Markdown split at equivalent document sizes.

### Security / Reliability

- Oversized documents (> 50 MB) rejected with `std::invalid_argument` before any allocation.

---

## wiki_rag_source (enhancements)

### Scope

Extend `WikiRagSource` to support multi-namespace fusion and per-namespace scoring weights.

### Design Constraints

- `retrieveFromWiki(ctx)` API remains unchanged; additional config fields are optional.

### Required Interfaces

- `WikiRagSourceConfig::namespaces` — list of named `IWikiIndexReader` instances with optional weight.
- `WikiRagSource::addNamespace(name, reader, weight)` — register additional reader for fusion.

### Implementation Notes

- Multi-namespace fusion: run `query()` on each reader; merge candidate lists by RRF or weight.
- Provenance tag extended: `"retrieve:wiki-hybrid:<namespace>"` per contributing namespace.

### Test Strategy

- Unit tests: `WRS-NS-01..04` — multi-namespace fixture; verify per-namespace provenance tags.

### Performance Targets

- `retrieveFromWiki()` latency increase ≤ 20% when adding a second namespace reader.

### Security / Reliability

- `fail_open = true` propagates to each namespace; single-namespace failure does not abort fusion.
