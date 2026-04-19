> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Training Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.6.0] — 2026-03-24
### Added
- **AdaLoRA** (`include/training/ada_lora_adapter.h`, `src/training/ada_lora_adapter.cpp`):
  Importance-based adaptive rank allocation.  `updateImportance()` estimates
  per-rank-component importance as the product of B and A column/row norms;
  `reallocateRanks()` redistributes a global budget proportionally.
  Forward pass applies only the active (unpruned) rank components.
  36 unit tests in `tests/test_ada_lora_adapter.cpp`. CMake target: `AdaLoRAFocusedTests`.
- **LoRAAdapterMerger** (`include/training/lora_adapter_merger.h`, `src/training/lora_adapter_merger.cpp`):
  Two merge strategies:
  - `mergeLinear()` / `mergeLinearAll()`: weighted sum of ΔW, re-factorised to (B', A').
  - `mergeTIES()` / `mergeTIESAll()`: Trim–Resolve–Merge per Yadav et al. (2023).
  32 unit tests in `tests/test_lora_adapter_merger.cpp`. CMake target: `LoRAMergerFocusedTests`.
- **LoRA+** (`IncrementalTrainingConfig::lora_plus_lambda` field):
  When `lora_plus_lambda > 1.0`, `IncrementalLoRATrainer` uses two separate
  `AdamOptimizer` instances: B matrices use `lr * λ`, A matrices use `lr`
  (Hayou et al., 2024).  Default `1.0` preserves backward compatibility.

### Changed
- `IncrementalTrainingConfig` gains `lora_plus_lambda` field (float, default 1.0).
- `IncrementalLoRATrainer::Impl` adds `optimizer_B_plus_`, `optimizer_A_plus_`,
  `using_lora_plus_` members; `setHyperparameters()` resets them on reconfiguration.

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
