<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/training/ -->

# Architecture — include/training/

This document describes the public-header architecture of the **training** module.

---

## Overview

The `training` module provides a **LoRA (Low-Rank Adaptation) fine-tuning pipeline**
for large language models integrated with ThemisDB. It is responsible for:

- **Pipeline Orchestration** — end-to-end training run management with checkpointing, resumption, and graceful cancellation.
- **LoRA Adapters** — parameter-efficient fine-tuning via rank-decomposed weight matrices stored as versioned ThemisDB artifacts.
- **Incremental / Online Training** — continuous learning from new data streams without full retraining.
- **Auto-Labeling** — automatic label derivation from ThemisDB query results.
- **Multimodal Input** — parsing of text, tabular, and structured inputs into a unified training representation.
- **Knowledge Graph Enrichment** — augmenting training samples with entity relationships from ThemisDB graph indices.
- **Provenance Tracking** — full data lineage from raw source to trained model weight.
- **Data Selection** — active-learning and uncertainty-sampling strategies for curation.

---

## Design Principles

- **Reproducibility by construction** — every training run is assigned a deterministic seed chain; `ProvenanceTracker` records the exact data snapshot, hyperparameters, and adapter version used.
- **Incremental-first** — `IncrementalLoRATrainer` is the primary interface; full retraining is a special case with `num_previous_steps = 0`.
- **Adapter isolation** — `LoRAAdapter` objects are value types; merging into a base model is an explicit, audited operation.
- **Fail-safe checkpointing** — `LoRACheckpointManager` writes atomically (write-to-temp + rename); a crash mid-checkpoint never corrupts the previous save.
- **Modality agnosticism** — `ModalityParser` normalises all input types to `TrainingSample` so downstream stages are modality-unaware.

---

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `training_interfaces.h` | `ITrainer`, `IAdapter`, `IDataSource` | Abstract base contracts for all training components |
| `training_pipeline.h` | `TrainingPipeline`, `PipelineConfig` | End-to-end run orchestration and lifecycle |
| `lora_adapter.h` | `LoRAAdapter`, `LoRAConfig` | LoRA rank-decomposed weight adapter value type |
| `incremental_lora_trainer.h` | `IncrementalLoRATrainer`, `IncrementalConfig` | Online/incremental LoRA training from data streams |
| `lora_checkpoint_manager.h` | `LoRACheckpointManager`, `CheckpointMeta` | Atomic checkpoint save/load with versioning |
| `lora_data_selection.h` | `DataSelector`, `SelectionStrategy` | Active-learning and uncertainty-sampling data curation |
| `auto_labeler.h` | `AutoLabeler`, `LabelSpec` | Automatic label derivation from ThemisDB query results |
| `modality_parser.h` | `ModalityParser`, `TrainingSample` | Normalisation of text/tabular/structured inputs |
| `knowledge_graph_enricher.h` | `KGEnricher`, `EnrichmentSpec` | Entity-relationship augmentation of training samples |
| `provenance_tracker.h` | `ProvenanceTracker`, `ProvenanceRecord` | Data lineage and reproducibility metadata |
| *(planned)* `training_interfaces.h` | `EncryptedGradient`, `GlobalAdapterDelta` | Federation bridge structs for cross-shard LoRA gradient exchange (IMPL-A3) |

> **Paper 1 additions (IMPL-A1, IMPL-A3):**
> - `DatabaseDomainAutoLabeler` (IMPL-A1): new labeler class in `auto_labeler.h` for `DomainType::DATABASE_OPTIMIZER`; labels `(query, explain_plan, Δlatency_ms)` triples with confidence `tanh(|Δlatency|/50)`
> - `IncrementalLoRATrainer::exportGradient()` / `applyGlobalDelta()` (IMPL-A3): federation bridge methods; structs defined in `training_interfaces.h`

---

## Component Diagram

```
+------------------------------------------------------------------+
|                        TrainingPipeline                          |
|                                                                  |
|  +--------------+  +--------------+  +----------------------+   |
|  | ModalityParser|  | AutoLabeler  |  | KGEnricher           |   |
|  | (text/tabular)|  | (ThemisDB)   |  | (graph indices)      |   |
|  +------+-------+  +------+-------+  +----------+-----------+   |
|         +------------------+                     |               |
|                    +--------+--------------------+               |
|                    |    DataSelector (active learning)           |
|                    +--------+----------------------------------   |
|                             |                                    |
|             +---------------+--------------+                     |
|             |    IncrementalLoRATrainer     |                     |
|             |    (ITrainer interface)       |                     |
|             +---------------+--------------+                     |
|                             |                                    |
|                  +----------+----------+                         |
|                  |    LoRAAdapter       |<-- LoRACheckpointMgr   |
|                  |    (IAdapter)        |                         |
|                  +---------------------+                         |
|                                                                  |
|  ProvenanceTracker  ------>  (records all stages)                |
+------------------------------------------------------------------+
```

---

## Related Documents

- `README.md` — module overview and quick-start
- `ROADMAP.md` — planned enhancements and milestones
- `FUTURE_ENHANCEMENTS.md` — design sketches for future capabilities
- `SECURITY.md` — threat model and data-handling controls
- `AUDIT.md` — header audit results

---

> Implementation in `../../src/training/`
