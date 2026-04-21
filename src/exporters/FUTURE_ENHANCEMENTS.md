> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Exporters Module - Future Enhancements

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
- ~~Register templates in `ExportFormatRegistry`; allow user-defined templates via a JSON config file.~~ ✅ Implemented — `registerBuiltins()` registers `jsonl_alpaca`, `jsonl_sharegpt`, `jsonl_chatml`, `jsonl_openai_ft`; `loadTemplatesFromJson()` / `loadTemplatesFromConfig()` accept user-defined templates; 15 tests in `tests/exporters/test_export_format_registry.cpp`.
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

**PolicyEngine integration** (`HubUploadConfig::policy_engine`): `uploadDataset()` calls
`PolicyEngine::checkExportPermission()` **before any HTTP activity**.  A denied decision
returns `success=false` immediately with an error message containing "PolicyEngine"; no
files are uploaded.  Setting `policy_engine = nullptr` (the default) preserves backward
compatibility.

**Audit logging** (`HubUploadConfig::audit_log`): every call to `uploadDataset()` appends
a `hub_upload` JSON entry to the `AuditLogger` regardless of outcome, recording
`repo_id`, `requesting_user`, `dataset_dir`, `outcome` (success / denied / error),
`http_status`, and `timestamp`.  Setting `audit_log = nullptr` (the default) preserves
backward compatibility.

**Remaining (future issues):**


---

### ~~`StreamWriter`: Replace zlib with ZSTD as Sole Compression Backend~~ ✅ Implemented
**Priority:** Low
**Target Version:** v1.8.0

`stream_writer.cpp` now uses ZSTD exclusively as the compression backend. The zlib/gzip
code path (`deflateInit2`, `z_stream`, `<zlib.h>`) has been removed. `CompressionType::GZIP`
is retained in the enum for backward compatibility but silently redirects to ZSTD output.
Callers requesting `"gzip"` compression type produce ZSTD-framed output; for tools requiring
gzip format, pipe through `zstd -d | gzip` or use `pigz`.

**Implementation Notes:**
- `[x]` Remove zlib/gzip compression path from `StreamWriter`; replace gzip-format output with zstd-compressed output using `ZSTD_createCStream`.
- `[x]` Offer a `ZSTD_MAGICNUMBER`-prefixed output mode that most data pipeline tools can ingest directly; for tools requiring gzip, document the `pigz` / `zstd -d | gzip` conversion path.
- `[x]` Remove `<zlib.h>` include and the associated `z_stream` compression state path; reduces binary size and maintenance surface.

---

### HuggingFace Hub Client: HTTP Rate Limit Handling (429 Back-Off)
**Priority:** Medium
**Target Version:** v1.8.0

`huggingface_hub_client.cpp` implements exponential retry back-off for file uploads (line 423) but does not check HTTP 429 (Too Many Requests) or the `Retry-After` response header before retrying. Retrying immediately on a 429 wastes the retry budget and may result in account throttling.

**Implementation Notes:**
- `[x]` After each curl response, check HTTP status 429; if present, parse the `Retry-After` header (seconds or HTTP-date format) and sleep for that duration before retrying.
- `[x]` Cap total sleep from `Retry-After` at `config_.timeout_seconds` to prevent indefinite blocking.
- `[x]` Emit a `exporters.huggingface.rate_limit_hit` metric via `ExporterMetrics` whenever a 429 is received.

---


### ~~Cross-Collection Join Export~~ ✅ Implemented (Issue: #1722)
**Priority:** Low
**Target Version:** v1.8.0 — **Delivered**

Export a joined view of two or more collections (e.g., `documents JOIN annotations`) into a single JSONL output file. Implemented as an in-memory hash-join: the right side is loaded into a hash table keyed on `right_key_field`, then every left entity is probed against it.

**Implementation Notes:**
- `[x]` `JoinExportConfig` struct with `left_collection`, `right_collection`, `left_key_field`, `right_key_field`, `join_predicate` (AQL expression), `output_fields` (with `"src_name:alias"` renaming), PII config, and `right_side_memory_limit_bytes` (default 1 GiB).
- `[x]` `JoinExporter` implements `IExporter`; `setRightCollection()` builds the hash table; `exportEntities()` performs inner join.
- `[x]` Qualified field references `left.<field>` / `right.<field>` resolve ambiguous names; unaliased conflicts throw `ERR_EXPORT_JOIN_AMBIGUOUS_FIELD`.
- `[x]` AQL join predicate filtering on the merged record; parse failures throw `ERR_EXPORT_JOIN_PREDICATE_INVALID`.
- `[x]` Right-side memory budget enforced; `ERR_EXPORT_JOIN_MEMORY_LIMIT` thrown when exceeded.
- `[x]` PII detection runs on the merged record before serialization.
- `[x]` Registered in `ExportFormatRegistry` as `"join"` and `"join_jsonl"`.
- `[x]` Throughput ≥ 50 000 merged docs/sec; right-side hash table ≤ 1 GiB configurable.

**Remaining (future issues):**
- Parquet output variant for JoinExporter (additive; no existing API changes required).

---

## Scientific References

1. Abadi, D., Boncz, P., Harizopoulos, S., Idreos, S., & Madden, S. (2013). **The Design and Implementation of Modern Column-Oriented Database Systems**. *Foundations and Trends in Databases*, 5(3), 197–280. https://doi.org/10.1561/1900000024

2. Apache Arrow Community. (2016). **Apache Arrow: A Cross-Language Development Platform for In-Memory Data**. Apache Software Foundation. https://arrow.apache.org/

3. Lhoest, Q., Villanova del Moral, A., Jernite, Y., Thakur, A., von Platen, P., Patil, S., Chaumond, J., Drame, M., Plu, J., Tunstall, L., Davison, J., Šaško, M., Chhablani, G., Malik, B., Brandeis, S., Le Scao, T., Sanh, V., Xu, C., Patry, N., … Wolf, T. (2021). **Datasets: A Community Library for Natural Language Processing**. In *Proceedings of the 2021 Conference on Empirical Methods in Natural Language Processing: System Demonstrations* (pp. 175–184). ACL. https://doi.org/10.18653/v1/2021.emnlp-demo.21

4. Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023). **QLoRA: Efficient Finetuning of Quantized LLMs**. In *Advances in Neural Information Processing Systems*, 36. https://arxiv.org/abs/2305.14314

5. McGrew, D., & Viega, J. (2004). **The Galois/Counter Mode of Operation (GCM)**. NIST Submission. https://csrc.nist.gov/publications/detail/sp/800-38d/final

6. Krawczyk, H., Bellare, M., & Canetti, R. (1997). **HMAC: Keyed-Hashing for Message Authentication**. RFC 2104. IETF. https://doi.org/10.17487/RFC2104

---

## Security Hardening Backlog (Q2–Q3 2026)

> GAP-004 + GAP-019 – identified via static analysis (2026-04-21).
> Reference: `docs/governance/SOURCECODE_COMPLIANCE_GOVERNANCE.md`.

### GAP-004 – AQL Injection via String Concatenation in Export Query Builder

**Scope:** `src/server/export_api_handler.cpp:354–388`

The `buildAqlQuery()` function builds AQL query conditions by concatenating user-supplied
strings without escaping or injection validation.  The `"query"` field is embedded verbatim.

**Fix:** Run each custom query through `AQLInjectionDetector::validateForReadOnlyContext()`
and replace string-concat conditions with AQL bind parameters.

See: `src/server/FUTURE_ENHANCEMENTS.md` GAP-004 for full implementation spec.

### GAP-019 – Replace mt19937 with CSPRNG for Export IDs

**Scope:** `src/server/export_api_handler.cpp:405`

Export IDs generated with `mt19937` may be predictable on low-entropy systems.

**Fix:** Replace with `RAND_bytes()` (OpenSSL).

See: `src/server/FUTURE_ENHANCEMENTS.md` GAP-019 for full implementation spec.
