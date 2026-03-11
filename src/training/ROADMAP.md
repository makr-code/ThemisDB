# Training Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Validated: 2026-03-11 (b2342851) -->

## Current Status
v1.x – Domain-specific AI fine-tuning toolchain for legal text. LegalAutoLabeler, IncrementalLoRATrainer, and KnowledgeGraphEnricher are production-ready with checkpoint/resume, adapter versioning, and graph-context enrichment.

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
- [x] Custom AQL query registration for domain-specific traversals
- [x] Pimpl pattern for ABI stability across all three components
- [x] LoRA Checkpoint Manager with SHA-256 integrity validation (Target: Q1 2026) — `lora_checkpoint_manager.h/.cpp` (b2342851)
- [x] Training Sample Provenance and Lineage Tracking (Target: Q1 2026) — `provenance_tracker.h/.cpp` (b2342851)
- [x] Knowledge Graph Enrichment LRU Cache (Target: Q1 2026) — `EnrichmentLRUCache` in `knowledge_graph_enricher.cpp` (b2342851)
- [x] ContentModality enum for multi-modality sample tracking (Target: Q1 2026) — `auto_labeler.h` (b2342851)
- [x] Confidence-Threshold Auto-Calibration via isotonic regression (Target: Q1 2026) — `ConfidenceCalibrator` in `training_pipeline.h/.cpp` (b2342851)
- [x] Multi-modality parser (`ModalityDetector`, `TextClauseExtractor`, `TableExtractor`, `CitationExtractor`, `OCRExtractor`) (Target: Q1 2026) — `modality_parser.h/.cpp` (b2342851)

## In Progress 🚧
- [~] Multi-GPU distributed training coordination (Target: Q2 2026)
  - Subsystems: `src/training/incremental_lora_trainer.cpp`, `src/acceleration/` GPU dispatch
  - Inputs: `IncrementalTrainingConfig.num_gpus`, device list; model partitioned across GPUs via NCCL/MPI
  - Outputs: single merged checkpoint per epoch; `TrainingResult.elapsed_ms` reflects wall-clock (not GPU-sum) time
  - Constraints: each GPU worker must use the same LoRA rank/alpha; gradient sync every `sync_steps` (default 1)
  - Errors: device-count mismatch → `std::runtime_error`; NCCL init failure → `TrainingResult.success=false` + message
  - Tests: unit — mock GPU dispatch, verify gradient-sync calls; integration — 2-GPU synthetic dataset
  - Perf: ≥1.8× throughput on 2 GPUs vs. 1 GPU baseline (7B model, batch_size=8, r=16)

- [~] Automated hyperparameter search (LoRA rank, learning rate sweep) (Target: Q2 2026)
  - Subsystems: `src/training/training_pipeline.cpp` (new `HyperparamSearch` inner class), `ConfidenceCalibrator`
  - Inputs: `HyperparamSearchConfig{rank_candidates, lr_candidates, max_trials, budget_seconds}`; validation split fraction
  - Outputs: `HyperparamResult{best_rank, best_lr, best_val_loss, trial_log}`; best config auto-applied to pipeline
  - Constraints: deterministic trial ordering (seeded random); concurrent trials capped at `num_gpus`
  - Errors: no improvement after `max_trials` → return best seen; budget exceeded → early-stop, return best so far
  - Tests: unit — mock trainer, verify trial scheduling; integration — sweep over 3 rank values on synthetic dataset
  - Perf: trial overhead (excluding training) ≤ 50 ms/trial; total sweep for 9-trial 3×3 grid ≤ 3× single-train time

- [~] Adapter serving integration with LLM inference layer (Target: Q3 2026)
  - Subsystems: `src/training/incremental_lora_trainer.cpp` (`deployVersion`), `src/llm/multi_lora_manager.h`
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
- [?] Automated hyperparameter search (LoRA rank and learning rate sweep) (Target: Q2 2026)
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
- Distributed/multi-GPU training is not yet coordinated; single-node only.
- LoRA adapter serving (inference) must be handled by the LLM integration layer.

## Breaking Changes
- `TrainingSample` struct is stable from v1.x; new optional fields only.
- `IncrementalTrainingConfig` may gain new hyperparameter fields in v1.5.0; backward-compatible.
