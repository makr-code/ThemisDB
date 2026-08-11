# ThemisDB Training Module

**Status:** PRODUCTION_CANDIDATE  
**Phase:** 6 (Documentation & Acceptance) — ✅ COMPLETE  
**Last Updated:** 2026-08-10  
**Owner:** ML Training Team

---

## Module Purpose

The training module provides dataset labeling, LoRA and AdaLoRA training support, training-pipeline orchestration, provenance tracking, and graph-enriched training context behavior for ThemisDB. Phase 1-6 complete with all release gates validated.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| auto_labeler.cpp | automatic training-label generation behavior |
| database_domain_auto_labeler.cpp | database-domain labeling behavior |
| incremental_lora_trainer.cpp | incremental LoRA training behavior |
| training_pipeline.cpp | end-to-end training pipeline behavior |
| knowledge_graph_enricher.cpp | graph-based training context enrichment behavior |
| lora_adapter.cpp | LoRA adapter core behavior |
| ada_lora_adapter.cpp | AdaLoRA adaptive-rank behavior |
| lora_adapter_merger.cpp | LoRA merge behavior |
| lora_checkpoint_manager.cpp | checkpoint lifecycle behavior |
| lora_data_selection.cpp | training data selection behavior |
| modality_parser.cpp | modality parsing behavior |
| provenance_tracker.cpp | provenance and lineage behavior |
| adalora_tt_bridge.cpp | AdaLoRA-TT bridge behavior |
| adapter_serving.cpp | adapter serving handoff behavior |

## Scope

In scope:
- training-sample generation and enrichment behavior
- LoRA/AdaLoRA training, checkpoint, merge, and serving-handoff behavior
- provenance, modality parsing, and training pipeline orchestration

Out of scope:
- standalone inference serving outside training handoff boundaries
- non-training storage and execution behavior owned by other modules

## Runtime Behavior and Limits

- labeling and enrichment behavior expose explicit outputs and skip/fail semantics.
- LoRA training and checkpoint behavior remain bounded by configured runtime constraints.
- provenance and selection paths remain deterministic and diagnosable.
- advanced GPU-backed training paths depend on runtime capability and build configuration.

## Sourcecode Verification (Module: training/readme)

- Verified files:
  - src/training/auto_labeler.cpp
  - src/training/database_domain_auto_labeler.cpp
  - src/training/incremental_lora_trainer.cpp
  - src/training/training_pipeline.cpp
  - src/training/knowledge_graph_enricher.cpp
  - src/training/lora_adapter.cpp
  - src/training/ada_lora_adapter.cpp
  - src/training/lora_adapter_merger.cpp
  - src/training/lora_checkpoint_manager.cpp
  - src/training/lora_data_selection.cpp
  - src/training/modality_parser.cpp
  - src/training/provenance_tracker.cpp
  - src/training/adalora_tt_bridge.cpp
  - src/training/adapter_serving.cpp
- Verified behavior surfaces:
  - labeling/training/checkpoint/enrichment/provenance and adapter-serving paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - Wave B dependency planning for training enhancements is tracked in issue `#5039`
  - upstream planning context links: Wave C `#5040`, Wave A `#5038`
  - historical entries remain in CHANGELOG.md
  - historical entries remain in CHANGELOG.md
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
