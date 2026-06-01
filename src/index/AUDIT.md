# Audit Report - Index Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Summary

| Metric | Result |
|---|---|
| Build registration | pass |
| Source set size | 45+ implementation files in src/index |
| Focused test presence | pass |
| Open hardening findings | yes |
| Critical blockers | none identified |

## Verified Files

- src/index/index_manager.cpp
- src/index/vector_index.cpp
- src/index/advanced_vector_index.cpp
- src/index/gpu_vector_index.cpp
- src/index/gpu_vector_index_vulkan.cpp
- src/index/secondary_index.cpp
- src/index/inverted_index.cpp
- src/index/spatial_index.cpp
- src/index/graph_index.cpp
- src/index/adaptive_index.cpp
- src/index/tiered_index_manager.cpp
- src/index/index_compression.cpp
- src/index/product_quantizer.cpp
- src/index/binary_quantizer.cpp
- src/index/residual_quantizer.cpp
- src/index/approximate_radius_search.cpp
- src/index/distributed_vector_index.cpp
- src/index/multi_gpu_vector_index.cpp
- src/index/workload_replay.cpp

## Findings

### Open

1. [INDEX-AUD-01] backend parity and fallback edge hardening remains active.
- Severity: medium
- Evidence: roadmap/future retain active work for mixed-capability execution scenarios.
- Action: close deterministic regressions across backend degradation and fallback transitions.

2. [INDEX-AUD-02] lifecycle diagnostics need further tightening.
- Severity: medium
- Evidence: active follow-up work for rebuild/tiering/distributed incident observability.
- Action: unify taxonomy and diagnostics for lifecycle failure classes.

3. [INDEX-AUD-03] benchmark depth should broaden for advanced index workflows.
- Severity: low
- Evidence: core mapping is valid, but specialized distributed and advanced retrieval cases need deeper coverage.
- Action: add benchmark depth for advanced index and distribution-heavy workflows.

4. [INDEX-AUD-GI-01] _sensitive boolean fallback in addEdge — legacy encryption field selector.
- Severity: medium
- Evidence: graph_index.cpp addEdge path retains backwards-compat branch for pre-v2.1 documents using `_sensitive=true` instead of `encrypt_fields`.
- Action: Remove after data migration confirms no _sensitive=true records remain. Tracked via LEGACY_COMPAT comment in source.
- Status: annotated; removal pending migration

5. [INDEX-AUD-GI-02] _sensitive boolean fallback in updateEdge — duplicate of GI-01.
- Severity: medium
- Evidence: updateEdge path has same backwards-compat branch as addEdge.
- Action: Remove together with GI-01 after migration.
- Status: annotated; removal pending migration

6. [INDEX-AUD-GI-03] Legacy key format support (pre-v2.0 graph:out/in without graphId segment).
- Severity: low
- Evidence: parseOutKey_, parseInKey_, and scanEdges_ retain branches for the pre-v2.0 key format that omits the graphId segment.
- Action: Remove after confirming no pre-v2.0 graph keys remain in production storage.
- Status: annotated; removal pending storage migration confirmation

### Closed

- core index runtime surfaces are present and source-verified.
- documentation set is synchronized to source-verifiable claims.
- changelog/roadmap role separation is aligned to module governance pattern.
- [INDEX-AUD-LOG-01] hardcoded std::cout/std::cerr in gpu_vector_index.cpp replaced with THEMIS structured logging macros (THEMIS_INFO/WARN/ERROR). All 22 instances fixed; `#include <iostream>` removed.
- [INDEX-AUD-MEM-01] hnswlib index memory leak in VectorIndexManager: hnswIndex_ is now freed in shutdown() and in loadIndex() before replacing existing index pointer.
- [INDEX-AUD-RACE-01] secondary_index write/delete paths now snapshot cached metadata to local containers before processing, removing repeated shared-structure dereferences in hot loops.
- [INDEX-AUD-MEM-02] vector_index HNSW space allocation is RAII-managed via `std::unique_ptr` in init/load paths, preventing leaks when HNSW constructors throw.
- [INDEX-AUD-PERF-01] O(n²) phrase normalization in secondary_index.cpp::computeBM25Scores_ eliminated: normalized phrases are now precomputed once before the outer loop.
- [INDEX-AUD-PERF-02] Multiple missing reserve() calls fixed: tokenResults, values (composite scan), validateProcess errors/warnings, evaluateGateway_ targets, deserializeVisitedNodes nodes.
- [INDEX-AUD-DTOR-01] StackEntry missing destructor in process_graph.cpp: added ~StackEntry() = default.

## Compliance Snapshot

| Requirement | Status |
|---|---|
| Source-verifiable behavior claims | pass |
| Structured forward planning in roadmap/future | pass |
| Historical completion tracked in changelog | pass |
| Core module docs synchronized | pass |