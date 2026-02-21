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
- [ ] Multi-GPU distributed training coordination (Target: Q2 2026)
- [ ] Automated hyperparameter search (LoRA rank, learning rate sweep) (Target: Q2 2026)
- [ ] Adapter serving integration with LLM inference layer (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Support for additional legal jurisdictions beyond German law
- [ ] Active learning loop (auto-select most informative samples for labeling)
- [ ] Training data deduplication and near-duplicate filtering
- [ ] Evaluation metrics dashboard (validation loss curves, accuracy)
- [ ] Export labeled datasets in standard formats (JSONL, Hugging Face datasets)

### Long-term (6-12 months)
- [ ] Reinforcement learning from human feedback (RLHF) training loop
- [ ] Multi-modal training samples (text + table + chart)
- [ ] Domain adaptation beyond legal (medical, financial)
- [ ] Federated learning for privacy-preserving cross-institution training
- [ ] Model distillation from large to small adapters

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
- [~] Adapter version management: atomic deploy/rollback with integrity verification (Target: Q2 2026)
- [~] Multi-domain support beyond German legal text (medical, financial) (Target: Q2 2026)
- [ ] Automated hyperparameter search (LoRA rank and learning rate sweep) (Target: Q2 2026)
- [ ] Adapter serving integration with the LLM inference layer (Target: Q3 2026)

### Phase 3: Multi-Modality & Provenance (Status: Planned 📋)
- [ ] Multi-modality parser for code snippets and tabular data (`training/modality_parser.cpp`)
- [ ] LoRA checkpoint manager with deduplication and storage quota enforcement
- [ ] Confidence auto-calibration (Platt scaling / isotonic regression)
- [ ] Provenance tracking – record source document and extraction path per training sample
- [ ] Active learning loop (auto-select most informative unlabelled samples)

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (label → train → evaluate → deploy lifecycle)
- [ ] Performance benchmarks (samples/sec labeling, training convergence)
- [ ] Security audit (training data access control, adapter integrity verification)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- NLP modality extractor is provided externally (`analytics::NlpTextAnalyzer`); not bundled.
- Distributed/multi-GPU training is not yet coordinated; single-node only.
- LoRA adapter serving (inference) must be handled by the LLM integration layer.

## Breaking Changes
- `TrainingSample` struct is stable from v1.x; new optional fields only.
- `IncrementalTrainingConfig` may gain new hyperparameter fields in v1.5.0; backward-compatible.
