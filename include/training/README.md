---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-14"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Training — Public C++ API

This directory declares the public contracts for training data preparation,
LoRA/AdaLoRA adapters, checkpointing, provenance, and serving handoff. The
declarations and their Doxygen comments are authoritative; this page is an
orientation, not a separate API contract.

## Main interfaces

| Header | Public types and responsibility |
|---|---|
| `training_pipeline.h` | `TrainingPipeline`, `PipelineConfig`, stage/result types |
| `auto_labeler.h` | `LegalAutoLabeler`, `AutoLabelConfig`, labeled samples |
| `database_domain_auto_labeler.h` | `DatabaseDomainAutoLabeler` |
| `modality_parser.h` | `ModalityDetector` and text, table, citation, and OCR extractors |
| `knowledge_graph_enricher.h` | `KnowledgeGraphEnricher` and graph/vector enrichment |
| `lora_data_selection.h` | `DataSelectionPipeline`, `DataSample`, selection configuration/results |
| `incremental_lora_trainer.h` | `IncrementalLoRATrainer`, `IncrementalTrainingConfig`, training results |
| `lora_adapter.h`, `ada_lora_adapter.h` | `LoRAAdapter`, `AdaLoRAAdapter` and their configurations |
| `lora_adapter_merger.h`, `multi_task_lora.h` | Adapter merge and multi-task training |
| `lora_checkpoint_manager.h` | `LoRACheckpointManager` and checkpoint metadata/configuration |
| `training_sample_provider.h` | Source-neutral `TrainingSampleRecord` and bounded `ITrainingSampleProvider` contract |
| `docs_db_training_sample_provider.h` | Read-only RocksDB provider for full docs chunks with legacy-preview fallback |
| `adalora_training_pipeline.h` | AdaLoRA consumer for AQL or injected sample providers |
| `provenance_tracker.h` | `ProvenanceTracker` and sample lineage |
| `adapter_serving.h` | `ILLMRouter`, `DeployResult`, deployment validation helpers |
| `training_interfaces.h` | Checkpoint, provenance, enrichment, calibration, pipeline, and lineage interfaces |
| `training_error_codes.h`, `training_error_diagnostics.h`, `training_exceptions.h`, `training_failsafe.h`, `training_incident_emitter.h` | Error, diagnostics, recovery, and incident contracts |

All module interfaces are in `themis::training`.

## Runtime and integration boundaries

- `TrainingPipeline` can run labeling, enrichment, quality/drift checks, data
  selection, and LoRA training according to `PipelineConfig`.
- With DB selection enabled, selected IDs are filtered before the trainer's
  result limit; duplicate, missing, or invalid selected records fail the run
  instead of falling back to collection-wide or synthetic samples.
- Database-backed components require their query/database dependencies to be
  injected. An offline/test mode is not evidence of a successful database read.
- AdaLoRA separates providers from the model consumer. The AQL provider can read
  any authorized collection supported by `TrainingDataIterator`; the concrete
  `DocsDbTrainingSampleProvider` is a read-only adapter for generated
  documentation databases. `trainFromProvider()` applies the same bounded,
  model-backed AdaLoRA consumer to either source.
- Docs databases prefer complete `chunk_` records and use `doc_` preview records
  only for older database formats. The caller must authorize the source before
  training. Production authorization and cryptographic adapter signing remain
  open requirements; this API does not claim either control is complete.
- `KnowledgeGraphEnricher` vector search uses an initialized, caller-owned
  `VectorIndexManager`; the caller must keep it alive.
- Adapter deployment and rollback use the injected `ILLMRouter`. Check the
  returned `DeployResult`; a successful training result alone does not mean that
  the adapter is serving.
- `AdaLoraTTBridge` stores serialized exports in `TensorNetworkStorageEngine`
  metadata and reloads them on a cache miss. Restart durability depends on the
  configured backend; its fingerprint index is in-process and `mapAdapter()` is
  an injection hook, not a proven serving handoff or latency guarantee.
- The data-selection clustering implementation is a lightweight
  character-statistic heuristic, not a call to the configured embedding model.
- The current OCR path does not provide recognized image text. See
  [`../../src/training/MODELING.md`](../../src/training/MODELING.md) and
  [`../../src/training/EMBEDDING.md`](../../src/training/EMBEDDING.md) for
  implementation limits.

## Build and tests

The training tests are registered from
[`../../tests/training/CMakeLists.txt`](../../tests/training/CMakeLists.txt) as
focused targets with the `training` label and a 120-second timeout. For
configuration and focused-test commands, see
[`../../src/training/MODULE_GUIDE.md`](../../src/training/MODULE_GUIDE.md).

## Related documentation

- [`../../src/training/API.md`](../../src/training/API.md) — public API index.
- [`../../src/training/ARCHITECTURE.md`](../../src/training/ARCHITECTURE.md) —
  implementation boundaries and data flow.
- [`../../src/training/ROADMAP.md`](../../src/training/ROADMAP.md) — current
  implementation work and acceptance state.
- [`TRAINING_CONTRACT.md`](./TRAINING_CONTRACT.md) — lifecycle and error-code
  contract summary.
