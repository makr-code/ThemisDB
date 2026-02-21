# Ingestion Module - Future Enhancements

## Scope

This document covers planned enhancements to the Ingestion module beyond what is tracked in `ROADMAP.md`. It focuses on `ingestion_manager.cpp`, `api_connector.cpp`, `filesystem_ingester.cpp`, and `huggingface_connector.cpp`. Features here describe the concrete engineering work required to add new source connectors (Kafka, S3, CDC), replace the stubbed `libcurl` HTTP client with a production implementation, introduce distributed ingestion coordination, and harden existing connectors for production deployments.

## Design Constraints

- All new source connectors must implement the same `ISourceConnector` interface already consumed by `IngestionManager`; they must support the existing `RateLimitConfig`, `RetryConfig`, `pause/resume`, and incremental checkpoint lifecycle.
- The `IngestionBuilder` fluent API must remain backward-compatible; new source types are added via new `withXxxSource()` builder methods and must not change the signatures of existing methods.
- The quarantine queue must capture persistently failing documents from every connector type; no connector may silently discard documents without either writing them to the database or to the quarantine queue with an error reason.
- HTTP clients in `api_connector.cpp` and `huggingface_connector.cpp` currently use stubs; production replacements must use `libcurl` with TLS certificate verification enabled by default and a configurable CA bundle path.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ISourceConnector::fetchBatch(checkpoint, batch_size)` | `IngestionManager::ingestAll()` | All new connectors must implement this; drives the common retry + checkpoint + quarantine loop |
| `IngestionCheckpointStore::commit(source_id, offset)` | `IngestionManager`, all connectors | Currently file-backed; must support a pluggable backend (local file, Redis, database) for distributed deployments |
| `HttpClient::get(url, headers)` | `GenericApiConnector`, `HuggingFaceConnector` | Production implementation using `curl_easy_perform`; replaces current stubs in `api_connector.cpp` and `huggingface_connector.cpp` |
| `IngestionMetricsExporter::exportText(report)` | Prometheus scrape endpoint, admin API | Already defined in `ingestion_manager.cpp`; extend with per-error-code breakdown labels |
| `IngestionAdminApi::retryQuarantineItem(doc_id)` | Admin REST API | Must support per-document retry (not full source restart) |

## Planned Features

### Production libcurl HTTP Client
**Priority:** High
**Target Version:** v1.6.0

Replace the simulated HTTP response stubs in `api_connector.cpp` and `huggingface_connector.cpp` with a real `libcurl`-based `HttpClient` class. The stubs currently return hardcoded JSON payloads, making both connectors non-functional in production deployments.

**Implementation Notes:**
- Add `http_client.cpp` with a `HttpClient` class wrapping `CURL*` handles from a per-thread handle pool (avoids per-request `curl_easy_init` overhead).
- Use `CURLOPT_CAINFO` for TLS certificate verification; default to the system CA bundle; allow override via `IngestionConfig::ca_bundle_path`. Never set `CURLOPT_SSL_VERIFYPEER = 0` in production code paths.
- Implement `CURLOPT_WRITEFUNCTION` callback that streams response bytes directly into the JSON parser buffer to avoid a double-copy.
- Map HTTP status codes to `IngestionErrorCode` values consistently: 429 → `RATE_LIMITED`, 401/403 → `AUTH_FAILED`, 5xx → `SERVER_ERROR`, timeout → `TIMEOUT`; these drive the existing exponential back-off retry logic in `IngestionManager`.
- Add `CURLOPT_TIMEOUT_MS` and `CURLOPT_CONNECTTIMEOUT_MS` configurable via `RetryConfig::request_timeout_ms` (default 30 000 ms).

**Performance Targets:**
- HTTP GET round-trip to a local test server ≤ 5 ms overhead vs. raw TCP (measured with `benchmarks/ingestion_bench.cpp`).
- Handle pool of 16 reusable CURL handles per thread; handle acquisition must not block under normal load.

---

### Kafka Consumer Source Connector
**Priority:** High
**Target Version:** v1.7.0

Add a `KafkaConnector` that consumes documents from one or more Kafka topics and ingests them into ThemisDB. This enables real-time data intake from event-driven systems without polling REST APIs.

**Implementation Notes:**
- Add `kafka_connector.cpp` implementing `ISourceConnector`; use `librdkafka` C API for consumer group management.
- Consumer group ID is configurable via `SourceConfig::options["consumer_group"]`; offset commit is tied to `IngestionCheckpointStore::commit()` so that ThemisDB-level checkpoints and Kafka offsets are kept in sync.
- Add `IngestionBuilder::withKafkaSource(source_id, brokers, topic, options, priority)` to the fluent API.
- Support both JSON-encoded and Avro-encoded messages; Avro requires a Schema Registry URL configured via `options["schema_registry_url"]`.
- Graceful shutdown: on `IngestionManager` stop, call `rd_kafka_consumer_close()` to commit final offsets before the process exits.

**Performance Targets:**
- Kafka consumer throughput ≥ 100 000 messages/sec (1 KB average message) with a single-partition topic.
- End-to-end latency from Kafka message publish to ThemisDB document available ≤ 500 ms p99.

---

### S3-Compatible Object Storage Source Connector
**Priority:** Medium
**Target Version:** v1.7.0

Add an `S3Connector` that lists and downloads objects from an S3-compatible bucket (AWS S3, MinIO, GCS via S3 interop) and ingests them as documents. This supports batch ingestion of data lake files without requiring a local copy.

**Implementation Notes:**
- Add `s3_connector.cpp` implementing `ISourceConnector`; use the AWS C++ SDK (`aws-sdk-cpp`) or, for lighter-weight builds, a minimal S3 client built on the `HttpClient` class introduced above.
- Incremental mode: checkpoint the last processed `LastModified` timestamp or lexicographic key prefix; on restart, use `ListObjectsV2` with a `StartAfter` marker.
- Support all flat-file formats handled by the Importers module flat-file path (`.jsonl`, `.csv`, `.parquet`) by delegating parsing to `FileSystemIngester`'s format readers.
- Configurable `max_keys_per_list` (default 1 000) and `max_concurrent_downloads` (default 4) to balance throughput against memory pressure.

**Performance Targets:**
- S3 object listing overhead ≤ 100 ms per 1 000 objects (single `ListObjectsV2` call).
- Concurrent download throughput ≥ 200 MB/s aggregate with 4 parallel downloads on a 10 Gbps network.

---

### Distributed Ingestion Coordinator
**Priority:** Medium
**Target Version:** v1.8.0

Enable `IngestionManager` to distribute source processing across multiple ThemisDB nodes so that large ingestion jobs are not bottlenecked on a single instance. The coordinator partitions sources and page ranges across worker nodes and aggregates `IngestionReport` results.

**Implementation Notes:**
- Add `IngestionCoordinator` class in `ingestion_coordinator.cpp`; acts as the leader that partitions work via consistent hashing of `source_id` across available worker nodes.
- Workers receive their assigned sources via a gRPC `IngestRequest` (new proto definition in `proto/ingestion_coordinator.proto`); they run the existing `IngestionManager::ingestAll()` locally and stream progress events back to the coordinator.
- `IngestionCheckpointStore` must switch to a shared backend (Redis or the ThemisDB checkpoint collection) so that all workers see the same incremental progress state.
- Leader election uses a lightweight lease mechanism (TTL-based lock in the checkpoint collection) to avoid split-brain during coordinator failover.

**Performance Targets:**
- Linear throughput scaling to at least 4 worker nodes (≥ 3.5× aggregate throughput vs single node) for API and filesystem sources.
- Coordinator overhead (partitioning + progress aggregation) ≤ 5 % of total ingestion wall-clock time.

---

### Per-Document Quarantine Retry
**Priority:** Low
**Target Version:** v1.6.0

Enhance `IngestionAdminApi::retryQuarantineItem(doc_id)` to retry a single quarantined document without triggering a full source re-ingestion. Currently the retry mechanism re-runs the entire source from the last checkpoint, which is wasteful for isolated document failures.

**Implementation Notes:**
- Store the raw document payload in each `QuarantineEntry` (currently only the error metadata is stored); add a `raw_payload` field to `QuarantineEntry` struct in `ingestion_manager.cpp`.
- `retryQuarantineItem(doc_id)` deserializes the stored payload, re-applies the connector's `transformDocument()` step, and attempts a single write to the database.
- On success, remove the entry from the quarantine queue and emit `ingestion_quarantine_retry_success_total`; on failure, increment `retry_count` and update `last_error`.
- Add a `max_quarantine_retries` config option (default 5); documents exceeding the limit are marked `PERMANENTLY_FAILED` and excluded from future retry attempts.

**Performance Targets:**
- Per-document retry overhead ≤ 10 ms (deserialization + write) for documents ≤ 1 MB.
- Quarantine queue scan for `retryAll()` ≤ 1 s for queues with up to 100 000 entries.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | `HttpClient` tested with a mock `libcurl` handle that returns controlled responses; `KafkaConnector` tested with `librdkafka` mock consumer; per-document retry tested with injected write failures |
| Integration | All connectors end-to-end | `GenericApiConnector` integration test against a local `httpbin` container; `HuggingFaceConnector` against a mock HuggingFace API server; Kafka connector against a `confluentinc/cp-kafka` Docker container |
| Performance | Throughput regression < 5% on existing connectors | `benchmarks/ingestion_bench.cpp` runs in CI on every PR touching `ingestion_manager.cpp`; Kafka throughput benchmark added alongside `KafkaConnector` implementation |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| API connector throughput (stub) | Simulated | ≥ 10 000 docs/sec (real libcurl) | `benchmarks/ingestion_bench.cpp` against local mock API |
| Kafka consumer throughput | N/A | ≥ 100 000 msgs/sec | librdkafka benchmark with single-partition topic |
| S3 aggregate download throughput | N/A | ≥ 200 MB/s (4 concurrent) | MinIO local instance benchmark |
| Per-document quarantine retry | N/A (full source restart) | ≤ 10 ms per document | Micro-benchmark in `tests/ingestion/bench_quarantine.cpp` |

## Security / Reliability

- TLS certificate verification must be enabled in the `HttpClient` production path; `CURLOPT_SSL_VERIFYPEER = 0` is prohibited outside of explicitly flagged test-only compilation units.
- API keys and bearer tokens stored in `SourceConfig::options` must not appear in log output at any log level; `IngestionManager` sanitizes the options map before emitting log entries.
- The `S3Connector` must validate that downloaded file paths do not escape the configured bucket prefix (path-traversal guard) before passing bytes to the file parser.
- Kafka consumer group offsets are committed only after a document is durably written to ThemisDB; at-least-once delivery semantics are preserved; idempotency at the storage layer handles duplicates.
- Quarantine entries containing raw document payloads may contain PII; the quarantine collection must be governed by the same `PolicyEngine` access controls as production collections.
