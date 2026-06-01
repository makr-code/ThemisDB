# Security - Index Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the index module focuses on safe index mutation/query boundaries, deterministic backend fallback behavior, and protection against unsafe cross-context retrieval or lifecycle operations.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe index mutation paths | explicit validation and guarded update/rebuild flows |
| backend capability mismatch | deterministic GPU/backend fallback and explicit error surfacing |
| cross-context retrieval leakage | index scoping and bounded lookup behaviors |
| hidden lifecycle regressions | explicit rebuild/tiering observability and operational surfaces |
| corrupted similarity/distance behavior | quantization/metric validation and benchmark-backed checks |

## Implemented Security Controls

- guarded index update and rebuild pathways avoid silent corruption.
- backend-specific flows expose deterministic unsupported/degraded outcomes.
- metric and quantization paths are bounded by explicit index configuration checks.
- lifecycle and distribution operations expose observable status and failure surfaces.
- **[v3-remediation, 2026-06-01]** All structured-logging violations in gpu_vector_index.cpp resolved: hardcoded `std::cout`/`std::cerr` replaced with THEMIS logging macros; no plaintext diagnostic output on sensitive vector operation paths.
- **[v3-remediation, 2026-06-01]** hnswlib memory leak closed: VectorIndexManager::shutdown() and loadIndex() now correctly free the HNSW index before replacement or on destruction, preventing use-after-free scenarios in reload flows.
- **[v3-remediation, 2026-06-01]** Legacy `_sensitive` encryption field selector annotated in graph_index.cpp (addEdge/updateEdge) with LEGACY_COMPAT tracking comments (INDEX-AUD-GI-01/02); removal tracked pending data migration.

## Security Follow-ups

- continue hardening multi-GPU/distributed edge behavior under partial capability.
- tighten diagnostics for rebuild/tiering failure classes.
- expand stress coverage for high-volume mixed index workloads.

## Sourcecode Verification (Module: index/security)

- Verified files:
  - src/index/index_manager.cpp
  - src/index/vector_index.cpp
  - src/index/gpu_vector_index.cpp
  - src/index/secondary_index.cpp
  - src/index/spatial_index.cpp
  - src/index/tiered_index_manager.cpp
  - src/index/index_compression.cpp
- Verified controls:
  - guarded index mutation/search paths
  - deterministic backend fallback behavior
  - observable lifecycle and integrity-relevant operations