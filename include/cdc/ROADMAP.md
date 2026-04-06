<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — CDC Module Public Headers

**Module Path:** `include/cdc/`  
**Implementation Roadmap:** `../../src/cdc/ROADMAP.md`

---

## Current Status

Public headers at v1.8.0. All five v1.8.0 interfaces (`ICDCPauseControl`,
`ICDCBackpressureSignal`, `ICDCFanIn`, `ICDCEventSchema`, `IDeliveryGuaranteeConfig`)
are present and implemented. Kafka, Debezium, outbox, schema registry, and
materialised view headers are stable.

---

## Completed Features

- [x] `IChangefeed` with delivery guarantee configuration
- [x] `IConsumerGroup` with backpressure injection
- [x] `ICDCPauseControl` for pause/resume control
- [x] `ICDCBackpressureSignal` for consumer-driven flow control
- [x] `ICDCFanIn` for multi-source stream merging
- [x] `ICDCEventSchema` for event schema validation
- [x] `IDeliveryGuaranteeConfig` for at-least/at-most/exactly-once contracts
- [x] `IDeliveryTracker` for per-event ack tracking
- [x] `DebeziumFormatter` and `IKafkaCDCProducer`
- [x] `IOutboxProcessor` for transactional outbox
- [x] `ISchemaRegistry` for schema versioning
- [x] `IDeadLetterQueue` for failed event routing
- [x] `ITenantBufferManager` for per-tenant isolation
- [x] `ICDCMaterializedView` for CDC-driven views
- [x] `ICDCWSHandler` and `IWSTransport` for WebSocket delivery

---

## Planned Features

- [ ] `ICDCReplayController` for time-based changefeed replay (Target: Q3 2026)
- [ ] `ICDCFilterPipeline` for server-side event filtering (Target: Q3 2026)
- [ ] `ICDCBatchCommitCoordinator` for exactly-once batch commits (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Core Changefeed
- [x] `IChangefeed`, `IChangefeedBuffer`, `IConsumerGroup`

### Phase 2: Delivery & Transport
- [x] `ICDCTransport`, `IDeliveryTracker`, `IDeliveryGuaranteeConfig`
- [x] Debezium, Kafka, Outbox headers

### Phase 3: Flow Control
- [x] `ICDCPauseControl`, `ICDCBackpressureSignal`, `ICDCFanIn`

### Phase 4: Schema & Reliability
- [x] `ICDCEventSchema`, `ISchemaRegistry`, `IDeadLetterQueue`

### Phase 5: Advanced Features
- [ ] `ICDCReplayController` (Q3 2026)
- [ ] `ICDCFilterPipeline` (Q3 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs present
- [ ] Doxygen fully annotated on all 24 headers

---

## Production Readiness Checklist

- [x] All v1.8.0 interfaces present and implemented
- [x] Dead letter queue and schema registry headers present
- [x] Tenant isolation via `ITenantBufferManager`
- [x] Backpressure and pause/resume interfaces stable
- [ ] Exactly-once batch commit coordinator header
- [ ] Doxygen fully annotated
