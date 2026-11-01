---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-11-01"
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
  - "src/lora_package_config.cc"
  - "../../config/ai_ml/retrieval.yaml"
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

- The `themis_retrieval` static library builds `src/lora_package.cc`, `src/lora_package_config.cc`, and `../../src/rag/deployment_gate_controller.cpp`. Its declarations are in `src/retrieval/include/lora_package.h`; the target exposes that directory and the repository `include/` directory publicly, links `nlohmann_json` and RocksDB, and requires C++20. It also uses `themis_core`, yaml-cpp, and OpenSSL Crypto privately.
- `DualReadValidator` and `RetrievalMetrics` are declared in `include/retrieval/dual_read_validator.h` and implemented in `src/retrieval/dual_read_validator.cpp`. The validator implementation is not part of the `themis_retrieval` source list.

The top-level `include/retrieval/retrieval.h` is currently a placeholder umbrella header. It includes and declares no retrieval API. This module is not a general ANN, hybrid-search, reranking, embedding, or RAG execution stack.

## Artifact store behavior and limits

`LoRAManifestStore::openFromThemisConfig()` loads `config/ai_ml/retrieval.yaml` through `ConfigPathResolver` and yaml-cpp, requires a non-empty RocksDB path and valid Ed25519 public-key pins, then opens the persistent store. Every production write/import verifies the content hash and detached signature over `manifest_hash`; unsigned or invalid records on reopen fail construction. The source-compatible no-argument constructor is non-production and rejects all artifact writes/imports and deployments.

The canonical YAML config supplies storage and key pins only. Deployment additionally requires an application-supplied quality controller, evidence validator, deployment authorizer, lifecycle transition authorizer, and persisted-state verifier through `RuntimeConfig`; the no-argument config factory does not configure these callbacks. Applications that need those operations must load `RuntimeConfig`, inject the owning-service integrations, and construct the store with that config. Without them, deployment and non-initial lifecycle transitions fail closed, and reopening persisted non-initial lifecycle states is rejected.

Artifact integrity checks fail closed unless the manifest hash matches, the
signature and `Ed25519` algorithm label are present, signer ID is pinned, and
the store's internally bound Ed25519 verifier accepts it. Public-key pins are
loaded from YAML and validated against their fingerprints; callers cannot
replace or disable the verifier.

Product deployment is allowed only via `deployProduct()`: it checks signed package/product artifacts and lifecycle readiness, invokes `themis::rag::quality::DeploymentGateController`, accepts only `kAllow`, then invokes the independent authorization callback. Missing configuration, absent/denied/throwing authorization, invalid quality metrics, or persistence errors deny deployment. This module does not invent an owner/RBAC policy; the owning deployment service must provide authorization. The current repository has no source-backed composition wiring for that callback, so direct module use cannot deploy by default.

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

The module-specific build option is `THEMIS_BUILD_RETRIEVAL`; runtime storage and key-pin settings are in `config/ai_ml/retrieval.yaml`. The checked-in configuration intentionally has an empty `pinned_keys` list and cannot initialize a production store until an administrator provisions approved keys. This target does not provide key provisioning/rotation, owner/RBAC policy, composition wiring for deployment callbacks, or live index integration. Do not infer end-to-end production readiness or measured performance from the artifact APIs, tests, roadmap history, or benchmark names.
