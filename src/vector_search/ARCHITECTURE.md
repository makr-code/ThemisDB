# Vector Search Module — Architecture

<!-- Status: current | live implementation under src/index and include/index | validated: 2026-10-12 -->

## Overview

This module documents the architecture and operational boundaries of ThemisDB vector search without creating a second implementation tree. The authoritative implementations live under `src/index/` and `include/index/`, and the module docs remain aligned to those files and their validation artifacts.

## Design Principles

1. **Source-of-truth ownership:** vector-search behavior is defined by the shared index subsystem, not by duplicated files under `src/vector_search/`.
2. **Contract clarity:** query semantics, metric validation, and result ranking remain explicit and reviewable.
3. **Operational evidence:** release gates are anchored in test and benchmark artifacts rather than in unstated assumptions.
4. **Correctness before scale:** approximate search is accepted only with measurable recall, stability, and workload validation.
5. **Explicit limitations:** sharding, persistence, and advanced tuning remain tracked as planned work instead of being implied as current production features.

## Architecture Boundary

```text
Client / Retrieval / Server
       │
       ▼
  vector-search contract
       │
       ▼
   shared index subsystem
   ├─ src/index/vector_index.cpp
   ├─ src/index/advanced_vector_index.cpp
   ├─ src/index/ann_index.cpp
   ├─ src/index/multi_vector_search.cpp
   ├─ include/index/multi_vector_search.h
   └─ include/index/*
       │
       ▼
  distance metric selection + query execution + result ranking
```

## Core Components

### Governance / Release Layer

This directory provides the module-facing description, release checklist, and operational expectations.

Responsibilities:
- define module scope, readiness gates, and release boundaries
- describe the supported runtime behavior of vector search
- retain audit, security, performance, and gap tracking evidence
- connect the module to the live implementation and benchmark/test files

### Shared Index Execution Layer

The live implementation remains in the broader `index` subsystem and performs the actual work:
- vector index construction and management
- ANN / nearest-neighbor search execution
- metric selection and result ranking
- query validation and failure handling

## Data Flow

```text
Input embedding vectors + metadata
       │
       ├─ validate dimension and value sanity
       │
       ├─ select index backend and metric
       │
       ├─ execute shared index path under src/index/
       │
       ├─ run query or batch retrieval
       │
       └─ return ranked results with error statuses when validation fails
```

## Concurrency Model

### Thread safety expectations

1. **Read-heavy searches:** concurrent read access is allowed under the shared index layer's synchronization model.
2. **Write operations:** modifications remain exclusive when rebuild or structural mutation occurs.
3. **Validation gate:** invalid dimensions, NaN/Inf values, and unsupported metrics fail before mutation or result calculation.

### Synchronization principles

- the canonical locking and behavior are defined in the shared index implementation, not by a second vector-search layer
- this module documents the contract and the operational constraints around that behavior

## Known constraints

- this directory is a governance summary, not a duplicate implementation surface
- distributed/search-sharding and persistence features remain planned work unless they exist in the shared index implementation
- real feature ownership must stay aligned with `src/index/` and `include/index/`

## Related artifacts

- [`../../src/index/README.md`](../../src/index/README.md)
- [`../../include/index/multi_vector_search.h`](../../include/index/multi_vector_search.h)
- [`../../src/index/multi_vector_search.cpp`](../../src/index/multi_vector_search.cpp)
- [`../../tests/integration/test_vector_search_soak.cpp`](../../tests/integration/test_vector_search_soak.cpp)
- [`../../tests/vector_search/test_vector_search_highcardinality_stress.cpp`](../../tests/vector_search/test_vector_search_highcardinality_stress.cpp)
- [`../../benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`](../../benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp)

## Performance characteristics

The module's expected performance is defined by the benchmark gate set in `benchmarks/vector_search/` and the soak test.

### Release targets

- insertion throughput: >= 1,000 ops/sec (P95)
- query latency: <= 10 ms for k=10, 128-dim workload (P99)
- concurrent throughput: >= 500 ops/sec at 4 threads
- memory overhead: < 40% over raw vector storage

## Error handling

### Failure modes

1. **Dimension mismatch** → reject invalid insert/query and return an explicit error
2. **NaN or Inf values** → reject before mutation
3. **Empty index** → return empty result or explicit empty-index status when required
4. **Unsupported metric** → fail closed rather than silently falling back
5. **Corruption or resource exhaustion** → preserve state and surface the error instead of continuing silently

## Integration points

### Retrieval-augmented generation (RAG)

- query embeddings reach the shared index search path
- top-k candidate IDs are returned for downstream retrieval logic
- correctness depends on stable metric selection and bounded result validation

### Search and retrieval coordination

- index lifecycle and data flow are owned by the `index` subsystem
- module docs remain the synchronization layer for audits, release readiness, and roadmap planning

## See also

- [`ROADMAP.md`](ROADMAP.md)
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- [`../../src/index/README.md`](../../src/index/README.md)
- [`../../include/index/multi_vector_search.h`](../../include/index/multi_vector_search.h)

## Implementation status

The vector-search module is currently in a governance-aligned, release-ready state with the actual implementation living under the broader index subsystem. The module docs are synchronized with the actual sources and the validation artifacts that support the release gate.
