# AdaLoRA + LoRA+ Federated Training with Knowledge Graph Enrichment in ThemisDB

**Status**: Review-ready draft
**Version**: 1.0
**Last Updated**: 2026-05-13
**Authors**: ThemisDB Research Team

> Scope note: This document is a repository-grounded technical review. Claims are limited to behavior visible in the current ThemisDB source tree and benchmark docs.

---

## Abstract

This document reviews the current ThemisDB implementation for parameter-efficient training based on LoRA-family methods, federated coordination, and graph-aware enrichment. The verified codebase contains: (1) an `AdaLoRAAdapter` with importance-based rank reallocation; (2) LoRA+ learning-rate asymmetry via `lora_plus_lambda`; (3) `LoRAAdapterMerger` with linear and TIES merge paths; (4) federated components for gradient exchange and distillation with differential-privacy parameters; and (5) a `KnowledgeGraphEnricher` API with AQL templates, cache support, and optional vector-index integration. Evaluation in this review is evidence-based (API inspection, implementation inspection, test presence, and benchmark mapping), not a new empirical model-quality study. The main limitation is that parts of the enrichment pipeline are intentionally stubbed/offline unless external dependencies (AQL execution and vector index wiring) are provided.

## Introduction

### Problem context

ThemisDB positions itself as a multi-model database with integrated AI/LLM functionality. For adapter-based training workflows, the relevant question is not only algorithmic quality but also whether lifecycle operations (training, merging, serving, checkpointing, and federated synchronization) are represented as first-class C++ APIs inside the system.

### Review objective

The objective of this article is to verify and consolidate the repository state for the topic **AdaLoRA + federated training + knowledge graph enrichment** and to replace unsupported statements with source-backed findings.

### Terminology used in this document

- **AQL**: Advanced Query Language in ThemisDB query layer.
- **Multi-model**: ThemisDB architecture across relational, graph, vector, document, and other models.
- **Federated training (in this scope)**: coordinator APIs for gradient aggregation and distillation exchange; not a claim of full end-to-end production deployment in every mode.

---

## Methodology / Approach

### M1. Source-of-truth artifacts

Primary artifacts inspected:

- Training APIs and implementations in `include/training/*` and `src/training/*`
- Federated APIs in `include/distributed_knowledge/*`
- Training roadmap and performance targets:
  - `src/training/ROADMAP.md`
  - `src/training/PERFORMANCE_EXPECTATIONS.md`
- Tests in `tests/` for directly related components

### M2. Verification criteria

A claim is included only if at least one of the following holds:

1. Directly visible public API contract in headers.
2. Observable implementation behavior in `.cpp`.
3. Test artifact exists and is discoverable in `tests/`.
4. Performance target is explicitly documented in `PERFORMANCE_EXPECTATIONS.md`.

### M3. Verified component map

| Component | Verified finding | Evidence |
|---|---|---|
| AdaLoRA | `AdaLoRAAdapter` exposes layer registration, importance updates, and rank reallocation; importance based on norm products in API docs. | `include/training/ada_lora_adapter.h` |
| LoRA+ | `IncrementalTrainingConfig::lora_plus_lambda` exists; implementation applies `lr_B = lr * lambda` when `lambda > 1.0`. | `include/training/incremental_lora_trainer.h`, `src/training/incremental_lora_trainer.cpp` |
| Adapter merge | Linear and TIES merge APIs are present (`mergeLinear*`, `mergeTIES*`). | `include/training/lora_adapter_merger.h` |
| Federated gradient path | `LoRAFederationCoordinator` API and `exportGradient()/applyGlobalDelta()` integration points exist. | `include/distributed_knowledge/lora_federation_coordinator.h`, `include/training/incremental_lora_trainer.h`, `src/training/incremental_lora_trainer.cpp` |
| Federated distillation | `FederatedDistillationCoordinator` includes DP config, policy gate, and rollback trigger hooks. | `include/distributed_knowledge/federated_distillation_coordinator.h` |
| Confidence calibration | `ConfidenceCalibrator` implements isotonic regression (PAV) and threshold selection. | `include/training/training_pipeline.h`, `src/training/training_pipeline.cpp` |
| Knowledge graph enrichment | Public API exists, including AQL query templates and cache controls; vector search requires `setVectorIndex()`. | `include/training/knowledge_graph_enricher.h`, `src/training/knowledge_graph_enricher.cpp` |

---

## Evaluation / Experiments

### E1. Code evidence and test footprint

The following related test files are present in the repository:

- `tests/test_ada_lora_adapter.cpp` (39 `TEST*` cases)
- `tests/test_lora_adapter_merger.cpp` (27 `TEST*` cases)
- `tests/test_training_lora_adapter.cpp` (39 `TEST*` cases)
- `tests/test_federated_distillation_coordinator.cpp` (19 `TEST*` cases)
- `tests/test_kge_vector_search.cpp` (18 `TEST*` cases)

Interpretation: there is component-level test coverage for the APIs discussed in this review. This article does **not** claim a full-system benchmark campaign for combined federated + enrichment workflows.

### E2. Benchmark and release-gate alignment

Documented training-related targets are currently mapped in `src/training/PERFORMANCE_EXPECTATIONS.md`, including:

- L-1 mapped to `RealLLMBench_RealModel_TextGeneration_50Tokens`
- L-3 mapped to `BM_Storage_LoadMetadata`
- L-5 mapped to `BM_Orchestrator_HealthCheck`
- Additional module gates TRNG-1..TRNG-4 with numeric thresholds/proxy rules

These are documented performance expectations and benchmark mappings; they are not reproduced as fresh measurements in this paper.

### E3. Consistency checks against unsupported claims

The prior draft included broad statements (for example, "first implementation" and universally complete production wiring). In the current repository state, such global superiority claims are not directly provable from source inspection alone and are therefore removed.

---

## Limitations / Known Issues

1. **KnowledgeGraphEnricher is partially offline/stub by design unless dependencies are wired**:
   `findRelatedProvisions`, `findRelatedCaseLaw`, and `findRelatedGuidance` currently return empty vectors in the shown implementation path. `findSimilarDocuments` requires an injected `VectorIndexManager`; without it, production builds throw and test mode can return empty results.

2. **AQL execution path in enrichment is template-ready but integration-dependent**:
   Query templates are present, but end-to-end enrichment depends on runtime database/query wiring outside this file-level review.

3. **Evaluation scope is artifact-based, not model-quality benchmarking**:
   This review validates implementation presence and documented performance gates; it does not provide new task-accuracy, robustness, or latency measurements from an executed benchmark suite.

4. **Roadmap still contains open items**:
   Training roadmap entries remain marked as blocked/open (e.g., active learning loop, deduplication, RLHF, some dashboard/export items), so the feature surface is not feature-complete.

---

## Conclusion

The current ThemisDB repository provides a substantial C++ implementation surface for LoRA-family training operations, federated coordination, and confidence calibration, with explicit APIs and dedicated tests. The strongest evidence-backed claims are about **available interfaces and implementation hooks**, not universal end-to-end production guarantees across all deployment modes. The most important caveat is the enrichment subsystem: parts of its behavior are intentionally dependency-gated (AQL runtime wiring, vector index injection), and this must be treated explicitly in any external publication.

---

## References

### Scientific literature

1. Hu, E. J., et al. *LoRA: Low-Rank Adaptation of Large Language Models*. ICLR 2022 (arXiv:2106.09685, 2021).
   URL: https://arxiv.org/abs/2106.09685
2. Zhang, Q., et al. *AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning*. arXiv:2303.10512 (2023).
   URL: https://arxiv.org/abs/2303.10512
3. Hayou, S., et al. *LoRA+: Efficient Low Rank Adaptation of Large Models*. arXiv:2402.12354 (2024).
   URL: https://arxiv.org/abs/2402.12354
4. Yadav, P., et al. *TIES-Merging: Resolving Interference When Merging Models*. arXiv:2306.01708 (2023).
   URL: https://arxiv.org/abs/2306.01708
5. McMahan, H. B., et al. *Communication-Efficient Learning of Deep Networks from Decentralized Data*. AISTATS 2017.
   URL: https://proceedings.mlr.press/v54/mcmahan17a.html
6. Dwork, C., Roth, A. *The Algorithmic Foundations of Differential Privacy*. FnT TCS, 2014.
   URL: https://www.cis.upenn.edu/~aaroth/Papers/privacybook.pdf
7. Mironov, I. *Rényi Differential Privacy*. IEEE CSF 2017. DOI: 10.1109/CSF.2017.11
   URL: https://doi.org/10.1109/CSF.2017.11
8. Lewis, P., et al. *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks*. NeurIPS 2020 (arXiv:2005.11401).
   URL: https://arxiv.org/abs/2005.11401

### ThemisDB repository artifacts

9. ThemisDB training roadmap and status:
   `src/training/ROADMAP.md`
10. ThemisDB training performance expectations:
    `src/training/PERFORMANCE_EXPECTATIONS.md`
11. ThemisDB architecture overview (AQL / multi-model context):
    `ARCHITECTURE.md`
