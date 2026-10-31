---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "CMakeLists.txt"
  - "include/lora_package.h"
  - "src/lora_package.cc"
  - "../../include/retrieval/dual_read_validator.h"
  - "dual_read_validator.cpp"
  - "../../tests/retrieval/CMakeLists.txt"
  - "../../benchmarks/retrieval/CMakeLists.txt"
---

Exact model backend not verifiable from final document text alone.

# Retrieval module

## What is implemented

`src/retrieval/` currently contains two separate surfaces:

- The `themis_retrieval` static library builds `src/lora_package.cc`. Its declarations are in `src/retrieval/include/lora_package.h`; the target exposes that directory publicly, links `nlohmann_json`, and requires C++17.
- `DualReadValidator` and `RetrievalMetrics` are declared in `include/retrieval/dual_read_validator.h` and implemented in `src/retrieval/dual_read_validator.cpp`. The validator implementation is not part of the `themis_retrieval` source list.

The top-level `include/retrieval/retrieval.h` is currently a placeholder umbrella header. It includes and declares no retrieval API. This module is not a general ANN, hybrid-search, reranking, embedding, or RAG execution stack.

## Artifact store behavior and limits

`LoRAManifestStore` stores packages and products in in-memory `std::unordered_map`s protected by a mutex. It supports CRUD, listing, JSON import/export, manifest SHA-256 verification, and an optional signature-verifier callback. Verification invokes that callback only when both a callback and a non-empty signature are present; without them, a valid manifest hash is sufficient for the method to return true. The store is not persistent and does not enforce lifecycle transitions, deployment authorization, or deployment policy.

## Dual-read validation behavior and limits

`DualReadValidator::ExecuteQuery()` is a placeholder that returns an empty result vector. `ValidateQuery()` obtains judgments from the map populated by `SetGroundTruth()`, invokes the placeholder for both index paths, and returns current-result metrics plus both result lists. It logs a previous-result metric but does not calculate or return a current-versus-previous delta. Missing ground-truth entries return an empty vector; empty results or judgments yield zero-valued quality metrics. `RetrievalMetrics` separately exposes `HasRegression()` and `ComputeDeltas()`.

## Build and tests

Retrieval artifact-library building is controlled by the default-ON CMake option `THEMIS_BUILD_RETRIEVAL`. From the repository root:

```sh
cmake -S . -B build
cmake --build build --target themis_retrieval
```

The exact CMake target is `themis_retrieval` (underscore). Focused tests are registered with CTest under the `retrieval` label; see [`../../tests/retrieval/README.md`](../../tests/retrieval/README.md) for what they cover and their mock/stub boundaries. The dedicated benchmark target is synthetic routing and stub CPU/GPU work, not a production retrieval or GPU benchmark; see [`../../benchmarks/retrieval/README.md`](../../benchmarks/retrieval/README.md).

## Configuration and maturity

There is no retrieval runtime configuration system in this target. The only module-specific configuration described here is the build option `THEMIS_BUILD_RETRIEVAL`. Do not infer production readiness, persistence, deployment controls, index integration, or measured performance from the artifact APIs, tests, roadmap history, or benchmark names.
