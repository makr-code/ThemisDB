> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Training Module

All notable changes to the training module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Fixed
- **checkpoint payload hardening** (incremental_lora_trainer, issue #5414):
  `resumeFromCheckpoint` now fails fast when LoRA checkpoint weight payloads are
  missing/malformed/truncated or contain unexpected trailing bytes, instead of
  silently resuming with fresh weights.
- **data_race** (incremental_lora_trainer): `llm_router_->setAdapterWeight` calls in
  `deployVersionEx` and `rollbackVersionEx` are now protected by `router_mutex_`
  (issue #5414 / Phase 1 safety hardening).
- **exception robustness** (incremental_lora_trainer, issue #5414 batch 7):
  `deployVersionEx`/`rollbackVersionEx` now catch router exceptions, roll back
  local version-registry state, and return `router_update_failed` instead of
  propagating exceptions across the module boundary.
- **concurrency hardening** (incremental_lora_trainer, issue #5414 batch 12):
  adapter registry reads/writes are now mutex-protected across deploy/rollback/
  list/select flows, and router attach/detach (`setLLMRouter`) is synchronized
  with deploy/rollback router propagation to avoid unsynchronised pointer access.
- **checkpoint-manager concurrency hardening** (incremental_lora_trainer,
  issue #5414 batch 13): shared `LoRACheckpointManager` lazy-init and access in
  integrity checks + checkpoint registration are now mutex-protected to prevent
  concurrent deploy/rollback/resume races on the manager instance.
- **checkpoint integrity semantics** (incremental_lora_trainer): `resumeFromCheckpoint`
  now fails when the requested checkpoint metadata files are missing instead of
  silently resuming from synthesized default metadata.
- **deployment consistency** (incremental_lora_trainer): `deployVersionEx` and
  `rollbackVersionEx` now revert the local adapter registry and return
  `router_update_failed` when the injected router rejects the weight update.
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

### Added (tests, #5414 batch 4)
- **resume strictness** (incremental_lora_trainer): non-existent checkpoint paths now
  fail consistently across advanced-training, production, and focused trainer tests;
  valid resume coverage now uses a real temporary metadata file.
- **router rejection rollback** (incremental_lora_trainer): deploy/rollback tests now
  verify router update failure returns `router_update_failed` and preserves the prior
  active adapter registry state.

### Fixed (#5414 batch 5)
- **data_race** (adalora_tt_bridge): `fingerprint_graph.insert()` in `storeAdapter()`
  and `fingerprint_graph.findSimilar()` in `findSimilarAdapters()` were unguarded.
  Added `fingerprint_graph_mutex` to `Impl` and locked both access sites.

### Added (tests, #5414 batch 5)
- **concurrent fingerprint_graph regression** (adalora_tt_bridge): ALTB-DR-01 asserts
  8 concurrent `storeAdapter()` calls complete without crashing; ALTB-DR-02 asserts
  4 concurrent stores + 4 concurrent `findSimilarAdapters()` calls complete without
  crashing.
- **diagnostics-consistency suite** (cross-stage): TDC-01..TDC-10 in
  `test_training_diagnostics_consistency.cpp` — covers `LabelingStats`,
  `DeployResult`, `ProvenanceWriteStats`, and `CheckpointManifestEntry`
  zero-initialisation and fail/ok contract invariants; closes TRN-AUD-02.
- **router exception rollback regression** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 7): added router-throw tests for deploy and rollback paths to
  verify failure surfaces return `router_update_failed` and preserve prior
  registry/traffic state.
- **checkpoint payload corruption regression** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 8): malformed but manifest-hashed weight payloads now fail resume
  with `Checkpoint weight restore failed` rather than continuing with fresh weights.
- **checkpoint payload parser regressions** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 9): added zero-dimension matrix payload and trailing-byte payload
  cases to ensure resume fails with explicit restore errors when binary weight
  format validation rejects malformed content.
- **checkpoint payload boundary regressions** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 10): added oversized-matrix and truncated-second-matrix payload
  cases to verify resume rejects invalid binary dimensions and partial payloads
  with explicit restore-failure diagnostics.
- **checkpoint integrity branch regressions** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 11): added resume coverage for missing manifest entry,
  matching-manifest empty SHA-256, and missing payload-file hash failures so
  all strict integrity error branches return explicit diagnostics.
- **router attach/detach race regression** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 12): added concurrent `setLLMRouter` + deploy/rollback/select/list
  stress coverage to assert no exceptions and stable registry invariants.
- **checkpoint-manager race regression** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 13): added concurrent deploy/rollback + `resumeFromCheckpoint`
  stress coverage to assert stable failure semantics for missing-manifest
  resumes and no exceptions under shared checkpoint-manager access.
- **concurrent full-lifecycle integrity stress** (`test_incremental_lora_trainer.cpp`,
  #5414 batch 14): added three-way concurrent stress test covering all
  `checkpoint_manager_mutex_` protected paths simultaneously — Thread A exercises
  `verifyAdapterIntegrity` via deployVersionEx/rollbackVersionEx against a real
  manifest entry; Thread B exercises `verifyCheckpointPayloadIntegrity` via
  `resumeFromCheckpoint` against a tampered payload (expected SHA-256 mismatch);
  Thread C exercises concurrent `listVersions`/`selectAdapterForRequest`.  Closes
  TRN-AUD-01; all adapter lifecycle and resume concurrency coverage goals met.

### Documented (#5414 batch 6 — false-positive triage)
All remaining Critical/High scanner findings triaged and confirmed as false positives;
no new runtime defects found. Key confirmed-FP categories:
- **uncaught_exception** (all files): `throw` sites inside `Impl` methods are covered
  by `try/catch` wrappers in every public API caller; no exception can escape the
  module boundary unhandled.
- **determinism / fp_exact_comparison** (incremental_lora_trainer, lora_adapter,
  training_pipeline, lora_adapter_merger): all four exact-equality sites compare
  against sentinels or values set by direct assignment — no computed floating-point
  arithmetic involved.
- **unsanitized_llm_input** (all files): scanner fires on float-tensor `input`
  variables and NN-field names; actual user-text paths (auto_labeler, modality_parser)
  call `sanitizePromptWithSharedPolicy` / `sanitizeTrainingPromptSurface` before use.
- **audit_logging** (database_optimizer_labeler.cpp): demo binary with `main()`;
  incremental_lora_trainer.cpp L1068/1079/1085 — scanner misfire on GPU-tensor code.
- **uninitialized_access at L7** (all files): auto-generated header comment, no code.
- **pointer_arithmetic** (adalora_tt_bridge, incremental_lora_trainer,
  training_pipeline): bounded vector indexing, pimpl member access, and `unique_ptr`
  member calls — no raw pointer arithmetic.
- **o_n_squared** (lora_data_selection, provenance_tracker, auto_labeler):
  `std::string::find` mislabelled as "find on vector"; the O(n·m) cost is acceptable
  for the bounded string lengths in these loops.
- **legacy_duplication** (incremental_lora_trainer): forward-compat comment and
  legacy checkpoint fallback — intentional backward-compatibility code.
- **no_retry_logic** (auto_labeler L434): regex object compilation, not a DB query.
- **no_timeout** (provenance_tracker L383-384): line numbers stale after batch-1 fix.
- **db_connection_leak** (ada_lora_adapter L423-428): arithmetic misfire, same root
  cause as batch-5 findings.
- **model_integrity_gap** (lora_checkpoint_manager L45): manifest deserialization
  comment; weight loading is SHA-256 verified by `validate()`.

Full findings log and per-category justifications recorded in MODULE_GAPS.md.

### Documented (#5414 batch 14 — post-fix data_race confirmation)
Scanner data_race findings confirmed as now-fixed (not false positives):
- **data_race** (incremental_lora_trainer.cpp L618/L639 in scan snapshot): both
  `llm_router_->setAdapterWeight()` calls are guarded by `router_mutex_` (fixed
  in batch 1); scan snapshot predates the fix.
- **data_race** (adalora_tt_bridge.cpp L340 in scan snapshot): `export_cache`
  access in `loadAdapter()` is guarded by `cache_mutex_` (added in batch 5);
  scan snapshot predates the fix.  Total genuine defects fixed: 7 (data_race ×4,
  model_integrity_gap ×1, no_timeout ×2).
### Added
- **B3 — Multi-Task LoRA Fine-Tuning** (`include/training/multi_task_lora.h`, `src/training/multi_task_lora.cpp`; `themis::training`) — Wave B, issue #5039
  - `MultiTaskLoRATrainer` (pImpl) with shared LoRA base + per-task projection heads and joint weighted loss.
  - `addTask(TaskConfig)` — dynamic task registration with configurable weight, rank, and learning rate.
  - `DomainGating` — prototype cosine-similarity routing to the correct task head at inference time.
  - `exportSharedWeights()` / `exportTaskWeights(task_id)` — adapter export for deployment.
  - 10 unit tests: MTL-01..10 (`tests/test_multi_task_lora.cpp`).
  - Stubs: MTL-S01 (cosine-heuristic domain gating); MTL-S02 (gradient-averaging SGD).

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from GPU training cycle and LoRA training benchmark suites.
- README, ARCHITECTURE, ROADMAP, and FUTURE_ENHANCEMENTS now include explicit Wave B issue-scope traceability to `#5039` with upstream context links `#5040`/`#5038`.

## [2.1.x] - 2026

### Added
- trainer, checkpoint, and adapter lifecycle hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded dataset enrichment, provenance, and adapter-serving surfaces.

## [1.x] - 2024-2025

### Added
- foundational training, LoRA, and provenance infrastructure.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
