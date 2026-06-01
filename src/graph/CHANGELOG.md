> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Graph Module

All notable changes to the graph module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added
- **B2 — RotatE Knowledge Graph Completion** (`include/graph/rotate_completion.h`, `src/graph/rotate_completion.cpp`; `themis::graph`) — Wave B, issue #5039
  - `RotatEModel` (pImpl, `shared_mutex`) implementing L1 complex-rotation scoring: `‖h ∘ r − t‖₁`.
  - `train(triples, neg_samples, epochs)` — SGD with negative sampling; `score(h, r, t)` → distance.
  - `LinkPredictionHead::predict(entity, relation, top_k)` → scored candidates ranked by RotatE distance.
  - `KGCompletionEngine::setReasoner()` — injects predicted links into `KnowledgeGraphReasoner` via `addFact()`.
  - 16 unit tests: KGC-01..16 (`tests/graph/test_rotate_completion.cpp`).
  - Training loop now applies per-triple L1 sub-gradient updates to entity/relation embeddings (no longer a no-op update path).
  - Stubs: RTE-S01 (approximate SGD; still not full self-adversarial RotatE gradient); RTE-S02 (CPU float32 only).

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified graph benchmark symbols from traversal, optimizer, and tensor-fingerprint benchmark suites.

## [1.3.0] - 2026-03-01

### Added
- EXPLAIN endpoint support and scheduled semantic edge refresh support.

### Changed
- optimizer and parallel traversal internals refined for production stability.

### Fixed
- traversal and concurrency correctness issues in large/fan-out graph scenarios.

## [1.2.0] - 2025-09-01

### Added
- subgraph isomorphism support, path-constraint validation, and optimizer rewrite integration.

### Changed
- configurable traversal parallelism behavior.

### Fixed
- duplicate-candidate behavior in symmetric matching scenarios.

## [1.1.0] - 2025-03-01

### Added
- parallel BFS/DFS traversal and distributed query support.

### Changed
- adjacency representation improvements for dynamic graph workloads.

### Fixed
- shortest-path correctness issue for invalid negative-weight assumptions in BFS mode.

## [1.0.0] - 2024-01-01

### Added
- foundational graph traversal, pattern matching integration, and property filtering paths.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
