<!-- Status: draft | validated: 2026-10-05 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · FUTURE_TENSOR_ROPE.md · TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md · THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md · ../training/ARCHITECTURE.md · ../llama_cpp/ARCHITECTURE.md -->

# Tensor → Full Model Training / ML Bridge (Design & Architecture Paper)

**Author:** ThemisDB Contributors  
**Created:** 2026-10-05  
**Last Updated:** 2026-10-05  
**Status:** draft

## Abstract

This document defines the architecture that connects ThemisDB tensor artifacts to end-to-end ML/training/runtime behavior. It identifies:

- existing production-capable bridge segments,
- capability-gated or partial segments,
- exact interface and ownership boundaries,
- required invariants for deterministic and fail-closed operation,
- rollout preconditions for "full bridge ready" status.

Companion execution plan: [TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md](./TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md)

---

## 1. Problem Statement

The current bridge is functionally present but distributed across modules (`tensor`, `training`, `storage`, `llama_cpp`). Without a unifying architecture contract, deployments risk:

- profile drift between write/query paths,
- conditional runtime behavior hidden behind compile/runtime gates,
- non-comparable benchmark evidence due to inconsistent execution modes.

The core requirement is a **deterministic, auditable, and fail-closed** bridge from:

1. dense embeddings,
2. TT artifacts and hybrid retrieval,
3. AdaLoRA adapter conversion and serving handoff,
4. runtime integration in llama.cpp-backed RAG flows.

---

## 2. Scope and Non-Goals

## In Scope

- architecture of bridge interfaces and control flow across modules,
- data/metadata contracts and compatibility semantics,
- failure behavior and observability requirements,
- integration preconditions for RoPE-enabled tensor paths.

## Out of Scope

- redesign of core TT math algorithms,
- model-quality policy decisions outside defined benchmark gates,
- replacing existing module ownership boundaries with a monolith.

---

## 3. Architectural Goals

1. **Determinism**  
   Same input + profile/version + seed must produce equivalent bridge behavior.

2. **Fail-Closed correctness**  
   Incompatible profile/version/context combinations must be rejected explicitly.

3. **Runtime composability**  
   Bridge components remain decoupled but contract-compatible.

4. **Operational auditability**  
   Artifact provenance and runtime decisions must be reconstructible from logs/metadata.

5. **Evidence-driven rollout**  
   Promotion requires benchmark and CTest evidence packet, not anecdotal success.

---

## 4. System Context and High-Level Topology

```text
[Data/Embeddings]
       |
       v
[TensorIngestionBridge] --(TT artifact + metadata)--> [Tensor storage/index plane]
       |                                                |
       |                                                v
       |                                   [HnswTTBridge + TensorFingerprintGraph]
       |                                                |
       +---------------------------> [Training plane: AdaLoraTTBridge]
                                                    |
                                                    v
                                  [Deployment plane: IncrementalLoRATrainer + ILLMRouter]
                                                    |
                                                    v
                              [Runtime plane: GgmlTensorBridge + LlamaCppPlugin]
```

Primary module architecture references:
- [../training/ARCHITECTURE.md](../training/ARCHITECTURE.md)
- [../llama_cpp/ARCHITECTURE.md](../llama_cpp/ARCHITECTURE.md)

---

## 5. Component Architecture (Bridge Anchors)

## 5.1 Ingestion and TT artifact creation

Files:
- [tensor_ingestion_bridge.cpp](./tensor_ingestion_bridge.cpp)
- [../../include/tensor/tensor_ingestion_bridge.h](../../include/tensor/tensor_ingestion_bridge.h)

Primary methods:
- `TensorIngestionBridge::shouldDecompose(...)`
- `TensorIngestionBridge::decompose(...)`

Responsibility:
- transform dense embeddings into TT artifacts with reproducible metadata.

Bridge relevance:
- first hard boundary where profile/version metadata must be anchored.

## 5.2 Hybrid retrieval and candidate refinement

Files:
- [hnsw_tt_bridge.cpp](./hnsw_tt_bridge.cpp)
- [../../include/tensor/hnsw_tt_bridge.h](../../include/tensor/hnsw_tt_bridge.h)

Primary methods:
- `add(...)`, `addFlat(...)`
- `search(...)`, `searchFlat(...)`
- `extractSketch(...)`

Responsibility:
- candidate generation and TT-domain reranking.

Bridge relevance:
- read path must enforce profile parity with ingestion artifacts.

## 5.3 Training conversion and rank-control bridge

Files:
- [../training/adalora_tt_bridge.cpp](../training/adalora_tt_bridge.cpp)
- [../../include/training/adalora_tt_bridge.h](../../include/training/adalora_tt_bridge.h)

Primary methods:
- `exportToTT(...)`, `importFromTT(...)`
- `roundAndReallocate(...)`
- `findSimilarAdapters(...)`
- `mapAdapter(...)`

Responsibility:
- conversion between adapter representations and TT artifacts; rank optimization hooks.

Bridge relevance:
- central bridge between tensor persistence shape and training lifecycle behavior.

## 5.4 Deployment and traffic switching

Files:
- [../training/incremental_lora_trainer.cpp](../training/incremental_lora_trainer.cpp)
- [../../include/training/adapter_serving.h](../../include/training/adapter_serving.h)

Primary methods:
- `deployVersionEx(...)`
- `rollbackVersionEx(...)`
- `ILLMRouter::setAdapterWeight(...)`

Responsibility:
- move training artifacts into live routing safely.

Bridge relevance:
- control-plane bridge from trained adapter state to live inference traffic.

## 5.5 Runtime mapping into ggml and llama.cpp

Files:
- [../../include/storage/ggml_tensor_bridge.h](../../include/storage/ggml_tensor_bridge.h)
- [../storage/ggml_tensor_bridge.cpp](../storage/ggml_tensor_bridge.cpp)
- [../../include/llama_cpp/llama_cpp_plugin.h](../../include/llama_cpp/llama_cpp_plugin.h)
- [../llama_cpp/llama_cpp_plugin.cpp](../llama_cpp/llama_cpp_plugin.cpp)

Primary methods:
- `GgmlTensorBridge::map(...)`, `mapAdapter(...)`
- `LlamaCppPlugin::generateRAG(...)`, `embed(...)`

Responsibility:
- expose tensor/adapters to runtime graph and retrieval-assisted generation.

Bridge relevance:
- final runtime bridge where capability gates and fallback behavior matter most.

---

## 6. Data Contract and Metadata Model

Required metadata keys for bridge-safe artifacts:

- `rotation_profile`
- `rotation_version`
- `rotation_params_hash`
- `rotation_seed`

## Contract invariants

1. **Write-read parity invariant**  
   Artifact profile/version must match query profile/version.

2. **Deterministic transform invariant**  
   Equal input + equal metadata + equal seed -> equivalent transformed artifact.

3. **No implicit fallback invariant**  
   Profile mismatch cannot auto-downgrade to `none`; must hard-fail.

4. **Audit completeness invariant**  
   All profile-relevant bridge operations must emit diagnosable metadata/log evidence.

---

## 7. Control-Flow Sequences

## 7.1 Ingestion sequence (write path)

1. receive dense embedding and context.
2. validate profile requirements (e.g., relation context for relational profile).
3. apply transform profile (or none).
4. run κ-gate and decomposition.
5. persist TT artifact with profile metadata.

Failure mode: invalid/missing profile context -> reject write with explicit error.

## 7.2 Retrieval sequence (read path)

1. load index/profile compatibility metadata.
2. validate query profile/version against artifact/index profile/version.
3. apply matching query transform.
4. run hybrid retrieval (`HNSW -> TT rerank`).
5. return ranked candidates with diagnostics.

Failure mode: profile mismatch -> fail-closed query rejection.

## 7.3 Training bridge sequence

1. export adapter -> TT (`exportToTT`).
2. optional rank-rounding (`roundAndReallocate`).
3. similarity retrieval (`findSimilarAdapters`) for adaptation flows.
4. import TT -> adapter (`importFromTT`) for continuation.

Failure mode: shape/rank inconsistency -> explicit bridge error and abort.

## 7.4 Deployment/runtime sequence

1. deploy/rollback through `IncrementalLoRATrainer`.
2. route update via `ILLMRouter::setAdapterWeight`.
3. map adapter/tensors to runtime via `GgmlTensorBridge`.
4. serve RAG request in `LlamaCppPlugin::generateRAG`.

Failure mode: runtime capability gate unmet -> blocked mapping and explicit status.

---

## 8. Failure Semantics and Safety Model

| Condition | Expected behavior | Layer |
|---|---|---|
| profile mismatch | reject operation, emit diagnostic | tensor write/query |
| relation context missing for relational profile | reject operation, emit diagnostic | ingestion/query |
| unsupported runtime mapping capability | reject/disable runtime bridge path | storage/llama runtime |
| embed backend unavailable | explicit degraded mode (non-production) marker | llama_cpp runtime |
| adapter deployment routing failure | rollback/abort with explicit status | training/deployment |

Guiding rule: **no success-shaped fallback** for profile/contract violations.

---

## 9. Security and Compliance Considerations

1. metadata integrity is part of correctness; tampered or missing profile metadata is invalid state.
2. bridge state transitions (deploy/rollback/profile changes) require audit-grade logging.
3. cross-tenant adapter/tensor handling must preserve tenant isolation semantics.
4. planned/private capability paths must remain clearly flagged until production-ready.

---

## 10. Observability Contract

Required observability fields for bridge-critical operations:

- profile and version identifiers,
- transform mode and seed hash,
- query/write rejection reason class,
- rank/compression summary for TT conversion steps,
- runtime mapping capability mode (enabled, gated, fallback).

Minimum evidence packet definition is maintained in
[PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md) and
[FUTURE_TENSOR_ROPE.md](./FUTURE_TENSOR_ROPE.md).

---

## 11. Capability Matrix (Current Assessment)

| Bridge segment | Status | Architectural assessment |
|---|---|---|
| Dense embedding -> TT artifact | Implemented | production-capable with metadata hardening pending |
| Hybrid retrieval + TT rerank | Implemented | production-capable; parity enforcement extension required |
| AdaLoRA <-> TT conversion | Implemented | production-capable; profile-aware invariants pending |
| Training -> live traffic switch | Implemented at interface level | requires concrete router validation in rollout profile |
| TT/adapters -> ggml runtime map | Partial/capability-gated | production behavior depends on build/runtime gates |
| llama.cpp tensor-assisted runtime path | Conditional | quality path depends on real embed backend availability |

---

## 12. Full-Bridge Ready Criteria (Architecture Gate)

A bridge is considered architecture-ready for production rollout only if all are true:

- [ ] parity contract and fail-closed behavior are implemented across write/read/training/runtime.
- [ ] deterministic profile behavior is verified under fixed-seed tests.
- [ ] runtime capability gating is explicit and validated for target production profile.
- [ ] benchmark + CTest evidence packet satisfies rollout gates.
- [ ] rollback path is documented and exercised.

---

## 13. Relationship to RoPE Program

This architecture paper is the structural counterpart to the RoPE track in:
- [FUTURE_TENSOR_ROPE.md](./FUTURE_TENSOR_ROPE.md)

Delivery sequencing and ownership are formalized in:
- [TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md](./TENSOR_ML_TRAINING_BRIDGE_SPRINT_PLAN.md)

---

## 14. Open Design Questions

1. Should profile/version compatibility be enforced at index namespace level or artifact-level granularity?
2. Which runtime profile(s) are considered mandatory for go-live (CPU-only, accelerated, both)?
3. What is the canonical degraded-mode behavior for runtime capability misses in production profile?
4. Is a dedicated bridge orchestrator object needed, or is contract-level composition sufficient?

---

## 15. Neurocognitive Comparison Reference

For a scientific module-by-module comparison between ThemisDB memory architecture and human short-/long-term memory systems, including an explicit gap register for currently missing brain-like functions, see:
- [THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md](./THEMIS_BRAIN_MEMORY_COMPARATIVE_PAPER.md)
