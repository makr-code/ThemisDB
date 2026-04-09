<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/cdc/ -->

# CDC Module - Future Enhancements
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · include/cdc/FUTURE_ENHANCEMENTS.md · docs/de/cdc/ -->

## Scope

This document covers implementation-specific future enhancements for the CDC (Change Data Capture) module (`src/cdc/`), comprising `changefeed.cpp` (core change tracking, 628 lines), `changefeed_buffer.cpp`, `tenant_buffer_manager.cpp`, and `cdc_admin.cpp`. Enhancements focus on delivery guarantees, new transports (WebSocket, Kafka), consumer group semantics, and operational tooling. Higher-level analytics diff integration (`src/analytics/diff_engine.cpp`) and storage-layer transaction management are out of scope.

## Design Constraints

- `[ ]` `Changefeed::ChangeEvent` JSON serialization format (`toJson()` / `fromJson()`) must remain stable for all events emitted to external consumers; new fields are additive (optional in deserialization).
- `[ ]` `TenantBufferManager` per-tenant memory buffers must not be bypassed by any new transport; all change events flow through `changefeed_buffer.cpp` before delivery.
- `[ ]` Kafka producer integration must be an opt-in, separately configured component; the core changefeed must remain functional without any Kafka dependency present.
- `[ ]` `CDCAdmin` operations (`purgeAll`, tenant-level purge) must remain available via the admin interface even during active WebSocket or Kafka streaming sessions.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `Changefeed::subscribe(filter, callback)` | SSE handler, WebSocket handler (`WsTransport`), Kafka producer (`KafkaCDCProducer`) | Filter by collection, key prefix, and event type |
| `Changefeed::ChangeEvent::toJson()` | All transports (SSE, WebSocket, Kafka) | Serialization format is the external contract |
| `CDCAdmin::purgeAll()` / `purgeByTenant()` | Admin REST API, planned admin CLI | Must be safe to call while subscriptions are active |
| `TenantBufferManager` | Multi-tenant CDC paths | Per-tenant buffer size quota must be enforced |
| `cdc::error::invalidArgument()` / error hierarchy | All CDC callers | Typed error codes must propagate to API error responses |

## Implemented Features

### At-Least-Once Delivery Tracker ✅ (Implemented - PR #2797)

`DeliveryTracker` (`include/cdc/delivery_tracker.h`, `src/cdc/delivery_tracker.cpp`) provides
transport-agnostic at-least-once delivery semantics for CDC change events.

- `trackDelivery(consumer_id, events)` — register dispatched events as pending per consumer
- `acknowledge(consumer_id, sequence)` — single-event point acknowledgement
- `acknowledgeUpTo(consumer_id, sequence)` — cumulative (TCP-style) acknowledgement
- `getPendingRedelivery(consumer_id)` — poll for events past `ack_timeout`; enforces `max_redelivery_attempts`
- Optional `RedeliveryCallback` + background thread for automatic redelivery without polling
- Configurable back-pressure limit (`max_pending_per_consumer`)
- All methods thread-safe; 18 unit tests in `tests/test_cdc_delivery_tracker.cpp`

### Cross-Collection Change Aggregation Stream ✅ (Implemented - PR #2848)

`CrossCollectionStream` (`include/cdc/cross_collection_stream.h`, `src/cdc/cross_collection_stream.cpp`)
provides a unified, globally-ordered view of change events across multiple named `Changefeed` instances.

- `addCollection(name, feed)` / `removeCollection(name)` — register or deregister named feeds
- `listEvents(StreamOptions)` — merge events from all (or a subset of) registered collections,
  sorted by `(timestamp_ms, collection, sequence)`
- `listEventsFor(collection_names, limit)` — convenience overload for collection-subset queries
- `getHighWatermark(collection_names)` — highest sequence seen across the specified collections
- `StreamOptions` supports per-collection resume cursors (`from_sequence`), `key_prefix`,
  `event_types`, `collections` (subset filter), and `limit`
- `AggregatedEvent` wraps `Changefeed::ChangeEvent` with a `collection` field; `toJson()` adds
  `"collection"` to the standard event JSON envelope
- Thread-safe: takes a lock snapshot before querying individual feeds
- 19 unit tests in `tests/test_cdc_cross_collection_stream.cpp`
### Debezium-compatible Change Event Format ✅ (Implemented - Issue #1614/#1621)

`DebeziumFormatter` (`include/cdc/debezium_format.h`, header-only) converts `Changefeed::ChangeEvent`
records into the standard Debezium unified change-event envelope, enabling zero-config
integration with Kafka Connect, Debezium Server sinks, and other Debezium-aware consumers.

- `toEnvelope(event, collection)` — produce a `DebeziumEnvelope` from a `ChangeEvent`
- `toJson(event, collection)` — serialize directly to Debezium wire JSON (payload only)
- `toJsonWithSchema(event, collection)` — include an embedded schema descriptor block
- Operation mapping: `EVENT_PUT` (no before_snapshot) → `c`; `EVENT_PUT` (with before_snapshot) → `u`;
  `EVENT_DELETE` → `d`; `EVENT_TRANSACTION_*` → `r`
- `before` / `after` populated from `before_snapshot` / `after_snapshot`; fallback to `value` field for creates
- Invalid JSON in snapshot fields degraded gracefully via `{"_raw": ...}` key
- Configurable via `DebeziumFormatter::Config` (server name, db name, version string)
- Redacted events encode `source.snapshot = "redacted"` for audit consumers
- 23 unit tests in `tests/test_cdc_debezium_format.cpp`
### Transactional Outbox Pattern ✅ (Implemented - PR #2850)

`OutboxWriter` + `OutboxRelay` (`include/cdc/outbox.h`, `src/cdc/outbox.cpp`) implement the
transactional outbox pattern, eliminating the dual-write problem between application data
mutations and CDC event emission.

**Storage layout (RocksDB):**
```
Key:     "cdc_outbox:{20-digit-zero-padded-sequence}"
Value:   JSON (OutboxRecord)
Counter: "cdc_outbox_sequence"
```

**`OutboxWriter`** — participates in the caller's existing RocksDB transaction:

```cpp
rocksdb::Transaction* txn = db->BeginTransaction(write_opts);
txn->Put(cf, "orders:42", order_json);          // application data

OutboxRecord rec;
rec.collection = "orders";
rec.key        = "orders:42";
rec.value      = order_json;
rec.event_type = Changefeed::ChangeEventType::EVENT_PUT;
writer.writeToOutbox(txn, rec);   // CDC record in the same txn

txn->Commit();   // both commits atomically or both roll back
```

**`OutboxRelay`** — background relay thread:
- `start()` / `stop()` — lifecycle
- `relayOnce()` — synchronous poll cycle (also used directly in tests)
- PENDING → PUBLISHED on success; PENDING → FAILED after `max_relay_attempts`
- `listRecords(state)` / `listAllRecords()` — inspect outbox state
- `removeRecord(seq)` / `purgePublished()` — maintenance operations
- `totalRelayed()` / `totalFailed()` — counters

**`OutboxRelayConfig`:**
- `poll_interval` (default 100 ms) — background thread sleep
- `batch_size` (default 100) — max records per relay cycle
- `max_relay_attempts` (default 5, 0 = unlimited) — before marking FAILED

All methods thread-safe; 16 unit tests in `tests/test_cdc_outbox.cpp`.

### Schema-aware CDC with Avro/Protobuf Schema Registry Integration ✅ (Implemented - Issue #2255)

`SchemaRegistryClient` + `CdcSchemaEncoder` (`include/cdc/schema_registry.h`, header-only) implement
schema registration, caching, and Confluent-compatible wire-format encoding for CDC change events,
enabling downstream consumers (Kafka, Kafka Connect, Flink, etc.) to decode messages via any
Confluent-compatible schema registry client.

**Wire format (Confluent Schema Registry protocol):**
```
+--------+------------------+--------------------------+
| 0x00   |  schema_id (4B)  |  serialized payload      |
| magic  |  big-endian      |  JSON / Avro / Protobuf  |
+--------+------------------+--------------------------+
```

**Components:**
- `SchemaFormat` — format identifier: `JSON`, `AVRO`, `PROTOBUF`
- `SchemaInfo` — schema metadata (id, version, subject, definition, format)
- `SchemaRegistryConfig` — connection and behaviour settings (URL, auth, TTL, auto-register)
- `ISchemaRegistryBackend` — abstract interface (register / lookup schemas)
- `InMemorySchemaRegistryBackend` — thread-safe in-memory backend for testing and standalone use
- `SchemaRegistryClient` — TTL-based caching client wrapping an `ISchemaRegistryBackend`
- `CdcSchemaEncoder` — encodes `ChangeEvent` records using the Confluent wire format

**`CdcSchemaEncoder`:**
- `encode(event, collection)` — wire-format bytes: `[magic][schema_id 4B BE][JSON payload]`
- `decodeToJson(wire_bytes)` — round-trip decode for testing and debugging
- `extractSchemaId(wire_bytes)` — extract schema ID from header (static)
- `ensureCollectionSchema(collection)` — auto-register default schema on first use per collection
- `clearLocalCache()` — invalidate per-encoder schema ID cache after schema evolution
- Default schema templates: `defaultAvroSchema()`, `defaultJsonSchema()`, `defaultProtobufSchema()`

**`SchemaRegistryClient`:**
- `ensureSchema(subject, schema_json, format)` — register (idempotent) and cache
- `getSchema(id)` — cached lookup by schema ID
- `getLatestSchema(subject)` — cached lookup by subject
- `clearCache()` — invalidate all cached entries
- Pluggable backend: pass a custom `ISchemaRegistryBackend` for HTTP-based registries
- Default: `InMemorySchemaRegistryBackend` when no backend is specified

**Auto-registration:**
When `auto_register_schemas = true` (default) the encoder registers the appropriate default
schema template (Avro record, JSON Schema, or Protobuf descriptor) the first time a collection
is seen, then caches the schema ID in-process.

**Payload serialisation:**
The payload is serialised as UTF-8 JSON bytes.  Full Avro binary (avro-cpp) and Protobuf
binary encoding is intentionally left to a custom `ISchemaRegistryBackend` implementation
that can also drive the serialiser; this keeps the header dependency-free.

**Thread safety:** `InMemorySchemaRegistryBackend` and `SchemaRegistryClient` are fully
thread-safe; `CdcSchemaEncoder` is thread-safe when the `SchemaRegistryClient` it holds is
thread-safe.

**Tests:** 18+ unit tests in `tests/test_cdc_schema_registry.cpp` covering wire-format header,
round-trip decode, auto-registration (JSON/Avro/Protobuf), error paths, and cache behaviour.

### CDC-based Materialized View Maintenance ✅ (Implemented - Issue #1617)

`CDCMaterializedViewMaintainer` (`include/cdc/cdc_materialized_view.h`,
`src/cdc/cdc_materialized_view.cpp`) bridges `Changefeed::ChangeEvent` records to
`analytics::IncrementalViewManager`, enabling GROUP BY materialized views to be kept
up-to-date incrementally as CDC events arrive — without a full re-scan per change.

**Collection derivation:** the collection name is the substring of the event key before the
first `':'` (e.g. `"orders:42"` → `"orders"`).  Keys without `':'` use the full key.

**Change-type mapping:**
- `EVENT_PUT` + no `before_snapshot`  →  `INSERT`
- `EVENT_PUT` + has `before_snapshot` →  `UPDATE`
- `EVENT_DELETE`                      →  `DELETE`
- `EVENT_TRANSACTION_*`               →  skipped (not counted in `totalEventsProcessed()`)

**Row data:** `before_snapshot` / `after_snapshot` JSON strings are parsed to typed
`ChangeRecord::Row` (null / bool / int64 / double / string / serialized-json).
Falls back to the event's `value` field when a snapshot is absent.

**API:**
- `createView(ViewDefinition)` / `dropView(name)` / `hasView(name)` / `listViews()`
- `getView(name)` — returns the underlying `IncrementalView` for direct inspection
- `applyEvent(event)` — single-event ingest
- `applyEvents(events)` — batch ingest (acquires per-view locks once for the batch)
- `query(view_name, filters, limit, offset)` — paged query with optional runtime filters
- `totalEventsProcessed()` — monotonic counter (TRANSACTION_* events not counted)

**Thread safety:** all public methods are thread-safe; locking is delegated entirely to
`IncrementalViewManager`'s existing `shared_mutex`.

**Tests:** 15 unit tests in `tests/test_cdc_materialized_view.cpp` covering INSERT /
DELETE / UPDATE delta correctness, transaction-event skipping, collection prefix extraction,
value-field fallback, batch ingestion, multi-view fan-out, pagination, missing-view query,
and invalid JSON graceful handling.

---

### Change Stream Compression for High-Volume Feeds ✅ (Implemented - Issue #1618)

`ChangeStreamCompressor` (`include/cdc/change_stream_compressor.h`, header-only) provides
batch compression of CDC change events for efficient transport over SSE or WebSocket streams,
targeting high-volume feed scenarios where uncompressed JSON payloads create network or
memory pressure.

**Wire format:**
```
+----------+---------+-----------+---------------+-------------+---...---+
| magic    | version | algorithm | original_size | event_count | payload |
| 4 bytes  | 1 byte  | 1 byte    | 4 bytes LE    | 4 bytes LE  | N bytes |
+----------+---------+-----------+---------------+-------------+---...---+
magic = 0x43 0x44 0x43 0x5A ("CDCZ")
```

**Components:**
- `StreamCompressionAlgorithm` — algorithm selector (`NONE`, `ZSTD`)
- `CompressedBatch` — self-describing wire-format batch; `serialize()` / `deserialize()` for framing
- `ChangeStreamCompressor` — compress/decompress batches of `Changefeed::ChangeEvent` records

**`ChangeStreamCompressor`:**
- `compress(events)` — serialize events as JSON array, compress with Zstd, return `CompressedBatch`
- `decompress(batch)` — decompress and reconstruct `Changefeed::ChangeEvent` records
- `Config` — `algorithm` (ZSTD default), `level` (1–22, default 3), `min_compression_size_bytes` (256)
- Batches below `min_compression_size_bytes` are stored uncompressed (`algorithm = NONE`) to avoid overhead
- `getStats()` / `resetStats()` — cumulative stats: `batches_compressed`, `events_compressed`,
  `bytes_in`, `bytes_out`, `batches_skipped`, `decompress_errors`, `compression_ratio()`

**Thread safety:** `compress()`, `decompress()`, `getStats()`, `resetStats()`, and `setConfig()`
are all thread-safe.  Config reads are snapshotted under `config_mutex_`; stats use atomics.

**Tests:** 20 unit tests in `tests/test_cdc_change_stream_compressor.cpp` covering:
default config, empty batch round-trip, single-event round-trip, large batch ZSTD selection,
below-threshold uncompressed path, explicit NONE config, serialize/deserialize header
preservation, truncated/wrong-magic rejection, NONE batch decompress, ZSTD round-trip,
corrupted payload exception, stats tracking, skipped-batch counter, decompress error counter,
compression ratio > 1, resetStats, setConfig, all event fields preserved, DELETE event
round-trip.

---

## Implemented Features (v1.8.0 additions)

### CDC Pause/Resume Control API ✅ Implemented (v1.8.0)

`ICDCPauseControl` (`include/cdc/icdc_pause_control.h`) provides atomic stream suspension and resumption for CDC change feeds. `InMemoryPauseControl` is the thread-safe concrete implementation.

- `pause(PauseReason) -> bool` — atomically suspend event delivery; buffered events accumulate in-memory up to `maxBufferBytes`; returns `false` if already paused
- `resume() -> std::vector<Changefeed::ChangeEvent>` — resume delivery and drain buffered events
- `isPaused() -> bool` — non-blocking query; safe to call from any thread
- `drainBufferedEvents()` — drain without resuming (for graceful shutdown)
- `PauseReason` enum: `AdminRequest`, `Backpressure`, `SchemaEvolution`
- All methods thread-safe via `std::mutex`; 229 lines; tested by `CDCPauseControlFocusedTests`

### Backpressure Signaling Interface ✅ Implemented (v1.8.0)

`ICDCBackpressureSignal` (`include/cdc/icdc_backpressure_signal.h`) provides advisory flow-control for CDC consumers. `InMemoryBackpressureSignal` is the concrete implementation.

- `signalBackpressure(BackpressureLevel)` — update the current pressure level; fires optional level-change callback
- `clearBackpressure()` — reset to `BackpressureLevel::None`
- `currentLevel() -> BackpressureLevel` — non-blocking query via `std::atomic`
- `setPauseControl(ICDCPauseControl*)` — register a pause handle for auto-pause at `Critical` level
- `setLevelCallback(std::function<void(BackpressureLevel)>)` — hook for monitoring integrations
- `BackpressureLevel` enum: `None`, `Low`, `Medium`, `High`, `Critical`
- All methods thread-safe; tested by `CDCBackpressureSignalFocusedTests`

### Multi-Source Fan-In API ✅ Implemented (v1.8.0)

`ICDCFanIn` (`include/cdc/icdc_fan_in.h`) provides a unified view of events from multiple named `Changefeed` instances. `InMemoryFanIn` is the concrete implementation.

- `addSource(CollectionId, Changefeed&)` / `removeSource(CollectionId)` — register/deregister named feeds
- `listEvents(FanInOptions)` — merge events from all (or subset of) registered feeds; order determined by the active `IFanInMergePolicy`
- `setMergePolicy(IFanInMergePolicy&)` — plug in a custom merge/ordering strategy
- `FanInEvent` value type — wraps `Changefeed::ChangeEvent` + originating `CollectionId`
- `IFanInMergePolicy` abstract interface; built-in: `TimestampMergePolicy` (wall-clock, ascending) and `SequenceMergePolicy` (global sequence counter)
- Thread-safe (snapshot under lock before querying individual feeds); tested by `CDCFanInFocusedTests`

### Schema Evolution Hook ✅ Implemented (v1.8.0)

`ICDCEventSchema` (`include/cdc/icdc_event_schema.h`) provides schema-aware CDC event delivery with schema evolution notifications. `InMemoryEventSchemaRegistry` is the concrete implementation.

- `registerSchema(collection, schema_json, SchemaFormat)` — register a schema for a collection
- `getSchema(collection) -> std::optional<std::string>` — retrieve the current schema
- `onSchemaEvolution(collection, SchemaEvolutionDescriptor, ISchemaEvolutionCallback&)` — notify listener of schema change
- `SchemaEvolutionDescriptor` — carries old/new schema version strings, `MigrationStrategy` enum (`BACKWARD`, `FORWARD`, `FULL`, `NONE`), and affected field list
- `ISchemaEvolutionCallback` pure-virtual: `onCompatible(SchemaEvolutionDescriptor)`, `onIncompatible(SchemaEvolutionDescriptor)`
- `SchemaFormat` enum: `JSON`, `AVRO`, `PROTOBUF`
- All methods thread-safe; tested by `CDCEventSchemaFocusedTests`

### Delivery Guarantee Configuration ✅ Implemented (v1.8.0)

`IDeliveryGuaranteeConfig` (`include/cdc/idelivery_guarantee_config.h`) configures per-listener delivery semantics. `InMemoryDeliveryGuaranteeConfig` is the concrete implementation with a rolling dedup hash window.

- `setMode(DeliveryMode)` — `AtLeastOnce` or `ExactlyOnce`
- `setAckTimeout(std::chrono::milliseconds)` — redelivery window for unacknowledged events
- `setDeduplicationWindow(std::chrono::milliseconds)` — rolling window for `ExactlyOnce` dedup
- `isDuplicate(event_id) -> bool` — O(1) check via `std::unordered_set` with window expiry
- `DeliveryMode` enum: `AtLeastOnce`, `ExactlyOnce`
- Thread-safe; tested by `CDCDeliveryGuaranteeConfigFocusedTests`

## Planned Features

### WebSocket Change Streaming Transport
**Priority:** High
**Target Version:** v1.7.0

Replace or supplement the SSE transport with a bidirectional WebSocket endpoint (`/v2/cdc/stream`) that supports both server-push change events and client-sent subscription management frames. WebSocket allows the client to change subscriptions without reconnecting.

**Implementation Notes:**
- `[x]` Create `cdc_ws_handler.cpp`; register `WS /v2/cdc/stream` in `src/server/http_server.cpp`. (transport implemented as `cdc/ws_transport.cpp`; endpoint wiring is a follow-up)
- `[x]` Protocol: JSON control frames for subscribe/unsubscribe; change event frames matching `ChangeEvent::toJson()` output.
- `[x]` Subscribe frame: `{"action":"subscribe","id":"sub-1","collection":"orders","key_prefix":"US-","event_types":["PUT","DELETE"]}`.
- `[x]` Unsubscribe frame: `{"action":"unsubscribe","id":"sub-1"}`.
- `[x]` Back-pressure: per-connection outbound queue capped at 1,000 pending frames; on overflow, close with code `1011` and record `cdc_ws_overflow_total` metric.
- `[x]` Reuse `Changefeed::subscribe()` with the same filter model as SSE; each WebSocket subscription ID maps to one `Changefeed` subscription handle.
- `[x]` TLS handshake reuses existing Beast SSL context; no new cert management needed.

**Performance Targets:**
- ≥ 5,000 concurrent WebSocket connections per node with < 100 MB additional RSS.
- Event delivery latency p99 < 20 ms from `Changefeed` emit to WebSocket frame write.

**API Sketch:**
```json
// Client → Server: subscribe
{"action":"subscribe","id":"sub-1","collection":"inventory","event_types":["PUT"]}

// Server → Client: change event (matches ChangeEvent::toJson())
{"sequence":10042,"type":"PUT","key":"inventory:SKU-9918","value":"{\"qty\":5}","timestamp_ms":1740000000000}

// Server → Client: subscription ack
{"action":"subscribed","id":"sub-1"}
```

---

### Consumer Group Semantics and Offset Tracking
**Priority:** High
**Target Version:** v1.8.0

Currently, multiple consumers of the same changefeed each receive all events independently (fan-out). Add consumer group support so a group of consumers cooperatively processes a partition of the change log, with durable offset tracking so replay can resume after disconnect without full log scan.

**Implementation Notes:**
- `[x]` Create `consumer_group_manager.cpp`; introduce `ConsumerGroup` with a durable `group_id` and per-group `committed_sequence` stored in RocksDB (key: `cdc_group:{group_id}:offset`). — `src/cdc/consumer_group.cpp`, `include/cdc/consumer_group.h`; `ConsumerGroupManager`
- `[x]` `Changefeed` assigns partitions by key-hash modulo `group.consumer_count`; each consumer receives only its assigned partition. — `ConsumerGroupManager::consumerHandlesKey()` / `partitionForKey()` / `partitionForConsumer()` (FNV-1a 32-bit hash mod N)
- `[x]` Consumer connects with `{"action":"subscribe","group_id":"etl-workers","consumer_id":"worker-3","collection":"orders"}`. — `CdcWebSocketHandler::handleFrame()` now accepts `group_id` + `consumer_id`; subscription key defaults to `{group_id}:{consumer_id}` when no `id` is supplied.
- `[x]` Consumer acknowledges processed events with `{"action":"ack","group_id":"etl-workers","sequence":10042}`; server advances committed offset. — `CdcWebSocketHandler::handleFrame()` ack path calls `ConsumerGroupManager::commitOffset()` for durable persistence.
- `[x]` On reconnect, resume from `committed_sequence + 1` without scanning the full log. — Subscribe path calls `ConsumerGroupManager::getCommittedOffset()` and sets `last_sent_sequence` accordingly.
- `[x]` Decision: **static assignment** chosen for v1.8.0. Partition is derived deterministically as `fnv1a32(consumer_id) % consumer_count`; the same consumer_id always maps to the same partition. Cooperative rebalance (Kafka-style) deferred to a future release.

**Performance Targets:**
- Consumer group offset commit (RocksDB write) < 1 ms p99.
- Resume-from-offset for a group that was offline for 24 h (≤ 10M buffered events) begins delivering in < 5 s.

---

### Kafka-Compatible Producer Interface
**Priority:** Medium
**Target Version:** v1.9.0

For enterprise deployments that use Kafka as a message bus, add a CDC-to-Kafka bridge that publishes `ChangeEvent` records to a configured Kafka topic. Implement using `librdkafka` to avoid a heavy JVM dependency.

**Implementation Notes:**
- `[x]` Create `kafka_cdc_producer.cpp`; implement `KafkaCDCProducer` class (`include/cdc/kafka_cdc_producer.h`, `src/cdc/kafka_cdc_producer.cpp`).
- `[x]` Define `ICDCTransport` abstract interface (`include/cdc/icdc_transport.h`); `KafkaCDCProducer` inherits from it enabling polymorphic transport use.
- `[x]` Topic routing: one topic per collection (e.g., `themis.cdc.orders`) or a single multiplexed topic; configurable via `config/data_management/cdc_kafka.yaml`.
- `[x]` Message key: `ChangeEvent::key`; message value: `ChangeEvent::toJson()` serialized to UTF-8 bytes.
- `[x]` Use `librdkafka` producer with `acks=all` and `enable.idempotence=true` for exactly-once semantics where broker supports it.
- `[x]` On `librdkafka` not found at build time, `kafka_cdc_producer.cpp` compiles as a no-op stub (same pattern as CUDA stubs in acceleration).
- `[x]` Expose `cdc_kafka_delivered_total`, `cdc_kafka_error_total` Prometheus counters.

**Performance Targets:**
- Kafka producer throughput ≥ 50,000 events/sec on a single producer thread (standard Kafka hardware).
- End-to-end latency (change committed → Kafka broker `ack`) < 10 ms p99 on LAN.

---

### Changefeed Sequence Counter: Periodic Checkpoint + O(log N) Scan
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Fully optimized (v1.8.0, PR #4294 + follow-up)

`Changefeed::nextSequence()` previously used a mutex + Read-Modify-Write (`Get` then `Put`) round-trip to RocksDB on every change event. A series of incremental optimizations reduced this to a pure in-process atomic increment:

**Optimization history:**
1. **v1.8.0 PR #4294**: Replaced mutex + RMW with `std::atomic<uint64_t> sequence_counter_` + `Merge(SEQUENCE_KEY, +1)` per event. Removed `sequence_mutex_`. `SequenceIncrementOperator` added.
2. **Follow-up 1**: Combined `Merge(SEQUENCE_KEY)` + `Put(event_key)` into a single `rocksdb::WriteBatch`. Reduced WAL appends from 2 to 1 per event.
3. **Follow-up 2 (current)**: Removed `Merge(SEQUENCE_KEY)` from the per-event hot path entirely. `recordEvent()` now issues only the event `Put` — one WAL write, no merge operations. SEQUENCE_KEY is persisted by a dedicated background checkpoint thread (every 100ms).

**Current hot path:**
```
nextSequence()  → sequence_counter_.fetch_add(1)  [lock-free, no I/O]
recordEvent()   → db_->Put(event_key, json)        [one WAL write]
                → sequence_dirty_.store(true)      [atomic, no I/O]
checkpoint_bg   → db_->Put(SEQUENCE_KEY, counter)  [async, every 100ms]
```

**Implementation Notes:**
- `[x]` `SequenceIncrementOperator` (`makeSequenceMergeOperator()`) retained for backward-compatible reading of legacy SEQUENCE_KEY Merge operands in pre-existing databases.
- `[x]` Background `sequenceCheckpointLoop()` thread writes `SEQUENCE_KEY = sequence_counter_.load()` every `kSequenceCheckpointIntervalMs` (100ms) using a plain `Put`. Thread starts unconditionally in the constructor; final checkpoint written on destruction.
- `[x]` `loadInitialSequence()` returns `max(checkpoint, scanMaxSequence())`. This handles: (a) fresh DB, (b) events written after the last checkpoint (crash recovery), (c) legacy DBs with SEQUENCE_KEY but no events, (d) old Merge-operand DBs.
- `[x]` `scanMaxSequence()` rewritten from O(N) full scan with JSON parsing to O(log N) `SeekForPrev` on the last changefeed key — sequence number extracted directly from the key string.
- `[x]` `clear()` holds the checkpoint mutex while resetting `sequence_counter_` and writing `SEQUENCE_KEY = 0` to prevent a racing checkpoint from storing a stale non-zero value.
- `[x]` Removed now-obsolete members: `persisted_sequence_`, `sequence_merge_supported_`, `sequence_persist_mutex_`.

**Performance SLO — platform baselines (8 writer threads, 80K events):**

| Platform / Storage | Measured baseline | Regression floor |
|---|---|---|
| Windows / MSVC + TransactionDB | ~20–25K seq/s | **19K seq/s** |

The throughput ceiling on Windows with `TransactionDB` is set by WAL serialization: 8 threads compete for a single WAL append per event. Further improvements (e.g., cross-event group-commit batching, `disableWAL` for the event store with a separate audit log) require architectural changes tracked as future work.

**Regression test:** `SequenceCounterTest.ThroughputAtLeast50KPerSecUnder8Threads` (threshold: **19K/s** — catches re-introduction of per-event Merge, unconstrained mutex, or O(N) subscriber callbacks; conservative enough to tolerate CI load jitter).




**Priority:** Medium
**Target Version:** v1.8.0

The change log grows unboundedly. Implement size-based and TTL-based retention policies managed by `CDCAdmin`, exposed via both the admin REST API and a background compaction thread.

**Implementation Notes:**
- `[x]` Add `RetentionPolicy` struct to `cdc_admin.h`: `max_age_seconds`, `max_bytes`, `max_entries`; load from `config/data_management/cdc_retention.yaml`.
- `[x]` Background compaction thread in `changefeed.cpp` (similar to L3 eviction thread in `adaptive_query_cache.cpp`): runs every `compaction_interval_s` (default 300 s).
- `[x]` Compaction deletes events older than `max_age_seconds` using `CDCAdmin::purgeOlderThan(timestamp)` (new method).
- `[x]` Size-based trigger: if change log RocksDB column family exceeds `max_bytes`, compact oldest entries until under 80% of limit.
- `[x]` `CDCAdmin::getRetentionStatus()` returns current log size, oldest event age, and next scheduled compaction time.
- `[x]` Admin endpoint: `GET /v1/admin/cdc/retention` and `PUT /v1/admin/cdc/retention` to read/update policy at runtime.

**Performance Targets:**
- Compaction of 1M expired events completes in < 30 s (background, no query impact).
- Compaction I/O bandwidth capped at 50 MB/s to avoid starving foreground writes (configurable).

---

### GDPR-Aware Change Log Redaction
**Priority:** Low
**Target Version:** v2.0.0

When a data-subject deletion request arrives, all historical change log entries referencing that subject's key prefix must have their `value` field scrubbed. Implement `CDCAdmin::redactByKeyPrefix(tenant_id, key_prefix)` that rewrites affected log entries in place.

**Implementation Notes:**
- `[x]` Scan RocksDB change log column family for entries where `key` matches `key_prefix`; replace `value` JSON field with `"[REDACTED]"` and append `"redacted":true` to the event JSON.
- `[x]` Preserve `sequence`, `type`, `key`, `timestamp_ms` for audit trail integrity; only `value` is scrubbed.
- `[x]` `CDCAdmin::redactByKeyPrefix(tenant_id, key_prefix, operator_id)` implemented in `src/cdc/cdc_admin.cpp`; returns `GDPRRedactionResult` with scan/redaction counts, timing, and full audit context.
- `[x]` HTTP endpoint `POST /changefeed/redact` exposed via `ChangefeedApiHandler::handleGdprRedact` (requires `cdc:admin` scope).
- `[x]` Unit + integration tests in `tests/test_cdc_gdpr_redaction.cpp` (10 tests covering redaction, audit-field preservation, idempotency, empty-prefix rejection, DELETE events).
- `[ ]` Record a redaction audit log entry in a separate `cdc_redactions` RocksDB column family: `{"key_prefix":"user:42","redacted_count":17,"timestamp_ms":...,"operator":"admin@acme"}`.
- `[ ]` Propagate redaction to Kafka: publish a tombstone record (null value, key = original key) if Kafka producer is configured.
- `[!]` Whether in-flight SSE/WebSocket consumers receive the redacted value or the original is unclear; decision needed before implementation.

**Performance Targets:**
- Redaction of 10,000 events matching a key prefix completes in < 60 s.
- Redaction does not block new change event delivery during execution (background operation with shared RocksDB iterator).

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Mock `Changefeed::subscribe()` for WebSocket handler tests; test `ConsumerGroupManager` offset tracking with in-memory RocksDB; test `RetentionPolicy` compaction logic with time-injected clock |
| Integration | SSE + WebSocket parity for same change event | `tests/cdc/cdc_integration_test.cpp`; add consumer group resume test with simulated disconnect |
| Performance | Event throughput ≥ 50K events/sec (SSE+WS combined) | `benchmarks/cdc_bench.cpp`; Kafka throughput bench with `librdkafka` test broker |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| SSE event delivery latency p99 | < 50 ms (estimated) | < 20 ms | `benchmarks/cdc_bench.cpp` with synthetic write load |
| WebSocket concurrent connections | implemented | ≥ 5,000 | Load test with `k6` WebSocket scenario |
| Consumer group offset commit | N/A | < 1 ms p99 | `tests/cdc/consumer_group_bench.cpp` |
| Kafka producer throughput | N/A | ≥ 50K events/sec | `benchmarks/kafka_producer_bench.cpp` |
| Log compaction (1M events) | Unbounded | < 30 s | `tests/cdc/compaction_bench.cpp` |

## Security / Reliability

- `[x]` WebSocket upgrade requests must be validated by `auth::JWTValidator` with `cdc:subscribe` scope before the HTTP 101 switch; reject with 401 before protocol upgrade.
- `[ ]` `CDCAdmin::redactByKeyPrefix()` requires `admin:cdc:redact` JWT scope and must write an immutable audit log entry before beginning redaction to ensure the operation is traceable even if it fails midway.
- `[ ]` Kafka producer credentials (SASL/TLS) must be loaded from `config/security/` paths via `ConfigPathResolver::resolve()`; credentials must never be logged even at DEBUG level.

---

## Scientific References

References for the features described in this document. IEEE/ACM format, numbered.

[1] M. Kleppmann, *Designing Data-Intensive Applications: The Big Ideas Behind Reliable, Scalable, and Maintainable Systems*. Sebastopol, CA: O'Reilly Media, 2017. ISBN: 978-1-449-37332-0.
*(Transactional outbox pattern, at-least-once vs. exactly-once delivery, log-based CDC, consumer group semantics.)*

[2] J. Gray, "The Transaction Concept: Virtues and Limitations," in *Proc. 7th Int. Conf. Very Large Data Bases (VLDB)*, Cannes, France, 1981, pp. 144–154.
*(Foundational transaction model underpinning at-least-once / exactly-once delivery guarantees.)*

[3] L. S. Colby, T. Griffin, L. Libkin, I. S. Mumick, and H. Trickey, "Algorithms for Deferred View Maintenance," in *Proc. ACM SIGMOD Int. Conf. Management of Data*, Montreal, Canada, 1996, pp. 469–480. https://doi.org/10.1145/233269.233364
*(Incremental materialized view maintenance algorithms; directly applicable to `CDCMaterializedViewMaintainer`.)*

[4] A. Gupta and I. S. Mumick, "Maintenance of Materialized Views: Problems, Techniques, and Applications," *IEEE Data Engineering Bulletin*, vol. 18, no. 2, pp. 3–18, 1995.
*(Survey of view maintenance strategies; basis for CDC-driven O(1)-per-change update model.)*

[5] T. Akidau, R. Bradshaw, C. Chambers, S. Chernyak, R. J. Fernández-Moctezuma, R. Lax, S. McVeety, D. Mills, F. Perry, E. Schmidt, and S. Whittle, "The Dataflow Model: A Practical Approach to Balancing Correctness, Latency, and Cost in Massive-Scale, Unbounded, Out-of-Order Data Processing," *Proc. VLDB Endowment*, vol. 8, no. 12, pp. 1792–1803, 2015. https://doi.org/10.14778/2824032.2824076
*(Exactly-once semantics and watermark-based progress tracking; reference for `IDeliveryGuaranteeConfig::ExactlyOnce` design.)*

[6] M. Zaharia, T. Das, H. Li, T. Hunter, S. Shenker, and I. Stoica, "Discretized Streams: Fault-Tolerant Streaming Computation at Scale," in *Proc. 24th ACM Symp. Operating Systems Principles (SOSP)*, Farmington, PA, 2013, pp. 423–438. https://doi.org/10.1145/2517349.2522737
*(Backpressure semantics and micro-batch recovery; informs `TenantBufferManager` high-water-mark design.)*

[7] J. Kreps, N. Narkhede, and J. Rao, "Kafka: A Distributed Messaging System for Log Processing," in *Proc. 6th Int. Workshop Networking Meets Databases (NetDB)*, Athens, Greece, 2011.
*(Log-structured distributed messaging; foundational for `KafkaCDCProducer` and Debezium-format design.)*

[8] Apache Software Foundation, "Apache Avro Specification, v1.11," 2023. [Online]. Available: https://avro.apache.org/docs/current/spec.html
*(Schema evolution rules (FORWARD, BACKWARD, FULL compatibility) underpinning `CdcSchemaEncoder` and `SchemaRegistryClient`.)*

[9] R. C. Richardson, *Microservices Patterns: With Examples in Java*. Shelter Island, NY: Manning Publications, 2018. ISBN: 978-1-617294549.
*(Transactional outbox pattern (Chapter 3) and saga/event-driven design patterns directly applied in `OutboxWriter`/`OutboxRelay`.)*
