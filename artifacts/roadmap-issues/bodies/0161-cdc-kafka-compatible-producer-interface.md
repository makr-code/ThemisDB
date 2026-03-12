### Context

This issue implements the roadmap item 'Kafka-Compatible Producer Interface' for the cdc domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.9.0.

Primary detail section: Kafka-Compatible Producer Interface

### Goal

Deliver the scoped changes for Kafka-Compatible Producer Interface in src/cdc/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Kafka-Compatible Producer Interface
**Priority:** Medium
**Target Version:** v1.9.0

For enterprise deployments that use Kafka as a message bus, add a CDC-to-Kafka bridge that publishes `ChangeEvent` records to a configured Kafka topic. Implement using `librdkafka` to avoid a heavy JVM dependency.

**Implementation Notes:**
- `[x]` Create `kafka_cdc_producer.cpp`; implement `KafkaCDCProducer` class (`include/cdc/kafka_cdc_producer.h`, `src/cdc/kafka_cdc_producer.cpp`).
- `[x]` Define `ICDCTransport` abstract interface (`include/cdc/icdc_transport.h`); `KafkaCDCProducer` inherits from it enabling polymorphic transport use.
- `[x]` Topic routing: one topic per collection (e.g., `themis.cdc.orders`) or a single multiplexed topic; configurable via `config/data_management/cdc_kafka.yaml`.
- `[x]` Message key: `ChangeEvent::key`; message value: `ChangeEvent::toJson()` serialized to UTF-8 bytes.
- `[x]` Use `librdkafka` producer with `acks=all` and `enable.idempotence=true` for exactly-once semantics where broker supports it.
- `[x]` On `librdkafka` not found at build time, `kafka_cdc_producer.cpp` compiles as a no-op stub (same pattern as CUDA stubs in acceleration).
- `[x]` Expose `cdc_kafka_delivered_total`, `cdc_kafka_error_total` Prometheus counters.

**Performance Targets:**
- Kafka producer throughput ≥ 50,000 events/sec on a single producer thread (standard Kafka hardware).
- End-to-end latency (change committed → Kafka broker `ack`) < 10 ms p99 on LAN.

---

### Acceptance Criteria

- [ ] Create `kafka_cdc_producer.cpp`; implement `KafkaCDCProducer` class (`include/cdc/kafka_cdc_producer.h`, `src/cdc/kafka_cdc_producer.cpp`).
- [ ] Define `ICDCTransport` abstract interface (`include/cdc/icdc_transport.h`); `KafkaCDCProducer` inherits from it enabling polymorphic transport use.
- [ ] Topic routing: one topic per collection (e.g., `themis.cdc.orders`) or a single multiplexed topic; configurable via `config/data_management/cdc_kafka.yaml`.
- [ ] Message key: `ChangeEvent::key`; message value: `ChangeEvent::toJson()` serialized to UTF-8 bytes.
- [ ] Use `librdkafka` producer with `acks=all` and `enable.idempotence=true` for exactly-once semantics where broker supports it.
- [ ] On `librdkafka` not found at build time, `kafka_cdc_producer.cpp` compiles as a no-op stub (same pattern as CUDA stubs in acceleration).
- [ ] Expose `cdc_kafka_delivered_total`, `cdc_kafka_error_total` Prometheus counters.
- [ ] Kafka producer throughput ≥ 50,000 events/sec on a single producer thread (standard Kafka hardware).
- [ ] End-to-end latency (change committed → Kafka broker `ack`) < 10 ms p99 on LAN.

### Relationships

- Roadmap row: #161 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cdc/FUTURE_ENHANCEMENTS.md#kafka-compatible-producer-interface
- Source key: roadmap:161:cdc:v1.9.0:kafka-compatible-producer-interface

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:161:cdc:v1.9.0:kafka-compatible-producer-interface -->
<!-- roadmap-ref: row=161;module=cdc;target=v1.9.0 -->
<!-- roadmap-detail: src/cdc/FUTURE_ENHANCEMENTS.md#kafka-compatible-producer-interface -->
