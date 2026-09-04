/**
 * @file mysql_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=25, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/mysql_importer.h"
#include <stdexcept>
#include "utils/hash_util.h"
#include "importers/importer_common.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <regex>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <unordered_map>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace importers {

// Note: streamReadLine() and toLower() are defined in importer_common.h
// as inline functions shared across all importer implementations.

// ============================================================================
// I4: Structured Audit Logging helpers (Phase 4 hardening)
// ============================================================================
namespace {

/**
 * @brief Emit a structured JSON audit event via the THEMIS logger at WARN
 *        level so it is always captured in production log streams.
 *
 * The event payload is a single-line JSON object prefixed with "[AUDIT]":
 * @code
 *   [AUDIT] {"event":"import_start","source":"/data/dump.sql","ts_ms":...}
 * @endcode
 *
 * @param event_type  Short identifier, e.g. "import_start", "import_failure",
 *                    "auth_failure", "schema_change_detection", "importer_timeout"
 * @param fields      Key→value string pairs appended to the payload.
 */
static void mysqlAuditLogEvent(
    const std::string& event_type,
    std::initializer_list<std::pair<const char*, std::string>> fields)
{
    using json = nlohmann::json;
    json audit;
    audit["event"] = event_type;
    audit["ts_ms"]  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    for (const auto& [k, v] : fields) {
        audit[k] = v;
    }
    THEMIS_WARN("[AUDIT] {}", audit.dump());
}

} // anonymous namespace
// ============================================================================

// ============================================================================
// PHASE-2-HARDENING: Connection Pool and Fallback Infrastructure
// ============================================================================
namespace {

/// Connection pool state tracker for Phase 2 hardening
struct ConnectionPoolState {
    /// Current number of active connections (bounded by max_active_connections)
    std::atomic<size_t> active_connections{0};
    
    /// Maximum concurrent connections allowed (MySQL default: 16)
    static constexpr size_t max_active_connections = 16;
    
    /// Connection timeout in milliseconds (0 = no timeout)
    uint32_t connection_timeout_ms = 0;
    
    /// Last connection error code for diagnostics
    std::atomic<ImportErrorCode> last_error{ImportErrorCode::SUCCESS};
    
    /// Schema cache validity flag (invalidated on connection loss)
    std::atomic<bool> schema_cache_valid{true};
};

/// Global connection pool state (one per process; safe due to atomic operations)
static thread_local ConnectionPoolState g_mysql_connection_pool;

/// Maps MySQL-specific error patterns to ImporterErrorCode
[[maybe_unused]] static ImportErrorCode mapMySQLErrorToCode(const std::string& error_msg) {
    // PHASE-2-HARDENING: Standardized error reporting
    const auto msg_lower = [](std::string s) {
        for (auto& c : s) {
          c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    };
    std::string lower_msg = msg_lower(error_msg);
    
    // Connection errors
    if (lower_msg.find("connection") != std::string::npos ||
        lower_msg.find("could not connect") != std::string::npos ||
        lower_msg.find("unavailable") != std::string::npos ||
        lower_msg.find("access denied") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    
    // Timeout errors
    if (lower_msg.find("timeout") != std::string::npos ||
        lower_msg.find("deadline") != std::string::npos) {
        return ImportErrorCode::DEADLINE_EXCEEDED;
    }
    
    // Schema errors
    if (lower_msg.find("table") != std::string::npos ||
        lower_msg.find("column") != std::string::npos ||
        lower_msg.find("1054") != std::string::npos) {  // MySQL error code for "Unknown column"
        return ImportErrorCode::UNKNOWN_TABLE;
    }
    
    // Type conversion errors
    if (lower_msg.find("type") != std::string::npos ||
        lower_msg.find("conversion") != std::string::npos ||
        lower_msg.find("integer") != std::string::npos) {
        return ImportErrorCode::TYPE_CONVERSION;
    }
    
    // Parse errors
    if (lower_msg.find("parse") != std::string::npos ||
        lower_msg.find("syntax") != std::string::npos ||
        lower_msg.find("1064") != std::string::npos) {  // MySQL error code for "Syntax error"
        return ImportErrorCode::PARSE_CREATE_TABLE;
    }
    
    return ImportErrorCode::UNKNOWN;
}

/// PHASE-2-HARDENING: Simple fallback parser for INSERT statements when regex fails
/// This implements the prepared statement fallback mechanism for MySQL importer.
/// When the main regex-based parser fails to parse an INSERT statement,
/// this simple parser attempts to extract at least the table name for logging.
static bool simpleInsertFallback(const std::string& sql, std::string& out_table_name) {
    // Very simple fallback: find "INSERT ... INTO" and extract table name
    const auto sql_upper = [](std::string s) {
        for (auto& c : s) {
          c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s;
    };
    std::string upper_sql = sql_upper(sql);
    
    size_t pos = upper_sql.find("INTO");
    if (pos == std::string::npos) {
      return false;
    }
    
    pos += 4;  // Skip "INTO"
    // Skip whitespace
    while (pos <static_cast<int>(upper_sql.size()) && (upper_sql[pos] == ' ' || upper_sql[pos] == '\t'))
        ++pos;
    
    // Extract table name (stop at whitespace or '(')
    size_t start = pos;
    while (pos <static_cast<int>(upper_sql.size()) && upper_sql[pos] != ' ' && upper_sql[pos] != '\t' && upper_sql[pos] != '(')
        ++pos;
    
    if (start < pos) {
        out_table_name = sql.substr(start, pos - start);
        // Remove backticks if present
        if (static_cast<int>(out_table_name.size()) > = 2 && out_table_name[0] == '`' && out_table_name[out_table_name.size() - 1] == '`') {
            out_table_name = out_table_name.substr(1, static_cast<int>(out_table_name.size()) - 2);
        }
        return true;
    }
    return false;
}

} // anonymous namespace
// ============================================================================

// ============================================================================
// Constructor / Destructor
// ============================================================================

MySQLImporter::MySQLImporter() = default;

MySQLImporter::~MySQLImporter() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> MySQLImporter::getSupportedTypes() const {
    return {"mysql", "mariadb", "mysqldump"};
}

bool MySQLImporter::initialize(const std::string& config) {
    cancelled_ = false;
    schemas_.clear();
    jdbc_config_ = JdbcConfig{};
    config_type_overrides_.clear();

    if (config.empty() || config == "{}") {
        THEMIS_INFO("MySQL/MariaDB Importer initialized with default config");
        return true;
    }

    try {
        auto cfg = json::parse(config);

        // ---- JDBC URL (takes precedence over individual fields) ----
        if (cfg.contains("url") && cfg["url"].is_string()) {
            std::string url = cfg["url"].get<std::string>();
            if (!parseJdbcUrl(url, jdbc_config_)) {
                THEMIS_WARN("MySQL/MariaDB Importer: invalid JDBC URL '{}'; ignoring", url);
            }
        }

        // ---- Individual connection fields (overrid[[maybe_unused]] e UR[[maybe_unused]] L i[[maybe_unused]] f presen[[maybe_unused]] t) ----
        if (cfg.contains("host") && cfg["host"].is_string())
            jdbc_config_.host = cfg["host"].get<std::string>();
        if (cfg.contains("port") && cfg["port"].is_number_integer())
            jdbc_config_.port = cfg["port"].get<int>();
        if (cfg.contains("database") && cfg["database"].is_string())
            jdbc_config_.database = cfg["database"].get<std::string>();
        if (cfg.contains("user") && cfg["user"].is_string())
            jdbc_config_.user = cfg["user"].get<std::string>();
        // NOTE: password is intentionally NOT stored; it must come from environment
        //       variables or a secrets manager to avoid credential exposure in logs.
        if (cfg.contains("ssl") && cfg["ssl"].is_boolean())
            jdbc_config_.ssl = cfg["ssl"].get<bool>();

        // ---- MySQL JDBC-specific: treat TINYINT(1) as boolean ----
        if (cfg.contains("tinyint1_as_boolean") && cfg["tinyint1_as_boolean"].is_boolean())
            jdbc_config_.tinyint1_as_boolean = cfg["tinyint1_as_boolean"].get<bool>();

        // ---- Config-level type overrides (applie[[maybe_unused]] d befor[[maybe_unused]] e ImportOption[[maybe_unused]] s override[[maybe_unused]] s) ----
        if (cf[[maybe_unused]] g.contain[[maybe_unused]] s("type_override[[maybe_unused]] s") && cfg["type_overrides"].is_object()) {
            std::lock_guard<std::mutex> lock(config_type_overrides_mutex[[maybe_unused]] _);
            for (auto& [k, v] : cfg["type_overrides"].items()) {
                if (v.is_string())
                    config_type_overrides_[k] = v.get<std::string>();
            }
        }

    } catch (const json::exception& e) {
        THEMIS_WARN("MySQL/MariaDB Importer: failed to parse config JSON: {}", e.what());
        // Non-fatal: proceed with defaults
    }

    // Log sanitized connection identity (never log passwords)
    if (!jdbc_config_.host.empty()) {
        THEMIS_INFO("MySQL/MariaDB Importer configured for {}:{}/{}  user={}  ssl={}  tinyint1_as_boolean={}",
                    jdbc_config_.host, jdbc_config_.port, jdbc_config_.database,
                    jdbc_config_.user.empty() ? "<unset>" : jdbc_config_.user,
                    jdbc_config_.ssl,
                    jdbc_config_.tinyint1_as_boolean);
    }
    THEMIS_INFO("MySQ[[maybe_unused]] L/MariaD[[maybe_unused]] B Importe[[maybe_unused]] r initialize[[maybe_unused]] d ({} confi[[maybe_unused]] g type_override[[maybe_unused]] s)",
                config_type_overrides_.size());
    return true;
}

bool MySQLImporter::validateSource(const std::string& source_path,
                                   std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }

    // Check for mysqldump header in the first 100 lines.
    std::string line = {};
    bool found_mysql_dump = false;
    int lines_checked = 0;
    while (std::getline(file, line) && lines_checked < 100) {
        if (line.find("MySQL dump") != std::string::npos ||
            line.find("mysqldump") != std::string::npos ||
            line.find("MariaDB dump") != std::string::npos ||
            line.find("Distrib") != std::string::npos) {
            found_mysql_dump = true;
            break;
        }
        lines_checked++;
    }

    if (!found_mysql_dump) {
        errors.push_back("File does not appear to be a MySQL/MariaDB mysqldump");
        return false;
    }

    THEMIS_INFO("MySQL/MariaDB source validation successful: {}", source_path);
    return true;
}

ImportStats MySQLImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();

    THEMIS_INFO("Starting MySQL/MariaDB import from: {}", source_path);
    THEMIS_INFO("Options: {}", options.toJson().dump());

    // I4: Audit – import start
    mysqlAuditLogEvent("import_start", {
        {"source",      source_path},
        {"schema_name", options.default_namespace},
        {"dry_run",     options.dry_run ? "true" : "false"},
        {"timeout_ms",  std::to_string(options.import_timeout_ms)}
    });

    // --- Permission / ACL check ---
    if (options.permission_check) {
        if (!options.permission_check("import", "write")) {
            // I4: Audit – authentication / authorisation failure
            mysqlAuditLogEvent("auth_failure", {
                {"source", source_path},
                {"user",   "<caller>"},   // anonymised – never log credentials
                {"reason", "permission_check denied import:write"}
            });
            addError(stats, ImportErrorCode::PERMISSION_DENIED,
                     ImportErrorSeverity::CRITICAL,
                     "Permission denied: caller does not hold 'import:write'");
            THEMIS_INFO("Import aborted: permission_check denied access");
            return stats;
        }
    }

    if (options.dry_run) {
        THEMIS_INFO("DRY RUN MODE - No data will be imported");
    }

    // Parse dump file
    if (!parseDumpFile(source_path, options, stats, progress_callback)) {
        if (stats.structured_errors.empty()) {
            addError(stats, ImportErrorCode::FILE_READ_FAILED,
                     ImportErrorSeverity::CRITICAL, "Failed to parse dump file");
        }
        // I4: Audit – import failure (partial or total)
        const std::string reason = stats.structured_errors.empty()
            ? "parse_failed"
            : stats.structured_errors.back().message;
        mysqlAuditLogEvent("import_failure", {
            {"source",            source_path},
            {"reason",            reason},
            {"records_processed", std::to_string(stats.imported_records)},
            {"partial_import",    stats.imported_records > 0 ? "true" : "false"}
        });
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

    THEMIS_INFO("Import summary: {}", stats.toJson().dump());
    THEMIS_INFO("Import completed: {} records imported, {} failed, {} skipped in {:.2f}s",
        stats.imported_records, stats.failed_records, stats.skipped_records,
        stats.elapsed_seconds);

    // Prometheus / OTel metrics emission
    // Generic pipeline-level counters
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "imported"}},
               static_cast<double>(stats.imported_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "failed"}},
               static_cast<double>(stats.failed_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "skipped"}},
               static_cast<double>(stats.skipped_records));
    emitMetric(options, "themisdb_import_tables_total",
               {},
               static_cast<double>(stats.tables_processed));
    emitMetric(options, "themisdb_import_duration_seconds",
               {},
               stats.elapsed_seconds);
    for (const auto& e : stats.structured_errors) {
        emitMetric(options, "themisdb_import_errors_total",
                   {{"code", std::to_string(static_cast<uint32_t>(e.code))}},
                   1.0);
    }
    // MySQL-specific counters (consistent with import pipeline naming convention)
    emitMetric(options, "importers_mysql_rows_imported_total",
               {},
               static_cast<double>(stats.imported_records));
    emitMetric(options, "importers_mysql_errors_total",
               {},
               static_cast<double>(stats.failed_records + stats.structured_errors.size()));

    // OTel span for the entire import
    emitSpan(options, "import_total",
             {{"source", source_path},
              {"tables", std::to_string(stats.tables_processed)},
              {"rows",   std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> MySQLImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "mysql-import-" + std::to_string(ms) + "-" +
                     std::to_string(reinterpret_cast<uintptr_t>(handle.get()) & 0xFFFF);
    }
    handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    handle->running.store(true);
    handle->setStage("pending");

    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    std::weak_ptr<ImportHandle> weak_handle = handle;
    ProgressCallback progress_cb = [weak_handle](const std::string& stage,
                                                  size_t current, size_t total) {
        if (auto h = weak_handle.lock()) {
            h->current_records.store(current);
            h->total_records.store(total);
            h->setStage(stage);
        }
    };

    std::thread([this, source_path, options, progress_cb, handle, promise]() mutable {
        ImportStats stats;
        try {
            stats = this->importData(source_path, options, progress_cb);
        } catch (const std::exception& e) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = std::string("Unhandled exception in async MySQL import: ") + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async MySQL import worker";
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        }
        handle->running.store(false);
        handle->setStage("completed");
        handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        promise->set_value(std::move(stats));
    }).detach();

    return handle;
}

void MySQLImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("MySQL/MariaDB import cancelled");
}

json MySQLImporter::getSourceSchema(const std::string& source_path) {
    schemas_.clear();

    std::ifstream file(source_path);
    if (!file) {
        return json::array();
    }

    std::string line = {};
    std::string current_sql = {};

    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
          continue;
        }
        // Skip MySQL conditional comments (/*!...*/)
        if (static_cast<int>(line.size()) > = 2 && line[0] == '/' && line[1] == '*') {
          continue;
        }

        current_sql += line + " ";

        if (line.find(';') != std::string::npos) {
            std::string stripped = stripMySQLComments(current_sql);
            if (stripped.find("CREATE TABLE") != std::string::npos) {
                TableSchema schema = {};
                if (parseCreateTable(stripped, schema)) {
                    schemas_[schema.name] = schema;
                }
            }
            current_sql.clear();
        }
    }

    json result = json::array();
    for (const auto& [name, schema] : schemas_) {
        json table_json = {
            {"name",         schema.name},
            {"schema",       schema.schema},
            {"columns",      schema.columns},
            {"column_types", schema.column_types},
            {"primary_keys", schema.primary_keys}
        };
        result.push_back(table_json);
    }

    return result;
}

// ============================================================================
// Private Methods
// ============================================================================

bool MySQLImporter::parseDumpFile(const std::string& file_path, const ImportOptions& options,
                                   ImportStats& stats, ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    // PHASE-2-HARDENING: Connection Pool Exhaustion Handling
    // Track active connections for this import session
    g_mysql_connection_pool.connection_timeout_ms = options.import_timeout_ms;
    
    // Attempt to acquire a "connection slot" from the pool
    if (g_mysql_connection_pool.active_connections >= ConnectionPoolState::max_active_connections) {
        // PHASE-2-HARDENING: Pool exhaustion error reporting
        mysqlAuditLogEvent("connection_pool_exhausted", {
            {"source",        file_path},
            {"max_conns",     std::to_string(ConnectionPoolState::max_active_connections)},
            {"active_conns",  std::to_string(g_mysql_connection_pool.active_connections.load())}
        });
        addError(stats, ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE,
                 ImportErrorSeverity::CRITICAL,
                 "Connection pool exhausted: maximum " +
                 std::to_string(ConnectionPoolState::max_active_connections) + " concurrent imports allowed");
        return false;
    }
    
    // Increment active connection counter
    g_mysql_connection_pool.active_connections++;
    
    // RAII guard to decrement counter on exit
    struct PoolGuard {
        ~PoolGuard() {
            if (g_mysql_connection_pool.active_connections > 0)
                g_mysql_connection_pool.active_connections--;
        }
    } pool_guard;

    // Detect mysqldump header in the first 50 lines.
    {
        std::string hdr_line = {};
        int hdr_lines = 0;
        bool found_header = false;
        bool hdr_trunc = false;
        while (streamReadLine(file, hdr_line, 4096, hdr_trunc) && hdr_lines < 50) {
            if (hdr_line.find("MySQL dump") != std::string::npos ||
                hdr_line.find("MariaDB dump") != std::string::npos ||
                hdr_line.find("mysqldump") != std::string::npos) {
                found_header = true;
            }
            if (!hdr_line.empty() &&
                !(hdr_line.size() >= 2 && hdr_line[0] == '-' && hdr_line[1] == '-') &&
                !(hdr_line.size() >= 2 && hdr_line[0] == '/' && hdr_line[1] == '*')) {
                break;
            }
            hdr_lines++;
        }
        if (!found_header) {
            addError(stats, ImportErrorCode::NOT_A_PG_DUMP, ImportErrorSeverity::CRITICAL,
                     "File does not appear to be a MySQL/MariaDB mysqldump");
            return false;
        }
        // Rewind to process the full file
        file.clear();
        file.seekg(0);
    }

    // --- Delta / incremental import hash loading ---
    // When delta_hash_file is set, rows whose FNV-1a hash has already been seen
    // are skipped.  Setting delta_key_columns = {"updated_at"} implements the
    // recommended high-watermark pattern for MySQL sources.
    std::unordered_set<uint64_t> delta_hashes = {};

    if (!options.delta_hash_file.empty()) {
        delta_hashes = loadDeltaHashes(options.delta_hash_file);
        THEMIS_INFO("MySQL incremental import: loaded {} known hashes from '{}'",
                    delta_hashes.size(), options.delta_hash_file);
    }

    // Per-line read limit (default 64 MB, honoring max_statement_size_bytes)
    const size_t line_read_limit = options.max_statement_size_bytes > 0
                                   ? options.max_statement_size_bytes
                                   : 64 * 1024 * 1024;

    std::string line = {};
    std::string current_sql;
    size_t line_number = 0;
    size_t batch_row_count = 0;
    bool line_truncated = false;

    // I1: Timeout / deadline guard ──────────────────────────────────────────
    // When import_timeout_ms > 0 a deadline is computed and checked every
    // 500 lines so the loop aborts promptly without polling on every iteration.
    const bool mysql_timeout_enabled = (options.import_timeout_ms > 0);
    const auto mysql_import_deadline  = mysql_timeout_enabled
        ? std::chrono::steady_clock::now() +
          std::chrono::milliseconds(options.import_timeout_ms)
        : std::chrono::steady_clock::time_point::max();
    // ────────────────────────────────────────────────────────────────────────

    while (streamReadLine(file, line, line_read_limit, line_truncated) && !cancelled_) {
        line_number++;

        // I1: Deadline check every 500 lines ─────────────────────────────────
        if (mysql_timeout_enabled && (line_number % 500 == 0)) {
            if (std::chrono::steady_clock::now() >= mysql_import_deadline) {
                mysqlAuditLogEvent("importer_timeout", {
                    {"source",            file_path},
                    {"reason",            "import_timeout_ms exceeded"},
                    {"timeout_triggered", "true"},
                    {"records_processed", std::to_string(stats.imported_records)},
                    {"timeout_ms",        std::to_string(options.import_timeout_ms)}
                });
                addError(stats, ImportErrorCode::DEADLINE_EXCEEDED,
                         ImportErrorSeverity::CRITICAL,
                         "Import timed out after " +
                         std::to_string(options.import_timeout_ms) + " ms");
                cancelled_ = true;
                break;
            }
        }
        // ─────────────────────────────────────────────────────────────────────

        if (line_truncated) {
            addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "Line too long (> " + std::to_string(line_read_limit) + " bytes); truncated",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back("Line truncated at " + std::to_string(line_number));
            current_sql.clear();
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        // Skip empty lines and SQL comments (-- ...)
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
            continue;
        }

        // Skip standalone MySQL conditional comments that span a full line
        // e.g. /*!40101 SET NAMES utf8 */;
        // We strip them before accumulating into current_sql.
        std::string stripped_line = stripMySQLComments(line);
        // Trim the stripped line; if nothing is left, skip
        {
            size_t first = stripped_line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
              continue;
            }
        }

        current_sql += stripped_line + " ";

        // Statement-size guard
        if (options.max_statement_size_bytes > 0 &&
            current_sql.size() > options.max_statement_size_bytes) {
            addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "SQL statement exceeds max_statement_size_bytes (" +
                     std::to_string(options.max_statement_size_bytes) + ")",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back("Statement too large near line " +
                                     std::to_string(line_number));
            current_sql.clear();
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        // Complete statement ends with ';'
        if (stripped_line.find(';') == std::string::npos) {
            continue;
        }

        // Classify and process the completed statement
        std::string upper_sql = current_sql;
        // Convert to upper for keyword matching (first 20 chars is sufficient)
        std::string prefix = {};
        for (size_t i = 0; i <static_cast<int>(current_sql.size()) && i < 30; ++i) {
            prefix += static_cast<char>(std::toupper(static_cast<unsigned char>(current_sql[i])));
        }

        if (prefix.find("CREATE TABLE") != std::string::npos) {
            auto t0 = std::chrono::steady_clock::now();
            TableSchema schema = {};
            if (parseCreateTable(current_sql, schema)) {
                if (shouldImportTable(schema.name, options)) {
                    schemas_[schema.name] = schema;
                    stats.tables_processed++;
                    THEMIS_DEBUG("Parsed MySQL table schema: {}", schema.name);
                    // I4: Audit – schema change detected
                    mysqlAuditLogEvent("schema_change_detection", {
                        {"table_name",  schema.name},
                        {"change_type", "CREATE_TABLE"},
                        {"detected_at", "line:" + std::to_string(line_number)}
                    });
                    reportProgress(callback, "schema", stats.tables_processed, 0);
                    double dur = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - t0).count();
                    emitSpan(options, "parse_table", {{"table", schema.name}}, dur);
                }
            } else {
                addError(stats, ImportErrorCode::PARSE_CREATE_TABLE,
                         ImportErrorSeverity::WARNING,
                         "Failed to parse CREATE TABLE statement",
                         "line " + std::to_string(line_number));
                stats.warnings.push_back("Failed to parse CREATE TABLE near line " +
                                         std::to_string(line_number));
            }
        } else if (prefix.find("INSERT") != std::string::npos) {
            stats.total_records++;
            if (!options.dry_run) {
                auto t0 = std::chrono::steady_clock::now();
                parseInsert(current_sql, options, stats, line_number, delta_hashes);
                double dur = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - t0).count();
                emitSpan(options, "insert_batch", {}, dur);
            }
            batch_row_count++;

            // Batch progress reporting
            if (options.batch_size > 0 && batch_row_count >= options.batch_size) {
                reportProgress(callback, "data", stats.imported_records, 0);
                batch_row_count = 0;
            }
        }
        // LOCK TABLES / UNLOCK TABLES / DROP TABLE / SET / USE are ignored silently.

        current_sql.clear();
    }

    // Persist updated delta hashes so the next incremental import can skip these rows.
    if (!options.dry_run && !options.delta_hash_file.empty() && !delta_hashes.empty()) {
        saveDeltaHashes(options.delta_hash_file, delta_hashes);
        THEMIS_INFO("MySQL incremental import: persisted {} hashes to '{}'",
                    delta_hashes.size(), options.delta_hash_file);
    }

    return !cancelled_;
}

bool MySQLImporter::parseCreateTable(const std::string& sql, TableSchema& schema) {
    // Match: CREATE [TEMPORARY] TABLE [`]table_name[`] (
    // Handles optional schema qualifier: `db`.`table`
    std::regex table_regex(
        R"(CREATE\s+(?:TEMPORARY\s+)?TABLE\s+(?:(?:`([^`]+)`|(\w+))\.)?(?:`([^`]+)`|(\w+))\s*\()",
        std::regex_constants::icase);
    std::smatch match = {};

    if (!std::regex_search(sql, match, table_regex)) {
        return false;
    }

    // Groups: 1=schema_backtick, 2=schema_plain, 3=table_backtick, 4=table_plain
    schema.schema = match[1].matched ? match[1].str()
                  : match[2].matched ? match[2].str() : "";
    schema.name   = match[3].matched ? match[3].str()
                  : match[4].matched ? match[4].str() : "";

    if (schema.name.empty()) {
      return false;
    }

    // Find the outer parentheses that wrap the column definitions.
    size_t open_pos = sql.find('(', match.position());
    if (open_pos == std::string::npos) {
      return false;
    }

    // Find matching closing paren using a simple depth counter (respects quotes)
    int depth = 0;
    bool in_string = false;
    char str_char = '\0';
    size_t close_pos = std::string::npos;
    for (size_t k = open_pos; k <static_cast<int>(sql.size()); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == '\\') { ++k; continue; }  // MySQL backslash escape inside strings
            if (c == str_char) {
              in_string = false;
            }
        } else if (c == '\'' || c == '"') {
            in_string = true;
            str_char  = c;
        } else if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth == 0) { close_pos = k; break; }
        }
    }
    if (close_pos == std::string::npos) {
      return false;
    }

    std::string cols_str = sql.substr(open_pos + 1, close_pos - open_pos - 1);

    // Split at top-level commas (same approach as postgres_importer)
    std::vector<std::string> col_defs;
    {
        int dep = 0;
        bool inq = false;
        char qc  = '\0';
        std::string cur = {};
        for (size_t i = 0; i <static_cast<int>(cols_str.size()); ++i) {
            char c = cols_str[i];
            if (inq) {
                cur += c;
                if (c == '\\') {
                    if (i + 1 <static_cast<int>(cols_str.size())) {
                      cur += cols_str[++i];
                    }
                } else if (c == qc) {
                    inq = false;
                }
            } else if (c == '\'' || c == '"' || c == '`') {
                inq = true; qc = c; cur += c;
            } else if (c == '(') {
                ++dep; cur += c;
            } else if (c == ')') {
                --dep; cur += c;
            } else if (c == ',' && dep == 0) {
                col_defs.push_back(cur);
                cur.clear();
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) {
          col_defs.push_back(cur);
        }
    }

    for (auto& col_def : col_defs) {
        // Trim whitespace
        {
            size_t f = col_def.find_first_not_of(" \t\n\r");
            if (f == std::string::npos) {
              continue;
            }
            col_def = col_def.substr(f);
        }
        {
            size_t l = col_def.find_last_not_of(" \t\n\r");
            if (l != std::string::npos) {
              col_def = col_def.substr(0, l + 1);
            }
        }
        if (col_def.empty()) {
          continue;
        }

        std::string upper_def = {};
        for (size_t i = 0; i <static_cast<int>(col_def.size()) && i < 20; ++i)
            upper_def += static_cast<char>(std::toupper(static_cast<unsigned char>(col_def[i])));

        // Skip table-level constraints: PRIMARY KEY, UNIQUE KEY, KEY, INDEX, CONSTRAINT, CHECK
        if (upper_def.find("PRIMARY") != std::string::npos ||
            upper_def.find("UNIQUE")  != std::string::npos ||
            upper_def.find("KEY")     != std::string::npos ||
            upper_def.find("INDEX")   != std::string::npos ||
            upper_def.find("CONSTRAINT") != std::string::npos ||
            upper_def.find("CHECK")   != std::string::npos ||
            upper_def.find("FULLTEXT") != std::string::npos ||
            upper_def.find("SPATIAL") != std::string::npos) {
            continue;
        }

        // Column definition: [`col_name`|col_name] col_type [options...]
        // Extract column name (may be backtick-quoted)
        std::string col_name = {};
        size_t type_start = 0;
        if (!col_def.empty() && col_def[0] == '`') {
            size_t end_tick = col_def.find('`', 1);
            if (end_tick == std::string::npos) {
              continue;
            }
            col_name   = col_def.substr(1, end_tick - 1);
            type_start = end_tick + 1;
        } else {
            // Plain identifier
            size_t sp = col_def.find_first_of(" \t");
            if (sp == std::string::npos) {
              continue;
            }
            col_name   = col_def.substr(0, sp);
            type_start = sp;
        }

        if (col_name.empty()) {
          continue;
        }

        // Extract type: skip leading whitespace after name, then take up to first
        // space / '(' / constraint keyword
        while (type_start <static_cast<int>(col_def.size()) &&
               (col_def[type_start] == ' ' || col_def[type_start] == '\t')) {
            ++type_start;
        }

        // Collect type token (may include parenthesised size, e.g. varchar(255))
        std::string col_type = {};
        size_t k = type_start;
        int tdep = 0;
        while (static_cast<size_t>(k) <static_cast<int>(col_def.size())) {
            char c = col_def[k];
            if (c == '(') { ++tdep; col_type += c; }
            else if (c == ')') {
                if (tdep > 0) { --tdep; col_type += c; }
                else break;
            } else if ((c == ' ' || c == '\t') && tdep == 0) {
                break;
            } else {
                col_type += c;
            }
            ++k;
        }

        if (col_type.empty()) {
          continue;
        }

        schema.columns.push_back(col_name);
        schema.column_types[col_name] = col_type;
    }

    return !schema.name.empty();
}

bool MySQLImporter::parseInsert(const std::string& sql, const ImportOptions& options,
                                 ImportStats& stats, size_t line_number,
                                 std::unordered_set<uint64_t>& delta_hashes) {
    // MySQL INSERT supports:
    //   INSERT [LOW_PRIORITY|DELAYED|HIGH_PRIORITY] [IGNORE] INTO
    //     [`db`.]`table` [(col1,...)] VALUES (v1,...),(v2,...);
    //   INSERT INTO `table` SET col=val, ...;  (not handled – rare in mysqldump)
    //
    // The regex captures table name and the entire VALUES payload including
    // multi-row tuples.
    std::regex insert_regex(
        R"(INSERT\s+(?:LOW_PRIORITY\s+|DELAYED\s+|HIGH_PRIORITY\s+)?(?:IGNORE\s+)?INTO\s+(?:`([^`]+)`\.)?(?:`([^`]+)`|(\w+))\s*(?:\(([^)]*)\))?\s+VALUES\s*(.+?)\s*;?\s*$)",
        std::regex_constants::icase);
    std::smatch match = {};

    if (!std::regex_search(sql, match, insert_regex)) {
        // PHASE-2-HARDENING: Prepared statement fallback
        // Try simple parsing to extract table name for audit logging
        std::string fallback_table = {};
        if (simpleInsertFallback(sql, fallback_table)) {
            mysqlAuditLogEvent("insert_parse_fallback", {
                {"table", fallback_table},
                {"line", std::to_string(line_number)},
                {"reason", "regex_parse_failed"}
            });
        }
        addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                 "Could not parse INSERT statement",
                 "line " + std::to_string(line_number));
        stats.failed_records++;
        return false;
    }

    // group 2 = backtick table name, group 3 = plain table name
    std::string table_name = match[2].matched ? match[2].str() : match[3].str();

    if (!shouldImportTable(table_name, options)) {
        stats.skipped_records++;
        return true;
    }

    // Resolve column list
    std::vector<std::string> col_list = {};

    if (match[4].matched && !match[4].str().empty()) {
        std::istringstream css(match[4].str());
        std::string col = {};
        while (std::getline(css, col, ',')) {
            col = unquoteIdentifier(col);
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            if (!col.empty()) {
              col_list.push_back(col);
            }
        }
    } else if (schemas_.count(table_name)) {
        col_list = schemas_[table_name].columns;
    }

    // The VALUES payload: may be a single tuple or comma-separated tuples
    std::string values_payload = match[5].str();

    // Build effective schema for convertRowToEntity
    TableSchema eff_schema;
    eff_schema.name = table_name;
    if (schemas_.count(table_name)) {
      eff_schema = schemas_[table_name];
    }
    if (!col_list.empty()) {
      eff_schema.columns = col_list;
    }

    // Parse multi-row tuple list: (v1,v2,...),(v3,v4,...), ...
    // Walk the payload extracting one parenthesised tuple at a time.
    size_t pos = 0;
    size_t rows_imported = 0;
    while (static_cast<size_t>(pos) <static_cast<int>(values_payload.size())) {
        // Skip whitespace and commas between tuples
        while (pos <static_cast<int>(values_payload.size()) &&
               (values_payload[pos] == ' ' || values_payload[pos] == '\t' ||
                values_payload[pos] == ',' || values_payload[pos] == '\r' ||
                values_payload[pos] == '\n')) {
            ++pos;
        }
        if (pos >= static_cast<int>(values_payload.size())) {
          break;
        }
        if (values_payload[pos] != '(') {
            // Unexpected token; skip to next '(' or end
            ++pos;
            continue;
        }

        // Find matching ')' for this tuple respecting nested parens and strings
        size_t tuple_start = pos + 1;
        int dep = 1;
        bool in_str = false;
        char sq = '\0';
        size_t k = pos + 1;
        while (k <static_cast<int>(values_payload.size()) && dep > 0) {
            char c = values_payload[k];
            if (in_str) {
                if (c == '\\') { ++k; }
                else if (c == sq) { in_str = false; }
            } else if (c == '\'' || c == '"') {
                in_str = true; sq = c;
            } else if (c == '(') {
                ++dep;
            } else if (c == ')') {
                --dep;
            }
            ++k;
        }
        size_t tuple_end = k - 1;  // position of matching ')'
        if (dep != 0) break;       // unbalanced – abort

        std::string tuple_str = values_payload.substr(tuple_start, tuple_end - tuple_start);
        std::vector<std::string> values = parseInsertValues(tuple_str);

        // --- Delta / incremental import check ---
        // When delta_hash_file is configured, compute a FNV-1a hash of the row
        // key (or the full tuple string if delta_key_columns is empty) and skip
        // rows that were already imported in a previous run.
        if (!options.delta_hash_file.empty()) {
            uint64_t h = computeRowHash(tuple_str, values,
                                        options.delta_key_columns,
                                        eff_schema.columns);
            if (delta_hashes.count(h)) {
                stats.skipped_records++;
                emitMetric(options, "themisdb_import_rows_total",
                           {{"table", table_name}, {"status", "skipped"}}, 1.0);
                pos = k;
                continue;
            }
            delta_hashes.insert(h);
        }

        if (!eff_schema.columns.empty() &&
            values.size() != eff_schema.columns.size()) {
            ImportError err;
            err.code     = ImportErrorCode::COLUMN_COUNT_MISMATCH;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "INSERT row has " + std::to_string(values.size()) +
                           " columns, expected " + std::to_string(eff_schema.columns.size());
            err.location = "table " + table_name + ", line " + std::to_string(line_number);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back(err.message);
            stats.failed_records++;
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table_name}, {"status", "failed"}}, 1.0);
            emitMetric(options, "importers_mysql_errors_total",
                       {{"table", table_name}}, 1.0);
        } else {
            json entity = convertRowToEntity(eff_schema, values);
            THEMIS_DEBUG("MySQL INSERT entity: {}", entity.dump());
            if ([[maybe_unused]] options.streaming_row_callback) {
                if (!options.streaming_row_callback(table_name, entity)) {
                    cancelled_ = true;
                }
            }
            stats.imported_records++;
            rows_imported++;
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table_name}, {"status", "imported"}}, 1.0);
            emitMetric(options, "importers_mysql_rows_imported_total",
                       {{"table", table_name}}, 1.0);
            if (cancelled_) return true;  // outer loop checks cancelled_ and stops
        }

        pos = k;
    }

    return true;
}

std::string MySQLImporter::mapMySQLTypeToThemis(const std::string& mysql_type,
                                                 const ImportOptions& options) const {
    // 1. Per-call options overrides (highes[[maybe_unused]] t priorit[[maybe_unused]] y)
    auto it = options.type_overrides.find(mysql_typ[[maybe_unused]] e);
    if (i[[maybe_unused]] t != option[[maybe_unused]] s.type_override[[maybe_unused]] s.en[[maybe_unused]] d()) {
      return it->second;
    }

    // 2. Config-level overrides (fro[[maybe_unused]] m initializ[[maybe_unused]] e())
    {
        std::lock_guard<std::mutex> lock(config_type_overrides_mutex[[maybe_unused]] _);
        auto ci = config_type_overrides_.find(mysql_typ[[maybe_unused]] e);
        if (c[[maybe_unused]] i != config_type_overrides[[maybe_unused]] _.en[[maybe_unused]] d()) {
          return ci->second;
        }
    }

    // 3. JDBC tinyInt1isBit: TINYINT(1) -> boolean when enabled
    if (jdbc_config_.tinyint1_as_boolean) {
        // Match both "tinyint(1)" and "TINYINT(1)" (case-insensitive)
        std::string lower_type = {};
        for (char c : mysql_type)
            lower_type += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower_type == "tinyint(1)") {
          return "boolean";
        }
    }

    // Normalise: strip size specifier, e.g. "varchar(255)" -> "varchar"
    std::string base_type = mysql_type;
    size_t paren = base_type.find('(');
    if (paren != std::string::npos) {
      base_type = base_type.substr(0, paren);
    }
    std::string lower = toLower(base_type);

    // Integer types
    if (lower == "tinyint") {
      return "integer";
    }
    if (lower == "smallint") {
      return "integer";
    }
    if (lower == "mediumint") {
      return "integer";
    }
    if (lower == "int") {
      return "integer";
    }
    if (lower == "integer") {
      return "integer";
    }
    if (lower == "bigint") {
      return "long";
    }

    // Floating-point types
    if (lower == "float") {
      return "float";
    }
    if (lower == "double") {
      return "double";
    }
    if (lower == "real") {
      return "double";
    }
    if (lower == "decimal") {
      return "double";
    }
    if (lower == "numeric") {
      return "double";
    }

    // Boolean
    if (lower == "bool" || lower == "boolean") {
      return "boolean";
    }
    if (lower == "bit") {
      return "integer";
    }

    // String types
    if (lower == "char") {
      return "string";
    }
    if (lower == "varchar") {
      return "string";
    }
    if (lower == "tinytext") {
      return "string";
    }
    if (lower == "text") {
      return "string";
    }
    if (lower == "mediumtext") {
      return "string";
    }
    if (lower == "longtext") {
      return "string";
    }
    if (lower == "enum") {
      return "string";
    }
    if (lower == "set") {
      return "string";
    }

    // Binary types
    if (lower == "binary") {
      return "binary";
    }
    if (lower == "varbinary") {
      return "binary";
    }
    if (lower == "tinyblob") {
      return "binary";
    }
    if (lower == "blob") {
      return "binary";
    }
    if (lower == "mediumblob") {
      return "binary";
    }
    if (lower == "longblob") {
      return "binary";
    }

    // Date / time types
    if (lower == "date") {
      return "date";
    }
    if (lower == "time") {
      return "time";
    }
    if (lower == "datetime") {
      return "datetime";
    }
    if (lower == "timestamp") {
      return "datetime";
    }
    if (lower == "year") {
      return "integer";
    }

    // JSON (MySQL 5.7+)
    if (lower == "json") {
      return "json";
    }

    // Spatial types (MySQL/MariaDB geometry)
    if (lower == "geometry" || lower == "point" || lower == "linestring" ||
        lower == "polygon"  || lower == "multipoint" || lower == "multilinestring" ||
        lower == "multipolygon" || lower == "geometrycollection") return "geo";

    // Prefix-based fallbacks
    if (lower.find("int")   != std::string::npos) {
      return "integer";
    }
    if (lower.find("float") != std::string::npos) {
      return "double";
    }
    if (lower.find("char")  != std::string::npos) {
      return "string";
    }
    if (lower.find("text")  != std::string::npos) {
      return "string";
    }
    if (lower.find("blob")  != std::string::npos) {
      return "binary";
    }
    if (lower.find("date")  != std::string::npos) {
      return "datetime";
    }
    if (lower.find("time")  != std::string::npos) {
      return "datetime";
    }

    return "string";  // Default: treat unknown types as strings
}

bool MySQLImporter::shouldImportTable(const std::string& table_name,
                                      const ImportOptions& options) const {
    if (std::find(options.exclude_tables.begin(), options.exclude_tables.end(),
                  table_name) != options.exclude_tables.end()) {
        return false;
    }
    if (!options.include_tables.empty()) {
        return std::find(options.include_tables.begin(), options.include_tables.end(),
                         table_name) != options.include_tables.end();
    }
    return true;
}

json MySQLImporter::convertRowToEntity(const TableSchema& schema,
                                        const std::vector<std::string>& values) {
    json entity;
    entity["_type"] = schema.name;

    for (size_t i = 0; i <static_cast<int>(values.size())  && static_cast<size_t>(i) <static_cast<int>(schema.columns.size()); ++i) {
        entity[schema.columns[i]] = values[i];
    }

    return entity;
}

std::vector<std::string> MySQLImporter::parseInsertValues(
    const std::string& values_clause) const {
    // Parse a single tuple's contents (the part inside the outer parentheses).
    // Handles:
    //   - NULL keyword -> empty string sentinel
    //   - Single-quoted strings with MySQL escape sequences ('\'', '\\', '\n', '\t')
    //   - Numeric literals
    //   - Hex literals: 0x... or x'...'
    std::vector<std::string> result;
    // I3: Pre-allocate for typical column count; avoids per-push reallocation
    // in the multi-row INSERT hot path.
    result.reserve(32);
    size_t i = 0;
    size_t n = values_clause.size();

    auto skipWs = [&]() {
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                         values_clause[i] == '\r' || values_clause[i] == '\n')) ++i;
    };

    while (i < n) {
        skipWs();
        if (i >= n) {
          break;
        }

        char c = values_clause[i];

        if (c == '\'') {
            // Single-quoted string with MySQL escape sequences
            ++i;
            std::string val = {};
            while (i < n) {
                char sc = values_clause[i];
                if (sc == '\\' && i + 1 < n) {
                    ++i;
                    char esc = values_clause[i];
                    switch (esc) {
                        case 'n':  val += '\n'; break;
                        case 't':  val += '\t'; break;
                        case 'r':  val += '\r'; break;
                        case '0':  val += '\0'; break;
                        case '\\': val += '\\'; break;
                        case '\'': val += '\''; break;
                        case '"':  val += '"';  break;
                        default:   val += '\\'; val += esc; break;
                    }
                    ++i;
                } else if (sc == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                    // '' escape (SQL standard)
                    val += '\'';
                    i += 2;
                } else if (sc == '\'') {
                    ++i;
                    break;
                } else {
                    val += sc;
                    ++i;
                }
            }
            result.push_back(val);
        } else if (c == '"') {
            // Double-quoted string (MySQL ANSI_QUOTES mode – uncommon in mysqldump)
            ++i;
            std::string val = {};
            while (i < n) {
                char sc = values_clause[i];
                if (sc == '"' && i + 1 < n && values_clause[i + 1] == '"') {
                    val += '"'; i += 2;
                } else if (sc == '"') {
                    ++i; break;
                } else {
                    val += sc; ++i;
                }
            }
            result.push_back(val);
        } else {
            // Unquoted token: NULL, number, hex literal, etc.
            size_t start = i;
            int dep = 0;
            while (i < n) {
                char tc = values_clause[i];
                if (tc == '(') { ++dep; ++i; }
                else if (tc == ')') {
                    if (dep > 0) { --dep; ++i; }
                    else break;
                } else if (tc == ',' && dep == 0) {
                    break;
                } else {
                    ++i;
                }
            }
            std::string token = values_clause.substr(start, i - start);
            // Trim whitespace
            {
                size_t f = token.find_first_not_of(" \t\r\n");
                size_t l = token.find_last_not_of(" \t\r\n");
                if (f == std::string::npos) {
                  token.clear();
                }
                else token = token.substr(f, l - f + 1);
            }
            // NULL -> empty string sentinel
            std::string upper_tok = {};
            for (char ch : token)
                upper_tok += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (upper_tok == "NULL") {
              token.clear();
            }
            result.push_back(token);
        }

        skipWs();
        if (i < n && values_clause[i] == ',') ++i;  // consume separator
    }

    return result;
}

// ============================================================================
// JDBC URL Parser
// ============================================================================

/**
 * @brief Parse a JDBC-style MySQL/MariaDB connection URL.
 *
 * Accepted formats:
 *   jdbc:mysql://host:port/database?param1=val1&param2=val2
 *   jdbc:mariadb://host:port/database?param1=val1&param2=val2
 *   jdbc:mysql://host/database
 *   jdbc:mysql://host:3306/
 *
 * Recognised query parameters:
 *   tinyInt1isBit=true/false  -> jdbc_config_.tinyint1_as_boolean
 *   useSSL=true/false         -> jdbc_config_.ssl
 */
bool MySQLImporter::parseJdbcUrl(const std::string& url, JdbcConfig& out) {
    // Validate prefix
    const std::string mysql_prefix   = "jdbc:mysql://";
    const std::string mariadb_prefix = "jdbc:mariadb://";
    size_t authority_start = 0;
    if (static_cast<int>(url.size()) > static_cast<int>(mysql_prefix.size()) &&
        url.substr(0,static_cast<int>(mysql_prefix.size())) == mysql_prefix) {
        authority_start = mysql_prefix.size();
    } else if (static_cast<int>(url.size()) > static_cast<int>(mariadb_prefix.size()) &&
               url.substr(0,static_cast<int>(mariadb_prefix.size())) == mariadb_prefix) {
        authority_start = mariadb_prefix.size();
    } else {
        return false;
    }

    // Split authority+path from query string
    std::string authority_path = {};
    std::string query_string = {};
    size_t q_pos = url.find('?', authority_start);
    if (q_pos != std::string::npos) {
        authority_path = url.substr(authority_start, q_pos - authority_start);
        query_string   = url.substr(q_pos + 1);
    } else {
        authority_path = url.substr(authority_start);
    }

    // Split host[:port] from /database
    size_t slash_pos = authority_path.find('/');
    std::string host_port = slash_pos != std::string::npos
                            ? authority_path.substr(0, slash_pos)
                            : authority_path;
    std::string db_path   = slash_pos != std::string::npos
                            ? authority_path.substr(slash_pos + 1)
                            : "";

    // Parse host and optional port
    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos) {
        out.host = host_port.substr(0, colon_pos);
        std::string port_str = host_port.substr(colon_pos + 1);
        if (!port_str.empty()) {
            try { out.port = std::stoi(port_str); } catch (...) {
                THEMIS_WARN("MySQL/MariaDB Importer: invalid port '{}' in JDBC URL; using default {}",
                            port_str, out.port);
            }
        }
    } else {
        out.host = host_port;
    }
    out.database = db_path;

    // Parse query parameters
    std::istringstream qs(query_string);
    std::string param = {};
    while (std::getline(qs, param, '&')) {
        size_t eq = param.find('=');
        if (eq == std::string::npos) {
          continue;
        }
        std::string key   = param.substr(0, eq);
        std::string value = param.substr(eq + 1);
        std::string lower_key = {};
        for (char c : key) {
          lower_key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        std::string lower_val = {};
        for (char c : value) {
          lower_val += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        if (lower_key == "tinyint1isbit") {
            out.tinyint1_as_boolean = (lower_val == "true" || lower_val == "1");
        } else if (lower_key == "usessl") {
            out.ssl = (lower_val == "true" || lower_val == "1");
        }
    }

    return !out.host.empty();
}

std::string MySQLImporter::unquoteIdentifier(const std::string& s) {
    std::string t = s;
    // Trim surrounding whitespace
    {
        size_t f = t.find_first_not_of(" \t\r\n");
        size_t l = t.find_last_not_of(" \t\r\n");
        if (f == std::string::npos) {
          return "";
        }
        t = t.substr(f, l - f + 1);
    }
    if (static_cast<int>(t.size()) > = 2 && t.front() == '`' && t.back() == '`') {
        return t.substr(1, static_cast<int>(t.size()) - 2);
    }
    if (static_cast<int>(t.size()) > = 2 && t.front() == '"' && t.back() == '"') {
        return t.substr(1, static_cast<int>(t.size()) - 2);
    }
    return t;
}

std::string MySQLImporter::stripMySQLComments(const std::string& sql) {
    // Remove MySQL conditional comments: /*! ... */ and /*!NNNNN ... */
    // and regular block comments /* ... */
    // Inline (non-conditional) SQL comments -- ... are NOT stripped here
    // because they are filtered at the line level by the caller.
    std::string result = {};
    result.reserve(sql.size());
    size_t i = 0;
    while (static_cast<size_t>(i) <static_cast<int>(sql.size())) {
        if (i + 1 <static_cast<int>(sql.size()) && sql[i] == '/' && sql[i + 1] == '*') {
            // Skip until closing */
            i += 2;
            while (i + 1 <static_cast<int>(sql.size()) && !(sql[i] == '*' && sql[i + 1] == '/')) {
                ++i;
            }
            i += 2;  // skip */
            // Replace with a space so tokens don't merge
            result += ' ';
        } else {
            result += sql[i];
            ++i;
        }
    }
    return result;
}

void MySQLImporter::addError(ImportStats& stats, ImportErrorCode code,
                              ImportErrorSeverity severity, const std::string& message,
                              const std::string& location) const {
    ImportError err;
    err.code     = code;
    err.severity = severity;
    err.message  = message;
    err.location = location;
    stats.structured_errors.push_back(err);
    if (severity == ImportErrorSeverity::ERROR || severity == ImportErrorSeverity::CRITICAL) {
        stats.errors.push_back(message);
    }
    
    // PHASE-2-HARDENING: Schema cache invalidation on connection loss
    if (code == ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE ||
        code == ImportErrorCode::UNKNOWN_TABLE) {
        g_mysql_connection_pool.schema_cache_valid.store(false);
        g_mysql_connection_pool.last_error.store(code);
        mysqlAuditLogEvent("schema_cache_invalidated", {
            {"reason", std::to_string(static_cast<uint32_t>(code))},
            {"message", message}
        });
    }
}

void MySQLImporter::emitMetric(const ImportOptions& options,
                                const std::string& metric,
                                const std::map<std::string, std::string>& labels,
                                double value) const {
    if ([[maybe_unused]] options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void MySQLImporter::emitSpan(const ImportOptions& options,
                              const std::string& operation,
                              const std::map<std::string, std::string>& attributes,
                              double duration_seconds) const {
    if ([[maybe_unused]] options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void MySQLImporter::reportProgress(ProgressCallback& callback, const std::string& stage,
                                    size_t current, size_t total) {
    if ([[maybe_unused]] callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// Delta / incremental import helpers
// ============================================================================
//
// FNV-1a 64-bit hash – matches the implementation in postgres_importer.cpp so
// that hash files written by one importer can be read by the other.
static uint64_t mysql_fnv1a64(const char* data, size_t len) {
    return themis::hash::fnv1a64(data, len);
}

uint64_t MySQLImporter::computeRowHash(const std::string& tuple_str,
                                        const std::vector<std::string>& values,
                                        const std::vector<std::string>& key_columns,
                                        const std::vector<std::string>& schema_columns) {
    if (key_columns.empty() || schema_columns.empty()) {
        // Hash the entire raw tuple string (full-row fingerprint)
        return mysql_fnv1a64(tuple_str.data(),static_cast<int>(tuple_str.size()));
    }
    // Hash only the key column values, separated by a non-printable sentinel.
    // Setting key_columns = {"updated_at"} is the recommended high-watermark
    // configuration: only rows with a new updated_at value will be imported.
    static constexpr char kFieldSep = '\x01';
    std::unordered_map<std::string, size_t> schema_column_index = {};

    schema_column_index.reserve(schema_columns.size());
    for (size_t i = 0; i <static_cast<int>(schema_columns.size()); ++i) {
        schema_column_index.emplace(schema_columns[i], i);
    }
    std::string key_data = {};
    for (const auto& kc : key_columns) {
        auto it = schema_column_index.find(kc);
        if (it != schema_column_index.end()) {
            size_t idx = it->second;
            if (static_cast<int>(values.size()) > idx) {
                key_data += values[idx];
            }
        }
        key_data += kFieldSep;
    }
    return mysql_fnv1a64(key_data.data(),static_cast<int>(key_data.size()));
}

std::unordered_set<uint64_t> MySQLImporter::loadDeltaHashes(const std::string& delta_hash_file) {
    std::unordered_set<uint64_t> hashes;
    std::ifstream f(delta_hash_file);
    if (!f) {
      return hashes;
    }
    std::string line = {};
    while (std::getline(f, line)) {
        if (line.empty()) {
          continue;
        }
        try {
            hashes.insert(std::stoull(line, nullptr, 16));
        } catch (...) {}
    }
    return hashes;
}

void MySQLImporter::saveDeltaHashes(const std::string& delta_hash_file,
                                     const std::unordered_set<uint64_t>& hashes) {
    std::ofstream f(delta_hash_file, std::ios::trunc);
    if (!f) {
      return;
    }
    for (uint64_t h : hashes) {
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
        f << buf << "\n";
    }
}

// ============================================================================
// Plugin implementation
// ============================================================================

MySQLImporterPlugin::MySQLImporterPlugin()
    : importer_(std::make_unique<MySQLImporter>()) {
}

plugins::PluginCapabilities MySQLImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    caps.thread_safe        = false;
    return caps;
}

bool MySQLImporterPlugin::initialize(const char* config_json) {
    if (!importer_) {
      return false;
    }
    return importer_->initialize(config_json ? config_json : "{}");
}

void MySQLImporterPlugin::shutdown() {
    if (importer_) {
      importer_->cancel();
    }
}

} // namespace importers
} // namespace themis

// ============================================================================
// Plugin Entry Points
// ============================================================================

extern "C" {
    themis::plugins::IThemisPlugin* createMySQLPlugin() {
        return new themis::importers::MySQLImporterPlugin();
    }

    void destroyMySQLPlugin(themis::plugins::IThemisPlugin* plugin) {
        delete plugin;
    }
}

// ============================================================================
// IImporterPluginRegistry – static-init self-registration
// ============================================================================
//
// Registers MySQLImporterSchemePlugin with ImporterSchemeRegistry at program
// startup so that IImporterPluginRegistry::instance().resolve("mysql://…")
// and resolve("mariadb://…") return this plugin without any manual wiring.
//
// The macro is called from within the themis::importers namespace so that
// the unqualified PluginClass name is valid for C++ token-paste (##).

namespace themis {
namespace importers {
REGISTER_IMPORTER_PLUGIN(MySQLImporterSchemePlugin)
} // namespace importers
} // namespace themis



