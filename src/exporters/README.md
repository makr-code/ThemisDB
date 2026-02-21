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
