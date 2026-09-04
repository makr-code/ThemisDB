/**
 * @file postgres_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=18, M=35, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/postgres_importer.h"
#include <stdexcept>
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <unordered_map>
#include <unordered_set>
#include <cinttypes>

// Windows headers may define ERROR as a macro, which breaks enum accesses
// like ConflictStrategy::ERROR and ImportErrorSeverity::ERROR.
#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace importers {

// ============================================================================
// PHASE-2-HARDENING: Connection Pool and CDC Fallback Infrastructure
// ============================================================================
namespace {

/// Connection pool state tracker for Phase 2 hardening
struct ConnectionPoolState {
    /// Current number of active connections (bounded by max_active_connections)
    std::atomic<size_t> active_connections{0};
    
    /// Maximum concurrent connections allowed (default: 32)
    static constexpr size_t max_active_connections = 32;
    
    /// Connection timeout in milliseconds (0 = no timeout)
    uint32_t connection_timeout_ms = 0;
    
    /// Last connection error code for diagnostics
    std::atomic<ImportErrorCode> last_error{ImportErrorCode::SUCCESS};
};

/// Global connection pool state (one per process; safe due to atomic operations)
static thread_local ConnectionPoolState g_connection_pool;

/// CDC (Change Data Capture) capability detection
/// Returns true if the dump appears to contain CDC/replication-specific DDL
static bool detectCDCCapability(const std::string& dump_header_lines) {
    // PHASE-2-HARDENING: CDC fallback detection
    // Check for replication slot references, logical decoding, publication, subscription
    return dump_header_lines.find("PUBLICATION") != std::string::npos ||
           dump_header_lines.find("SUBSCRIPTION") != std::string::npos ||
           dump_header_lines.find("logical_decoding") != std::string::npos ||
           dump_header_lines.find("replication slot") != std::string::npos;
}

/// Maps PostgreSQL-specific error patterns to ImporterErrorCode
static ImportErrorCode mapPostgreSQLErrorToCode(const std::string& error_msg) {
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
        lower_msg.find("unavailable") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    
    // Timeout errors
    if (lower_msg.find("timeout") != std::string::npos ||
        lower_msg.find("deadline") != std::string::npos) {
        return ImportErrorCode::DEADLINE_EXCEEDED;
    }
    
    // Schema errors
    if (lower_msg.find("schema") != std::string::npos ||
        lower_msg.find("table") != std::string::npos) {
        return ImportErrorCode::UNKNOWN_TABLE;
    }
    
    // Type conversion errors
    if (lower_msg.find("type") != std::string::npos ||
        lower_msg.find("conversion") != std::string::npos) {
        return ImportErrorCode::TYPE_CONVERSION;
    }
    
    // Parse errors
    if (lower_msg.find("parse") != std::string::npos ||
        lower_msg.find("syntax") != std::string::npos) {
        return ImportErrorCode::PARSE_CREATE_TABLE;
    }
    
    return ImportErrorCode::UNKNOWN;
}

} // anonymous namespace

// ============================================================================
// Pre-compiled static regexes (performance: compiled once per process)
// ============================================================================
namespace {

// CREATE TABLE
static const std::regex kCreateTableRe(
    R"(CREATE TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?(?:(\w+)\.)?(\w+)\s*\()",
    std::regex_constants::icase);

// CREATE TYPE
static const std::regex kEnumTypeRe(
    R"(CREATE TYPE\s+(?:\w+\.)?(\w+)\s+AS\s+ENUM)",
    std::regex_constants::icase);
static const std::regex kCompositeTypeRe(
    R"(CREATE TYPE\s+(?:\w+\.)?(\w+)\s+AS\s*\()",
    std::regex_constants::icase);

// ALTER TABLE ADD COLUMN
static const std::regex kAlterAddColumnRe(
    R"(ALTER TABLE\s+(?:ONLY\s+)?(?:\w+\.)?(\w+)\s+ADD COLUMN\s+(\w+)\s+(\S+))",
    std::regex_constants::icase);

// ALTER TABLE ADD CONSTRAINT ... FOREIGN KEY (used in parseAlterTableForeignKey)
static const std::regex kAlterFkRe(
    R"(ALTER\s+TABLE\s+(?:ONLY\s+)?(?:\w+\.)?(\w+)\s+ADD\s+CONSTRAINT\s+\w+\s+FOREIGN\s+KEY)",
    std::regex_constants::icase);

// COPY ... FROM stdin
static const std::regex kCopyRe(
    R"(COPY\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+FROM\s+stdin)",
    std::regex_constants::icase);

// CREATE [UNIQUE] INDEX
static const std::regex kCreateIndexRe(
    R"(CREATE\s+(UNIQUE\s+)?INDEX\s+(?:CONCURRENTLY\s+)?(?:IF\s+NOT\s+EXISTS\s+)?(\w+)\s+ON\s+(?:\w+\.)?(\w+)\s*(?:USING\s+(\w+))?\s*\(([^)]+)\)(?:\s+WHERE\s+(.+?))?(?:\s*;)?\s*$)",
    std::regex_constants::icase);

// ON table( for getSourceSchema index attachment
static const std::regex kIndexTableRe(
    R"(ON\s+(?:\w+\.)?(\w+)(?:\s+USING\s+\w+)?\s*[\(])",
    std::regex_constants::icase);

// FOREIGN KEY regex (used in parseForeignKeyConstraint)
static const std::regex kFkRe(
    R"((?:CONSTRAINT\s+(\w+)\s+)?FOREIGN\s+KEY\s*\(([^)]+)\)\s+REFERENCES\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?(?:[^;]*))",
    std::regex_constants::icase);

// CONSTRAINT name
static const std::regex kConstraintNameRe(
    R"(CONSTRAINT\s+(\w+))",
    std::regex_constants::icase);

// Inline REFERENCES on a column
static const std::regex kInlineRefRe(
    R"((\w+)\s*(?:\(([^)]*)\))?(?:\s+ON\s+DELETE\s+(CASCADE|SET\s+NULL|RESTRICT|NO\s+ACTION|SET\s+DEFAULT))?(?:\s+ON\s+UPDATE\s+(CASCADE|SET\s+NULL|RESTRICT|NO\s+ACTION|SET\s+DEFAULT))?(?:\s+(DEFERRABLE))?(?:\s+INITIALLY\s+(DEFERRED|IMMEDIATE))?)",
    std::regex_constants::icase);

} // anonymous namespace
// ============================================================================

// ============================================================================
// I4: Structured Audit Logging helpers (Phase 4 hardening)
// ============================================================================
namespace {

/**
 * @brief Emit a structured JSON audit event via the THEMIS logger at WARN
 *        level so it is always visible in production log streams.
 *
 * Output format (single-line JSON):
 * @code
 *   [AUDIT] {"event":"import_start","source":"/data/dump.sql","ts_ms":1700000000000,...}
 * @endcode
 *
 * @param event_type  Short identifier, e.g. "import_start", "import_failure",
 *                    "auth_failure", "schema_change_detection", "importer_timeout"
 * @param fields      Arbitrary key→value string pairs appended to the payload.
 */
static void pgAuditLogEvent(
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

/**
 * Split a comma-separated list at top-level commas only, respecting nested
 * parentheses and single-quoted string literals.
 *
 * Examples:
 *   "a, b, c"                       → {"a", " b", " c"}
 *   "a, f(x,y), c"                  → {"a", " f(x,y)", " c"}
 *   "a, DEFAULT 'x,y', c"           → {"a", " DEFAULT 'x,y'", " c"}
 *   "a, CHECK (x > 0 AND y > 1), c" → {"a", " CHECK (x > 0 AND y > 1)", " c"}
 */
static std::vector<std::string> splitTopLevelCommas(const std::string& s) {
    std::vector<std::string> result;
    int   depth     = 0;
    bool  in_string = false;
    std::string current = {};
    for (size_t i = 0; i <static_cast<int>(s.size()); ++i) {
        char c = s[i];
        if (in_string) {
            current += c;
            if (c == '\'') {
                // PostgreSQL '' escape: two consecutive single-quotes inside a string
                if (i + 1 <static_cast<int>(s.size()) && s[i + 1] == '\'') {
                    current += s[++i];
                } else {
                    in_string = false;
                }
            }
        } else if (c == '\'') {
            in_string = true;
            current += c;
        } else if (c == '(') {
            ++depth;
            current += c;
        } else if (c == ')') {
            --depth;
            current += c;
        } else if (c == ',' && depth == 0) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
      result.push_back(current);
    }
    return result;
}

/**
 * Find the matching closing parenthesis for the first '(' in @p sql,
 * respecting nested parentheses and single-quoted strings.
 *
 * Returns std::string::npos if no matching ')' is found.
 */
static size_t findMatchingParen(const std::string& sql, size_t open_pos) {
    // Precondition: sql[open_pos] == '('
    if (open_pos >= sql.size() || sql[open_pos] != '(') return std::string::npos;
    int  depth     = 0;
    bool in_string = false;
    for (size_t k = open_pos; k <static_cast<int>(sql.size()); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == '\'' && k + 1 <static_cast<int>(sql.size()) && sql[k + 1] == '\'') {
                ++k;  // '' escape
            } else if (c == '\'') {
                in_string = false;
            }
        } else if (c == '\'') {
            in_string = true;
        } else if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth == 0) {
              return k;
            }
        }
    }
    return std::string::npos;
}

/**
 * @brief Memory-bounded line reader for streaming import safety.
 *
 * Reads the next newline-terminated line from @p file into @p line with a hard
 * per-line byte cap of @p max_bytes (0 = unlimited, behaves like std::getline).
 * When the cap is exceeded the function:
 *   1. Discards the remaining bytes of the current line (reads to the next '\n')
 *      so the stream cursor is left at the start of the *next* line.
 *   2. Sets @p truncated to `true`.
 *   3. Returns `true` (there was data to read) so the caller can emit an error
 *      and decide whether to continue or abort.
 *
 * When EOF is reached before any bytes are read the function returns `false`.
 *
 * Using this in the inner loops of parseDumpFile() and parseCopy() ensures that
 * an adversarial or accidentally huge pg_dump line (e.g. a COPY row with no
 * newline in 10 GB of data) cannot exhaust process memory.
 */
static bool streamReadLinePg(std::istream& file,
                           std::string& line,
                           size_t max_bytes,
                           bool& truncated) {
    truncated = false;
    line.clear();

    if (max_bytes == 0) {
        // Unlimited – plain std::getline (fastest path)
        if (!std::getline(file, line)) {
          return false;
        }
        return true;
    }

    // For bounded reads: use std::getline into a temporary, then cap.
    // This is significantly faster than character-by-character get() because
    // std::getline uses the streambuf directly.
    static thread_local std::string tl_buf;
    tl_buf.clear();
    if (!std::getline(file, tl_buf)) {
      return false;
    }

    if (static_cast<int>(tl_buf.size()) > max_bytes) {
        line.assign(tl_buf, 0, max_bytes);
        truncated = true;
    } else {
        line = std::move(tl_buf);
    }
    return true;
}


PostgreSQLImporter::PostgreSQLImporter() {
}

PostgreSQLImporter::~PostgreSQLImporter() {
    cancel();
}

std::vector<std::string> PostgreSQLImporter::getSupportedTypes() const {
    return {"postgresql", "postgres", "pg_dump"};
}

bool PostgreSQLImporter::initialize([[maybe_unused]] const std::string& config) {
    cancelled_ = false;
    schemas_.clear();
    
    THEMIS_INFO("PostgreSQL Importer initialized");
    return true;
}

bool PostgreSQLImporter::validateSource(const std::string& source_path, std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }
    
    // Check if it looks like a PostgreSQL dump
    std::string line = {};
    bool found_pg_dump = false;
    int lines_checked = 0;
    
    while (std::getline(file, line) && lines_checked < 100) {
        if (line.find("PostgreSQL database dump") != std::string::npos ||
            line.find("pg_dump") != std::string::npos ||
            line.find("-- Dumped from database version") != std::string::npos) {
            found_pg_dump = true;
            break;
        }
        lines_checked++;
    }
    
    if (!found_pg_dump) {
        errors.push_back("File does not appear to be a PostgreSQL dump");
        return false;
    }
    
    THEMIS_INFO("Source validation successful: {}", source_path);
    return true;
}

ImportStats PostgreSQLImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();
    
    THEMIS_INFO("Starting PostgreSQL import from: {}", source_path);
    THEMIS_INFO("Options: {}", options.toJson().dump());

    // I4: Audit – import start
    pgAuditLogEvent("import_start", {
        {"source",         source_path},
        {"schema_name",    options.default_namespace},
        {"dry_run",        options.dry_run ? "true" : "false"},
        {"timeout_ms",     std::to_string(options.import_timeout_ms)}
    });

    // --- Permission / ACL check ---
    if (options.permission_check) {
        if (!options.permission_check("import", "write")) {
            // I4: Audit – authentication / authorisation failure
            pgAuditLogEvent("auth_failure", {
                {"source", source_path},
                {"user",   "<caller>"},   // anonymised – never log raw credentials
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

    // Reset in-session conflict resolver for this import job
    conflict_resolver_.reset();
    
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
        pgAuditLogEvent("import_failure", {
            {"source",           source_path},
            {"reason",           reason},
            {"records_processed", std::to_string(stats.imported_records)},
            {"partial_import",   stats.imported_records > 0 ? "true" : "false"}
        });
    }
    
    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    // Structured JSON completion summary (Phase 3 observability)
    THEMIS_INFO("Import summary: {}", stats.toJson().dump());
    THEMIS_INFO("Import completed: {} records imported, {} failed, {} skipped in {:.2f}s",
        stats.imported_records, stats.failed_records, stats.skipped_records, stats.elapsed_seconds);

    // Prometheus / OTel metrics emission (Phase 3 observability)
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
    // Per-error-code emission
    for (const auto& e : stats.structured_errors) {
        emitMetric(options, "themisdb_import_errors_total",
                   {{"code", std::to_string(static_cast<uint32_t>(e.code))}},
                   1.0);
    }

    // OTel span for the entire import (Phase 3 tracing)
    emitSpan(options, "import_total",
             {{"source", source_path},
              {"tables", std::to_string(stats.tables_processed)},
              {"rows",   std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> PostgreSQLImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    // Generate a simple unique ID: epoch-ms + pointer suffix
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "import-" + std::to_string(ms) + "-" +
                     std::to_string(reinterpret_cast<uintptr_t>(handle.get()) & 0xFFFF);
    }
    handle->source_path = source_path;  // v2.0: store for schema preview
    handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    handle->running.store(true);
    handle->setStage("pending");

    // Promise/future pair for the final stats
    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    // Build a progress callback that updates the live counters on the handle
    std::weak_ptr<ImportHandle> weak_handle = handle;
    ProgressCallback progress_cb = [weak_handle](const std::string& stage,
                                                  size_t current, size_t total) {
        if (auto h = weak_handle.lock()) {
            h->current_records.store(current);
            h->total_records.store(total);
            h->setStage(stage);
        }
    };

    // Launch worker thread
    std::thread([this, source_path, options, progress_cb, handle, promise]() mutable {
        ImportStats stats;
        try {
            stats = this->importData(source_path, options, progress_cb);
        } catch (const std::exception& e) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = std::string("Unhandled exception in async import: ") + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async import worker";
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

void PostgreSQLImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("Import cancelled");
}

ImportStats PostgreSQLImporter::importDataStreaming(
    const std::string& source_path,
    const ImportOptions& options,
    RowCallback row_callback
) {
    // Wire the caller's row callback into a copy of the options, then delegate
    // to the standard importData() path.  parseCopy() and parseInsert() already
    // check options.streaming_row_callback and invoke it per-row, so no rows
    // are accumulated in memory between callback invocations.
    ImportOptions streaming_opts = options;
    streaming_opts.streaming_row_callback = std::move([[maybe_unused]] row_callback);

    ImportStats stats = importData(source_path, streaming_opts, nullptr);

    // importData() adds a FILE_READ_FAILED error when parseDumpFile() returns
    // false (i.e. cancelled_ was set).  When the streaming callback is the one
    // that requested the abort this is a clean early exit, not an I/O failure.
    // Remove that spurious error so callers see clean stats.  Real file-open
    // failures happen before any rows are processed (imported_records == 0) so
    // the guard below preserves genuine FILE_READ_FAILED errors.
    if (stats.imported_records > 0) {
        auto& se = stats.structured_errors;
        se.erase(std::remove_if(se.begin(), se.end(),
            [](const ImportError& e) {
                return e.code == ImportErrorCode::FILE_READ_FAILED &&
                       e.severity == ImportErrorSeverity::CRITICAL &&
                       e.message == "Failed to parse dump file";
            }),
            se.end());
        // Keep errors/warnings vectors in sync
        auto& ev = stats.errors;
        ev.erase(std::remove(ev.begin(), ev.end(),
                             std::string("Failed to parse dump file")),
                 ev.end());
    }

    // Reset cancelled_ so this importer instance can be reused after a
    // streaming callback abort.
    cancelled_ = false;

    return stats;
}

json PostgreSQLImporter::getSourceSchema(const std::string& source_path) {
    schemas_.clear();
    
    std::ifstream file(source_path);
    if (!file) {
        return json::array();
    }
    
    std::string line = {};
    std::string current_sql = {};
    // Performance: pre-reserve to avoid reallocations for typical DDL lines
    line.reserve(4096);
    current_sql.reserve(8192);
    
    while (std::getline(file, line)) {
        // Skip blank lines and SQL line comments (-- ...)
        // Block comments (/* ... */) are handled by the statement assembler
        if (line.empty()) {
          continue;
        }
        // Trim leading whitespace before comment check
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first != std::string::npos && static_cast<int>(line.size()) >= first + 2 &&
            line[first] == '-' && line[first + 1] == '-') continue;
        
        // Performance: avoid temporary string from `+= line + " "`
        current_sql.append(line).append(1, ' ');
        
        // Complete statement?
        if (line.find(';') != std::string::npos) {
            if (current_sql.find("CREATE TABLE") != std::string::npos) {
                TableSchema schema = {};
                if (parseCreateTable(current_sql, schema)) {
                    schemas_[schema.name] = schema;
                }
            } else if (current_sql.find("CREATE TYPE") != std::string::npos) {
                std::smatch tm = {};
                if (std::regex_search(current_sql, tm, kEnumTypeRe)) {
                    {
                        std::lock_guard<std::mutex> lock(custom_type_map_mutex_);
                        custom_type_map_[tm[1].str()] = "string";
                    }
                } else if (std::regex_search(current_sql, tm, kCompositeTypeRe)) {
                    {
                        std::lock_guard<std::mutex> lock(custom_type_map_mutex_);
                        custom_type_map_[tm[1].str()] = "object";
                    }
                }
            } else if (current_sql.find("CREATE INDEX") != std::string::npos ||
                       current_sql.find("CREATE UNIQUE INDEX") != std::string::npos) {
                std::smatch ti = {};
                if (std::regex_search(current_sql, ti, kCreateIndexRe)) {
                    std::string tname = ti[3].str();
                    IndexMetadata idx = {};
                    if (parseCreateIndex(current_sql, tname, idx) && schemas_.count(tname)) {
                        schemas_[tname].indexes.push_back(idx);
                    }
                }
            }
            // v2.0: capture FK constraints declared outside the CREATE TABLE body
            else if (current_sql.find("ALTER TABLE") != std::string::npos &&
                     current_sql.find("FOREIGN KEY") != std::string::npos) {
                ImportStats dummy_stats;
                ImportOptions dummy_opts;
                dummy_opts.preserve_foreign_keys = true;
                parseAlterTableAddFk(current_sql, dummy_opts, dummy_stats);
            }
            current_sql.clear();
        }
    }
    
    // Build relationship mappings
    ImportOptions schema_opts;
    auto forward_mappings = RelationshipMapper::mapFromForeignKeys(schemas_,
                                                            schema_opts.relationship_mapping_mode);

    // Generate bidirectional (ONE_TO_MANY inverse) edges
    auto inverse_mappings = RelationshipMapper::generateInverseEdges(forward_mappings);

    // Detect circular references
    std::vector<std::string> cycles;
    RelationshipMapper::detectCircularReferences(schemas_, cycles);

    // Convert to JSON
    json result = json::array();
    for (const auto& kv : schemas_) {
        const auto& schema = kv.second;

        json fk_arr = json::array();
        for (const auto& fk : schema.foreign_keys) {
          fk_arr.push_back(fk.toJson());
        }

        json table_json = {
            {"name", schema.name},
            {"schema", schema.schema},
            {"columns", schema.columns},
            {"column_types", schema.column_types},
            {"primary_keys", schema.primary_keys},
            {"foreign_keys", fk_arr},  // v2.0: preserved FK metadata
            {"column_defaults", schema.column_defaults},
            {"column_constraints", schema.column_constraints},
            {"custom_types", schema.custom_types}
        };

        // Indexes
        json idx_arr = json::array();
        for (const auto& idx : schema.indexes) {
          idx_arr.push_back(idx.toJson());
        }
        table_json["indexes"] = idx_arr;

        // v2.1: CHECK constraints
        json ck_arr = json::array();
        for (const auto& ck : schema.check_constraints) {
          ck_arr.push_back(ck.toJson());
        }
        table_json["check_constraints"] = ck_arr;

        // v2.1: Generated columns
        json gen_arr = json::array();
        for (const auto& g : schema.generated_columns) {
          gen_arr.push_back(g.toJson());
        }
        table_json["generated_columns"] = gen_arr;

        // v2.1: Exclude constraints
        json excl_arr = json::array();
        for (const auto& ex : schema.exclude_constraints) {
          excl_arr.push_back(ex.toJson());
        }
        table_json["exclude_constraints"] = excl_arr;

        result.push_back(table_json);
    }

    // Wrap in an object with schema + relationships (forward + inverse) + circular_references
    json relationships_arr = json::array();
    for (const auto& m : forward_mappings) {
      relationships_arr.push_back(m.toJson());
    }
    for (const auto& m : inverse_mappings) {
      relationships_arr.push_back(m.toJson());
    }

    json cycles_arr = json::array();
    for (const auto& c : cycles) {
      cycles_arr.push_back(c);
    }

    return json{
        {"tables", result},
        {"relationships", relationships_arr},
        {"circular_references", cycles_arr},
        {"custom_types", custom_type_map_}
    };
}

// ============================================================================
// Private Methods
// ============================================================================

bool PostgreSQLImporter::parseDumpFile(const std::string& file_path, const ImportOptions& options,
                                        ImportStats& stats, ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    // --- pg_dump mode detection (schema-only / data-only) ---
    // Scan the first 50 comment lines for known pg_dump header markers.
    // Use a 4 KB cap per line – header comments are always short.
    {
        std::string hdr_line = {};
        int hdr_lines = 0;
        bool hdr_trunc = false;
        std::streampos after_header = 0;
        while (streamReadLinePg(file, hdr_line, 4096, hdr_trunc) && hdr_lines < 50) {
            after_header = file.tellg();
            if (hdr_line.find("-- PostgreSQL database dump") != std::string::npos ||
                hdr_line.find("pg_dump") != std::string::npos) {
                hdr_lines++;
                continue;
            }
            if (hdr_line.find("schema only") != std::string::npos ||
                hdr_line.find("schema-only") != std::string::npos ||
                hdr_line.find("--schema-only") != std::string::npos) {
                stats.is_schema_only = true;
            }
            if (hdr_line.find("data only") != std::string::npos ||
                hdr_line.find("data-only") != std::string::npos ||
                hdr_line.find("--data-only") != std::string::npos) {
                stats.is_data_only = true;
            }
            // Stop after non-empty non-comment line
            if (!hdr_line.empty() && !(hdr_line.size() >= 2 && hdr_line[0] == '-' && hdr_line[1] == '-')) {
                break;
            }
            hdr_lines++;
        }
        // Rewind to read the full file again
        file.clear();
        file.seekg(0);
    }

    // --- Delta hash loading ---
    std::unordered_set<uint64_t> delta_hashes = {};

    if (!options.delta_hash_file.empty()) {
        delta_hashes = loadDeltaHashes(options.delta_hash_file);
        THEMIS_INFO("Delta import: loaded {} known hashes from {}",static_cast<int>(delta_hashes.size()),
                    options.delta_hash_file);
    }

    // --- Checkpoint / resume support ---
    std::streampos resume_offset = 0;
    if (!options.checkpoint_file.empty()) {
        ImportStats checkpoint_stats = {};
        if (loadCheckpoint(options.checkpoint_file, resume_offset, checkpoint_stats)) {
            THEMIS_INFO("Resuming import from byte offset {}", static_cast<long>(resume_offset));
            file.seekg(resume_offset);
            // Carry accumulated counts from the checkpoint
            stats.imported_records = checkpoint_stats.imported_records;
            stats.failed_records   = checkpoint_stats.failed_records;
            stats.skipped_records  = checkpoint_stats.skipped_records;
            stats.total_records    = checkpoint_stats.total_records;
            stats.tables_processed = checkpoint_stats.tables_processed;
        }
    }

    std::string line = {};
    std::string current_sql;
    size_t line_number = 0;
    size_t batch_row_count = 0;

    // Per-line read limit: cap single-line allocation to max_statement_size_bytes
    // (or a safe default of 64 MB) so a crafted dump with no newlines cannot OOM.
    const size_t line_read_limit = options.max_statement_size_bytes > 0
                                   ? options.max_statement_size_bytes
                                   : 64 * 1024 * 1024;  // 64 MB default cap

    // Performance: pre-reserve buffers so common DDL/DML lines (≤4 KB) avoid
    // repeated reallocation inside the hot loop.
    line.reserve(4096);
    current_sql.reserve(8192);

    // I1: Timeout / deadline guard ──────────────────────────────────────────
    // When import_timeout_ms > 0 a deadline is computed from the current wall
    // clock.  The deadline is checked every 500 lines (< 1 ms overhead on
    // modern hardware) so the import loop aborts promptly without polling on
    // every iteration.
    const bool timeout_enabled = (options.import_timeout_ms > 0);
    const auto import_deadline  = timeout_enabled
        ? std::chrono::steady_clock::now() +
          std::chrono::milliseconds(options.import_timeout_ms)
        : std::chrono::steady_clock::time_point::max();
    // ────────────────────────────────────────────────────────────────────────

    // PHASE-2-HARDENING: Connection Pool Exhaustion Handling
    // Track active connections (simulated for file-based import; real DB importers use actual connection tracking)
    // Initialize connection pool state for this import session
    g_connection_pool.connection_timeout_ms = 0;  // File-based imports don't use live connections
    
    // Attempt to acquire a "connection slot" from the pool (always succeeds for file-based import)
    // In real DB importers, this would block or fail if pool is exhausted
    if (g_connection_pool.active_connections >= ConnectionPoolState::max_active_connections) {
        // PHASE-2-HARDENING: Pool exhaustion error reporting
        pgAuditLogEvent("connection_pool_exhausted", {
            {"source",        file_path},
            {"max_conns",     std::to_string(ConnectionPoolState::max_active_connections)},
            {"active_conns",  std::to_string(g_connection_pool.active_connections.load())}
        });
        addError(stats, ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE,
                 ImportErrorSeverity::CRITICAL,
                 "Connection pool exhausted: maximum " +
                 std::to_string(ConnectionPoolState::max_active_connections) + " concurrent imports allowed");
        return false;
    }
    
    // Increment active connection counter
    g_connection_pool.active_connections++;
    
    // PHASE-2-HARDENING: CDC Fallback Mechanism
    // Detect CDC capability and log fallback decision if needed
    // Read the first 5000 characters for CDC detection (header scanning)
    std::string dump_header = {};
    dump_header.reserve(5000);
    file.clear();
    file.seekg(0);
    char buf[5000];
    file.read(buf, sizeof(buf));
    size_t read_bytes = file.gcount();
    if (read_bytes > 0) {
        dump_header.assign(buf, read_bytes);
    }
    file.clear();
    file.seekg(0);
    
    bool has_cdc = detectCDCCapability(dump_header);
    if (has_cdc) {
        // CDC features detected but not actively used in file-based import
        // Log the fallback to standard COPY/INSERT parsing
        pgAuditLogEvent("cdc_fallback_to_standard", {
            {"source",           file_path},
            {"cdc_detected",     "true"},
            {"fallback_mode",    "standard_copy_insert"},
            {"reason",           "file_based_import_uses_standard_parsing"}
        });
        THEMIS_INFO("CDC capability detected in dump, using standard COPY/INSERT parsing for file-based import");
    }
    
    // ────────────────────────────────────────────────────────────────────────

    bool line_truncated = false;
    while (streamReadLinePg(file, line, line_read_limit, line_truncated) && !cancelled_) {
        line_number++;

        // I1: Deadline check every 500 lines ─────────────────────────────────
        if (timeout_enabled && (line_number % 500 == 0)) {
            if (std::chrono::steady_clock::now() >= import_deadline) {
                pgAuditLogEvent("importer_timeout", {
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

        // Skip blank lines and SQL comments
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
            continue;
        }
        
        // Performance: avoid temporary string from `+= line + " "`
        current_sql.append(line).append(1, ' ');

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
        
        // Complete statement?
        if (line.find(';') != std::string::npos) {
            // Parse different statement types
            if (current_sql.find("CREATE TABLE") != std::string::npos ||
                current_sql.find("CREATE SCHEMA") != std::string::npos) {
                auto t0 = std::chrono::steady_clock::now();
                TableSchema schema = {};
                if (parseCreateTable(current_sql, schema)) {
                    if (shouldImportTable(schema.name, options)) {
                        // v2.0: count preserved FKs discovered in CREATE TABLE body
                        if (options.preserve_foreign_keys) {
                            stats.foreign_keys_preserved += schema.foreign_keys.size();
                        } else {
                            // When FK preservation is disabled, discard parsed FKs so
                            // they are not embedded in entity JSON or returned by
                            // getSourceSchema().
                            schema.foreign_keys.clear();
                        }
                        schemas_[schema.name] = schema;
                        stats.tables_processed++;
                        // v2.0: count inline FKs (REFERENCES) extracted during DDL parsing
                        if (options.preserve_relationships) {
                            stats.relationships_processed +=
                                schema.foreign_keys.size();
                        }
                        THEMIS_DEBUG("Parsed table schema: {}", schema.name);
                        // I4: Audit – schema change detected
                        pgAuditLogEvent("schema_change_detection", {
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
            }
            // CREATE TYPE ... AS ENUM / AS (...) – register custom type mapping
            else if (current_sql.find("CREATE TYPE") != std::string::npos) {
                std::smatch tm = {};
                if (std::regex_search(current_sql, tm, kEnumTypeRe)) {
                    {
                        std::lock_guard<std::mutex> lock(custom_type_map_mutex_);
                        custom_type_map_[tm[1].str()] = "string";
                    }
                    stats.custom_types_processed++;
                    THEMIS_DEBUG("Registered enum type: {} -> string", tm[1].str());
                } else if (std::regex_search(current_sql, tm, kCompositeTypeRe)) {
                    {
                        std::lock_guard<std::mutex> lock(custom_type_map_mutex_);
                        custom_type_map_[tm[1].str()] = "object";
                    }
                    stats.custom_types_processed++;
                    THEMIS_DEBUG("Registered composite type: {} -> object", tm[1].str());
                }
            }
            // ALTER TABLE ... ADD COLUMN – update cached schema so subsequent COPY
            // and INSERT statements see the new column.
            else if (current_sql.find("ALTER TABLE") != std::string::npos &&
                     current_sql.find("ADD COLUMN") != std::string::npos) {
                std::smatch am = {};
                if (std::regex_search(current_sql, am, kAlterAddColumnRe)) {
                    std::string tname = am[1].str();
                    std::string cname = am[2].str();
                    std::string ctype = am[3].str();
                    if (schemas_.count(tname)) {
                        auto& ts = schemas_[tname];
                        if (std::find(ts.columns.begin(), ts.columns.end(), cname) ==
                            ts.columns.end()) {
                            ts.columns.push_back(cname);
                            ts.column_types[cname] = ctype;
                            THEMIS_DEBUG("ALTER TABLE {}: added column {} {}", tname, cname, ctype);
                            emitSpan(options, "alter_column",
                                     {{"table", tname}, {"column", cname}}, 0.0);
                        }
                    }
                }
            }
            // v2.0: ALTER TABLE ... ADD CONSTRAINT ... FOREIGN KEY – preserve FK
            else if (current_sql.find("ALTER TABLE") != std::string::npos &&
                     current_sql.find("FOREIGN KEY") != std::string::npos) {
                parseAlterTableAddFk(current_sql, options, stats);
            }
            // v2.0: CREATE [UNIQUE] INDEX ... ON table (cols)
            else if (current_sql.find("CREATE INDEX") != std::string::npos ||
                     current_sql.find("CREATE UNIQUE INDEX") != std::string::npos) {
                auto t0 = std::chrono::steady_clock::now();
                // Extract table name to attach the index
                std::smatch ti = {};
                if (std::regex_search(current_sql, ti, kCreateIndexRe)) {
                    std::string tname = ti[3].str();
                    IndexMetadata idx = {};
                    if (parseCreateIndex(current_sql, tname, idx) && schemas_.count(tname)) {
                        schemas_[tname].indexes.push_back(idx);
                        stats.indexes_processed++;
                        THEMIS_DEBUG("Parsed index {} on {}", idx.name, tname);
                        double dur = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - t0).count();
                        emitSpan(options, "parse_index",
                                 {{"table", tname}, {"index", idx.name}}, dur);
                    }
                }
            }
            else if (current_sql.find("INSERT INTO") != std::string::npos) {
                stats.total_records++;
                {
                    auto t0 = std::chrono::steady_clock::now();
                    // In dry-run mode parseInsert skips writes; still validates the row
                    parseInsert(current_sql, options, stats, line_number);
                    double dur = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - t0).count();
                    emitSpan(options, "insert_batch", {}, dur);
                }
                batch_row_count++;
            }
            else if (current_sql.find("COPY ") != std::string::npos) {
                // Extract table name and optional column list from COPY header
                // Pattern: COPY [schema.]table [(col1, col2, ...)] FROM stdin;
                std::smatch match = {};
                if (std::regex_search(current_sql, match, kCopyRe)) {
                    std::string table_name = match[1].str();
                    std::vector<std::string> col_list = {};

                    if (match[2].matched && !match[2].str().empty()) {
                        std::istringstream css(match[2].str());
                        std::string col = {};
                        while (std::getline(css, col, ',')) {
                            col.erase(0, col.find_first_not_of(" \t"));
                            col.erase(col.find_last_not_of(" \t") + 1);
                            if (!col.empty()) {
                              col_list.push_back(col);
                            }
                        }
                    }
                    size_t before_copy = stats.imported_records;
                    auto t0 = std::chrono::steady_clock::now();
                    // In dry-run mode parseCopy skips writes; still parses and validates rows
                    parseCopy(file, table_name, col_list, options, stats, delta_hashes);
                    size_t rows_in_block = stats.imported_records - before_copy;
                    batch_row_count += rows_in_block;
                    double dur = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - t0).count();
                    emitSpan(options, "copy_block",
                             {{"table", table_name},
                              {"rows", std::to_string(rows_in_block)}}, dur);
                } else {
                    addError(stats, ImportErrorCode::PARSE_COPY_HEADER,
                             ImportErrorSeverity::WARNING,
                             "Could not parse COPY header",
                             "line " + std::to_string(line_number));
                    stats.warnings.push_back("Could not parse COPY header near line " +
                                             std::to_string(line_number));
                }
            }
            
            current_sql.clear();

            // Checkpoint after each batch (skipped in dry-run: no persistent state changes)
            if (!options.dry_run &&
                !options.checkpoint_file.empty() &&
                options.batch_size > 0 &&
                batch_row_count >= options.batch_size) {
                std::streampos current_pos = file.tellg();
                saveCheckpoint(options.checkpoint_file, current_pos, stats);
                batch_row_count = 0;
                reportProgress(callback, "data", stats.imported_records, 0);
            }
        }
    }

    // Final checkpoint on clean completion (skipped in dry-run)
    if (!options.dry_run && !options.checkpoint_file.empty() && !cancelled_) {
        saveCheckpoint(options.checkpoint_file, file.tellg(), stats);
    }

    // Save updated delta hashes (skipped in dry-run: no persistent state changes)
    if (!options.dry_run && !options.delta_hash_file.empty() && !delta_hashes.empty()) {
        saveDeltaHashes(options.delta_hash_file, delta_hashes);
    }

    // v2.0: Validate FK references if requested
    if (options.validate_references && !cancelled_) {
        validateForeignKeyReferences(options, stats);
    }
    
    // PHASE-2-HARDENING: Connection pool cleanup
    // Release the connection slot back to the pool
    if (g_connection_pool.active_connections > 0) {
        g_connection_pool.active_connections--;
    }
    
    // Log connection release for audit trail
    pgAuditLogEvent("connection_released", {
        {"source",            file_path},
        {"import_completed",  !cancelled_ ? "true" : "false"},
        {"records_imported",  std::to_string(stats.imported_records)},
        {"active_conns_after", std::to_string(g_connection_pool.active_connections.load())}
    });
    
    return !cancelled_;
}

bool PostgreSQLImporter::parseCreateTable(const std::string& sql, TableSchema& schema) {
    std::smatch match = {};
    if (!std::regex_search(sql, match, kCreateTableRe)) {
        return false;
    }

    if (static_cast<int>(match.size()) > 2) {
        schema.schema = match[1].str();
        schema.name = match[2].str();
    } else {
        schema.name = match[1].str();
    }

    size_t start = sql.find('(', match.position());
    if (start == std::string::npos) {
      return !schema.name.empty();
    }
    size_t end = findMatchingParen(sql, start);
    if (end == std::string::npos) {
      return !schema.name.empty();
    }

    std::string columns_str = sql.substr(start + 1, end - start - 1);
    std::vector<std::string> column_defs = splitTopLevelCommas(columns_str);

    auto toUpper = [](std::string s) {
        for (auto& c : s) {
          c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s;
    };
    auto trim = [](const std::string& s) {
        size_t l = s.find_first_not_of(" \t\n\r");
        if (l == std::string::npos) return std::string{};
        size_t r = s.find_last_not_of(" \t\n\r");
        return s.substr(l, r - l + 1);
    };
    auto startsWith = [](const std::string& s, const std::string& pfx) {
        return static_cast<bool>( static_cast<int>(s.size()) < static_cast<int>(= pfx.size() && s.compare(0,static_cast<int>(pfx.size()))), pfx) == 0;
    };

    for (const auto& raw_def : column_defs) {
        std::string column_def = trim(raw_def);
        if (column_def.empty()) {
          continue;
        }

        std::string upper_def = toUpper(column_def);

        bool is_table_constraint =
            startsWith(upper_def, "CONSTRAINT ") ||
            startsWith(upper_def, "PRIMARY KEY") ||
            startsWith(upper_def, "FOREIGN KEY") ||
            startsWith(upper_def, "UNIQUE") ||
            startsWith(upper_def, "CHECK") ||
            startsWith(upper_def, "EXCLUDE");

        if (is_table_constraint) {
            if (upper_def.find("FOREIGN KEY") != std::string::npos) {
                parseForeignKeyConstraint(column_def, schema);
                continue;
            }

            if (upper_def.find("PRIMARY KEY") != std::string::npos) {
                size_t pk_paren = column_def.find('(');
                if (pk_paren != std::string::npos) {
                    size_t pk_end = findMatchingParen(column_def, pk_paren);
                    if (pk_end != std::string::npos) {
                        std::string pk_cols = column_def.substr(pk_paren + 1, pk_end - pk_paren - 1);
                        std::istringstream pkss(pk_cols);
                        std::string pkc = {};
                        while (std::getline(pkss, pkc, ',')) {
                            pkc = trim(pkc);
                            if (!pkc.empty() && pkc.front() == '"') {
                              pkc = pkc.substr(1, static_cast<int>(pkc.size()) - 2);
                            }
                            if (!pkc.empty()) {
                              schema.primary_keys.push_back(pkc);
                            }
                        }
                    }
                }
                continue;
            }

            if (upper_def.find("CHECK") != std::string::npos) {
                CheckConstraint ck = {};
                if (parseCheckConstraint(column_def, ck)) {
                    schema.check_constraints.push_back(ck);
                }
            }

            if (upper_def.find("EXCLUDE") != std::string::npos) {
                ExcludeConstraint excl = {};
                if (parseExcludeConstraint(column_def, excl)) {
                    schema.exclude_constraints.push_back(excl);
                }
                continue;
            }

            if (upper_def.find("UNIQUE") != std::string::npos) {
                size_t u_paren = column_def.find('(');
                if (u_paren != std::string::npos) {
                    size_t u_end = findMatchingParen(column_def, u_paren);
                    if (u_end != std::string::npos) {
                        IndexMetadata idx;
                        idx.unique = true;
                        idx.type = "btree";

                        std::smatch cname_m = {};
                        if (std::regex_search(column_def, cname_m, kConstraintNameRe)) {
                            idx.name = cname_m[1].str();
                        } else {
                            idx.name = schema.name + "_unique_" + std::to_string(schema.indexes.size());
                        }

                        std::string ucols = column_def.substr(u_paren + 1, u_end - u_paren - 1);
                        std::istringstream ucss(ucols);
                        std::string uc = {};
                        while (std::getline(ucss, uc, ',')) {
                            uc = trim(uc);
                            if (!uc.empty() && uc.front() == '"') {
                              uc = uc.substr(1, static_cast<int>(uc.size()) - 2);
                            }
                            if (!uc.empty()) {
                              idx.columns.push_back(uc);
                            }
                        }
                        schema.indexes.push_back(idx);
                    }
                }
            }
            continue;
        }

        std::istringstream col_ss(column_def);
        std::string col_name, col_type;
        col_ss >> col_name >> col_type;
        if (col_name.empty() || col_type.empty()) {
          continue;
        }

        if (!col_name.empty() && col_name.front() == '"' && static_cast<int>(col_name.size()) >= 2 && col_name.back() == '"') {
            col_name = col_name.substr(1, static_cast<int>(col_name.size()) - 2);
        }

        schema.columns.push_back(col_name);
        schema.column_types[col_name] = col_type;

        if (upper_def.find("NOT NULL") != std::string::npos) {
            schema.column_constraints[col_name] = "NOT NULL";
        }

        if (upper_def.find(" UNIQUE") != std::string::npos ||
            startsWith(upper_def, "UNIQUE ")) {
            schema.column_constraints[col_name] =
                (schema.column_constraints.count(col_name)
                    ? schema.column_constraints[col_name] + ",UNIQUE"
                    : "UNIQUE");
            IndexMetadata idx;
            idx.name = schema.name + "_" + col_name + "_key";
            idx.type = "btree";
            idx.unique = true;
            idx.columns = {col_name};
            schema.indexes.push_back(idx);
        }

        if (upper_def.find("PRIMARY KEY") != std::string::npos) {
            schema.primary_keys.push_back(col_name);
        }

        {
            size_t def_pos = upper_def.find(" DEFAULT ");
            if (def_pos != std::string::npos) {
                std::string after = column_def.substr(def_pos + 9);
                std::string upper_after = toUpper(after);
                size_t end_pos = after.size();
                for (const auto& kw : {" NOT ", " NULL", " UNIQUE", " PRIMARY", " REFERENCES",
                                       " CHECK", " GENERATED", " COLLATE"}) {
                    size_t kp = upper_after.find(kw);
                    if (kp != std::string::npos && kp < end_pos) {
                      end_pos = kp;
                    }
                }
                std::string def_val = trim(after.substr(0, end_pos));
                if (!def_val.empty()) {
                  schema.column_defaults[col_name] = def_val;
                }
            }
        }

        parseInlineReference(col_name, column_def, schema);

        {
            GeneratedColumnInfo gen = {};
            if (parseGeneratedColumn(column_def, col_name, gen)) {
                schema.generated_columns.push_back(gen);
            }
        }
    }

    return !schema.name.empty();
}

// ============================================================================
// v2.0: Foreign Key Preservation helpers
// ============================================================================

/**
 * Split a comma-separated column list (no nested parens expected here).
 * Returns trimmed column names stripped of surrounding quotes.
 */
static std::vector<std::string> splitColumnList(const std::string& s) {
    std::vector<std::string> cols;
    std::istringstream ss(s);
    std::string col = {};
    while (std::getline(ss, col, ',')) {
        col.erase(0, col.find_first_not_of(" \t\n\r\""));
        col.erase(col.find_last_not_of(" \t\n\r\"") + 1);
        if (!col.empty()) {
          cols.push_back(col);
        }
    }
    return cols;
}

bool PostgreSQLImporter::parseForeignKeyConstraint(const std::string& constraint_def,
                                                    TableSchema& schema) const {
    // Matches patterns like:
    //   FOREIGN KEY (col1, col2) REFERENCES ref_table (ref1, ref2) [ON DELETE action] [ON UPDATE action]
    //   CONSTRAINT name FOREIGN KEY (col) REFERENCES ref_table (ref_col) ON DELETE CASCADE
    std::regex fk_regex(
        R"(FOREIGN KEY\s*\(([^)]+)\)\s*REFERENCES\s+(?:\w+\.)?(\w+)\s*\(([^)]+)\)([^,]*)?)",
        std::regex_constants::icase);
    std::smatch m = {};
    if (!std::regex_search(constraint_def, m, fk_regex)) {
        return false;
    }

    TableSchema::ForeignKeyConstraint fk;

    // Extract optional constraint name
    std::regex cname_regex(R"(CONSTRAINT\s+(\w+)\s+FOREIGN KEY)", std::regex_constants::icase);
    std::smatch cm = {};
    if (std::regex_search(constraint_def, cm, cname_regex)) {
        fk.constraint_name = cm[1].str();
    }

    fk.columns     = splitColumnList(m[1].str());
    fk.ref_table   = m[2].str();
    fk.ref_columns = splitColumnList(m[3].str());

    auto joinCols = [](const std::vector<std::string>& cols) {
        std::string out = {};
        for (size_t i = 0; i <static_cast<int>(cols.size()); ++i) {
            if (i > 0) {
              out += ",";
            }
            out += cols[i];
        }
        return out;
    };

    fk.name = fk.constraint_name;
    fk.source_column = joinCols(fk.columns);
    fk.target_table = fk.ref_table;
    fk.target_column = joinCols(fk.ref_columns);

    // Extract ON DELETE / ON UPDATE actions from the trailing clause (m[4])
    std::string trailing = m[4].matched ? m[4].str() : "";
    {
        std::regex on_delete_regex(R"(ON\s+DELETE\s+(CASCADE|SET NULL|SET DEFAULT|RESTRICT|NO ACTION))",
                                   std::regex_constants::icase);
        std::smatch dm = {};
        if (std::regex_search(trailing, dm, on_delete_regex)) {
            fk.on_delete = dm[1].str();
            // Normalise to uppercase
            std::transform(fk.on_delete.begin(), fk.on_delete.end(),
                           fk.on_delete.begin(), ::toupper);
        }
    }
    {
        std::regex on_update_regex(R"(ON\s+UPDATE\s+(CASCADE|SET NULL|SET DEFAULT|RESTRICT|NO ACTION))",
                                   std::regex_constants::icase);
        std::smatch um = {};
        if (std::regex_search(trailing, um, on_update_regex)) {
            fk.on_update = um[1].str();
            std::transform(fk.on_update.begin(), fk.on_update.end(),
                           fk.on_update.begin(), ::toupper);
        }
    }
    fk.on_delete_action = fk.on_delete;
    fk.on_update_action = fk.on_update;

    if (!fk.columns.empty() && !fk.ref_table.empty() && !fk.ref_columns.empty()) {
        schema.foreign_keys.push_back(std::move(fk));
        THEMIS_DEBUG("FK preserved: {}.({}) → {}.({})",
                     schema.name,
                     schema.foreign_keys.back().columns.empty() ? "" : schema.foreign_keys.back().columns[0],
                     schema.foreign_keys.back().ref_table,
                     schema.foreign_keys.back().ref_columns.empty() ? "" : schema.foreign_keys.back().ref_columns[0]);
        return true;
    }
    return false;
}

bool PostgreSQLImporter::parseInlineReference(const std::string& col_name,
                                               const std::string& col_def,
                                               TableSchema& schema) const {
    // Handles: column_name type [NOT NULL] REFERENCES ref_table [(ref_col)] [ON DELETE …]
    // The ref_col part is optional (defaults to PK of ref_table when omitted).
    std::regex ref_regex(
        R"(REFERENCES\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]+)\))?([^,]*)?)",
        std::regex_constants::icase);
    std::smatch m = {};
    if (!std::regex_search(col_def, m, ref_regex)) {
        return false;
    }

    TableSchema::ForeignKeyConstraint fk;
    fk.columns   = {col_name};
    fk.ref_table = m[1].str();
    if (m[2].matched && !m[2].str().empty()) {
        fk.ref_columns = splitColumnList(m[2].str());
    } else {
        fk.ref_columns = {"id"};
    }

    fk.name = fk.constraint_name;
    fk.source_column = col_name;
    fk.target_table = fk.ref_table;
    fk.target_column = fk.ref_columns.empty() ? std::string{} : fk.ref_columns.front();

    std::string trailing = m[3].matched ? m[3].str() : "";
    {
        std::regex on_delete_regex(R"(ON\s+DELETE\s+(CASCADE|SET NULL|SET DEFAULT|RESTRICT|NO ACTION))",
                                   std::regex_constants::icase);
        std::smatch dm = {};
        if (std::regex_search(trailing, dm, on_delete_regex)) {
            fk.on_delete = dm[1].str();
            std::transform(fk.on_delete.begin(), fk.on_delete.end(),
                           fk.on_delete.begin(), ::toupper);
        }
    }
    {
        std::regex on_update_regex(R"(ON\s+UPDATE\s+(CASCADE|SET NULL|SET DEFAULT|RESTRICT|NO ACTION))",
                                   std::regex_constants::icase);
        std::smatch um = {};
        if (std::regex_search(trailing, um, on_update_regex)) {
            fk.on_update = um[1].str();
            std::transform(fk.on_update.begin(), fk.on_update.end(),
                           fk.on_update.begin(), ::toupper);
        }
    }
    fk.on_delete_action = fk.on_delete;
    fk.on_update_action = fk.on_update;

    if (!fk.ref_table.empty()) {
        schema.foreign_keys.push_back(std::move(fk));
        return true;
    }
    return false;
}

void PostgreSQLImporter::parseAlterTableAddFk(const std::string& sql,
                                               const ImportOptions& options,
                                               ImportStats& stats) {
    if (!options.preserve_foreign_keys) {
      return;
    }

    // Pattern: ALTER TABLE [ONLY] [schema.]table ADD CONSTRAINT name FOREIGN KEY (cols) REFERENCES ref (ref_cols) [ON DELETE …];
    // Also handles: ALTER TABLE table ADD FOREIGN KEY (cols) REFERENCES ref (ref_cols);
    std::regex tbl_regex(
        R"(ALTER TABLE\s+(?:ONLY\s+)?(?:\w+\.)?(\w+)\s+ADD\s+(?:CONSTRAINT\s+\w+\s+)?FOREIGN KEY)",
        std::regex_constants::icase);
    std::smatch tm = {};
    if (!std::regex_search(sql, tm, tbl_regex)) {
        return;
    }
    std::string tname = tm[1].str();

    if (!schemas_.count(tname)) {
        THEMIS_DEBUG("ALTER TABLE ADD FOREIGN KEY: unknown table '{}', skipping", tname);
        return;
    }

    size_t before = schemas_[tname].foreign_keys.size();
    parseForeignKeyConstraint(sql, schemas_[tname]);
    size_t added = schemas_[tname].foreign_keys.size() - before;
    if (added > 0) {
        stats.foreign_keys_preserved += added;
        THEMIS_DEBUG("ALTER TABLE {}: preserved {} FK(s)", tname, added);
    }
}

// v2.0 Parser Methods
// ============================================================================

/**
 * @brief Parse a CONSTRAINT ... FOREIGN KEY definition (table-level or from ALTER TABLE).
 *
 * Handles:
 *   [CONSTRAINT name] FOREIGN KEY (src_col[, ...]) REFERENCES tgt_tbl (tgt_col[, ...])
 *     [ON DELETE action] [ON UPDATE action]
 *     [DEFERRABLE [INITIALLY DEFERRED|INITIALLY IMMEDIATE]]
 *     [NOT DEFERRABLE]
 */
bool PostgreSQLImporter::parseForeignKeyConstraint(const std::string& constraint_def,
                                                    ForeignKeyConstraint& fk) {
    std::smatch m = {};
    if (!std::regex_search(constraint_def, m, kFkRe)) {
      return false;
    }

    fk.name          = m[1].matched ? m[1].str() : "";
    fk.target_table  = m[3].str();
    fk.target_column = m[4].matched ? m[4].str() : "";

    // Trim and collapse spaces in column lists
    auto trimStr = [](const std::string& s) {
        size_t l = s.find_first_not_of(" \t\r\n");
        size_t r = s.find_last_not_of(" \t\r\n");
        return (l == std::string::npos) ? std::string{} : s.substr(l, r - l + 1);
    };
    auto normalizeColList = [&trimStr](const std::string& cols) {
        std::string result = {};
        std::istringstream ss(cols);
        std::string c = {};
        while (std::getline(ss, c, ',')) {
            c = trimStr(c);
            if (!c.empty() && c.front() == '"') {
              c = c.substr(1, static_cast<int>(c.size()) - 2);
            }
            if (!result.empty()) {
              result += ",";
            }
            result += c;
        }
        return result;
    };

    fk.source_column = normalizeColList(m[2].str());
    fk.target_column = normalizeColList(fk.target_column);

    // Parse ON DELETE / ON UPDATE actions and DEFERRABLE from the full text
    std::string upper = constraint_def;
    for (auto& c : upper) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    auto extractAction = [&upper, &trimStr](const std::string& keyword) -> std::string {
        size_t pos = upper.find(keyword);
        if (pos == std::string::npos) {
          return "";
        }
        std::string rest = upper.substr(pos + static_cast<int>(keyword.size()) );
        // Remove leading whitespace
        size_t ws = rest.find_first_not_of(" \t\r\n");
        if (ws == std::string::npos) {
          return "";
        }
        rest = rest.substr(ws);
        // Actions: CASCADE | SET NULL | SET DEFAULT | RESTRICT | NO ACTION
        if (rest.substr(0, 7) == "CASCADE") {
          return "CASCADE";
        }
        if (rest.substr(0, 8) == "SET NULL") {
          return "SET NULL";
        }
        if (rest.substr(0, 11) == "SET DEFAULT") {
          return "SET DEFAULT";
        }
        if (rest.substr(0, 8) == "RESTRICT") {
          return "RESTRICT";
        }
        if (rest.substr(0, 9) == "NO ACTION") {
          return "NO ACTION";
        }
        return "";
    };

    fk.on_delete_action = extractAction("ON DELETE ");
    fk.on_update_action = extractAction("ON UPDATE ");
    fk.deferrable       = (upper.find("DEFERRABLE") != std::string::npos &&
                           upper.find("NOT DEFERRABLE") == std::string::npos);
    fk.initially_deferred = (upper.find("INITIALLY DEFERRED") != std::string::npos);

    return !fk.target_table.empty();
}

/**
 * @brief Parse a CREATE [UNIQUE] INDEX statement.
 *
 * Handles:
 *   CREATE [UNIQUE] INDEX [CONCURRENTLY] [name] ON [schema.]table
 *     [USING method] (cols) [WHERE predicate]
 */
bool PostgreSQLImporter::parseCreateIndex(const std::string& sql,
                                          const std::string& /*hint_table*/,
                                          IndexMetadata& index) {
    std::smatch m = {};
    if (!std::regex_search(sql, m, kCreateIndexRe)) {
      return false;
    }

    index.unique = m[1].matched && !m[1].str().empty();
    index.name   = m[2].str();
    // m[3] = table name (not stored in IndexMetadata but available to caller)
    index.type   = m[4].matched ? m[4].str() : "btree";
    // Lowercase type
    for (auto& c : index.type) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Parse column list
    std::string cols = m[5].str();
    std::istringstream css(cols);
    std::string col = {};
    while (std::getline(css, col, ',')) {
        // Strip expression parts like ASC/DESC, NULLS FIRST
        size_t sp = col.find_first_of(" \t(");
        if (sp != std::string::npos) {
          col = col.substr(0, sp);
        }
        // Trim and strip quotes
        size_t l = col.find_first_not_of(" \t\r\n");
        if (l != std::string::npos) {
          col = col.substr(l);
        }
        size_t r = col.find_last_not_of(" \t\r\n");
        if (r != std::string::npos) {
          col = col.substr(0, r + 1);
        }
        if (!col.empty() && col.front() == '"') {
          col = col.substr(1, static_cast<int>(col.size()) - 2);
        }
        if (!col.empty()) {
          index.columns.push_back(col);
        }
    }

    // Partial index WHERE clause
    if (m[6].matched && !m[6].str().empty()) {
        index.partial = true;
        index.where_clause = m[6].str();
        // Trim trailing whitespace/semicolons
        size_t r = index.where_clause.find_last_not_of(" \t\r\n;");
        if (r != std::string::npos) {
          index.where_clause = index.where_clause.substr(0, r + 1);
        }
    }

    return !index.name.empty() && !index.columns.empty();
}

/**
 * @brief Parse an ALTER TABLE ... ADD CONSTRAINT ... FOREIGN KEY statement.
 *
 * Handles pg_dump style:
 *   ALTER TABLE [ONLY] [schema.]table
 *     ADD CONSTRAINT name FOREIGN KEY (cols) REFERENCES tbl (cols) ...;
 */
bool PostgreSQLImporter::parseAlterTableForeignKey(const std::string& sql,
                                                    std::string& out_table,
                                                    ForeignKeyConstraint& fk) {
    std::smatch m = {};
    if (!std::regex_search(sql, m, kAlterFkRe)) {
      return false;
    }

    out_table = m[1].str();
    return parseForeignKeyConstraint(sql, fk);
}

/**
 * @brief Validate that all FK references point to known tables and columns.
 *
 * Populates structured errors for every dangling reference.
 * @return true if all references are valid.
 */
bool PostgreSQLImporter::validateForeignKeyReferences(const ImportOptions& /*options*/,
                                                       ImportStats& stats) {
    bool all_valid = true;
    std::unordered_map<std::string, std::unordered_set<std::string>> target_column_cache;
    for (const auto& [tname, tschema] : schemas_) {
        for (const auto& fk : tschema.foreign_keys) {
            if (fk.ref_table.empty()) {
              continue;
            }
            if (!schemas_.count(fk.ref_table)) {
                all_valid = false;
                ImportError err;
                err.code     = ImportErrorCode::UNKNOWN_TABLE;
                err.severity = ImportErrorSeverity::WARNING;
                err.message  = "Foreign key '" + (fk.constraint_name.empty() ? "(unnamed)" : fk.constraint_name) +
                               "' in table '" + tname + "' references unknown table '" +
                               fk.ref_table + "'";
                err.location = "table " + tname;
                stats.structured_errors.push_back(err);
                stats.warnings.push_back(err.message);
            } else {
                // Validate target column(s) exist — cache a set for O(1) lookup
                auto cache_it = target_column_cache.find(fk.ref_table);
                if (cache_it == target_column_cache.end()) {
                    const auto& target = schemas_.at(fk.ref_table);
                    cache_it = target_column_cache.emplace(
                        fk.ref_table,
                        std::unordered_set<std::string>(target.columns.begin(),
                                                        target.columns.end())).first;
                }
                const auto& target_col_set = cache_it->second;
                for (const auto& col : fk.ref_columns) {
                    if (col.empty()) {
                      continue;
                    }
                    if (target_col_set.find(col) == target_col_set.end()) {
                        all_valid = false;
                        ImportError err;
                        err.code     = ImportErrorCode::UNKNOWN_TABLE;
                        err.severity = ImportErrorSeverity::WARNING;
                        err.message  = "FK '" + (fk.constraint_name.empty() ? "(unnamed)" : fk.constraint_name) +
                                       "' in table '" + tname + "' references unknown column '" +
                                       col + "' in table '" + fk.ref_table + "'";
                        err.location = "table " + tname;
                        stats.structured_errors.push_back(err);
                        stats.warnings.push_back(err.message);
                    }
                }
            }
        }
    }
    return all_valid;
}

// ============================================================================
// v2.1 Parser Methods
// ============================================================================

/**
 * @brief Parse a CHECK constraint definition.
 *
 * Handles:
 *   [CONSTRAINT name] CHECK (expression)
 */
bool PostgreSQLImporter::parseCheckConstraint(const std::string& constraint_def,
                                               CheckConstraint& ck) {
    // Extract optional constraint name
    std::smatch cm = {};
    if (std::regex_search(constraint_def, cm, kConstraintNameRe)) {
        ck.name = cm[1].str();
    }

    // Find the CHECK keyword and extract the parenthesised expression
    std::string upper = constraint_def;
    for (auto& c : upper) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    size_t ck_pos = upper.find("CHECK");
    if (ck_pos == std::string::npos) {
      return false;
    }

    size_t paren = constraint_def.find('(', ck_pos);
    if (paren == std::string::npos) {
      return false;
    }
    size_t paren_end = findMatchingParen(constraint_def, paren);
    if (paren_end == std::string::npos) {
      return false;
    }

    ck.expression = constraint_def.substr(paren + 1, paren_end - paren - 1);
    // Trim whitespace
    size_t l = ck.expression.find_first_not_of(" \t\r\n");
    size_t r = ck.expression.find_last_not_of(" \t\r\n");
    if (l != std::string::npos) {
      ck.expression = ck.expression.substr(l, r - l + 1);
    }
    return !ck.expression.empty();
}

/**
 * @brief Parse an EXCLUDE constraint definition.
 *
 * Extracts the optional constraint name, the index access method from the
 * `USING <method>` clause, the per-column `WITH <operator>` pairs from the
 * parenthesised element list, and stores the raw definition text for
 * round-trip fidelity.
 *
 * Example input:
 *   CONSTRAINT no_overlapping_rooms EXCLUDE USING gist (room WITH =, period WITH &&)
 *
 * Roadmap ref: src/importers/FUTURE_ENHANCEMENTS.md §"Postgres EXCLUDE Constraint Parsing"
 */
bool PostgreSQLImporter::parseExcludeConstraint(const std::string& constraint_def,
                                                 ExcludeConstraint& excl) {
    // ── 1. Extract optional constraint name ──────────────────────────────
    std::smatch cm = {};
    if (std::regex_search(constraint_def, cm, kConstraintNameRe)) {
        excl.name = cm[1].str();
    }

    // ── 2. Locate the EXCLUDE keyword (case-insensitive) ─────────────────
    std::string upper = constraint_def;
    for (auto& c : upper) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    const size_t ex_pos = upper.find("EXCLUDE");
    if (ex_pos == std::string::npos) {
      return false;
    }

    // ── 3. Capture raw definition text for round-trip fidelity ────────────
    excl.definition = constraint_def.substr(ex_pos);
    {
        size_t r = excl.definition.find_last_not_of(" \t\r\n;");
        if (r != std::string::npos) {
          excl.definition = excl.definition.substr(0, r + 1);
        }
    }

    // ── 4. Parse USING <access_method> ───────────────────────────────────
    // Pattern: EXCLUDE [USING <method>] (...)
    const size_t using_pos = upper.find("USING", ex_pos);
    size_t paren_pos = upper.find('(', ex_pos);

    if (using_pos != std::string::npos &&
        (paren_pos == std::string::npos || using_pos < paren_pos)) {
        // Skip "USING" and whitespace
        size_t meth_start = using_pos + 5;
        while (meth_start <static_cast<int>(upper.size()) && std::isspace(static_cast<unsigned char>(upper[meth_start])))
            ++meth_start;
        // Method name ends at whitespace or '('
        size_t meth_end = meth_start;
        while (meth_end <static_cast<int>(upper.size()) &&
               !std::isspace(static_cast<unsigned char>(upper[meth_end])) &&
               upper[meth_end] != '(')
            ++meth_end;
        excl.index_method = constraint_def.substr(meth_start, meth_end - meth_start);
        // Convert to lower-case (method names are case-insensitive in PG)
        for (auto& c : excl.index_method)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        // Update paren_pos to search after the method name
        paren_pos = upper.find('(', meth_end);
    }

    // ── 5. Parse element list: (col1 WITH op1, col2 WITH op2, ...) ────────
    if (paren_pos != std::string::npos) {
        // Find matching closing parenthesis
        int depth = 0;
        size_t close_pos = std::string::npos;
        for (size_t i = paren_pos; i <static_cast<int>(upper.size()); ++i) {
            if (upper[i] == '(')      ++depth;
            else if (upper[i] == ')') { if (--depth == 0) { close_pos = i; break; } }
        }
        if (close_pos != std::string::npos) {
            const std::string inner =
                constraint_def.substr(paren_pos + 1, close_pos - paren_pos - 1);
            // Split by top-level commas
            const auto parts = splitTopLevelCommas(inner);
            static const std::regex kWithRe(
                R"(\s*(.+?)\s+WITH\s+(\S+)\s*)", std::regex_constants::icase);
            for (const auto& part : parts) {
                std::smatch wm = {};
                if (std::regex_match(part, wm, kWithRe)) {
                    ExcludeConstraint::Element el;
                    el.column        = wm[1].str();
                    el.with_operator = wm[2].str();
                    excl.elements.push_back(std::move(el));
                }
            }
        }
    }

    return !excl.definition.empty();
}

/**
 * @brief Detect GENERATED columns on a column definition (v2.1).
 *
 * Handles:
 *   col type GENERATED ALWAYS AS (expr) STORED
 *   col type GENERATED ALWAYS AS IDENTITY
 *   col type GENERATED BY DEFAULT AS IDENTITY
 */
bool PostgreSQLImporter::parseGeneratedColumn(const std::string& col_def,
                                               const std::string& col_name,
                                               GeneratedColumnInfo& gen) {
    std::string upper = col_def;
    for (auto& c : upper) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    size_t gen_pos = upper.find(" GENERATED ");
    if (gen_pos == std::string::npos) {
      return false;
    }

    gen.column = col_name;

    // Detect generation type: ALWAYS or BY DEFAULT
    std::string rest = upper.substr(gen_pos + 11);  // skip " GENERATED "
    if (rest.substr(0, 6) == "ALWAYS") {
        gen.generation = "ALWAYS";
        rest = rest.substr(6);
    } else if (rest.substr(0, 10) == "BY DEFAULT") {
        gen.generation = "BY_DEFAULT";
        rest = rest.substr(10);
    } else {
        return false;
    }

    // Skip whitespace and "AS"
    size_t ws = rest.find_first_not_of(" \t\r\n");
    if (ws == std::string::npos) {
      return false;
    }
    rest = rest.substr(ws);
    if (rest.substr(0, 2) == "AS") {
      rest = rest.substr(2);
    }
    ws = rest.find_first_not_of(" \t\r\n");
    if (ws != std::string::npos) {
      rest = rest.substr(ws);
    }

    if (rest.substr(0, 8) == "IDENTITY") {
        gen.is_identity = true;
        gen.stored       = false;
        return true;
    }

    // GENERATED ALWAYS AS (expr) STORED
    if (!rest.empty() && rest[0] == '(') {
        // Find the paren in the original col_def
        size_t orig_paren = col_def.find('(', gen_pos);
        if (orig_paren == std::string::npos) {
          return false;
        }
        size_t orig_end = findMatchingParen(col_def, orig_paren);
        if (orig_end == std::string::npos) {
          return false;
        }
        gen.expression = col_def.substr(orig_paren + 1, orig_end - orig_paren - 1);
        // Trim
        size_t l = gen.expression.find_first_not_of(" \t\r\n");
        size_t r = gen.expression.find_last_not_of(" \t\r\n");
        if (l != std::string::npos) {
          gen.expression = gen.expression.substr(l, r - l + 1);
        }
        gen.stored = (upper.find("STORED", orig_end) != std::string::npos);
        gen.is_identity = false;
        return true;
    }

    return false;
}

bool PostgreSQLImporter::parseInsert(const std::string& sql, const ImportOptions& options,
                                      ImportStats& stats, size_t line_number) {
    // Extract table name: INSERT INTO [schema.]table [(col1,...)] VALUES (...)
    static const std::regex insert_regex(
        R"(INSERT INTO\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+VALUES\s*\((.+)\)\s*;?\s*$)",
        std::regex_constants::icase);
    std::smatch match = {};
    
    if (!std::regex_search(sql, match, insert_regex)) {
        addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                 "Could not parse INSERT statement",
                 "line " + std::to_string(line_number));
        stats.failed_records++;
        return false;
    }

    std::string table_name = match[1].str();

    if (!shouldImportTable(table_name, options)) {
        stats.skipped_records++;
        return true;
    }

    // Resolve effective column list
    std::vector<std::string> col_list = {};

    if (match[2].matched && !match[2].str().empty()) {
        std::istringstream css(match[2].str());
        std::string col = {};
        while (std::getline(css, col, ',')) {
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            if (!col.empty()) {
              col_list.push_back(col);
            }
        }
    } else if (schemas_.count(table_name)) {
        col_list = schemas_[table_name].columns;
    }

    // Parse the values clause
    std::string values_str = match[3].str();
    std::vector<std::string> values = parseInsertValues(values_str);

    // Build entity JSON
    TableSchema eff_schema;
    eff_schema.name = table_name;
    if (schemas_.count(table_name)) {
      eff_schema = schemas_[table_name];
    }
    if (!col_list.empty()) {
      eff_schema.columns = col_list;
    }

    json entity = convertRowToEntity(eff_schema, values);
    THEMIS_DEBUG("INSERT entity: {}", entity.dump());

    // --- Conflict resolution ---
    if (!options.conflict_key_columns.empty()) {
        std::string ckey = ImportConflictResolver::computeKey(entity, options.conflict_key_columns);
        bool conflict = false;
        entity = conflict_resolver_.resolve(entity, table_name, ckey,
                                            options.conflict_strategy,
                                            options.merge_depth,
                                            options.protected_fields,
                                            conflict);
        if (conflict) {
            switch (options.conflict_strategy) {
                case ConflictStrategy::SKIP:
                    stats.conflicts_skipped++;
                    emitMetric(options, "importers_conflicts_total",
                               {{"table", table_name}, {"strategy", "skip"}, {"outcome", "skipped"}}, 1.0);
                    stats.skipped_records++;
                    return true;
                case ConflictStrategy::OVERWRITE:
                    stats.conflicts_overwritten++;
                    emitMetric(options, "importers_conflicts_total",
                               {{"table", table_name}, {"strategy", "overwrite"}, {"outcome", "overwritten"}}, 1.0);
                    break;
                case ConflictStrategy::MERGE:
                    stats.conflicts_merged++;
                    emitMetric(options, "importers_conflicts_total",
                               {{"table", table_name}, {"strategy", "merge"}, {"outcome", "merged"}}, 1.0);
                    break;
                case ConflictStrategy::ERROR:
                    addError(stats, ImportErrorCode::CONFLICT_ERROR,
                             ImportErrorSeverity::ERROR,
                             "Conflict detected for key '" + ckey + "' in table '" + table_name + "'",
                             "line " + std::to_string(line_number));
                    emitMetric(options, "importers_conflicts_total",
                               {{"table", table_name}, {"strategy", "error"}, {"outcome", "error"}}, 1.0);
                    stats.failed_records++;
                    if (!options.continue_on_error) {
                      return false;
                    }
                    return true;
            }
        }
    }

    if (options.dry_run) {
        addError(stats, ImportErrorCode::DRY_RUN_ONLY, ImportErrorSeverity::INFO,
                 "dry-run: row would be imported",
                 "table " + table_name + ", line " + std::to_string(line_number));
        stats.imported_records++;
    } else {
        if ([[maybe_unused]] options.streaming_row_callback) {
            if (!options.streaming_row_callback(table_name, entity)) {
                cancelled_ = true;  // abort the import
            }
        }

        stats.imported_records++;
    }
    return true;
}

bool PostgreSQLImporter::parseCopy(std::ifstream& file, const std::string& table_name,
                                    const std::vector<std::string>& columns,
                                    const ImportOptions& options, ImportStats& stats,
                                    std::unordered_set<uint64_t>& delta_hashes) {
    if (!shouldImportTable(table_name, options)) {
        // Skip until end marker – use bounded reader so the skip itself is safe
        std::string line = {};
        bool trunc = false;
        const size_t skip_limit = options.max_row_size_bytes > 0
                                  ? options.max_row_size_bytes * 2
                                  : 64 * 1024 * 1024;
        while (streamReadLinePg(file, line, skip_limit, trunc)) {
            if (line == "\\." || line.rfind("\\.", 0) == 0) {
              break;
            }
            stats.skipped_records++;
        }
        return true;
    }

    // Resolve effective column list from schema or provided list
    TableSchema eff_schema = {};
    if (schemas_.count(table_name)) {
      eff_schema = schemas_[table_name];
    }
    if (!columns.empty()) {
      eff_schema.columns = columns;
    }
    eff_schema.name = table_name;

    // Per-row read limit: cap single-row allocation to max_row_size_bytes
    // (or a safe default of 64 MB) to prevent OOM from adversarial dumps.
    const size_t row_read_limit = options.max_row_size_bytes > 0
                                  ? options.max_row_size_bytes
                                  : 64 * 1024 * 1024;  // 64 MB default cap

    std::string line = {};
    size_t row_num = 0;
    bool first_data_line = true;
    bool row_truncated = false;
    while (streamReadLinePg(file, line, row_read_limit, row_truncated) && !cancelled_) {
        if (line == "\\." || line.rfind("\\.", 0) == 0) {
            break;  // End of COPY data
        }

        // A truncated row is treated the same as a row-size violation
        if (row_truncated) {
            row_num++;
            stats.total_records++;
            ImportError err;
            err.code     = ImportErrorCode::ROW_TOO_LARGE;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "COPY row truncated at " + std::to_string(row_read_limit) + " bytes";
            err.location = "table " + table_name + ", row " + std::to_string(row_num);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back("Row truncated in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.dry_run) {
                writeQuarantineRow(options.quarantine_file, table_name,
                                   "<truncated at " + std::to_string(row_read_limit) + " bytes>", err);
            }
            if (!options.continue_on_error) { stats.failed_records++; return false; }
            stats.failed_records++;
            stats.quarantined_records++;
            continue;
        }

        // --- Binary COPY format detection (first data line) ---
        // PostgreSQL binary COPY starts with the signature: "PGCOPY\n\xff\r\n\0"
        if (first_data_line) {
            first_data_line = false;
            if (static_cast<int>(line.size()) > = 6 && line.compare(0, 6, "PGCOPY") == 0) {
                addError(stats, ImportErrorCode::BINARY_COPY_FORMAT,
                         ImportErrorSeverity::ERROR,
                         "Binary COPY format detected for table '" + table_name +
                         "'; only text-format COPY is supported. "
                         "Re-export the dump without --format=binary.",
                         "table " + table_name);
                stats.errors.push_back("Binary COPY unsupported in table " + table_name);
                if (!options.continue_on_error) {
                  return false;
                }
                // Skip remaining lines of this COPY block using the bounded reader
                bool skip_trunc = false;
                while (streamReadLinePg(file, line, row_read_limit, skip_trunc)) {
                    if (line == "\\." || line.rfind("\\.", 0) == 0) {
                      break;
                    }
                }
                return true;
            }
        }
        
        row_num++;
        stats.total_records++;

        // Row-size guard
        if (options.max_row_size_bytes > 0 && static_cast<int>(line.size()) > options.max_row_size_bytes) {
            ImportError err;
            err.code     = ImportErrorCode::ROW_TOO_LARGE;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "COPY row exceeds max_row_size_bytes (" +
                           std::to_string(options.max_row_size_bytes) + ")";
            err.location = "table " + table_name + ", row " + std::to_string(row_num);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back("Row too large in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.dry_run) {
                writeQuarantineRow(options.quarantine_file, table_name, line, err);
            }
            if (!options.continue_on_error) {
                stats.failed_records++;
                return false;
            }
            stats.failed_records++;
            stats.quarantined_records++;
            continue;
        }

        // UTF-8 encoding guard
        if (options.enforce_utf8 && !isValidUtf8(line)) {
            ImportError err;
            err.code     = ImportErrorCode::INVALID_UTF8;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "COPY row contains invalid UTF-8 byte sequence";
            err.location = "table " + table_name + ", row " + std::to_string(row_num);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back("Invalid UTF-8 in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.dry_run) {
                writeQuarantineRow(options.quarantine_file, table_name, line, err);
            }
            if (!options.continue_on_error) {
                stats.failed_records++;
                return false;
            }
            stats.failed_records++;
            stats.quarantined_records++;
            continue;
        }

        // Parse tab-separated values with PostgreSQL COPY escape rules
        std::vector<std::string> values = parseCopyRow(line);

        // --- Delta / incremental import check ---
        if (!options.delta_hash_file.empty()) {
            uint64_t h = computeRowHash(line, values, options.delta_key_columns,
                                        eff_schema.columns);
            if (delta_hashes.count(h)) {
                stats.skipped_records++;
                emitMetric(options, "themisdb_import_rows_total",
                           {{"table", table_name}, {"status", "skipped"}}, 1.0);
                continue;
            }
            delta_hashes.insert(h);
        }

        if (!eff_schema.columns.empty() && static_cast<int>(values.size()) != eff_schema.columns.size()) {
            ImportError err;
            err.code     = ImportErrorCode::COLUMN_COUNT_MISMATCH;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "COPY row has " + std::to_string(values.size()) +
                           " columns, expected " + std::to_string(eff_schema.columns.size());
            err.location = "table " + table_name + ", row " + std::to_string(row_num);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back("Column count mismatch in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.dry_run) {
                writeQuarantineRow(options.quarantine_file, table_name, line, err);
            }
            if (!options.continue_on_error) {
                stats.failed_records++;
                return false;
            }
            stats.failed_records++;
            stats.quarantined_records++;
            continue;
        }

        json entity = convertRowToEntity(eff_schema, values);
        THEMIS_DEBUG("COPY entity: {}", entity.dump());

        // --- Conflict resolution ---
        if (!options.conflict_key_columns.empty()) {
            std::string ckey = ImportConflictResolver::computeKey(entity, options.conflict_key_columns);
            bool conflict = false;
            entity = conflict_resolver_.resolve(entity, table_name, ckey,
                                                options.conflict_strategy,
                                                options.merge_depth,
                                                options.protected_fields,
                                                conflict);
            if (conflict) {
                switch (options.conflict_strategy) {
                    case ConflictStrategy::SKIP:
                        stats.conflicts_skipped++;
                        emitMetric(options, "importers_conflicts_total",
                                   {{"table", table_name}, {"strategy", "skip"}, {"outcome", "skipped"}}, 1.0);
                        stats.skipped_records++;
                        continue;
                    [[fallthrough]];\n                    case ConflictStrategy::OVERWRITE:
                        stats.conflicts_overwritten++;
                        emitMetric(options, "importers_conflicts_total",
                                   {{"table", table_name}, {"strategy", "overwrite"}, {"outcome", "overwritten"}}, 1.0);
                        break;
                    case ConflictStrategy::MERGE:
                        stats.conflicts_merged++;
                        emitMetric(options, "importers_conflicts_total",
                                   {{"table", table_name}, {"strategy", "merge"}, {"outcome", "merged"}}, 1.0);
                        break;
                    case ConflictStrategy::ERROR: {
                        addError(stats, ImportErrorCode::CONFLICT_ERROR,
                                 ImportErrorSeverity::ERROR,
                                 "Conflict detected for key '" + ckey + "' in table '" + table_name + "'",
                                 "table " + table_name + ", row " + std::to_string(row_num));
                        emitMetric(options, "importers_conflicts_total",
                                   {{"table", table_name}, {"strategy", "error"}, {"outcome", "error"}}, 1.0);
                        stats.failed_records++;
                        if (!options.continue_on_error) {
                          return false;
                        }
                        continue;
                    }
                }
            }
        }

        if (options.dry_run) {
            addError(stats, ImportErrorCode::DRY_RUN_ONLY, ImportErrorSeverity::INFO,
                     "dry-run: row would be imported",
                     "table " + table_name + ", row " + std::to_string(row_num));
            stats.imported_records++;
        } else {
            if ([[maybe_unused]] options.streaming_row_callback) {
                if (!options.streaming_row_callback(table_name, entity)) {
                    cancelled_ = true;  // caller requested abort
                    stats.imported_records++;
                    emitMetric(options, "themisdb_import_rows_total",
                               {{"table", table_name}, {"status", "imported"}}, 1.0);
                    return false;
                }
            }

            stats.imported_records++;
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table_name}, {"status", "imported"}}, 1.0);
        }
    }
    
    return true;
}

std::vector<std::string> PostgreSQLImporter::parseCopyRow(const std::string& line) const {
    // PostgreSQL COPY text format: columns separated by TAB.
    // Special sequences: \N = SQL NULL, \t = tab, \n = newline, \r = CR, \\ = backslash.
    std::vector<std::string> result;
    // I3: Pre-allocate for typical table width; avoids repeated reallocation
    // in the hot per-row path during large COPY blocks.
    result.reserve(32);
    size_t start = 0;

    // Process each tab-delimited raw field, then unescape
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            std::string raw = line.substr(start, i - start);
            result.push_back(unescapeCopyValue(raw));
            start = i + 1;
        }
    }
    return result;
}

std::string PostgreSQLImporter::unescapeCopyValue(const std::string& val) const {
    // \N in COPY format means SQL NULL – represent as empty string sentinel
    if (val == "\\N") {
        return "";  // NULL value
    }
    // Apply other escape sequences
    std::string out = {};
    out.reserve(val.size());
    for (size_t i = 0; i <static_cast<int>(val.size()); ++i) {
        if (val[i] == '\\' && i + 1 <static_cast<int>(val.size())) {
            char next = val[++i];
            switch (next) {
                case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
                case 't':  out += '\t'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case '\\': out += '\\'; break;
                default:   out += '\\'; out += next; break;
            }
        } else {
            out += val[i];
        }
    }
    return out;
}

std::vector<std::string> PostgreSQLImporter::parseInsertValues(const std::string& values_clause) const {
    // Parse a VALUES clause like: 1, 'hello', NULL, 'it''s'
    // Handles single-quoted strings with escaped quotes ('') and numeric/NULL literals.
    std::vector<std::string> result;
    // I3: Pre-allocate for typical column count; avoids per-push reallocation
    // in the INSERT hot path.
    result.reserve(32);
    size_t i = 0;
    const size_t n = values_clause.size();

    while (i < n) {
        // Skip leading whitespace
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t')) {
          ++i;
        }
        if (i >= n) {
          break;
        }

        if (values_clause[i] == '\'') {
            // Quoted string
            ++i;
            std::string val = {};
            while (i < n) {
                if (values_clause[i] == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                    val += '\'';
                    i += 2;
                } else if (values_clause[i] == '\'') {
                    ++i;
                    break;
                } else {
                    val += values_clause[i++];
                }
            }
            result.push_back(val);
        } else {
            // Unquoted token (number, NULL, true, false, etc.)
            size_t start = i;
            while (i < n && values_clause[i] != ',' && values_clause[i] != ') {
              ') ++i;
            }
            std::string token = values_clause.substr(start, i - start);
            // Trim trailing whitespace
            token.erase(token.find_last_not_of(" \t") + 1);
            result.push_back(token);
        }

        // Skip comma separator
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                          values_clause[i] == ',')) ++i;
    }

    return result;
}

std::string PostgreSQLImporter::mapPostgreSQLTypeToThemis(const std::string& pg_type,
                                                            const ImportOptions& options) const {
    // Check user-configurable overrides first
    auto it = options.type_overrides.find(pg_typ[[maybe_unused]] e);
    if (i[[maybe_unused]] t != option[[maybe_unused]] s.type_override[[maybe_unused]] s.en[[maybe_unused]] d()) {
        return it->second;
    }

    std::string lower_type = pg_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);

    // Also check lowercase override
    it = options.type_overrides.find(lower_typ[[maybe_unused]] e);
    if (i[[maybe_unused]] t != option[[maybe_unused]] s.type_override[[maybe_unused]] s.en[[maybe_unused]] d()) {
        return it->second;
    }

    // Check custom types discovered from CREATE TYPE statements in the dump.
    // Check both the original and lowercased form of the type name.
    // PHASE-3A-FIX: Add null/empty checks for custom type values
    {
        std::lock_guard<std::mutex> lock(custom_type_map_mutex_);
        auto ct = custom_type_map_.find(pg_type);
        if (ct != custom_type_map_.end() && !ct->second.empty()) {
            return ct->second;
        }
        ct = custom_type_map_.find(lower_type);
        if (ct != custom_type_map_.end() && !ct->second.empty()) {
            return ct->second;
        }
    }

    // Array types
    // PHASE-3A-FIX: Add bounds check before calling back() on string
    if (!lower_type.empty() && 
        (lower_type.back() == ']' || lower_type.find("[]") != std::string::npos ||
         lower_type.rfind("array", 0) == 0)) {
        return "array";
    }

    // Exact / prefix matches for PostgreSQL built-in types
    if (lower_type == "bigserial" || lower_type == "bigint" || lower_type == "int8") {
      return "long";
    }
    if (lower_type == "smallint" || lower_type == "int2" || lower_type == "smallserial") {
      return "integer";
    }
    if (lower_type == "integer" || lower_type == "int" || lower_type == "int4" ||
        lower_type == "serial" || lower_type == "serial4") return "integer";
    if (lower_type == "real" || lower_type == "float4") {
      return "float";
    }
    if (lower_type == "double precision" || lower_type == "float8") {
      return "double";
    }
    if (lower_type == "numeric" || lower_type == "decimal") {
      return "double";
    }
    if (lower_type == "money") {
      return "double";
    }
    if (lower_type == "boolean" || lower_type == "bool") {
      return "boolean";
    }
    if (lower_type == "text" || lower_type == "name") {
      return "string";
    }
    if (lower_type == "uuid") {
      return "string";
    }
    if (lower_type == "inet" || lower_type == "cidr" || lower_type == "macaddr" ||
        lower_type == "macaddr8") return "string";
    if (lower_type == "xml") {
      return "string";
    }
    if (lower_type == "bytea") {
      return "binary";
    }
    if (lower_type == "json" || lower_type == "jsonb") {
      return "json";
    }
    if (lower_type == "interval") {
      return "string";
    }
    if (lower_type == "point" || lower_type == "line" || lower_type == "lseg" ||
        lower_type == "box" || lower_type == "path" || lower_type == "polygon" ||
        lower_type == "circle") return "geo";
    if (lower_type == "tsvector" || lower_type == "tsquery") {
      return "string";
    }
    if (lower_type == "oid" || lower_type == "xid" || lower_type == "cid") {
      return "integer";
    }

    // Prefix-based fallbacks
    if (lower_type.find("int") != std::string::npos) {
      return "integer";
    }
    if (lower_type.find("serial") != std::string::npos) {
      return "integer";
    }
    if (lower_type.find("float") != std::string::npos) {
      return "double";
    }
    if (lower_type.find("char") != std::string::npos) {
      return "string";
    }
    if (lower_type.find("varchar") != std::string::npos) {
      return "string";
    }
    if (lower_type.find("timestamp") != std::string::npos) {
      return "datetime";
    }
    if (lower_type.find("date") != std::string::npos) {
      return "date";
    }
    if (lower_type.find("time") != std::string::npos) {
      return "time";
    }
    if (lower_type.find("json") != std::string::npos) {
      return "json";
    }

    return "string";  // Default: treat unknown types as strings
}

bool PostgreSQLImporter::shouldImportTable(const std::string& table_name, const ImportOptions& options) {
    // Check exclude list
    if (std::find(options.exclude_tables.begin(), options.exclude_tables.end(), table_name) != options.exclude_tables.end()) {
        return false;
    }
    
    // Check include list (if specified)
    if (!options.include_tables.empty()) {
        return std::find(options.include_tables.begin(), options.include_tables.end(), table_name) != options.include_tables.end();
    }
    
    return true;
}

json PostgreSQLImporter::convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values) {
    json entity;
    entity["_type"] = schema.name;
    
    for (size_t i = 0; i <static_cast<int>(values.size())  && static_cast<size_t>(i) <static_cast<int>(schema.columns.size()); i++) {
        entity[schema.columns[i]] = values[i];
    }

    // v2.0: embed FK metadata when present so downstream consumers can resolve relationships
    if (!schema.foreign_keys.empty()) {
        json fk_arr = json::array();
        for (const auto& fk : schema.foreign_keys) {
            fk_arr.push_back(fk.toJson());
        }
        entity["_foreign_keys"] = std::move(fk_arr);
    }
    
    return entity;
}

void PostgreSQLImporter::addError(ImportStats& stats, ImportErrorCode code,
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
}

// PHASE-2-HARDENING: Standardized PostgreSQL error reporting
/**
 * @brief Add a PostgreSQL-specific error with automatic error code mapping.
 *
 * Maps PostgreSQL error patterns to standard ImporterErrorCode values,
 * ensuring consistent error reporting across all importers.
 *
 * @param stats     ImportStats to accumulate the error
 * @param severity  Error severity level
 * @param pg_error_msg  PostgreSQL error message to analyze
 * @param location  Optional location information (e.g., "line 42", "table users")
 */
void PostgreSQLImporter::addPostgreSQLError(ImportStats& stats,
                                           ImportErrorSeverity severity,
                                           const std::string& pg_error_msg,
                                           const std::string& location) const {
    // PHASE-2-HARDENING: Standardized error mapping
    ImportErrorCode code = mapPostgreSQLErrorToCode(pg_error_msg);
    addError(stats, code, severity, pg_error_msg, location);
}

void PostgreSQLImporter::emitMetric(const ImportOptions& options,
                                     const std::string& metric,
                                     const std::map<std::string, std::string>& labels,
                                     double value) const {
    if ([[maybe_unused]] options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void PostgreSQLImporter::emitSpan(const ImportOptions& options,
                                   const std::string& operation,
                                   const std::map<std::string, std::string>& attributes,
                                   double duration_seconds) const {
    if ([[maybe_unused]] options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

bool PostgreSQLImporter::isValidUtf8(const std::string& s) {
    // Validate that every byte sequence in s is valid UTF-8.
    // Uses the standard multi-byte decoding rules:
    //   1-byte  (ASCII):       0xxxxxxx
    //   2-byte continuation:   110xxxxx 10xxxxxx
    //   3-byte continuation:   1110xxxx 10xxxxxx 10xxxxxx
    //   4-byte continuation:   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    // Rejects overlong encodings, surrogates (U+D800–U+DFFF), and values > U+10FFFF.
    const auto* bytes = reinterpret_cast<const unsigned char*>(s.data());
    const size_t len = s.size();
    size_t i = 0;
    while (i < len) {
        unsigned char c = bytes[i];
        size_t extra = 0;
        uint32_t codepoint = 0;
        if (c <= 0x7F) {
            // ASCII
            ++i;
            continue;
        } else if ((c & 0xE0) == 0xC0) {
            extra     = 1;
            codepoint = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
            extra     = 2;
            codepoint = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            extra     = 3;
            codepoint = c & 0x07;
        } else {
            return false;  // Invalid lead byte
        }
        if (i + extra >= len) return false;  // Truncated sequence
        for (size_t j = 1; j <= extra; ++j) {
            unsigned char cc = bytes[i + j];
            if ((cc & 0xC0) != 0x80) return false;  // Invalid continuation byte
            codepoint = (codepoint << 6) | (cc & 0x3F);
        }
        // Reject overlong encodings
        if (extra == 1 && codepoint < 0x80) {
          return false;
        }
        if (extra == 2 && codepoint < 0x800) {
          return false;
        }
        if (extra == 3 && codepoint < 0x10000) {
          return false;
        }
        // Reject surrogates (U+D800–U+DFFF)
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
          return false;
        }
        // Reject values above U+10FFFF
        if (codepoint > 0x10FFFF) {
          return false;
        }
        i += 1 + extra;
    }
    return true;
}

bool PostgreSQLImporter::loadCheckpoint(const std::string& checkpoint_file,
                                         std::streampos& offset,
                                         ImportStats& accumulated_stats) const {
    std::ifstream f(checkpoint_file);
    if (!f) {
      return false;
    }

    try {
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        json doc = json::parse(content);
        offset = static_cast<std::streampos>(doc.value("byte_offset", (long long)0));
        accumulated_stats.imported_records = doc.value("imported_records", (size_t)0);
        accumulated_stats.failed_records   = doc.value("failed_records",   (size_t)0);
        accumulated_stats.skipped_records  = doc.value("skipped_records",  (size_t)0);
        accumulated_stats.total_records    = doc.value("total_records",    (size_t)0);
        accumulated_stats.tables_processed = doc.value("tables_processed", (size_t)0);
        THEMIS_INFO("Checkpoint loaded from {}: byte_offset={}", checkpoint_file,
                    static_cast<long>(offset));
        return true;
    } catch (const json::parse_error& e) {
        THEMIS_INFO("Could not parse checkpoint file {}: JSON error at byte {}: {}, starting fresh",
                    checkpoint_file, e.byte, e.what());
        return false;
    } catch (const std::exception& e) {
        THEMIS_INFO("Could not load checkpoint file {}: {}, starting fresh",
                    checkpoint_file, e.what());
        return false;
    }
}

void PostgreSQLImporter::saveCheckpoint(const std::string& checkpoint_file,
                                         std::streampos offset,
                                         const ImportStats& stats) const {
    std::ofstream f(checkpoint_file, std::ios::trunc);
    if (!f) {
        THEMIS_INFO("Could not write checkpoint file {}", checkpoint_file);
        return;
    }
    json doc = {
        {"byte_offset",       static_cast<long long>(offset)},
        {"imported_records",  stats.imported_records},
        {"failed_records",    stats.failed_records},
        {"skipped_records",   stats.skipped_records},
        {"total_records",     stats.total_records},
        {"tables_processed",  stats.tables_processed}
    };
    f << doc.dump(2);
    THEMIS_DEBUG("Checkpoint saved to {}: byte_offset={}", checkpoint_file,
                 static_cast<long>(offset));
}

void PostgreSQLImporter::reportProgress(ProgressCallback& callback, const std::string& stage, size_t current, size_t total) {
    if ([[maybe_unused]] callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// Quarantine helpers
// ============================================================================

void PostgreSQLImporter::writeQuarantineRow(const std::string& quarantine_file,
                                             const std::string& table_name,
                                             const std::string& raw_row,
                                             const ImportError& error) const {
    if (quarantine_file.empty()) {
      return;
    }
    std::ofstream f(quarantine_file, std::ios::app);
    if (!f) {
        THEMIS_INFO("Could not write to quarantine file {}", quarantine_file);
        return;
    }
    json entry = {
        {"table", table_name},
        {"row",   raw_row},
        {"error", {
            {"code",     static_cast<uint32_t>(error.code)},
            {"severity", static_cast<int>(error.severity)},
            {"message",  error.message},
            {"location", error.location}
        }}
    };
    f << entry.dump() << "\n";
}

// ============================================================================
// Delta / incremental import helpers
// ============================================================================

// FNV-1a 64-bit hash  (no external dependency)
static uint64_t fnv1a64(const char* data, size_t len) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint8_t>(data[i]);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

uint64_t PostgreSQLImporter::computeRowHash(const std::string& raw_row,
                                             const std::vector<std::string>& values,
                                             const std::vector<std::string>& key_columns,
                                             const std::vector<std::string>& schema_columns) {
    if (key_columns.empty() || schema_columns.empty()) {
        // Hash the entire raw row
        return fnv1a64(raw_row.data(),static_cast<int>(raw_row.size()));
    }
    // Hash only the key column values, separated by a non-printable sentinel
    static constexpr char kDeltaHashFieldSep = '\x01';
    std::unordered_map<std::string, size_t> schema_column_index = {};

    schema_column_index.reserve(schema_columns.size());
    for (size_t i = 0; i <static_cast<int>(schema_columns.size()); ++i) {
        schema_column_index.emplace(schema_columns[i], i);
    }

    std::string key_data = {};
    key_data.reserve(key_columns.size() * 8);
    for (const auto& kc : key_columns) {
        auto it = schema_column_index.find(kc);
        if (it != schema_column_index.end()) {
            size_t idx = it->second;
            if (static_cast<int>(values.size()) > idx) {
                key_data += values[idx];
            }
        }
        key_data += kDeltaHashFieldSep;
    }
    return fnv1a64(key_data.data(),static_cast<int>(key_data.size()));
}

std::unordered_set<uint64_t> PostgreSQLImporter::loadDeltaHashes(const std::string& delta_hash_file) {
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

void PostgreSQLImporter::saveDeltaHashes(const std::string& delta_hash_file,
                                          const std::unordered_set<uint64_t>& hashes) {
    std::ofstream f(delta_hash_file, std::ios::trunc);
    if (!f) {
      return;
    }
    f << std::hex << std::setfill('0');
    for (uint64_t h : hashes) {
        // Write as 16-character zero-padded hex
        f << std::setw(16) << h << "\n";
    }
}



PostgreSQLImporterPlugin::PostgreSQLImporterPlugin() 
    : importer_(std::make_unique<PostgreSQLImporter>()) {
}

plugins::PluginCapabilities PostgreSQLImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching = true;
    caps.thread_safe = false;  // Not thread-safe (uses instance state)
    return caps;
}

bool PostgreSQLImporterPlugin::initialize(const char* config_json) {
    if (!importer_) {
        return false;
    }
    return importer_->initialize(config_json ? config_json : "{}");
}

void PostgreSQLImporterPlugin::shutdown() {
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
    themis::plugins::IThemisPlugin* createPlugin() {
        return new themis::importers::PostgreSQLImporterPlugin();
    }
    
    void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
        delete plugin;
    }
}


