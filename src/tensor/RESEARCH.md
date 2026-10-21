---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Tensor Module Research Notes

## Scope and evidence base

This document summarizes the engineering assumptions behind the tensor module as they are reflected in the code and the module-level docs. The primary evidence is the runtime behavior in:

- `src/tensor/tensor_index_manager.cpp`
- `src/tensor/tensor_fingerprint_graph.cpp`
- `src/tensor/adapter_repository.cpp`
- `include/storage/tensor_router.h`
- `include/storage/tensor_train_decomposer.h`
- `src/tensor/ARCHITECTURE.md`

The tensor module is not a generic ML framework; it is the TT-based indexing, fingerprinting, and bridge layer that makes ThemisDB able to store and retrieve compact tensor representations for adapters and vector-like workloads.

## Why this module exists

The code treats tensor compression as a latency and memory control problem rather than a pure model-serving problem. The `TensorIndexManager` decides whether a workload should be represented by a TT-store, a hybrid TT/native path, or a standard HNSW route. The routing choice is intentionally heuristic and guarded by `TensorRouter::DataProfile` / `kappa_estimate` thresholds in `TensorRouter::decide()`, so the system can choose an appropriate storage path before a full decomposition is performed.

The module then ties together three contracts:

1. TT decomposition and reuse from `storage::TTTrain` and `TensorTrainDecomposer`
2. Storage-level routing and hybrid policy from `storage::TensorRouter`
3. Similarity and dedup surfaces from `TensorFingerprintGraph` and `AdapterRepository`

That separation keeps core decomposition logic near storage and preserves a clear failure boundary at the module API.

## Research questions the implementation addresses

### 1. When is TT compression worth it?

`TensorRouter::decide()` encodes simple routing thresholds:

- `kappa_estimate >= 1.7 && dim >= 256` -> `TENSOR_TRAIN`
- `kappa_estimate >= 1.3` -> `HYBRID`
- otherwise -> `HNSW`

This heuristic reflects the project’s assumption that higher-dimensional and more compressible workloads are the best fit for TT-based storage, while low-dimensional or unpredictable workloads should stay on the standard vector index path.

### 2. Can retrieval stay bounded when the full TT graph is not loaded?

`TensorFingerprintGraph::findSimilarByFingerprint()` explicitly computes cosine similarity using cached norms and zero-padding semantics. The code avoids redoing expensive TT products during approximate key lookups. It also enforces a degenerate case limit: if either vector has near-zero norm, it returns `0.0` rather than propagating invalid math.

This is a deliberate design trade-off: approximate fingerprint matching is cheap and bounded, while exact TT similarity remains available in the `findSimilar()` path when a registered train is available.

### 3. Can graph operations fail explicitly rather than silently?

The fingerprint graph code contains explicit diagnostics for invalid query self-inner products, invalid score values, missing candidate trains, and bad cross products. This is direct evidence that silent degradation is treated as a correctness issue. The code emits `emitFingerprintDiagnostic()` and returns an empty result path instead of prolonging a bad state.

## Core assumptions and trade-offs

- Compressible high-dimensional workloads benefit most from TT decomposition.
- The module assumes a data profile can be approximated from dimension and scale without a full pilot decomposition.
- Similarity ranking may use a fingerprint approximation first, but exact scoring must remain explicit and bounded.
- Failure signals are more important than best-effort fallback behavior.
- `TensorIndexManager` keeps a bounded `pending_operations_` count to avoid unbounded in-flight create storms during concurrent workloads.

## Validation and evaluation path

The module’s design is evaluated through:

- `tests/tensor` focused contract-hardening and edge-case suites
- benchmark gates in `benchmarks` for tensor hot paths
- the module-local roadmap and future enhancements, especially `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`

The docs and code indicate that the tensor module is deliberately designed for explicit diagnostics, bounded latency, and recoverable index lifecycle behavior rather than for opaque automatic optimization. That is the core of the module’s scientific and engineering posture.
