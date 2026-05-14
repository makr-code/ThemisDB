# Bi-Temporal CDC with At-Least-Once Transport, Schema Evolution Hooks, and Exactly-Once-Oriented Processing in ThemisDB

**Status**: Review-ready
**Version**: 1.0
**Last Updated**: 2026-05-14
**Authors**: ThemisDB Research Team

---

## Abstract / Zusammenfassung

This paper reviews the current ThemisDB implementation state for bi-temporal change data capture (CDC) and focuses on three engineering concerns: (1) reliable event transport under failures, (2) schema evolution compatibility for downstream consumers, and (3) GDPR-aligned redaction handling in append-oriented logs.

The current codebase provides production-oriented primitives for these concerns, but with clearly scoped limits. In particular, transport paths provide at-least-once guarantees (`DeliveryTracker`, outbox relay semantics), while exactly-once behavior is supported as an application pattern via deduplication configuration (`IDeliveryGuaranteeConfig`) and idempotent downstream processing, not as a single global end-to-end guarantee. Schema evolution is exposed through explicit interfaces (`ICDCEventSchema`, schema registry integration), and GDPR operations are implemented through `CDCAdmin::redactByKeyPrefix(...)` with optional audit-log and tombstone propagation wiring.

All claims in this document are tied to concrete source artifacts (headers, module roadmaps, benchmark expectation files, and tests) to keep the narrative reviewable and reproducible.

---

## Introduction / Einleitung

ThemisDB is positioned as a multi-model database with an AQL-based query layer and distributed operation modes ([README.md](../README.md), [ARCHITECTURE.md](../ARCHITECTURE.md)). In this context, CDC serves as an integration boundary between transactional writes and downstream systems (replication, streaming, analytics, audit).

For a bi-temporal setting, CDC events must preserve both system-time commit context and valid-time semantics where available (for example in temporal components such as `TemporalCDC`). At the same time, operational expectations in enterprise environments require:

1. robust delivery behavior under network and consumer failures,
2. schema evolution compatibility over long-lived streams,
3. compliance workflows such as GDPR erasure/redaction with traceability.

This review updates and restructures the previous draft into a publication-ready state and removes unsupported superlative claims.

---

## Methodik / Ansatz

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
- **Exactly-once (scoped)**: achieved through deduplication and idempotent processing contracts, not assumed as universal end-to-end property.
- **Schema evolution**: compatibility and migration signaling via CDC schema interfaces and registry tooling.

### M3. Exclusion rule for unverified claims

Claims without direct code/doc/benchmark linkage are removed or explicitly marked as hypotheses/future work.

---

## System Findings (Code-Backed)

### 1) Delivery and replay primitives

The CDC module documents implemented delivery and streaming capabilities, including SSE streaming, WebSocket transport, consumer groups, delivery tracker support, DLQ support, and replay interfaces ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).

Relevant concrete interfaces/components:

- `IDeliveryGuaranteeConfig` with delivery mode and deduplication-window configuration ([include/cdc/idelivery_guarantee_config.h](../include/cdc/idelivery_guarantee_config.h)).
- `DeliveryTracker` (at-least-once acknowledgement/redelivery flow in module roadmap and tests) ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
- `ICDCReplayController` and replay session model for bounded replay sessions ([include/cdc/icdc_replay_controller.h](../include/cdc/icdc_replay_controller.h)).

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

### 5) Bi-temporal CDC linkage

`TemporalCDC` provides event structures including transaction time and valid-time fields (`valid_from`, `valid_to`), plus replay APIs ([include/temporal/temporal_cdc.h](../include/temporal/temporal_cdc.h), [src/temporal/temporal_cdc.cpp](../src/temporal/temporal_cdc.cpp)).

---

## Evaluation / Experiments

### E1. Evidence-backed evaluation scope

This review does not introduce new benchmark runs. Instead, it evaluates implementation readiness against already documented module gates and test artifacts.

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

## Limitations / Known Issues

The current implementation remains production-oriented but not without operational limits:

1. **In-memory in-flight state**: parts of at-least-once and outbox relay runtime state are in-memory and can replay after restart (at-least-once behavior) ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
2. **Retention automation gap**: runtime retention automation remains limited; manual/admin flows are still relevant in practice ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
3. **Tenant purge path constraints**: roadmap notes a tenant-purge limitation tied to current build/linking context ([src/cdc/ROADMAP.md](../src/cdc/ROADMAP.md)).
4. **Exactly-once boundary**: dedup-window approaches have finite memory windows and require downstream idempotency discipline.
5. **No new experimental run in this revision**: this revision is a code/document evidence review, not a fresh benchmark campaign.

---

## Conclusion

ThemisDB provides a substantial CDC foundation for bi-temporal and enterprise integration scenarios, including delivery tracking, replay control, schema evolution hooks, DLQ/outbox patterns, and GDPR redaction controls. The strongest evidence-supported wording is:

- transport is robust and at-least-once capable,
- exactly-once behavior is achievable as a scoped end-to-end design pattern,
- schema evolution and compliance hooks are implemented but require disciplined operational rollout.

This revised article is structured for review and avoids unsupported absolute claims.

---

## References

1. ThemisDB README (multi-model architecture overview): https://github.com/makr-code/ThemisDB/blob/develop/README.md
2. ThemisDB ARCHITECTURE (AQL layer and distributed consistency context): https://github.com/makr-code/ThemisDB/blob/develop/ARCHITECTURE.md
3. ThemisDB CDC Roadmap: https://github.com/makr-code/ThemisDB/blob/develop/src/cdc/ROADMAP.md
4. ThemisDB CDC Performance Expectations: https://github.com/makr-code/ThemisDB/blob/develop/src/cdc/PERFORMANCE_EXPECTATIONS.md
5. ThemisDB `CDCAdmin` header: https://github.com/makr-code/ThemisDB/blob/develop/include/cdc/cdc_admin.h
6. ThemisDB `DeadLetterQueue` header: https://github.com/makr-code/ThemisDB/blob/develop/include/cdc/dead_letter_queue.h
7. ThemisDB `IDeliveryGuaranteeConfig` header: https://github.com/makr-code/ThemisDB/blob/develop/include/cdc/idelivery_guarantee_config.h
8. ThemisDB `ICDCEventSchema` header: https://github.com/makr-code/ThemisDB/blob/develop/include/cdc/icdc_event_schema.h
9. ThemisDB schema registry integration header: https://github.com/makr-code/ThemisDB/blob/develop/include/cdc/schema_registry.h
10. ThemisDB temporal CDC API: https://github.com/makr-code/ThemisDB/blob/develop/include/temporal/temporal_cdc.h
11. Apache Kafka documentation (delivery semantics): https://kafka.apache.org/documentation/#semantics
12. Debezium documentation: https://debezium.io/documentation/reference/stable/
13. European GDPR legal text (EUR-Lex): https://eur-lex.europa.eu/eli/reg/2016/679/oj
14. Kulkarni et al., Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases: https://arxiv.org/abs/1407.4765
