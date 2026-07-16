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

## Public API by Concern

- **Core stream ingestion/replay:** `changefeed.h`, `changefeed_buffer.h`
- **Delivery guarantees and replay control:** `delivery_tracker.h`, `consumer_group.h`, `icdc_replay_controller.h`, `idelivery_guarantee_config.h`
- **Flow control and pipeline orchestration:** `icdc_pause_control.h`, `icdc_backpressure_signal.h`, `icdc_filter_pipeline.h`, `icdc_batch_commit_coordinator.h`, `icdc_fan_in.h`
- **Transports:** `icdc_transport.h`, `ws_transport.h`, `cdc_ws_handler.h`, `kafka_cdc_producer.h`
- **Operational/admin surfaces:** `cdc_admin.h`, `dead_letter_queue.h`, `tenant_buffer_manager.h`, `cdc_metrics.h`, `cdc_error.h`
- **Integration/data-shape helpers:** `debezium_format.h`, `schema_registry.h`, `change_stream_compressor.h`, `cross_collection_stream.h`, `outbox.h`, `cdc_materialized_view.h`, `icdc_event_schema.h`

## Runtime Configuration Surfaces

Public header APIs expose the CDC knobs used by runtime configuration:

- `changefeed.h` → `Changefeed::RetentionPolicy` (`enabled`, `max_age`, `max_entries`, `max_bytes`, `cleanup_interval`, `compact_on_cleanup`)
- `delivery_tracker.h` → `DeliveryTrackerConfig` (`ack_timeout`, `max_redelivery_attempts`, sweep intervals)
- `consumer_group.h` → `fetchEventsAtLeastOnce(..., ack_timeout_ms)` and group offset controls
- `kafka_cdc_producer.h` → `KafkaProducerConfig` (`brokers`, `topic_prefix`, batching/auth settings)
- `cdc_admin.h` → retention/admin read/write status endpoints and operator controls

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-cdc
```

## Usage

### C++: create a delivery tracker and acknowledge events

```cpp
#include "cdc/delivery_tracker.h"

themis::cdc::DeliveryTrackerConfig cfg;
cfg.ack_timeout = std::chrono::milliseconds(30'000);
cfg.max_redelivery_attempts = 5;

themis::cdc::DeliveryTracker tracker(cfg);
tracker.trackDelivery("ops-bot", 12345, "orders/42");
tracker.acknowledge("ops-bot", 12345);
```

### C++: configure Kafka producer topic prefix

```cpp
#include "cdc/kafka_cdc_producer.h"

themis::cdc::KafkaProducerConfig cfg;
cfg.brokers = "kafka-1:9092,kafka-2:9092";
cfg.topic_prefix = "themis.cdc.";
```

## Runtime Behavior, Errors, and Limits

- CDC delivery semantics are at-least-once; duplicate deliveries are expected across reconnect/timeouts.
- Slow consumers can trigger backlog growth and redelivery cycles; use ack and backpressure controls.
- Kafka publishing is available only when built with `THEMIS_ENABLE_KAFKA`; no-op stub otherwise.
- DLQ persistence captures retry-exhausted events for later replay or drain.

## Troubleshooting

- **`start()`/`publish()` returns `false` for Kafka producer:** verify build uses `THEMIS_ENABLE_KAFKA` and librdkafka is present.
- **Unexpected duplicates:** confirm consumer acknowledgement logic and deduplication behavior for at-least-once mode.
- **Replay gaps:** check retention policy limits and whether source events aged out before replay.

## See Also

- [`../../src/cdc/README.md`](../../src/cdc/README.md) — implementation details
- [`../../src/cdc/ARCHITECTURE.md`](../../src/cdc/ARCHITECTURE.md) — architecture guide
- [`../../src/cdc/SECURITY.md`](../../src/cdc/SECURITY.md) — threat model and controls
- [`../../src/cdc/AUDIT.md`](../../src/cdc/AUDIT.md) — build/test/compliance snapshot
- [`../../src/cdc/CHANGELOG.md`](../../src/cdc/CHANGELOG.md) — module history
- [`../../src/cdc/PERFORMANCE_EXPECTATIONS.md`](../../src/cdc/PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [`../../src/cdc/ROADMAP.md`](../../src/cdc/ROADMAP.md) — status and implementation phases
- [`../../src/cdc/FUTURE_ENHANCEMENTS.md`](../../src/cdc/FUTURE_ENHANCEMENTS.md) — long-term backlog
- [`../../docs/en/cdc/PRIMARY_SOURCES.md`](../../docs/en/cdc/PRIMARY_SOURCES.md) — canonical source index (EN)
- [`../../docs/de/cdc/PRIMARY_SOURCES.md`](../../docs/de/cdc/PRIMARY_SOURCES.md) — Quellenindex (DE)

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
