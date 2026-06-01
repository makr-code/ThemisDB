> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/cdc/ARCHITECTURE.md -->

# CDC (Change Data Capture) Module — Public Header Architecture

**Module Path:** `include/cdc/`  
**Implementation:** `../../src/cdc/`  
**Canonical architecture doc:** [`../../src/cdc/ARCHITECTURE.md`](../../src/cdc/ARCHITECTURE.md)

---

## 1. Overview

`include/cdc/` defines the **public change feeds, delivery guarantees, Kafka CDC, Debezium format, fan-in, replay, pause/resume, WebSocket transport, and outbox pattern API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/cdc/ARCHITECTURE.md`](../../src/cdc/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core CDC API

| Header | Public Type | Purpose |
|--------|------------|---------|
| `changefeed.h` | `Changefeed` | Primary change-feed subscription entry point |
| `changefeed_buffer.h` | `ChangefeedBuffer` | Bounded change-event buffer |
| `cdc_admin.h` | `CDCAdmin` | Admin API for feed lifecycle management |
| `cdc_error.h` | `CDCError` | Typed CDC error codes |
| `cdc_metrics.h` | `CDCMetrics` | Change-feed telemetry and counters |
| `cdc_ws_handler.h` | `CDCWSHandler` | WebSocket handler for change events |
### 2.2 Delivery and Reliability

| Header | Public Type | Purpose |
|--------|------------|---------|
| `icdc_backpressure_signal.h` | `ICDCBackpressureSignal` | Back-pressure signalling interface |
| `icdc_batch_commit_coordinator.h` | `ICDCBatchCommitCoordinator` | Batched commit coordination |
| `icdc_pause_control.h` | `ICDCPauseControl` | Feed pause and resume control |
| `icdc_replay_controller.h` | `ICDCReplayController` | Historical replay from checkpoint |
| `delivery_tracker.h` | `DeliveryTracker` | At-least-once delivery tracking |
| `dead_letter_queue.h` | `DeadLetterQueue` | Failed-delivery quarantine queue |
| `idelivery_guarantee_config.h` | `IDeliveryGuaranteeConfig` | Delivery-guarantee configuration interface |
| `outbox.h` | `Outbox` | Transactional outbox for durable CDC |
### 2.3 Schema and Format

| Header | Public Type | Purpose |
|--------|------------|---------|
| `icdc_event_schema.h` | `ICDCEventSchema` | Event schema contract interface |
| `debezium_format.h` | `DebeziumFormat` | Debezium-compatible change event serialisation |
| `schema_registry.h` | `SchemaRegistry` | Schema registry client for CDC events |
| `icdc_filter_pipeline.h` | `ICDCFilterPipeline` | Composable event filter pipeline |
### 2.4 Transport and Fan-out

| Header | Public Type | Purpose |
|--------|------------|---------|
| `kafka_cdc_producer.h` | `KafkaCDCProducer` | Kafka producer for change events |
| `icdc_transport.h` | `ICDCTransport` | Pluggable transport interface |
| `ws_transport.h` | `WSTransport` | WebSocket transport adapter |
| `icdc_fan_in.h` | `ICDCFanIn` | Multi-source fan-in aggregator |
| `cross_collection_stream.h` | `CrossCollectionStream` | Cross-collection change stream |
| `consumer_group.h` | `ConsumerGroup` | Consumer group coordination |
| `tenant_buffer_manager.h` | `TenantBufferManager` | Per-tenant event buffer management |
| `change_stream_compressor.h` | `ChangeStreamCompressor` | Lossless change stream compression |
### 2.5 Derived Views

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cdc_materialized_view.h` | `CDCMaterializedView` | CDC-driven materialised view maintenance |

---

## 3. Namespace Layout

All public types reside in the `themis::cdc` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/cdc/` expose the **stable public API**; internal types live in `src/cdc/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph/Tensor**.
