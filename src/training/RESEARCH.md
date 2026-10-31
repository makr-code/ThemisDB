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

# Training Research Notes

## Purpose and scope

This page records the methods that the training module currently implements and
the evidence needed to evaluate them. It is for maintainers comparing training
data preparation and adapter-lifecycle choices; it is not a report of measured
model quality. The implementation and contracts in `include/training/` remain
authoritative.

## Implemented methods

- `DataSelectionPipeline` filters by approximate token length, required
  language, toxicity heuristic, PII check, and shared prompt-safety policy.
- Near-duplicate removal estimates Jaccard similarity from MinHash signatures
  and retains the first sample in input order when the configured threshold is
  reached.
- Diversity selection builds normalized eight-value, character-statistic
  vectors and performs five Lloyd iterations. This is a lightweight
  hash-derived representation, not a learned sentence embedding.
- Selection scores combine a character-entropy pseudo-perplexity estimate,
  token-type ratio (TTR), and BM25-style domain keyword relevance. Difficulty
  is derived from pseudo-perplexity and inverse domain relevance.
- `IncrementalLoRATrainer` exposes initial, incremental, and fine-tuning modes,
  validation metrics, optional quantization, and checkpoint/resume controls.

These are code-level descriptions, not evidence that a heuristic correlates
with downstream model quality. In particular, the `perplexity_model` and
`embedding_model` configuration fields do not by themselves establish that
external models are invoked by the selection implementation.

## Evaluation questions and measurements

For a dataset or adapter change, compare against a fixed input snapshot and
record:

1. Samples entering and leaving each selection stage, duplicate removal rate,
   selected domain distribution, and audit-entry configuration fingerprint.
2. Training and validation loss, accuracy, steps, elapsed time, and adapter
   version from `TrainingMetrics` / `TrainingResult`.
3. Checkpoint validation outcome and resume selection after an interrupted or
   corrupted newest checkpoint.
4. Release-profile runtime and regression against the mapped training
   benchmarks in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

Use the focused suites in [`tests/training`](../../tests/training/) for
correctness and lifecycle regressions. Use benchmark results for performance
claims; do not infer a quality or speed improvement from implementation
completion alone.

## Assumptions and limits

- Results depend on input ordering, selection settings, tokenizer approximation,
  language/domain annotations, and the base model and training configuration.
- The default score weights are configurable and normalized at scoring time;
  they are not a learned calibration.
- A fixed random seed is part of reproducible adapter tests, but it does not
  guarantee identical results across different backends or build settings.
- The roadmap's multi-task acceptance gates remain open until their stated
  measured performance and convergence criteria are demonstrated.

## Source evidence

- [`lora_data_selection.cpp`](./lora_data_selection.cpp) — filter, MinHash,
  hash-derived clustering vectors, scoring, and staged run.
- [`incremental_lora_trainer.h`](../../include/training/incremental_lora_trainer.h)
  — modes, configuration, metrics, and result contracts.
- [`test_deterministic_training_lifecycle.cpp`](../../tests/training/test_deterministic_training_lifecycle.cpp)
  — fixed-seed lifecycle and repeatability checks.
- [`ROADMAP.md`](./ROADMAP.md) and
  [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — current work and
  unclosed evaluation gates.

Related: [`MODELING.md`](./MODELING.md),
[`EMBEDDING.md`](./EMBEDDING.md), and
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
