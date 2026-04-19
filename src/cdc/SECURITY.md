> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — CDC (Change Data Capture) Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The CDC module streams database change events to subscribers via SSE, WebSocket, and Kafka. Security concerns include: preventing unauthorized access to change streams, GDPR-compliant PII redaction in change logs, secure delivery guarantees, tenant isolation of change feeds, and safe Kafka integration.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthorized access to change streams | All SSE and WebSocket endpoints enforce authentication middleware; subscription requires valid tenant identity |
| Cross-tenant change event leakage | Tenant buffer manager isolates per-tenant change queues; cross-collection streams are tenant-scoped |
| PII exposure in change log | GDPR-aware redaction masks fields annotated with `encryption`/`pii` in `before_snapshot`; `after_snapshot` subject to same masking rules |
| Replay of stale SSE events | `consumer_id` + `ack_timeout_ms` parameters enable at-least-once delivery with consumer-controlled acknowledgement |
| Kafka message injection | `KafkaCDCProducer` uses configured broker credentials; topic names are sanitized before publishing |
| Dead-letter queue overflow | RocksDB-backed DLQ with `drain()` API; configurable max size prevents disk exhaustion |
| Subscription denial-of-service | Per-subscription filter evaluation is O(1) per event; subscription count can be limited per tenant |
| Change stream replay from unauthorized offset | `replayFrom()` validates replay offset against tenant's accessible change log range |
| Debezium envelope tampering | Debezium format is generated server-side; `toJsonWithSchema()` output is treated as untrusted by consumers |

## Security Controls

### Authentication and Tenant Isolation
- All SSE (`GET /changefeed/stream`) and WebSocket (`/v2/cdc/stream`) endpoints require valid authentication tokens.
- `TenantBufferManager` provides per-tenant change event isolation; subscribers only receive events from their own collections.
- `ConsumerGroupManager` offset tracking is per-group within a tenant scope.

### GDPR PII Redaction
- Fields annotated with `encryption` or `pii` schema metadata are masked in `ChangeEvent::before_snapshot` before delivery to SSE/WebSocket consumers.
- `after_snapshot` is subject to the same masking rules.
- Masking is applied at the changefeed layer before any transport serialization.

### Delivery Guarantees
- At-least-once delivery: `DeliveryTracker` tracks unacknowledged events; `getPendingRedelivery()` surfaces events requiring redelivery.
- Dead-letter queue stores events that fail repeated delivery; accessible only via `admin:cdc:write` scoped admin endpoints.
- SSE acknowledgement endpoint (`POST /changefeed/stream/ack`) validates `consumer_id` ownership before marking events as delivered.

### Kafka Integration
- `KafkaCDCProducer` is opt-in (`THEMIS_ENABLE_KAFKA`); not loaded unless explicitly configured.
- Broker connection credentials are injected via configuration; not logged.
- Topic names are validated against an allowlist to prevent topic injection.

## Data Handling

- Change log entries contain full document `before`/`after` snapshots which may include sensitive data; PII fields are redacted before streaming but stored in the raw change log.
- Raw change log storage (RocksDB-backed) should be encrypted at rest at the storage layer — not managed by this module.
- DLQ entries in RocksDB contain failed event payloads including (possibly redacted) document snapshots.
- Kafka messages contain Debezium-envelope-formatted change events; PII redaction is applied before publishing.
- Change log retention is configurable via TTL and size-based policies (`CDCAdmin::purgeOlderThan()`).

## Known Limitations

- Raw change log (before PII redaction) is stored in RocksDB; delayed redaction means raw snapshots may persist until TTL expiry.
- Kafka producer TLS and SASL authentication configuration is operator-managed.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| RocksDB | Change log, DLQ persistence | Encrypt at rest at OS/filesystem layer |
| librdkafka (optional) | Kafka CDC producer | Enabled by `THEMIS_ENABLE_KAFKA`; TLS/SASL operator-configured |
| zstd | Change stream compression | Bounds-checked decompression |
