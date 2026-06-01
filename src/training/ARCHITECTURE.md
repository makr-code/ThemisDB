# Architecture - Training Module

<!-- Status: current | validated: 2026-05-31 -->
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

## Planning Traceability

- Wave B dependency planning issue: `#5039`
- Upstream planning context: Wave C `#5040`, Wave A `#5038`
- Note:
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
