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

# Architecture - Training Module

<!-- Status: current | source paths and dataflow reviewed: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The training module composes auto-labeling and enrichment behavior, LoRA/AdaLoRA training lifecycle behavior, checkpoint/provenance handling, and training-pipeline orchestration into a bounded subsystem.

## Main Execution Planes

1. Dataset preparation plane
- auto-labeling, domain labeling, modality parsing, and graph enrichment behavior

2. Adapter training plane
- LoRA/AdaLoRA adapter, merge, checkpoint, and serving-handoff behavior

3. Governance and orchestration plane
- provenance tracking, data selection, and end-to-end training pipeline behavior

## Training Data Paths

- Prepared input paths include in-memory/custom inputs and configured collections.
- DB-backed paths: `LegalAutoLabeler` reads a source collection through an
  injected `QueryEngine`; the pipeline can persist generated samples to the
  configured target collection; `IncrementalLoRATrainer` can read bounded
  `input`/`output` records from its configured training collection. Invalid
  collection identifiers, query errors, invalid result shapes, and empty valid
  sample sets fail closed.
- The focused RocksDB pipeline test verifies persisted labels are consumed by
  training when selection is disabled. The pipeline's selection stage currently
  records the selected subset and counts but does not pass those samples to the
  trainer; the trainer reads its configured collection separately.
- `TrainingDataIterator` is implemented in the `llm` module and has its own AQL
  batch-reading path. Its behavior and integration are not owned by
  `src/training/`.
- Trainer/pipeline DB paths have focused tests, but per-input-route benchmarks
  and broader integration acceptance remain open. Offline/test behavior is not
  evidence of a live database read.

## Core Contracts

| Contract | Behavior |
|---|---|
| dataset contract | deterministic sample extraction and enrichment behavior |
| training contract | explicit LoRA training, checkpoint, and resume semantics |
| adapter contract | bounded adapter merge and serving-handoff behavior |
| governance contract | explicit provenance and selection lifecycle behavior |

## Failure Semantics

- invalid training or labeling inputs surface explicit failures.
- checkpoint and adapter lifecycle faults remain diagnosable and non-silent.
- enrichment and provenance failures remain observable.
- capability-gated GPU paths degrade explicitly by runtime configuration.

## Sourcecode Verification (Module: training/architecture)

- Verified files:
  - src/training/auto_labeler.cpp
  - src/training/incremental_lora_trainer.cpp
  - src/training/knowledge_graph_enricher.cpp
  - src/training/lora_adapter.cpp
  - src/training/ada_lora_adapter.cpp
  - src/training/lora_checkpoint_manager.cpp
  - src/training/lora_adapter_merger.cpp
  - src/training/provenance_tracker.cpp
  - src/training/training_pipeline.cpp
- Verified architecture claims:
  - dataset + adapter-training + governance/orchestration plane split
  - explicit failure boundaries for training, checkpoint, and enrichment faults
  - module-local ownership of training behavior

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| llm | `include/llm/` | LLM inference layer provides model interfaces for adapter training and serving handoff |
| rag | `include/rag/` | RAG module provides document/entity data feeds for auto-labeling and graph enrichment |
| index | `include/index/` | Index structures used for embedding lookups during data-selection and evaluation |
| storage | `include/storage/` | Persists training checkpoints and adapter artifacts |
| observability (utils) | `include/utils/tracing.h`, `audit_logger.h` | Training pipeline audit and telemetry |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| rag | `include/training/lora_data_selection.h` | `ContinuousLearningOrchestrator` composes `DataSelectionPipeline` for RAG retraining; see `include/rag/continuous_learning_orchestrator.h` and `src/rag/continuous_learning_orchestrator.cpp` |

## Integration Points

### Critical Integration: LLM Adapter Serving Handoff
**Files:** `src/training/incremental_lora_trainer.cpp`, `include/training/adapter_serving.h` ↔ the injected `ILLMRouter`.
**Contract:** `deployVersionEx()` and `rollbackVersionEx()` return a `DeployResult`; the caller checks the result and confirms the external router state. Router availability and the traffic-weight update are separate from training success.
**Thread Safety:** The trainer protects router access and serializes weight updates with `router_mutex_`. Atomic request-level routing behavior is the injected router implementation's contract, not an adapter-object swap performed by this module.

### Critical Integration: RAG LoRA Data Selection
**Files:** `src/rag/continuous_learning_orchestrator.cpp` ↔ `include/training/lora_data_selection.h`
**Contract:** The RAG continuous-learning orchestrator owns and calls `DataSelectionPipeline` to select candidate samples before retraining. This is separate from `TrainingPipeline`'s selection stage; that stage currently records results but does not pass its selected subset directly to its trainer.
**Thread Safety:** Ownership and synchronization are defined by the RAG orchestrator and training selection interfaces; do not assume there is no shared mutable state without checking those contracts.

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`
- Note:
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
