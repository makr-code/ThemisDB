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

class DebeziumCDCImporter : public IImporter {
public:
    DebeziumCDCImporter();
    ~DebeziumCDCImporter() override;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    const char* getName() const override { return "Debezium CDC Importer"; }

    std::vector<std::string> getSupportedTypes() const override;

    bool initialize(const std::string& config) override;

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr) override;

    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;

    void cancel() override;

    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // CDC-specific API
    // -------------------------------------------------------------------------

    enum class ChangeOp { Read, Create, Update, Delete, Unknown };

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
     * @brief Stream Events.
     * @param[in] options Input parameter.
     * @param[in] callback Input parameter.
     * @return Return value.
     */
    ImportStats streamEvents(const ImportOptions& options,
                             CDCEventCallback callback);

    // -------------------------------------------------------------------------
    // Testing support
    // -------------------------------------------------------------------------

    /**
     * @brief Set Mock Events For Testing.
     * @param[in] events Input parameter.
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

    /**
     * @brief Parse Debezium Envelope.
     * @param[in] envelope Input parameter.
     * @param[in,out] error_out Input/output parameter.
     * @return Return value.
     */
    static CDCEvent parseDebeziumEnvelope(const json& envelope,
                                           std::string& error_out);

    /**
     * @brief Map Op Char.
     * @param[in] op_str Input parameter.
     * @return Return value.
     */
    static ChangeOp mapOpChar(const std::string& op_str);

    /**
     * @brief Sanitise Brokers.
     * @param[in] brokers Input parameter.
     * @return Return value.
     */
    static std::string sanitiseBrokers(const std::string& brokers);

    /**
     * @brief Table Allowed.
     * @param[in] table Input parameter.
     * @return True when the operation succeeds.
     */
    bool tableAllowed(const std::string& table) const;

    Config config_;
    std::atomic<bool> cancelled_{false};
    std::vector<CDCEvent> mock_events_;
};

} // namespace importers
} // namespace themis
