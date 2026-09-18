#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <map>

namespace themis {
namespace importers {

/**
 * @brief Debezium Change Data Capture (CDC) Importer
 *
 * Continuously ingests database change events produced by a Debezium connector
 * into ThemisDB.  Debezium serialises change events as JSON (or Avro) messages
 * on a Kafka topic; this importer consumes those messages and applies them as
 * ThemisDB upsert/delete operations.
 *
 * Supported source databases via Debezium connectors:
 * - PostgreSQL (wal2json / pgoutput)
 * - MySQL / MariaDB (binlog)
 * - SQL Server (CDC tables)
 * - MongoDB (change streams)
 *
 * Supported capabilities:
 * - Full-snapshot replay followed by incremental CDC streaming
 * - At-least-once delivery with idempotent upsert (document fingerprinting)
 * - Configurable consumer group and initial offset (earliest / latest / stored)
 * - Schema registry integration (Confluent Schema Registry or Apicurio)
 * - Field-level encryption metadata passthrough
 * - Dead-letter routing for unparseable or schema-rejected events
 * - Structured error reporting via ImportErrorCode
 * - Observability: progress callback, metrics, trace spans
 * - Permission-check callback for ACL enforcement
 * - Async streaming mode via importDataAsync() (runs until cancel())
 * - Credential redaction: broker passwords/SASL secrets never logged
 *
 * Build guard: define @c THEMIS_ENABLE_DEBEZIUM to compile the full
 * librdkafka-backed implementation.  Without it every @c importData() call
 * returns @c ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE with a message
 * describing the missing build flag.
 *
 * @note Thread-safety: DebeziumCDCImporter instances are not thread-safe.
 *       Use importDataAsync() for background streaming.
 *
 * Configuration JSON keys (passed to initialize()):
 * @code{.json}
 * {
 *   "brokers":         "localhost:9092",      // Kafka bootstrap servers
 *   "topic_prefix":    "debezium.mydb",       // Debezium topic prefix
 *   "tables":          ["public.orders"],     // optional table filter (empty = all)
 *   "consumer_group":  "themisdb-cdc",
 *   "auto_offset":     "latest",              // "earliest" | "latest" | "stored"
 *   "schema_registry": "http://localhost:8081", // optional
 *   "sasl_username":   "...",
 *   "sasl_password":   "...",                 // never logged
 *   "tls":             false,
 *   "max_batch_size":  500,
 *   "poll_timeout_ms": 100,
 *   "snapshot_mode":   "initial",             // "initial" | "never" | "always"
 *   "dead_letter_topic": "themisdb.dlq"       // optional
 * }
 * @endcode
 *
 * Debezium event payload contract (envelope schema v2):
 * @code{.json}
 * {
 *   "op":     "c|u|d|r",   // create / update / delete / read (snapshot)
 *   "before": { ... },      // pre-image (null for INSERT / snapshot)
 *   "after":  { ... },      // post-image (null for DELETE)
 *   "source": { "table": "orders", "ts_ms": 1234567890 },
 *   "ts_ms":  1234567890
 * }
 * @endcode
 *
 * References:
 *   Debezium Documentation – https://debezium.io/documentation/
 *   Flink, Pulsar, Kafka Streams: Event-driven integration patterns.
 *   Kleppmann, M. (2017). "Designing Data-Intensive Applications." O'Reilly.
 */
class DebeziumCDCImporter : public IImporter {
public:
    DebeziumCDCImporter();
    ~DebeziumCDCImporter() override;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    /** @brief Returns "Debezium CDC Importer". */
    const char* getName() const override { return "Debezium CDC Importer"; }

    /**
     * @brief Returns supported source type identifiers.
     * @return {"debezium", "debezium-cdc", "cdc"}
     */
    std::vector<std::string> getSupportedTypes() const override;

    /**
     * @brief Initializes the importer with a JSON configuration string.
     *
     * Parses broker addresses, consumer group, topic prefix, and SASL
     * credentials.  Credentials are never stored in plaintext in log output.
     *
     * @param config  JSON configuration (see class-level documentation).
     * @return true on success; false if required fields are missing or invalid.
     */
    bool initialize(const std::string& config) override;

    /**
     * @brief Validates connectivity to the Kafka cluster.
     *
     * Performs a metadata request and verifies that the expected Debezium
     * topic prefix exists.  On failure, appends diagnostics to @p errors
     * with broker addresses but never SASL credentials.
     *
     * @param source_path  Optional broker override (empty = use config).
     * @param errors       Output: list of validation error messages.
     * @return true if the cluster is reachable and the topic prefix is visible.
     */
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    /**
     * @brief Imports change events from the Debezium Kafka topic.
     *
     * If snapshot_mode is "initial" and no committed offsets exist for the
     * consumer group, performs a full snapshot pass (read "r" events) first,
     * then streams incremental changes ("c", "u", "d") until the deadline
     * is reached or cancel() is called.
     *
     * Each change event is decoded, field-filtered, and applied to ThemisDB:
     *   - "c" / "r" → upsert using the primary-key fields as document ID
     *   - "u"        → upsert (merge or overwrite per conflict strategy)
     *   - "d"        → delete by document ID
     *
     * @param source_path        Optional broker override.
     * @param options            Import options (batch size, conflict strategy,
     *                           table filters via include_tables, deadline_ms).
     * @param progress_callback  Optional callback invoked after each batch.
     * @return Import statistics.  For streaming mode this accumulates until
     *         cancel() is called or the deadline expires.
     */
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr) override;

    /**
     * @brief Starts an asynchronous, long-running CDC stream.
     *
     * The returned handle remains active until cancel() is called on the
     * importer or the deadline in @p options expires.  Progress counters in
     * the handle are updated atomically after each batch.
     *
     * @param source_path  Optional broker override.
     * @param options      Import options.
     * @return Shared handle to the running CDC stream.
     */
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;

    /** @brief Signals the running import/stream to stop after the current batch. */
    void cancel() override;

    /**
     * @brief Returns the inferred schema from Debezium topic metadata.
     *
     * Reads the Debezium schema envelope from the first available message on
     * each topic and converts it to a JSON schema description compatible with
     * ThemisDB's SchemaInferenceEngine output format.
     *
     * @param source_path  Optional broker override.
     * @return JSON schema description per table, or empty object on failure.
     */
    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // CDC-specific API
    // -------------------------------------------------------------------------

    /**
     * @brief Change event types produced by Debezium.
     */
    enum class ChangeOp { Read, Create, Update, Delete, Unknown };

    /**
     * @brief Decoded CDC change event (Debezium envelope v2 unpacked).
     */
    struct CDCEvent {
        ChangeOp op{ChangeOp::Unknown};   ///< Operation type
        std::string table;                 ///< Fully-qualified table name
        json before;                       ///< Pre-image; null for INSERT/READ
        json after;                        ///< Post-image; null for DELETE
        int64_t source_ts_ms{0};           ///< Source commit timestamp (epoch ms)
        std::string transaction_id;        ///< Optional transaction boundary ID
        uint64_t offset{0};               ///< Kafka partition offset
    };

    using CDCEventCallback = std::function<bool(const CDCEvent&)>;

    /**
     * @brief Streams CDC events via a caller-supplied callback.
     *
     * Alternative to importData(): decoded events are delivered directly to
     * the callback without ThemisDB write overhead.  Returning false from the
     * callback stops the stream.
     *
     * @param options   Import options (table filters, deadline_ms).
     * @param callback  Invoked for each decoded CDC event.
     * @return Statistics collected during the stream.
     */
    ImportStats streamEvents(const ImportOptions& options,
                             CDCEventCallback callback);

    // -------------------------------------------------------------------------
    // Testing support
    // -------------------------------------------------------------------------

    /**
     * @brief Injects a sequence of pre-decoded CDC events for unit testing.
     *
     * When set, importData() and streamEvents() consume this sequence instead
     * of polling Kafka.  The mock is consumed once and then cleared.
     *
     * @note Available in all build configurations.
     */
    void setMockEventsForTesting(std::vector<CDCEvent> events);

private:
    struct Config {
        std::string brokers{"localhost:9092"};
        std::string topic_prefix;
        std::vector<std::string> table_filter;
        std::string consumer_group{"themisdb-cdc"};
        std::string auto_offset{"latest"};
        std::string schema_registry_url;
        // SASL password is never stored after configuration; used only during connect.
        bool tls{false};
        int max_batch_size{500};
        int poll_timeout_ms{100};
        std::string snapshot_mode{"initial"};
        std::string dead_letter_topic;
    };

    /// Parses a Debezium envelope JSON into a CDCEvent.
    /// Returns CDCEvent with op=Unknown on parse failure; sets @p error_out.
    static CDCEvent parseDebeziumEnvelope(const json& envelope,
                                           std::string& error_out);

    /// Maps a Debezium op character ("c"/"u"/"d"/"r") to ChangeOp.
    static ChangeOp mapOpChar(const std::string& op_str);

    /// Sanitises broker string for log output (strips embedded credentials).
    static std::string sanitiseBrokers(const std::string& brokers);

    /// Returns true if the event's table passes the current table_filter.
    bool tableAllowed(const std::string& table) const;

    Config config_;
    std::atomic<bool> cancelled_{false};
    std::vector<CDCEvent> mock_events_;
};

} // namespace importers
} // namespace themis
