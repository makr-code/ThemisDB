### Context

This issue implements the roadmap item 'Apache Kafka Consumer Importer' for the importers domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Apache Kafka Consumer Importer

### Goal

Deliver the scoped changes for Apache Kafka Consumer Importer in src/importers/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Apache Kafka Consumer Importer
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented (`src/importers/kafka_importer.cpp`, `include/importers/kafka_importer.h`)

Consumes messages from Apache Kafka topics and imports them as ThemisDB entities.
Enables real-time data intake from event-driven systems without polling REST APIs.

**Implementation Notes:**
- `kafka_importer.cpp` implements `IImporter`; uses the librdkafka C API for consumer group management, gated on the `THEMIS_ENABLE_KAFKA` compile-time flag.  When the flag is absent the importer compiles but returns an error at runtime, so builds without Kafka support are unaffected.
- Consumer group ID is configurable via JSON `"consumer_group"` key; offset commit occurs on `rd_kafka_consumer_close()` after all messages have been processed, preserving at-least-once delivery semantics.
- `KafkaImporter::parseKafkaUrl()` accepts `kafka://broker:9092/topic` URLs or bare topic names (broker list provided via `initialize()` config JSON).
- Supports JSON, Avro (Confluent wire format: magic byte + 4-byte schema ID stripped), and plaintext message formats.
- Security: SASL/SSL options are supported; credentials (`sasl_password`) are never written to log messages, error strings, or observability output.
- Plugin descriptor: `plugins/importers/kafka/plugin.json`.
- Unit tests: `tests/test_kafka_importer.cpp` (37 test cases, no live broker required; uses mock injection via `setMessageFetchForTesting()`).

**Performance Targets:**
- Consume throughput ≥ 100 000 small messages/sec from a local Kafka broker (single partition, JSON format, no TLS).
- Per-message JSON parse overhead ≤ 5 µs for messages up to 4 KB.
- Benchmarks to be added to `benchmarks/importers_bench.cpp` once the benchmarking harness covers streaming connectors.

---

### Acceptance Criteria

- [ ] `kafka_importer.cpp` implements `IImporter`; uses the librdkafka C API for consumer group management, gated on the `THEMIS_ENABLE_KAFKA` compile-time flag. When the flag is absent the importer compiles but returns an error at runtime, so builds without Kafka support are unaffected.
- [ ] Consumer group ID is configurable via JSON `"consumer_group"` key; offset commit occurs on `rd_kafka_consumer_close()` after all messages have been processed, preserving at-least-once delivery semantics.
- [ ] `KafkaImporter::parseKafkaUrl()` accepts `kafka://broker:9092/topic` URLs or bare topic names (broker list provided via `initialize()` config JSON).
- [ ] Supports JSON, Avro (Confluent wire format: magic byte + 4-byte schema ID stripped), and plaintext message formats.
- [ ] Security: SASL/SSL options are supported; credentials (`sasl_password`) are never written to log messages, error strings, or observability output.
- [ ] Plugin descriptor: `plugins/importers/kafka/plugin.json`.
- [ ] Unit tests: `tests/test_kafka_importer.cpp` (37 test cases, no live broker required; uses mock injection via `setMessageFetchForTesting()`).
- [ ] Consume throughput ≥ 100 000 small messages/sec from a local Kafka broker (single partition, JSON format, no TLS).
- [ ] Per-message JSON parse overhead ≤ 5 µs for messages up to 4 KB.
- [ ] Benchmarks to be added to `benchmarks/importers_bench.cpp` once the benchmarking harness covers streaming connectors.

### Relationships

- Roadmap row: #70 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/importers/FUTURE_ENHANCEMENTS.md#apache-kafka-consumer-importer
- Source key: roadmap:70:importers:v1.7.0:apache-kafka-consumer-importer

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:70:importers:v1.7.0:apache-kafka-consumer-importer -->
<!-- roadmap-ref: row=70;module=importers;target=v1.7.0 -->
<!-- roadmap-detail: src/importers/FUTURE_ENHANCEMENTS.md#apache-kafka-consumer-importer -->
