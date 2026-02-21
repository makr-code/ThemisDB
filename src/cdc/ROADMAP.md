# CDC (Change Data Capture) Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Real-time change notifications, SSE-based event streaming, filtered subscriptions, and historical change replay are functional. WebSocket transport and Kafka/Kinesis integration are planned.

## Completed ✅
- [x] Changefeed implementation for real-time change tracking
- [x] Server-Sent Events (SSE) streaming of change events
- [x] Change log management and persistence
- [x] Subscription management (per-collection, per-key filtering)
- [x] Filtered change subscriptions (table/key/event-type filters)
- [x] Historical change replay from stored change log
- [x] Integration with analytics diff engine

## In Progress 🚧
- [I] WebSocket-based change streaming as alternative to SSE (Target: Q2 2026) (Issue: #1604)
- [I] Change log compaction and archival policies (Target: Q2 2026) (Issue: #1605)
- [I] At-least-once delivery guarantees with consumer acknowledgement (Target: Q3 2026) (Issue: #1606)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] WebSocket transport for bidirectional change feeds (Issue: #1607)
- [I] Change log TTL and size-based retention policies (Issue: #1608)
- [I] Consumer group semantics (multiple consumers, offset tracking) (Issue: #1609)
- [I] Dead-letter queue for failed event deliveries (Issue: #1610)
- [I] Change event enrichment (before/after document snapshots) (Issue: #1611)
- [I] Outbox pattern support for transactional change publishing (Issue: #1612)

### Long-term (6-12 months)
- [I] Kafka-compatible producer interface for enterprise integration (Issue: #1613)
- [I] Debezium-compatible change event format (Issue: #1614)
- [I] Cross-collection change aggregation streams (Issue: #1615)
- [I] GDPR-aware change log redaction (PII field scrubbing) (Issue: #1616)
- [I] CDC-based materialized view maintenance (Issue: #1617)
- [I] Change stream compression for high-volume feeds (Issue: #1618)

## Implementation Phases

### Phase 1: Changefeed and SSE Streaming (Status: Completed)
- [x] Implemented changefeed engine tracking insert/update/delete events per collection
- [x] Implemented Server-Sent Events (SSE) streaming transport (`cdc/sse_transport.cpp`)
- [x] Implemented persistent change log with append-only storage
- [x] Implemented subscription management with per-collection and per-key filters
- [x] Implemented historical change replay from stored change log offset
- [x] Integrated with analytics diff engine for before/after document snapshots

### Phase 2: WebSocket Transport and Delivery Guarantees (Status: In Progress)
- [I] Implement WebSocket transport as alternative to SSE (`cdc/ws_transport.cpp`) (Issue: #1626)
- [I] Implement change log compaction to merge superseded entries by key (Issue: #1627)
- [I] Implement at-least-once delivery with consumer acknowledgement and redelivery (Issue: #1628)

### Phase 3: Consumer Groups and Enterprise Integration (Status: Planned)
- [I] Implement consumer group semantics with offset tracking per group (`cdc/consumer_group.cpp`) (Issue: #1619)
- [I] Implement Kafka-compatible producer interface for enterprise CDC pipelines (Issue: #1620)
- [I] Add Debezium-compatible change event envelope format (Issue: #1621)
- [I] Implement GDPR-aware change log redaction for configured PII fields (Issue: #1622)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1623)
- [x] Integration tests (SSE streaming, change replay, subscription filtering)
- [I] Performance benchmarks (event throughput, latency) (Issue: #1624)
- [I] Security audit (subscription authorization, data leakage) (Issue: #1625)
- [x] Documentation complete
- [x] API stability guaranteed for changefeed and subscription APIs

## Known Issues & Limitations
- WebSocket transport not yet implemented; SSE only
- No consumer offset tracking; replay requires full log scan
- Change log retention policies are not configurable at runtime
- At-least-once delivery is not yet guaranteed for SSE connections

## Breaking Changes
- Consumer group API will be a new interface (additive, non-breaking to existing subscriptions)
- Kafka producer interface will require separate configuration block
