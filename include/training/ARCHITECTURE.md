> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/training/ARCHITECTURE.md -->

# Training Module — Public Header Architecture

**Module Path:** `include/training/`
**Implementation:** `../../src/training/`
**Canonical architecture doc:** [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md)

---

## 1. Overview

`include/training/` defines the **public LoRA adapter training, knowledge-graph enrichment, data labelling, and model-serving API contract** for ThemisDB. The 15 headers cover LoRA and AdaLoRA adapters, adapter merging, TT-bridge, checkpoint management, incremental and data-selection-driven training, auto-labelling, knowledge-graph enrichment, provenance tracking, adapter serving, and training pipeline interfaces.

For runtime composition — trainer loops, data pipeline scheduling, checkpoint management internals, and serving hot-swap — see:
→ [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 LoRA Adapter Contract

| Header | Public Type | Purpose |
|--------|------------|---------|
| `lora_adapter.h` | `LoRAAdapter` | LoRA adapter definition, configuration, and serialisation |
| `ada_lora_adapter.h` | `AdaLoRAAdapter` | Adaptive LoRA (AdaLoRA) adapter with dynamic rank |
| `adalora_tt_bridge.h` | `AdaLoRATTBridge` | Tensor-Train bridge for AdaLoRA low-rank decomposition |
| `lora_adapter_merger.h` | `LoRAAdapterMerger` | Merge multiple LoRA adapters into a consolidated weight set |
| `lora_checkpoint_manager.h` | `LoRACheckpointManager` | Adapter checkpoint save/restore lifecycle |

### 2.2 Training Pipeline and Data

| Header | Public Type | Purpose |
|--------|------------|---------|
| `incremental_lora_trainer.h` | `IncrementalLoRATrainer` | Incremental online LoRA fine-tuning |
| `lora_data_selection.h` | `LoRADataSelection` | Data-selection strategies for efficient LoRA training |
| `training_pipeline.h` | `TrainingPipeline` | Top-level training pipeline orchestration |
| `training_interfaces.h` | `ITrainer`, `IDataIterator` | Backend-agnostic trainer and data-iterator contracts |
| `modality_parser.h` | `ModalityParser` | Multi-modal input parsing for training data |

### 2.3 Auto-Labelling and Knowledge

| Header | Public Type | Purpose |
|--------|------------|---------|
| `auto_labeler.h` | `AutoLabeler` | LLM-assisted automatic data labelling |
| `database_domain_auto_labeler.h` | `DatabaseDomainAutoLabeler` | Domain-specific auto-labelling for database artefacts |
| `knowledge_graph_enricher.h` | `KnowledgeGraphEnricher` | Training-data enrichment via knowledge-graph lookup |

### 2.4 Provenance and Serving

| Header | Public Type | Purpose |
|--------|------------|---------|
| `provenance_tracker.h` | `ProvenanceTracker` | Training-data and model-version provenance recording |
| `adapter_serving.h` | `AdapterServing` | Adapter registration and hot-serve lifecycle |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::training` | All LoRA training, data-selection, and serving types |

---

## 4. Public Contract Notes

- LoRA adapter headers define stable serialisation and configuration contracts; weight-tensor layout is opaque.
- `ITrainer` and `IDataIterator` in `training_interfaces.h` provide the public extension points for custom training backends and data sources.
- Checkpoint management headers expose stable save/restore contracts; checkpoint format versioning is tracked in `lora_checkpoint_manager.h`.
- Provenance headers define immutable recording contracts; provenance storage backends remain internal.
- Adapter-serving headers integrate with `include/llm/adapter_deployment_manager.h` for hot-swap coordination.
