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

# Training API and lifecycle summary

This is a source-checked summary of current training interfaces, not a frozen
state machine or a substitute for header contracts. The declarations in
`include/training/*.h` define exact signatures, ownership, and error behavior.
In particular, `TrainingPipeline`'s synchronous `run()` API is distinct from
the asynchronous `ITrainingPipeline` interface in `training_interfaces.h`.

## Pipeline and database data flow

`TrainingPipeline::run()` conditionally runs labeling, enrichment,
quality/drift checks, selection, and training according to `PipelineConfig`.
It returns `PipelineStats`; callers must check `error_message` and
`training_success` rather than infer success from a non-throwing call.

The trainer accepts an optional non-owning `QueryEngine*`, which must outlive
the trainer. When configured, it reads up to `max_training_samples` from
`training_data_collection`, validates the collection identifier, and fails
closed on a query error, invalid result shape, or absence of valid
input/output samples. Malformed rows are skipped; a collection with no valid
rows is an error.

The pipeline can use injected query and sample-storage dependencies to persist
labeled samples and train from the configured collection. The focused RocksDB
pipeline test verifies persisted and consumed sample counts with selection
disabled. In the current implementation, selection results are recorded by
the pipeline but are not passed directly to the trainer, which reads its
configured collection separately. `TrainingDataIterator` is owned by the
`llm` module and has a separate AQL batch-reading path.

## Checkpoint and serving behavior

- `LoRACheckpointManager::save()` copies a source checkpoint through a
  temporary file, computes its SHA-256, renames it, and updates the manifest.
  See its header for exact exception behavior and configuration.
- `resume()` and `resumeWithDiagnostics()` return
  `std::optional<CheckpointManifestEntry>`; `validate()` reports a boolean.
  A missing valid checkpoint is not represented by a `TrainingErrorCode`
  return value from these methods.
- `IncrementalLoRATrainer::deployVersionEx()` and
  `rollbackVersionEx()` return `DeployResult`. Check its `success` and `error`
  fields and verify external router state after a failed handoff.
- The pipeline's stage sequence does not include serving handoff as an
  automatic `run()` stage; deployment is a separate trainer/router operation.

## Error-code reference

`TrainingErrorCode` values and descriptions are defined in
[`training_error_codes.h`](./training_error_codes.h). The high-byte ranges are:

| Range | Component |
|---|---|
| `0x8000`–`0x80FF` | Checkpoint management |
| `0x8100`–`0x81FF` | Training execution |
| `0x8200`–`0x82FF` | Adapter merge |
| `0x8300`–`0x83FF` | Knowledge-graph enrichment |
| `0x8400`–`0x84FF` | Auto-labeling |
| `0x8500`–`0x85FF` | Adapter serving |
| `0x8600`–`0x86FF` | Provenance |
| `0x8700`–`0x87FF` | Dataset validation |

Examples from the enum include `CHECKPOINT_SHA256_MISMATCH = 0x8044`,
`LABELING_NO_CONTENT = 0x8400`, `SERVING_ADAPTER_NOT_FOUND = 0x8500`, and
`SERVING_ADAPTER_INCOMPATIBLE = 0x8503`. Numeric codes do not define retry
policy; use the relevant result/exception contract and integration policy.

## Focused validation

The training module's CMake registration discovers `test_*.cpp` files under
`tests/training/` and registers focused targets with the `training` label and a
120-second timeout. DB behavior is covered by
`test_incremental_lora_db_training.cpp` and `test_training_pipeline_db.cpp`.
See [`../../src/training/MODULE_GUIDE.md`](../../src/training/MODULE_GUIDE.md)
for build and test commands.

When public signatures or error codes change, update the authoritative headers,
focused tests, this summary, and applicable changelog/release documentation.
