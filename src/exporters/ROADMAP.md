# Exporters Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — JSONL export optimized for LLM training data, configurable field selection, batch export, LoRA adapter metadata generation, and Parquet columnar export are functional. Apache Arrow IPC export and Hugging Face Hub push are planned.

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
- [x] Arrow IPC export for zero-copy pipelines (Issue: #1714)

## In Progress 🚧
- [I] Parquet export for training datasets (Target: Q2 2026) (Issue: #1710)
- [P] Hugging Face Datasets-compatible export format (Target: Q2 2026) (Issue: #1711)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Export filtering with AQL predicates (Issue: #1715)
- [I] Incremental/delta export (only changed documents since last export) (Issue: #1716)
- [I] Export progress tracking and resumability (Issue: #1717)
- [I] Configurable output compression (zstd, gzip) (Issue: #1718)

### Long-term (6-12 months)
- [I] Hugging Face Hub direct upload integration (Issue: #1719)
- [I] Training data quality scoring (deduplication, toxicity filtering) (Issue: #1720)
- [I] Instruction-tuning format templates (Alpaca, ShareGPT, ChatML) (Issue: #1721)
- [I] Cross-collection join export for complex training datasets (Issue: #1722)
- [I] Synthetic data augmentation pipeline (Issue: #1723)
- [I] Export encryption for sensitive training data (Issue: #1724)

## Implementation Phases

### Phase 1: JSONL and LLM Export (Status: Completed)
- [x] Implemented JSONL exporter writing newline-delimited JSON records (`exporters/jsonl_exporter.cpp`)
- [x] Implemented configurable field selection to include/exclude document fields per export
- [x] Implemented batch export with configurable batch size and output file rotation
- [x] Implemented LoRA adapter metadata generation compatible with PEFT format
- [x] Implemented vLLM multi-LoRA integration support with adapter registry output

### Phase 2: Streaming and Progress (Status: Completed)
- [x] Implement streaming export for collections exceeding available memory (`exporters/streaming_exporter.cpp`)
- [x] Add progress callbacks reporting records exported, bytes written, and estimated ETA

### Phase 3: Parquet, Incremental Export, and Security (Status: In Progress)
- [x] Implement Parquet export with configurable Arrow schema (`exporters/parquet_exporter.cpp`)
- [I] Implement incremental/delta export tracking last-exported sequence number per collection (Issue: #1726)
- [I] Add instruction-tuning format templates (Alpaca, ShareGPT, ChatML) as export transforms (Issue: #1727)
- [I] Implement export encryption using AES-256-GCM for sensitive training data (Issue: #1728)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1729)
- [x] Integration tests (JSONL export, LoRA metadata)
- [I] Performance benchmarks (export throughput) (Issue: #1730)
- [I] Security audit (sensitive field redaction, export authorization) (Issue: #1731)
- [x] Documentation complete (JSONL exporter, LoRA metadata, vLLM integration)
- [x] API stability guaranteed for JSONL exporter

## Known Issues & Limitations
- No delta/incremental export; each run exports the full selection
- JSONL exporter deduplication is opt-in (`quality.skip_duplicates`); Parquet exporter always deduplicates by primary key

## Breaking Changes
- Export format registry will be introduced to add new formats without changing the API signature (additive, non-breaking)
