# Ingestion Module - Future Header Enhancements

## Scope

- `IIngestionSource` interface extensions for multi-protocol source abstraction
- Apache Kafka connector source API exposed via `IKafkaConnector` header
- S3-compatible object storage source connector interface
- Distributed ingestion coordinator interface for multi-node coordination
- Per-document quarantine retry interface for fault-tolerant ingestion
- Third-party source plugin API for sandboxed external connectors

## Design Constraints

- [ ] All ingestion sources must implement `IIngestionSource`; no direct instantiation of concrete source types across module boundaries
- [ ] Distributed coordinator is optional; when absent, ingestion defaults to single-node mode without requiring code changes in callers
- [ ] Quarantine retry is bounded by a configurable `maxRetries`; exhausted retries surface `IngestionStatus::QUARANTINE_EXHAUSTED` and do not silently drop documents
- [ ] Third-party source plugins are sandboxed; they cannot open network connections to hosts not declared in `PluginManifest::allowedHosts`
- [ ] S3 source connector requires server-side encryption; connections to buckets without SSE-S3 or SSE-KMS are rejected at `open()` time
- [ ] Plugin sandbox prevents file-system access outside the declared `PluginManifest::allowedPaths`

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IIngestionSource` | Ingestion pipeline, REST ingest endpoint, batch loader | Base interface; all ingestion sources derive from this |
| `IKafkaConnector` | Kafka ingestion pipeline, streaming ingest coordinator | Wraps consumer group management and offset tracking |
| `IS3SourceConnector` | S3 batch ingestion, archive ingestion pipeline | Iterates S3 objects by prefix; supports resumable listing |
| `IDistributedIngestionCoordinator` | Multi-node ingestion, shard-aware ingest planner | Optional; injected via `IngestionPipeline::setCoordinator()` |
| `IQuarantineRetryHandler` | Fault-tolerant ingestion, dead-letter queue processor | Receives failed documents with error context; schedules retry |
| `IIngestionSourcePlugin` | Plugin registry, third-party connector loader | Entry point; provides `IIngestionSource` factory and `PluginManifest` |

## Planned Features

### Kafka Consumer Source Connector Interface

- [ ] Define `IKafkaConnector` extending `IIngestionSource` with `subscribe(topics)`, `poll(timeout) -> DocumentBatch`, and `commitOffset()`
- [ ] Consumer group ID and broker list injected via `KafkaConnectorConfig`; credentials never stored in the interface type
- [ ] `IKafkaConnector` exposes `lag() -> PartitionLagMap` for backlog monitoring
- [ ] Rebalance events delivered via `IKafkaRebalanceListener` registered through `IKafkaConnector::setRebalanceListener()`

### S3-Compatible Source Connector Interface

- [ ] Define `IS3SourceConnector` extending `IIngestionSource` with `listObjects(prefix) -> S3ObjectIterator` and `fetchObject(S3ObjectKey) -> DocumentStream`
- [ ] SSE configuration (SSE-S3, SSE-KMS, SSE-C) injected via `S3ConnectorConfig`; connector rejects buckets without encryption at `open()` time
- [ ] `S3ObjectIterator` is resumable; continuation token stored in `IS3SourceConnector::lastContinuationToken()`
- [ ] Multipart object support: objects larger than `S3ConnectorConfig::singlePartThreshold` are fetched in parallel parts

### Distributed Ingestion Coordinator API

- [ ] Define `IDistributedIngestionCoordinator` with `acquireShard(ShardKey) -> ShardLease` and `releaseShard(ShardLease)`
- [ ] Shard lease carries a TTL; coordinators must call `renewLease()` periodically or the lease expires
- [ ] Coordinator exposes `clusterHealth() -> CoordinatorHealth` for liveness and quorum status
- [ ] When coordinator is absent, `IngestionPipeline` operates in single-shard mode; interface is the same

### Per-Document Quarantine Retry Interface

- [ ] Define `IQuarantineRetryHandler` with `quarantine(Document, IngestionError) -> QuarantineTicket` and `retry(QuarantineTicket) -> RetryStatus`
- [ ] `QuarantineTicket` is a serializable, opaque value type; tickets survive process restarts
- [ ] Retry delay is configurable via `QuarantineConfig::retryDelaySeconds` (default: 5 s); exponential backoff optional
- [ ] `RetryStatus::EXHAUSTED` returned when `maxRetries` is exceeded; document moved to dead-letter store

### Third-Party Source Plugin API

- [ ] Define `IIngestionSourcePlugin` with `manifest() -> PluginManifest` and `createSource(IngestionConfig) -> std::unique_ptr<IIngestionSource>`
- [ ] `PluginManifest` declares `allowedHosts`, `allowedPaths`, `pluginId`, and `version`
- [ ] Sandbox enforces manifest at runtime; violations surface `PluginSandboxError::UNAUTHORIZED_ACCESS`
- [ ] Plugin registry keyed by `pluginId`; duplicate IDs rejected with `PluginError::DUPLICATE_ID` at registration time

## Test Strategy

- Unit-test `IKafkaConnector` with a mock broker: subscribe, poll, commit, and rebalance notification sequence
- Integration-test `IS3SourceConnector` against a local MinIO instance; verify SSE enforcement and resumable listing
- Test `IDistributedIngestionCoordinator` lease TTL expiry: let a lease expire without renewal; verify another node can acquire it
- Test `IQuarantineRetryHandler` full cycle: quarantine a document, verify ticket persistence, retry, assert document re-enters the ingestion pipeline
- Plugin sandbox test: attempt network access to an undeclared host from a mock plugin; assert `PluginSandboxError::UNAUTHORIZED_ACCESS`
- Load test `IIngestionSource::dispatch()` at 10,000 documents/s for 60 s; verify no document loss and stable memory usage

## Performance Targets

- Source connector setup (`IIngestionSource::open()`) completes in ≤ 500 ms including authentication handshake
- Document dispatch (`IIngestionSource::dispatch()`) ≤ 1 ms per document under nominal load
- Quarantine retry delay configurable (default: 5 s); minimum enforced delay ≥ 1 s to avoid tight retry loops
- Distributed coordinator coordination overhead (`acquireShard()` + `releaseShard()`) ≤ 10 ms p95 over LAN
- Kafka connector `poll()` with a 100 ms timeout returns within 105 ms p99 under normal broker conditions
- S3 object listing (`listObjects()`) returns first page of 1,000 keys in ≤ 300 ms on a standard S3-compatible endpoint

## Security / Reliability

- Source connector credentials (access keys, tokens, passwords) use secret store injection; they are never stored in header-visible config types as plaintext
- S3 source connector requires server-side encryption; buckets without SSE-S3 or SSE-KMS are rejected at `open()` time with `S3Error::ENCRYPTION_REQUIRED`
- Third-party plugin sandbox prevents network access to hosts not declared in `PluginManifest::allowedHosts`; violations are logged and surfaced as errors
- Quarantine tickets are signed to prevent forged retry injection; signature verified in `IQuarantineRetryHandler::retry()`
- Distributed coordinator shard leases are fenced with a monotonic token to prevent split-brain writes from expired lease holders
- `IIngestionSource::dispatch()` is idempotent by document ID when deduplication is enabled; duplicate delivery does not produce duplicate records
