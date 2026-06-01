# Audit Report - Training Module

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Summary

| Metric | Result |
|---|---|
| Build registration | pass |
| Source set size | pass (module core files present) |
| Focused test presence | pass |
| Open hardening findings | none — all Critical/High findings triaged (batches 1–6) |
| Critical blockers | none identified |

## Verified Files

- src/training/auto_labeler.cpp
- src/training/database_domain_auto_labeler.cpp
- src/training/incremental_lora_trainer.cpp
- src/training/training_pipeline.cpp
- src/training/knowledge_graph_enricher.cpp
- src/training/lora_adapter.cpp
- src/training/ada_lora_adapter.cpp
- src/training/lora_adapter_merger.cpp
- src/training/lora_checkpoint_manager.cpp
- src/training/lora_data_selection.cpp
- src/training/modality_parser.cpp
- src/training/provenance_tracker.cpp
- src/training/adalora_tt_bridge.cpp
- src/training/adapter_serving.cpp

## Findings

### Open

1. [TRN-AUD-01] trainer and checkpoint hardening remains active.
- Severity: medium
- Evidence: roadmap/future retain active work for adapter lifecycle and resume edge scenarios.
- Action: extend deterministic failure-path regression and stress coverage.
- **Progress (2026-06-01, issue #5414 batch 3):** `resumeFromCheckpoint` failure-path
  coverage now added — RFC-01 (empty path), RFC-02 (non-existent path), RFC-03
  (elapsed time always recorded). Core resume guard paths are tested.
- **Progress (2026-06-01, issue #5414 batch 4):** missing checkpoint files no longer
  fall back to synthesized metadata, and router-propagation failures now revert local
  deploy/rollback state instead of leaving serving metadata diverged.

2. [TRN-AUD-02] diagnostics consistency across labeling, training, and serving incident classes needs tightening.
- Severity: medium
- Evidence: active follow-up work for unified training incident taxonomy.
- Action: standardize diagnostics output across dataset, checkpoint, and adapter stages.
- **Progress (2026-06-01, issue #5414 batch 5):** diagnostics-consistency test suite
  added (TDC-01..TDC-10 in `test_training_diagnostics_consistency.cpp`) — covers
  `LabelingStats`, `DeployResult`, `ProvenanceWriteStats`, and
  `CheckpointManifestEntry` zero-initialisation and fail/ok contract invariants.
  All known fail codes used by `deployVersionEx`/`rollbackVersionEx` are verified
  distinct and non-empty.
- **Resolution:** closed — diagnostics contracts are now regression-tested across
  all three stages (labeling, checkpoint, serving).

3. [TRN-AUD-03] benchmark depth should broaden for training pipeline and enrichment workloads.
- Severity: low
- Evidence: core mapping is valid while wider workload diversity remains desirable.
- Action: add benchmark depth for complex training orchestration scenarios.

### Closed

- core training runtime surfaces are present and source-verified.
- documentation set is synchronized to source-verifiable claims.
- changelog/roadmap role separation is aligned to module governance pattern.
- [TRN-AUD-02] diagnostics consistency — all three stages (labeling, checkpoint,
  serving) now have regression-tested contracts (TDC-01..TDC-10, batch 5).
- [scanner findings] All 295 Critical/High scanner findings fully triaged across
  batches 1–6: 6 genuine defects fixed (data_race ×3, model_integrity_gap ×1,
  no_timeout ×2); remaining 289 confirmed false positives documented in MODULE_GAPS.md.

## Compliance Snapshot

| Requirement | Status |
|---|---|
| Source-verifiable behavior claims | pass |
| Structured forward planning in roadmap/future | pass |
| Historical completion tracked in changelog | pass |
| Core module docs synchronized | pass |