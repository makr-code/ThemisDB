/**
 * @file database_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>

namespace themis {
namespace ingestion {

/**
 * @brief JDBC-compatible database source connector
 *
 * Fetches rows from a relational database table or SQL query and ingests them
 * as documents into ThemisDB.  The connector accepts JDBC-style connection
 * strings (`jdbc:<subprotocol>://<host>:<port>/<database>`) and maps them to
 * the appropriate ODBC Data Source Name or driver-specific connection string.
 *
 * When `THEMIS_ENABLE_ODBC` is defined at compile time, the ODBC API
 * (`sql.h` / `sqlext.h`) is used to execute the query.  Without that flag
 * the connector still compiles but always returns `CONNECTOR_NOT_SUPPORTED`
 * unless a row-fetch mock has been injected via `setRowFetchForTesting()`.
 *
 * Supported `SourceConfig::options` keys:
 * | Key              | Description                                                  | Default         |
 * |------------------|--------------------------------------------------------------|-----------------|
 * | `query`          | SQL SELECT statement to execute                              | `SELECT * FROM <table>` |
 * | `table`          | Table name used when `query` is not set                      | (required if no query) |
 * | `text_columns`   | Comma-separated column names to concatenate as document text | (all columns)   |
 * | `batch_size`     | Number of rows to fetch per iteration                        | `500`           |
 * | `max_rows`       | Maximum total rows to fetch (0 = unlimited)                  | `0`             |
 * | `username`       | Database username (never logged)                             | (from DSN)      |
 * | `password`       | Database password (never logged)                             | (from DSN)      |
 * | `driver`         | ODBC driver name (e.g. `"PostgreSQL Unicode"`)               | (from DSN)      |
 * | `timeout_s`      | Login and query timeout in seconds                           | `30`            |
 *
 * `SourceConfig::location` must be a JDBC-style connection URL:
 *   `jdbc:postgresql://localhost:5432/mydb`
 *   `jdbc:mysql://db.example.com:3306/inventory`
 *   `jdbc:sqlserver://host:1433;databaseName=sales`
 *   `jdbc:sqlite:/path/to/file.db`
 *
 * The connector parses the URL and constructs an ODBC connection string of
 * the form: `DRIVER={<driver>};SERVER=<host>;PORT=<port>;DATABASE=<db>;
 * UID=<user>;PWD=<password>;TIMEOUT=<timeout_s>`
 *
 * Each fetched row is serialized as a JSON object
 * (`{"col1":"val1","col2":"val2"}`) and the text for ingestion is either the
 * concatenation of all configured `text_columns` values or the full JSON
 * serialization when `text_columns` is not set.
 *
 * Example usage (direct):
 * @code
 * SourceConfig cfg{
 *     .source_id = "product_db",
 *     .type      = SourceType::DATABASE,
 *     .location  = "jdbc:postgresql://localhost:5432/shop",
 *     .options   = {{"username","reader"},
 *                   {"password","s3cret"},
 *                   {"query","SELECT id, description FROM products"},
 *                   {"text_columns","description"},
 *                   {"batch_size","200"}}
 * };
 * DatabaseConnector conn;
 * conn.initialize(cfg);
 * auto stats = conn.ingest("products_collection", nullptr);
 * @endcode
 *
 * Example usage via IngestionBuilder:
 * @code
 * auto mgr = IngestionBuilder("mydb")
 *     .withDatabaseSource("product_db",
 *                         "jdbc:postgresql://localhost:5432/shop",
 *                         {{"username","reader"},
 *                          {"password","s3cret"},
 *                          {"query","SELECT id, description FROM products"},
 *                          {"text_columns","description"}})
 *     .build();
 * auto report = mgr->ingestAll();
 * @endcode
 */
class DatabaseConnector : public ISourceConnector {
public:
    DatabaseConnector();
    ~DatabaseConnector() override;

    // Non-copyable
    DatabaseConnector(const DatabaseConnector&) = delete;
    DatabaseConnector& operator=(const DatabaseConnector&) = delete;

    /**
     * @brief Initialize the connector from a source configuration.
     * @param config  Must have `type == SourceType::DATABASE`; `location` is
     *                the JDBC connection URL.
     * @return true on success
     */
    bool initialize(const SourceConfig& config) override;

    /**
     * @brief Check whether the database is reachable.
     *
     * Attempts to open and immediately close an ODBC connection.  Returns
     * true when a test mock is injected.
     */
    bool isAvailable() const override;

    /**
     * @brief Estimate the total row count.
     *
     * Executes `SELECT COUNT(*) FROM (<query>) AS __count` when the ODBC
     * driver supports it, or returns 0 when unavailable.
     */
    size_t getDocumentCount() const override;

    /**
     * @brief Execute the configured query and ingest each row as a document.
     *
     * Rows are fetched in batches of `batch_size` and converted to document
     * text.  Progress callbacks are invoked after each batch.  Pagination
     * stops when `max_rows` is reached or the result set is exhausted.
     *
     * When `THEMIS_ENABLE_ODBC` is not defined and no test mock is present,
     * returns immediately with a `CONNECTOR_NOT_SUPPORTED` error.
     */
    IngestionStats ingest(const std::string& target_collection,
                          ProgressCallback progress_callback) override;

    /**
     * @brief Configure retry behaviour for this connector.
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Row type: a mapping of column-name → cell-value strings.
     */
    using DbRow = std::unordered_map<std::string, std::string>;

    /**
     * @brief Function type for providing database row batches.
     *
     * Each call should return the next batch of rows.  Return an empty vector
     * to signal end-of-result-set.  Injected via `setRowFetchForTesting()`.
     */
    using RowFetchFn = std::function<std::vector<DbRow>()>;

    /**
     * @brief Inject a database row-batch provider.
     *
     * When set, every database query that would normally be executed via ODBC
     * is replaced by calls to @p fn.  Pass an empty `RowFetchFn{}` to restore
     * the real ODBC path.
     */
    void setRowBatchProvider(RowFetchFn fn);
    void setRowFetchForTesting(RowFetchFn fn);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis

