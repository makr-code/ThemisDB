/**
 * @file cdc_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef DELETE
#undef DELETE
#endif

#include "ingestion_manager.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace themis {
namespace ingestion {

/**
 * @brief CDC (Change-Data-Capture) source connector for live database streams
 *
 * Consumes change events from a live database replication stream (PostgreSQL
 * logical replication, MySQL binlog, or any compatible CDC source) and ingests
 * each event as a document into ThemisDB.
 *
 * When `THEMIS_ENABLE_CDC_STREAM` is defined at compile time, the real
 * replication-stream backend is compiled.  Without that flag the connector
 * still compiles but returns `CONNECTOR_NOT_SUPPORTED` on any live call,
 * unless a mock has been injected via `setCdcEventFetchForTesting()`.
 *
 * Supported `SourceConfig::options` keys:
 * | Key                  | Description                                              | Default              |
 * |----------------------|----------------------------------------------------------|----------------------|
 * | `slot_name`          | Replication slot name (PostgreSQL) or equivalent         | `themis_cdc`         |
 * | `table_filter`       | Comma-separated table names to capture (empty = all)     | (all tables)         |
 * | `operations`         | Comma-separated ops to capture: `INSERT,UPDATE,DELETE`   | `INSERT,UPDATE,DELETE` |
 * | `text_columns`       | Comma-separated columns to use as document text          | (full event JSON)    |
 * | `batch_size`         | Number of events to fetch per poll iteration             | `500`                |
 * | `max_events`         | Maximum events to consume (0 = unlimited)                | `0`                  |
 * | `poll_timeout_ms`    | Poll sleep duration between empty polls (milliseconds)   | `1000`               |
 * | `max_empty_polls`    | Consecutive empty polls before stopping (≥1)             | `3`                  |
 * | `from_lsn`           | Start from this LSN / binlog position (empty = start)    | (from beginning)     |
 *
 * `SourceConfig::location` must be a database connection URL:
 *   `postgresql://localhost:5432/mydb`
 *   `mysql://db.example.com:3306/inventory`
 *
 * Each change event is serialized as a JSON object of the form:
 * @code
 * {
 *   "operation": "INSERT",
 *   "table": "orders",
 *   "key": "42",
 *   "lsn": 12345678,
 *   "timestamp_ms": 1706438400000,
 *   "before": {},
 *   "after": {"id":"42","status":"created"}
 * }
 * @endcode
 *
 * The text for ingestion is either the concatenation of all configured
 * `text_columns` values from the `after` image (or `before` for DELETE), or
 * the full JSON serialization when `text_columns` is not set.
 *
 * Example usage (direct):
 * @code
 * SourceConfig cfg{
 *     .source_id = "orders_cdc",
 *     .type      = SourceType::CDC,
 *     .location  = "postgresql://localhost:5432/shop",
 *     .options   = {{"slot_name","themis_orders"},
 *                   {"table_filter","orders,order_items"},
 *                   {"text_columns","status,description"},
 *                   {"max_events","10000"}}
 * };
 * CdcConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("orders_collection", nullptr);
 * @endcode
 *
 * Example usage via IngestionBuilder:
 * @code
 * auto mgr = IngestionBuilder("mydb")
 *     .withCdcSource("orders_cdc",
 *                    "postgresql://localhost:5432/shop",
 *                    {{"slot_name","themis_orders"},
 *                     {"table_filter","orders"},
 *                     {"text_columns","status,description"},
 *                     {"max_events","10000"}})
 *     .build();
 * auto report = mgr->ingestAll();
 * @endcode
 */
class CdcConnector : public ISourceConnector {
public:
    CdcConnector();
    ~CdcConnector() override;

    // Non-copyable
    CdcConnector(const CdcConnector&) = delete;
    CdcConnector& operator=(const CdcConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration.
     * @param config  Must have `type == SourceType::CDC`; `location` is the
     *                database connection URL.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /**
     * @brief Check whether the CDC stream source is reachable.
     *
     * Attempts to open and immediately close a connection to the database.
     * Returns true when a test mock is injected.
     */
    bool isAvailable() const override;

    /**
     * @brief Estimate available event count.
     *
     * Returns 0 when the total is not known in advance (streaming sources
     * typically do not expose a count up front).
     */
    size_t getDocumentCount() const override;

    /**
     * @brief Consume CDC events from the stream and ingest each as a document.
     *
     * Events are fetched in batches of `batch_size`.  Progress callbacks are
     * invoked after each batch.  Consumption stops when `max_events` is
     * reached, the stream is drained, or the poll timeout expires with no new
     * events.
     *
     * When `THEMIS_ENABLE_CDC_STREAM` is not defined and no test mock is
     * present, returns immediately with a `CONNECTOR_NOT_SUPPORTED` error.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Configure retry behaviour for this connector.
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief A single CDC change event.
     *
     * Represents one row-level change captured from the database replication
     * stream.
     */
    struct CdcEvent {
        enum class Operation { INSERT, UPDATE, DELETE };

        Operation operation = Operation::INSERT;
        std::string table;  ///< Table that was changed
        std::string key;    ///< Primary-key value (stringified)
        uint64_t lsn = 0;   ///< Log sequence number / binlog position
        int64_t timestamp_ms = 0;  ///< Event timestamp in milliseconds

        /// Row state before the change (non-empty for UPDATE and DELETE)
        std::unordered_map<std::string, std::string> before;
        /// Row state after the change (non-empty for INSERT and UPDATE)
        std::unordered_map<std::string, std::string> after;
    };

    /**
     * @brief Function type for providing CDC event batches.
     *
     * Each call should return the next batch of events.  Return an empty
     * vector to signal end-of-stream.  Injected via
     * `setCdcEventFetchForTesting()`.
     */
    using CdcEventFetchFn = std::function<std::vector<CdcEvent>()>;

    /**
     * @brief Inject a CDC event-batch provider.
     *
     * When set, every replication-stream poll that would normally be executed
     * against a live database is replaced by calls to @p fn.  Pass an empty
     * `CdcEventFetchFn{}` to restore the real stream path.
     */
    void setEventBatchProvider(CdcEventFetchFn fn);
    void setCdcEventFetchForTesting(CdcEventFetchFn fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
