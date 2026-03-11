# Training Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Domain-specific AI fine-tuning toolchain for legal text. LegalAutoLabeler, IncrementalLoRATrainer, and KnowledgeGraphEnricher are production-ready with checkpoint/resume, adapter versioning, and graph-context enrichment.

## Completed ✅
- [x] LegalAutoLabeler – automated training sample generation from legal documents via NLP modality extraction
- [x] `labelAll()`, `labelDocument()`, `labelQuery()` APIs
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

## In Progress 🚧
- [x] LoRA Checkpoint Manager with SHA-256 integrity validation (Target: Q1 2026)
- [x] Training Sample Provenance and Lineage Tracking (Target: Q1 2026)
- [x] Knowledge Graph Enrichment LRU Cache (Target: Q1 2026)
- [x] ContentModality enum for multi-modality sample tracking (Target: Q1 2026)
- [x] Confidence-Threshold Auto-Calibration via isotonic regression (Target: Q1 2026)
- [?] Multi-GPU distributed training coordination (Target: Q2 2026)
- [?] Automated hyperparameter search (LoRA rank, learning rate sweep) (Target: Q2 2026)
- [?] Adapter serving integration with LLM inference layer (Target: Q3 2026)

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

### Phase 3: Multi-Modality & Provenance (Status: In Progress 🚧)
- [x] `ContentModality` enum (TEXT_CLAUSE, TABLE, CITATION, OCR_IMAGE, UNKNOWN) added to `auto_labeler.h`
- [x] `modality` field added to `TrainingSample` struct for per-modality confidence thresholds
- [x] `LoRACheckpointManager` – SHA-256 integrity validation, atomic rotation, rolling 3-checkpoint window, manifest JSON (`lora_checkpoint_manager.h/.cpp`)
- [x] `ProvenanceTracker` – ProvenanceRecord, write(), recordFilteredSample(), queryLineage(), getRecord() (`provenance_tracker.h/.cpp`)
- [x] `EnrichmentLRUCache` – thread-safe LRU map inside `KnowledgeGraphEnricher`, enableCache/disableCache/getCacheStats API
- [x] `ConfidenceCalibrator` – isotonic regression (PAV algorithm) per-category threshold selection in `training_pipeline.h/.cpp`
- [?] Multi-modality full parser for code snippets and tabular data (`training/modality_parser.cpp`)
- [?] Active learning loop (auto-select most informative unlabelled samples)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% — `test_lora_checkpoint_manager.cpp`, `test_provenance_tracker.cpp`, `test_lora_data_selection.cpp`, `test_training_pipeline_e2e.cpp`; focused standalone targets: `LoRACheckpointManagerFocusedTests`, `ProvenanceTrackerFocusedTests`
- [?] Integration tests (label → train → evaluate → deploy lifecycle)
- [?] Performance benchmarks (samples/sec labeling, training convergence)
- [?] Security audit (training data access control, adapter integrity verification)
- [?] Documentation complete
- [~] API stability guaranteed — `TrainingSample`, `IncrementalTrainingConfig` stable; new Phase-3 classes (`LoRACheckpointManager`, `ProvenanceTracker`) additive

## Known Issues & Limitations
- NLP modality extractor is provided externally (`analytics::NlpTextAnalyzer`); not bundled.
- Distributed/multi-GPU training is not yet coordinated; single-node only.
- LoRA adapter serving (inference) must be handled by the LLM integration layer.

## Breaking Changes
- `TrainingSample` struct is stable from v1.x; new optional fields only.
- `IncrementalTrainingConfig` may gain new hyperparameter fields in v1.5.0; backward-compatible.
