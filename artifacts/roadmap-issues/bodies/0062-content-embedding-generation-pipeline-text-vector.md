### Context

This issue implements the roadmap item 'Embedding Generation Pipeline (Text → Vector)' for the content domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Embedding Generation Pipeline (Text → Vector)

### Goal

Deliver the scoped changes for Embedding Generation Pipeline (Text → Vector) in src/content/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Embedding Generation Pipeline (Text → Vector)
**Priority:** High
**Target Version:** v1.8.0

After text extraction (from documents, PDF, OCR output), automatically generate vector embeddings for semantic search. Wire `content_manager_llm.cpp` into the ingestion pipeline so that every ingested text document optionally receives an embedding stored alongside the content.

**Implementation Notes:**
- `[x]` Add `EmbeddingStage` to the ingestion pipeline in `content_manager.cpp`; activated when `ContentPolicy::embeddingModel` is set for a collection.
- `[x]` `content_manager_embedding.cpp` exposes `ContentManager::generateEmbedding(text, model_name)` returning `std::vector<float>`; delegates to `EmbeddingPipeline::generateEmbedding()` when a pipeline is attached, falls back to the registered `TextProcessor::generateEmbedding()`. (`content_manager_llm.cpp` handles LLM analysis; embedding is separate.)
- `[x]` Store embedding under `emb:<ContentId>` in RocksDB for direct lookup by ContentId; also registered in the vector index via `vector_index_->addEntity()` under `chunks:<chunk_id>`.
- `[x]` Batch API available: `EmbeddingPipeline::generateEmbeddingBatch()` processes up to `batch_size=32` texts per call.
- `[x]` On model failure (timeout > 5 s or error), content is stored without embedding and `content_embedding_failures_total` is incremented via `ContentMetrics::recordEmbeddingFailure()` when a metrics sink is configured.

**Performance Targets:**
- Embedding latency (384-dim model, batch=32): < 50 ms on CPU; < 5 ms on CUDA GPU.
- Ingestion pipeline with embedding adds < 100 ms overhead vs ingestion without embedding (batch amortised).

### Acceptance Criteria

- [x] Add `EmbeddingStage` to the ingestion pipeline in `content_manager.cpp`; activated when `ContentPolicy::embeddingModel` is set for a collection.
- [x] `content_manager_embedding.cpp` exposes `ContentManager::generateEmbedding(text, model_name)` returning `std::vector<float>`; delegates to `EmbeddingPipeline::generateEmbedding()` when a pipeline is attached, falls back to the registered `TextProcessor::generateEmbedding()`. (`content_manager_llm.cpp` handles LLM analysis; embedding is separate.)
- [x] Store embedding under `emb:<ContentId>` in RocksDB for direct lookup by ContentId; also registered in the vector index via `vector_index_->addEntity()` under `chunks:<chunk_id>`.
- [x] Batch API available: `EmbeddingPipeline::generateEmbeddingBatch()` processes up to `batch_size=32` texts per call.
- [x] On model failure (timeout > 5 s or error), content is stored without embedding and `content_embedding_failures_total` is incremented via `ContentMetrics::recordEmbeddingFailure()` when a metrics sink is configured.
- [ ] Embedding latency (384-dim model, batch=32): < 50 ms on CPU; < 5 ms on CUDA GPU.
- [ ] Ingestion pipeline with embedding adds < 100 ms overhead vs ingestion without embedding (batch amortised).

### Relationships

- Roadmap row: #62 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/content/FUTURE_ENHANCEMENTS.md#embedding-generation-pipeline-text--vector
- Source key: roadmap:62:content:v1.8.0:embedding-generation-pipeline-text-vector

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:62:content:v1.8.0:embedding-generation-pipeline-text-vector -->
<!-- roadmap-ref: row=62;module=content;target=v1.8.0 -->
<!-- roadmap-detail: src/content/FUTURE_ENHANCEMENTS.md#embedding-generation-pipeline-text--vector -->
