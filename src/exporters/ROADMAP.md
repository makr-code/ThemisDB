# Exporters Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — JSONL export, Parquet columnar export, Apache Arrow IPC export (file and stream), Hugging Face Datasets-compatible export, streaming export, incremental/delta export, AQL predicate filtering, instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI), AES-256-GCM export encryption, PII detection and redaction, and data augmentation are all operational. Hugging Face Hub direct upload integration and cross-collection join export remain planned.

## Completed ✅
- [x] JSONL exporter for LLM training data
- [x] Configurable field selection for export
- [x] Batch export operations
- [x] LoRA adapter metadata generation
- [x] Export pipeline infrastructure
- [x] vLLM multi-LoRA integration support
- [x] Parquet export with configurable Arrow schema (`exporters/parquet_exporter.cpp`)
- [x] Streaming export for large collections (`exporters/streaming_exporter.cpp`)
- [x] Progress callbacks with records exported, bytes written, and estimated ETA
- [x] Resumable export with checkpoint support
- [x] Parquet export with configurable schema (Issue: #1713)
- [x] Arrow IPC export for zero-copy pipelines (`exporters/arrow_ipc_exporter.cpp`) (Issue: #1714)
- [x] Hugging Face Datasets-compatible export format (`exporters/huggingface_exporter.cpp`) (Issue: #1711)
- [x] Parquet export for training datasets (Issue: #1710)
- [x] Export filtering with AQL predicates (`exporters/aql_predicate_filter.cpp`) (Issue: #1715)
- [x] Export progress tracking and resumability (Issue: #1717)
- [x] Incremental/delta export with watermark and checkpoint (`exporters/incremental_exporter.cpp`) (Issue: #1726)
- [x] Instruction-tuning format templates: Alpaca, ShareGPT, ChatML, OpenAI (`exporters/format_template.cpp`) (Issue: #1727)
- [x] AES-256-GCM export encryption for sensitive training data (`exporters/export_encryption.cpp`) (Issue: #1728)
- [x] Synthetic data augmentation pipeline (`exporters/data_augmentation.cpp`)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Hugging Face Hub direct upload integration (Issue: #1719)
- [I] Cross-collection join export for complex training datasets (Issue: #1722)

## Implementation Phases

### Phase 1: JSONL and LLM Export (Status: Completed)
- [x] Implemented JSONL exporter writing newline-delimited JSON records (`exporters/jsonl_llm_exporter.cpp`)
- [x] Implemented configurable field selection to include/exclude document fields per export
- [x] Implemented batch export with configurable batch size and output file rotation
- [x] Implemented LoRA adapter metadata generation compatible with PEFT format
- [x] Implemented vLLM multi-LoRA integration support with adapter registry output

### Phase 2: Streaming and Progress (Status: Completed)
- [x] Implement streaming export for collections exceeding available memory (`exporters/streaming_exporter.cpp`)
- [x] Add progress callbacks reporting records exported, bytes written, and estimated ETA

### Phase 3: Parquet, Incremental Export, and Security (Status: Completed ✅)
- [x] Implement Parquet export with configurable Arrow schema (`exporters/parquet_exporter.cpp`)
- [x] Implement incremental/delta export tracking last-exported sequence number per collection (Issue: #1726)
- [x] Add instruction-tuning format templates (Alpaca, ShareGPT, ChatML) as export transforms (Issue: #1727)
- [x] Implement export encryption using AES-256-GCM for sensitive training data (Issue: #1728)

### Phase 4: Arrow IPC, HuggingFace, AQL Filtering, and Augmentation (Status: Completed ✅)
- [x] Implement Apache Arrow IPC file (`.arrow`) and stream (`.arrows`) export (`exporters/arrow_ipc_exporter.cpp`) (Issue: #1714)
- [x] Implement Hugging Face Datasets-compatible export: JSONL data shards + `dataset_card.md` + `dataset_info.json` (`exporters/huggingface_exporter.cpp`) (Issue: #1711)
- [x] Implement AQL predicate filtering to restrict exported records without code changes (`exporters/aql_predicate_filter.cpp`) (Issue: #1715)
- [x] Implement synthetic data augmentation pipeline for training data diversity (`exporters/data_augmentation.cpp`)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1729)
- [x] Integration tests (JSONL export, LoRA metadata, Parquet, Arrow IPC, HuggingFace, streaming, incremental)
- [x] Focused test targets for all 10 exporter test files registered in tests/CMakeLists.txt
- [I] Performance benchmarks (export throughput) (Issue: #1730)
- [x] Security audit (sensitive field redaction, export authorization) (Issue: #1731)
- [x] Documentation complete (JSONL exporter, LoRA metadata, vLLM integration, Parquet, Arrow IPC, HuggingFace)
- [x] API stability guaranteed for JSONL exporter
- [x] All 13 source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake

## Known Issues & Limitations
- JSONL exporter deduplication is opt-in (`quality.skip_duplicates`); Parquet exporter always deduplicates by primary key
- JSONL exporter toxicity filtering is opt-in (`quality.enable_toxicity_filter`); heuristic word-list approach, not ML-based

## Breaking Changes
- Export format registry will be introduced to add new formats without changing the API signature (additive, non-breaking)
