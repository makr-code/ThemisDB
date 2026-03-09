# CDC (Change Data Capture) Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — Real-time change notifications, SSE-based event streaming, WebSocket transport, consumer group semantics, and Kafka producer integration are all implemented.

## Completed ✅
- [x] Changefeed implementation for real-time change tracking
- [x] Server-Sent Events (SSE) streaming of change events
- [x] Change log management and persistence
- [x] Subscription management (per-collection, per-key filtering)
- [x] Filtered change subscriptions (table/key/event-type filters)
- [x] Historical change replay from stored change log
- [x] Integration with analytics diff engine
- [x] Dead-letter queue for failed event deliveries (Issue: #1610)
- [x] WebSocket-based change streaming as alternative to SSE (Target: Q2 2026) (Issue: #1604)
- [x] Change log compaction and archival policies (Target: Q2 2026) (Issue: #1605)
- [x] At-least-once delivery guarantees with consumer acknowledgement (Target: Q3 2026) (Issue: #1606)
- [x] Change log TTL and size-based retention policies (Issue: #1608)
- [x] Consumer group semantics (multiple consumers, offset tracking) (Issue: #1609)
- [x] Change event enrichment (before/after document snapshots) (Issue: #1611)
- [x] Kafka-compatible producer interface for enterprise integration (Issue: #1613)
- [x] Debezium-compatible change event format (Issue: #1614)
- [x] Cross-collection change aggregation streams (Issue: #1615)
- [x] GDPR-aware change log redaction (PII field scrubbing) (Issue: #1616)
- [x] CDC-based materialized view maintenance (Issue: #1617)
- [x] Change stream compression for high-volume feeds (Issue: #1618)
- [x] Outbox pattern support for transactional change publishing (`cdc/outbox.cpp`) (Issue: #1612)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
*(no open short-term items -- all previously planned features are implemented)*

### Long-term (6-12 months)
*(no open items — all previously planned features are implemented)*

## Implementation Phases

### Phase 1: Changefeed and SSE Streaming (Status: Completed)
- [x] Implemented changefeed engine tracking insert/update/delete events per collection
- [x] Implemented Server-Sent Events (SSE) streaming transport (embedded in `cdc/changefeed.cpp`)
- [x] Implemented persistent change log with append-only storage
- [x] Implemented subscription management with per-collection and per-key filters
- [x] Implemented historical change replay from stored change log offset
- [x] Integrated with analytics diff engine for before/after document snapshots

### Phase 2: WebSocket Transport and Delivery Guarantees (Status: Completed ✅)
- [x] Implement WebSocket transport as alternative to SSE (`cdc/cdc_ws_handler.cpp`, `/v2/cdc/stream`) (Issue: #1626)
- [x] Implement change log compaction to merge superseded entries by key (Issue: #1627)
- [x] Implement at-least-once delivery with consumer acknowledgement and redelivery (Issue: #1628)

### Phase 3: Consumer Groups and Enterprise Integration (Status: Completed ✅)
- [x] Implement consumer group semantics with offset tracking per group (`cdc/consumer_group.cpp`) (Issue: #1619)
- [x] Implement Kafka-compatible producer interface for enterprise CDC pipelines (Issue: #1620)
- [x] Add Debezium-compatible change event envelope format (Issue: #1621)
- [x] Implement GDPR-aware change log redaction for configured PII fields (Issue: #1622)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1623)
- [x] Integration tests (SSE streaming, change replay, subscription filtering)
- [x] Performance benchmarks (event throughput, latency) (Issue: #1624)
- [x] Security audit (subscription authorization, data leakage) (Issue: #1625)
- [x] Documentation complete
- [x] API stability guaranteed for changefeed and subscription APIs

## Known Issues & Limitations
- WebSocket transport is implemented (`cdc_ws_handler.cpp`); endpoint wiring to HTTP server is a follow-up
- Consumer offset tracking is available via `ConsumerGroupManager`;
  full log scan is no longer required for existing groups
- Change log retention policies are not configurable at runtime
- At-least-once delivery is implemented via
  `ConsumerGroupManager::fetchEventsAtLeastOnce`;
  in-flight state is in-memory and resets on server restart
  (consumers resume from the last durably committed offset)
- At-least-once delivery is not yet guaranteed for SSE connections
- Dead-letter queue captures events that exhaust delivery retries; events that fail due to payload decompression errors are logged but not enqueued in the DLQ (data is not recoverable in that case)
- Outbox relay in-flight state is in-memory; FAILED records survive restarts but PENDING records relayed-but-not-marked would be re-relayed after restart (at-least-once semantics)

## Breaking Changes
- Consumer group API will be a new interface (additive, non-breaking to existing subscriptions)
- Kafka producer interface will require separate configuration block
