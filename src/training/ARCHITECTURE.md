# Architecture - Training Module

<!-- Status: current | validated: 2026-09-09 -->
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
| rag | `include/training/` (lora_data_selection.h) | RAG data-selection step calls training module for LoRA dataset curation |

## Integration Points

### Critical Integration: LLM Adapter Serving Handoff
**Files:** `src/training/lora_adapter.cpp`, `lora_adapter_merger.cpp` ↔ `llm/`
**Contract:** Trained adapters are handed to the LLM module via a standardised serving-handoff interface; adapter format must match the LLM module's expected schema.
**Thread Safety:** Handoff is a one-time atomic swap; no concurrent writes to the adapter object are permitted after handoff.

### Critical Integration: RAG LoRA Data Selection
**Files:** `src/training/training_pipeline.cpp` ↔ `rag/lora_data_selection.h`
**Contract:** RAG calls training data-selection to filter and score training samples; selection outputs are deterministic for the same input dataset snapshot.
**Thread Safety:** Selection runs as an isolated pipeline stage; no shared mutable state with RAG during selection.

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`
- Note:
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
