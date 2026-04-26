> **Build:** `cmake --preset release && cmake --build build/release`

# CDC Module — Public Headers

**Module Path:** `include/cdc/`
**Implementation:** `../../src/cdc/`

## Purpose

Public interfaces for ThemisDB's Change Data Capture (CDC) subsystem — streaming change events, delivery guarantees, and fan-out pipelines.

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `changefeed.h` | `Changefeed` — primary CDC stream entry point |
| `changefeed_buffer.h` | `ChangefeedBuffer` — in-memory change event buffer |
| `icdc_transport.h` | `ICDCTransport` — abstract transport interface |
| `icdc_event_schema.h` | `ICDCEventSchema` — event serialization contract |
| `icdc_filter_pipeline.h` | `ICDCFilterPipeline` — composable event filter chain |
| `icdc_fan_in.h` | `ICDCFanIn` — multi-source event aggregation |
| `icdc_batch_commit_coordinator.h` | `ICDCBatchCommitCoordinator` — atomic batch delivery |
| `icdc_backpressure_signal.h` | `ICDCBackpressureSignal` — consumer flow-control interface |
| `icdc_pause_control.h` | `ICDCPauseControl` — pause/resume stream control |
| `icdc_replay_controller.h` | `ICDCReplayController` — offset-based event replay |
| `idelivery_guarantee_config.h` | `IDeliveryGuaranteeConfig` — at-least-once / exactly-once config |
| `consumer_group.h` | `ConsumerGroup` — coordinated multi-consumer group |
| `cross_collection_stream.h` | `CrossCollectionStream` — multi-collection CDC stream |
| `dead_letter_queue.h` | `DeadLetterQueue` — failed-event DLQ |
| `delivery_tracker.h` | `DeliveryTracker` — per-event delivery state tracking |
| `tenant_buffer_manager.h` | `TenantBufferManager` — per-tenant isolated change buffers |
| `kafka_cdc_producer.h` | `KafkaCDCProducer` — Kafka transport adapter |
| `ws_transport.h` | `WsTransport` — WebSocket CDC transport |
| `debezium_format.h` | `DebeziumFormat` — Debezium-compatible event serializer |
| `schema_registry.h` | `SchemaRegistry` — Avro/Protobuf schema registry client |
| `outbox.h` | `Outbox` — transactional outbox pattern implementation |
| `change_stream_compressor.h` | `ChangeStreamCompressor` — delta compression for change events |
| `cdc_materialized_view.h` | `CDCMaterializedView` — CDC-driven incremental materialized view |
| `cdc_ws_handler.h` | `CDCWsHandler` — WebSocket handler for CDC subscriptions |
| `cdc_admin.h` | `CDCAdmin` — administrative API (pause, resume, reset) |
| `cdc_metrics.h` | `CDCMetrics` — throughput, lag and error counters |
| `cdc_error.h` | `CDCError`, `CDCException` — structured CDC error types |

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-cdc
```

## See Also

- [`../../src/cdc/README.md`](../../src/cdc/README.md) — implementation details
- [`../../src/cdc/ARCHITECTURE.md`](../../src/cdc/ARCHITECTURE.md) — architecture guide

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
