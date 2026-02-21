# Exporters Module - Future Enhancements

## Scope

This document covers planned enhancements to the Exporters module beyond what is tracked in `ROADMAP.md`. It focuses on `jsonl_llm_exporter.cpp`, `stream_writer.cpp`, `pii_detector.cpp`, and `exporter_metrics.cpp`. Features here describe concrete engineering work required to promote the module from Beta to production-grade and extend export format coverage to Parquet, Arrow IPC, and Hugging Face Hub.

## Design Constraints

- The JSONL export API surface (`JasonlLlmExporter`) must remain stable; new formats are added via a pluggable format registry without altering existing call sites.
- PII detection in `pii_detector.cpp` must run before any data leaves the process boundary, regardless of output format.
- Streaming export must not buffer more than a configurable memory limit (default 256 MB) at any point in the pipeline.
- LoRA adapter metadata generation must stay decoupled from document serialization so that vLLM multi-LoRA integration can consume metadata independently.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ExportFormatRegistry::registerFormat()` | All new format writers (Parquet, Arrow, HuggingFace) | Replaces ad-hoc format switches in `jsonl_llm_exporter.cpp` |
| `StreamWriter::writeChunk(span<byte>)` | `JasonlLlmExporter`, Parquet writer, Arrow IPC writer | Already exists in `stream_writer.cpp`; must be extended with backpressure |
| `PiiDetector::scrub(Document&)` | Every export path before serialization | Defined in `pii_detector.cpp`; must be called in format registry pipeline |
| `ExporterMetrics::recordExport(bytes, docs, format)` | All format writers | Defined in `exporter_metrics.cpp`; extend label set with `format` dimension |
| `LoraMetadataGenerator::generate(collection)` | vLLM multi-LoRA adapter loader | Must remain independent of serialization format |

## Planned Features

### Parquet Export with Configurable Schema
**Priority:** High
**Target Version:** v1.6.0

Add a Parquet export writer backed by Apache Arrow's C++ library. Export documents from any ThemisDB collection to columnar `.parquet` files suitable for direct consumption by PyTorch/Hugging Face `datasets.load_dataset()`. Field mapping is driven by the same `FieldSelector` already used by `jsonl_llm_exporter.cpp`.

**Implementation Notes:**
- Add `parquet_exporter.cpp` alongside `jsonl_llm_exporter.cpp`; register via `ExportFormatRegistry`.
- Link `arrow` and `parquet` from the existing vcpkg manifest (`vcpkg.json`).
- Reuse `PiiDetector::scrub()` before column construction; add a unit test asserting no PII columns appear in output schema.
- Use `arrow::RecordBatchWriter` with row-group size configurable via `ExportConfig::parquet_row_group_size` (default 65 536 rows).
- Emit `exporter_parquet_bytes_written_total` counter in `exporter_metrics.cpp`.

**Performance Targets:**
- Export throughput ≥ 500 MB/s uncompressed on a single core (measured with `benchmarks/export_bench.cpp`).
- Snappy-compressed output ≤ 40 % of raw JSONL size for typical text-heavy collections.

---

### Streaming Export for Large Collections
**Priority:** High
**Target Version:** v1.6.0

Replace the current full-batch export path in `jsonl_llm_exporter.cpp` with a cursor-driven streaming pipeline. The `StreamWriter` class in `stream_writer.cpp` will gain a backpressure mechanism so that the database cursor advances only when downstream I/O has consumed the previous chunk.

**Implementation Notes:**
- Add `ExportCursor` abstraction wrapping the AQL query cursor; advance one page at a time (`page_size` configurable, default 1 000 documents).
- Integrate with `StreamWriter::writeChunk()` and enforce `max_buffer_bytes` (default 256 MB) before advancing the cursor.
- Support resumable export: persist cursor position to a checkpoint file; on restart, resume from the last committed offset.
- Expose `ExporterMetrics::recordCheckpoint()` so operators can track resume events in Prometheus/Grafana.

**Performance Targets:**
- Peak resident memory during export of a 50 GB collection ≤ 512 MB process-wide.
- Export of 10 M documents at ≥ 200 000 docs/sec sustained throughput (single thread, local NVMe).

---

### Incremental / Delta Export
**Priority:** Medium
**Target Version:** v1.7.0

Add a delta-export mode that exports only documents created or modified since the last export run. A watermark file records the highest `_last_modified` timestamp committed to the output. Subsequent runs query `FOR doc IN collection FILTER doc._last_modified > @watermark` using the AQL engine.

**Implementation Notes:**
- Store watermark as a JSON file alongside the output; path configurable via `ExportConfig::watermark_path`.
- Add `--incremental` flag to the CLI export command in `tools/`.
- Atomic watermark update: write to `.tmp` then `rename()` to prevent corrupt state on crash.
- Emit `exporter_delta_docs_skipped_total` metric to distinguish full-export from delta-export runs.

**Performance Targets:**
- A delta run over a collection with 0.1 % change rate completes ≥ 10× faster than a full export.
- Watermark file read/write adds ≤ 1 ms overhead per export invocation.

---

### Instruction-Tuning Format Templates
**Priority:** Medium
**Target Version:** v1.7.0

Extend `jsonl_llm_exporter.cpp` to emit documents in named instruction-tuning schemas (Alpaca, ShareGPT, ChatML, OpenAI fine-tuning JSONL) via a `FormatTemplate` enum. Templates define how ThemisDB fields map to `instruction`, `input`, `output`, `messages`, etc.

**Implementation Notes:**
- Add `format_template.cpp` with one concrete `IFormatTemplate` implementation per schema.
- Register templates in `ExportFormatRegistry`; allow user-defined templates via a JSON config file.
- Include a `validate_template` dry-run mode that checks all required fields exist in the source collection schema before export begins.
- Add golden-file tests in `tests/exporters/` asserting exact JSONL output for each template type.

**Performance Targets:**
- Template rendering overhead ≤ 5 % CPU vs. raw JSONL export at the same throughput.
- All four built-in templates validated against their published specifications (Alpaca repo, OpenAI fine-tuning docs).

---

### Export Encryption and Authorization
**Priority:** Medium
**Target Version:** v1.8.0

Integrate with the Governance module to enforce per-collection export authorization and add optional AES-256-GCM output encryption for sensitive training datasets. The `PiiDetector` pipeline is the last in-process defense; encryption provides an additional layer for data at rest.

**Implementation Notes:**
- Before export, call `PolicyEngine::checkExportPermission(collection, requester_id)`; abort and log to the audit trail if denied.
- Accept a key-encryption-key (KEK) reference (not the raw key) from `ExportConfig::encryption_kek_id`; derive a data-encryption-key (DEK) per export job using HKDF-SHA256.
- Write DEK-encrypted data with an AAD header containing the export job ID so decryption failures are attributable.
- Never log raw keys; log only key IDs and algorithm identifiers.

**Performance Targets:**
- AES-256-GCM encryption throughput ≥ 2 GB/s on hardware with AES-NI (measured via `benchmarks/export_bench.cpp`).
- Authorization check latency ≤ 2 ms p99 (local policy engine).

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Each new format writer (`parquet_exporter.cpp`, `format_template.cpp`) must have isolated unit tests with mock `StreamWriter`; PII scrub must have dedicated negative tests |
| Integration | Full pipeline per format | Run JSONL, Parquet, and ChatML template exports against a live test collection; assert byte-for-byte parity of a known 1 000-document fixture |
| Performance | Throughput regression < 5% | `benchmarks/export_bench.cpp` runs in CI on every PR touching the exporters module; alert if p50 throughput drops below baseline |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| JSONL export throughput | ~150 MB/s (full batch) | ≥ 200 000 docs/sec sustained | `benchmarks/export_bench.cpp`, 10 M doc fixture |
| Peak memory (50 GB export) | Unbounded (full batch) | ≤ 512 MB | `/proc/self/status` VmRSS sampled in bench harness |
| Parquet export throughput | N/A (not yet implemented) | ≥ 500 MB/s uncompressed | Same bench harness, Parquet writer path |
| Delta export speedup (0.1% change) | N/A | ≥ 10× vs full export | Timed comparison in integration test suite |

## Security / Reliability

- `PiiDetector::scrub()` must be invoked in the format registry pipeline before any serialization; a static assertion in `ExportFormatRegistry::write()` enforces call order.
- Export authorization must be checked against `PolicyEngine` before any collection cursor is opened; a denied request must never partially write data.
- Encryption keys are referenced by ID only; raw key material must never appear in logs, metrics labels, or exported file headers.
- Watermark files are written atomically (`rename()`) to prevent partial state that could cause duplicate or missing records on restart.
- `exporter_metrics.cpp` must not emit document content or field values in Prometheus labels to prevent data leakage through the metrics endpoint.
