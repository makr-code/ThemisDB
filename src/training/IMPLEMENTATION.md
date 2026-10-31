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

# Training Implementation Guide

## Scope

This page is for developers changing the internal implementation in
`src/training/`. Public types and call contracts are in
[`include/training`](../../include/training/). Keep a change within the owning
component unless a contract change requires coordinated updates.

## Component map

| Implementation | Responsibility |
|---|---|
| `auto_labeler.cpp`, `database_domain_auto_labeler.cpp` | Label/sample creation from configured sources |
| `modality_parser.cpp` | Text, table, citation, and optional OCR-tagged sample extraction |
| `knowledge_graph_enricher.cpp` | Graph context and optional vector-index similarity lookup |
| `lora_data_selection.cpp` | Filtering, deduplication, diversity selection, scoring, audit output |
| `incremental_lora_trainer.cpp` | LoRA training lifecycle, metrics, deployment, rollback, federated bridge |
| `lora_adapter.cpp`, `ada_lora_adapter.cpp`, `multi_task_lora.cpp` | Adapter representations and training behavior |
| `lora_adapter_merger.cpp`, `adalora_tt_bridge.cpp` | Adapter merge and tensor-train bridge |
| `lora_checkpoint_manager.cpp` | Checkpoint file, manifest, integrity validation, recovery |
| `provenance_tracker.cpp` | Sample lineage and audit records |
| `training_pipeline.cpp`, `adapter_serving.cpp` | Stage orchestration and serving integration contract |

## Pipeline and invariants

`TrainingPipeline::run()` validates the target collection, drift threshold,
and required selection language before invoking work. It then conditionally
runs labeling, graph enrichment, quality checks, drift detection, data
selection, and LoRA training according to `PipelineConfig`. Callback messages
are sanitized before they are emitted. Keep stage enable flags and returned
`PipelineStats` consistent when changing stage behavior.

Checkpoint writes must not expose partial files as valid checkpoints. Resume
must honor validation and configured automatic fallback; never destroy the
previous valid checkpoint before a replacement has been validated. Deployment
and rollback must report a failed router update and restore the local version
registry rather than claiming success.

Prompt-safety checks reject blocked content and redact control tokens on the
documented extraction, selection, and callback paths. Preserve fail-closed
behavior when changing those paths.

## Edge cases and current limitations

- Empty required pipeline settings fail closed before LLM-facing work.
- Empty data-selection input yields an empty selection result.
- Missing/corrupt checkpoints may leave no resumable checkpoint; report that
  state rather than silently training from an unintended source.
- Similar-document lookup requires a live initialized vector index in
  production; see [`EMBEDDING.md`](./EMBEDDING.md).
- The current OCR implementation emits a placeholder sample rather than
  recognized OCR text. Do not describe or extend it as a production OCR
  integration without implementing the missing extraction behavior.
- An absent query engine can select offline/test behavior in components that
  document that option. Do not use such paths as evidence of database-backed
  behavior.

## Validation expectations

Use focused tests before broader suites:

- `test_training_pipeline_e2e.cpp` for orchestration and stage behavior.
- `test_training_diagnostics_consistency.cpp` for diagnostic contracts.
- `test_deterministic_training_lifecycle.cpp` and
  `test_training_failsafe_edge_cases_focused.cpp` for lifecycle and error paths.
- `test_training_lora_adapter.cpp` for adapter behavior.
- `test_stress_training_lifecycle.cpp` for extended lifecycle pressure.

Run the relevant registered tests from a configured build directory; see
[`MODULE_GUIDE.md`](./MODULE_GUIDE.md). Changes to measured performance require
the mapped benchmarks and release baseline, not just unit tests.

## Source references

- [`training_pipeline.cpp`](./training_pipeline.cpp)
- [`lora_checkpoint_manager.cpp`](./lora_checkpoint_manager.cpp)
- [`incremental_lora_trainer.cpp`](./incremental_lora_trainer.cpp)
- [`tests/training/CMakeLists.txt`](../../tests/training/CMakeLists.txt)
- [`TRAINING_CONTRACT.md`](../../include/training/TRAINING_CONTRACT.md)

Related: [`ARCHITECTURE.md`](./ARCHITECTURE.md),
[`API.md`](./API.md), and
[`RUNBOOK.md`](./RUNBOOK.md).
