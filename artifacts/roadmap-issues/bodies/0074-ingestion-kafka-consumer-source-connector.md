### Context

This issue implements the roadmap item 'Kafka Consumer Source Connector' for the ingestion domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Kafka Consumer Source Connector

### Goal

Deliver the scoped changes for Kafka Consumer Source Connector in src/ingestion/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Kafka Consumer Source Connector
**Priority:** High
**Target Version:** v1.7.0

Add a `KafkaConnector` that consumes documents from one or more Kafka topics and ingests them into ThemisDB. This enables real-time data intake from event-driven systems without polling REST APIs.

**Implementation Notes:**
- Add `kafka_connector.cpp` implementing `ISourceConnector`; use `librdkafka` C API for consumer group management.
- Consumer group ID is configurable via `SourceConfig::options["consumer_group"]`; offset commit is tied to `IngestionCheckpointStore::commit()` so that ThemisDB-level checkpoints and Kafka offsets are kept in sync.
- Add `IngestionBuilder::withKafkaSource(source_id, brokers, topic, options, priority)` to the fluent API.
- Support both JSON-encoded and Avro-encoded messages; Avro requires a Schema Registry URL configured via `options["schema_registry_url"]`.
- Graceful shutdown: on `IngestionManager` stop, call `rd_kafka_consumer_close()` to commit final offsets before the process exits.

**Performance Targets:**
- Kafka consumer throughput ≥ 100 000 messages/sec (1 KB average message) with a single-partition topic.
- End-to-end latency from Kafka message publish to ThemisDB document available ≤ 500 ms p99.

---

### Acceptance Criteria

- [ ] Add `kafka_connector.cpp` implementing `ISourceConnector`; use `librdkafka` C API for consumer group management.
- [ ] Consumer group ID is configurable via `SourceConfig::options["consumer_group"]`; offset commit is tied to `IngestionCheckpointStore::commit()` so that ThemisDB-level checkpoints and Kafka offsets are kept in sync.
- [ ] Add `IngestionBuilder::withKafkaSource(source_id, brokers, topic, options, priority)` to the fluent API.
- [ ] Support both JSON-encoded and Avro-encoded messages; Avro requires a Schema Registry URL configured via `options["schema_registry_url"]`.
- [ ] Graceful shutdown: on `IngestionManager` stop, call `rd_kafka_consumer_close()` to commit final offsets before the process exits.
- [ ] Kafka consumer throughput ≥ 100 000 messages/sec (1 KB average message) with a single-partition topic.
- [ ] End-to-end latency from Kafka message publish to ThemisDB document available ≤ 500 ms p99.

### Relationships

- Roadmap row: #74 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/ingestion/FUTURE_ENHANCEMENTS.md#kafka-consumer-source-connector
- Source key: roadmap:74:ingestion:v1.7.0:kafka-consumer-source-connector

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:74:ingestion:v1.7.0:kafka-consumer-source-connector -->
<!-- roadmap-ref: row=74;module=ingestion;target=v1.7.0 -->
<!-- roadmap-detail: src/ingestion/FUTURE_ENHANCEMENTS.md#kafka-consumer-source-connector -->
