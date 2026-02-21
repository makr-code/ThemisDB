# Exporters Module Roadmap

## Current Status
**Beta** — JSONL export optimized for LLM training data, configurable field selection, batch export, and LoRA adapter metadata generation are functional. Parquet and Apache Arrow export, and Hugging Face Hub push are planned.

## Completed ✅
- [x] JSONL exporter for LLM training data
- [x] Configurable field selection for export
- [x] Batch export operations
- [x] LoRA adapter metadata generation
- [x] Export pipeline infrastructure
- [x] vLLM multi-LoRA integration support

## In Progress 🚧
- [ ] Parquet export for training datasets (Target: Q2 2026)
- [ ] Hugging Face Datasets-compatible export format (Target: Q2 2026)
- [ ] Streaming export for large collections (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Parquet export with configurable schema
- [ ] Arrow IPC export for zero-copy pipelines
- [ ] Export filtering with AQL predicates
- [ ] Incremental/delta export (only changed documents since last export)
- [ ] Export progress tracking and resumability
- [ ] Configurable output compression (zstd, gzip)

### Long-term (6-12 months)
- [ ] Hugging Face Hub direct upload integration
- [ ] Training data quality scoring (deduplication, toxicity filtering)
- [ ] Instruction-tuning format templates (Alpaca, ShareGPT, ChatML)
- [ ] Cross-collection join export for complex training datasets
- [ ] Synthetic data augmentation pipeline
- [ ] Export encryption for sensitive training data

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (JSONL export, LoRA metadata)
- [ ] Performance benchmarks (export throughput)
- [ ] Security audit (sensitive field redaction, export authorization)
- [x] Documentation complete (JSONL exporter, LoRA metadata, vLLM integration)
- [x] API stability guaranteed for JSONL exporter

## Known Issues & Limitations
- Only JSONL format is currently implemented; Parquet and Arrow are planned
- No streaming export for very large collections; full batch only
- No delta/incremental export; each run exports the full selection
- No built-in deduplication of training examples

## Breaking Changes
- Export format registry will be introduced to add new formats without changing the API signature (additive, non-breaking)
