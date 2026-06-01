> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - AI Module

All notable changes to the AI module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Added
- Wave B ML enhancements complete (#5039):
  - B1 Self-RAG: `SelfRAGController` fully integrated with `InferenceEnhancementEngine`; ALCE benchmark simulation tests (ALCE-01..05) added.
  - B2 RotatE KGC: `RotatEModel`, `LinkPredictionHead`, `KGCompletionEngine` with `KnowledgeGraphReasoner` integration; KGC-01..15 tests added.
  - B3 Multi-Task LoRA: `MultiTaskLoRATrainer` ablation and 3-task benchmark tests (MTL-ABL-01..07) added; `multi_task_lora.cpp` registered in modular build.
- Focused CMake targets added for all three Wave B test suites.

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit existing proxy benchmark symbols from plugin-system benchmark sources.

## [1.9.1] - 2026-05-13

### Changed
- Documentation consolidation for phase-1 AI module set completed and link/lint checks passed.

## [1.9.0] - 2026-05-11

### Added
- Initial AI module documentation set in src/ai plus public API reference in include/ai.

## [1.0.0] - 2024-06-01

### Added
- `AIPluginGenerator` base implementation and public prompt/result contracts.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
