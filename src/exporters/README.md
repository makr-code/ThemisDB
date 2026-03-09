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
- `incremental_exporter.cpp` — delta/incremental export with watermark-based change tracking
- `aql_predicate_filter.cpp` — AQL predicate filtering to restrict exported records
- `format_template.cpp` — instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI)
- `export_encryption.cpp` — AES-256-GCM encryption for sensitive export data
- `pii_detector.cpp` — PII detection and redaction before export
- `data_augmentation.cpp` — synthetic data augmentation pipeline for training data diversity
- `exporter_metrics.cpp` — export throughput and quality metrics

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — JSONL, Parquet, Arrow IPC, Hugging Face, and streaming export all operational.

## Components

- JSONL exporter for LLM training data
- Parquet columnar exporter
- Arrow IPC exporter (file and stream format)
- Hugging Face dataset exporter
- Streaming exporter for large collections
- Incremental/delta exporter with watermark-based change tracking
- AQL predicate filter for record-level export filtering
- Instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI)
- Export encryption (AES-256-GCM) for sensitive training data
- Synthetic data augmentation pipeline
- PII detection and redaction
- Export metrics and telemetry

## Features

- Export documents in JSONL format optimized for LLM training
- Export to Apache Parquet columnar format
- Export to Apache Arrow IPC file (`.arrow`) or stream (`.arrows`) format for zero-copy pipelines
- Hugging Face Datasets-compatible export (JSONL shards + `dataset_card.md` + `dataset_info.json`)
- Configurable field selection (include/exclude)
- Batch export operations
- Streaming export without full in-memory load
- Incremental/delta export: only records modified since the last export watermark
- AQL predicate filtering to restrict exported records without code changes
- Instruction-tuning format templates: Alpaca, ShareGPT, ChatML, OpenAI fine-tuning JSONL
- AES-256-GCM encryption for sensitive export data (key referenced by ID via HKDF-SHA256)
- Synthetic data augmentation (synonym replacement, back-translation stubs, paraphrase variants)
- LoRA adapter metadata generation
- PII detection and redaction (mask, hash, remove, partial)
- Multi-tenant isolation with scope-based authorization
- Progress callbacks with records exported, bytes written, and estimated ETA

## Documentation

For exporter documentation, see:
- [Implementation Summary](../../docs/exporters/IMPLEMENTATION_SUMMARY.md)
- [P0 Implementation (Foundation)](../../docs/exporters/P0_IMPLEMENTATION.md)
- [P1/P2 Implementation (Security & Performance)](../../docs/exporters/P1_P2_IMPLEMENTATION.md)

## Scientific References

1. Abadi, D., Boncz, P., Harizopoulos, S., Idreos, S., & Madden, S. (2013). **The Design and Implementation of Modern Column-Oriented Database Systems**. *Foundations and Trends in Databases*, 5(3), 197–280. https://doi.org/10.1561/1900000024

2. Apache Arrow Community. (2016). **Apache Arrow: A Cross-Language Development Platform for In-Memory Data**. Apache Software Foundation. https://arrow.apache.org/

3. Vohra, D. (2016). **Apache Parquet**. Apress. https://doi.org/10.1007/978-1-4842-1592-5

4. Deutsch, L. P. (1996). **DEFLATE Compressed Data Format Specification version 1.3**. RFC 1951. IETF. https://doi.org/10.17487/RFC1951

5. Lhoest, Q., Villanova del Moral, A., Jernite, Y., Thakur, A., von Platen, P., Patil, S., Chaumond, J., Drame, M., Plu, J., Tunstall, L., Davison, J., Šaško, M., Chhablani, G., Malik, B., Brandeis, S., Le Scao, T., Sanh, V., Xu, C., Patry, N., … Wolf, T. (2021). **Datasets: A Community Library for Natural Language Processing**. In *Proceedings of the 2021 Conference on Empirical Methods in Natural Language Processing: System Demonstrations* (pp. 175–184). Association for Computational Linguistics. https://doi.org/10.18653/v1/2021.emnlp-demo.21

6. Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023). **QLoRA: Efficient Finetuning of Quantized LLMs**. In *Advances in Neural Information Processing Systems*, 36. https://arxiv.org/abs/2305.14314

7. McGrew, D., & Viega, J. (2004). **The Galois/Counter Mode of Operation (GCM)**. NIST Submission. https://csrc.nist.gov/publications/detail/sp/800-38d/final
