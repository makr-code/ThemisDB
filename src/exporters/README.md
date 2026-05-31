# ThemisDB Exporters Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The exporters module provides data export runtime surfaces for ThemisDB, including JSONL/Parquet/Arrow/HuggingFace formats, streaming and incremental export, join export workflows, encryption, template transformation, and export policy controls.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| jsonl_llm_exporter.cpp | JSONL export for LLM training workflows |
| parquet_exporter.cpp | Parquet export pipeline |
| arrow_ipc_exporter.cpp | Arrow IPC export paths |
| huggingface_exporter.cpp | HuggingFace dataset export format generation |
| huggingface_hub_client.cpp | HuggingFace Hub upload client workflows |
| streaming_exporter.cpp | bounded-memory streaming export |
| incremental_exporter.cpp | watermark-based delta export |
| join_exporter.cpp | cross-collection join export |
| aql_predicate_filter.cpp | AQL predicate export filtering |
| format_template.cpp | export format templates and validation |
| export_encryption.cpp | export encryption controls |
| pii_detector.cpp | PII detection/redaction paths |
| data_augmentation.cpp | export-time augmentation helpers |
| export_format_registry.cpp | export format registration and resolution |
| stream_writer.cpp | stream writer output primitives |
| exporter_metrics.cpp | exporter metrics and observability |

## Scope

In scope:
- multi-format export generation and serialization paths
- streaming/incremental/join export orchestration
- export filtering, encryption, and PII controls
- registry, metrics, and hub-upload operational integrations

Out of scope:
- import pipeline ownership
- non-export transformation workflows outside export contracts
- storage-engine internals outside exporter interfaces

## Runtime Behavior and Limits

- export behavior depends on selected format and exporter options.
- policy, filtering, and PII paths can reject or mutate records before output.
- streaming and incremental modes provide bounded-memory export behavior.

## Sourcecode Verification (Module: exporters/readme)

- Verified files:
  - src/exporters/jsonl_llm_exporter.cpp
  - src/exporters/parquet_exporter.cpp
  - src/exporters/arrow_ipc_exporter.cpp
  - src/exporters/huggingface_exporter.cpp
  - src/exporters/huggingface_hub_client.cpp
  - src/exporters/streaming_exporter.cpp
  - src/exporters/incremental_exporter.cpp
  - src/exporters/join_exporter.cpp
  - src/exporters/aql_predicate_filter.cpp
  - src/exporters/format_template.cpp
  - src/exporters/export_encryption.cpp
  - src/exporters/pii_detector.cpp
  - src/exporters/data_augmentation.cpp
  - src/exporters/export_format_registry.cpp
  - src/exporters/stream_writer.cpp
  - src/exporters/exporter_metrics.cpp
- Verified behavior surfaces:
  - format-specific export pipelines and hub integrations
  - streaming/incremental/join export behavior
  - policy/filter/encryption/PII observability surfaces
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md