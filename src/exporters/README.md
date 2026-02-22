# Exporters Module

Data export functionality for ThemisDB.

## Module Purpose

Provides data export functionality for ThemisDB, primarily JSONL export optimized for LLM training with LoRA adapter metadata generation and vLLM multi-LoRA integration.

## Subsystem Scope

**In scope:** JSONL export for LLM training, configurable field selection, batch export, LoRA adapter metadata, vLLM multi-LoRA integration.

**Out of scope:** Data transformation (handled by content module), import functionality (handled by importers module), data compression (delegated to utils/zstd).

## Relevant Interfaces

- `jsonl_llm_exporter.cpp` — primary JSONL export with LLM training format
- `export_pipeline.cpp` — export orchestration
- `lora_metadata_writer.cpp` — LoRA adapter metadata generation

## Current Delivery Status

**Maturity:** 🟡 Beta — JSONL/LoRA export operational; Parquet export and streaming export in progress.

## Components

- JSONL exporter for LLM training data
- Custom export format handlers
- Export pipeline

## Features

- Export documents in JSONL format optimized for LLM training
- Configurable field selection
- Batch export operations
- LoRA adapter metadata generation

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
