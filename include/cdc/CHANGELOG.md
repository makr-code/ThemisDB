<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — CDC Module Public Headers

All notable changes to public headers in `include/cdc/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `icdc_pause_control.h`: `ICDCPauseControl` for fine-grained pause/resume of feeds and consumer groups
- `icdc_backpressure_signal.h`: `ICDCBackpressureSignal` for consumer-driven backpressure propagation
- `icdc_fan_in.h`: `ICDCFanIn` and `FanInSource` for multi-source ordered stream merging
- `icdc_event_schema.h`: `ICDCEventSchema` and `EventSchemaSpec` for event schema validation contract
- `idelivery_guarantee_config.h`: `IDeliveryGuaranteeConfig` and `DeliveryGuarantee` enum for delivery semantics

### Changed
- `changefeed.h`: `ChangefeedConfig` extended with `delivery_guarantee` field
- `consumer_group.h`: `ConsumerGroupConfig` now accepts `ICDCBackpressureSignal` injection
- `delivery_tracker.h`: `DeliveryStatus` extended with `retry_count` and `last_error`

## [1.7.0] — 2026-03-10
### Added
- `tenant_buffer_manager.h`: `ITenantBufferManager` for per-tenant CDC buffer lifecycle
- `change_stream_compressor.h`: `IChangeStreamCompressor` for event stream compression
- `cdc_materialized_view.h`: `ICDCMaterializedView` for CDC-driven materialised view maintenance
- `cdc_ws_handler.h`: `ICDCWSHandler` for CDC events over WebSocket
- `ws_transport.h`: `IWSTransport` for WebSocket transport
- `schema_registry.h`: `ISchemaRegistry` and `SchemaVersion` for schema versioning

## [1.6.0] — 2026-01-20
### Added
- Initial public header set: `changefeed.h`, `changefeed_buffer.h`, `consumer_group.h`
- `icdc_transport.h`, `delivery_tracker.h`
- `debezium_format.h`, `kafka_cdc_producer.h`, `outbox.h`
- `dead_letter_queue.h`, `cross_collection_stream.h`
- `cdc_admin.h`, `cdc_error.h`, `cdc_metrics.h`
