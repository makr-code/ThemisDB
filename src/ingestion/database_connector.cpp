/**
 * @file database_connector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=12; TODO=1, Stub=3, Unimpl=0, Mock=6, Sim=2, Debt=0, C=3, H=2, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// When THEMIS_ENABLE_ODBC is defined the full ODBC-backed implementation is
// compiled.  Without that flag the connector still compiles and:
//   - returns CONNECTOR_NOT_SUPPORTED on any live DB call, OR
//   - uses injected mock functions (unit tests).

#include "ingestion/database_connector.h"

#ifdef THEMIS_ENABLE_ODBC
#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <cctype>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// JDBC URL parser helpers
// ---------------------------------------------------------------------------

namespace {

struct JdbcUrl {
    std::string subprotocol; ///< e.g. "postgresql", "mysql", "sqlserver", "sqlite"
    std::string host;
    int         port = 0;
    std::string database;
    std::string raw; ///< original location string
};

/// Parse `jdbc:<sub>://<host>:<port>/<db>` or `jdbc:<sub>://<host>/<db>`.
/// SQLite: `jdbc:sqlite:/path/to/file.db`
static JdbcUrl parseJdbcUrl(const std::string& url) {
    JdbcUrl result;
    result.raw = url;

    // Must start with "jdbc:"
    const std::string prefix = "jdbc:";
    if (url.size() <= prefix.size() ||
        url.substr(0, prefix.size()) != prefix) {
        return result;
    }

    std::string rest = url.substr(prefix.size()); // e.g. "postgresql://host:5432/db"

    // Extract subprotocol (everything before first ':')
    auto colon = rest.find(':');
    if (colon == std::string::npos) return result;
    result.subprotocol = rest.substr(0, colon);

    // Convert to lowercase for comparison
    std::transform(result.subprotocol.begin(), result.subprotocol.end(),
                   result.subprotocol.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    std::string after_sub = rest.substr(colon + 1); // e.g. "//host:5432/db"

    // SQLite uses a file path, not a host/port
    if (result.subprotocol == "sqlite") {
        // jdbc:sqlite:/path/to/file.db  → database = /path/to/file.db
        if (after_sub.size() >= 2 && after_sub[0] == '/' && after_sub[1] == '/') {
            after_sub = after_sub.substr(2);
        }
        result.database = after_sub;
        return result;
    }

    // Strip leading "//"
    if (after_sub.size() >= 2 && after_sub[0] == '/' && after_sub[1] == '/') {
        after_sub = after_sub.substr(2);
    }

    // SQL Server uses semicolons: "host:port;databaseName=db"
    // Generic: "host:port/db" or "host/db"
    std::string host_part;
    std::string db_part;

    auto semicolon = after_sub.find(';');
    if (semicolon != std::string::npos) {
        // SQL Server style
        host_part = after_sub.substr(0, semicolon);
        std::string params = after_sub.substr(semicolon + 1);
        // Look for databaseName=<db>
        const std::string dbkey = "databaseName=";
        auto pos = params.find(dbkey);
        if (pos != std::string::npos) {
            std::string val = params.substr(pos + dbkey.size());
            auto semi2 = val.find(';');
            db_part = (semi2 != std::string::npos) ? val.substr(0, semi2) : val;
        }
    } else {
        auto slash = after_sub.find('/');
        if (slash != std::string::npos) {
            host_part = after_sub.substr(0, slash);
            db_part   = after_sub.substr(slash + 1);
        } else {
            host_part = after_sub;
        }
    }

    // Split host_part into host:port
    auto port_colon = host_part.rfind(':');
    if (port_colon != std::string::npos) {
        result.host = host_part.substr(0, port_colon);
        try { result.port = std::stoi(host_part.substr(port_colon + 1)); }
        catch (...) { result.port = 0; }
    } else {
        result.host = host_part;
    }
    result.database = db_part;
    return result;
}

/// Build an ODBC connection string from the parsed JDBC URL and options.
static std::string buildOdbcConnectionString(
        const JdbcUrl& jdbc,
        const std::string& username,
        const std::string& password,
        const std::string& driver_override,
        int timeout_s) {

    // Default driver names per subprotocol
    std::string driver;
    if (!driver_override.empty()) {
        driver = driver_override;
    } else if (jdbc.subprotocol == "postgresql") {
        driver = "PostgreSQL Unicode";
    } else if (jdbc.subprotocol == "mysql") {
        driver = "MySQL ODBC 8.0 Unicode Driver";
    } else if (jdbc.subprotocol == "sqlserver" ||
               jdbc.subprotocol == "jtds" ||
               jdbc.subprotocol == "mssql") {
        driver = "ODBC Driver 17 for SQL Server";
    } else if (jdbc.subprotocol == "sqlite") {
        driver = "SQLite3 ODBC Driver";
    } else {
        driver = jdbc.subprotocol; // pass-through
    }

    std::ostringstream cs;
    cs << "DRIVER={" << driver << "};";

    if (jdbc.subprotocol == "sqlite") {
        cs << "Database=" << jdbc.database << ";";
    } else {
        if (!jdbc.host.empty()) cs << "SERVER=" << jdbc.host << ";";
        if (jdbc.port > 0)      cs << "PORT=" << jdbc.port << ";";
        if (!jdbc.database.empty()) cs << "DATABASE=" << jdbc.database << ";";
    }

    if (!username.empty()) cs << "UID=" << username << ";";
    if (!password.empty()) cs << "PWD=" << password << ";";
    if (timeout_s > 0)     cs << "TIMEOUT=" << timeout_s << ";";

    return cs.str();
}

/// Return a copy of an ODBC connection string with the PWD value masked.
/// This is used in log/error messages to avoid credential leakage.
[[maybe_unused]] static std::string sanitisedConnectionString(const std::string& cs) {
    // Case-insensitive search for "PWD=" without copying the whole string.
    static const std::string target = "pwd=";
    auto it = std::search(cs.begin(), cs.end(),
                          target.begin(), target.end(),
                          [](char a, char b) {
                              return std::tolower(static_cast<unsigned char>(a))
                                  == std::tolower(static_cast<unsigned char>(b));
                          });
    if (it == cs.end()) return cs;

    std::string result = cs;
    std::size_t pos = static_cast<std::size_t>(it - cs.begin()) + target.size();
    auto end = result.find(';', pos);
    if (end == std::string::npos) {
        result.replace(pos, result.size() - pos, "***");
    } else {
        result.replace(pos, end - pos, "***");
    }
    return result;
}

/// Serialize a DbRow to a minimal JSON object (no external dependencies).
static std::string rowToJson(const DatabaseConnector::DbRow& row) {
    std::ostringstream js;
    js << '{';
    bool first = true;
    for (const auto& kv : row) {
        if (!first) js << ',';
        first = false;
        // Simple JSON string escaping
        auto escape = [](const std::string& s) -> std::string {
            std::string out;
            out.reserve(s.size() + 4);
            for (unsigned char c : s) {
                if (c == '"')  { out += "\\\""; }
                else if (c == '\\') { out += "\\\\"; }
                else if (c == '\n') { out += "\\n"; }
                else if (c == '\r') { out += "\\r"; }
                else if (c == '\t') { out += "\\t"; }
                else { out += static_cast<char>(c); }
            }
            return out;
        };
        js << '"' << escape(kv.first) << "\":\"" << escape(kv.second) << '"';
    }
    js << '}';
    return js.str();
}

/// Extract document text from a row: concatenate the requested columns with a
/// space separator; fall back to full JSON serialization when no columns are
/// specified or a requested column is absent.
static std::string rowToText(const DatabaseConnector::DbRow& row,
                              const std::vector<std::string>& text_columns) {
    if (text_columns.empty()) {
        return rowToJson(row);
    }
    std::string text;
    for (const auto& col : text_columns) {
        auto it = row.find(col);
        if (it != row.end() && !it->second.empty()) {
            if (!text.empty()) text += ' ';
            text += it->second;
        }
    }
    return text.empty() ? rowToJson(row) : text;
}

/// Split a comma-separated string into a vector of trimmed tokens.
static std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // trim whitespace
        auto b = token.find_first_not_of(" \t");
        auto e = token.find_last_not_of(" \t");
        if (b != std::string::npos) {
            result.push_back(token.substr(b, e - b + 1));
        }
    }
    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

/** @brief Pimpl. */
class DatabaseConnector::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::DATABASE) return false;
        config_ = config;

        auto opt = [&](const std::string& k, const std::string& def) {
            auto it = config.options.find(k);
            return (it != config.options.end()) ? it->second : def;
        };

        jdbc_       = parseJdbcUrl(config.location);
        username_   = opt("username", "");
        password_   = opt("password", "");
        driver_     = opt("driver", "");
        table_      = opt("table", "");
        user_query_ = opt("query", "");

        std::string text_cols_str = opt("text_columns", "");
        text_columns_ = text_cols_str.empty() ? std::vector<std::string>{}
                                              : splitComma(text_cols_str);

        try { batch_size_ = static_cast<size_t>(std::stoull(opt("batch_size", "500"))); }
        catch (...) { batch_size_ = 500; }
        if (batch_size_ == 0) batch_size_ = 500;

        try { max_rows_ = static_cast<size_t>(std::stoull(opt("max_rows", "0"))); }
        catch (...) { max_rows_ = 0; }

        try { timeout_s_ = std::stoi(opt("timeout_s", "30")); }
        catch (...) { timeout_s_ = 30; }

        // Build the effective SQL query
        if (user_query_.empty()) {
            if (table_.empty()) return false; // need at least a table
            effective_query_ = "SELECT * FROM " + table_;
        } else {
            effective_query_ = user_query_;
        }

        // Build ODBC connection string (used in production path only)
        odbc_conn_str_ = buildOdbcConnectionString(
            jdbc_, username_, password_, driver_, timeout_s_);

        return true;
    }

    bool isAvailable() const {
        if (row_fetch_fn_) return true; // test mock always available

#ifdef THEMIS_ENABLE_ODBC
        SQLHENV henv = SQL_NULL_HENV;
        SQLHDBC hdbc = SQL_NULL_HDBC;

        if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv) != SQL_SUCCESS)
            return false;
        SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                      reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        if (SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
            return false;
        }
        SQLSetConnectAttr(hdbc, SQL_ATTR_LOGIN_TIMEOUT,
                          reinterpret_cast<SQLPOINTER>(
                              static_cast<intptr_t>(timeout_s_)), 0);

        SQLCHAR out_conn[1024];
        SQLSMALLINT out_len = 0;
        SQLRETURN rc = SQLDriverConnect(
            hdbc, nullptr,
            reinterpret_cast<SQLCHAR*>(
                const_cast<char*>(odbc_conn_str_.c_str())),
            SQL_NTS,
            out_conn, sizeof(out_conn), &out_len,
            SQL_DRIVER_NOPROMPT);

        bool ok = (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO);
        if (ok) SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        return ok;
#else
        return false;
#endif
    }

    size_t getDocumentCount() const {
        if (row_fetch_fn_) return 0; // not known from mock
#ifdef THEMIS_ENABLE_ODBC
        // Execute COUNT(*) wrapper
        std::string count_query =
            "SELECT COUNT(*) FROM (" + effective_query_ + ") AS __count_alias";
        SQLHENV henv = SQL_NULL_HENV;
        SQLHDBC hdbc = SQL_NULL_HDBC;
        SQLHSTMT hstmt = SQL_NULL_HSTMT;
        size_t count = 0;

        if (!openConnection(henv, hdbc)) return 0;
        if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS) {
            closeConnection(hdbc, henv);
            return 0;
        }

        SQLRETURN rc = SQLExecDirect(
            hstmt,
            reinterpret_cast<SQLCHAR*>(const_cast<char*>(count_query.c_str())),
            SQL_NTS);

        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
            if (SQLFetch(hstmt) == SQL_SUCCESS) {
                SQLLEN ind = 0;
                SQLBIGINT val = 0;
                if (SQLGetData(hstmt, 1, SQL_C_SBIGINT, &val, sizeof(val), &ind)
                        == SQL_SUCCESS) {
                    count = static_cast<size_t>(val);
                }
            }
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        closeConnection(hdbc, henv);
        return count;
#else
        return 0;
#endif
    }

    IngestionStats ingest(const std::string& /*target_collection*/,
                          ProgressCallback progress_callback) {
        IngestionStats stats;
        auto start_time = std::chrono::steady_clock::now();

        if (effective_query_.empty()) {
            stats.addError(IngestionErrorCode::SOURCE_NOT_CONFIGURED,
                           IngestionErrorSeverity::FATAL,
                           "DatabaseConnector not configured: no query or table set",
                           config_.source_id);
            finaliseStats(stats, start_time);
            return stats;
        }

        // ------------------------------------------------------------------
        // Test mock path: no ODBC required
        // ------------------------------------------------------------------
        if (row_fetch_fn_) {
            ingestFromMock(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }

        // ------------------------------------------------------------------
        // Production path: ODBC
        // ------------------------------------------------------------------
#ifdef THEMIS_ENABLE_ODBC
        ingestFromOdbc(stats, progress_callback);
#else
        stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                       IngestionErrorSeverity::FATAL,
                       "DatabaseConnector requires THEMIS_ENABLE_ODBC at build time",
                       config_.source_id);
#endif
        finaliseStats(stats, start_time);
        return stats;
    }

    void setRetryConfig(const RetryConfig& c)    { retry_config_ = c; }
    void setRowFetchForTesting(RowFetchFn fn)     { row_fetch_fn_ = std::move(fn); }

private:
    // -----------------------------------------------------------------------
    // STUB/SIMULATION NOTE:
    // Purpose: Enable unit-testing of DatabaseConnector without a live RDBMS
    //   by using an injected row_fetch_fn_ instead of a real SQL connection.
    // Activation: Active when row_fetch_fn_ is non-null (set via
    //   DatabaseConnector::setRowFetchForTesting()).
    // Production Delta: Rows come from the injected lambda instead of a real
    //   database connection.  No connection pooling, no transaction management.
    // Roadmap ref: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
    // Removal Plan: Not removed — remains the test-injection path.
    // Roadmap ref: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
    // -----------------------------------------------------------------------
    void ingestFromMock(IngestionStats& stats,
                        ProgressCallback& progress_callback) {
        size_t fetched = 0;
        try {
            while (true) {
                if (max_rows_ > 0 && fetched >= max_rows_) break;

                auto batch = row_fetch_fn_();
                if (batch.empty()) break;

                for (const auto& row : batch) {
                    if (max_rows_ > 0 && fetched >= max_rows_) break;

                    std::string text = rowToText(row, text_columns_);
                    std::string json = rowToJson(row);
                    stats.bytes_processed += json.size();
                    if (!text.empty()) {
                        ++stats.documents_processed;
                    } else {
                        ++stats.documents_failed;
                        stats.addError(IngestionErrorCode::EXTRACTION_FAILED,
                                       IngestionErrorSeverity::WARNING,
                                       "Empty text extracted from row",
                                       config_.source_id);
                    }
                    ++fetched;
                }

                if (progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0, // total unknown
                                      "fetched " + std::to_string(fetched) + " rows");
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in DatabaseConnector mock ingest: " +
                           std::string(e.what()),
                           config_.source_id);
        }
    }

#ifdef THEMIS_ENABLE_ODBC
    // -----------------------------------------------------------------------
    // ODBC-based ingestion (production)
    // -----------------------------------------------------------------------
    bool openConnection(SQLHENV& henv, SQLHDBC& hdbc) const {
        henv = SQL_NULL_HENV;
        hdbc = SQL_NULL_HDBC;

        if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv) != SQL_SUCCESS)
            return false;
        SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                      reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        if (SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
            henv = SQL_NULL_HENV;
            return false;
        }
        SQLSetConnectAttr(hdbc, SQL_ATTR_LOGIN_TIMEOUT,
                          reinterpret_cast<SQLPOINTER>(
                              static_cast<intptr_t>(timeout_s_)), 0);

        SQLCHAR out_conn[1024];
        SQLSMALLINT out_len = 0;
        SQLRETURN rc = SQLDriverConnect(
            hdbc, nullptr,
            reinterpret_cast<SQLCHAR*>(
                const_cast<char*>(odbc_conn_str_.c_str())),
            SQL_NTS,
            out_conn, sizeof(out_conn), &out_len,
            SQL_DRIVER_NOPROMPT);

        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
            hdbc = SQL_NULL_HDBC;
            henv = SQL_NULL_HENV;
            return false;
        }
        return true;
    }

    static void closeConnection(SQLHDBC hdbc, SQLHENV henv) {
        if (hdbc != SQL_NULL_HDBC) {
            SQLDisconnect(hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        }
        if (henv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
        }
    }

    void ingestFromOdbc(IngestionStats& stats,
                        ProgressCallback& progress_callback) {
        SQLHENV henv = SQL_NULL_HENV;
        SQLHDBC hdbc = SQL_NULL_HDBC;

        if (!openConnection(henv, hdbc)) {
            stats.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                           IngestionErrorSeverity::FATAL,
                           "Failed to open ODBC connection: " +
                           sanitisedConnectionString(odbc_conn_str_),
                           config_.source_id);
            return;
        }

        SQLHSTMT hstmt = SQL_NULL_HSTMT;
        if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS) {
            stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                           IngestionErrorSeverity::FATAL,
                           "SQLAllocHandle(STMT) failed",
                           config_.source_id);
            closeConnection(hdbc, henv);
            return;
        }

        // Set query timeout
        SQLSetStmtAttr(hstmt, SQL_ATTR_QUERY_TIMEOUT,
                       reinterpret_cast<SQLPOINTER>(
                           static_cast<intptr_t>(timeout_s_)), 0);

        SQLRETURN rc = SQLExecDirect(
            hstmt,
            reinterpret_cast<SQLCHAR*>(
                const_cast<char*>(effective_query_.c_str())),
            SQL_NTS);

        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                           IngestionErrorSeverity::FATAL,
                           "SQLExecDirect failed for query: " + effective_query_,
                           config_.source_id);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            closeConnection(hdbc, henv);
            return;
        }

        // Determine column count and names
        SQLSMALLINT col_count = 0;
        SQLNumResultCols(hstmt, &col_count);

        std::vector<std::string> col_names;
        col_names.reserve(static_cast<size_t>(col_count));
        for (SQLSMALLINT i = 1; i <= col_count; ++i) {
            SQLCHAR name[256];
            SQLSMALLINT name_len = 0;
            SQLDescribeCol(hstmt, i, name, sizeof(name), &name_len,
                           nullptr, nullptr, nullptr, nullptr);
            col_names.push_back(std::string(reinterpret_cast<char*>(name),
                                            static_cast<size_t>(name_len)));
        }

        // Fetch rows
        size_t fetched = 0;
        const size_t buf_size = 4096;

        try {
            while (true) {
                if (max_rows_ > 0 && fetched >= max_rows_) break;

                rc = SQLFetch(hstmt);
                if (rc == SQL_NO_DATA) break;
                if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                    stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                                   IngestionErrorSeverity::WARNING,
                                   "SQLFetch returned error at row " +
                                   std::to_string(fetched),
                                   config_.source_id);
                    break;
                }

                DbRow row;
                for (SQLSMALLINT i = 1; i <= col_count; ++i) {
                    std::string cell_value;
                    SQLCHAR buf[buf_size];
                    SQLLEN ind = 0;
                    SQLRETURN grc = SQLGetData(hstmt, i, SQL_C_CHAR,
                                               buf, sizeof(buf), &ind);
                    if (grc == SQL_SUCCESS || grc == SQL_SUCCESS_WITH_INFO) {
                        if (ind != SQL_NULL_DATA && ind > 0) {
                            cell_value.assign(reinterpret_cast<char*>(buf),
                                              static_cast<size_t>(
                                                  std::min(ind,
                                                      static_cast<SQLLEN>(
                                                          buf_size - 1))));
                        }
                    }
                    if (i - 1 < static_cast<SQLSMALLINT>(col_names.size())) {
                        row[col_names[static_cast<size_t>(i - 1)]] = cell_value;
                    }
                }

                std::string text = rowToText(row, text_columns_);
                std::string json = rowToJson(row);
                stats.bytes_processed += json.size();

                if (!text.empty()) {
                    ++stats.documents_processed;
                } else {
                    ++stats.documents_failed;
                    stats.addError(IngestionErrorCode::EXTRACTION_FAILED,
                                   IngestionErrorSeverity::WARNING,
                                   "Empty text extracted from row " +
                                   std::to_string(fetched),
                                   config_.source_id);
                }
                ++fetched;

                // Invoke progress callback once per batch
                if (fetched % batch_size_ == 0 && progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0,
                                      "fetched " + std::to_string(fetched) + " rows");
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in ODBC ingest: " + std::string(e.what()),
                           config_.source_id);
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        closeConnection(hdbc, henv);
    }
#endif // THEMIS_ENABLE_ODBC

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    static void finaliseStats(IngestionStats& stats,
                               const std::chrono::steady_clock::time_point& start) {
        auto end = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end - start).count();
    }

    // Fields
    SourceConfig               config_;
    JdbcUrl                    jdbc_;
    std::string                username_;
    std::string                password_;
    std::string                driver_;
    std::string                table_;
    std::string                user_query_;
    std::string                effective_query_;
    std::string                odbc_conn_str_;
    std::vector<std::string>   text_columns_;
    size_t                     batch_size_   = 500;
    size_t                     max_rows_     = 0;
    int                        timeout_s_    = 30;
    RetryConfig                retry_config_;
    RowFetchFn                 row_fetch_fn_;
};

// ---------------------------------------------------------------------------
// DatabaseConnector public API (thin wrappers around Pimpl)
// ---------------------------------------------------------------------------

DatabaseConnector::DatabaseConnector()
    : impl_(std::make_unique<Impl>()) {}

DatabaseConnector::~DatabaseConnector() = default;

bool DatabaseConnector::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool DatabaseConnector::isAvailable() const {
    return impl_->isAvailable();
}

size_t DatabaseConnector::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats DatabaseConnector::ingest(const std::string& target_collection,
                                          ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, std::move(progress_callback));
}

void DatabaseConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void DatabaseConnector::setRowFetchForTesting(RowFetchFn fn) {
    setRowBatchProvider(std::move(fn));
}

void DatabaseConnector::setRowBatchProvider(RowFetchFn fn) {
    impl_->setRowFetchForTesting(std::move(fn));
}

} // namespace ingestion
} // namespace themis


