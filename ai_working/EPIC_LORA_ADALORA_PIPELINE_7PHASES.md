# EPIC: LoRA/AdaLoRA Training Pipeline Implementation
## 7-Phase Specification & Implementation Roadmap

**Status:** Planning / Phase 1 Foundation  
**Target Milestone:** Q3 2026 / LoRA-Produktisierung  
**Maintainer:** @makr-code  
**Created:** 2026-07-01  
**Last Updated:** 2026-07-01  

---

## 1. Executive Summary

### Goal

Develop a SOP-compliant and scientifically-grounded end-to-end pipeline for:
- RAG-driven training data selection and auditability
- Package provenance tracking with cryptographic integrity
- Model-switching resilient adapter distribution
- Reproducible, policy-compliant LoRA/AdaLoRA artifact management

### Key Deliverables

| Phase | Focus Area | Key Classes/Interfaces | Target Date |
|-------|-----------|----------------------|------------|
| 1 | Training core hardening | `LoRAPackage`, `TrainingAuditLog`, `DeterminismValidator` | Q3 2026 |
| 2 | Data selection policy | `DatasetSnapshot`, `SelectionPolicy`, `EligibilityChain` | Q3 2026 |
| 3 | Artifact manifests | `LoRAPackage`, `PortableAdapterProduct`, `ManifestSerializer` | Q3 2026 |
| 4 | Provenance layer | `ProvenanceTracker`, `HashChain`, `AuditLedger` | Q3 2026 |
| 5 | Distribution | `AdapterDistributionManifest`, `ShardingIntegration` | Q4 2026 |
| 6 | Model resilience | `CompatibilityMatrix`, `ModelMigrationWorkflow` | Q4 2026 |
| 7 | Documentation & SOP | Runbooks, Best Practices, Training Material | Q4 2026 |

---

## 2. Current State Analysis (2026-06-01)

### Existing Components

**Production-Ready (86-100 score):**
- `include/training/incremental_lora_trainer.h` — Core trainer with quantization support
- `include/training/lora_data_selection.h` — Data selection with 5-stage pipeline
- `include/training/auto_labeler.h` — Auto-labeling with modality detection
- `include/llm/lora_framework/lora_provenance.h` — Provenance records (local + external)
- `include/rag/continuous_learning_orchestrator.h` — 4-loop orchestration (Layer 11A)

**Test Coverage:**
- `tests/test_lora_data_selection.cpp` — Data selection validation
- `tests/incremental/test_incremental_lora_trainer.cpp` — Trainer unit tests
- `tests/test_auto_labeler_production.cpp` — Auto-labeler production validation
- `tests/test_continuous_learning_orchestrator.cpp` — Orchestration tests

### GAP Analysis (Phase 1 Focus)

| Gap | Category | Impact | Severity |
|-----|----------|--------|----------|
| No determinism guarantee in training loops | Safety | Model reproducibility | 🔴 CRITICAL |
| No integrated audit logging across stages | Auditability | Compliance/forensics | 🔴 CRITICAL |
| Package/provenance layer missing | Integrity | Distribution risk | 🟠 HIGH |
| No cross-shard consistency checks | Distributed | Data corruption risk | 🟠 HIGH |
| Model-switch resilience untested | Reliability | Adapter portability | 🟡 MEDIUM |
| No structured dataset snapshot manifest | Policy | Governance gap | 🟡 MEDIUM |
| Selection policy not externalized | Policy | Reproducibility | 🟡 MEDIUM |

---

## 3. Seven-Phase Implementation Plan

### Phase 1: Harden Training Core (Safety, Determinism, Auditability, Integrity)

**Goals:**
- Guarantee deterministic training reproducibility
- Implement comprehensive audit logging across all training stages
- Add exception handling and fail-fast validation
- Ensure data integrity and transaction semantics

**Key Classes/Interfaces to Create:**

1. **TrainingAuditLog** — Immutable audit event ledger
   - Event types: TRAINING_START, BATCH_PROCESSED, CHECKPOINT_SAVED, TRAINING_END, ERROR
   - Fields: timestamp, event_type, actor_id, resource_hash, operation_result
   - Persistence: JSON Lines to audit_log_path + WAL

2. **DeterminismValidator** — Runtime reproducibility enforcement
   - Seed checkpoints at each epoch boundary
   - RNG state serialization/validation
   - Floating-point operation ordering enforcement
   - GPU determinism flags (CUDA_LAUNCH_BLOCKING, cuDNN deterministic mode)

3. **TrainingStateCheckpoint** — Serializable training state
   - Epoch, step, RNG state, loss history
   - Optimizer state (momentum buffers)
   - Dataset position (cursor/seed)
   - Provenance snapshot (hashes)

4. **IntegrityValidator** — Data and model integrity checks
   - Dataset hash verification at start
   - Model weights hash before/after training
   - Adapter weights integrity (NaN/Inf detection)
   - Cross-shard consistency (if distributed)

5. **TrainingErrorRecovery** — Exception handling and recovery
   - Fail-fast on corrupted data
   - Graceful OOM handling with checkpoint save
   - Timeout enforcement with partial state recovery
   - Detailed error logging with context

**New Tests (P1 Phase 1):**
- `test_determinism_reproducibility.cpp` — Train same dataset twice, verify identical weights
- `test_audit_log_completeness.cpp` — Verify all events logged
- `test_integrity_validation.cpp` — Detect corrupted data/models
- `test_error_recovery.cpp` — Handle OOM, timeout, data corruption
- `test_training_state_serialization.cpp` — Checkpoint/restore round-trip

**Implementation Tasks:**
- [ ] Create `include/training/training_audit_log.h`
- [ ] Create `include/training/determinism_validator.h`
- [ ] Create `include/training/training_state_checkpoint.h`
- [ ] Create `include/training/integrity_validator.h`
- [ ] Integrate audit logging into `IncrementalLoRATrainer::train()`
- [ ] Add determinism validation hooks at epoch boundaries
- [ ] Add integrity checks before/after training
- [ ] Implement error recovery with state rollback
- [ ] Add 5 comprehensive P1 tests
- [ ] Update documentation with determinism guarantees

**Acceptance Criteria:**
- [ ] All training runs produce identical weights (same data, same seed)
- [ ] 100% audit coverage (every stage logged)
- [ ] Data/model corruption detected within 1 event cycle
- [ ] OOM recovery preserves checkpoint state
- [ ] All error cases fail-fast with diagnostic logging

---

### Phase 2: DatasetSnapshot & Selection-Policy Layer

**Goals:**
- Create structured dataset snapshot manifests
- Externalize selection policies
- Track dataset lineage and eligibility

**Key Classes/Interfaces:**

1. **DatasetSnapshot** — Immutable dataset state record
   - Dataset version, sample count, hash
   - Split manifest (train/val/test indices)
   - Policy used for selection
   - Eligibility criteria snapshot
   - Timestamp and creator identity

2. **SelectionPolicy** — Externalized selection logic
   - Policy name, version, parameters
   - YAML serialization
   - Eligibility rules engine
   - Policy versioning and compatibility

3. **SelectionPolicyRegistry** — Policy management
   - Load policy by name/version
   - Validate policy compatibility
   - Policy change tracking

4. **EligibilityChain** — Lineage of inclusion/exclusion decisions
   - Per-sample eligibility trace
   - Reason for inclusion/exclusion
   - Policy version applied

**New Tests:**
- `test_dataset_snapshot_manifest.cpp`
- `test_selection_policy_versioning.cpp`
- `test_eligibility_chain_tracing.cpp`

**Acceptance Criteria:**
- [ ] DatasetSnapshot captures full selection provenance
- [ ] Selection policies are externalized and versioned
- [ ] Policy changes are tracked and discoverable
- [ ] Reproducible dataset selection across training runs

---

### Phase 3: LoRAPackage & PortableAdapterProduct Manifest

**Goals:**
- Introduce canonical LoRAPackage artifact class
- Define PortableAdapterProduct for deployment
- Standardize serialization and audit

**Key Classes/Interfaces:**

1. **LoRAPackage** — Complete reproducible training artifact
   - Training configuration (config.json)
   - Dataset snapshot (dataset.json)
   - Training state (checkpoint files)
   - Provenance record (provenance.json)
   - Audit log (audit.jsonl)
   - Adapter weights (weights.safetensors)

2. **PortableAdapterProduct** — Deployable artifact
   - Adapter ID (UUID)
   - Model binding (base model hash)
   - Weights (quantized)
   - Compatibility metadata
   - Digital signature
   - Deployment manifest

3. **PackageSerializer** — Serialization format
   - Zip-based packaging
   - JSON manifest format
   - Integrity hashing
   - Encryption support (optional)

4. **PackageValidator** — Package validation
   - Structure validation
   - Integrity checks
   - Signature verification
   - Model compatibility checks

**New Tests:**
- `test_lora_package_serialization.cpp`
- `test_portable_adapter_product.cpp`
- `test_package_validator.cpp`

**Acceptance Criteria:**
- [ ] LoRAPackage captures all training artifacts
- [ ] Packages are reproducible and auditable
- [ ] PortableAdapterProduct is deployable to production
- [ ] Package signatures prevent tampering

---

### Phase 4: HashChain/Provenance Layer

**Goals:**
- Extend ProvenanceTracker to full audit ledger
- Implement cryptographic hash chain
- Enable distributed verification

**Key Classes/Interfaces:**

1. **ProvenanceTracker** (extended) — Central tracking
   - Event recording with sequential hash linking
   - Hash chain verification
   - Merkle root computation
   - Timestamp integration

2. **HashChain** — Cryptographic linking
   - SHA-256 hashes with sequential linking
   - Merkle tree construction
   - Chain validation and integrity proof
   - Fork detection

3. **AuditLedger** — Immutable audit record
   - Hash chain storage
   - Rollback protection
   - Distribution receipts
   - Compliance event markers

4. **ProvenanceProof** — Auditable evidence
   - Chain of custody
   - Merkle proofs
   - Timestamp evidence
   - Digital signatures

**New Tests:**
- `test_hash_chain_integrity.cpp`
- `test_audit_ledger_rollback_protection.cpp`
- `test_provenance_proof_verification.cpp`

**Acceptance Criteria:**
- [ ] Hash chain prevents provenance tampering
- [ ] Audit ledger is rollback-protected
- [ ] Merkle proofs enable efficient verification
- [ ] All provenance events cryptographically linked

---

### Phase 5: Adapter-Distribution & Sharding-Coupling

**Goals:**
- Integrate adapter distribution with sharding
- Enable cross-shard adapter propagation
- Implement RAID/redundancy for adapters

**Key Classes/Interfaces:**

1. **AdapterDistributionManifest** — Distribution plan
   - Adapter ID and version
   - Target shards
   - Replication factor
   - Distribution receipt tracking

2. **ShardingIntegration** — Sharding bridge
   - Adapter placement logic
   - Cross-shard synchronization
   - Distribution verification
   - Failover handling

3. **DistributionReceipt** — Acknowledgment tracking
   - Shard acknowledgments
   - Hash verification per shard
   - Timestamp and latency info
   - Failure tracking

4. **AdapterRedundancy** — RAID-style redundancy
   - Erasure coding option
   - Multi-replica coordination
   - Recovery procedures
   - Merkle root proofs

**New Tests:**
- `test_adapter_distribution_manifest.cpp`
- `test_sharding_distribution_integration.cpp`
- `test_distribution_receipts.cpp`
- `test_adapter_redundancy.cpp`

**Acceptance Criteria:**
- [ ] Adapters distributed to all target shards
- [ ] Distribution receipts track success/failure
- [ ] Cross-shard consistency verified
- [ ] Automatic failover on shard loss

---

### Phase 6: Model-Wechsel-Workflow & Compatibility Matrix

**Goals:**
- Enable LLM model switching without retraining
- Implement compatibility assessment
- Provide migration workflows and runbooks

**Key Classes/Interfaces:**

1. **CompatibilityMatrix** — Model compatibility tracking
   - Source model → Target model mappings
   - Compatibility score (0-100)
   - Migration feasibility assessment
   - Test coverage per combination

2. **ModelMigrationWorkflow** — Migration orchestration
   - Pre-migration validation
   - Adapter transformation/bridging
   - Post-migration verification
   - Rollback capability

3. **AdapterBridge** — Cross-model adaptation
   - Adapter transformation logic
   - Layer mapping (source → target)
   - Fallback strategies
   - Performance prediction

4. **MigrationRunbook** — Documented procedures
   - Step-by-step migration guide
   - Rollback procedures
   - Validation checklist
   - Monitoring alerts

**New Tests:**
- `test_compatibility_matrix.cpp`
- `test_model_migration_workflow.cpp`
- `test_adapter_bridge.cpp`
- `test_migration_rollback.cpp`

**Acceptance Criteria:**
- [ ] Compatibility assessment available before migration
- [ ] Migration workflow automated and documented
- [ ] Rollback to previous model/adapter available
- [ ] Post-migration performance verified

---

### Phase 7: SOP/Docs/Review

**Goals:**
- Create comprehensive documentation
- Develop operational procedures
- Document best practices and lessons learned

**Deliverables:**

1. **Technical Runbooks**
   - Data Selection Runbook (Phase 2)
   - Training Hardening Runbook (Phase 1)
   - Package Distribution Runbook (Phase 5)
   - Model Migration Runbook (Phase 6)
   - Emergency Recovery Playbook

2. **API Documentation**
   - Comprehensive Doxygen documentation
   - Example workflows
   - Error handling guide
   - Performance tuning guide

3. **Best Practices Guide**
   - Data preparation guidelines
   - Hyperparameter tuning
   - Monitoring and alerting
   - Incident response

4. **Lessons Learned**
   - Design decisions and rationale
   - Known limitations and workarounds
   - Performance characteristics
   - Security considerations

5. **Training Material**
   - Video walkthroughs
   - Hands-on tutorials
   - Case studies
   - FAQ

**New Tests:**
- Documentation build verification
- Example code compilation
- Tutorial execution validation

**Acceptance Criteria:**
- [ ] All modules have comprehensive API docs
- [ ] Runbooks are tested and validated
- [ ] Best practices documented and followed
- [ ] All example code compiles and runs

---

## 4. Cross-Phase Dependencies & Integration Points

### Integration with Existing Systems

1. **Continuous Learning Orchestrator (Layer 11A)**
   - Feeds LoRA training with feedback loop data
   - Consumes trained adapters
   - Coordinated via Phase 4 provenance

2. **Sharding Subsystem**
   - Phase 5 distribution uses sharding layer
   - Adapter consistency via cross-shard validation

3. **Storage Layer**
   - Package persistence via storage service
   - Checkpoint management via RocksDB

4. **Security/Audit Infrastructure**
   - Audit logging via audit_logger.h
   - Cryptographic operations via security layer

### Rollout Dependencies

- Phase 1 is prerequisite for all others (training hardening foundation)
- Phase 2 can proceed parallel to Phase 1
- Phase 3 depends on Phase 1 + 2
- Phase 4 depends on Phase 3
- Phase 5 can proceed parallel to Phase 4
- Phase 6 depends on Phase 3 + 5
- Phase 7 depends on all phases complete

---

## 5. Success Metrics & Quality Gates

### Phase 1 Metrics

| Metric | Target | Verification |
|--------|--------|--------------|
| Determinism reproducibility | 100% identical weights | Unit test |
| Audit coverage | 100% events logged | Audit log inspection |
| Error detection latency | ≤ 1 event | Error injection test |
| Recovery time | ≤ 30 seconds | Timeout test |

### Overall EPIC Metrics

| Metric | Target | Notes |
|--------|--------|-------|
| Code coverage | ≥ 85% | All new phases |
| Test execution time | ≤ 5 minutes | All P1 tests combined |
| Documentation completeness | 100% | API + runbooks + best practices |
| Production readiness | GA | All acceptance criteria met |

---

## 6. Risk Assessment & Mitigation

### High-Risk Items

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Determinism instability across GPU architectures | Reproducibility broken | Extensive GPU-specific testing |
| Audit logging performance impact | Training slowdown | Asynchronous logging + batching |
| Distributed consistency bugs | Data corruption | Comprehensive cross-shard validation |
| Model-switch compatibility issues | Adapter failures | Pre-migration validation matrix |

### Mitigation Strategies

1. **Extensive GPU Testing**
   - Test on CUDA, Hip, Metal, Vulkan
   - Validate determinism on each platform
   - Performance profiling for each path

2. **Audit Logging Optimization**
   - Asynchronous event batching
   - Compression of repetitive events
   - Configurable detail levels

3. **Distributed Validation**
   - Merkle proofs for consistency
   - Cross-shard polling verification
   - Forensic replay capability

4. **Pre-Migration Validation**
   - Compatibility scoring algorithm
   - Dry-run testing framework
   - Staged rollout support

---

## 7. Implementation Checklist

### Phase 1: Harden Training Core

- [ ] Design review: TrainingAuditLog schema
- [ ] Design review: DeterminismValidator approach
- [ ] Implementation: TrainingAuditLog class
- [ ] Implementation: DeterminismValidator class
- [ ] Implementation: TrainingStateCheckpoint class
- [ ] Implementation: IntegrityValidator class
- [ ] Implementation: TrainingErrorRecovery
- [ ] Integration: Audit logging into IncrementalLoRATrainer
- [ ] Integration: Determinism validation hooks
- [ ] Integration: Integrity checks
- [ ] Integration: Error recovery
- [ ] Testing: Determinism reproducibility test
- [ ] Testing: Audit log completeness test
- [ ] Testing: Integrity validation test
- [ ] Testing: Error recovery test
- [ ] Testing: State serialization test
- [ ] Documentation: API documentation
- [ ] Documentation: Determinism guarantees guide
- [ ] Code review: Phase 1 implementation
- [ ] Performance profiling: Audit logging overhead
- [ ] Pre-commit verification: All P1 tests pass

### Phases 2-7: Per-phase checklists

(Similar structure for each phase)

---

## 8. Timeline & Milestones

### Q3 2026 (Starts: 2026-07-01)

| Week | Phase | Milestone |
|------|-------|-----------|
| 27-28 | 1 | Phase 1 design + implementation kick-off |
| 28-29 | 1-2 | Phase 1 testing + Phase 2 design |
| 30-31 | 2-3 | Phase 2 implementation + Phase 3 design |
| 32-34 | 3-4 | Phase 3-4 implementation + testing |

### Q4 2026

| Quarter | Phases | Milestone |
|---------|--------|-----------|
| Q4 Early | 4-5 | Phase 4-5 implementation + integration |
| Q4 Mid | 5-6 | Phase 5 distribution testing, Phase 6 design |
| Q4 Late | 6-7 | Phase 6-7 implementation + documentation |
| Q4 End | All | GA readiness, final verification |

---

## 9. References & Related Documents

### Architecture Documents
- `ARCHITECTURE.md` — System architecture overview
- `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md` — LoRA research foundation
- `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` — Optimization layers
- `src/llm/ARCHITECTURE.md` — LLM module architecture
- `src/training/ARCHITECTURE.md` — Training module architecture

### Related Roadmap Items
- `ROADMAP.md` § 2.6 LoRA Foundation
- `ROADMAP.md` § 2.7 LLM Optimization Layers
- `ROADMAP.md` § 2.8 Distributed Knowledge

### Standards & References
- RFC 3161 — Timestamp Protocol
- OWASP Cryptographic Storage Cheat Sheet
- eIDAS Regulation (EU) 2014/910
- NIST Special Publication 800-53 — Audit and Accountability

---

## 10. Approval & Sign-Off

**EPIC Owner:** @makr-code  
**Architecture Review:** Pending  
**Security Review:** Pending  
**Performance Review:** Pending  

**Approvals:**
- [ ] Architecture Lead
- [ ] Security Lead
- [ ] Performance Lead
- [ ] Maintainer (@makr-code)

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-01  
**Status:** DRAFT — Awaiting Design Review
