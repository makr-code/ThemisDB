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
- [ ] WebSocket-based change streaming as alternative to SSE (Target: Q2 2026)
- [ ] Change log compaction and archival policies (Target: Q2 2026)
- [ ] At-least-once delivery guarantees with consumer acknowledgement (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] WebSocket transport for bidirectional change feeds
- [ ] Change log TTL and size-based retention policies
- [ ] Consumer group semantics (multiple consumers, offset tracking)
- [ ] Dead-letter queue for failed event deliveries
- [ ] Change event enrichment (before/after document snapshots)
- [ ] Outbox pattern support for transactional change publishing

### Long-term (6-12 months)
- [ ] Kafka-compatible producer interface for enterprise integration
- [ ] Debezium-compatible change event format
- [ ] Cross-collection change aggregation streams
- [ ] GDPR-aware change log redaction (PII field scrubbing)
- [ ] CDC-based materialized view maintenance
- [ ] Change stream compression for high-volume feeds

## Implementation Phases

### Phase 1: Changefeed and SSE Streaming (Status: Completed)
- [x] Implemented changefeed engine tracking insert/update/delete events per collection
- [x] Implemented Server-Sent Events (SSE) streaming transport (`cdc/sse_transport.cpp`)
- [x] Implemented persistent change log with append-only storage
- [x] Implemented subscription management with per-collection and per-key filters
- [x] Implemented historical change replay from stored change log offset
- [x] Integrated with analytics diff engine for before/after document snapshots

### Phase 2: WebSocket Transport and Delivery Guarantees (Status: In Progress)
- [~] Implement WebSocket transport as alternative to SSE (`cdc/ws_transport.cpp`)
- [~] Implement change log compaction to merge superseded entries by key
- [~] Implement at-least-once delivery with consumer acknowledgement and redelivery

### Phase 3: Consumer Groups and Enterprise Integration (Status: Planned)
- [ ] Implement consumer group semantics with offset tracking per group (`cdc/consumer_group.cpp`)
- [ ] Implement Kafka-compatible producer interface for enterprise CDC pipelines
- [ ] Add Debezium-compatible change event envelope format
- [ ] Implement GDPR-aware change log redaction for configured PII fields

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (SSE streaming, change replay, subscription filtering)
- [ ] Performance benchmarks (event throughput, latency)
- [ ] Security audit (subscription authorization, data leakage)
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
