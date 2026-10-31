---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# LoRA Training Data-Path Benchmarks

## Scope

Add reproducible benchmarks for each supported way training samples reach LoRA training, including DB-backed collection and AQL paths. Do not report synthetic fallback work as DB-backed training.

## Current Source Evidence

- `AQLDatasetBuilder`: built-in samples, custom in-memory samples, JSON file/object import.
- `LegalAutoLabeler`: can fetch source documents through an injected `QueryEngine`; target persistence is currently a no-op.
- `IncrementalLoRATrainer`: configured collection is not read; `training_data` remains empty and the trainer generates deterministic synthetic inputs.
- `TrainingDataIterator`: declarations describe AQL batches, but its implementation currently only defines constructors.
- `TrainingPipeline`: constructs the labeler without a `QueryEngine`; data-selection candidates are empty.
- `LoRATrainingService`: accepts prepared `TrainingData`; requires an existing GGUF model for its real tokenizer path.

## Acceptance Criteria

- [ ] DB reads use a real `QueryEngine` backed by an isolated RocksDB fixture; records include input/output fields and deterministic sample IDs.
- [ ] Empty DB, malformed sample, query failure, collection identifier validation, batch limit, and cancellation/error behavior are covered by focused tests.
- [ ] Incremental training reports the number of DB samples actually consumed; a configured DB collection never silently becomes a synthetic-data benchmark.
- [ ] AQL iterator returns stable bounded batches and reports processed/skipped counts.
- [ ] Labeling writes generated samples to the configured target collection before the trainer reads them.
- [ ] One Google Benchmark case per path reports DB/read time, sample throughput, training-step time, and end-to-end time as separate counters.
- [ ] Built-in, custom/in-memory, JSON, direct-DB, AQL-iterator, and pipeline-DB cases have distinct names and result JSONs.
- [ ] DB benchmark fixtures are isolated and cleaned up; no external model is required for data-path-only measurements. Real model-backed fine-tuning is separately preflighted and explicitly skipped when its GGUF artifact is unavailable.
- [ ] The focused training tests and all new benchmark targets build; every benchmark has a `tier:benchmark` CTest smoke.
- [ ] `src/training/PERFORMANCE_EXPECTATIONS.md`, module architecture, roadmap, future enhancements, and changelog describe the new behavior and gates.

## Implementation Sequence

1. Add an internal, bounded DB sample loader using the existing `QueryEngine`/collection contract.
2. Implement the declared `TrainingDataIterator` AQL initialization, filtering, batching, and statistics.
3. Inject `QueryEngine` into `IncrementalLoRATrainer` and `TrainingPipeline` without breaking existing constructor call sites.
4. Persist auto-labeled samples and make pipeline training consume the target collection; populate data-selection candidates from the same DB source.
5. Add focused DB fixture tests for each stage and full-path result accounting.
6. Add a dedicated benchmark target with per-path counters and CTest smoke registration.
7. Build and run the focused targets, then run the benchmark smoke and validate all emitted JSON/report artifacts.
