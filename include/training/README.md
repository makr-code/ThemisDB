> **Build:** `cmake --preset release && cmake --build build/release`

<!-- Status: current | validated: 2026-04-06 -->

# training — Public Headers

The `training` module provides a **LoRA fine-tuning pipeline** for large language
models, integrated with ThemisDB storage and query infrastructure.

---

## Module Purpose

| Capability | Description |
|---|---|
| LoRA fine-tuning | Parameter-efficient LLM adaptation via rank-decomposed weight matrices |
| Incremental learning | Online training from live ThemisDB data streams |
| Auto-labeling | Label derivation from ThemisDB query results |
| Multimodal input | Unified normalisation of text, tabular, and structured data |
| Knowledge graph enrichment | Augment samples with entity relationships from graph indices |
| Provenance tracking | Full data lineage from source to trained adapter |
| Data selection | Active-learning and uncertainty-sampling for data curation |
| Checkpoint management | Atomic save/resume across training interruptions |

---

## Header Reference

| Header | Key Types | Description |
|---|---|---|
| `ada_lora_adapter.h` | `AdaLoRAAdapter`, `AdaLoRAConfig` | Adaptive LoRA with dynamic rank allocation |
| `adapter_serving.h` | `AdapterServer`, `ServingConfig` | Runtime adapter hot-swap and serving |
| `database_domain_auto_labeler.h` | `DatabaseDomainAutoLabeler` | Domain-specific auto-labeling from DB schema |
| `lora_adapter_merger.h` | `LoRAAdapterMerger`, `MergeConfig` | Merge multiple LoRA adapters into a single model |
| `training_interfaces.h` | `ITrainer`, `IAdapter`, `IDataSource` | Abstract contracts |
| `training_pipeline.h` | `TrainingPipeline`, `PipelineConfig` | End-to-end run orchestration |
| `lora_adapter.h` | `LoRAAdapter`, `LoRAConfig` | LoRA adapter value type |
| `incremental_lora_trainer.h` | `IncrementalLoRATrainer`, `IncrementalConfig` | Online LoRA training |
| `lora_checkpoint_manager.h` | `LoRACheckpointManager`, `CheckpointMeta` | Atomic checkpoint I/O |
| `lora_data_selection.h` | `DataSelector`, `SelectionStrategy` | Training data curation |
| `auto_labeler.h` | `AutoLabeler`, `LabelSpec` | Automatic label derivation |
| `modality_parser.h` | `ModalityParser`, `TrainingSample` | Multimodal input normalisation |
| `knowledge_graph_enricher.h` | `KGEnricher`, `EnrichmentSpec` | Graph-based sample enrichment |
| `provenance_tracker.h` | `ProvenanceTracker`, `ProvenanceRecord` | Data lineage tracking |

---

## Quick-Start

```cpp
#include "training/training_pipeline.h"
#include "training/lora_checkpoint_manager.h"

themis::training::PipelineConfig cfg{
    .base_model_path = "/models/llama-3-8b",
    .lora_config     = { .rank = 16, .alpha = 32 },
    .checkpoint_dir  = "/checkpoints/run-001",
    .max_steps       = 10000,
};

auto pipeline = themis::training::TrainingPipeline::create(cfg);
pipeline->run();

auto mgr     = themis::training::LoRACheckpointManager::open(cfg.checkpoint_dir);
auto adapter = mgr->load_latest();
```

---

## Requirements

- C++17 or later
- ThemisDB core libraries (`themisdb::storage`, `themisdb::graph`)
- Compatible LLM backend — see `../../src/training/README.md`

---

## Related Documents

- `ARCHITECTURE.md` — design and interface inventory
- `ROADMAP.md` — planned features and milestones
- `AUDIT.md` — header audit results
- `SECURITY.md` — threat model and data handling
- `CHANGELOG.md` — version history

---

> Implementation details: `../../src/training/`

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "training/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
