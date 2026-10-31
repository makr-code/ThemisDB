---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "api-contract"
Canonical source references: "include/cdc/changefeed.h; include/cdc/consumer_group.h; include/cdc/outbox.h; include/cdc/kafka_cdc_producer.h; include/cdc/cdc_ws_handler.h; tests/cdc/CMakeLists.txt"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC public headers

The headers in this directory expose the CDC feed, delivery, replay, transport,
and administration interfaces. `Changefeed` is declared in namespace `themis`;
most other CDC types are in `themis::cdc`. The headers and
[`cdc_delivery_contract.h`](./cdc_delivery_contract.h) define the normative
contracts. Optional transport declarations do not mean the backend is enabled
in a particular build.

## Header map

| Header | Main public type(s) | Purpose |
|---|---|---|
| `changefeed.h` | `themis::Changefeed` | Event persistence, sequence queries, subscriptions, retention |
| `changefeed_buffer.h` | `themis::ChangefeedBuffer` | Configurable buffering and batch flush |
| `consumer_group.h` | `themis::cdc::ConsumerGroupManager` | Durable group configuration/offsets and partitioned fetch/ack |
| `delivery_tracker.h` | `themis::cdc::DeliveryTracker` | In-memory pending delivery, acknowledgement, redelivery |
| `icdc_replay_controller.h` | `ICDCReplayController` | Bounded replay sessions |
| `outbox.h` | `OutboxWriter`, `OutboxRelay` | Transaction-aware enqueue and asynchronous feed relay |
| `dead_letter_queue.h` | `DeadLetterQueue` | Failed-event storage and replay operations |
| `icdc_transport.h` | `ICDCTransport` | Minimal `start`/`stop`/single-event `publish` interface |
| `kafka_cdc_producer.h` | `KafkaCDCProducer`, `KafkaProducerConfig` | Optional Kafka transport (`THEMIS_ENABLE_KAFKA`) |
| `ws_transport.h` | `WsTransport` | Optional WebSocket polling and subscription transport |
| `cdc_ws_handler.h` | `CdcWebSocketHandler` | WebSocket frame handling and feed polling |
| `debezium_format.h` | `DebeziumFormatter` | Debezium-compatible event envelope formatting |
| `schema_registry.h` | `SchemaRegistryClient`, `CdcSchemaEncoder` | Schema registry abstraction and encoding |
| `icdc_event_schema.h` | `ICDCEventSchema` | Event schema interface |
| `icdc_filter_pipeline.h` | `ICDCFilterPipeline` | Composable event filtering |
| `icdc_fan_in.h` | `ICDCFanIn` | Multi-source event aggregation |
| `icdc_batch_commit_coordinator.h` | `ICDCBatchCommitCoordinator` | Batch commit coordination |
| `icdc_backpressure_signal.h` | `ICDCBackpressureSignal` | Backpressure signaling |
| `icdc_pause_control.h` | `ICDCPauseControl` | Pause/resume control |
| `idelivery_guarantee_config.h` | `IDeliveryGuaranteeConfig` | Delivery configuration interface |
| `cross_collection_stream.h` | `CrossCollectionStream` | Cross-collection stream operations |
| `tenant_buffer_manager.h` | `TenantBufferManager` | Tenant-scoped buffer management |
| `change_stream_compressor.h` | `ChangeStreamCompressor` | Change-event compression |
| `cdc_materialized_view.h` | `CDCMaterializedViewMaintainer` | CDC event application to incremental views |
| `cdc_admin.h` | `CDCAdmin` | Purge, replay, compaction, redaction, health, and diagnostics |
| `cdc_metrics.h` | `CDCMetrics` | CDC counters, rates, and latency summaries |
| `cdc_error.h` | `CDCException`, `ErrorCode` | Typed CDC errors |
| `cdc_delivery_contract.h` | Contract constants and failure classes | Shared sequence, delivery, replay, and failure invariants |

## Configuration and runtime limits

- `Changefeed::RetentionPolicy` uses `max_age_hours`, `max_event_count`,
  `max_size_bytes`, `cleanup_interval`, and `compact_on_cleanup`. Automatic
  cleanup is disabled by default.
- `DeliveryTrackerConfig` includes acknowledgement timeout, recheck interval,
  redelivery-attempt limit, and pending-event cap. `trackDelivery` accepts a
  vector of events and returns `false` when delivery cannot be tracked.
- `ConsumerGroupManager::fetchEventsAtLeastOnce` accepts a limit and
  acknowledgement timeout; committed offsets persist, while in-flight records
  are process-local.
- `KafkaProducerConfig` supplies broker, topic, acknowledgement, idempotence,
  polling, security, and flush settings. Kafka requires `THEMIS_ENABLE_KAFKA`
  and librdkafka. Without that flag, `KafkaCDCProducer::start()` and
  `publish()` return `false` unless the fallback's injectable callbacks are
  explicitly configured; the fallback does not connect to Kafka.
- `WsTransport` is declared only when `THEMIS_ENABLE_WEBSOCKET` is enabled.
- Example deployment configuration files are
  [`cdc_retention.yaml`](../../config/data_management/cdc_retention.yaml) and
  [`cdc_kafka.yaml`](../../config/data_management/cdc_kafka.yaml). The Kafka
  configuration contains credentials as placeholders; resolve secrets through
  the documented `ConfigPathResolver` instead of committing credentials.

## Build and focused tests

The repository presets are defined in [`CMakePresets.json`](../../CMakePresets.json).
Configure with a preset supported on the host and enable tests:

```sh
cmake --preset linux-debug -DTHEMIS_BUILD_TESTS=ON
cmake --build build-gcc-linux-debug --target module_cdc_test_cdc_admin_focused
ctest --test-dir build-gcc-linux-debug -R '^test_cdc_admin_cdc_FocusedTests$' --output-on-failure
```

`tests/cdc/CMakeLists.txt` creates a `module_cdc_<test-name>_focused` target for
each `test_*.cpp`; use the corresponding target and CTest name for another
focused test. Required dependencies must be available to the selected preset.

To configure the two registered CDC benchmarks, enable benchmarks in the
chosen preset and build `bench_cdc_pipeline` or `bench_cdc_delivery_gates`.
`bench_changefeed_throughput.cpp` is a benchmark source in this directory but
is not registered as a CMake target in `benchmarks/CMakeLists.txt`.

## Delivery and transaction boundaries

CDC delivery is at-least-once where specified by the delivery contract, so
consumers must account for redelivery. An outbox record is written using the
caller's RocksDB transaction; forwarding it to the changefeed and updating its
outbox state are later, separate operations. Do not treat the outbox relay as
an atomic transaction spanning the application write, feed append, and relay
state update.

## Related documentation

- [`../../src/cdc/README.md`](../../src/cdc/README.md) — module overview
- [`../../src/cdc/ARCHITECTURE.md`](../../src/cdc/ARCHITECTURE.md) — data flow and implementation boundaries
- [`../../src/cdc/API.md`](../../src/cdc/API.md) — public API contracts
- [`../../src/cdc/ROADMAP.md`](../../src/cdc/ROADMAP.md) — canonical implementation status
- [`../../src/cdc/SECURITY.md`](../../src/cdc/SECURITY.md) — security context
- [`../../benchmarks/cdc/README.md`](../../benchmarks/cdc/README.md) — benchmark sources and targets

**Exact model backend not verifiable from final document text alone.**
