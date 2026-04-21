> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/ingestion/ -->

# Ingestion Module - Future Enhancements
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md · ../../docs/de/ingestion/README.md -->

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

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | ≥ 80% line coverage on new connector code | `HttpClient` tested with a mock `libcurl` handle returning controlled responses; `KafkaConnector` tested with `librdkafka` mock consumer; per-document retry tested with injected write failures |
| Integration | All connector types exercise a real backend end-to-end | `GenericApiConnector` against a local `httpbin` container; `KafkaConnector` against a `confluentinc/cp-kafka` Docker instance; `S3Connector` against a local MinIO instance |
| Throughput | Aggregate ingestion throughput ≥ 50 000 docs/sec on a single node | `benchmarks/ingestion_bench.cpp`; throughput regression gate of < 5% vs. previous baseline runs in CI on every PR touching `ingestion_manager.cpp` |
| Property-based | Quarantine retry logic handles all error code permutations without silent data loss | Property-based test generates random `IngestionErrorCode` sequences and asserts that every document either reaches the database or the quarantine queue |
| Checkpoint | Connector restarts resume from the last committed checkpoint without re-ingesting already-processed documents | Simulated crash-and-restart test for HTTP, Kafka, and S3 connector types |

## Planned Features

### SoC Refactoring: Ingestion/AI Separation (DIP)
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (2026-04-15)

The ingestion module no longer includes any `llm/` headers directly.  The concrete
LLM backend is injected via the new `ITextGenerationBackend` interface
(`include/ingestion/inference_backend.h`).  The cross-module binding lives in
`LlmIngestionBridge` (`include/llm/llm_ingestion_bridge.h` / `src/llm/llm_ingestion_bridge.cpp`)
which wraps `LLMPluginManager::instance().generate()`.

Wiring code (main/server bootstrap) creates an `LlmIngestionBridge` and passes
it to `LegalLlmAdapter(bridge)`.  12 tests in `tests/test_ingestion_inference_backend.cpp`
(IB-01..IB-12).

---

### `LLMIngestionAdapter` Phase 2: Wire llama.cpp
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented (superseded by SoC refactoring above)

`ingestion/llm_adapter.cpp` uses the injected `ITextGenerationBackend`
(formerly called `LLMPluginManager::instance().generate()` directly).
The prompt assembly and JSON parsing logic is unchanged; only the backend
call is now abstracted through the interface.

**Implementation Notes:**
- `[x]` SoC: replaced direct `llm/` includes with `ITextGenerationBackend` injection.
- `[x]` Replace the naive line-by-line JSON parser with `nlohmann::json::parse()` — locates the outermost `{…}` block to handle LLM preamble noise, then extracts `deontic_category`, `confidence`, and `entities`.
- `[x]` Add integration tests for `LLMIngestionAdapter` with a real (small) GGUF model file (see `tests/test_ingestion_llm_adapter.cpp`, skipped with `GTEST_SKIP` when no model is available).

---


**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (Issue: INGESTION-MISSING-001, PR: 2026-03-11)

Replace the simulated HTTP response stubs in `api_connector.cpp` and `huggingface_connector.cpp` with a real `libcurl`-based `HttpClient` class. The stubs currently return hardcoded JSON payloads, making both connectors non-functional in production deployments.

**Implementation Notes:**
- `hfHttpGet()` / `hfHttpPost()` in `huggingface_connector.cpp` and `apiHttpGet()` / `apiHttpPost()` in `api_connector.cpp` use `curl_easy_perform` with TLS verification, Bearer-Token header, and configurable timeout.
- `RetryConfig::ca_bundle_path` (std::string, default empty) added to `include/ingestion/ingestion_manager.h`. When non-empty, `CURLOPT_CAINFO` is set to override the system CA bundle. `CURLOPT_SSL_VERIFYPEER = 1L` is always enabled.
- `ca_bundle_path` is parsed from `SourceConfig::options["ca_bundle_path"]` in both `initialize()` methods and can also be set directly via `setRetryConfig()`.
- Both connectors expose `setHttpGetForTesting()` / `setHttpPostForTesting()` injection points so unit tests run without network access.

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
**Status:** ✅ Implemented (Issue: #178, PR: 2026-03-14)

Add an `S3Connector` that lists and downloads objects from an S3-compatible bucket (AWS S3, MinIO, GCS via S3 interop) and ingests them as documents. This supports batch ingestion of data lake files without requiring a local copy.

**Implementation Notes:**
- [x] `s3_connector.cpp` / `s3_connector.h` implementing `ISourceConnector`; uses the AWS C++ SDK (`aws-sdk-cpp`) when `THEMIS_ENABLE_S3` is defined; falls back gracefully to `CONNECTOR_NOT_SUPPORTED` (or mock path) when not.
- [x] Incremental mode: `IngestionCheckpoint::cursor` stores the last processed object key; on restart, `ListObjectsV2` `StartAfter` is set to the stored cursor.  `setCheckpointStore()` injection mirrors the Kafka connector pattern.
- [x] Flat-file format delegation: objects with `.jsonl`, `.csv`, `.parquet`, `.json`, `.txt`, `.html`, `.xml` extensions are written to a temporary file and parsed by `FileSystemIngester`'s format readers; raw body fallback for unknown extensions.
- [x] `max_keys_per_list` (default 1 000) controls the `MaxKeys` parameter of each `ListObjectsV2` call.  `max_concurrent_downloads` (default 4) issues parallel `GetObject` requests using `std::async`.
- [x] `IngestionManager::ingestSource()` routes `OBJECT_STORAGE` sources with `provider == "s3"` to `S3Connector`; GCS / Azure continue to use `ObjectStorageConnector`.
- [x] 32 unit tests in `tests/test_s3_connector.cpp` → `S3ConnectorFocusedTests` (no AWS credentials required; all tests use mock injection).

**Performance Targets:**
- S3 object listing overhead ≤ 100 ms per 1 000 objects (single `ListObjectsV2` call).
- Concurrent download throughput ≥ 200 MB/s aggregate with 4 parallel downloads on a 10 Gbps network.

---

### Distributed Ingestion Coordinator
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented

Enable `IngestionManager` to distribute source processing across multiple ThemisDB nodes so that large ingestion jobs are not bottlenecked on a single instance. The coordinator partitions sources and page ranges across worker nodes and aggregates `IngestionReport` results.

**Implementation Notes:**
- `[x]` Add `IngestionCoordinator` class in `ingestion_coordinator.cpp`; acts as the leader that partitions work via consistent hashing of `source_id` across available worker nodes.
- `[x]` Workers receive their assigned sources via a gRPC `IngestRequest` (new proto definition in `proto/ingestion_coordinator.proto`); they run the existing `IngestionManager::ingestAll()` locally and stream progress events back to the coordinator.
- `[x]` `ISharedCheckpointStore` interface + `InMemorySharedCheckpointStore` added to `ingestion_coordinator.h/.cpp`; `setSharedCheckpointStoreForTesting()` allows injection of a custom shared backend so all workers see the same incremental progress state.
- `[x]` Leader election uses a lightweight TTL-based lease mechanism (`InProcessLeaderElection`) to avoid split-brain during coordinator failover.

**Performance Targets:**
- `[x]` Linear throughput scaling to at least 4 worker nodes (≥ 3.5× aggregate throughput vs single node) for API and filesystem sources. (AC-COORD-5, `THEMIS_RUN_PERF_TESTS=1` gated)
- `[x]` Coordinator overhead (partitioning + progress aggregation) ≤ 5 % of total ingestion wall-clock time. (AC-COORD-6, `THEMIS_RUN_PERF_TESTS=1` gated)

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

### Plugin API for Third-Party Source Connectors
**Priority:** Medium
**Target Version:** v1.5.0
**Status:** ✅ Implemented (Issue: #1908)

Enable third-party code to register custom source connectors at runtime without modifying `IngestionManager` or recompiling the core library.  The plugin author implements `ISourceConnector`, registers a zero-argument factory, and the existing ingestion pipeline drives it identically to first-party connectors.

**Implementation Notes:**
- `ConnectorFactory` (`std::function<std::unique_ptr<ISourceConnector>()>`) is the factory type stored in the registry and forwarded to `IngestionManager`.
- `ConnectorPluginRegistry` (declared in `ingestion_manager.h`, implemented in `ingestion_manager.cpp`) is a thread-safe `unordered_map` mapping plugin names to `ConnectorFactory` callables.  All operations are protected by an internal `std::mutex`.
- `IngestionManager` owns one `ConnectorPluginRegistry` instance and exposes three public methods: `registerConnectorPlugin(name, factory)`, `unregisterConnectorPlugin(name) → bool`, and `listConnectorPlugins() → vector<string>` (sorted).
- `SourceType::PLUGIN` is the new enum value that routes a source to the plugin path in `Impl::ingestSource()`.  The source must carry `options["plugin_name"]` naming the registered factory; missing or unknown names produce `IngestionErrorCode::CONNECTOR_NOT_SUPPORTED` errors without aborting the entire ingestion run.
- `IngestionBuilder` fluent API additions: `withConnectorPlugin(name, factory)` stores the factory for transfer to the manager on `build()`; `withPluginSource(source_id, plugin_name, location, options, priority)` adds a `SourceType::PLUGIN` source config with `plugin_name` stored in `options`.
- `IngestionMetricsExporter::exportText(stats, source_id, "PLUGIN")` includes `source_type="PLUGIN"` in all emitted Prometheus labels.

**Security Notes:**
- Connector factories are caller-supplied code; no sandbox isolation is provided.  Deploy third-party connectors only from trusted sources.
- `options["plugin_name"]` values are looked up in the in-process registry and never evaluated as file paths or shell commands.

**Test Coverage:**
- 14 unit tests in `tests/test_ingestion_plugin_api.cpp` covering registry CRUD, concurrent isolation (distinct instances per `create()`), sorted listing, `ingestSource` success and all error paths (`missing plugin_name`, unregistered name, init failure), and the full `IngestionBuilder` fluent API.

---



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
- **WebCrawlerConnector**: only `http://` and `https://` seed URLs and link targets are permitted; all other URI schemes (`file://`, `ftp://`, `data:`, `ldap://`, etc.) are rejected at `initialize()` and `resolveUrl()` to prevent SSRF attacks.

---

## Identified Gaps (from AI_ML_IMPACT_ASSESSMENT.md)

### Gap 8 — Data Classification Gate for External Connectors (HuggingFaceConnector) (Target: Q3 2026)

**Source:** `AI_ML_IMPACT_ASSESSMENT.md §7, Gap 8 (Severity: Medium/S1)`
**Status:** ✅ Implemented (2026-04-21) — policy gate via `ModelGovernancePolicy`.

**Problem (resolved):** `HuggingFaceConnector::initialize()` fetched datasets without
any classification gate, allowing PII or restricted data to enter collections unchecked.

**Implemented changes:**
- `HuggingFaceConnector::setIngestionPolicy(shared_ptr<ModelGovernancePolicy>)` added.
- `initialize()` now calls `ModelGovernancePolicy::checkExportPermission()` with
  `purpose="DATA_INGESTION"` before any HTTP request.  DENY → returns `false` + ERROR log.
  PERMIT → continues with existing connector logic + INFO log.
- `config.options["classification"]` is forwarded to the governance request; defaults
  to `"offen"` when not provided.
- No policy set (nullptr) → WARN log + existing behavior (backward compatible).
- Tests: `test_huggingface_connector_governance.cpp` (HFC_GOV_01..05) registered as
  `HuggingFaceConnectorGovernanceFocusedTests`.

**Deferred (Q4 2026):**
- `DataClassificationGate` dedicated class (full domain/URL-based restriction).
- `IngestionConfig::require_classification_gate` enforcement flag.
- Same gate applied to `WebCrawlerConnector` and future external connectors.

**Inputs:** `SourceConfig::location` (HuggingFace dataset ID); `classification` option.
**Outputs:** `initialize()` returns `false` on DENY.
**Perf target:** Gate adds ≤ 2 ms (synchronous local policy lookup; no network).

---

## Implemented Connectors (as of v1.5.0)

The following connectors from this enhancement document have been implemented and are production-ready:

| Connector | File | Status |
|-----------|------|--------|
| Production libcurl HTTP Client | `api_connector.cpp` | ✅ Implemented |
| Kafka Consumer Source | `kafka_connector.cpp` | ✅ Implemented |
| S3-Compatible Object Storage | `object_storage_connector.cpp` | ✅ Implemented |
| JDBC-Compatible Database Source | `database_connector.cpp` | ✅ Implemented |
| Web Crawler / Sitemap Source | `web_crawler_connector.cpp` | ✅ Implemented |
| Per-Document Quarantine Retry | `ingestion_manager.cpp` | ✅ Implemented |
| Distributed Ingestion Coordinator | `ingestion_coordinator.cpp` | ✅ Implemented |
| CDC Source (live database streams) | `cdc_connector.cpp`         | ✅ Implemented |
| Plugin API for third-party connectors | `ingestion_manager.h`, `ingestion_manager.cpp` | ✅ Implemented |

---

## Scientific References

The following IEEE-formatted references support the research basis for features described in this document. References cover stream processing, distributed data ingestion, change-data capture, Kafka/message-queue systems, web crawling, and reliability engineering.

### Stream Processing & Ingestion Architecture

[1] M. Zaharia, T. Das, H. Li, T. Hunter, S. Shenker, and I. Stoica, "Discretized Streams: Fault-Tolerant Streaming Computation at Scale," in *Proc. 24th ACM Symp. Operating Systems Principles (SOSP)*, 2013, pp. 423–438. https://doi.org/10.1145/2517349.2522737

[2] P. Carbone, A. Katsifodimos, S. Ewen, V. Markl, S. Haridi, and K. Tzoumas, "Apache Flink: Stream and Batch Processing in a Single Engine," *IEEE Data Engineering Bulletin*, vol. 38, no. 4, pp. 28–38, Dec. 2015. https://asterios.katsifodimos.com/assets/publications/flink-deb.pdf

[3] J. Kreps, N. Narkhede, and J. Rao, "Kafka: A Distributed Messaging System for Log Processing," in *Proc. 6th Int. Workshop on Networking Meets Databases (NetDB)*, 2011. https://kafka.apache.org/papers/kafka-netdb-2011.pdf

### Change-Data Capture (CDC)

[4] R. Ramakrishnan, D. Donjerkovic, A. Ranganathan, K. S. Beyer, and M. Krishnaprasad, "SRQL: Sorted Relational Query Language," in *Proc. 10th Int. Conf. Scientific and Statistical Database Management (SSDBM)*, 1998. (foundational replication/CDC concepts)

[5] M. Stonebraker, U. Çetintemel, and S. Zdonik, "The 8 Requirements of Real-Time Stream Processing," *ACM SIGMOD Record*, vol. 34, no. 4, pp. 42–47, Dec. 2005. https://doi.org/10.1145/1107499.1107504

### Distributed Ingestion & Coordination

[6] D. Ongaro and J. Ousterhout, "In Search of an Understandable Consensus Algorithm," in *Proc. USENIX Annual Technical Conf. (ATC)*, 2014, pp. 305–319. https://raft.github.io/raft.pdf

[7] G. DeCandia et al., "Dynamo: Amazon's Highly Available Key-Value Store," in *Proc. 21st ACM Symp. Operating Systems Principles (SOSP)*, 2007, pp. 205–220. https://doi.org/10.1145/1294261.1294281

### Web Crawling & Robots.txt

[8] A. Heydon and M. Najork, "Mercator: A Scalable, Extensible Web Crawler," *World Wide Web*, vol. 2, no. 4, pp. 219–229, 1999. https://doi.org/10.1023/A:1019213109274

[9] The Internet Archive, "robots.txt — Standard for Web Robots" (RFC draft), 2022. https://datatracker.ietf.org/doc/draft-rep-wg-topic/

### Rate Limiting & Back-pressure

[10] P. Karn and C. Partridge, "Improving Round-Trip Time Estimates in Reliable Transport Protocols," *ACM SIGCOMM Computer Communication Review*, vol. 17, no. 5, pp. 2–7, Aug. 1987. https://doi.org/10.1145/55483.55484 (exponential backoff foundation)

[11] N. Bronson, Z. Amsden, G. Cabrera, P. Chakka, P. Dimov, H. Ding, J. Ferris, A. Giardullo, S. Kulkarni, H. C. Li, M. Marchukov, D. Petrov, L. Puzar, Y. J. Song, and V. Venkataramani, "TAO: Facebook's Distributed Data Store for the Social Graph," in *Proc. USENIX Annual Technical Conf. (ATC)*, 2013, pp. 49–60. (rate limiting + backpressure in production data pipelines)

### Data Quality & Schema Validation

[12] F. Naumann and M. Herschel, "An Introduction to Duplicate Detection," *Synthesis Lectures on Data Management*, vol. 2, no. 1, pp. 1–87, 2010. https://doi.org/10.2200/S00262ED1V01Y201003DTM003

[13] X. L. Dong, E. Gabrilovich, G. Heitz, W. Horn, N. Lao, K. Murphy, T. Strohmann, S. Sun, and W. Zhang, "Knowledge Vault: A Web-Scale Approach to Probabilistic Knowledge Fusion," in *Proc. 20th ACM SIGKDD Int. Conf. Knowledge Discovery and Data Mining*, 2014, pp. 601–610. https://doi.org/10.1145/2623330.2623623
