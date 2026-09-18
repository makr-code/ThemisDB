/**
 * @file postgres_cdc.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

struct CDCOptions {
    bool full_sync_first{true};          ///< Perform an initial snapshot?
    bool include_truncate{true};         ///< Stream TRUNCATE events?
    uint64_t polling_interval_ms{100};   ///< Reconnect back-off (ms)
    size_t buffer_size{10000};           ///< In-memory event buffer capacity
    std::string slot_name{"themisdb_cdc_slot"};
    std::string publication_name{"themisdb_publication"};
};

class PostgreSQLCDC {
public:
    // ------------------------------------------------------------------
    // Change event
    // ------------------------------------------------------------------
    struct ChangeEvent {
        enum class Operation { INSERT, UPDATE, DELETE, TRUNCATE } op;
        std::string table_name;
        json old_values;          ///< Pre-image (UPDATE / DELETE only)
        json new_values;          ///< Post-image (INSERT / UPDATE only)
        uint64_t lsn{0};          ///< Log Sequence Number
        std::string timestamp;    ///< RFC 3339 commit timestamp
        std::string replica_identity; ///< Which columns identify the row
    };

    using ChangeCallback = std::function<void(const ChangeEvent&)>;

    // ------------------------------------------------------------------
    // Logical Decoder
    // ------------------------------------------------------------------
    class LogicalDecoder {
    public:
        /**
         * @brief Logical Decoder.
         * @param[in] connection_string Input parameter.
         * @return Return value.
         */
        explicit LogicalDecoder(const std::string& connection_string);
        ~LogicalDecoder();

        bool createPublication(
            const std::string& publication_name,
            const std::vector<std::string>& tables = {}
        );

        bool createReplicationSlot(
            const std::string& slot_name,
            bool temporary = false
        );

        /**
         * @brief Subscribe To Changes.
         * @param[in] slot_name Name of the slot.
         * @param[in] on_change Input parameter.
         */
        void subscribeToChanges(
            const std::string& slot_name,
            const ChangeCallback& on_change
        );

        /**
         * @brief Confirm LSN.
         * @param[in] lsn Input parameter.
         */
        void confirmLSN(uint64_t lsn);

        /**
         * @brief Cancel.
         */
        void cancel();

    private:
        std::string connection_string_;
        bool cancelled_{false};
        uint64_t last_confirmed_lsn_{0};
    };

    // ------------------------------------------------------------------
    // High-level factory
    // ------------------------------------------------------------------

    static std::unique_ptr<LogicalDecoder> createDecoder(
        const std::string& connection_string,
        const CDCOptions& opts = CDCOptions{}
    );
};

} // namespace importers
} // namespace themis
