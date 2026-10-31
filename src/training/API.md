---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Training API Reference

## Contract ownership

This page is an index to the public C++ API, not a duplicate declaration of
header contracts. The headers in [`include/training`](../../include/training/)
and their Doxygen comments are authoritative. Public types use the
`themis::training` namespace.

## Main interfaces

| Header | Primary interface | Inputs and outputs |
|---|---|---|
| `training_pipeline.h` | `TrainingPipeline` | `PipelineConfig`; full or individual stages return stats/results |
| `auto_labeler.h` | `LegalAutoLabeler` | source/target collections and label thresholds; returns `TrainingSample` values and `LabelingStats` |
| `modality_parser.h` | `ModalityDetector` | content, document ID, optional MIME hint; returns typed samples and parse statistics |
| `knowledge_graph_enricher.h` | `KnowledgeGraphEnricher` | sample/document IDs and enrichment settings; returns graph context or similar-document scores |
| `lora_data_selection.h` | `DataSelectionPipeline` | `DataSample` values and selection configuration; returns selected samples and audit metadata |
| `incremental_lora_trainer.h` | `IncrementalLoRATrainer` | training configuration and mode; returns metrics, adapter version, and explicit error text |
| `lora_checkpoint_manager.h` | `LoRACheckpointManager` | checkpoint directory and file/manifest metadata; returns checkpoint entries or `std::nullopt` |
| `provenance_tracker.h` | `ProvenanceTracker` | provenance records and optional query engine; returns write stats and lineage |
| `adapter_serving.h` | `ILLMRouter`, deployment helpers | adapter version and traffic weight; returns readiness/fingerprint or router result |

## Integration requirements

- Configure non-empty source/target/training collections where required; the
  pipeline rejects empty target collection and empty required selection
  language before execution.
- A database-backed labeler or provenance tracker needs its query engine wired
  in. A null engine is explicitly documented as offline/test behavior and does
  not fetch documents from the database.
- Semantic similarity requires an initialized `VectorIndexManager` injected
  into `KnowledgeGraphEnricher`; the caller retains ownership. See
  [`EMBEDDING.md`](./EMBEDDING.md).
- An injected `ILLMRouter` is non-owning and must remain alive while the
  trainer may deploy or roll back versions.
- Checkpoint validation, retention, automatic fallback, and I/O timeout are
  controlled by `CheckpointManagerConfig`; choose settings deliberately for
  deployment requirements.

## Result and error handling

- Inspect result success flags and error fields; do not infer success from a
  non-throwing call alone.
- `TrainingPipeline::run()` returns `PipelineStats`, including stage counts,
  training outcome, timing, data-quality findings, selection counts, and
  provenance counts.
- `LoRACheckpointManager::resume()` may return `std::nullopt`; use
  `resumeWithDiagnostics()` to see recovery attempts.
- `deployVersionEx()` and `rollbackVersionEx()` return a `DeployResult` with
  explicit reasons such as `invalid_split`, `integrity_failure`,
  `router_unavailable`, or `router_update_failed`.
- Callbacks can execute during stage work. The pipeline and related extraction
  paths sanitize callback/progress messages; preserve this contract when
  adding new output paths.

## Compatibility

`include/training/TRAINING_CONTRACT.md` documents the current v2.x lifecycle
and error-code compatibility expectations. An API or error-code change must
update that contract, relevant Doxygen comments, tests, and the module
changelog as required by
[`DOCUMENTATION_GOVERNANCE.md`](../../DOCUMENTATION_GOVERNANCE.md).

## Source references

- [`include/training/ARCHITECTURE.md`](../../include/training/ARCHITECTURE.md)
- [`include/training/TRAINING_CONTRACT.md`](../../include/training/TRAINING_CONTRACT.md)
- [`training_pipeline.h`](../../include/training/training_pipeline.h)
- [`incremental_lora_trainer.h`](../../include/training/incremental_lora_trainer.h)
- [`lora_checkpoint_manager.h`](../../include/training/lora_checkpoint_manager.h)

Related: [`IMPLEMENTATION.md`](./IMPLEMENTATION.md),
[`MODULE_GUIDE.md`](./MODULE_GUIDE.md), and
[`RUNBOOK.md`](./RUNBOOK.md).
