<!-- Status: current | validated: 2026-03-22 -->

# Changelog — include/training/

All notable changes to the **public headers** of the `training` module.
Follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) conventions.

For implementation-level changes see `../../src/training/CHANGELOG.md`.

---

## [Unreleased]

### Planned
- `distributed_lora_trainer.h` — multi-node LoRA training coordinator (Target: Q3 2026)
- `rlhf_reward_model.h` — RLHF reward model interface (Target: Q4 2026)

---

## [1.5.0] — 2026-03-12

### Added
- `knowledge_graph_enricher.h` — `KGEnricher` and `EnrichmentSpec` for graph-based sample enrichment.
- `lora_data_selection.h` — `DataSelector` and `SelectionStrategy` for active-learning data curation.
- `provenance_tracker.h` — `ProvenanceTracker` and `ProvenanceRecord` for full data lineage.
- `IncrementalConfig` gains `warmup_steps` field (default: 0) — backward-compatible.
- `PipelineConfig` gains `max_parallel_enrich_workers` field (default: 4).

### Changed
- `lora_adapter.h` — `LoRAAdapter` is now move-constructible; tensor data is moved during pipeline transfer.
- `training_pipeline.h` — `TrainingPipeline::cancel()` is now `noexcept`.

### Fixed
- `lora_checkpoint_manager.h` — `CheckpointMeta::epoch` widened from 32-bit to `uint64_t`.

---

## [1.4.0] — 2025-12-01

### Added
- `incremental_lora_trainer.h` — `IncrementalLoRATrainer` for online fine-tuning from live data streams.
- `lora_checkpoint_manager.h` — atomic checkpoint save/load with versioned metadata.
- `modality_parser.h` — `ModalityParser` normalises text, tabular, and structured inputs.

### Changed
- `training_interfaces.h` — `IDataSource` gains `estimated_size()` method.

---

## [1.3.0] — 2025-09-15

### Added
- `auto_labeler.h` — `AutoLabeler` with rule-based and embedding-similarity strategies.

### Changed
- `lora_adapter.h` — `LoRAConfig` adds `dropout` field (default: 0.0f).

---

## [1.2.0] — 2025-06-20

### Added
- `training_pipeline.h` — `TrainingPipeline` end-to-end run orchestration.
- `lora_adapter.h` — `LoRAAdapter` value type and `LoRAConfig`.

---

## [1.1.0] — 2025-03-10

### Added
- `training_interfaces.h` — `ITrainer`, `IAdapter`, `IDataSource` abstract contracts.

---

## [1.0.0] — 2024-12-01

### Added
- Initial module skeleton; core interfaces drafted.

---

> Full implementation changelog: `../../src/training/CHANGELOG.md`
