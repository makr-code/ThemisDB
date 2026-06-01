> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Training Module

All notable changes to the training module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

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
