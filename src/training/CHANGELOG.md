<!-- Status: current | validated: 2026-03-12 -->
# Changelog — Training Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- QLoRA quantization: INT8 and NF4 precision paths via `QLoRALayer`
- `using_qlora_` flag in `IncrementalLoRATrainer::Impl` for INT8/NF4 dispatch
- QLoRA checkpoint: `get_lora_weights()` / `set_lora_weights()` for adapter persistence
- 32 tests in `test_advanced_training_features.cpp` (28 base + 4 QLoRA)
- `LegalAutoLabeler` for domain-specific annotation
- `KnowledgeGraphEnricher` for graph-context enrichment during training

### Changed
- INT8/NF4 paths use `QLoRALayer`; NONE/FP16 paths use `LoRALayer`; both share `AdamOptimizer`

## [1.0.0] — 2024-01-01
### Added
- `IncrementalLoRATrainer` with checkpoint/resume
- LoRA adapter versioning
- AdamW optimizer with configurable learning rate schedules
