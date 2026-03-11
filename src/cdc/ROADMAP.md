# CDC (Change Data Capture) Module Roadmap
<!-- Status: current | validated: 2026-03-10 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · include/cdc/FUTURE_ENHANCEMENTS.md · docs/de/cdc/ -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — Real-time change notifications, SSE-based event streaming, WebSocket transport, consumer group semantics, and Kafka producer integration are all implemented. Build system audit completed (2026-03-10): all CDC source files are now registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`; standalone focused test targets added for `CDCAdminFocusedTests` and `TenantBufferManagerFocusedTests`.

## Completed ✅
- [x] Changefeed implementation for real-time change tracking — `changefeed.cpp`, `include/cdc/changefeed.h`
- [x] Server-Sent Events (SSE) streaming of change events — SSE transport embedded in `changefeed.cpp`
- [x] Change log management and persistence — `changefeed.cpp`, `changefeed_buffer.cpp`
- [x] Subscription management (per-collection, per-key filtering) — `changefeed.cpp`, `cdc_admin.cpp`
- [x] Filtered change subscriptions (table/key/event-type filters) — `changefeed.cpp`
- [x] Historical change replay from stored change log — `changefeed.cpp` (`replayFrom()`)
- [x] Integration with analytics diff engine — `cdc_materialized_view.cpp`
- [x] Dead-letter queue for failed event deliveries (Issue: #1610) — `dead_letter_queue.cpp`, `include/cdc/dead_letter_queue.h`; RocksDB key prefix `dlq:`; `listEntries()`, `replay()`, `drain()`
- [x] WebSocket-based change streaming as alternative to SSE (Target: Q2 2026) (Issue: #1604) — `ws_transport.cpp` (`WsTransport`, implements `ICDCTransport`), `cdc_ws_handler.cpp` (`/v2/cdc/stream`)
- [x] Change log compaction and archival policies (Target: Q2 2026) (Issue: #1605) — `changefeed.cpp`
- [x] At-least-once delivery guarantees with consumer acknowledgement (Target: Q3 2026) (Issue: #1606) — `delivery_tracker.cpp`, `include/cdc/delivery_tracker.h`; `trackDelivery()`, `acknowledge()`, `acknowledgeUpTo()`, `getPendingRedelivery()`; 18 unit tests in `tests/test_cdc_delivery_tracker.cpp`; SSE at-least-once added: `consumer_id` + `ack_timeout_ms` query params on `GET /changefeed/stream`, `POST /changefeed/stream/ack`, 5 integration tests in `tests/test_http_changefeed_sse.cpp`
- [x] Change log TTL and size-based retention policies (Issue: #1608) — `changefeed.cpp`; manual admin trim via `CDCAdmin::purgeOlderThan()` (see Known Issues: runtime config not yet available)
- [x] Consumer group semantics (multiple consumers, offset tracking) (Issue: #1609) — `consumer_group.cpp`, `include/cdc/consumer_group.h`; `ConsumerGroupManager`, `fetchEventsAtLeastOnce()`
- [x] Change event enrichment (before/after document snapshots) (Issue: #1611) — `changefeed.cpp`; `ChangeEvent::before_snapshot`, `after_snapshot`
- [x] Kafka-compatible producer interface for enterprise integration (Issue: #1613) — `kafka_cdc_producer.cpp`, `include/cdc/kafka_cdc_producer.h`; `KafkaCDCProducer` (opt-in: `THEMIS_ENABLE_KAFKA`)
- [x] Debezium-compatible change event format (Issue: #1614) — `include/cdc/debezium_format.h` (header-only); `DebeziumFormatter::toEnvelope()`, `toJson()`, `toJsonWithSchema()`; 23 unit tests in `tests/test_cdc_debezium_format.cpp`
- [x] Cross-collection change aggregation streams (Issue: #1615) — `cross_collection_stream.cpp`, `include/cdc/cross_collection_stream.h`; `CrossCollectionStream`; 19 unit tests in `tests/test_cdc_cross_collection_stream.cpp`
- [x] GDPR-aware change log redaction (PII field scrubbing) (Issue: #1616) — `changefeed.cpp`; PII field masking on `before` field for fields annotated with `encryption`/`pii`
- [x] CDC-based materialized view maintenance (Issue: #1617) — `cdc_materialized_view.cpp`, `include/cdc/cdc_materialized_view.h`; `CDCMaterializedViewMaintainer`
- [x] Change stream compression for high-volume feeds (Issue: #1618) — `include/cdc/change_stream_compressor.h`
- [x] Outbox pattern support for transactional change publishing (`cdc/outbox.cpp`) (Issue: #1612) — `outbox.cpp`, `include/cdc/outbox.h`; `OutboxWriter`, `OutboxRelay`; 16 unit tests in `tests/test_cdc_outbox.cpp`

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
*(no open short-term items -- all previously planned features are implemented)*

### Long-term (6-12 months)
*(no open items — all previously planned features are implemented)*

## Implementation Phases

### Phase 1: Changefeed and SSE Streaming (Status: Completed)
- [x] Implemented changefeed engine tracking insert/update/delete events per collection — `changefeed.cpp`, `include/cdc/changefeed.h`
- [x] Implemented Server-Sent Events (SSE) streaming transport (embedded in `cdc/changefeed.cpp`)
- [x] Implemented persistent change log with append-only storage — `changefeed.cpp`, `changefeed_buffer.cpp`, `include/cdc/changefeed_buffer.h`
- [x] Implemented subscription management with per-collection and per-key filters — `changefeed.cpp`, `cdc_admin.cpp`
- [x] Implemented historical change replay from stored change log offset — `changefeed.cpp` (`replayFrom()`)
- [x] Integrated with analytics diff engine for before/after document snapshots — `cdc_materialized_view.cpp`

### Phase 2: WebSocket Transport and Delivery Guarantees (Status: Completed ✅)
- [x] Implement WebSocket transport as alternative to SSE (`cdc/cdc_ws_handler.cpp`, `/v2/cdc/stream`) (Issue: #1626) — `ws_transport.cpp` (`WsTransport`), `cdc_ws_handler.cpp`; implements `ICDCTransport` (`include/cdc/icdc_transport.h`)
- [x] Implement change log compaction to merge superseded entries by key (Issue: #1627) — `changefeed.cpp`
- [x] Implement at-least-once delivery with consumer acknowledgement and redelivery (Issue: #1628) — `delivery_tracker.cpp`; `trackDelivery()`, `acknowledge()`, `getPendingRedelivery()`; 18 tests in `tests/test_cdc_delivery_tracker.cpp`

### Phase 3: Consumer Groups and Enterprise Integration (Status: Completed ✅)
- [x] Implement consumer group semantics with offset tracking per group (`cdc/consumer_group.cpp`) (Issue: #1619) — `consumer_group.cpp`, `include/cdc/consumer_group.h`; `ConsumerGroupManager`
- [x] Implement Kafka-compatible producer interface for enterprise CDC pipelines (Issue: #1620) — `kafka_cdc_producer.cpp`, `include/cdc/kafka_cdc_producer.h`; `KafkaCDCProducer`
- [x] Add Debezium-compatible change event envelope format (Issue: #1621) — `include/cdc/debezium_format.h`; 23 tests in `tests/test_cdc_debezium_format.cpp`
- [x] Implement GDPR-aware change log redaction for configured PII fields (Issue: #1622) — `changefeed.cpp`; fields with `encryption`/`pii` annotations masked in `before` field

### Phase 4: Build System Audit (Status: Completed ✅)
- [x] Register `changefeed_buffer.cpp` and `tenant_buffer_manager.cpp` in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake` — previously missing from both build files
- [x] Register `cross_collection_stream.cpp` and `cdc_materialized_view.cpp` in `cmake/CMakeLists.txt` — previously only in `ModularBuild.cmake`
- [x] Register `cdc_admin.cpp` in `cmake/CMakeLists.txt` (conditional on `THEMIS_ENABLE_HTTP_SERVER`) — previously only in `ModularBuild.cmake`
- [x] Register `consumer_group.cpp`, `delivery_tracker.cpp`, `outbox.cpp`, and `ws_transport.cpp` in `cmake/ModularBuild.cmake` THEMIS_STORAGE_SOURCES — previously only in `CMakeLists.txt`
- [x] Add all CDC source files to `_themis_test_extra_sources` in `tests/CMakeLists.txt` (including `delivery_tracker.cpp` and `ws_transport.cpp`)
- [x] Add standalone focused test targets `CDCAdminFocusedTests` and `TenantBufferManagerFocusedTests` in `tests/CMakeLists.txt`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1623) — `test_cdc_changefeed_buffer.cpp` (ChangefeedBuffer direct tests) and `test_cdc_changefeed_core.cpp` (subscribe API, SubscriptionHandle/Filter, listEvents variants, getStats, clear, JSON roundtrip) added; closes Issue #1623
- [x] Integration tests (SSE streaming, change replay, subscription filtering)
- [x] Performance benchmarks (event throughput, latency) (Issue: #1624)
- [x] Security audit (subscription authorization, data leakage) (Issue: #1625)
- [x] Documentation complete
- [x] API stability guaranteed for changefeed and subscription APIs
- [x] Build system audit complete — all source files registered in cmake (2026-03-10)

## Known Issues & Limitations
- Consumer offset tracking is available via `ConsumerGroupManager`; full log scan is no longer required for existing groups
- At-least-once delivery is implemented via `ConsumerGroupManager::fetchEventsAtLeastOnce`; in-flight state is in-memory and resets on server restart (consumers resume from the last durably committed offset)
- Dead-letter queue captures events that exhaust delivery retries; events that fail due to payload decompression errors are logged but not enqueued in the DLQ (data is not recoverable in that case)
- Outbox relay in-flight state is in-memory; FAILED records survive restarts but PENDING records relayed-but-not-marked would be re-relayed after restart (at-least-once semantics)

## Breaking Changes
- Consumer group API will be a new interface (additive, non-breaking to existing subscriptions)
- Kafka producer interface will require separate configuration block
