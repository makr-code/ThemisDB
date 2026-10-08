# Vector Search Module — Architecture

<!-- Status: governance-facade | index implementation lives under src/index and include/index | validated: 2026-10-08 -->

## Overview

The vector search module is the governance and release-facing description of ThemisDB's similarity-search feature. The active algorithmic implementation is owned by the shared index subsystem, especially under `src/index/` and `include/index/`, but this module documents the contract, operational expectations, and risk model that the wider system depends on.

## Design Principles

1. **Contract-first behavior:** vector search semantics remain bounded by explicit dimension, metric, and result validation rules
2. **Implementation delegation:** algorithmic work remains in the index module instead of duplicated in `src/vector_search/`
3. **Operational clarity:** release gates, soak tests, and benchmark thresholds remain traceable to concrete artifacts
4. **Correctness over novelty:** approximate search is accepted only with measurable recall and stability checks
5. **Scalability with explicit limits:** distributed and persistence features are tracked as planned work rather than implied as current implementation

## Architecture Boundary

```
Client / Retrieval / Server
       │
       ▼
   vector search contract
       │
       ▼
   shared index subsystem
   ├─ src/index/vector_index.cpp
   ├─ src/index/advanced_vector_index.cpp
   ├─ src/index/ann_index.cpp
   ├─ src/index/multi_vector_search.cpp
   ├─ src/index/distributed_vector_index.cpp
   └─ include/index/*
       │
       ▼
  distance metric + query execution + result ranking
```

## Core Components

### Public Contract Layer

This module describes the behavior expected by downstream users rather than reimplementing the algorithms locally.

Responsibilities:
- define module scope and release gates
- document correctness and operational expectations
- track known gaps and future roadmap items
- connect the feature to the benchmark and validation artifacts in the repository

### Shared Index Execution Layer

The live implementation remains under the broader index subsystem and performs the actual work:
- index construction and maintenance
- ANN search execution
- distance metric routing and result ranking
- index rebuild and sharding pathways when enabled by configuration

## Data Flow

### Index Construction and Search

```text
Input embedding vectors + metadata
       │
       ├─ validate dimension and value sanity
       │
       ├─ select configured index backend (vector/ANN/advanced index)
       │
       ├─ execute index build or update path in src/index/
       │
       ├─ run query/retrieval path
       │
       └─ return ranked neighbors and metrics to caller
```

### Operational Boundaries

- The governance docs here should remain aligned with the actual index implementations under `src/index/`.
- When implementation details change, update this module's docs and the shared index evidence in the same change.
- Distributed and persistence features remain planned unless they are actually present in the live source tree.

## Concurrency Model

### Thread Safety Expectations

1. **Read-heavy search operations:** concurrent read access is allowed under the index layer's locking model.
2. **Write operations:** index modification remains exclusive when rebuild or structural changes occur.
3. **Bounds and validation:** invalid dimensions, NaN/Inf values, and unsupported metrics fail before mutation or result calculation.

### Synchronization Principles

- Shared index code handles locking near the implementation boundary rather than through a duplicate `vector_search` shadow implementation.
- Search correctness depends on the selected algorithm and its runtime validation, not on this documentation-only module.

## Known Constraints

- This directory is not the authoritative source of the implementation; it is the authoritative summary of the module contract.
- The live feature is subject to the broader index module roadmap and release gates.
- Any feature intentionally implemented outside the shared index tree must be reflected in `README.md`, `ROADMAP.md`, and `MODULE_GAPS.md` immediately.

## Related Artifacts

- `src/index/README.md` for the concrete index implementation overview
- `tests/integration/test_vector_search_soak.cpp` for soak and recall validation
- `tests/vector_search/test_vector_search_highcardinality_stress.cpp` for stress checks
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` for performance gate references

## Performance Characteristics

### Target Latencies (P99)

- **Insertion:** < 100 µs per vector (HNSW)
- **Search k=10:** < 10 ms (HNSW with 1M vectors)
- **Search k=100:** < 50 ms (IVF with large index)

### Throughput

- **Insert Throughput:** > 1000 vectors/sec
- **Query Throughput:** > 100 queries/sec
- **Concurrent Queries:** ≥ 100 with minimal overhead

### Resource Consumption

- **Per-Vector Memory:** ~0.1-0.5 MB (HNSW with M=16)
- **Per-Vector Memory:** ~0.05-0.2 MB (IVF with clustering)
- **Index Overhead:** 20-40% above raw vector storage

## Error Handling

### Graceful Degradation

1. **Dimension Mismatch** → Return error; don't corrupt index
2. **Invalid Vector** (NaN, inf) → Skip insertion; log warning
3. **Empty Index** → Return empty results
4. **Index Corruption** → Rebuild from scratch if possible
5. **Out of Memory** → Reject insertions; preserve existing index

### Error Codes (E5400–E5499)

- E5400: Invalid vector dimension
- E5401: Vector contains NaN or inf
- E5402: Index is empty
- E5403: Search returned no results
- E5404: Index corruption detected

## Integration Points

### Retrieval-Augmented Generation (RAG)

Vector search integrates into RAG pipelines:
- Query embedding passed to similarity search
- Top-k documents retrieved for context
- Results fed to LLM for answer generation

### Semantic Search

Used in document retrieval:
- Natural language queries converted to embeddings
- Vector search finds relevant documents
- Results ranked by similarity

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/vector_search/vector_index.h`](../../include/vector_search/vector_index.h) — Public API

## Implementation Status

Vector search module provides a **stable, production-ready facade** for similarity search operations. Core algorithms (HNSW, IVF) and distance kernels are implemented and integrated from:
- `include/index/` — Algorithm foundations (HNSW, IVF data structures)
- `include/utils/` — SIMD distance computation
- Test verification: `tests/integration/test_vector_search_soak.cpp`, `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- Benchmark validation: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`, `benchmarks/ann/bench_vector_search.cpp`

**Wave D Status:** ✓ COMPLETE (2026-09-22)
- Soak tests delivered and passing (≥ 2 000 ops/sec, recall ≥ 0.9, no corruption)
- Benchmark gates defined and measurable (VS-BM-01 to VS-BM-04)
- Production readiness checklist signed off
- Module available for production deployment

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose | Status |
|--------|-----------------|---------|--------|
| index | `include/index/` | Foundational index structures (HNSW, IVF) reused by vector search | ✓ VERIFIED |
| storage | `include/storage/` | Planned: persistence of serialized vector indices | PHASE 6 |
| utils | `include/utils/` (simd_distance.cpp) | SIMD-accelerated distance computation helpers | ✓ VERIFIED |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes | Status |
|--------|-----|-------|--------|
| rag | `include/vector_search/vector_index.h` | RAG uses ANN index for embedding similarity retrieval | ✓ VERIFIED |
| server | `include/vector_search/` | Server exposes vector similarity query APIs | ✓ VERIFIED |

## Integration Points

### Critical Integration: ANN Index for RAG
**Files:** `include/vector_search/vector_index.h` (planned) ↔ `rag/`  
**Contract:** RAG calls vector search operations and receives ranked embedding IDs; index does not mutate result objects after delivery.  
**Thread Safety:** Read queries concurrent-safe via shared_mutex; index mutations (insert/delete) serialized via exclusive write lock.  
**Status:** ✓ VERIFIED via `tests/integration/test_vector_search_soak.cpp`
