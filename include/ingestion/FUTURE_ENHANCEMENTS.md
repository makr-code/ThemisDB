# Ingestion Module - Future Header Enhancements

<!-- Status: current | validated: 2026-03-09 | Primary: include/ingestion/ | Secondary: docs/de/ingestion/ -->
<!-- Links: ../../src/ingestion/FUTURE_ENHANCEMENTS.md · ../../src/ingestion/README.md · ../../src/ingestion/ROADMAP.md -->

## Scope

The following interface extensions were planned for this module. Most have been implemented in v1.5.x:

- `IKafkaConnector` — **implemented** in `include/ingestion/kafka_connector.h` + `src/ingestion/kafka_connector.cpp`
- `IS3SourceConnector` / `ObjectStorageConnector` — **implemented** in `include/ingestion/object_storage_connector.h` + `src/ingestion/object_storage_connector.cpp`
- Distributed coordinator — **implemented** in `include/ingestion/ingestion_coordinator.h` + `src/ingestion/ingestion_coordinator.cpp`
- Per-document quarantine retry — **implemented** in `ingestion_manager.cpp` (`IngestionAdminApi::retryQuarantineItem`); write-success branch unreachable (see Known Limitations)
- Plugin API — **implemented** in `include/ingestion/ingestion_manager.h` (`ConnectorPluginRegistry`, `IngestionBuilder::withPluginSource`, `IngestionBuilder::withConnectorPlugin`)

Remaining planned interface work: plugin sandbox enforcement (IIngestionSourcePlugin with `PluginManifest`).

## Design Constraints

- [x] All ingestion sources must implement `IIngestionSource`; no direct instantiation of concrete source types across module boundaries
- [x] Distributed coordinator is optional; when absent, ingestion defaults to single-node mode without requiring code changes in callers
- [x] Quarantine retry is bounded by a configurable `maxRetries`; exhausted retries surface `IngestionStatus::QUARANTINE_EXHAUSTED` and do not silently drop documents
- [ ] Third-party source plugins are sandboxed; they cannot open network connections to hosts not declared in `PluginManifest::allowedHosts` (sandbox enforcement not yet implemented)
- [x] S3 source connector requires server-side encryption; connections to buckets without SSE-S3 or SSE-KMS are rejected at `open()` time
- [ ] Plugin sandbox prevents file-system access outside the declared `PluginManifest::allowedPaths` (not yet implemented)

## Required Interfaces

| Interface | Consumer | Status |
|---|---|---|
| `IIngestionSource` | Ingestion pipeline, REST ingest endpoint | ✅ Implemented (`ingestion_manager.h`) |
| `IKafkaConnector` | Kafka ingestion pipeline | ✅ Implemented (`kafka_connector.h`) |
| `ObjectStorageConnector` | S3 batch ingestion | ✅ Implemented (`object_storage_connector.h`) |
| `IngestionCoordinator` | Multi-node ingestion | ✅ Implemented (`ingestion_coordinator.h`) |
| `IQuarantineRetryHandler` | Fault-tolerant ingestion | ✅ Implemented (via `IngestionAdminApi::retryQuarantineItem`) |
| `IIngestionSourcePlugin` | Plugin registry | ✅ Partial (registry + factory; sandbox not enforced) |

## Implemented Features

### Kafka Consumer Source Connector Interface ✅

- [x] `KafkaConnector` implementing `IIngestionSource` with consumer group management (`librdkafka`)
- [x] Consumer group ID and broker list configurable via `SourceConfig::options["consumer_group"]`
- [x] Offset commit tied to `IngestionCheckpointStore::commit()` for synchronized checkpoint tracking
- [x] Graceful shutdown: `rd_kafka_consumer_close()` on `IngestionManager` stop

### S3-Compatible Source Connector Interface ✅

- [x] `ObjectStorageConnector` with AWS S3 / GCS / Azure Blob provider support
- [x] SSE enforcement at `open()` time; connections to unencrypted buckets rejected
- [x] Resumable listing via continuation token; incremental mode via `LastModified` checkpoint
- [x] Path-traversal protection for downloaded object paths

### Distributed Ingestion Coordinator API ✅

- [x] `IngestionCoordinator` with consistent-hashing work-stealing thread pool
- [x] Shared checkpoint backend support for distributed state
- [x] Leader election via TTL-based lease in checkpoint collection

### Per-Document Quarantine Retry Interface ✅

- [x] `IngestionAdminApi::retryQuarantineItem(doc_id)` implemented
- [x] `raw_payload` stored in `QuarantineEntry` for per-document retry without full source restart
- [x] Exponential back-off retry; `permanently_failed` gate after `max_quarantine_retries`
- [!] Write-success is always `true` (unreachable failure branch); wire real storage-write result

### Plugin API for Third-Party Source Connectors ✅

- [x] `ConnectorPluginRegistry` thread-safe registry with `registerConnectorPlugin()`, `unregisterConnectorPlugin()`, `listConnectorPlugins()`
- [x] `IngestionBuilder::withConnectorPlugin(name, factory)` and `withPluginSource(source_id, plugin_name, ...)`
- [x] `SourceType::PLUGIN` enum value routing in `ingestSource()`
- [ ] Plugin sandbox (`PluginManifest::allowedHosts`, `allowedPaths`) not yet enforced

## Test Strategy

- Unit-test `KafkaConnector` with a mock broker: subscribe, poll, commit, and rebalance notification sequence
- Integration-test `ObjectStorageConnector` against a local MinIO instance; verify SSE enforcement and resumable listing
- Test `IngestionCoordinator` lease TTL expiry: let a lease expire without renewal; verify another node can acquire it
- Test quarantine retry: quarantine a document, verify ticket persistence, retry, assert document re-enters the ingestion pipeline
- Plugin registry test: register/unregister/list, concurrent isolation (distinct instances per `create()`), missing plugin_name error path
- Load test `IIngestionSource::dispatch()` at 10,000 documents/s for 60 s; verify no document loss and stable memory usage

## Performance Targets

- Source connector setup (`IIngestionSource::open()`) completes in ≤ 500 ms including authentication handshake
- Kafka connector `poll()` with a 100 ms timeout returns within 105 ms p99 under normal broker conditions
- S3 object listing (`listObjects()`) returns first page of 1,000 keys in ≤ 300 ms
- Distributed coordinator coordination overhead (`acquireShard()` + `releaseShard()`) ≤ 10 ms p95 over LAN

## Security / Reliability

- Source connector credentials use secret store injection; they are never stored in header-visible config types as plaintext
- S3 source connector requires server-side encryption; buckets without SSE-S3 or SSE-KMS are rejected at `open()` time
- `IIngestionSource::dispatch()` is idempotent by document ID when deduplication is enabled
- `IngestionManager` sanitizes the options map before emitting log entries (no credential leakage)
