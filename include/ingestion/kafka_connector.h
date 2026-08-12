/**
 * @file kafka_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace themis {
namespace ingestion {

/**
 * @brief Kafka consumer source connector
 *
 * Consumes documents from one or more Kafka topics and ingests them into
 * ThemisDB.  Implemented using the librdkafka C API; when
 * `THEMIS_ENABLE_KAFKA` is not defined at compile time the connector
 * compiles but always returns `CONNECTOR_NOT_SUPPORTED`.
 *
 * Consumer group offsets are committed only after a message has been
 * durably written to ThemisDB, preserving at-least-once delivery semantics.
 *
 * Supported `SourceConfig::options` keys:
 * | Key                   | Description                                              | Default              |
 * |-----------------------|----------------------------------------------------------|----------------------|
 * | `topic`               | Kafka topic to subscribe to                              | `SourceConfig::location` |
 * | `consumer_group`      | Consumer group ID                                        | `themis-ingestion`   |
 * | `message_format`      | `"json"` or `"avro"`                                     | `json`               |
 * | `text_field`          | JSON key whose value is the document text                | `text`               |
 * | `schema_registry_url` | Avro Schema Registry URL (required when format is avro)  | (none)               |
 * | `poll_timeout_ms`     | Per-poll timeout in milliseconds                         | `1000`               |
 * | `max_messages`        | Maximum messages to consume per run (0 = unlimited)      | `0`                  |
 * | `session_timeout_ms`  | Consumer session timeout in milliseconds                 | `10000`              |
 * | `security_protocol`   | `"plaintext"`, `"ssl"`, `"sasl_plaintext"`, `"sasl_ssl"` | `plaintext`          |
 * | `sasl_mechanism`      | SASL mechanism when using SASL security protocol         | (none)               |
 * | `sasl_username`       | SASL username (never logged)                             | (none)               |
 * | `sasl_password`       | SASL password (never logged)                             | (none)               |
 * | `ssl_ca_location`     | Path to CA certificate bundle for TLS                    | (system default)     |
 * | `auto_offset_reset`   | Where to start when no committed offset exists: `"earliest"`, `"latest"`, or `"none"` | `earliest` |
 *
 * The broker list is taken from `SourceConfig::location`.
 *
 * Example usage (JSON format):
 * @code
 * SourceConfig cfg{
 *     .source_id = "events_topic",
 *     .type      = SourceType::KAFKA,
 *     .location  = "broker1:9092,broker2:9092",
 *     .options   = {{"topic","events"},
 *                   {"consumer_group","themis-ingest"},
 *                   {"text_field","payload"},
 *                   {"max_messages","1000"}}
 * };
 * KafkaConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("documents", nullptr);
 * @endcode
 *
 * Example usage via IngestionBuilder:
 * @code
 * auto mgr = IngestionBuilder("mydb")
 *     .withKafkaSource("events_topic",
 *                      "broker1:9092",
 *                      "events",
 *                      {{"consumer_group","themis-ingest"},{"max_messages","500"}})
 *     .build();
 * auto report = mgr->ingestAll();
 * @endcode
 */
class KafkaConnector : public ISourceConnector {
public:
    KafkaConnector();
    ~KafkaConnector() override;

    // Non-copyable
    KafkaConnector(const KafkaConnector&) = delete;
    KafkaConnector& operator=(const KafkaConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration.
     * @param config  Must have `type == SourceType::KAFKA`; `location` is the
     *                comma-separated broker list.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /**
     * @brief Check whether the Kafka broker(s) are reachable.
     *
     * Performs a lightweight metadata request.  Returns false when
     * `THEMIS_ENABLE_KAFKA` is not defined.
     */
    bool isAvailable() const override;

    /**
     * @brief Returns 0 – Kafka topics do not expose a reliable total count.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief Consume messages from the configured topic and ingest them.
     *
     * Polls the Kafka consumer until `max_messages` is reached or no new
     * messages arrive within `poll_timeout_ms`.  Each message payload is
     * extracted as a document and accumulated in the returned statistics.
     *
     * When `THEMIS_ENABLE_KAFKA` is not defined, returns immediately with a
     * `CONNECTOR_NOT_SUPPORTED` error.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Configure retry behaviour for this connector.
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Inject a checkpoint store so that ThemisDB checkpoints and
     *        Kafka consumer-group offsets are committed in the correct order.
     *
     * When set, `ingest()` writes the ThemisDB checkpoint via the store
     * **before** calling `rd_kafka_consumer_close()` (which commits Kafka
     * offsets).  This guarantees at-least-once delivery semantics:
     *
     * 1. ThemisDB checkpoint written → database knows documents were processed.
     * 2. Kafka offsets committed      → broker will not re-deliver messages.
     *
     * If the process crashes between steps 1 and 2, Kafka re-delivers the
     * messages and ThemisDB deduplicates them via the checkpoint.  The
     * `IngestionManager` automatically injects the shared store whenever
     * incremental mode is active.
     *
     * @param store  Shared checkpoint store; pass nullptr to disable.
     */
    void setCheckpointStore(std::shared_ptr<CheckpointStore> store);

    /**
     * @brief Function type for providing Kafka message batches.
     *
     * Each call to the function should return a batch of raw message payloads
     * (as strings).  Return an empty vector to signal "no more messages".
     * Injected via `setMessageFetchForTesting()`.
     */
    using KafkaMessageFn = std::function<std::vector<std::string>()>;

    /**
     * @brief Inject a Kafka message-batch provider.
     *
     * When set, every Kafka poll that would normally be performed via
     * librdkafka is replaced by a call to @p fn.  Pass an empty
     * `KafkaMessageFn{}` to restore the real librdkafka consumer.
     */
    void setMessageBatchProvider(KafkaMessageFn fn);
    void setMessageFetchForTesting(KafkaMessageFn fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
