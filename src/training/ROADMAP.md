# Training Module Roadmap

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
