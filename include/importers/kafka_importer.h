/**
 * @file kafka_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: kafka_importer.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 96/100 | Lines: 323
 * Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4185 feat(importers/kafka): AC7 ... (2026-03-13) | #3135 feat(importers): Kafka cons... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace importers {

/**
 * @brief Kafka Consumer Importer for real-time streaming ingestion.
 *
 * Consumes messages from a Kafka topic and imports them as ThemisDB entities.
 * Implemented using the librdkafka C API; when `THEMIS_ENABLE_KAFKA` is not
 * defined at compile time the importer still compiles but `importData()` always
 * returns immediately with an `UNKNOWN` error describing the missing build flag.
 *
 * ### Source path format
 *
 * | Format                                | Description                        |
 * |---------------------------------------|------------------------------------|
 * | `kafka://broker:9092/topic`           | Single broker, single topic        |
 * | `kafka://b1:9092,b2:9092/topic`       | Multiple brokers, single topic     |
 *
 * The importer URL is parsed so that everything between `kafka://` and the
 * last `/` is treated as the broker list, and everything after the last `/`
 * is the topic name.  A plain topic name (no `kafka://` prefix) may also be
 * supplied when the broker list is configured via `initialize()`.
 *
 * ### Configuration JSON (passed to initialize())
 *
 * @code{.json}
 * {
 *   "brokers":              "localhost:9092",
 *   "consumer_group":       "themis-import",
 *   "message_format":       "json",
 *   "text_field":           "text",
 *   "poll_timeout_ms":      1000,
 *   "max_messages":         0,
 *   "session_timeout_ms":   10000,
 *   "security_protocol":    "plaintext",
 *   "sasl_mechanism":       "",
 *   "sasl_username":        "",
 *   "sasl_password":        "",
 *   "ssl_ca_location":      "",
 *   "auto_offset_reset":    "earliest"
 * }
 * @endcode
 *
 * ### Message formats
 *
 * | `message_format` | Behaviour                                                       |
 * |------------------|-----------------------------------------------------------------|
 * | `json`  (default)| Payload is JSON; the entity IS the parsed JSON object           |
 * | `avro`           | Confluent wire format; magic byte + 4-byte schema ID stripped;  |
 * |                  | remaining bytes treated as a JSON string in the `content` field |
 * | `plaintext`      | Raw bytes wrapped as `{"content": "<payload>"}`                 |
 *
 * ### Security
 * - Credentials (`sasl_username`, `sasl_password`) are never written to log
 *   messages, error strings, or checkpoint files.
 * - The sanitised connection identifier used in observability output is
 *   `kafka://<broker-list>/<topic>` (no credentials).
 *
 * ### Example (fluent build + streaming callback)
 * @code
 * KafkaImporter imp;
 * imp.initialize(R"({"consumer_group":"my-group","max_messages":1000})");
 *
 * ImportOptions opts;
 * opts.streaming_row_callback = [](const std::string& table,
 *                                   const nlohmann::json& entity) -> bool {
 *     store(entity);
 *     return true;  // continue
 * };
 *
 * auto stats = imp.importData("kafka://broker:9092/events", opts);
 * @endcode
 */
class KafkaImporter : public IImporter {
public:
    KafkaImporter();
    ~KafkaImporter() override;

    // Non-copyable
    KafkaImporter(const KafkaImporter&) = delete;
    KafkaImporter& operator=(const KafkaImporter&) = delete;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    const char* getName() const override { return "Kafka Importer"; }

    /// Returns {"kafka", "kafka-json", "kafka-avro", "kafka-plaintext"}.
    std::vector<std::string> getSupportedTypes() const override;

    /**
     * @brief Initialize with a JSON configuration string.
     *
     * Accepted keys are listed in the class documentation.  All keys are
     * optional; sensible defaults are applied for missing entries.
     *
     * @param config  JSON object string.  An empty string or `"{}"` uses defaults.
     * @return true on success (always true unless JSON parse fails).
     */
    bool initialize(const std::string& config) override;

    /**
     * @brief Validate the source URL and check broker connectivity.
     *
     * Checks that @p source_path is a valid `kafka://brokers/topic` URL and,
     * when `THEMIS_ENABLE_KAFKA` is defined, performs a lightweight metadata
     * request to verify the broker is reachable.
     *
     * @param source_path  Kafka URL (see class documentation for formats).
     * @param errors       Appended with human-readable error descriptions.
     * @return true if the source is valid and reachable.
     */
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    /**
     * @brief Consume messages from a Kafka topic and import them as entities.
     *
     * Polls the Kafka consumer until `max_messages` is reached or no new
     * messages arrive within `poll_timeout_ms`.  Each message payload is
     * converted to a JSON entity according to `message_format` and delivered
     * via `options.streaming_row_callback` (if set), then counted in the
     * returned `ImportStats`.
     *
     * In dry-run mode (`options.dry_run == true`) messages are consumed and
     * parsed but the streaming callback is NOT invoked and
     * `stats.imported_records` is left at zero; `stats.total_records` still
     * reflects the number of messages consumed.
     *
     * @param source_path        `kafka://brokers/topic` URL.
     * @param options            Import options.
     * @param progress_callback  Optional stage/current/total progress report.
     * @return Accumulated import statistics.
     */
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) override;

    /**
     * @brief Launch importData() on a background thread.
     *
     * Returns immediately with a shared handle; poll `handle->getStatus()` or
     * `handle->future.get()` for the final `ImportStats`.
     */
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) override;

    /// Set the cancelled_ flag; the running import loop will exit on the next
    /// poll iteration.
    void cancel() override;

    /**
     * @brief Return an empty schema (Kafka topics have no fixed schema).
     *
     * Returns `{"type": "kafka", "topic": "<topic>", "schema": null}`.
     */
    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // Testing hook
    // -------------------------------------------------------------------------

    /**
     * @brief Function type for injecting mock Kafka messages in unit tests.
     *
     * Each invocation should return a batch of raw message payloads.  Return
     * an empty vector to signal end-of-stream.
     */
    using KafkaMessageFn = std::function<std::vector<std::string>()>;

    /**
     * @brief Inject a mock message-fetch function (unit-testing only).
     *
     * When set, every librdkafka poll is replaced by a call to @p fn.  Pass
     * an empty `KafkaMessageFn{}` to restore the real consumer.
     */
    void setMessageFetchForTesting(KafkaMessageFn fn);

    // -------------------------------------------------------------------------
    // URL parsing helper (public for testability)
    // -------------------------------------------------------------------------

    /**
     * @brief Parse a Kafka URL into broker list and topic name.
     *
     * Accepts:
     *   - `kafka://broker:9092/topic`
     *   - `kafka://b1:9092,b2:9092/topic`
     *   - `topic`  (bare topic; brokers must be set via initialize())
     *
     * @param url      Input URL.
     * @param brokers  Output: comma-separated broker list.
     * @param topic    Output: topic name.
     * @return true if parsing succeeded (non-empty topic extracted).
     */
    static bool parseKafkaUrl(const std::string& url,
                               std::string& brokers,
                               std::string& topic);

private:
    // Parsed from initialize() JSON config
    std::string default_brokers_;
    std::string consumer_group_  = "themis-import";
    std::string message_format_  = "json";
    std::string text_field_      = "text";
    int         poll_timeout_ms_ = 1000;
    size_t      max_messages_    = 0;
    int         session_timeout_ms_ = 10000;
    std::string security_protocol_  = "plaintext";
    std::string sasl_mechanism_;
    std::string sasl_username_;
    std::string sasl_password_;
    std::string ssl_ca_location_;
    std::string auto_offset_reset_  = "earliest";

    // PHASE-2-HARDENING: Bounded buffer configuration
    size_t      max_buffer_messages_ = 1000;  ///< Max messages in buffer before pausing
    size_t      buffer_drain_threshold_ = 500; ///< Resume when buffer drains below this

    std::atomic<bool> cancelled_{false};

    // Testing hook
    KafkaMessageFn message_fn_;

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Convert a raw message payload to a JSON entity.
     *
     * @param payload  Raw message bytes.
     * @return JSON object (or null if payload is empty).
     */
    json extractEntity(const std::string& payload) const;

    /** Run the consume loop against the mock message function. */
    void consumeFromMock(const std::string& topic,
                         const ImportOptions& options,
                         ImportStats& stats,
                         ProgressCallback& progress_cb);

#ifdef THEMIS_ENABLE_KAFKA
    /** Run the consume loop against a live Kafka broker via librdkafka. */
    void consumeFromKafka(const std::string& brokers,
                          const std::string& topic,
                          const ImportOptions& options,
                          ImportStats& stats,
                          ProgressCallback& progress_cb);
#endif

    void addError(ImportStats& stats,
                  ImportErrorCode code,
                  ImportErrorSeverity severity,
                  const std::string& message,
                  const std::string& location = "") const;

    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;

    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;

    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current,
                        size_t total);
};

// ============================================================================
// Plugin wrapper
// ============================================================================

/**
 * @brief Kafka Importer Plugin
 *
 * Wraps KafkaImporter as a ThemisDB plugin for runtime discovery and loading.
 */
class KafkaImporterPlugin : public plugins::IThemisPlugin {
public:
    KafkaImporterPlugin();
    ~KafkaImporterPlugin() override = default;

    const char* getName() const override { return "kafka_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override {
        return plugins::PluginType::IMPORTER;
    }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<KafkaImporter> importer_;
};

} // namespace importers
} // namespace themis

