<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# CDC Module — Public Header Architecture

**Version:** 1.8.0
**Last Updated:** 2026-04-06
**Module Path:** `include/cdc/`
**Implementation:** `../../src/cdc/`

---

## 1. Overview

The `include/cdc/` directory exposes public C++ headers for ThemisDB's Change Data Capture
(CDC) subsystem. CDC enables real-time, ordered streaming of database change events to external
consumers. Key capabilities include: changefeed management, consumer groups, delivery guarantees,
backpressure control, pause/resume, fan-in from multiple sources, event schema validation,
Debezium-format serialisation, Kafka integration, dead letter queues, and materialised view
maintenance.

---

## 2. Design Principles

- **Delivery Contract** – `idelivery_guarantee_config.h` defines at-least-once, at-most-once,
  and exactly-once delivery contracts; all transport implementations must declare their guarantee.
- **Backpressure First** – `icdc_backpressure_signal.h` is a required interface; consumers
  that cannot keep up propagate backpressure to the CDC engine.
- **Schema Validated Events** – `icdc_event_schema.h` defines the event schema contract;
  all events are validated against the schema before delivery.
- **Pause/Resume Control** – `icdc_pause_control.h` provides fine-grained pause/resume for
  individual feeds and consumer groups.
- **Fan-In Aggregation** – `icdc_fan_in.h` allows multiple upstream CDC sources to be merged
  into a single ordered stream.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `changefeed.h` | `IChangefeed`, `ChangefeedConfig`, `ChangeEvent` | Core changefeed subscription |
| `changefeed_buffer.h` | `IChangefeedBuffer` | In-memory ordered change event buffer |
| `consumer_group.h` | `IConsumerGroup`, `ConsumerGroupConfig` | Consumer group coordination |
| `icdc_transport.h` | `ICDCTransport` | Abstract CDC transport interface |
| `icdc_pause_control.h` | `ICDCPauseControl` | Pause/resume for feeds and consumer groups |
| `icdc_backpressure_signal.h` | `ICDCBackpressureSignal` | Consumer backpressure signalling |
| `icdc_fan_in.h` | `ICDCFanIn`, `FanInSource` | Multi-source stream aggregation |
| `icdc_event_schema.h` | `ICDCEventSchema`, `EventSchemaSpec` | Event schema validation contract |
| `idelivery_guarantee_config.h` | `IDeliveryGuaranteeConfig`, `DeliveryGuarantee` enum | Delivery semantics configuration |
| `delivery_tracker.h` | `IDeliveryTracker`, `DeliveryStatus` | Per-event delivery acknowledgement tracking |
| `debezium_format.h` | `DebeziumFormatter`, `DebeziumEvent` | Debezium-compatible event serialisation |
| `kafka_cdc_producer.h` | `IKafkaCDCProducer`, `KafkaProducerConfig` | Kafka CDC event producer |
| `outbox.h` | `IOutboxProcessor`, `OutboxEntry` | Transactional outbox pattern |
| `schema_registry.h` | `ISchemaRegistry`, `SchemaVersion` | Event schema versioning and registry |
| `dead_letter_queue.h` | `IDeadLetterQueue`, `DLQEntry` | Failed event dead letter queue |
| `cross_collection_stream.h` | `ICrossCollectionStream` | Multi-collection change stream |
| `tenant_buffer_manager.h` | `ITenantBufferManager` | Per-tenant CDC buffer lifecycle |
| `change_stream_compressor.h` | `IChangeStreamCompressor` | Change stream compression |
| `cdc_materialized_view.h` | `ICDCMaterializedView` | CDC-driven materialised view maintenance |
| `cdc_ws_handler.h` | `ICDCWSHandler` | CDC events over WebSocket |
| `ws_transport.h` | `IWSTransport` | WebSocket transport implementation |
| `cdc_admin.h` | `ICDCAdmin`, `CDCAdminConfig` | CDC admin operations |
| `cdc_error.h` | `CDCErrorCode` enum | Canonical CDC error taxonomy |
| `cdc_metrics.h` | `CDCMetrics` | CDC metric descriptors |

> **Implementation details:** `../../src/cdc/`
