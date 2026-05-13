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
| `adalora_tt_bridge.h` | `AdaLoraTTBridge`, `AdaLoraTTBridgeConfig` | Conversion bridge between AdaLoRA adapters and tensor-train cores |
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

## Public Entry Points

- `LegalAutoLabeler` (`auto_labeler.h`) — DB-backed or offline labeling (`labelAll`, `labelQuery`, `labelDocument`)
- `IncrementalLoRATrainer` (`incremental_lora_trainer.h`) — train/resume/evaluate/deploy/rollback workflow
- `KnowledgeGraphEnricher` (`knowledge_graph_enricher.h`) — graph + vector context enrichment for samples
- `TrainingPipeline` (`training_pipeline.h`) — orchestration layer for label → enrich → train
- `DatabaseDomainAutoLabeler` (`database_domain_auto_labeler.h`) — optimizer-domain dataset construction

## Configuration Options (Selected)

- `AutoLabelConfig` (`auto_labeler.h`): `source_collection`, `target_collection`, `language_code`, `min_confidence`, `flag_low_confidence`, `domain_type`
- `IncrementalTrainingConfig` (`incremental_lora_trainer.h`): `adapter_version`, `num_epochs`, `batch_size`, `learning_rate`, `rank`, `alpha`, `checkpoint_dir`, `quantization`, `num_gpus`
- `EnrichmentConfig` (`knowledge_graph_enricher.h`): `include_provisions`, `include_case_law`, `include_similar_docs`, `max_related_items`, `similarity_threshold`
- `PipelineConfig` (`training_pipeline.h`): end-to-end pipeline flags and stage configuration

## Runtime Behaviour, Errors, Limits

- `LegalAutoLabeler` with `QueryEngine* == nullptr` runs in offline/test mode and does not fetch DB documents.
- `IncrementalLoRATrainer` validates hyperparameters and throws `std::invalid_argument` / `std::runtime_error` on invalid runtime configuration.
- Checkpoint paths use rotating, integrity-checked writes through `LoRACheckpointManager`.
- Distributed scheduling/orchestration is an external dependency and not provided by this module.
- Production serving orchestration is handled by integration layers outside `training`.
- Adapter inference routing is exposed via `adapter_serving.h` and expected to be wired by the LLM integration runtime (see `src/llm/` plus `../../src/training/README.md` integration section).

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

- [`../../src/training/README.md`](../../src/training/README.md) — implementation overview, runtime details, troubleshooting
- [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md) — design and component/data-flow diagrams
- [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md) — roadmap and implementation phases
- [`../../src/training/FUTURE_ENHANCEMENTS.md`](../../src/training/FUTURE_ENHANCEMENTS.md) — planned enhancements and constraints
- [`../../src/training/SECURITY.md`](../../src/training/SECURITY.md) — threat model and data handling
- [`../../docs/de/training/README.md`](../../docs/de/training/README.md) — German module overview

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

See [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md) and
[`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md) for details.
