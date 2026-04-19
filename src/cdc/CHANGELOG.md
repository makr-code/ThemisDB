> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — CDC (Change Data Capture) Module

All notable changes to the CDC module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
*(All planned features are implemented — see `FUTURE_ENHANCEMENTS.md` for long-horizon items.)*

## [1.8.0] — 2026-03-22
### Added
- Five new public CDC interface headers in `include/cdc/`:
  - `ICDCPauseControl` (`icdc_pause_control.h`): atomic stream pause/resume with `PauseReason` enum (`AdminRequest`, `Backpressure`, `SchemaEvolution`), buffered event accumulation during pause, and `InMemoryPauseControl` concrete implementation
  - `ICDCBackpressureSignal` (`icdc_backpressure_signal.h`): advisory flow-control with `BackpressureLevel` enum (`None`/`Low`/`Medium`/`High`/`Critical`), `setLevelCallback()`, and `InMemoryBackpressureSignal` concrete implementation; Critical level triggers automatic pause when a `ICDCPauseControl` handle is registered
  - `ICDCFanIn` (`icdc_fan_in.h`): multi-source fan-in with `FanInEvent` (tagged with `CollectionId`), pluggable `IFanInMergePolicy` (timestamp-order + sequence-order policies), and `InMemoryFanIn` concrete implementation
  - `ICDCEventSchema` (`icdc_event_schema.h`): schema-aware event delivery with `SchemaEvolutionDescriptor` (old/new schema version, `MigrationStrategy` enum, affected fields), `ISchemaEvolutionCallback` (`onCompatible`/`onIncompatible`), and `InMemoryEventSchemaRegistry` concrete implementation
  - `IDeliveryGuaranteeConfig` (`idelivery_guarantee_config.h`): per-listener delivery semantics with `DeliveryMode` enum (`AtLeastOnce`/`ExactlyOnce`), `setDeduplicationWindow()`, and `InMemoryDeliveryGuaranteeConfig` concrete implementation with rolling dedup hash window
- Five focused test executables (≈1 300 lines total):
  - `CDCPauseControlFocusedTests` (`tests/test_cdc_pause_control.cpp`)
  - `CDCBackpressureSignalFocusedTests` (`tests/test_cdc_backpressure_signal.cpp`)
  - `CDCFanInFocusedTests` (`tests/test_cdc_fan_in.cpp`)
  - `CDCEventSchemaFocusedTests` (`tests/test_cdc_event_schema.cpp`)
  - `CDCDeliveryGuaranteeConfigFocusedTests` (`tests/test_cdc_delivery_guarantee_config.cpp`)
- CI workflow `cdc-interfaces-ci.yml` covering all five focused test targets

## [1.7.0] — 2026-03-10
### Added
- Build system audit: all CDC source files registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`; standalone focused test targets `CDCAdminFocusedTests` and `TenantBufferManagerFocusedTests`
- All CDC source files added to `_themis_test_extra_sources` in `tests/CMakeLists.txt` (including `delivery_tracker.cpp` and `ws_transport.cpp`)

## [1.6.0] — 2026-02-15
### Added
- Cross-collection change aggregation streams: `CrossCollectionStream` (`cross_collection_stream.cpp`); 19 unit tests (Issue #1615)
- Debezium-compatible change event format: `DebeziumFormatter::toEnvelope()`, `toJson()`, `toJsonWithSchema()`; 23 unit tests (Issue #1614)
- Outbox pattern support for transactional change publishing: `OutboxWriter`, `OutboxRelay` (`outbox.cpp`); 16 unit tests (Issue #1612)
- CDC-based materialized view maintenance: `CDCMaterializedViewMaintainer` (`cdc_materialized_view.cpp`) (Issue #1617)
- Change stream compression for high-volume feeds (`include/cdc/change_stream_compressor.h`) (Issue #1618)
- GDPR-aware change log redaction: PII field masking in `before` snapshot for fields annotated with `encryption`/`pii` (Issue #1616)

## [1.5.0] — 2026-01-20
### Added
- At-least-once delivery guarantees: `DeliveryTracker` with `trackDelivery()`, `acknowledge()`, `acknowledgeUpTo()`, `getPendingRedelivery()`; 18 unit tests (Issue #1606)
- SSE at-least-once: `consumer_id` + `ack_timeout_ms` query params on `GET /changefeed/stream`; `POST /changefeed/stream/ack` endpoint; 5 integration tests
- Consumer group semantics: `ConsumerGroupManager`, `fetchEventsAtLeastOnce()` with offset tracking (`consumer_group.cpp`) (Issue #1609)
- Dead-letter queue for failed event deliveries: RocksDB key prefix `dlq:`; `listEntries()`, `replay()`, `drain()` (`dead_letter_queue.cpp`) (Issue #1610)
- Change event enrichment: `ChangeEvent::before_snapshot` and `after_snapshot` fields (Issue #1611)
- Kafka-compatible producer interface: `KafkaCDCProducer` (opt-in: `THEMIS_ENABLE_KAFKA`) (`kafka_cdc_producer.cpp`) (Issue #1613)
- Tenant buffer manager for per-tenant change event isolation (`tenant_buffer_manager.cpp`)

## [1.4.0] — 2025-11-01
### Added
- WebSocket transport as alternative to SSE: `WsTransport` (implements `ICDCTransport`), `cdc_ws_handler.cpp` (`/v2/cdc/stream`) (Issue #1604)
- Change log compaction and archival policies (Issue #1605)
- Change log TTL and size-based retention via `CDCAdmin::purgeOlderThan()` (Issue #1608)

## [1.0.0] — 2024-01-01
### Added
- Changefeed engine tracking insert/update/delete events per collection (`changefeed.cpp`)
- Server-Sent Events (SSE) streaming transport
- Persistent change log with append-only storage (`changefeed_buffer.cpp`)
- Subscription management with per-collection and per-key filters (`cdc_admin.cpp`)
- Historical change replay from stored change log offset (`replayFrom()`)
- Integration with analytics diff engine (`cdc_materialized_view.cpp`)
