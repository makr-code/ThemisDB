# Exporters Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/exporters/`

---

## 1. Overview

The Exporters module provides data export functionality for ThemisDB, with a focus on
LLM training data pipelines. Its primary output format is JSONL (JSON Lines) optimized for
fine-tuning workflows, with LoRA adapter metadata generation and vLLM multi-LoRA integration.
Parquet and HuggingFace dataset formats are also supported.

---

## 2. Design Principles

- **Export-Only** – this module reads data and writes files/streams; it never modifies
  the source data.
- **PII Awareness** – `pii_detector.cpp` scans exported fields before writing, enabling
  configurable PII masking or exclusion.
- **Streaming** – `stream_writer.cpp` supports streaming export to avoid loading entire
  result sets into memory.
- **Training-Format Optimized** – JSONL output uses instruction/input/output fields
  matching standard fine-tuning dataset conventions (Alpaca, ShareGPT, ChatML).
- **LoRA-Ready** – `jsonl_llm_exporter.cpp` generates LoRA adapter metadata alongside the
  training data.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `jsonl_llm_exporter.cpp` | JSONL export: instruction/input/output format, batching, LoRA metadata |
| `huggingface_exporter.cpp` | HuggingFace dataset format export (Dataset card + JSONL shards) |
| `parquet_exporter.cpp` | Apache Parquet columnar export |
| `arrow_ipc_exporter.cpp` | Apache Arrow IPC file (`.arrow`) and stream (`.arrows`) export for zero-copy pipelines |
| `streaming_exporter.cpp` | Streaming export for large collections (avoids full in-memory load) |
| `stream_writer.cpp` | Low-level streaming output writer (avoids full in-memory buffering) |
| `incremental_exporter.cpp` | Delta/incremental export: exports only records modified since last watermark |
| `aql_predicate_filter.cpp` | AQL predicate filtering to restrict exported records at query time |
| `format_template.cpp` | Instruction-tuning format templates: Alpaca, ShareGPT, ChatML, OpenAI |
| `export_encryption.cpp` | AES-256-GCM encryption for exported data; key material referenced by ID |
| `data_augmentation.cpp` | Synthetic data augmentation pipeline (synonym replacement, paraphrase) |
| `pii_detector.cpp` | PII detection and masking before export |
| `exporter_metrics.cpp` | Export throughput, record count, PII hit rate metrics |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Caller (API handler, CLI, training pipeline)        │
│   ExportRequest { collection, fields, format, pii_policy }      │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     Export Pipeline                              │
│                                                                  │
│  Query storage → batch records → PII scan → format → write      │
│                                                                  │
│  ┌──────────────────┐  ┌────────────────────────────────────┐  │
│  │  PII Detector    │  │          Format Writers            │  │
│  │  (mask/exclude)  │  │  JSONL│Parquet│Arrow IPC│HuggingFace│  │
│  └──────────────────┘  └────────────────────────────────────┘  │
│                                                                  │
│  StreamWriter (chunked I/O, configurable buffer size)           │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 JSONL LLM Training Export

```
ExportRequest { collection: "qa_pairs", format: "jsonl_alpaca", pii: "mask" }
    │
    ▼
Query storage: iterate collection records in batches
    │
    ▼
PII Detector: scan each record's text fields
    ├─ PII found → mask (e.g. "[EMAIL]") or exclude record
    └─ clean → pass through
    │
    ▼
JSONL formatter:
    {"instruction": "...", "input": "...", "output": "..."}
    │
    ▼
StreamWriter: write line to output file/S3/pipe
    │
    ▼
LoraMetadataWriter: generate adapter_config.json + README.md
```

### 4.2 HuggingFace Dataset Export

```
ExportRequest { format: "huggingface", split: "train/test=90/10" }
    │
    ▼
Split records by ratio → train shard, test shard
    │
    ▼
ParquetExporter: write sharded Parquet files
    │
    ▼
HuggingFaceExporter: generate dataset_card.md + dataset_info.json
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Reads from** | `src/storage/` | Collection iteration and range scans |
| **Uses** | `src/utils/` | PII detection, compression utilities |
| **Consumed by** | `src/server/` | Export API endpoints |
| **Consumed by** | `src/training/` | Training data pipeline |
| **Outputs to** | Filesystem / S3 | Export destination |

---

## 6. Threading & Concurrency Model

- Export operations are single-threaded per export job; concurrent jobs are managed by
  the caller's thread pool.
- `StreamWriter` is not thread-safe; use one instance per export job.
- `PiiDetector` is stateless and safe for concurrent invocation.
- `exporter_metrics.cpp` uses lock-free atomic counters.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Streaming I/O | `stream_writer.cpp` writes in configurable chunks (default: 64 KB) |
| Batch record reads | Storage reads use large batches to reduce I/O round-trips |
| Parquet row groups | Configurable row group size for optimal Parquet read performance |

---

## 8. Security Considerations

- PII detection (`pii_detector.cpp`) scans for email, phone, SSN, and credit card patterns.
- PII policy is configurable per export: `mask`, `exclude`, or `allow` (with audit log).
- Export destinations are validated to prevent path traversal and SSRF.
- Exported files inherit the requesting tenant's data access scope.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `exporters.batch_size` | 1000 | Records per storage read batch |
| `exporters.stream_buffer_bytes` | 65536 | Stream writer buffer size |
| `exporters.pii.enabled` | true | Enable PII detection |
| `exporters.pii.policy` | "mask" | PII policy: mask / exclude / allow |
| `exporters.parquet.row_group_size` | 10000 | Parquet row group size |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Storage read failure | Abort export; log error; return partial file with error manifest |
| PII detection failure | Fail export (safe default); configurable to warn and continue |
| Output write failure | Abort export; log error; clean up partial file |
| Out of disk space | Abort export; return structured error |

---

## 11. Known Limitations & Future Work

- Streaming export to S3 and GCS is planned (currently filesystem only).
- Hugging Face Hub direct upload integration is planned (currently filesystem export only).
- Export scheduling (cron-based exports) is handled by the scheduler module.
- Parquet and HuggingFace exporters have known edge cases in nested JSON types.

---

## 12. References

- `src/exporters/README.md` — module overview
- `docs/exporters/IMPLEMENTATION_SUMMARY.md` — complete implementation summary
- `docs/exporters/P0_IMPLEMENTATION.md` — foundation layer documentation
- `docs/exporters/P1_P2_IMPLEMENTATION.md` — security and performance documentation
- `ARCHITECTURE.md` (root) — full system architecture
