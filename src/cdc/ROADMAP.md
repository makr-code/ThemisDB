# CDC (Change Data Capture) Module Roadmap

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
