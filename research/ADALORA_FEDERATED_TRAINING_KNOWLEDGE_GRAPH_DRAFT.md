# AdaLoRA + LoRA+ Federated Training with Knowledge Graph Enrichment and Multi-GPU Domain Adaptation

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-05-04  
**Target Venue**: MLSys 2026 / NeurIPS 2027  
**Authors**: ThemisDB Research Team

> **Source Validation Note**: Every technical claim in this paper is backed by a concrete source code reference. All citations of the form `[SRC: path/file.h]` denote a verified file in the ThemisDB repository. Performance targets are from `src/training/PERFORMANCE_EXPECTATIONS.md` unless stated otherwise.

---

## I. Abstract

Fine-tuning large language models for database-native tasks requires a training infrastructure that manages adapter lifecycle, federated coordination, and knowledge enrichment without leaving the database engine. We present ThemisDB's **database-native LoRA training pipeline** — the first implementation integrating: (1) **AdaLoRA** (Zhang et al., 2023) adaptive rank allocation via B/A Frobenius-norm importance scoring; (2) **LoRA+** asymmetric learning rates (Hayou et al., 2024) with configurable λ-multiplier for the B matrix; (3) **TIES-Merging** (Yadav et al., 2023) multi-adapter composition with Trim–Resolve–Merge steps; (4) a **KnowledgeGraphEnricher** connecting adapter training context to HNSW-indexed vector search and AQL graph traversal; (5) a **FederatedDistillationCoordinator** enabling privacy-preserving teacher-student soft-label transfer with Gaussian DP and policy-gate governance; and (6) a **PAV isotonic regression Confidence Calibrator**. All components are implemented in C++ with CUDA/HIP multi-GPU support and Pimpl ABI stability. The training pipeline is database-native: adapter checkpoints are stored alongside document collections, AQL queries feed training samples, and LoRA adapters are hot-loaded into the LLM inference layer at deploy time.

---

## II. Problem Statement

### A. The Database-Native Training Gap

Existing LoRA fine-tuning frameworks (HuggingFace PEFT, LLM-Adapters) operate as external Python pipelines that require: (a) data export to filesystem, (b) external GPU cluster management, (c) manual adapter checkpoint storage, and (d) separate serving infrastructure. This creates a fundamental impedance mismatch with database applications where training data lives in document collections, knowledge graph context is stored in AQL-queryable graphs, and inference happens within the database engine.

### B. Adaptive Rank Allocation

Fixed-rank LoRA applies a uniform parameter budget across all weight matrices, despite the empirical observation that some layers contribute more to task-specific adaptation than others. AdaLoRA addresses this by dynamically reallocating rank budget from low-importance to high-importance layers during training — reducing the effective parameter count without accuracy degradation.

### C. Federated Privacy Constraints

Cross-institution model training (e.g., multi-hospital medical LoRA fine-tuning) requires: (a) no raw data leaving the institution, (b) gradient privacy via Differential Privacy, (c) Byzantine-resilient aggregation, and (d) model governance (rollback triggers, policy gates).

---

## III. System Architecture

### A. AdaLoRA: Adaptive Rank Allocation

**Source**: `include/training/ada_lora_adapter.h` (v0.0.12, Production-Ready, Quality Score: 100/100)

AdaLoRA approximates per-layer importance as the squared Frobenius norm of ΔW = (B @ A) × scaling:

```
importance(layer_i) = ||ΔW_i||²_F = ||B_i @ A_i × scaling_i||²_F
```

This approximation is the Frobenius-norm scoring approach described in the AdaLoRA paper (Zhang et al., arXiv:2303.10512), cited explicitly in the header:

> "AdaLoRA (Zhang et al., 2023, arXiv:2303.10512) adaptively reallocates the parameter budget among weight matrices during fine-tuning based on their estimated importance."

**API** (from `include/training/ada_lora_adapter.h`):
```cpp
class AdaLoRAAdapter {
public:
    // Recompute importance scores from current B/A weights
    void updateImportance();
    // Redistribute global rank budget proportionally to importance
    void reallocateRanks(size_t target_budget);
    // Forward pass applies only active (non-pruned) rank components
    std::vector<float> forward(const std::vector<float>& input) const;
    // Report current rank allocation per layer
    std::unordered_map<std::string, size_t> getRankAllocation() const;
};
```

Thread-safety note (from header): "NOT guaranteed. Callers must provide external synchronisation."

**Test coverage**: 36 tests in `tests/test_ada_lora_adapter.cpp`, CMake target `AdaLoRAFocusedTests` [SRC: `src/training/ROADMAP.md`].

### B. LoRA+ Asymmetric Learning Rates

**Source**: `src/training/ROADMAP.md` (completed item)

LoRA+ (Hayou et al., 2024) uses different learning rates for the A matrix (standard LR) and the B matrix (λ × LR) to exploit the asymmetric role of A (feature extraction) and B (task adaptation):

```
lr_A = config.learning_rate
lr_B = config.learning_rate × config.lora_plus_lambda
```

Implementation detail (from ROADMAP):
> "`IncrementalTrainingConfig::lora_plus_lambda`; when > 1.0, B uses `lr*λ` and A uses `lr` (Hayou et al., 2024); dual `AdamOptimizer` instances in `IncrementalLoRATrainer::Impl`"

**Source**: `include/training/incremental_lora_trainer.h` — `IncrementalTrainingConfig` struct.

### C. LoRAAdapterMerger: Linear + TIES

**Source**: `include/training/lora_adapter_merger.h` (v0.0.12, Production-Ready, Quality Score: 100/100)

Two merge strategies (documented in header):

**Linear Merge**:
> "Computes ΔW_merged = Σ_i( w_i × (B_i @ A_i) × scaling_i ) and then factorises the result back into a single B and A via a rank-1 SVD approximation (outer product of the dominant left/right singular vectors scaled by the largest singular value)."

**TIES-Merging** (Yadav et al., arXiv:2306.01708 — cited in header):
> "Three-step algorithm:
> 1. **Trim**: Zero out weight deltas whose absolute value is below a per-adapter threshold (fraction of the max absolute value).
> 2. **Resolve**: For each element, take the sign that appears in the majority of adapters (element-wise majority vote).
> 3. **Merge**: Average only the adapters that agree with the resolved sign."

**API**:
```cpp
class LoRAAdapterMerger {
public:
    LoRAAdapter mergeLinear(const std::vector<LoRAAdapter>& adapters,
                             const std::vector<float>& weights) const;
    LoRAAdapter mergeTIES(const std::vector<LoRAAdapter>& adapters,
                           float trim_fraction = 0.2f) const;
    // Batch variants
    std::vector<LoRAAdapter> mergeLinearAll(...) const;
    std::vector<LoRAAdapter> mergeTIESAll(...) const;
};
```

**Test coverage**: 32 tests in `tests/test_lora_adapter_merger.cpp`, `LoRAMergerFocusedTests` CMake target [SRC: `src/training/ROADMAP.md`].

### D. KnowledgeGraphEnricher

**Source**: `include/training/knowledge_graph_enricher.h`, `src/training/knowledge_graph_enricher.cpp`

Three enrichment operations (from ROADMAP):
```cpp
// AQL graph traversal
std::vector<Document> findRelatedProvisions(const Document& input);
std::vector<Document> findRelatedCaseLaw(const Document& input);
// HNSW-indexed vector cosine-similarity search (wired to VectorIndexManager)
std::vector<Document> findSimilarDocuments(const Document& input);
```

HNSW vector search wiring (belegt durch ROADMAP v1.6.0):
> "`findSimilarDocuments()` wired to `VectorIndexManager` for real cosine-similarity search via `setVectorIndex()`"

**LRU Cache** (belegt durch ROADMAP Q1 2026):
> "`EnrichmentLRUCache` in `knowledge_graph_enricher.cpp`"

### E. FederatedDistillationCoordinator

**Source**: `include/distributed_knowledge/federated_distillation_coordinator.h`, `src/distributed_knowledge/federated_distillation_coordinator.cpp`

**Status**: Production-Ready [SRC: `src/distributed_knowledge/ROADMAP.md`]

```
Teacher node → Gaussian DP soft labels → PolicyGate → Student nodes
                                                ↓ (rollback trigger)
                                         DistillationModelCard governance
```

Key components (from ROADMAP):
- `FederatedDistillationCoordinator`: teacher-student soft-label transfer with Gaussian DP
- `PolicyGate`: approval/rejection of incoming soft labels
- Rollback trigger: automatic rollback on policy violation or accuracy degradation
- `DistillationModelCard`: governance snapshot per federation round
- Audit hook: per-round DP budget logging

**Test coverage**: FDF-01..15 tests in `tests/test_federated_distillation_coordinator.cpp` [SRC: `src/distributed_knowledge/ROADMAP.md`].

**Threat model**: `docs/en/security/FEDERATED_DISTILLATION_THREAT_MODEL.md` — T-1..T-6, SEC-FDF-01..07 requirements [SRC: `src/distributed_knowledge/ROADMAP.md`].

### F. LoRAFederationCoordinator

**Source**: `include/distributed_knowledge/lora_federation_coordinator.h`

Wired to `IncrementalLoRATrainer::exportGradient()` / `applyGlobalDelta()` [SRC: `src/training/ROADMAP.md`]:
> "Federated learning for privacy-preserving cross-institution training (Target: Q2 2026) — [x]: `LoRAFederationCoordinator` + `IncrementalLoRATrainer::exportGradient()/applyGlobalDelta()` in `distributed_knowledge` and `training` modules"

### G. Confidence Calibration via PAV

**Source**: `include/training/training_pipeline.h`, `src/training/training_pipeline.cpp`

`ConfidenceCalibrator` using isotonic regression (Pool Adjacent Violators algorithm) [SRC: `src/training/ROADMAP.md`]:
> "Confidence-Threshold Auto-Calibration via isotonic regression (Target: Q1 2026) — `ConfidenceCalibrator` in `training_pipeline.h/.cpp`"

### H. Multi-GPU Distributed Training

**Source**: `include/training/incremental_lora_trainer.h`

`IncrementalTrainingConfig` fields (from ROADMAP):
```cpp
struct IncrementalTrainingConfig {
    int num_gpus;                      // number of GPU devices
    std::vector<int> gpu_ids;          // explicit GPU device IDs
    int sync_steps;                    // all-reduce gradient sync frequency
    TrainingQuantizationType quantization;  // NONE | FP16 | INT8 | NF4 (QLoRA)
    float lora_plus_lambda;            // > 1.0 → LoRA+ mode
    std::string checkpoint_dir;        // LoRACheckpointManager integration
};
```

**QLoRA path** (INT8, NF4): base weights frozen/compressed, only LoRA adapters A and B trained in full-precision [SRC: `src/training/ROADMAP.md`].

---

## IV. Source Code Evidence

### A. Implementierungsstand — vollständige Checkbox-Übersicht

**Quelle**: `src/training/ROADMAP.md`

```
[x] AdaLoRA adaptive rank allocation — ada_lora_adapter.h/.cpp; 36 tests
[x] LoRAAdapterMerger linear + TIES — lora_adapter_merger.h/.cpp; 32 tests
[x] LoRA+ asymmetric learning rates — IncrementalTrainingConfig::lora_plus_lambda
[x] Multi-GPU distributed training — MultiGPULoRATrainer; num_gpus/gpu_ids/sync_steps
[x] Model quantization — TrainingQuantizationType (NONE/FP16/INT8/NF4), QLoRALayer
[x] Training metrics — EpochMetrics, TrainingMetrics, IncrementalLoRATrainer::getMetrics()
[x] LoRACheckpointManager integration — atomic writes, SHA-256 integrity, rolling-window rotation
[x] Hyperparameter search — HyperparamSearchConfig, runHyperparamSearch(), 9 tests
[x] Adapter serving integration — ILLMRouter, deployVersionEx()/rollbackVersionEx(), 29 tests
[x] Federated learning — LoRAFederationCoordinator, exportGradient()/applyGlobalDelta()
[x] Model distillation — FederatedDistillationCoordinator, FDF-01..15 tests
[x] KnowledgeGraphEnricher — AQL traversal + VectorIndexManager HNSW search + LRU cache
[x] ConfidenceCalibrator — PAV isotonic regression
[x] MultiModalityParser — ModalityDetector, TextClauseExtractor, TableExtractor, CitationExtractor, OCRExtractor
[x] DatabaseDomainAutoLabeler — DomainType LEGAL/MEDICAL/FINANCIAL; AQL document-ID fetch
```

### B. Performance-Targets

**Quelle**: `src/training/PERFORMANCE_EXPECTATIONS.md`

| Ziel-ID | Beschreibung | Benchmark-Case |
|---------|-------------|----------------|
| L-1 | Time-to-First-Token (512-Token, A10G), Ziel: "Siehe Zielbeschreibung" | `RealLLMBench_RealModel_TextGeneration_50Tokens` |
| L-3 | LoRA Adapter Hot-Load (7B, Rank 64), Ziel: "Siehe Zielbeschreibung" | `BM_Storage_LoadMetadata` |
| L-5 | Work-Stealing Dispatch P99, Ziel: "Siehe Zielbeschreibung" | `BM_Orchestrator_HealthCheck` |

**Benchmark-Dateien**: `benchmarks/bench_lora_framework.cpp`, `benchmarks/bench_llm_real_models.cpp` [SRC: `src/training/PERFORMANCE_EXPECTATIONS.md`]

TIES-Merging Latenz-Target (aus Problembeschreibung in ROADMAP):
> "TIES-Merging (8 Adapter, < 100 ms)" — aspirationales Ziel aus Problemstatement, kein belegter Benchmark-Target-Wert.

---

## V. Related Work

### A. Parameter-Efficient Fine-Tuning

Hu et al. (2022) introduced LoRA (arXiv:2106.09685). Zhang et al. (2023) proposed AdaLoRA (arXiv:2303.10512) — cited directly in `ada_lora_adapter.h`. Hayou et al. (2024) proposed LoRA+ — cited in `src/training/ROADMAP.md`. Yadav et al. (2023) proposed TIES-Merging (arXiv:2306.01708) — cited in `lora_adapter_merger.h`.

### B. Federated Learning

McMahan et al. (2017) introduced FedAvg. Blanchard et al. (2017) established Byzantine-robust gradient aggregation. Geyer et al. (2017) applied Differential Privacy to federated learning. ThemisDB's `FederatedDistillationCoordinator` builds on this with Gaussian DP + policy-gate governance, validated in a database-native C++ runtime.

### C. Knowledge-Augmented Training

Lewis et al. (2020) introduced Retrieval-Augmented Generation (RAG). ThemisDB's `KnowledgeGraphEnricher` extends RAG by connecting training sample enrichment directly to AQL graph traversal and HNSW vector indexes — eliminating the data export step required by external RAG pipelines.

---

## VI. Open Problems and Future Work

1. **Active Learning Loop**: Auto-select the most informative training samples using model uncertainty — `[?]` in ROADMAP [SRC: `src/training/ROADMAP.md`].
2. **Training Data Deduplication**: Near-duplicate filtering for training corpora — `[?]` in ROADMAP.
3. **RLHF Training Loop**: Reinforcement Learning from Human Feedback — `[?]` in ROADMAP.
4. **Multi-Modal Training**: Combined text + table + chart training samples — `[?]` in ROADMAP.
5. **Additional Jurisdictions**: Legal domains beyond German law — `[?]` in ROADMAP.

---

## VII. Conclusion

We presented ThemisDB's database-native LoRA training pipeline — the first C++ production system integrating AdaLoRA adaptive rank allocation (Zhang et al., 2023), LoRA+ asymmetric learning rates (Hayou et al., 2024), TIES-Merging multi-adapter composition (Yadav et al., 2023), AQL/HNSW knowledge graph enrichment, Gaussian DP federated distillation, and PAV confidence calibration within a single database engine. All components are production-ready (Quality Score: 100/100 for `ada_lora_adapter.h` and `lora_adapter_merger.h`) with 36 + 32 + 15 + 29 dedicated tests.

---

## References

[1] Hu E., Shen Y., Wallis P., et al. "LoRA: Low-Rank Adaptation of Large Language Models." *ICLR 2022* (arXiv:2106.09685).

[2] Zhang Q., Chen M., Bukharin A., et al. "AdaLoRA: Adaptive Budget Allocation for Parameter-Efficient Fine-Tuning." *ICLR 2023* (arXiv:2303.10512).

[3] Hayou S., Ghosh N., Yu B. "LoRA+: Efficient Low Rank Adaptation of Large Models." *arXiv:2402.12354, 2024*.

[4] Yadav P., Tam D., Choshen L., Raffel C., Bansal M. "TIES-Merging: Resolving Interference When Merging Models." *NeurIPS 2023* (arXiv:2306.01708).

[5] McMahan H.B., Moore E., Ramage D., Hampson S., Arcas B.A.y. "Communication-Efficient Learning of Deep Networks from Decentralized Data." *AISTATS 2017*.

[6] Blanchard P., Mhamdi E.M.E., Guerraoui R., Stainer J. "Machine Learning with Adversaries: Byzantine Tolerant Gradient Descent." *NeurIPS 2017*.

[7] Mironov I. "Rényi Differential Privacy of the Gaussian Mechanism." *CSF 2017*.

[8] Lewis P., Perez E., Piktus A., et al. "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks." *NeurIPS 2020*.

[9] Lester B., Al-Rfou R., Constant N. "The Power of Scale for Parameter-Efficient Prompt Tuning." *EMNLP 2021*.

[10] Dettmers T., Pagnoni A., Holtzman A., Zettlemoyer L. "QLoRA: Efficient Finetuning of Quantized LLMs." *NeurIPS 2023*.

---

## Appendix A: Key Source File Map

| Component | Header | Implementation | Tests |
|-----------|--------|---------------|-------|
| AdaLoRA | `include/training/ada_lora_adapter.h` | `src/training/ada_lora_adapter.cpp` | `tests/test_ada_lora_adapter.cpp` (36 tests) |
| LoRAMerger | `include/training/lora_adapter_merger.h` | `src/training/lora_adapter_merger.cpp` | `tests/test_lora_adapter_merger.cpp` (32 tests) |
| LoRAAdapter | `include/training/lora_adapter.h` | `src/training/lora_adapter.cpp` | `tests/test_training_lora_adapter.cpp` (39 tests) |
| KGEnricher | `include/training/knowledge_graph_enricher.h` | `src/training/knowledge_graph_enricher.cpp` | `tests/test_kge_vector_search.cpp` |
| FedDistil | `include/distributed_knowledge/federated_distillation_coordinator.h` | `src/distributed_knowledge/federated_distillation_coordinator.cpp` | `tests/test_federated_distillation_coordinator.cpp` (15 tests) |
| LoRAFedCoord | `include/distributed_knowledge/lora_federation_coordinator.h` | `src/distributed_knowledge/lora_federation_coordinator.cpp` | `tests/test_distributed_knowledge.cpp` |

---

*ThemisDB Training Module — Production-Ready, Apache 2.0*  
*Module: `include/training/`, `src/training/`, `include/distributed_knowledge/`*  
*Version: v1.6.0 | Quality Score: 100/100 (AdaLoRA, LoRAMerger, LoRAAdapter)*
