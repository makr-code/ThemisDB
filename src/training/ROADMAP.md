# Training Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Validated: 2026-03-24 -->

## Current Status
v1.6.0 – AdaLoRA (adaptive rank pruning), LoRAAdapterMerger (TIES + linear), and LoRA+ (asymmetric LR) added. Full LoRA fine-tuning toolchain including checkpoint/resume, adapter versioning, quantization (QLoRA), and multi-GPU training.

## Completed ✅
- [x] LegalAutoLabeler – automated training sample generation from legal documents via NLP modality extraction
- [x] `labelAll()`, `labelDocument()`, `labelQuery()` APIs
- [x] AQL query executor wired to `labelAll()` and `labelQuery()` for DB document-ID fetch (v1.7.0) — `auto_labeler.cpp`
- [x] Low-confidence sample flagging and human-review queue
- [x] `updateSampleConfidence()` for recording human review decisions
- [x] German (`de`) and multi-language support
- [x] IncrementalLoRATrainer – full LoRA lifecycle (train, evaluate, deploy, rollback)
- [x] INITIAL and INCREMENTAL training modes
- [x] Checkpoint save and resume (`resumeFromCheckpoint`)
- [x] Adapter version management (`deployVersion`, `rollbackVersion`, `listVersions`)
- [x] Configurable LoRA rank, alpha, learning rate
- [x] Training progress callback (epoch, step, loss)
- [x] KnowledgeGraphEnricher – AQL graph traversal context enrichment
- [x] `findRelatedProvisions()`, `findRelatedCaseLaw()`, `findSimilarDocuments()`
- [x] `findSimilarDocuments()` wired to `VectorIndexManager` for real cosine-similarity search via `setVectorIndex()` (v1.6.0) — `knowledge_graph_enricher.h/.cpp`, `tests/test_kge_vector_search.cpp`
- [x] Custom AQL query registration for domain-specific traversals
- [x] Pimpl pattern for ABI stability across all three components
- [x] LoRA Checkpoint Manager with SHA-256 integrity validation (Target: Q1 2026) — `lora_checkpoint_manager.h/.cpp` (b2342851)
- [x] Training Sample Provenance and Lineage Tracking (Target: Q1 2026) — `provenance_tracker.h/.cpp` (b2342851)
- [x] Knowledge Graph Enrichment LRU Cache (Target: Q1 2026) — `EnrichmentLRUCache` in `knowledge_graph_enricher.cpp` (b2342851)
- [x] ContentModality enum for multi-modality sample tracking (Target: Q1 2026) — `auto_labeler.h` (b2342851)
- [x] Confidence-Threshold Auto-Calibration via isotonic regression (Target: Q1 2026) — `ConfidenceCalibrator` in `training_pipeline.h/.cpp` (b2342851)
- [x] Multi-modality parser (`ModalityDetector`, `TextClauseExtractor`, `TableExtractor`, `CitationExtractor`, `OCRExtractor`) (Target: Q1 2026) — `modality_parser.h/.cpp` (b2342851)
- [x] Real LoRA weight manipulation in `IncrementalLoRATrainer` (Target: Q1 2026) — replaced `computeSimulatedLoss()` with `LoRALayer` + `AdamOptimizer` forward/backward/step; CUDA/HIP via `GPULoRALayer`; binary checkpoint serialization for B and A matrices (`incremental_lora_trainer.cpp`)
- [x] Dedicated LoRA adapter weight manipulation layer `LoRAAdapter` (Target: v1.7.0) — `lora_adapter.h/.cpp`; real forward pass (input @ B @ A × scaling), additive single/batch weight updates, Kaiming-B/zero-A init, export/import for checkpoint integration; 39 unit tests (`test_training_lora_adapter.cpp`)
- [x] Multi-GPU distributed training coordination (Target: Q2 2026) — `IncrementalTrainingConfig.num_gpus/gpu_ids/sync_steps` fields; `MultiGPULoRATrainer` wired in `incremental_lora_trainer.cpp`; data-parallel sharding, all-reduce gradient sync; device-count mismatch → `std::runtime_error`; fallback to single-GPU on init failure; `TrainingResult.gpus_used` field
- [x] Model quantization configuration (Target: Q2 2026) — `TrainingQuantizationType` enum (NONE/FP16/INT8/NF4), `QuantizationConfig` struct, `IncrementalTrainingConfig.quantization` field; validated in `validateHyperparameters()`; INT8 and NF4 activate `QLoRALayer` in the CPU training path (base weights frozen/compressed, only LoRA adapters A and B trained in full-precision); NONE/FP16 use standard `LoRALayer`
- [x] Training metrics tracking (Target: Q2 2026) — `EpochMetrics` (per-epoch loss/accuracy/lr/elapsed), `TrainingMetrics` (step_losses, epoch_metrics, best_train_loss, best_val_loss, total_elapsed_seconds); `IncrementalLoRATrainer::getMetrics()` public API; metrics reset at start of each `train()` call
- [x] `LoRACheckpointManager` integration in `IncrementalLoRATrainer` (Target: Q2 2026) — `IncrementalTrainingConfig.checkpoint_dir` field; when set, each `saveCheckpoint()` call delegates to `LoRACheckpointManager::save()` for atomic writes, SHA-256 integrity, and rolling-window rotation (3 checkpoints default)
- [x] **AdaLoRA** adaptive rank allocation (Target: Q2 2026) — `ada_lora_adapter.h/.cpp`; importance scoring via B/A norm products; `reallocateRanks()` proportional budget distribution; active-rank forward pass; 36 tests (`test_ada_lora_adapter.cpp`); `AdaLoRAFocusedTests` CMake target
- [x] **LoRAAdapterMerger** linear + TIES merging (Target: Q2 2026) — `lora_adapter_merger.h/.cpp`; `mergeLinear()` weighted ΔW sum + SVD factorisation; `mergeTIES()` Trim–Resolve–Merge (Yadav et al.); `*All()` batch overloads; 32 tests (`test_lora_adapter_merger.cpp`); `LoRAMergerFocusedTests` CMake target
- [x] **LoRA+** asymmetric learning rates (Target: Q2 2026) — `IncrementalTrainingConfig::lora_plus_lambda`; when > 1.0, B uses `lr*λ` and A uses `lr` (Hayou et al., 2024); dual `AdamOptimizer` instances in `IncrementalLoRATrainer::Impl`

## In Progress 🚧
- [x] Automated hyperparameter search (LoRA rank, learning rate sweep) (Target: Q2 2026) — `HyperparamSearchConfig`, `HyperparamResult`, `HyperparamTrialResult`, `HyperparamSearchCallback` in `training_pipeline.h`; `runHyperparamSearch()` in `training_pipeline.cpp`; 9 tests in `tests/test_training_pipeline_e2e.cpp`
  - Subsystems: `src/training/training_pipeline.cpp` (new `HyperparamSearch` inner class), `ConfidenceCalibrator`
  - Inputs: `HyperparamSearchConfig{rank_candidates, lr_candidates, max_trials, budget_seconds}`; validation split fraction
  - Outputs: `HyperparamResult{best_rank, best_lr, best_val_loss, trial_log}`; best config auto-applied to pipeline
  - Constraints: deterministic trial ordering (seeded random); concurrent trials capped at `num_gpus`
  - Errors: no improvement after `max_trials` → return best seen; budget exceeded → early-stop, return best so far
  - Tests: unit — mock trainer, verify trial scheduling; integration — sweep over 3 rank values on synthetic dataset
  - Perf: trial overhead (excluding training) ≤ 50 ms/trial; total sweep for 9-trial 3×3 grid ≤ 3× single-train time

- [~] Adapter serving integration with LLM inference layer (Target: Q3 2026)
  - Subsystems: `src/training/incremental_lora_trainer.cpp` (`deployVersion`), `include/llm/multi_lora_manager.h`
  - Inputs: `adapter_version` (string), `traffic_split` (float [0,1]); LLM router must expose `setAdapterWeight(version, weight)`
  - Outputs: LLM module routes `traffic_split` fraction of requests to new adapter; `DeployResult{active_version, split_applied}`
  - Constraints: atomic routing update (no mid-request split change); rollback must complete within 1 request cycle
  - Errors: adapter not found → `DeployResult.error = "version_not_found"`; LLM router unavailable → propagate `std::runtime_error`
  - Tests: integration — deploy v1.1 at 10% split, verify 10% of mock requests routed to v1.1; rollback test
  - Perf: routing weight update latency ≤ 1 ms; zero dropped requests during split change

## Planned Features 📋

### Short-term (Next 3-6 months)
- [?] Support for additional legal jurisdictions beyond German law
- [?] Active learning loop (auto-select most informative samples for labeling)
- [?] Training data deduplication and near-duplicate filtering
- [?] Evaluation metrics dashboard (validation loss curves, accuracy)
- [?] Export labeled datasets in standard formats (JSONL, Hugging Face datasets)

### Long-term (6-12 months)
- [?] Reinforcement learning from human feedback (RLHF) training loop
- [?] Multi-modal training samples (text + table + chart)
- [?] Domain adaptation beyond legal (medical, financial)
- [?] Federated learning for privacy-preserving cross-institution training
- [?] Model distillation from large to small adapters

## Implementation Phases

### Phase 1: Auto-Labeling & LoRA Training Pipeline (Status: Completed ✅)
- [x] LegalAutoLabeler – NLP modality extraction from legal documents
- [x] `labelAll()`, `labelDocument()`, `labelQuery()` public APIs
- [x] `labelAll()` and `labelQuery()` fetch document IDs from the DB via AQL query executor (`executeAql()`); offline/nullptr-engine fallback for tests
- [x] Low-confidence sample flagging and human-review queue with `updateSampleConfidence()`
- [x] IncrementalLoRATrainer – full LoRA lifecycle (train, evaluate, deploy, rollback)
- [x] INITIAL and INCREMENTAL training modes with configurable rank/alpha/lr
- [x] Checkpoint save and resume (`resumeFromCheckpoint()`)
- [x] Adapter version management (`deployVersion`, `rollbackVersion`, `listVersions`)
- [x] KnowledgeGraphEnricher – AQL graph traversal context enrichment (`findRelatedProvisions`, `findRelatedCaseLaw`)
- [x] Confidence-threshold filtering for automatic sample acceptance
- [x] Pimpl pattern for ABI stability across all three components

### Phase 2: Adapter Management & Multi-Domain (Status: In Progress 🚧)
- [?] Adapter version management: atomic deploy/rollback with integrity verification (Target: Q2 2026)
- [?] Multi-domain support beyond German legal text (medical, financial) (Target: Q2 2026)
- [x] Automated hyperparameter search (LoRA rank and learning rate sweep) (Target: Q2 2026)
- [?] Adapter serving integration with the LLM inference layer (Target: Q3 2026)

### Phase 3: Multi-Modality & Provenance (Status: Completed ✅)
- [x] `ContentModality` enum (TEXT_CLAUSE, TABLE, CITATION, OCR_IMAGE, UNKNOWN) added to `auto_labeler.h`
- [x] `modality` field added to `TrainingSample` struct for per-modality confidence thresholds
- [x] `LoRACheckpointManager` – SHA-256 integrity validation, atomic rotation, rolling 3-checkpoint window, manifest JSON (`lora_checkpoint_manager.h/.cpp`)
- [x] `ProvenanceTracker` – ProvenanceRecord, write(), recordFilteredSample(), queryLineage(), getRecord() (`provenance_tracker.h/.cpp`)
- [x] `EnrichmentLRUCache` – thread-safe LRU map inside `KnowledgeGraphEnricher`, enableCache/disableCache/getCacheStats API
- [x] `ConfidenceCalibrator` – isotonic regression (PAV algorithm) per-category threshold selection in `training_pipeline.h/.cpp`
- [x] Multi-modality full parser (`training/modality_parser.h/.cpp`): `ModalityDetector`, `TextClauseExtractor`, `TableExtractor`, `CitationExtractor`, `OCRExtractor`
- [x] Standalone focused test targets for training module (`ModalityParserFocusedTests`, `TrainingConvergenceFocusedTests`)
- [?] Active learning loop (auto-select most informative unlabelled samples)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (8 test files, 4,381 lines; ConfidenceCalibrator, ModalityParser, Pipeline E2E, Data Selection, Checkpoint, Provenance all covered)
- [x] Integration tests (label → train → evaluate → deploy lifecycle) – `test_training_pipeline_e2e.cpp`
- [x] Performance benchmarks – `benchmarks/bench_legal_lora_pipeline.cpp`
- [?] Security audit (PII scanning, tenant isolation, checkpoint encryption at rest – see FUTURE_ENHANCEMENTS.md Security/Reliability section)
- [x] Documentation complete (README.md, ARCHITECTURE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md)
- [x] API stability guaranteed (Pimpl pattern; `TrainingSample` struct stable from v1.x)

## Known Issues & Limitations
- NLP modality extractor is provided externally (`analytics::NlpTextAnalyzer`); not bundled.
- Multi-GPU training requires `THEMIS_ENABLE_LLM && THEMIS_ENABLE_GPU` at build time; single-GPU fallback is automatic.
- `IncrementalTrainingConfig.quantization` governs the _training-module_ view of quantization; INT8/NF4 use `QLoRALayer` (from `llm/lora_framework/quantized_model.h`) so only LoRA adapters are updated in full precision while the base weights remain compressed. The LLM inference layer uses a separate `QuantizationType` defined in `llm/lora_framework/quantization.h`.
- LoRA adapter serving (inference) must be handled by the LLM integration layer.
- Real LoRA weight updates use the embedded Tensor framework; base-model tokenization (llama.cpp) is not yet wired — training batches are encoded as float feature vectors from sample hashes.
- `LoRAAdapter` (training module) operates independently of the LLM-layer `LoRALayer`; integration with `IncrementalLoRATrainer` checkpoints is the caller's responsibility via `exportWeights()` / `importWeights()`.

## Breaking Changes
- `TrainingSample` struct is stable from v1.x; new optional fields only.
- `IncrementalTrainingConfig` may gain new hyperparameter fields in v1.5.0; backward-compatible.
