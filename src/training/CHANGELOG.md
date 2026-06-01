> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Training Module

All notable changes to the training module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Fixed
- **data_race** (incremental_lora_trainer): `llm_router_->setAdapterWeight` calls in
  `deployVersionEx` and `rollbackVersionEx` are now protected by `router_mutex_`
  (issue #5414 / Phase 1 safety hardening).
- **model_integrity_gap** (lora_checkpoint_manager): `parseManifest` now validates
  checkpoint_path for path-traversal sequences and rejects entries with a present but
  malformed SHA-256 field (must be 64 lowercase hex characters).
- **no_timeout** (provenance_tracker): `ProvenanceTrackerConfig` gains a
  `write_timeout_ms` field (default 0 = no limit); `write()` enforces the deadline
  per batch, returning early and counting remaining records as rejected when exceeded.

### Added (tests, #5414 batch 3)
- **resumeFromCheckpoint failure paths** (incremental_lora_trainer): RFC-01 asserts
  empty checkpoint path returns `success=false` with descriptive error; RFC-02 asserts
  a non-existent path returns `success=false`; RFC-03 asserts `training_time_seconds`
  is non-negative on failure.
- **provenance timeout integration** (training_pipeline): pipeline completes with a
  1 ms write timeout (no hang); pipeline stats satisfy `written + rejected = total`.
- **SelfImprovementConfig invalid delta** (lora_data_selection): YAML with a
  non-numeric delta value in `adaptive_rules` throws `std::runtime_error`.
- **False-positive documentation** (MODULE_GAPS.md): scanner triggers for
  `prompt_injection`, `model_integrity_gap`, `hardcoded_secret`, `fp_exact_comparison`,
  and remaining `no_timeout` / `null_dereference` / `iterator_invalidation` findings
  are confirmed false positives and annotated in the remediation note.

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from GPU training cycle and LoRA training benchmark suites.

## [2.1.x] - 2026

### Added
- trainer, checkpoint, and adapter lifecycle hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded dataset enrichment, provenance, and adapter-serving surfaces.

## [1.x] - 2024-2025

### Added
- foundational training, LoRA, and provenance infrastructure.