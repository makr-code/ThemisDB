> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

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

- [x] Adapter serving integration with LLM inference layer (Target: Q3 2026) — `ILLMRouter` abstract interface + `DeployResult` in `include/training/adapter_serving.h`; `setLLMRouter(ILLMRouter*)` on `IncrementalLoRATrainer`; `deployVersionEx()`/`rollbackVersionEx()` propagate weight to router; 29 focused tests in `test_training_phase2.cpp`

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
- [x] Domain adaptation beyond legal (medical, financial) — `DomainType` LEGAL/MEDICAL/FINANCIAL in `auto_labeler.h`; domain-specific keyword extraction for medical/financial domains in `auto_labeler.cpp`
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

### Phase 2: Adapter Management & Multi-Domain (Status: Completed ✅)
- [x] Adapter version management: atomic deploy/rollback with integrity verification (Target: Q2 2026) — `deployVersionEx()`/`rollbackVersionEx()` in `incremental_lora_trainer.h/.cpp`; `verifyAdapterIntegrity()` calls `LoRACheckpointManager::validate()` when `checkpoint_dir` is set; bypass for unmanaged adapters; `DeployResult{success,active_version,split_applied,error}` result struct; error codes: `"version_not_found"`, `"integrity_failure"`, `"router_unavailable"`, `"invalid_split"`
- [x] Multi-domain support beyond German legal text (medical, financial) (Target: Q2 2026) — `DomainType` enum (LEGAL/MEDICAL/FINANCIAL) added to `auto_labeler.h`; `AutoLabelConfig::domain_type` field; `extractFallbackModalities()` in `auto_labeler.cpp` dispatches domain-specific obligation/recommendation/permission/prohibition patterns for medical (must/shall/required/should/recommended/may/contraindicated/verboten) and financial (must/shall/required/should/may/prohibited/forbidden/disclose/report/offenlegen/melden) domains; German and English terms both covered
- [x] Automated hyperparameter search (LoRA rank and learning rate sweep) (Target: Q2 2026)
- [x] Adapter serving integration with the LLM inference layer (Target: Q3 2026) — `ILLMRouter` abstract interface (`adapter_serving.h/.cpp`): `setAdapterWeight(version,weight)`, `isAvailable()`, `activeVersion()`; `IncrementalLoRATrainer::setLLMRouter(ILLMRouter*)` wires the router; `deployVersionEx()`/`rollbackVersionEx()` propagate weight updates to the router atomically after local registry update; unavailable router → `DeployResult.error = "router_unavailable"`

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

### Phase 4: DATABASE_OPTIMIZER Domain AutoLabeler — IMPL-A1 (Target: Q3 2026)

> *Paper 1 — §5 Training Data Pipeline / §7.4 Golden Dataset Construction*
> Issue: [docs/issues/lora_loops/IMPL-A1-dataset-construction.md](../../docs/issues/lora_loops/IMPL-A1-dataset-construction.md)

- [x] Add `DomainType::DATABASE_OPTIMIZER` to `DomainType` enum in `include/training/auto_labeler.h`
- [x] Implement `DatabaseDomainAutoLabeler` class (`include/training/database_domain_auto_labeler.h`, `src/training/database_domain_auto_labeler.cpp`): labels `(query, plan, Δlatency)` triples
- [x] Add `DATABASE_OPTIMIZER` branch to `LegalAutoLabeler::categorize()` dispatch table
- [x] Add domain keywords (EXPLAIN, index scan, seq scan, hash join, latency, p99) to `LoRADataSelectionConfig`
- [ ] Implement optimizer-log export CLI: emits JSONL with `(query, explain_plan, latency_delta_ms)` fields
- [x] Confidence score: `tanh(|Δlatency_ms| / 50)` — labels with |Δlatency| < 5 ms auto-rejected
- [ ] Validation against `LoRADataSelectionPipeline` quality filters (duplicate-query dedup, min confidence 0.85)
- [ ] Collect 1 000 labeled pairs from all 4 loops as minimum viable golden dataset
- [ ] 8 new unit tests: `DBO-01` … `DBO-08` in `tests/test_training_database_optimizer.cpp`

### Phase 5: Federation Bridges — IMPL-A3 (Target: Q3 2026)

> *Paper 1+3 — §4.5 Adapter Lifecycle / Distributed Knowledge §Layer B*
> Issue: [docs/issues/lora_loops/IMPL-A3-federation-hooks.md](../../docs/issues/lora_loops/IMPL-A3-federation-hooks.md)

- [ ] Implement `IncrementalLoRATrainer::exportGradient()` → `EncryptedGradient` (opaque blob, AES-256-GCM)
- [ ] Implement `IncrementalLoRATrainer::applyGlobalDelta(const GlobalAdapterDelta&)` → applies FedAvg aggregate to local adapter weights
- [ ] Define `EncryptedGradient` and `GlobalAdapterDelta` structs in `training_interfaces.h`
- [ ] Privacy invariant: `exportGradient()` output must never contain raw training samples — enforced by unit test
- [ ] 5 new unit tests: `FED-01` … `FED-05` in `tests/test_training_federation_hooks.cpp`
  - `FED-01` `exportGradient()` returns non-empty blob after training
  - `FED-02` `applyGlobalDelta()` verifiably changes adapter weights (weight-diff ≠ 0)
  - `FED-03` applying zero-delta leaves weights unchanged
  - `FED-04` privacy: raw sample text absent from `EncryptedGradient` serialised bytes
  - `FED-05` double-apply is idempotent when delta == 0

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
