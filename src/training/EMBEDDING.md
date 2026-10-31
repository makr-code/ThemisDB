---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embeddings and Vector Search

## Scope and ownership

The training module has two distinct vector-related paths:

1. `DataSelectionPipeline` performs diversity-oriented sample selection.
2. `KnowledgeGraphEnricher` can query an existing ThemisDB vector index to find
   similar documents.

Neither path establishes that the training module owns or trains an embedding
model. Vector-index construction and embedding generation belong to the
integrating indexing/model components.

## Data-selection representation

`LoRADataSelectionConfig` contains an `embedding_model` setting (default
`multilingual-e5-large`), but `DataSelectionPipeline::clusterAndSample()`
currently derives an eight-element normalized vector from character values,
initializes centroids from samples, runs five Lloyd iterations, and selects
samples closest to centroids. It does not invoke that configured model or an
embedding service. Treat its output as a lightweight diversity heuristic, not
semantic embedding-based clustering.

This distinction matters when comparing experiment results or changing the
configuration: changing the model-name field alone does not switch the
algorithm. Any future replacement needs a defined embedding source, vector
dimension/metric contract, failure behavior, and parity/performance tests.

## Enricher vector-index contract

`KnowledgeGraphEnricher::setVectorIndex()` accepts a non-owning
`VectorIndexManager*`. The caller must initialize the index, populate document
vectors under keys matching document IDs, and keep the manager alive for the
enricher's lifetime. `findSimilarDocuments()` obtains the query document's
stored vector, asks the index for `max_results + 1` neighbors so it can exclude
the query itself, and maps cosine distance to a clamped similarity score.
`similarity_threshold` is applied to returned scores by the enricher.

Edge behavior:

- Empty document IDs or a zero result limit produce no results.
- A missing query vector produces an empty result.
- An unsuccessful index search produces an empty result.
- In production, requesting similarity search without an injected vector
  manager throws a configuration error. Under `THEMIS_TEST_MODE`, it returns an
  empty result instead.

Do not treat an empty result as proof that no similar documents exist until
index initialization, key coverage, and search status have been checked.

## Evaluation and diagnostics

Validate a vector change using a fixed set of document IDs with known stored
vectors. Record index status, query ID, requested `k`, returned IDs, score
range, self-exclusion, and behavior for absent vectors and failed searches.
For selection clustering, test deterministic input ordering and selected
sample counts separately from semantic-retrieval quality. No embedding-quality
benchmark is claimed by this module's current performance mapping.

## Source evidence

- [`lora_data_selection.cpp`](./lora_data_selection.cpp) — hash-derived
  selection vectors and clustering.
- [`lora_data_selection.h`](../../include/training/lora_data_selection.h) —
  configuration field and clustering interface.
- [`knowledge_graph_enricher.h`](../../include/training/knowledge_graph_enricher.h)
  and [`knowledge_graph_enricher.cpp`](./knowledge_graph_enricher.cpp) —
  vector-manager ownership, query flow, score threshold, and failure behavior.
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — module and index boundaries.

Related: [`MODELING.md`](./MODELING.md),
[`API.md`](./API.md), and
[`TRUST.md`](./TRUST.md).
