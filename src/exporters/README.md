> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Exporters Module

Data export functionality for ThemisDB.

## Module Purpose

Provides data export functionality for ThemisDB. Supported formats:

- **JSONL** — optimized for LLM training data (instruction/input/output, Alpaca/ShareGPT/ChatML)
- **Apache Parquet** — columnar export for analytics workloads
- **Apache Arrow IPC** — zero-copy pipelines (file and stream formats)
- **Hugging Face Datasets** — dataset card + Parquet shards for Hub upload
- **Streaming** — large-collection export without full in-memory buffering

## Subsystem Scope

**In scope:**

- Export formats: JSONL, Apache Parquet, Apache Arrow IPC, Hugging Face Datasets
- Configurable field selection (include/exclude per export)
- Batch and streaming export
- LoRA adapter metadata generation; vLLM multi-LoRA integration
- PII detection and redaction; multi-tenant isolation with scope-based authorization

**Out of scope:** Data transformation (handled by content module), import functionality (handled by importers module), data compression (delegated to utils/zstd).

## Relevant Interfaces

- `jsonl_llm_exporter.cpp` — primary JSONL export with LLM training format
- `parquet_exporter.cpp` — Apache Parquet columnar export
- `arrow_ipc_exporter.cpp` — Apache Arrow IPC file and stream export (zero-copy pipelines)
- `huggingface_exporter.cpp` — Hugging Face Datasets-compatible export (local filesystem)
- `huggingface_hub_client.cpp` — **HuggingFace Hub direct upload** with PolicyEngine + audit
- `streaming_exporter.cpp` — streaming export for large collections
- `stream_writer.cpp` — low-level streaming output writer
- `incremental_exporter.cpp` — delta/incremental export with watermark-based change tracking
- `aql_predicate_filter.cpp` — AQL predicate filtering to restrict exported records
- `format_template.cpp` — instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI)
- `export_encryption.cpp` — AES-256-GCM encryption for sensitive export data
- `pii_detector.cpp` — PII detection and redaction before export
- `data_augmentation.cpp` — synthetic data augmentation pipeline for training data diversity
- `join_exporter.cpp` — cross-collection hash-join export (inner join, output-field aliasing, AQL predicate, PII redaction)
- `exporter_metrics.cpp` — export throughput and quality metrics

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — JSONL, Parquet, Arrow IPC, Hugging Face, streaming, and cross-collection join export all operational.

## Components

- JSONL exporter for LLM training data
- Parquet columnar exporter
- Arrow IPC exporter (file and stream format)
- Hugging Face dataset exporter
- Streaming exporter for large collections
- Incremental/delta exporter with watermark-based change tracking
- Cross-collection join exporter (hash-join, output-field aliasing, AQL predicate filter)
- AQL predicate filter for record-level export filtering
- Instruction-tuning format templates (Alpaca, ShareGPT, ChatML, OpenAI)
- Export encryption (AES-256-GCM) for sensitive training data
- Synthetic data augmentation pipeline
- PII detection and redaction
- Export metrics and telemetry

## Features

- Export documents in JSONL format optimized for LLM training
- Export to Apache Parquet columnar format
- Export to Apache Arrow IPC file (`.arrow`) or stream (`.arrows`) format for zero-copy pipelines
- Hugging Face Datasets-compatible export (JSONL shards + `dataset_card.md` + `dataset_info.json`)
- **HuggingFace Hub direct upload** — push exported datasets to the Hub without manual steps
- **Cross-collection join export** — hash-join two collections into a single JSONL output; output-field aliasing, AQL join predicate, PII redaction, right-side memory budget ≤ 1 GiB
- Configurable field selection (include/exclude)
- Batch export operations
- Streaming export without full in-memory load
- Incremental/delta export: only records modified since the last export watermark
- AQL predicate filtering to restrict exported records without code changes
- Instruction-tuning format templates: Alpaca, ShareGPT, ChatML, OpenAI fine-tuning JSONL
- AES-256-GCM encryption for sensitive export data (key referenced by ID via HKDF-SHA256)
- Synthetic data augmentation (synonym replacement, back-translation stubs, paraphrase variants)
- LoRA adapter metadata generation
- PII detection and redaction (mask, hash, remove, partial)
- Multi-tenant isolation with scope-based authorization
- Progress callbacks with records exported, bytes written, and estimated ETA

## Hugging Face Hub Direct Upload

`HuggingFaceHubClient` (`include/exporters/huggingface_hub_client.h`) uploads a
Hugging Face Datasets-compatible directory — produced by `HuggingFaceExporter` — directly
to the Hub using libcurl.  Authentication, PolicyEngine authorization, and audit logging
are fully integrated.

### Basic upload

```cpp
#include "exporters/huggingface_hub_client.h"
using namespace themis::exporters;

HubUploadConfig cfg;
cfg.repo_id         = "my-org/my-dataset";      // Hub repo in owner/name form
cfg.hf_token        = "";                        // empty → read HF_TOKEN env var
cfg.commit_message  = "Add training data v2";
cfg.create_repo     = true;    // create if it doesn't exist
cfg.private_repo    = false;

HuggingFaceHubClient client(cfg);
HubUploadResult result = client.uploadDataset("/path/to/exported/dataset");

if (result.success) {
    // Dataset is available at result.dataset_url
} else {
    // result.error_message describes the failure
    // result.http_status is the last Hub API HTTP status (0 if no response)
}
```

### Upload with progress callback

```cpp
HubUploadResult result = client.uploadDataset(
    "/path/to/exported/dataset",
    [](double fraction) {
        std::cout << "Upload progress: " << (fraction * 100.0) << "%\n";
    });
```

### Upload with PolicyEngine authorization

```cpp
#include "exporters/huggingface_hub_client.h"
#include "governance/policy_engine.h"
using namespace themis::exporters;

// Obtain a pre-configured PolicyEngine instance (lifetime must exceed upload).
themis::governance::PolicyEngine& engine = getMyPolicyEngine();

HubUploadConfig cfg;
cfg.repo_id          = "my-org/sensitive-dataset";
cfg.hf_token         = "hf_...";
cfg.requesting_user  = "alice";      // forwarded to PolicyEngine + audit log
cfg.policy_engine    = &engine;      // null = no check (backward compat)

HuggingFaceHubClient client(cfg);
HubUploadResult result = client.uploadDataset(export_dir);

if (!result.success) {
    if (result.error_message.find("PolicyEngine") != std::string::npos) {
        // Upload was blocked by PolicyEngine::checkExportPermission()
    }
}
```

`PolicyEngine::checkExportPermission()` is called **before any HTTP activity**.  A
denied decision returns `success=false` immediately; no files are uploaded.

### Upload with audit logging

```cpp
#include "exporters/huggingface_hub_client.h"
#include "utils/audit_logger.h"
using namespace themis::exporters;

// Build a minimal AuditLogger (encryption + PKI can be null for basic use).
themis::utils::AuditLoggerConfig audit_cfg;
audit_cfg.log_path = "/var/log/themis/hub_upload.jsonl";
audit_cfg.enabled  = true;
auto audit_log = std::make_shared<themis::utils::AuditLogger>(
    nullptr, nullptr, audit_cfg);

HubUploadConfig cfg;
cfg.repo_id         = "my-org/my-dataset";
cfg.hf_token        = "hf_...";
cfg.requesting_user = "alice";
cfg.audit_log       = audit_log;    // null = no audit trail (backward compat)

HuggingFaceHubClient client(cfg);
client.uploadDataset(export_dir);

// The audit log will contain a JSON line like:
// {"event_type":"hub_upload","repo_id":"my-org/my-dataset",
//  "requesting_user":"alice","outcome":"success","http_status":200,...}
```

Every call to `uploadDataset()` appends one audit entry regardless of outcome
(success, policy denial, or network/HTTP error).

### Retry and timeout configuration

```cpp
HubUploadConfig cfg;
cfg.repo_id         = "my-org/my-dataset";
cfg.max_retries     = 5;        // default: 3
cfg.retry_delay_ms  = 500;      // initial delay, doubles each retry; default: 1000 ms
cfg.timeout_seconds = 60;       // curl connect+operation timeout; default: 120 s
```

### Memory-streaming upload (no filesystem required)

`uploadShards()` accepts pre-built in-memory shards and uploads them directly via a
libcurl read callback — **no temporary files are written to disk**.  This is the
recommended path for container / serverless environments with read-only or absent local
storage.

```cpp
#include "exporters/huggingface_hub_client.h"
using namespace themis::exporters;

// Build one shard per JSONL split in memory.
MemoryShardSpec shard;
shard.relative_path = "data/train-00000-of-00001.jsonl";  // path inside the Hub repo

const std::string jsonl = R"({"text":"hello world"})" "\n"
                          R"({"text":"foo bar"})" "\n";
shard.content.assign(jsonl.begin(), jsonl.end());

HubUploadConfig cfg;
cfg.repo_id  = "my-org/my-dataset";
cfg.hf_token = "hf_...";

HuggingFaceHubClient client(cfg);
HubUploadResult result = client.uploadShards({shard});

if (result.success) {
    // Dataset is live at result.dataset_url
}
```

All features available in `uploadDataset()` — progress callbacks, PolicyEngine
authorization, AuditLogger, and retry / timeout configuration — work identically with
`uploadShards()`.

```cpp
// Memory upload with progress + policy + audit (same config as uploadDataset())
HubUploadResult result = client.uploadShards(
    shards,
    [](double fraction) {
        std::cout << "Memory upload progress: " << (fraction * 100.0) << "%\n";
    });
```

Both `uploadDataset()` (disk path) and `uploadShards()` (memory path) share the same
`HubUploadConfig`, retry logic, and error surface — they are interchangeable upload
strategies backed by the same `httpPutBytes()` transport.

### Compile-time dependency

`HuggingFaceHubClient` requires libcurl.  When `CURL_ENABLED` is not defined at
compile time, both `uploadDataset()` and `uploadShards()` return immediately with
`success=false` and an explanatory `error_message`.  All other `HubUploadConfig` fields
remain available for configuration in both modes.

## Documentation

For exporter documentation, see:
- [Implementation Summary](../../docs/exporters/IMPLEMENTATION_SUMMARY.md)
- [P0 Implementation (Foundation)](../../docs/exporters/P0_IMPLEMENTATION.md)
- [P1/P2 Implementation (Security & Performance)](../../docs/exporters/P1_P2_IMPLEMENTATION.md)

## Scientific References

1. Abadi, D., Boncz, P., Harizopoulos, S., Idreos, S., & Madden, S. (2013). **The Design and Implementation of Modern Column-Oriented Database Systems**. *Foundations and Trends in Databases*, 5(3), 197–280. https://doi.org/10.1561/1900000024

2. Apache Arrow Community. (2016). **Apache Arrow: A Cross-Language Development Platform for In-Memory Data**. Apache Software Foundation. https://arrow.apache.org/

3. Vohra, D. (2016). **Apache Parquet**. Apress. https://doi.org/10.1007/978-1-4842-1592-5

4. Deutsch, L. P. (1996). **DEFLATE Compressed Data Format Specification version 1.3**. RFC 1951. IETF. https://doi.org/10.17487/RFC1951

5. Lhoest, Q., Villanova del Moral, A., Jernite, Y., Thakur, A., von Platen, P., Patil, S., Chaumond, J., Drame, M., Plu, J., Tunstall, L., Davison, J., Šaško, M., Chhablani, G., Malik, B., Brandeis, S., Le Scao, T., Sanh, V., Xu, C., Patry, N., … Wolf, T. (2021). **Datasets: A Community Library for Natural Language Processing**. In *Proceedings of the 2021 Conference on Empirical Methods in Natural Language Processing: System Demonstrations* (pp. 175–184). Association for Computational Linguistics. https://doi.org/10.18653/v1/2021.emnlp-demo.21

6. Dettmers, T., Pagnoni, A., Holtzman, A., & Zettlemoyer, L. (2023). **QLoRA: Efficient Finetuning of Quantized LLMs**. In *Advances in Neural Information Processing Systems*, 36. https://arxiv.org/abs/2305.14314

7. McGrew, D., & Viega, J. (2004). **The Galois/Counter Mode of Operation (GCM)**. NIST Submission. https://csrc.nist.gov/publications/detail/sp/800-38d/final

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/exporters/README.md`](../../include/exporters/README.md) for the public API.
