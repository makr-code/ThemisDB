# Exporters Module

Data export functionality for ThemisDB.

## Module Purpose

Provides data export functionality for ThemisDB. Supported formats:

- **JSONL** — optimized for LLM training data (instruction/input/output, Alpaca/ShareGPT/ChatML)
- **Apache Parquet** — columnar export for analytics workloads
- **Apache Arrow IPC** — zero-copy pipelines (file and stream formats)
- **Hugging Face Datasets** — dataset card + Parquet shards for Hub upload
- **Streaming** — large-collection export without full in-memory buffering

## Subsystem Scope

**In scope:**

- Export formats: JSONL, Apache Parquet, Apache Arrow IPC, Hugging Face Datasets
- Configurable field selection (include/exclude per export)
- Batch and streaming export
- LoRA adapter metadata generation; vLLM multi-LoRA integration
- PII detection and redaction; multi-tenant isolation with scope-based authorization

**Out of scope:** Data transformation (handled by content module), import functionality (handled by importers module), data compression (delegated to utils/zstd).

## Relevant Interfaces

- `jsonl_llm_exporter.cpp` — primary JSONL export with LLM training format
- `parquet_exporter.cpp` — Apache Parquet columnar export
- `arrow_ipc_exporter.cpp` — Apache Arrow IPC file and stream export (zero-copy pipelines)
- `huggingface_exporter.cpp` — Hugging Face Datasets-compatible export
- `streaming_exporter.cpp` — streaming export for large collections
- `stream_writer.cpp` — low-level streaming output writer
- `pii_detector.cpp` — PII detection and redaction before export
- `exporter_metrics.cpp` — export throughput and quality metrics

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — JSONL, Parquet, Arrow IPC, Hugging Face, and streaming export all operational.

## Components

- JSONL exporter for LLM training data
- Parquet columnar exporter
- Arrow IPC exporter (file and stream format)
- Hugging Face dataset exporter
- Streaming exporter for large collections
- PII detection and redaction
- Export metrics and telemetry

## Features

- Export documents in JSONL format optimized for LLM training
- Export to Apache Parquet columnar format
- Export to Apache Arrow IPC file (`.arrow`) or stream (`.arrows`) format for zero-copy pipelines
- Hugging Face Datasets-compatible export
- Configurable field selection (include/exclude)
- Batch export operations
- Streaming export without full in-memory load
- LoRA adapter metadata generation
- PII detection and redaction (mask, hash, remove, partial)
- Multi-tenant isolation with scope-based authorization
- Progress callbacks with records exported, bytes written, and estimated ETA

## Documentation

For exporter documentation, see:
- [JSONL LLM Exporter](../../docs/exporters/JSONL_LLM_EXPORTER.md)
- [Implementation Summary](../../docs/exporters/IMPLEMENTATION_SUMMARY.md)
- [LoRA Adapter Metadata](../../docs/exporters/LORA_ADAPTER_METADATA.md)
- [vLLM Integration](../../docs/exporters/VLLM_MULTI_LORA_INTEGRATION.md)

## Scientific References

1. Abadi, D., Boncz, P., Harizopoulos, S., Idreos, S., & Madden, S. (2013). **The Design and Implementation of Modern Column-Oriented Database Systems**. *Foundations and Trends in Databases*, 5(3), 197–280. https://doi.org/10.1561/1900000024

2. Apache Arrow Community. (2016). **Apache Arrow: A Cross-Language Development Platform for In-Memory Data**. Apache Software Foundation. https://arrow.apache.org/

3. Vohra, D. (2016). **Apache Parquet**. Apress. https://doi.org/10.1007/978-1-4842-1592-5

4. Deutsch, L. P. (1996). **DEFLATE Compressed Data Format Specification version 1.3**. RFC 1951. IETF. https://doi.org/10.17487/RFC1951
