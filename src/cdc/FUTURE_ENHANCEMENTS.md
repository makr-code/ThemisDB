# CDC Module - Future Enhancements

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
| `Changefeed::subscribe(filter, callback)` | SSE handler, planned WebSocket handler, planned Kafka producer | Filter by collection, key prefix, and event type |
| `Changefeed::ChangeEvent::toJson()` | All transports (SSE, WebSocket, Kafka) | Serialization format is the external contract |
| `CDCAdmin::purgeAll()` / `purgeByTenant()` | Admin REST API, planned admin CLI | Must be safe to call while subscriptions are active |
| `TenantBufferManager` | Multi-tenant CDC paths | Per-tenant buffer size quota must be enforced |
| `cdc::error::invalidArgument()` / error hierarchy | All CDC callers | Typed error codes must propagate to API error responses |

## Planned Features

### WebSocket Change Streaming Transport
**Priority:** High
**Target Version:** v1.7.0

Replace or supplement the SSE transport with a bidirectional WebSocket endpoint (`/v2/cdc/stream`) that supports both server-push change events and client-sent subscription management frames. WebSocket allows the client to change subscriptions without reconnecting.

**Implementation Notes:**
- `[ ]` Create `cdc_ws_handler.cpp`; register `WS /v2/cdc/stream` in `src/server/http_server.cpp`.
- `[ ]` Protocol: JSON control frames for subscribe/unsubscribe; change event frames matching `ChangeEvent::toJson()` output.
- `[~]` Subscribe frame: `{"action":"subscribe","id":"sub-1","collection":"orders","key_prefix":"US-","event_types":["PUT","DELETE"]}`.
- `[ ]` Unsubscribe frame: `{"action":"unsubscribe","id":"sub-1"}`.
- `[ ]` Back-pressure: per-connection outbound queue capped at 1,000 pending frames; on overflow, close with code `1011` and record `cdc_ws_overflow_total` metric.
- `[ ]` Reuse `Changefeed::subscribe()` with the same filter model as SSE; each WebSocket subscription ID maps to one `Changefeed` subscription handle.
- `[ ]` TLS handshake reuses existing Beast SSL context; no new cert management needed.

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
- `[ ]` Create `consumer_group_manager.cpp`; introduce `ConsumerGroup` with a durable `group_id` and per-group `committed_sequence` stored in RocksDB (key: `cdc_group:{group_id}:offset`).
- `[ ]` `Changefeed` assigns partitions by key-hash modulo `group.consumer_count`; each consumer receives only its assigned partition.
- `[ ]` Consumer connects with `{"action":"subscribe","group_id":"etl-workers","consumer_id":"worker-3","collection":"orders"}`.
- `[ ]` Consumer acknowledges processed events with `{"action":"ack","group_id":"etl-workers","sequence":10042}`; server advances committed offset.
- `[ ]` On reconnect, resume from `committed_sequence + 1` without scanning the full log.
- `[?]` Decision needed: how to rebalance partitions when consumers join/leave mid-session (static assignment vs. cooperative rebalance protocol).

**Performance Targets:**
- Consumer group offset commit (RocksDB write) < 1 ms p99.
- Resume-from-offset for a group that was offline for 24 h (≤ 10M buffered events) begins delivering in < 5 s.

---

### Kafka-Compatible Producer Interface
**Priority:** Medium
**Target Version:** v1.9.0

For enterprise deployments that use Kafka as a message bus, add a CDC-to-Kafka bridge that publishes `ChangeEvent` records to a configured Kafka topic. Implement using `librdkafka` to avoid a heavy JVM dependency.

**Implementation Notes:**
- `[ ]` Create `kafka_cdc_producer.cpp`; implement `ICDCTransport` interface alongside the existing SSE transport.
- `[ ]` Topic routing: one topic per collection (e.g., `themis.cdc.orders`) or a single multiplexed topic; configurable via `config/data_management/cdc_kafka.yaml`.
- `[ ]` Message key: `ChangeEvent::key`; message value: `ChangeEvent::toJson()` serialized to UTF-8 bytes.
- `[ ]` Use `librdkafka` producer with `acks=all` and `enable.idempotence=true` for exactly-once semantics where broker supports it.
- `[ ]` On `librdkafka` not found at build time, `kafka_cdc_producer.cpp` compiles as a no-op stub (same pattern as CUDA stubs in acceleration).
- `[ ]` Expose `cdc_kafka_delivered_total`, `cdc_kafka_error_total` Prometheus counters.

**Performance Targets:**
- Kafka producer throughput ≥ 50,000 events/sec on a single producer thread (standard Kafka hardware).
- End-to-end latency (change committed → Kafka broker `ack`) < 10 ms p99 on LAN.

---

### Change Log Compaction and Retention Policies
**Priority:** Medium
**Target Version:** v1.8.0

The change log grows unboundedly. Implement size-based and TTL-based retention policies managed by `CDCAdmin`, exposed via both the admin REST API and a background compaction thread.

**Implementation Notes:**
- `[ ]` Add `RetentionPolicy` struct to `cdc_admin.h`: `max_age_seconds`, `max_bytes`, `max_entries`; load from `config/data_management/cdc_retention.yaml`.
- `[ ]` Background compaction thread in `changefeed.cpp` (similar to L3 eviction thread in `adaptive_query_cache.cpp`): runs every `compaction_interval_s` (default 300 s).
- `[ ]` Compaction deletes events older than `max_age_seconds` using `CDCAdmin::purgeOlderThan(timestamp)` (new method).
- `[ ]` Size-based trigger: if change log RocksDB column family exceeds `max_bytes`, compact oldest entries until under 80% of limit.
- `[ ]` `CDCAdmin::getRetentionStatus()` returns current log size, oldest event age, and next scheduled compaction time.
- `[ ]` Admin endpoint: `GET /v1/admin/cdc/retention` and `PUT /v1/admin/cdc/retention` to read/update policy at runtime.

**Performance Targets:**
- Compaction of 1M expired events completes in < 30 s (background, no query impact).
- Compaction I/O bandwidth capped at 50 MB/s to avoid starving foreground writes (configurable).

---

### GDPR-Aware Change Log Redaction
**Priority:** Low
**Target Version:** v2.0.0

When a data-subject deletion request arrives, all historical change log entries referencing that subject's key prefix must have their `value` field scrubbed. Implement `CDCAdmin::redactByKeyPrefix(tenant_id, key_prefix)` that rewrites affected log entries in place.

**Implementation Notes:**
- `[ ]` Scan RocksDB change log column family for entries where `key` matches `key_prefix`; replace `value` JSON field with `"[REDACTED]"` and append `"redacted":true` to the event JSON.
- `[ ]` Preserve `sequence`, `type`, `key`, `timestamp_ms` for audit trail integrity; only `value` is scrubbed.
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
| WebSocket concurrent connections | 0 (not implemented) | ≥ 5,000 | Load test with `k6` WebSocket scenario |
| Consumer group offset commit | N/A | < 1 ms p99 | `tests/cdc/consumer_group_bench.cpp` |
| Kafka producer throughput | N/A | ≥ 50K events/sec | `benchmarks/kafka_producer_bench.cpp` |
| Log compaction (1M events) | Unbounded | < 30 s | `tests/cdc/compaction_bench.cpp` |

## Security / Reliability

- `[ ]` WebSocket upgrade requests must be validated by `auth::JWTValidator` with `cdc:subscribe` scope before the HTTP 101 switch; reject with 401 before protocol upgrade.
- `[ ]` `CDCAdmin::redactByKeyPrefix()` requires `admin:cdc:redact` JWT scope and must write an immutable audit log entry before beginning redaction to ensure the operation is traceable even if it fails midway.
- `[ ]` Kafka producer credentials (SASL/TLS) must be loaded from `config/security/` paths via `ConfigPathResolver::resolve()`; credentials must never be logged even at DEBUG level.
