# ThemisDB CDC Implementation: Bi-Temporal Event Delivery, Schema Evolution, and GDPR Redaction

**Status**: Review-ready
**Version**: 1.0
**Last Updated**: 2026-05-14
**Authors**: ThemisDB Research Team

---

## Abstract

This paper reviews the current ThemisDB implementation state for bi-temporal change data capture (CDC) and focuses on three engineering concerns: (1) reliable event transport under failures, (2) schema evolution compatibility for downstream consumers, and (3) GDPR-aligned redaction handling in append-oriented logs.

The current codebase provides production-oriented primitives for these concerns, but with clearly scoped limits:

- Transport paths provide at-least-once guarantees (`DeliveryTracker`, outbox relay semantics).
- Exactly-once behavior is supported as an application pattern via deduplication configuration (`IDeliveryGuaranteeConfig`) and idempotent downstream processing, not as a single global end-to-end guarantee.
- Schema evolution is exposed through explicit interfaces (`ICDCEventSchema`, schema registry integration).
- GDPR operations are implemented through `CDCAdmin::redactByKeyPrefix(...)` with optional audit-log and tombstone propagation wiring.

All claims in this document are tied to concrete source artifacts (headers, module roadmaps, benchmark expectation files, and tests) to keep the narrative reviewable and reproducible.

---

## Introduction

ThemisDB is positioned as a multi-model database with an AQL-based query layer and distributed operation modes ([README.md](../README.md), [ARCHITECTURE.md](../ARCHITECTURE.md)). In this context, CDC serves as an integration boundary between transactional writes and downstream systems (replication, streaming, analytics, audit).

For a bi-temporal setting, CDC events must preserve both system-time commit context and valid-time semantics where available (for example in temporal components such as `TemporalCDC`). At the same time, operational expectations in enterprise environments require:

1. robust delivery behavior under network and consumer failures,
2. schema evolution compatibility over long-lived streams,
3. compliance workflows such as GDPR erasure/redaction with traceability.

From a distributed-systems perspective, this aligns with established practice around ordered event processing and clock-aware consistency models [1, 7].

This review updates and restructures the previous draft into a publication-ready state and removes unsupported superlative claims.

---

## Methodology

### M1. Evidence model

We validate each central statement using at least one of the following artifact classes:

- public API headers in `include/cdc/` and `include/temporal/`,
- implementation files in `src/cdc/` and `src/temporal/`,
- module roadmap/status documents (`src/cdc/ROADMAP.md`),
- benchmark expectation documents (`src/cdc/PERFORMANCE_EXPECTATIONS.md`),
- dedicated CDC test targets listed in roadmap and tests.

### M2. Terminology normalization

To keep terminology consistent across ThemisDB documentation and this article:

- **ThemisDB**: multi-model database platform.
- **AQL**: query-layer language/component term; not used as a synonym for CDC.
- **Delivery semantics**: transport-level at-least-once vs. exactly-once-oriented processing.
- **Idempotent exactly-once-oriented processing**: achieved through deduplication and idempotent consumer contracts; not assumed as universal end-to-end transport semantics.
- **Schema evolution**: compatibility and migration signaling via CDC schema interfaces and registry tooling.

### M3. Claim validation policy

Claims are treated as valid only when they have direct linkage to code, module docs, benchmarks, or external literature in this manuscript.

---

## System Findings (Code-Backed)

### 1) Delivery and replay primitives

The CDC module documents implemented delivery and streaming capabilities, including SSE streaming, WebSocket transport, consumer groups, delivery tracker support, DLQ support, and replay interfaces ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).

Relevant concrete interfaces/components:

- `IDeliveryGuaranteeConfig` with delivery mode and deduplication-window configuration ([include/cdc/idelivery_guarantee_config.h](../include/cdc/idelivery_guarantee_config.h)).
- `DeliveryTracker` (at-least-once acknowledgement/redelivery flow in module roadmap and tests) ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
- `ICDCReplayController` and replay session model for bounded replay sessions ([include/cdc/icdc_replay_controller.h](../include/cdc/icdc_replay_controller.h)).

These components map to common CDC ecosystem expectations from Kafka and Debezium deployments [1, 2].

### 2) Schema evolution support

The codebase exposes explicit schema-evolution contracts:

- `ICDCEventSchema` and `SchemaEvolutionDescriptor` (migration strategies and callbacks) ([include/cdc/icdc_event_schema.h](../include/cdc/icdc_event_schema.h)).
- Schema registry integration (`SchemaRegistryClient`, `CdcSchemaEncoder`) for Confluent-compatible framing and schema ID handling ([include/cdc/schema_registry.h](../include/cdc/schema_registry.h)).

This supports versioned event compatibility workflows, while rollout policy (e.g., backward/forward compatibility governance) remains an operational responsibility.

### 3) GDPR redaction and auditability

`CDCAdmin` exposes redaction operations and optional dependency wiring for persistence/propagation:

- `setAuditStorage(...)`: writes structured redaction records (when wired).
- `setTransport(...)`: publishes delete tombstones (when wired).
- `redactByKeyPrefix(tenant_id, key_prefix, operator_id)`: redaction entry point.

See [include/cdc/cdc_admin.h](../include/cdc/cdc_admin.h) and [src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md).

### 4) DLQ and outbox operational pattern

- Dead-letter queue storage layout and replay lifecycle are defined in `DeadLetterQueue` (RocksDB key format `dlq:{20-digit-zero-padded-sequence}`) ([include/cdc/dead_letter_queue.h](../include/cdc/dead_letter_queue.h)).
- Transactional outbox (`OutboxWriter` / `OutboxRelay`) is documented in CDC roadmap/future-enhancement docs and implemented in module sources ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md), [src/cdc/FUTURE_ENHANCEMENTS.md](../src/cdc/FUTURE_ENHANCEMENTS.md)).

The patterns are consistent with established integration guidance for transactional outbox and dead-letter handling [4, 5].

### 5) Bi-temporal CDC linkage

`TemporalCDC` provides event structures including transaction time and valid-time fields (`valid_from`, `valid_to`), plus replay APIs ([include/temporal/temporal_cdc.h](../include/temporal/temporal_cdc.h), [src/temporal/temporal_cdc.cpp](../src/temporal/temporal_cdc.cpp)).

---

## Evaluation

### E1. Artifact-backed evaluation scope

This revision evaluates implementation readiness against documented module gates and test artifacts.

### E2. Verification matrix

| Claim area | Verified artifact(s) | Result |
|---|---|---|
| CDC transport/replay components are implemented | `src/cdc/ROADMAP.md`, CDC headers in `include/cdc/` | Supported |
| Exactly-once is scoped and configuration-based, not global magic | `include/cdc/idelivery_guarantee_config.h`, roadmap delivery notes | Supported (scoped wording required) |
| Schema evolution hooks exist in public API | `include/cdc/icdc_event_schema.h`, `include/cdc/schema_registry.h` | Supported |
| GDPR redaction flow + optional audit/tombstone wiring exist | `include/cdc/cdc_admin.h`, `src/cdc/ROADMAP.md` | Supported |
| DLQ key format and replay lifecycle are concretely defined | `include/cdc/dead_letter_queue.h` | Supported |
| CDC performance targets are documented as module-specific gates | `src/cdc/PERFORMANCE_EXPECTATIONS.md` | Supported |

### E3. Benchmark interpretation

For CDC, documented expectations emphasize regression gates and module thresholds (e.g., throughput and latency envelopes) rather than universally claimed absolute results in this paper ([src/cdc/PERFORMANCE_EXPECTATIONS.md](../src/cdc/PERFORMANCE_EXPECTATIONS.md)).

---

## Limitations

The current implementation remains production-oriented but not without operational limits:

1. **In-memory in-flight state**: parts of at-least-once and outbox relay runtime state are in-memory and can replay after restart (at-least-once behavior) ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
2. **Retention automation gap**: runtime retention automation remains limited; manual/admin flows are still relevant in practice ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
3. **Tenant purge path constraints**: roadmap notes a tenant-purge limitation tied to current build/linking context ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
4. **Exactly-once boundary**: dedup-window approaches have finite memory windows and require downstream idempotency discipline.
5. **Compliance operations**: GDPR redaction duties impose requirements for audit retention, downstream tombstone propagation, and tenant-scoped traceability; these constraints go beyond pure transport mechanics [3].
---

## Discussion and Conclusion

ThemisDB provides a substantial CDC foundation for bi-temporal and enterprise integration scenarios, including delivery tracking, replay control, schema evolution hooks, DLQ/outbox patterns, and GDPR redaction controls. The strongest evidence-supported wording is:

- transport is robust and at-least-once capable,
- exactly-once behavior is achievable as a scoped end-to-end design pattern,
- schema evolution and compliance hooks are implemented but require disciplined operational rollout.

This revised article is structured for review and avoids unsupported absolute claims.

---

## Source Artifacts (ThemisDB)

- README and architecture context: [README.md](../README.md), [ARCHITECTURE.md](../ARCHITECTURE.md)
- CDC module status and limits: [src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)
- CDC performance gates: [src/cdc/PERFORMANCE_EXPECTATIONS.md](../src/cdc/PERFORMANCE_EXPECTATIONS.md)
- CDC public APIs:
  - [include/cdc/cdc_admin.h](../include/cdc/cdc_admin.h)
  - [include/cdc/dead_letter_queue.h](../include/cdc/dead_letter_queue.h)
  - [include/cdc/idelivery_guarantee_config.h](../include/cdc/idelivery_guarantee_config.h)
  - [include/cdc/icdc_event_schema.h](../include/cdc/icdc_event_schema.h)
  - [include/cdc/schema_registry.h](../include/cdc/schema_registry.h)
- Temporal CDC linkage: [include/temporal/temporal_cdc.h](../include/temporal/temporal_cdc.h), [src/temporal/temporal_cdc.cpp](../src/temporal/temporal_cdc.cpp)

## References

1. Apache Kafka documentation (delivery semantics): https://kafka.apache.org/documentation/#semantics (Accessed: 2026-05-14)
2. Debezium documentation: https://debezium.io/documentation/reference/stable/ (Accessed: 2026-05-14)
3. European GDPR legal text (EUR-Lex): https://eur-lex.europa.eu/eli/reg/2016/679/oj (Accessed: 2026-05-14)
4. Transactional Outbox pattern: https://microservices.io/patterns/data/transactional-outbox.html (Accessed: 2026-05-14)
5. Dead Letter Channel pattern: https://www.enterpriseintegrationpatterns.com/patterns/messaging/DeadLetterChannel.html (Accessed: 2026-05-14)
6. Kreps, J. Exactly-Once Semantics Are Possible: https://www.confluent.io/blog/simplified-robust-exactly-one-semantics-in-kafka-2-5/ (Accessed: 2026-05-14)
7. Kulkarni et al., Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases: https://arxiv.org/abs/1407.4765 (Accessed: 2026-05-14)
8. Jensen, C. S., Snodgrass, R. T. Temporal Data Management. IEEE TKDE (1999), DOI: https://doi.org/10.1109/69.755613
