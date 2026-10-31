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

# Training Data and Adapter Model

## Purpose

This document describes the module's data and adapter representations for
developers modifying training, selection, or checkpoint code. Public contracts
are defined by the headers under [`include/training`](../../include/training/);
this page does not define a new serialization format.

## Data path

1. A source document is labeled into `TrainingSample` values. Each sample
   carries input/output text, category, confidence, source identifier,
   metadata, and a `ContentModality`.
2. `ModalityDetector` can produce text-clause, table, citation, or OCR-tagged
   samples. Configured minimum lengths, extraction limits, and modality
   confidence defaults bound this extraction.
3. Selection consumes `DataSample` values (`id`, text, language, and optional
   domain). It filters, de-duplicates, clusters for diversity, scores, and
   returns a selected subset with an audit entry.
4. `IncrementalLoRATrainer` consumes the configured training collection and
   produces a `TrainingResult`; metrics include loss, accuracy, samples,
   elapsed time, and version.
5. Provenance records link sample IDs to source-document URNs and associated
   extraction metadata. Checkpoint manifests associate adapter files with
   version, step/epoch, metrics, and SHA-256 digest.

The training sample and data-selection sample are separate public structures;
do not assume their fields or identifiers are interchangeable without an
explicit conversion in the implementation.

## Adapter and checkpoint invariants

- LoRA/AdaLoRA configuration (rank, alpha, learning rate, batch size, epoch
  count, sequence length, device, and optional quantization) affects the
  resulting adapter and must be captured with the experiment inputs.
- Adapter lifecycle operations report explicit success/failure; deployment
  validates the adapter and traffic split before updating the registry/router.
- A checkpoint save writes a temporary file, computes SHA-256, renames it to
  the final path, then updates the rolling manifest. Resume validates entries
  when configured and can try older entries when automatic fallback is enabled.
- Provenance write limits and checkpoint I/O limits are configurable. A zero
  provenance `write_timeout_ms` means no deadline; do not describe it as
  bounded unless deployments set a limit.

## Evaluation record

For a model or training configuration change, preserve the base-model identity,
training-data snapshot and selection configuration, seed where applicable,
backend/device and build capabilities, adapter version, validation metrics,
and checkpoint digest. Compare the same task set against a stated baseline.
The benchmark mapping and current release thresholds are in
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md); the roadmap's
multi-task gates are not considered passed until their measurements are
recorded.

## Implementation limitations to account for

- The data-selection `perplexity` value is currently a normalized
  character-frequency entropy estimate, not model perplexity.
- Selection clustering currently uses an eight-value character-statistic
  vector. The configurable `embedding_model` name does not change that
  implementation.
- `OCRExtractor` does not call Tesseract to obtain recognized text in its
  current implementation. Its enabled path emits a placeholder sample based
  on the image path. Do not use that output as OCR-transcribed training data.
- Offline/test modes can omit database-backed fetches; their empty or
  in-process results do not demonstrate production data integration.

## Source evidence

- [`auto_labeler.h`](../../include/training/auto_labeler.h) and
  [`modality_parser.h`](../../include/training/modality_parser.h) — sample and
  modality types.
- [`lora_data_selection.h`](../../include/training/lora_data_selection.h) —
  selection configuration and result.
- [`incremental_lora_trainer.h`](../../include/training/incremental_lora_trainer.h)
  — trainer configuration and metrics.
- [`provenance_tracker.h`](../../include/training/provenance_tracker.h) and
  [`lora_checkpoint_manager.h`](../../include/training/lora_checkpoint_manager.h)
  — provenance and checkpoint records.
- [`modality_parser.cpp`](./modality_parser.cpp) and
  [`lora_data_selection.cpp`](./lora_data_selection.cpp) — current extraction
  and scoring implementations.

Related: [`RESEARCH.md`](./RESEARCH.md),
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md), and
[`API.md`](./API.md).
