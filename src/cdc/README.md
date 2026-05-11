> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Change Data Capture (CDC) Module
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · include/cdc/FUTURE_ENHANCEMENTS.md · docs/de/cdc/ -->

Change Data Capture and changefeed implementation for ThemisDB.

## Module Purpose

Implements Change Data Capture for ThemisDB, providing real-time change notifications via SSE streaming, filtered subscriptions, change log management, historical change replay, and CDC-driven incremental materialized view maintenance.

## Subsystem Scope

**In scope:** Changefeed engine, SSE event streaming, per-collection/per-key filtering, change log persistence, historical replay, subscription lifecycle management, cross-collection aggregated streams, CDC-based materialized view maintenance, WebSocket transport (`WsTransport`, `cdc_ws_handler.cpp`), Kafka producer transport (`KafkaCDCProducer`), `ICDCTransport` abstract interface, consumer group semantics (`ConsumerGroupManager`), at-least-once delivery (`DeliveryTracker`), dead-letter queue (`DeadLetterQueue`), transactional outbox (`OutboxWriter`, `OutboxRelay`), change stream compression, Debezium format support, schema registry integration, GDPR-aware change log redaction.

## Relevant Interfaces

- `changefeed.cpp` — core change capture engine
- `changefeed_buffer.cpp` — per-tenant in-memory ring buffer for pending events
- `tenant_buffer_manager.cpp` — per-tenant buffer lifecycle and quota enforcement
- `ws_transport.cpp` — WebSocket transport (`WsTransport`, implements `ICDCTransport`)
- `cdc_ws_handler.cpp` — WebSocket HTTP handler wiring for CDC streams
- `kafka_cdc_producer.cpp` — Kafka transport backend (`KafkaCDCProducer`, opt-in via `THEMIS_ENABLE_KAFKA`)
- `consumer_group.cpp` — consumer group semantics with durable offset tracking (`ConsumerGroupManager`)
- `delivery_tracker.cpp` — at-least-once delivery with redelivery and acknowledgement (`DeliveryTracker`)
- `dead_letter_queue.cpp` — persistence of events that exhaust delivery retries (`DeadLetterQueue`)
- `outbox.cpp` — transactional outbox pattern for atomic CDC + application data publishing (`OutboxWriter`, `OutboxRelay`)
- `cross_collection_stream.cpp` — cross-collection change aggregation (`CrossCollectionStream`)
- `cdc_materialized_view.cpp` — CDC-driven incremental materialized view maintenance (`CDCMaterializedViewMaintainer`)
- `cdc_admin.cpp` — admin API for subscription and buffer management

## Current Delivery Status

**Maturity:** 🟢 Production — SSE-based changefeeds, filtered subscriptions, WebSocket transport, consumer groups, and Kafka producer integration operational.

## Components

- Changefeed engine (`changefeed.cpp`)
- Per-tenant in-memory ring buffer for pending events (`changefeed_buffer.cpp`)
- Per-tenant buffer lifecycle and quota enforcement with backpressure (`tenant_buffer_manager.cpp`)
- Server-Sent Events (SSE) streaming
- WebSocket-based change streaming (`ws_transport.cpp`, `cdc_ws_handler.cpp`)
- Kafka producer transport for enterprise CDC pipelines (`kafka_cdc_producer.cpp`)
- Consumer group semantics with durable offset tracking (`consumer_group.cpp`)
- At-least-once delivery tracker with acknowledgement and redelivery (`delivery_tracker.cpp`)
- Dead-letter queue for failed event deliveries (`dead_letter_queue.cpp`)
- Transactional outbox pattern for atomic CDC + application data publishing (`outbox.cpp`)
- Cross-collection change aggregation (`cross_collection_stream.cpp`)
- CDC-based incremental materialized view maintenance (`cdc_materialized_view.cpp`)
- Admin API for subscription and buffer management (`cdc_admin.cpp`)

## Features

- Real-time change notifications
- SSE-based and WebSocket-based event streaming
- Filtered change subscriptions (collection, key prefix, operation type)
- Historical change replay from stored change log
- Consumer group semantics with durable offset tracking and partition assignment
- At-least-once delivery guarantees with consumer acknowledgement and redelivery — available for SSE connections (`GET /changefeed/stream?consumer_id=...`) and Consumer Groups (`/v2/cdc/stream`)
- Dead-letter queue for events that exhaust delivery retries
- Transactional outbox pattern for atomic CDC + application data publishing
- Cross-collection merged event streams with per-collection resume cursors
- CDC-driven incremental materialized view maintenance (GROUP BY aggregations updated in O(1) per change)
- Kafka-compatible producer interface for enterprise integration (Debezium envelope format supported)
- GDPR-aware change log redaction (PII field scrubbing)
- Change stream compression for high-volume feeds

## Documentation

For CDC documentation, see:
- [Architecture Guide](ARCHITECTURE.md) — component diagram, data flow, threading model
- [Security Guide](SECURITY.md) — threat model, controls, and known limitations
- [Audit Report](AUDIT.md) — verification snapshot for build/tests/compliance
- [Changelog](CHANGELOG.md) — versioned module history
- [Performance Expectations](PERFORMANCE_EXPECTATIONS.md) — release-gate benchmark targets
- [Roadmap](ROADMAP.md) — implementation status and planned work
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) — long-horizon backlog
- [Public API Headers](../../include/cdc/README.md) — entry points under `include/cdc/`
- [CDC Operations Runbook](../../docs/CDC_OPERATIONS_RUNBOOK.md) — production operations
- [CDC Implementation Summary](../../docs/implementation-history/summaries/CDC_IMPLEMENTATION_SUMMARY.md) — implementation history
- [Primary Sources (EN)](../../docs/en/cdc/PRIMARY_SOURCES.md) — canonical source index
- [Primary Sources (DE)](../../docs/de/cdc/PRIMARY_SOURCES.md) — kanonischer Quellenindex
- [Change Data Capture (DE)](../../docs/de/features/features_change_data_capture.md) — end-user guide (German)

## Public API Entry Points (`include/cdc`)

| Header | Purpose |
|---|---|
| `changefeed.h` | Core CDC stream ingestion, replay, and retention APIs |
| `consumer_group.h` | Durable consumer group offsets and at-least-once fetch semantics |
| `delivery_tracker.h` | In-flight delivery tracking, ack, and redelivery selection |
| `dead_letter_queue.h` | Persist/replay/drain failed deliveries |
| `kafka_cdc_producer.h` | Optional Kafka publisher (`THEMIS_ENABLE_KAFKA`) |
| `ws_transport.h`, `cdc_ws_handler.h` | WebSocket transport and HTTP handler wiring |
| `cdc_admin.h` | Admin operations (retention, purge, status, redaction, tombstones) |
| `changefeed_buffer.h`, `tenant_buffer_manager.h` | Per-tenant buffering and quotas |
| `cross_collection_stream.h` | Multi-collection merge stream APIs |
| `outbox.h` | Transactional outbox writer/relay contracts |
| `cdc_materialized_view.h` | Incremental materialized view maintenance bridge |
| `debezium_format.h`, `schema_registry.h` | Debezium envelopes and schema registry integration |
| `icdc_*.h`, `idelivery_guarantee_config.h` | Interface-layer contracts for pause/backpressure/filter/replay/batch delivery |
| `change_stream_compressor.h`, `cdc_metrics.h`, `cdc_error.h` | Compression, metrics, and structured CDC errors |

## Configuration Surfaces

### Build-time flags

| Flag | Behavior |
|---|---|
| `THEMIS_ENABLE_KAFKA` | Enables librdkafka-backed producer implementation; otherwise no-op stub in `kafka_cdc_producer.h` |
| `THEMIS_ENABLE_HTTP_SERVER` | Enables HTTP-admin wiring (`cdc_admin.cpp`) in build registration |

### Runtime settings and knobs

| Surface | Key Fields |
|---|---|
| `Changefeed::RetentionPolicy` | `enabled`, `max_age`, `max_entries`, `max_bytes`, `cleanup_interval`, `compact_on_cleanup` |
| SSE at-least-once stream | `consumer_id`, `ack_timeout_ms` on `GET /changefeed/stream` plus `POST /changefeed/stream/ack` |
| Consumer groups | Group identity, partition ownership, and ack timeout in `ConsumerGroupManager::fetchEventsAtLeastOnce()` |
| Kafka producer | `brokers`, `topic_prefix`, `batch_size`, `linger_ms`, optional auth/TLS in `KafkaProducerConfig` |

## Runtime Behavior, Failure Modes, and Limits

- Delivery model is **at-least-once** for SSE and consumer groups: duplicates are possible after reconnect/timeouts; consumers must be idempotent.
- Backpressure behavior favors availability: slow consumers may observe dropped oldest buffered events plus gap semantics.
- DLQ receives events that exceed retry policy; decompression-corrupted payloads are logged but not recoverable.
- Tenant isolation is enforced by per-tenant buffers and scoped subscriptions.
- Kafka path is opt-in and build-flag dependent; without `THEMIS_ENABLE_KAFKA`, publish/start return `false`.
- Raw change-log-at-rest encryption is not performed by this module; operators must enforce storage-layer encryption.

## Scientific References

1. Stonebraker, M., Rowe, L. A., & Hirohama, M. (1990). **The Implementation of Postgres**. *IEEE Transactions on Knowledge and Data Engineering*, 2(1), 125–142. https://doi.org/10.1109/69.43410

2. Kleppmann, M. (2017). **Designing Data-Intensive Applications: The Big Ideas Behind Reliable, Scalable, and Maintainable Systems**. O'Reilly Media. ISBN: 978-1-449-37332-0

3. Mohan, C., Haderle, D., Lindsay, B., Pirahesh, H., & Schwarz, P. (1992). **ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging**. *ACM Transactions on Database Systems*, 17(1), 94–162. https://doi.org/10.1145/128765.128770

4. Flink Community. (2015). **Apache Flink: Stream and Batch Processing in a Single Engine**. *IEEE Data Engineering Bulletin*, 38(4), 28–38. http://sites.computer.org/debull/A15dec/p28.pdf

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/cdc/README.md`](../../include/cdc/README.md) for the public API.

**Example: SSE subscription with at-least-once acknowledgement**

```bash
# 1) Open stream
curl -N -H "Authorization: Bearer $TOKEN" \
  "https://themis.example/changefeed/stream?collection=orders&consumer_id=ops-bot&ack_timeout_ms=30000"

# 2) Acknowledge delivered sequence(s)
curl -X POST -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"consumer_id":"ops-bot","acked_sequence":12345}' \
  "https://themis.example/changefeed/stream/ack"
```

**Example: enable periodic retention cleanup in code**

```cpp
themis::cdc::Changefeed::RetentionPolicy policy = themis::cdc::Changefeed::RetentionPolicy::defaults();
policy.enabled = true;
policy.max_age = std::chrono::hours(24 * 7);
policy.max_entries = 2'000'000;
policy.max_bytes = 8ull * 1024 * 1024 * 1024;

changefeed.updateRetentionPolicy(policy);
```

## Troubleshooting

- **No Kafka traffic**: confirm build includes `-DTHEMIS_ENABLE_KAFKA=1` and `KafkaCDCProducer::start()` succeeds.
- **Repeated redelivery**: verify consumers call ack endpoint before `ack_timeout_ms` expires.
- **High CDC lag**: inspect tenant buffer pressure and subscription fan-out; tune retention/consumer throughput.
- **Missing replay events**: validate requested cursor range is within retained change-log window.
