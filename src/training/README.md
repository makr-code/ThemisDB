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

Exact model backend not verifiable from final document text alone.

# Training Module

This module implements training-data preparation, LoRA/AdaLoRA adapters,
incremental training, checkpoint and provenance handling, selection, and
training-pipeline orchestration. Its public contracts are declared in
[`../../include/training/`](../../include/training/). This overview describes
the current implementation; it does not assert release readiness or that every
planned data path is complete.

## Component map

| Component | Responsibility |
|---|---|
| `auto_labeler.cpp`, `database_domain_auto_labeler.cpp` | Create labeled samples from configured inputs |
| `modality_parser.cpp` | Extract text clauses, tables, citations, and modality-tagged samples |
| `knowledge_graph_enricher.cpp` | Add graph context and optionally query an injected vector index |
| `lora_data_selection.cpp` | Filter, de-duplicate, cluster, score, and audit sample selection |
| `incremental_lora_trainer.cpp`, `multi_task_lora.cpp` | Train/update adapters and report training results |
| `lora_adapter.cpp`, `ada_lora_adapter.cpp`, `lora_adapter_merger.cpp` | Adapter representation, adaptive rank, and merge operations |
| `lora_checkpoint_manager.cpp` | Checkpoint persistence, validation, and recovery |
| `provenance_tracker.cpp` | Record sample lineage and provenance |
| `training_pipeline.cpp`, `adapter_serving.cpp` | Coordinate configured stages and adapter-serving handoff |

## Data flow and limits

`TrainingPipeline` runs enabled labeling, enrichment, quality/drift, selection,
and training stages according to `PipelineConfig`. Individual components also
expose their own interfaces; their input types and failure results are defined
in the corresponding public headers.

The trainer supports bounded reads from its configured training collection
through an injected `QueryEngine`; invalid collections, query failures, and
empty valid datasets fail closed. The pipeline can persist labeled samples and
train from that collection; focused RocksDB tests cover both paths. The
pipeline's selection stage currently records selection results but does not
pass the selected subset directly to the trainer, which reads its configured
collection separately. `TrainingDataIterator` is owned by `llm`, not this
module. Broader input-route benchmarks and end-to-end acceptance remain open
in [`ROADMAP.md`](./ROADMAP.md). Selection clustering currently uses a
character-statistic heuristic rather than the configured embedding model, and
the optional OCR path does not return recognized image text. See
[`MODELING.md`](./MODELING.md) and [`EMBEDDING.md`](./EMBEDDING.md).

## Build and focused tests

Training tests under [`../../tests/training/`](../../tests/training/) are
registered as focused targets with the `training` label and a 120-second
timeout. Use the repository's supported CMake preset and build directory, then
build the target for the changed component. See
[`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for example commands and test selection.
Training benchmarks are optional and require `THEMIS_BUILD_BENCHMARKS=ON`; see
[`../../benchmarks/training/README.md`](../../benchmarks/training/README.md).

## Developer references

- [`API.md`](./API.md) — public interface index and integration expectations.
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — module boundaries and data flow.
- [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) — implementation invariants and
  focused validation.
- [`RUNBOOK.md`](./RUNBOOK.md) — training and checkpoint incident diagnosis.
- [`SECURITY.md`](./SECURITY.md) and [`TRUST.md`](./TRUST.md) — controls and
  trust boundaries.
- [`ROADMAP.md`](./ROADMAP.md) and
  [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — current work and
  planned changes.
