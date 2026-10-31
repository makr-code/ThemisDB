---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Export Data Modeling

Exact model backend not verifiable from final document text alone.

## Purpose and scope

This document describes the record and schema model used when `BaseEntity`
values are projected into export formats. It does not describe a learned
predictive model. Exporter implementations serialize or reshape supplied
records; they do not own source-schema evolution or infer business meaning.

The shared contract is declared in
[`include/exporters/exporter_interface.h`](../../include/exporters/exporter_interface.h).
Format-specific behavior is implemented under `src/exporters/` and exposed
through [`include/exporters/README.md`](../../include/exporters/README.md).

## Record-to-artifact model

An export operation combines:

1. **Input records:** a sequence of `BaseEntity` values from the caller.
2. **Projection:** `include_fields`, `exclude_fields`, and where supported an
   AQL predicate determine the data reaching serialization.
3. **Schema/format:** JSONL field mappings or templates, Parquet column hints
   and nullability, Arrow columns and null tracking, or Hugging Face feature
   definitions/inference.
4. **Artifact metadata:** optional JSONL metadata, Parquet/Arrow metadata,
   Hugging Face dataset information, or adapter metadata.
5. **Transport/storage options:** output path, compression, file-size limit,
   and optional encryption.

The output is a representation of selected entity values, not a canonical
replacement for the collection schema. Format-specific schema inference can
depend on the records supplied to that run; specify hints or explicit features
when consumers require a stable schema.

## Invariants and edge cases

- The API contract specifies null as an empty CSV cell, a clear Parquet
  definition/null bit, and a clear Arrow validity bit. Do not equate null with
  an absent field without checking the selected exporter.
- JSONL LLM styles map configured question/context/answer or
  system/user/assistant fields. Named Alpaca, ShareGPT, ChatML, and OpenAI
  fine-tuning templates validate required fields; preflight representative
  records with `validateTemplate()` before a large export.
- Default JSONL quality settings skip empty outputs and duplicates; those
  settings affect counts and artifact contents.
- Join output combines left and right entities by configured key fields.
  Colliding output names require explicit aliases; right-side data is loaded
  into an in-memory hash table and is subject to its configured memory budget.
- Incremental export compares a configured sequence field against a persisted
  watermark. Missing-sequence behavior is configurable and must be tested
  against the source collection's actual sequence semantics.

## Evaluation and trade-offs

Choose explicit schema hints/features where downstream compatibility is more
important than convenient inference. Measure schema inference, projection,
formatting, compression, and serialization separately when investigating
latency or memory. Preserve representative null, heterogeneous-value, empty
record, duplicate, and missing-field fixtures in
`tests/exporters/`; review output against the format contract rather than
assuming every format uses JSON semantics.

The schema contract and reproducibility process are detailed in
[RESEARCH.md](./RESEARCH.md). Embedding vectors are covered separately in
[EMBEDDING.md](./EMBEDDING.md).
