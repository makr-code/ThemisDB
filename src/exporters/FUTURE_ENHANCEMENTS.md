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

### ~~Parquet Export with Configurable Schema~~ ✅ Implemented (Issue #1710)
**Priority:** High  
**Target Version:** v1.6.0 — **Delivered**

Parquet export is implemented in `parquet_exporter.cpp` / `parquet_exporter.h`. A fallback
minimal Parquet v2 writer is always available; the Arrow C++ path is activated when
`ARROW_ENABLED` is defined. Field mapping reuses the same `FieldSelector` used by
`jsonl_llm_exporter.cpp`. The `exporter_parquet_bytes_written_total` counter is emitted
in `exporter_metrics.cpp`.

**Remaining (future issues):**
- Register Parquet writer in a formal `ExportFormatRegistry` (additive, non-breaking). ✅ Implemented — `ExportFormatRegistry::registerBuiltins()` registers "parquet".

### ~~Streaming Export for Large Collections~~ ✅ Implemented
**Priority:** High  
**Target Version:** v1.6.0 — **Delivered**

Cursor-driven streaming export is implemented in `streaming_exporter.cpp` / `streaming_exporter.h`.
`VectorExportCursor` advances one page at a time (`page_size` configurable). `StreamWriter`
enforces `max_buffer_bytes` (default 256 MB). Checkpoint-based resumable export is supported;
`ExporterMetrics::recordCheckpoint()` emits resume events.

**Remaining (future issues):**
- Backpressure integration with real storage cursor when the AQL engine exposes an async pull API.

---

### ~~Incremental / Delta Export~~ ✅ Implemented (Issue #1726)
**Priority:** Medium  
**Target Version:** v1.7.0 — **Delivered**

Delta export is implemented in `incremental_exporter.cpp` / `incremental_exporter.h`.
A JSON watermark file records the highest sequence number committed to the output.
Atomic watermark update uses `.tmp` + `rename()`. The
`exporter_delta_docs_skipped_total` metric distinguishes full-export from delta runs.

**Remaining (future issues):**
- `--incremental` flag on the CLI export command in `tools/`.

---

### ~~Instruction-Tuning Format Templates~~ ✅ Implemented (Issue #1727)
**Priority:** Medium  
**Target Version:** v1.7.0 — **Delivered**

Named instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI fine-tuning JSONL) are implemented in `format_template.cpp` / `format_template.h`.  The `JSONLLLMExporter` activates a template via `JSONLLLMConfig::format_template_type`; field-name overrides go in `template_field_mapping`.  See `tests/exporters/test_format_template.cpp` for 35 test cases.

**Remaining (future issues):**
- Register templates in `ExportFormatRegistry`; allow user-defined templates via a JSON config file.
- ~~`validate_template` dry-run mode that checks all required fields exist in the source collection schema before export begins.~~ ✅ Implemented — `validateTemplate()` free function in `format_template.h/cpp`; `JSONLLLMExporter::validateTemplate()` wrapper; 18 test cases in `tests/exporters/test_format_template.cpp`.

---

### ~~Export Encryption and Authorization~~ ✅ Implemented (Issue #1728)
**Priority:** Medium  
**Target Version:** v1.8.0 — **Delivered**

AES-256-GCM encryption is implemented in `export_encryption.cpp` / `export_encryption.h`.
A file-format header (`TENC` magic, format version, IV, GCM tag) ensures authenticated
decryption. Key material is referenced by ID only; DEK is derived via HKDF-SHA256 from a
KEK reference in `ExportConfig::encryption_kek_id`. Raw keys are never logged.

**Remaining (future issues):**
- ~~Full integration with `PolicyEngine::checkExportPermission()` for per-collection
  authorization checks before any cursor is opened.~~ ✅ Implemented — `enforceExportPolicy()` called at the top of all 6 exporters; `ERR_EXPORT_POLICY_DENIED` (9310) added (EXP-001).

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
| Peak memory (50 GB export) | Bounded via `StreamingExporter` (≤ 512 MB) | ≤ 512 MB | `/proc/self/status` VmRSS sampled in bench harness |
| Parquet export throughput | Implemented (fallback writer) | ≥ 500 MB/s uncompressed (Arrow path) | Same bench harness, Parquet writer path |
| Delta export speedup (0.1% change) | Implemented | ≥ 10× vs full export | Timed comparison in integration test suite |

## Security / Reliability

- `PiiDetector::scrub()` must be invoked in the format registry pipeline before any serialization; a static assertion in `ExportFormatRegistry::write()` enforces call order.
- Export authorization must be checked against `PolicyEngine` before any collection cursor is opened; a denied request must never partially write data.
- Encryption keys are referenced by ID only; raw key material must never appear in logs, metrics labels, or exported file headers.
- Watermark files are written atomically (`rename()`) to prevent partial state that could cause duplicate or missing records on restart.
- `exporter_metrics.cpp` must not emit document content or field values in Prometheus labels to prevent data leakage through the metrics endpoint.

---

## Planned Features (Next Milestones)

### ~~Hugging Face Hub Direct Upload Integration~~ ✅ Implemented (Issue: #1719)
**Priority:** Medium
**Target Version:** v1.9.0 — **Delivered**

`HuggingFaceHubClient` implemented in `include/exporters/huggingface_hub_client.h` and
`src/exporters/huggingface_hub_client.cpp`.  Authenticates via `HubUploadConfig::hf_token`
or `HF_TOKEN` env variable.  Retry logic (exponential back-off, `max_retries = 3`).
401 Unauthorized surfaced immediately; 413 Payload Too Large returns a shard-split hint.
Progress via `std::function<void(double)>` callback.

**Remaining:**
- Stream JSONL shards directly from memory (current impl reads files from disk; avoids double filesystem write in future PR).
- `hf_token_kek_id` for KMS-protected token lookup (future security hardening).

---

### Cross-Collection Join Export
**Priority:** Low
**Target Version:** v2.0.0 (Issue: #1722)

Export a joined view of two or more collections (e.g., `documents JOIN annotations`) into a single JSONL or Parquet output file. Uses the AQL engine to evaluate the join predicate.

**Implementation Notes:**
- Add `JoinExportConfig` struct with `left_collection`, `right_collection`, `join_predicate` (AQL expression), and `output_fields`.
- Implement as a new exporter class `JoinExporter` that opens two `AqlPredicateFilter` cursors and merges record batches.
- PII detection runs on the merged record before serialization.
- Error cases: collection not found, join predicate parse failure, ambiguous field names (rename via `output_fields` alias map).

**Performance Targets:**
- Join export throughput ≥ 50 000 merged docs/sec (hash-join on in-memory right side ≤ 10 M rows).
- Memory budget for right-side hash table ≤ 1 GB configurable.

---

## Scientific References

1. Abadi, D., Boncz, P., Harizopoulos, S., Idreos, S., & Madden, S. (2013). **The Design and Implementation of Modern Column-Oriented Database Systems**. *Foundations and Trends in Databases*, 5(3), 197–280. https://doi.org/10.1561/1900000024

2. Apache Arrow Community. (2016). **Apache Arrow: A Cross-Language Development Platform for In-Memory Data**. Apache Software Foundation. https://arrow.apache.org/

3. Lhoest, Q., Villanova del Moral, A., Jernite, Y., Thakur, A., von Platen, P., Patil, S., Chaumond, J., Drame, M., Plu, J., Tunstall, L., Davison, J., Šaško, M., Chhablani, G., Malik, B., Brandeis, S., Le Scao, T., Sanh, V., Xu, C., Patry, N., … Wolf, T. (2021). **Datasets: A Community Library for Natural Language Processing**. In *Proceedings of the 2021 Conference on Empirical Methods in Natural Language Processing: System Demonstrations* (pp. 175–184). ACL. https://doi.org/10.18653/v1/2021.emnlp-demo.21

4. Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023). **QLoRA: Efficient Finetuning of Quantized LLMs**. In *Advances in Neural Information Processing Systems*, 36. https://arxiv.org/abs/2305.14314

5. McGrew, D., & Viega, J. (2004). **The Galois/Counter Mode of Operation (GCM)**. NIST Submission. https://csrc.nist.gov/publications/detail/sp/800-38d/final

6. Krawczyk, H., Bellare, M., & Canetti, R. (1997). **HMAC: Keyed-Hashing for Message Authentication**. RFC 2104. IETF. https://doi.org/10.17487/RFC2104
