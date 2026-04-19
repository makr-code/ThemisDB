> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Exporters Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — JSONL export, Parquet columnar export, Apache Arrow IPC export (file and stream), Hugging Face Datasets-compatible export, HuggingFace Hub direct upload, streaming export, incremental/delta export, AQL predicate filtering, instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI), AES-256-GCM export encryption, PII detection and redaction, data augmentation, ExportFormatRegistry, PolicyEngine authorization, and cross-collection join export are all operational.

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
- [x] PolicyEngine authorization check before cursor open — `enforceExportPolicy()` in all 6 exporters; `ERR_EXPORT_POLICY_DENIED` (9310) added to error_registry.h (EXP-001)
- [x] Hugging Face Hub direct upload via libcurl — `HuggingFaceHubClient` in
  `include/exporters/huggingface_hub_client.h` and
  `src/exporters/huggingface_hub_client.cpp`
  - `HubUploadConfig` fields: `policy_engine`, `audit_log`, `requesting_user`
  - PolicyEngine authorization + audit logging on all return paths
  - (Issue: #1719, EXP-002)
- [x] `--incremental` CLI flag — `tools/export_cli.cpp` (themis-export binary) supports `--format incremental` and `--incremental` shorthand (EXP-004)
- [x] ExportFormatRegistry singleton — `include/exporters/export_format_registry.h` and `src/exporters/export_format_registry.cpp`; 13 built-in formats registered (9 plain + 4 instruction-tuning template shortcuts: `jsonl_alpaca`, `jsonl_sharegpt`, `jsonl_chatml`, `jsonl_openai_ft`); user-defined templates loadable via `loadTemplatesFromConfig()` / `loadTemplatesFromJson()` (EXP-005)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Cross-collection join export for complex training datasets (`exporters/join_exporter.cpp`) (Issue: #1722)
- [x] `validate_template` dry-run mode — `validateTemplate()` free function in `format_template.h/cpp`; `JSONLLLMExporter::validateTemplate()`; 18 tests in `tests/exporters/test_format_template.cpp`

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

### Phase 5: Authorization, Hub Upload, Registry, and CLI (Status: Completed ✅)
- [x] PolicyEngine authorization check (`enforceExportPolicy()`) in all 6 exporters; `ERR_EXPORT_POLICY_DENIED` (9310) (EXP-001)
- [x] HuggingFace Hub direct upload client (`HuggingFaceHubClient`) using libcurl; PolicyEngine authorization (`HubUploadConfig::policy_engine`); audit logging (`HubUploadConfig::audit_log`) on all return paths (Issue: #1719, EXP-002)
- [x] `--incremental` CLI flag in `tools/export_cli.cpp` (EXP-004)
- [x] `ExportFormatRegistry` singleton with 15 built-in format factories (11 plain + 4 instruction-tuning template shortcuts); `loadTemplatesFromConfig()` / `loadTemplatesFromJson()` for user-defined templates (EXP-005)

### Phase 6: Cross-Collection Join Export (Status: Completed ✅)
- [x] Cross-collection hash-join export (`exporters/join_exporter.cpp`): inner join semantics, right-side in-memory hash table, ≥ 50 000 merged docs/sec (Issue: #1722)
- [x] `JoinExportConfig` struct: `left_collection`, `right_collection`, `left_key_field`, `right_key_field`, `join_predicate` (AQL), `output_fields` with `"src:alias"` renaming, PII config, 1 GiB right-side memory budget
- [x] Qualified field references (`left.<field>` / `right.<field>`) resolve ambiguous names; `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD` for unaliased conflicts
- [x] AQL join predicate filtering on merged record; `ERR_EXPORT_JOIN_PREDICATE_INVALID` for parse failures
- [x] Right-side memory budget enforcement; `ERR_EXPORT_JOIN_MEMORY_LIMIT` when exceeded
- [x] PII detection and redaction on merged serialised JSON
- [x] `JoinExporter` registered in `ExportFormatRegistry` as `"join"` and `"join_jsonl"`
- [x] 676-line test suite (`tests/exporters/test_join_exporter.cpp`); CI: `.github/workflows/join-exporter-ci.yml`

## Production Readiness Checklist
- [P] Unit tests coverage > 80% (Issue: #1729)
- [x] Integration tests (JSONL export, LoRA metadata, Parquet, Arrow IPC, HuggingFace, streaming, incremental, join)
- [x] Focused test targets for all 13 exporter test files registered in tests/CMakeLists.txt
- [~] Performance benchmarks (export throughput) — `benchmarks/bench_exporters.cpp` added (Issue: #1730)
- [x] Security audit (sensitive field redaction, export authorization via PolicyEngine) (Issue: #1731)
- [x] Documentation complete (JSONL exporter, LoRA metadata, vLLM integration, Parquet, Arrow IPC, HuggingFace, join)
- [x] API stability guaranteed for JSONL exporter
- [x] All 16 source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake

## Known Issues & Limitations
- JSONL exporter deduplication is opt-in (`quality.skip_duplicates`); Parquet exporter always deduplicates by primary key
- JSONL exporter toxicity filtering is opt-in (`quality.enable_toxicity_filter`); heuristic word-list approach, not ML-based
- HuggingFaceHubClient requires `CURL_ENABLED` compile flag and libcurl link; without it, uploadDataset() returns an error
- `export_cli.cpp` loads data from stdin (`@collection_name` prefix) when not linked to an embedded store

## Breaking Changes
- ~~Export format registry will be introduced to add new formats without changing the API signature (additive, non-breaking)~~ — implemented as `ExportFormatRegistry` (EXP-005)
