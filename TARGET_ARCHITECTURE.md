# TARGET_ARCHITECTURE.md

# ThemisDB Target Architecture
## Hybrid Knowledge Retrieval Architecture

**Status:** Active (source-backed LLM Wiki layer reality-check refresh)
**Date:** 2026-09-15

---

## 1. Overview

The target architecture of ThemisDB is a layered hybrid retrieval and reasoning system composed of five primary layers:

1. ANN Frontdoor
2. Tensor Mid-Layer
3. Graph Truth Layer
4. LLM Wiki / Knowledge Context Layer
5. LLM / LoRA Final Layer

This architecture is intended to support scalable, explainable, distributed, and adaptive RAG.

---

## 2. Layer Model

## 2.1 ANN Frontdoor

### Technologies
- HNSW (hot tier, datasets ≤ 1 M vectors)
- ScaNN (medium tier, datasets 1 M – 50 M vectors)
- DiskANN (cold/large tier, billion-scale, requires `THEMIS_ENABLE_DISKANN`)
- Distributed fan-out (cross-shard aggregation for ShardSummary scopes)

### Responsibilities
- fast semantic candidate retrieval
- top-k shortlist generation
- retrieval over documents, chunks, entities, adapters, packages, and shard summaries
- hot/cold separation

### Supported ANN Artifact Classes

All six artifact classes are registered as first-class `AnnScopeKind` values:

| Scope kind   | Typical scope_id prefix | Default routing              |
|--------------|-------------------------|------------------------------|
| Document     | `doc:`                  | HNSW (hot) / ScaNN (cold)    |
| Chunk        | `chunk:`                | HNSW (hot) / ScaNN (cold)    |
| Entity       | `entity:`               | HNSW (hot) / DiskANN (cold)  |
| Adapter      | `adapter:`              | HNSW (hot) / DiskANN (cold)  |
| Package      | `pkg:`                  | HNSW (hot) / ScaNN (cold)    |
| ShardSummary | `shard:`                | DISTRIBUTED when shard_aware |

### HNSW vs DiskANN Decision Tree

```
dataset_size ≤ hnsw_max_elements (default 1 M) AND hot_tier=true
  → HNSW

dataset_size > hnsw_max_elements AND ≤ scann_max_elements (50 M)
  → ScaNN

dataset_size > scann_max_elements OR (hot_tier=false AND diskann_available)
  → DiskANN

shard_aware=true AND shard backends registered
  → DISTRIBUTED (fan-out + merge)

no backend registered
  → FLAT_BRUTE_FORCE (safe fallback)
```

### Output
- semantic candidate set for tensor refinement

---

## 2.2 Tensor Mid-Layer

### Responsibilities
- candidate compression
- redundancy reduction
- routing
- tensor fingerprints
- structural summaries
- relational approximation
- shard relevance estimation
- adapter/package similarity

### Output
- condensed candidate space
- tensor summaries
- routing/prioritization signals

---

## 2.3 Graph Truth Layer

### Responsibilities
- exact relation validation
- provenance
- evidence chains
- ACL / permissions
- policy-aware constraints
- exact multi-hop validation

### Output
- validated evidence set
- provenance-aware context

---

## 2.4 LLM Wiki / Knowledge Context Layer

**Source:** `src/llm_wiki/`, `include/llm_wiki/`, `include/llm/wiki_index_store.h`

### Current Source-Backed Responsibilities
- expose the enterprise `ILLMWikiPlugin` integration surface for ingest, query, workspace, and lint operations
- enforce edition gating (`enterprise`, `hyperscaler`, `military`) and the optional `llm_wiki_wikipedia` sub-feature
- manage persistent wiki workspaces via `WikiWorkspaceOrchestrator` and checksum-validated workspace state
- apply guardrails for prompt-injection-style unsafe query/content patterns
- load and hot-reload YAML process policy with fail-closed validation
- activate RocksDB-backed persistence when available; otherwise keep the documented degraded in-memory fallback for non-production/test paths

### Current Runtime Backends

The checked-in implementation currently builds one wiki runtime around the public `ILLMWikiPlugin` contract:

| Runtime Path | Current State | Source Evidence |
|--------------|---------------|-----------------|
| Markdown / workspace ingest-query flow | Implemented in `src/llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp` | `ingest()`, `query()`, `wikiInit()`, `wikiIngest()`, `wikiQuery()`, `wikiLint()` |
| Wikipedia dump ingest | Implemented behind the `llm_wiki_wikipedia` feature gate | `ingestWikipediaDump()`, `edition_gate.*` |
| Confluence / Notion / internal-wiki connectors | Not present in the current repository snapshot | no checked-in backend code under `src/llm_wiki/` |

### Retrieval / Persistence Notes
- the long-term hybrid path uses `WikiIndexStore` (BM25 + HNSW + RRF) for production retrieval
- current checked-in `src/llm_wiki/` runtime still centers on the plugin/workspace flow and staged Phase B persistence work
- RocksDB persistence is source-backed through `rocksdb_wiki_store.*` and compile-time `THEMIS_USE_ROCKSDB` wiring

### Output
- workspace-scoped wiki context for downstream LLM prompting
- persisted workspace/audit artefacts via the wiki workspace flow
- gating/guardrail outcomes for caller-visible allow/deny behavior

### Error Handling Surface
- edition or feature not enabled -> `PermissionDenied`
- plugin not initialized -> `NotInitialized`
- invalid process policy / hot-reload failure -> fail closed with `Status::Error(...)`
- unsafe query with `fail_open=false` -> empty result after guardrail detection

---

## 2.5 LLM / LoRA Final Layer

### Responsibilities
- final grounded generation
- domain adaptation via LoRA/AdaLoRA
- structured prompt assembly
- adaptive answer generation

### Input Sources
- vector candidates
- tensor summaries
- graph evidence
- wiki context (from LLM Wiki layer)
- trust/provenance metadata

### Output
- grounded answer
- optionally justification metadata

## 3. Retrieval Pipeline

### Current Classical Pattern
`query -> embedding -> top-k chunks -> prompt -> answer`

### Target Pattern
`query -> ANN frontdoor -> tensor compression/routing -> graph validation/evidence -> wiki context enrichment -> LLM/LoRA generation`

---

## 4. Long-Term Extensions

## 4.1 Unified Knowledge Tensor Layer
Shared tensor-based representation substrate across:
- graph knowledge
- embeddings
- documents
- process models
- adapter knowledge
- provenance

## 4.2 Tensor-native Graph Reasoning
Approximate reasoning support through:
- factorized relation propagation
- tensor contraction
- approximate relational inference

## 4.3 Federated / Cross-shard Tensor Summaries
Distributed architecture based on:
- shard-level tensor summaries
- selective exact loading
- reduced fan-out and network transfer

---

## 5. Architectural Principles

1. ANN is not the truth layer.
2. Tensor is not a graph replacement.
3. Graph remains exact and evidence-bearing.
4. LLM is not the retrieval source of truth.
5. Approximation may prioritize, but not replace exact correctness where governance matters.

---

## 6. Non-Goals

- replacing graph with tensors
- replacing ANN with tensors
- replacing exact policy checks with approximations
- making every subsystem tensor-native prematurely

---

## 7. Success Criteria

- reduced retrieval latency
- smaller prompt contexts
- better evidence quality
- lower cross-shard traffic
- better adapter and package lifecycle handling
- stronger provenance integration

---

## 8. Recommended Follow-Ups

- gap analysis
- per-layer inventory
- issue-based implementation plan
- ADRs per major architectural decision
