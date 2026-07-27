# ADR: LLM Wiki Secondary Index — C++20 Hybrid Retrieval

<!-- Status: accepted | validated: 2026-07-27 -->
<!-- Links: include/llm/wiki_index_store.h · include/llm/wiki_chunk_splitter.h · include/llm/wiki_rag_source.h -->

## Context

ThemisDB's Python MVP (`scripts/llm_wiki_mvp.py`) provides offline wiki indexing with
hash-based embeddings and BM25 scoring.  Production LLM-assisted query answering
requires an always-available, sub-10 ms retrieval path that integrates with the
existing `ModularRAGPipeline` and can leverage real dense embeddings via `EmbeddedLLM`.

## Decision

Implement a **dual secondary index** for wiki/Markdown documents in C++20:

| Layer | Technology | ThemisDB API |
|-------|-----------|--------------|
| Fulltext BM25 | Inverted token index | `SecondaryIndexManager::createFulltextIndex` |
| Dense KNN | HNSW (COSINE metric) | `VectorIndexManager::init` + `searchKnn` |
| Fusion | Reciprocal Rank Fusion (RRF) | `HybridRetriever::fuse` |
| RAG integration | Stage handler | `WikiRagSource::retrieveFromWiki` |
| Phase A fallback | In-memory BM25 over JSON | `JsonWikiIndexReader` |

### Files introduced

```
include/llm/wiki_index_store.h      — WikiChunk, WikiIndexConfig, interfaces,
                                       WikiIndexStore, JsonWikiIndexReader
include/llm/wiki_chunk_splitter.h   — Heading-aware Markdown splitter
include/llm/wiki_rag_source.h       — RAGStageHandler wrapper
src/llm/wiki_index_store.cpp        — Full implementation
src/llm/wiki_chunk_splitter.cpp     — Full implementation
src/llm/wiki_rag_source.cpp         — Full implementation
tests/llm/test_wiki_index_store.cpp — WIS-01..16 unit tests
tests/llm/test_wiki_rag_quality.cpp — WISQ-01..05 quality gate tests
```

## C++20 Rationale

- `std::shared_mutex` (C++17, available since our toolchain baseline) guards the
  embedding cache with fine-grained read/write separation.
- `[[nodiscard]]` on all query methods enforces result handling at call sites.
- `std::atomic<bool>` for `ready_` ensures a sequentially consistent visibility
  fence between the constructor and concurrent `isReady()` calls.
- `std::filesystem` is used in `wiki_chunk_splitter.cpp` for path normalisation.

## Phase A vs Phase B

| Phase | Reader | Embedding | RocksDB |
|-------|--------|-----------|---------|
| A (current) | `JsonWikiIndexReader` | none (BM25 only) | ❌ |
| B (next) | `WikiIndexStore` | `EmbeddedLLM::embedBatch` | ✅ |

Phase A is self-contained and always runnable in unit tests.  Phase B requires a
live `RocksDB` instance and a loaded `EmbeddedLLM` model.

## Thread Safety Contract

- `WikiIndexStore::query()` — shared lock (multiple concurrent readers allowed).
- `WikiIndexStore::writeChunk()` / `writeBatch()` — exclusive lock.
- `JsonWikiIndexReader` — immutable after `load()`; fully concurrent reads.

## Chunk ID Scheme

```
chunk_id = fnv1a64(file_path + ":" + section_title + ":" + seq)[0:12 hex] + "-" + seq
```

FNV-1a 64-bit is deterministic within a process and across platforms (fixed
algorithm, no stdlib `std::hash` variability).  The 12-char prefix provides
$16^{12} \approx 2.8 \times 10^{14}$ distinct IDs before collision.

## Consequences

- `JsonWikiIndexReader` is the primary entry point for CI/CD unit tests — no
  infrastructure dependencies.
- `WikiIndexStore` is the production entry point; it is tested at the integration
  level only when a RocksDB fixture is available.
- `WikiRagSource::retrieveFromWiki` is a first-class `RAGStageHandler` and can
  be composed with other retrieve handlers without modifying `modular_rag_pipeline.h`.
