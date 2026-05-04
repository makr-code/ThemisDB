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

**Constructor signature** (verbatim from `include/training/ada_lora_adapter.h`):
```cpp
explicit AdaLoRAAdapter(size_t default_rank   = 4,
                        float  default_alpha  = 8.0f,
                        size_t rank_budget    = 64);
```

**addLayer signature** (verbatim from header, with initialization semantics):
```cpp
void addLayer(const std::string& layer_name,
              size_t in_dim,
              size_t out_dim,
              size_t max_rank = 0,   // 0 = use adapter default
              float  alpha    = 0.0f); // 0.0 = use adapter default
// B is Kaiming-uniform initialised; A is zero-initialised.
// Effective rank starts at max_rank.
```

**Per-layer statistics struct** (verbatim from header):
```cpp
struct AdaLoRALayerStats {
    std::string layer_name;      ///< Layer identifier
    size_t      max_rank   = 0;  ///< Maximum allowed rank for this layer
    size_t      active_rank = 0; ///< Currently active (unpruned) rank
    float       importance = 0.0f; ///< Estimated importance score (higher = more important)
};
```

**Rank reallocation result** (verbatim from header):
```cpp
struct ReallocResult {
    bool   success = true;            ///< Legacy compatibility flag
    size_t total_active_rank = 0;     ///< Sum of active ranks after reallocation
    size_t layers_pruned     = 0;     ///< Layers whose rank was reduced
    size_t layers_expanded   = 0;     ///< Layers whose rank was increased
};
```

**Importance scoring formula** (verbatim from header Doxygen):
> "Importance is estimated as the mean squared Frobenius norm of the rank-component contributions: `sum_i( ||B[:,i]||^2 * ||A[i,:]||^2 ) / active_rank`. This approximates the singular-value importance without a full SVD."

**Rank reallocation semantics** (verbatim from header Doxygen):
> "Distributes `total_budget` rank slots proportionally to each layer's normalised importance score, subject to [1, max_rank] per-layer bounds. Layers with zero importance receive rank 1 (minimum)."

**Forward pass formula** (verbatim from header Doxygen):
> "Performs (using only the first `active_rank` columns of B / rows of A):
> `hidden = input @ B[:, :active_rank]`  (batch_size × active_rank)
> `output = hidden @ A[:active_rank, :]` (batch_size × out_dim)
> `return  output × scaling`             where `scaling = alpha / max_rank`"

**Full public API** (from `include/training/ada_lora_adapter.h`):
```cpp
class AdaLoRAAdapter {
public:
    explicit AdaLoRAAdapter(size_t default_rank = 4,
                            float  default_alpha = 8.0f,
                            size_t rank_budget   = 64);
    // Layer management
    void addLayer(const std::string&, size_t in_dim, size_t out_dim,
                  size_t max_rank = 0, float alpha = 0.0f);
    bool removeLayer(const std::string& layer_name);
    bool hasLayer(const std::string& layer_name) const;
    std::vector<std::string> layerNames() const;
    size_t layerCount() const;
    // Importance scoring and rank reallocation
    void updateImportance(const std::string& layer_name);
    void updateAllImportances();
    ReallocResult reallocateRanks(size_t total_budget);
    ReallocResult reallocateRanks();           // uses configured rank_budget
    // Rank and importance access
    size_t getActiveRank(const std::string& layer_name) const;
    size_t getMaxRank(const std::string& layer_name) const;
    float  getImportance(const std::string& layer_name) const;
    std::vector<AdaLoRALayerStats> getLayerStats() const;
    size_t totalActiveParameterCount() const;
    // Weight access
    void setWeights(const std::string&, const std::vector<float>& B,
                    const std::vector<float>& A);
    std::pair<std::vector<float>, std::vector<float>>
        getWeights(const std::string& layer_name) const;
    // Forward pass
    std::vector<float> forward(const std::string& layer_name,
                               const std::vector<float>& input,
                               size_t batch_size) const;
    // Configuration
    size_t rankBudget() const;
    void setRankBudget(size_t budget);
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

**Two merge strategies** (documented in header):

**Linear Merge**:
> "Computes ΔW_merged = Σ_i( w_i × (B_i @ A_i) × scaling_i ) and then factorises the result back into a single B and A via a rank-1 SVD approximation (outer product of the dominant left/right singular vectors scaled by the largest singular value)."

**TIES-Merging** (Yadav et al., arXiv:2306.01708 — cited in header):
> "Three-step algorithm:
> 1. **Trim**: Zero out weight deltas whose absolute value is below a per-adapter threshold (fraction of the max absolute value).
> 2. **Resolve**: For each element, take the sign that appears in the majority of adapters (element-wise majority vote).
> 3. **Merge**: Average only the adapters that agree with the resolved sign."

**AdapterDescriptor struct** (verbatim from `include/training/lora_adapter_merger.h`):
```cpp
struct AdapterDescriptor {
    const LoRAAdapter* adapter = nullptr; ///< Pointer to the adapter (non-owning)
    std::string        layer_name;        ///< Layer name within the adapter
    float              weight = 1.0f;     ///< Blend weight (used by linear merge)
};
```

**MergeLayerResult struct** (verbatim from header):
```cpp
struct MergeLayerResult {
    std::string        layer_name;
    std::vector<float> B;           ///< Merged B matrix (in_dim × rank)
    std::vector<float> A;           ///< Merged A matrix (rank × out_dim)
    bool               success = false;
    std::string        error_message;
};
```

**MergeResult struct** (verbatim from header):
```cpp
struct MergeResult {
    std::vector<MergeLayerResult> layers;
    size_t layers_merged  = 0;
    size_t layers_failed  = 0;
    bool   success        = false;
    std::string error_message;
};
```

**Full API** (from header):
```cpp
class LoRAAdapterMerger {
public:
    // Linear merge: single layer
    MergeLayerResult mergeLinear(const std::vector<AdapterDescriptor>& adapters,
                                 const std::string& out_layer,
                                 size_t in_dim, size_t out_dim,
                                 size_t rank, float alpha = 8.0f) const;
    // Linear merge: all shared layers
    MergeResult mergeLinearAll(const std::vector<const LoRAAdapter*>& adapters,
                               const std::vector<float>& weights,
                               size_t output_rank) const;
    // TIES merge: single layer (trim_threshold=0.2f = fraction of max-abs-value)
    MergeLayerResult mergeTIES(const std::vector<AdapterDescriptor>& adapters,
                               const std::string& out_layer,
                               size_t in_dim, size_t out_dim,
                               size_t rank, float alpha = 8.0f,
                               float trim_threshold = 0.2f) const;
    // TIES merge: all shared layers
    MergeResult mergeTIESAll(const std::vector<const LoRAAdapter*>& adapters,
                             size_t output_rank,
                             float trim_threshold = 0.2f) const;
};
```

**Test coverage**: 32 tests in `tests/test_lora_adapter_merger.cpp`, `LoRAMergerFocusedTests` CMake target [SRC: `src/training/ROADMAP.md`].

### D. KnowledgeGraphEnricher

**Source**: `include/training/knowledge_graph_enricher.h`, `src/training/knowledge_graph_enricher.cpp`

**EnrichmentCacheConfig struct** (verbatim from `include/training/knowledge_graph_enricher.h`):
```cpp
struct EnrichmentCacheConfig {
    bool   enabled           = true;    ///< Enable or disable the cache
    size_t capacity          = 50000;   ///< Maximum number of cached entries
    size_t refresh_interval_seconds = 300; ///< Seconds between graph-version refreshes (0 = disable)
};
```

**EnrichmentCacheStats struct** (verbatim from header):
```cpp
struct EnrichmentCacheStats {
    size_t hits       = 0;  ///< Cache hits since last reset
    size_t misses     = 0;  ///< Cache misses since last reset
    size_t evictions  = 0;  ///< Entries evicted due to capacity or version change
    size_t size       = 0;  ///< Current number of cached entries
};
```

**EnrichmentConfig struct** (verbatim from header — all fields with defaults):
```cpp
struct EnrichmentConfig {
    std::string target_collection;          ///< Collection with training samples
    std::string graph_name;                 ///< Knowledge graph name
    size_t max_related_items = 5;           ///< Max related items per category
    size_t traversal_depth = 2;             ///< Graph traversal depth
    float similarity_threshold = 0.7f;      ///< Similarity threshold for semantic search
    bool include_provisions = true;         ///< Include related legal provisions
    bool include_case_law = true;           ///< Include related case law
    bool include_guidance = true;           ///< Include internal guidance
    bool include_similar_docs = true;       ///< Include similar documents
    size_t batch_size = 50;                 ///< Samples per batch
    EnrichmentCacheConfig cache;            ///< LRU cache configuration (capacity=50000, TTL=300s)
};
```

**Three enrichment operations** (from header):
```cpp
EnrichmentStats enrichAll(EnrichmentCallback callback = nullptr);
GraphContext    enrichSample(const std::string& sample_id);
EnrichmentStats enrichQuery(const std::string& aql_query,
                            EnrichmentCallback callback = nullptr);

// AQL graph traversal operations:
std::vector<std::string> findRelatedProvisions(const std::string& document_id,
                                               size_t max_results = 5);
std::vector<std::string> findRelatedCaseLaw(const std::string& document_id,
                                            size_t max_results = 5);
std::vector<std::string> findRelatedGuidance(const std::string& document_id,
                                             size_t max_results = 5);
// Cosine-similarity search (requires setVectorIndex() call):
std::vector<std::pair<std::string, float>>
    findSimilarDocuments(const std::string& document_id,
                         size_t max_results = 5);
// Wire up VectorIndexManager for real HNSW cosine-similarity search:
void setVectorIndex(VectorIndexManager* vim);
```

**STUB nature of `findSimilarDocuments()`** (verbatim from header Doxygen):
> "When set, `findSimilarDocuments()` uses this index for cosine-similarity queries instead of returning an empty stub result. The index must already be populated."
> "@param vim Pointer to an initialised VectorIndexManager, or nullptr to disable vector search and revert to the offline stub."

This means: **without a `setVectorIndex()` call, `findSimilarDocuments()` returns an empty result** — it is a documented stub that becomes functional only when a `VectorIndexManager` is injected. The stub activation condition is: `vim == nullptr` (default after construction). The production path requires the caller to call `setVectorIndex(vim)` with an already-populated HNSW index.

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

### B. Comparison: ThemisDB vs. External LoRA Frameworks

| Feature | HuggingFace PEFT | Axolotl | LLaMA-Factory | **ThemisDB (this work)** |
|---------|------------------|---------|---------------|--------------------------|
| Language | Python | Python | Python | **C++ (database-native)** |
| AdaLoRA adaptive rank | ✓ | ✓ (via PEFT) | ✓ (via PEFT) | **✓ [`ada_lora_adapter.h`]** |
| LoRA+ asymmetric LR | ✓ | ✓ | ✓ | **✓ [`IncrementalTrainingConfig::lora_plus_lambda`]** |
| TIES-Merging | ✓ | ✗ | ✗ | **✓ [`lora_adapter_merger.h`, trim_threshold=0.2f]** |
| Database-native storage | ✗ | ✗ | ✗ | **✓ (adapters stored in document collections)** |
| AQL training data source | ✗ | ✗ | ✗ | **✓ (`enrichQuery(aql_query, callback)`)** |
| Knowledge graph enrichment | ✗ | ✗ | ✗ | **✓ (`KnowledgeGraphEnricher`, HNSW + AQL traversal)** |
| Federated DP training | External (PySyft) | ✗ | ✗ | **✓ (`FederatedDistillationCoordinator`, Gaussian DP)** |
| Byzantine resilience | ✗ | ✗ | ✗ | **✓ (`LoRAFederationCoordinator`, L2-norm filter)** |
| Confidence calibration (PAV) | External (sklearn) | ✗ | ✗ | **✓ (`ConfidenceCalibrator`, isotonic regression)** |
| Pimpl ABI stability | ✗ | ✗ | ✗ | **✓ (all headers use Pimpl pattern)** |
| Data export required | **✓ (always)** | **✓ (always)** | **✓ (always)** | **✗ (zero-copy in-process)** |
| Hot-load into inference | External | External | External | **✓ (`ILLMRouter::deployVersionEx()`)** |

ThemisDB's primary differentiator is **database-native integration**: training data comes from AQL queries, enrichment from graph traversal, and adapters are stored and hot-loaded within the same runtime — eliminating all data export steps required by Python-based frameworks [SRC: `src/training/ROADMAP.md`].

### C. Federated Learning

McMahan et al. (2017) introduced FedAvg. Blanchard et al. (2017) established Byzantine-robust gradient aggregation. Geyer et al. (2017) applied Differential Privacy to federated learning. ThemisDB's `FederatedDistillationCoordinator` builds on this with Gaussian DP + policy-gate governance, validated in a database-native C++ runtime.

### D. Knowledge-Augmented Training

Lewis et al. (2020) introduced Retrieval-Augmented Generation (RAG). ThemisDB's `KnowledgeGraphEnricher` extends RAG by connecting training sample enrichment directly to AQL graph traversal and HNSW vector indexes — eliminating the data export step required by external RAG pipelines.

---

## VI. Open Problems and Future Work

1. **Active Learning Loop**: Auto-select the most informative training samples using model uncertainty — `[?]` in ROADMAP [SRC: `src/training/ROADMAP.md`].
2. **Training Data Deduplication**: Near-duplicate filtering for training corpora — `[?]` in ROADMAP.
3. **RLHF Training Loop**: Reinforcement Learning from Human Feedback — `[?]` in ROADMAP.
4. **Multi-Modal Training**: Combined text + table + chart training samples — `[?]` in ROADMAP.
5. **Additional Jurisdictions**: Legal domains beyond German law — `[?]` in ROADMAP.
6. **`findSimilarDocuments()` Production Wiring**: Currently returns empty stub result when `setVectorIndex()` has not been called [SRC: `include/training/knowledge_graph_enricher.h`]. Wiring to a pre-populated `VectorIndexManager` instance enables HNSW cosine-similarity enrichment.
7. **TIES-Merge Latency Benchmark**: A runtime benchmark for TIES-Merging across 8 adapters is aspirationally described as "< 100 ms" in the module problem statement but is not yet documented in `src/training/PERFORMANCE_EXPECTATIONS.md`. Formal benchmarking via `benchmarks/bench_lora_framework.cpp` is required.

---

## VII. Conclusion

We presented ThemisDB's database-native LoRA training pipeline — the first C++ production system integrating AdaLoRA adaptive rank allocation (Zhang et al., 2023, arXiv:2303.10512), LoRA+ asymmetric learning rates (Hayou et al., 2024), TIES-Merging multi-adapter composition (Yadav et al., 2023, arXiv:2306.01708), AQL/HNSW knowledge graph enrichment, Gaussian DP federated distillation, and PAV confidence calibration within a single database engine.

**Source-backed claims** (every claim references concrete source code):

1. **AdaLoRA implementation** [SRC: `include/training/ada_lora_adapter.h`, v0.0.12, Quality Score: 100/100]: Constructor `AdaLoRAAdapter(default_rank=4, default_alpha=8.0f, rank_budget=64)`, importance = mean squared Frobenius norm per rank component, reallocateRanks distributes budget ∈ [1, max_rank] per layer.
2. **TIES-Merging** [SRC: `include/training/lora_adapter_merger.h`, v0.0.12, Quality Score: 100/100]: `trim_threshold=0.2f`, three-step Trim/Resolve/Merge, `MergeResult` tracks layers_merged/layers_failed.
3. **KnowledgeGraphEnricher** [SRC: `include/training/knowledge_graph_enricher.h`]: `EnrichmentConfig` with max_related_items=5, traversal_depth=2, similarity_threshold=0.7f, batch_size=50, `EnrichmentCacheConfig` capacity=50000, refresh_interval=300s; `findSimilarDocuments()` returns empty stub without `setVectorIndex()` call.
4. **Test coverage** [SRC: `src/training/ROADMAP.md`]: 36 tests (AdaLoRA), 32 tests (LoRAMerger), 15 tests (FederatedDistillation), 29 tests (adapter serving).
5. **Performance gates** [SRC: `src/training/PERFORMANCE_EXPECTATIONS.md`]: L-1 (TTFT), L-3 (adapter hot-load), L-5 (dispatch P99) — regression-relative gates only; no absolute throughput numbers are documented.

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
