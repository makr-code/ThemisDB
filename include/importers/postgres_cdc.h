/**
 * @file postgres_cdc.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief CDC configuration options.
 */
struct CDCOptions {
    bool full_sync_first{true};          ///< Perform an initial snapshot?
    bool include_truncate{true};         ///< Stream TRUNCATE events?
    uint64_t polling_interval_ms{100};   ///< Reconnect back-off (ms)
    size_t buffer_size{10000};           ///< In-memory event buffer capacity
    std::string slot_name{"themisdb_cdc_slot"};
    std::string publication_name{"themisdb_publication"};
};

/**
 * @brief Change Data Capture via PostgreSQL Logical Decoding.
 *
 * Uses the pgoutput replication protocol (standard since PostgreSQL 10) to
 * stream DML changes as push-based events rather than polling.
 *
 * References:
 *   - Lin et al. (2020) "LogicalLog: A High-Performance Logical Data Replication Engine"
 *   - PostgreSQL Logical Replication Protocol documentation
 */
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
        explicit LogicalDecoder(const std::string& connection_string);
        ~LogicalDecoder();

        /**
         * @brief Create a logical replication publication for the given tables.
         * @param publication_name  Name of the PostgreSQL publication.
         * @param tables            Table names to include (empty = all tables).
         * @return true on success.
         */
        bool createPublication(
            const std::string& publication_name,
            const std::vector<std::string>& tables = {}
        );

        /**
         * @brief Create or reuse a replication slot.
         * @param slot_name   Slot name (must be unique per server).
         * @param temporary   If true the slot is dropped when the connection closes.
         * @return true on success.
         */
        bool createReplicationSlot(
            const std::string& slot_name,
            bool temporary = false
        );

        /**
         * @brief Start streaming changes.  Calls on_change for each event.
         *
         * This method blocks until cancel() is called or a fatal error occurs.
         *
         * @param slot_name  Replication slot to consume from.
         * @param on_change  Callback invoked for every decoded event.
         */
        void subscribeToChanges(
            const std::string& slot_name,
            const ChangeCallback& on_change
        );

        /**
         * @brief Acknowledge all changes up to (and including) lsn.
         * Advances the replication slot's confirmed_flush_lsn.
         */
        void confirmLSN(uint64_t lsn);

        /** @brief Stop the subscribeToChanges loop. */
        void cancel();

    private:
        std::string connection_string_;
        bool cancelled_{false};
        uint64_t last_confirmed_lsn_{0};
    };

    // ------------------------------------------------------------------
    // High-level factory
    // ------------------------------------------------------------------

    /**
     * @brief Create a LogicalDecoder connected to the given PostgreSQL instance.
     * @param connection_string  libpq-style connection string.
     * @param opts               CDC configuration.
     */
    static std::unique_ptr<LogicalDecoder> createDecoder(
        const std::string& connection_string,
        const CDCOptions& opts = CDCOptions{}
    );
};

} // namespace importers
} // namespace themis
