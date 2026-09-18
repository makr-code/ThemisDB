/**
 * @file kafka_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    std::vector<std::string> getSupportedTypes() const override;

    bool initialize(const std::string& config) override;

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) override;

    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) override;

    void cancel() override;

    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // Testing hook
    // -------------------------------------------------------------------------

    using KafkaMessageFn = std::function<std::vector<std::string>()>;

    /**
     * @brief Set Message Fetch For Testing.
     * @param[in] fn Input parameter.
     */
    void setMessageFetchForTesting(KafkaMessageFn fn);

    /**
     * @brief ------------------------------------------------------------------------- URL parsing helper (public for testability) -------------------------------------------------------------------------
     * @param[in] url Input parameter.
     * @param[in,out] brokers Input/output parameter.
     * @param[in,out] topic Input/output parameter.
     * @return True when the operation succeeds.
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
     * @brief Extract Entity.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    json extractEntity(const std::string& payload) const;

    /**
     * @brief Consume From Mock.
     * @param[in] topic Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] progress_cb Input/output parameter.
     */
    void consumeFromMock(const std::string& topic,
                         const ImportOptions& options,
                         ImportStats& stats,
                         ProgressCallback& progress_cb);

#ifdef THEMIS_ENABLE_KAFKA
    /**
     * @brief Consume From Kafka.
     * @param[in] brokers Input parameter.
     * @param[in] topic Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] progress_cb Input/output parameter.
     */
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

    /**
     * @brief Report Progress.
     * @param[in,out] callback Input/output parameter.
     * @param[in] stage Input parameter.
     * @param[in] current Input parameter.
     * @param[in] total Input parameter.
     */
    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current,
                        size_t total);
};

// ============================================================================
// Plugin wrapper
// ============================================================================

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

