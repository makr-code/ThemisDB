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
- [Audit Report](AUDIT.md) — current audit findings and open risks
- [Changelog](CHANGELOG.md) — release history and behavior changes
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) — planned features and design constraints
- [Performance Expectations](PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [Roadmap](ROADMAP.md) — feature status and planned work
- [Security Notes](SECURITY.md) — threat model and controls
- [Public API Headers](../../include/cdc/README.md) — header reference
- [CDC Operations Runbook](../../docs/CDC_OPERATIONS_RUNBOOK.md) — production operations
- [CDC Implementation Summary](../../docs/implementation-history/summaries/CDC_IMPLEMENTATION_SUMMARY.md) — implementation history
- [Change Data Capture (DE)](../../docs/de/features/features_change_data_capture.md) — end-user guide (German)

## Scientific References

1. Stonebraker, M., Rowe, L. A., & Hirohama, M. (1990). **The Implementation of Postgres**. *IEEE Transactions on Knowledge and Data Engineering*, 2(1), 125–142. https://doi.org/10.1109/69.43410

2. Kleppmann, M. (2017). **Designing Data-Intensive Applications: The Big Ideas Behind Reliable, Scalable, and Maintainable Systems**. O'Reilly Media. ISBN: 978-1-449-37332-0

3. Mohan, C., Haderle, D., Lindsay, B., Pirahesh, H., & Schwarz, P. (1992). **ARIES: A Transaction Recovery Method Supporting Fine-Granularity Locking and Partial Rollbacks Using Write-Ahead Logging**. *ACM Transactions on Database Systems*, 17(1), 94–162. https://doi.org/10.1145/128765.128770

4. Flink Community. (2015). **Apache Flink: Stream and Batch Processing in a Single Engine**. *IEEE Data Engineering Bulletin*, 38(4), 28–38. http://sites.computer.org/debull/A15dec/p28.pdf

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
Kafka support requires librdkafka and `-DTHEMIS_ENABLE_KAFKA=1`.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/cdc/README.md`](../../include/cdc/README.md) for the public API.

## Troubleshooting

### SSE stream delivers no events
- Confirm the change feed is registered for the target collection via `cdc_admin.cpp` (`CDCAdmin::listSubscriptions()`).
- Check that the storage commit hook invokes the changefeed post-commit path.
- Review `cdc.buffer.max_events_per_tenant` and `cdc.buffer.high_watermark_pct` — a full buffer silently drops oldest events and inserts a gap marker.

### Consumer receives duplicate events
- CDC guarantees at-least-once delivery. Consumers **must** be idempotent.
- Use `consumer_id` + `ack_timeout_ms` on `GET /changefeed/stream` and acknowledge events via `POST /changefeed/stream/ack`.
- Dead-letter queue (`DeadLetterQueue`) stores events that exhausted delivery retries; drain via admin endpoint.

### Kafka CDC producer does nothing
- Ensure `THEMIS_ENABLE_KAFKA=1` CMake flag is set; without it the stub implementation returns `false` on every call.
- Verify `THEMIS_KAFKA_BOOTSTRAP_SERVERS` env var or config YAML broker address.
- Check `delivery_report_cb` logs for broker-side rejection or authentication errors.

### GDPR redaction not applied
- Verify the schema annotation (`encryption` or `pii`) is set on the affected fields.
- Redaction is applied before transport serialization; check `CDCAdmin::redactByKeyPrefix()` and the `cdc_redactions` column family audit log.
