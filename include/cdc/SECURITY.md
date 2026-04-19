<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — CDC Module Public Headers

**Module Path:** `include/cdc/`
**Implementation Security:** `../../src/cdc/SECURITY.md`

---

## Scope

Security considerations for the CDC module's public header API surface. Covers event
stream authentication, tenant isolation, event schema validation, and protection against
event injection and replay attacks.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Cross-tenant event leakage | Shared changefeed without tenant scoping | `tenant_buffer_manager.h` — per-tenant buffer lifecycle |
| Unauthenticated event consumption | Consumer connecting without credentials | `icdc_transport.h` — transport contract requires auth context |
| Event schema injection | Malformed or unexpected event fields | `icdc_event_schema.h` — schema validation before delivery |
| Kafka message tampering | Unsigned Kafka CDC events | `kafka_cdc_producer.h` — HMAC signing recommended; see `../../src/cdc/SECURITY.md` |
| Dead letter queue data exposure | DLQ events containing PII | `dead_letter_queue.h` — DLQ access requires `admin:cdc:read` capability |
| Backpressure denial of service | Consumer sends false backpressure | `icdc_backpressure_signal.h` — signals validated against consumer identity |
| Schema version downgrade | Attacker forcing old schema version | `schema_registry.h` — monotonic version enforcement; downgrades rejected |
| WebSocket event stream hijacking | Missing auth on WS CDC endpoint | `cdc_ws_handler.h` — auth token required in connection handshake |
| Changefeed replay abuse | Unauthorised replay of historical events | `IChangefeed` — replay requires `cdc:replay` capability |

---

## Security Controls

### Tenant Isolation
`ITenantBufferManager` ensures each tenant has an independent, bounded change event buffer.
Cross-tenant events are architecturally prevented by tenant-keyed buffer allocation.

### Schema Validation
`ICDCEventSchema::validate(event)` is called on all events before delivery; invalid events
are routed to the dead letter queue, not silently dropped.

### Transport Authentication
`ICDCTransport` requires an auth context; unauthenticated transports are rejected at
construction time.

### Dead Letter Queue Access Control
`IDeadLetterQueue` operations require the `admin:cdc:read` capability; DLQ contents are
not accessible to regular consumers.

### Backpressure Signal Integrity
`ICDCBackpressureSignal` signals are bound to the consumer's identity; a consumer may only
signal backpressure for its own group.

---

## Known Limitations

- Kafka message signing is recommended but not enforced at the header contract level;
  operators must configure Kafka TLS and optionally HMAC signing.
- WebSocket CDC transport (`cdc_ws_handler.h`) does not enforce end-to-end encryption
  beyond TLS at the connection level; in-cluster traffic is assumed trusted.
- Exactly-once delivery (`DeliveryGuarantee::EXACTLY_ONCE`) requires idempotent consumers;
  the header contract declares the guarantee but does not implement consumer-side deduplication.
