/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mssql_importer.cpp                                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          ThemisDB Contributors                              ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~1000                                          ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/mssql_importer.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <regex>
#include <cctype>

namespace themis {
namespace importers {

// ============================================================================
// File-level helpers
// ============================================================================

/**
 * @brief Memory-bounded line reader.
 *
 * Reads the next newline-terminated line from @p file with a hard per-line
 * byte cap of @p max_bytes (0 = unlimited). When the cap is exceeded the
 * remaining bytes of the current line are discarded and @p truncated is set
 * to true. Returns false only when EOF is reached before any bytes are read.
 */
static bool streamReadLine(std::istream& file,
                            std::string& line,
                            size_t max_bytes,
                            bool& truncated) {
    truncated = false;
    line.clear();

    if (max_bytes == 0) {
        if (!std::getline(file, line)) return false;
        return true;
    }

    char c = '\0';
    size_t count = 0;
    bool got_any = false;

    while (file.get(c)) {
        got_any = true;
        if (c == '\n') break;

        if (count < max_bytes) {
            line += c;
            ++count;
        } else {
            truncated = true;
            while (file.get(c) && c != '\n') { /* discard */ }
            break;
        }
    }

    return got_any;
}

/**
 * @brief Convert a string to lower-case (ASCII only).
 */
static std::string toLower(const std::string& s) {
    std::string result = s;
    for (auto& ch : result) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return result;
}

/**
 * @brief Return true if @p s looks like a GO batch separator line
 *        (optional leading whitespace, "GO", optional trailing whitespace/comment).
 */
static bool isGoBatch(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    if (i + 2 > s.size()) return false;
    if (std::toupper(static_cast<unsigned char>(s[i]))     != 'G') return false;
    if (std::toupper(static_cast<unsigned char>(s[i + 1])) != 'O') return false;
    i += 2;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    return (i == s.size() || s[i] == '-' || s[i] == '\r');
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

MSSQLImporter::MSSQLImporter() = default;

MSSQLImporter::~MSSQLImporter() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> MSSQLImporter::getSupportedTypes() const {
    return {"sqlserver", "mssql", "tsql"};
}

bool MSSQLImporter::initialize(const std::string& /*config*/) {
    cancelled_ = false;
    schemas_.clear();
    THEMIS_INFO("SQL Server Importer initialized");
    return true;
}

bool MSSQLImporter::validateSource(const std::string& source_path,
                                   std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }

    // Check for SQL Server dump markers in the first 100 lines.
    std::string line;
    bool found_mssql = false;
    int lines_checked = 0;
    while (std::getline(file, line) && lines_checked < 100) {
        if (line.find("Microsoft SQL Server")         != std::string::npos ||
            line.find("SQL Server")                   != std::string::npos ||
            line.find("SET QUOTED_IDENTIFIER")        != std::string::npos ||
            line.find("SET ANSI_NULLS")               != std::string::npos ||
            line.find("SET IDENTITY_INSERT")          != std::string::npos ||
            line.find("USE [")                        != std::string::npos) {
            found_mssql = true;
            break;
        }
        // A bare GO on a line is also a strong indicator
        if (isGoBatch(line)) {
            found_mssql = true;
            break;
        }
        lines_checked++;
    }

    if (!found_mssql) {
        errors.push_back("File does not appear to be a SQL Server T-SQL script dump");
        return false;
    }

    THEMIS_INFO("SQL Server source validation successful: {}", source_path);
    return true;
}

ImportStats MSSQLImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();

    THEMIS_INFO("Starting SQL Server import from: {}", source_path);
    THEMIS_INFO("Options: {}", options.toJson().dump());

    // --- Permission / ACL check ---
    if (options.permission_check) {
        if (!options.permission_check("import", "write")) {
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

    std::unordered_set<uint64_t> delta_hashes;
    if (!options.delta_hash_file.empty()) {
        delta_hashes = loadDeltaHashes(options.delta_hash_file);
        THEMIS_INFO("Loaded {} delta hashes from {}", delta_hashes.size(),
                    options.delta_hash_file);
    }

    // Parse dump file
    if (!parseDumpFile(source_path, options, stats, progress_callback)) {
        if (stats.structured_errors.empty()) {
            addError(stats, ImportErrorCode::FILE_READ_FAILED,
                     ImportErrorSeverity::CRITICAL, "Failed to parse dump file");
        }
    }

    if (!options.delta_hash_file.empty() && !delta_hashes.empty()) {
        saveDeltaHashes(options.delta_hash_file, delta_hashes);
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

    THEMIS_INFO("Import summary: {}", stats.toJson().dump());
    THEMIS_INFO("Import completed: {} records imported, {} failed, {} skipped in {:.2f}s",
        stats.imported_records, stats.failed_records, stats.skipped_records,
        stats.elapsed_seconds);

    // Prometheus / OTel metrics emission
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

    // OTel span for the entire import
    emitSpan(options, "import_total",
             {{"source", source_path},
              {"tables", std::to_string(stats.tables_processed)},
              {"rows",   std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> MSSQLImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "mssql-import-" + std::to_string(ms) + "-" +
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
            err.message  = std::string("Unhandled exception in async SQL Server import: ") + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async SQL Server import worker";
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

void MSSQLImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("SQL Server import cancelled");
}

json MSSQLImporter::getSourceSchema(const std::string& source_path) {
    schemas_.clear();

    std::ifstream file(source_path);
    if (!file) {
        return json::array();
    }

    std::string line;
    std::string current_sql;

    while (std::getline(file, line)) {
        // Skip GO batch separator and empty/comment lines
        if (isGoBatch(line)) {
            current_sql.clear();
            continue;
        }
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
            continue;
        }

        std::string stripped = stripBlockComments(line);
        {
            size_t first = stripped.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
        }

        current_sql += stripped + " ";

        if (line.find(';') != std::string::npos) {
            std::string upper_prefix;
            for (size_t i = 0; i < current_sql.size() && i < 30; ++i) {
                upper_prefix += static_cast<char>(
                    std::toupper(static_cast<unsigned char>(current_sql[i])));
            }
            if (upper_prefix.find("CREATE TABLE") != std::string::npos) {
                TableSchema schema;
                if (parseCreateTable(current_sql, schema)) {
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

bool MSSQLImporter::parseDumpFile(const std::string& file_path, const ImportOptions& options,
                                   ImportStats& stats, ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    // Detect SQL Server dump header in the first 50 lines.
    {
        std::string hdr_line;
        int hdr_lines = 0;
        bool found_header = false;
        bool hdr_trunc = false;
        while (streamReadLine(file, hdr_line, 4096, hdr_trunc) && hdr_lines < 50) {
            if (hdr_line.find("Microsoft SQL Server")  != std::string::npos ||
                hdr_line.find("SQL Server")            != std::string::npos ||
                hdr_line.find("SET QUOTED_IDENTIFIER") != std::string::npos ||
                hdr_line.find("SET ANSI_NULLS")        != std::string::npos ||
                hdr_line.find("SET IDENTITY_INSERT")   != std::string::npos ||
                hdr_line.find("USE [")                 != std::string::npos ||
                isGoBatch(hdr_line)) {
                found_header = true;
                break;
            }
            hdr_lines++;
        }
        if (!found_header) {
            addError(stats, ImportErrorCode::NOT_A_PG_DUMP, ImportErrorSeverity::CRITICAL,
                     "File does not appear to be a SQL Server T-SQL script dump");
            return false;
        }
        file.clear();
        file.seekg(0);
    }

    const size_t line_read_limit = options.max_statement_size_bytes > 0
                                   ? options.max_statement_size_bytes
                                   : 64 * 1024 * 1024ULL;

    std::string line;
    std::string current_sql;
    size_t line_number = 0;
    size_t batch_row_count = 0;
    bool line_truncated = false;
    std::unordered_set<uint64_t> delta_hashes;
    if (!options.delta_hash_file.empty()) {
        delta_hashes = loadDeltaHashes(options.delta_hash_file);
    }

    while (streamReadLine(file, line, line_read_limit, line_truncated) && !cancelled_) {
        line_number++;

        if (line_truncated) {
            addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "Line too long (> " + std::to_string(line_read_limit) + " bytes); truncated",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back("Line truncated at " + std::to_string(line_number));
            current_sql.clear();
            if (!options.continue_on_error) return false;
            continue;
        }

        // GO batch separator: flush current statement buffer
        if (isGoBatch(line)) {
            current_sql.clear();
            continue;
        }

        // Skip empty lines and SQL line comments (-- …)
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
            continue;
        }

        // Strip T-SQL block comments
        std::string stripped_line = stripBlockComments(line);
        {
            size_t first = stripped_line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
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
            if (!options.continue_on_error) return false;
            continue;
        }

        // Complete statement ends with ';'
        if (stripped_line.find(';') == std::string::npos) {
            continue;
        }

        // Classify the completed statement
        std::string prefix;
        for (size_t i = 0; i < current_sql.size() && i < 30; ++i) {
            prefix += static_cast<char>(std::toupper(
                static_cast<unsigned char>(current_sql[i])));
        }

        if (prefix.find("CREATE TABLE") != std::string::npos) {
            auto t0 = std::chrono::steady_clock::now();
            TableSchema schema;
            if (parseCreateTable(current_sql, schema)) {
                if (shouldImportTable(schema.name, options)) {
                    schemas_[schema.name] = schema;
                    stats.tables_processed++;
                    THEMIS_DEBUG("Parsed SQL Server table schema: {}", schema.name);
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

            if (options.batch_size > 0 && batch_row_count >= options.batch_size) {
                reportProgress(callback, "data", stats.imported_records, 0);
                batch_row_count = 0;
            }
        }
        // SET IDENTITY_INSERT, CREATE INDEX, ALTER TABLE, etc. are ignored.

        current_sql.clear();
    }

    if (!options.delta_hash_file.empty() && !delta_hashes.empty()) {
        saveDeltaHashes(options.delta_hash_file, delta_hashes);
    }

    return !cancelled_;
}

bool MSSQLImporter::parseCreateTable(const std::string& sql, TableSchema& schema) {
    // Match: CREATE TABLE [[schema].]table  or  CREATE TABLE [schema].table  etc.
    // T-SQL allows [schema].[table], "schema"."table", plain schema.table
    static const std::regex table_regex(
        R"REGEX(CREATE\s+TABLE\s+(?:(?:\[([^\]]+)\]|"([^"]+)"|(\w+))\.)?(?:\[([^\]]+)\]|"([^"]+)"|(\w+))\s*\()REGEX",
        std::regex_constants::icase);
    std::smatch match;

    if (!std::regex_search(sql, match, table_regex)) {
        return false;
    }

    // Groups:
    //  1=schema_bracket, 2=schema_dq, 3=schema_plain
    //  4=table_bracket,  5=table_dq,  6=table_plain
    schema.schema = match[1].matched ? match[1].str()
                  : match[2].matched ? match[2].str()
                  : match[3].matched ? match[3].str() : "";
    schema.name   = match[4].matched ? match[4].str()
                  : match[5].matched ? match[5].str()
                  : match[6].matched ? match[6].str() : "";

    if (schema.name.empty()) return false;

    // Find outer parentheses wrapping the column definitions
    size_t open_pos = sql.find('(', match.position());
    if (open_pos == std::string::npos) return false;

    // Find matching closing paren using a depth counter
    int depth = 0;
    bool in_string = false;
    char str_char = '\0';
    size_t close_pos = std::string::npos;
    for (size_t k = open_pos; k < sql.size(); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == str_char) {
                // Check for '' escape
                if (c == '\'' && k + 1 < sql.size() && sql[k + 1] == '\'') {
                    ++k; // skip the escaped quote
                } else {
                    in_string = false;
                }
            }
        } else if (c == '\'' || c == '"') {
            in_string = true;
            str_char = c;
        } else if (c == '[') {
            // T-SQL bracket identifier – consume until ']'
            while (k < sql.size() && sql[k] != ']') ++k;
        } else if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth == 0) { close_pos = k; break; }
        }
    }
    if (close_pos == std::string::npos) return false;

    std::string cols_str = sql.substr(open_pos + 1, close_pos - open_pos - 1);

    // Split at top-level commas
    std::vector<std::string> col_defs;
    {
        int dep = 0;
        bool inq = false;
        char qc  = '\0';
        bool in_bracket = false;
        std::string cur;
        for (size_t i = 0; i < cols_str.size(); ++i) {
            char c = cols_str[i];
            if (in_bracket) {
                cur += c;
                if (c == ']') in_bracket = false;
            } else if (inq) {
                cur += c;
                if (c == qc) {
                    if (qc == '\'' && i + 1 < cols_str.size() && cols_str[i + 1] == '\'') {
                        cur += cols_str[++i]; // '' escape
                    } else {
                        inq = false;
                    }
                }
            } else if (c == '[') {
                in_bracket = true; cur += c;
            } else if (c == '\'' || c == '"') {
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
        if (!cur.empty()) col_defs.push_back(cur);
    }

    for (auto& col_def : col_defs) {
        // Trim leading/trailing whitespace
        {
            size_t f = col_def.find_first_not_of(" \t\n\r");
            if (f == std::string::npos) continue;
            col_def = col_def.substr(f);
        }
        {
            size_t l = col_def.find_last_not_of(" \t\n\r");
            if (l != std::string::npos) col_def = col_def.substr(0, l + 1);
        }
        if (col_def.empty()) continue;

        // Build upper prefix for constraint detection
        std::string upper_def;
        for (size_t i = 0; i < col_def.size() && i < 30; ++i) {
            upper_def += static_cast<char>(
                std::toupper(static_cast<unsigned char>(col_def[i])));
        }

        // Skip table-level constraints.
        // A constraint definition starts with a constraint keyword as the first word:
        // PRIMARY, UNIQUE, CONSTRAINT, CHECK, FOREIGN, INDEX.
        // Column definitions start with an identifier ([bracket], "dq", or word), so
        // we must not skip column types that contain these words (e.g. UNIQUEIDENTIFIER).
        {
            std::string first_word;
            if (col_def[0] != '[' && col_def[0] != '"') {
                size_t ws_pos = col_def.find_first_of(" \t(");
                first_word = (ws_pos == std::string::npos) ? col_def
                                                           : col_def.substr(0, ws_pos);
                for (auto& ch : first_word)
                    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            if (first_word == "PRIMARY"    || first_word == "UNIQUE"  ||
                first_word == "CONSTRAINT" || first_word == "CHECK"   ||
                first_word == "FOREIGN"    || first_word == "INDEX") {
                continue;
            }
        }

        // Extract column name — may be [bracket], "dq", or plain
        std::string col_name;
        size_t type_start = 0;

        if (!col_def.empty() && col_def[0] == '[') {
            // Square-bracket identifier: [col_name]
            size_t end_br = col_def.find(']', 1);
            if (end_br == std::string::npos) continue;
            col_name   = col_def.substr(1, end_br - 1);
            type_start = end_br + 1;
        } else if (!col_def.empty() && col_def[0] == '"') {
            // Double-quoted identifier
            size_t end_dq = col_def.find('"', 1);
            if (end_dq == std::string::npos) continue;
            col_name   = col_def.substr(1, end_dq - 1);
            type_start = end_dq + 1;
        } else {
            // Plain identifier
            size_t sp = col_def.find_first_of(" \t");
            if (sp == std::string::npos) continue;
            col_name   = col_def.substr(0, sp);
            type_start = sp;
        }

        if (col_name.empty()) continue;

        // Skip leading whitespace after the column name
        while (type_start < col_def.size() &&
               (col_def[type_start] == ' ' || col_def[type_start] == '\t')) {
            ++type_start;
        }

        // Collect type token (may include precision like NVARCHAR(255))
        std::string col_type;
        size_t k = type_start;
        int tdep = 0;
        while (k < col_def.size()) {
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

        if (col_type.empty()) continue;

        schema.columns.push_back(col_name);
        schema.column_types[col_name] = col_type;
    }

    return !schema.name.empty();
}

bool MSSQLImporter::parseInsert(const std::string& sql, const ImportOptions& options,
                                 ImportStats& stats, size_t line_number,
                                 std::unordered_set<uint64_t>& delta_hashes) {
    // T-SQL INSERT:
    //   INSERT INTO [[schema].]table ([[col1],[col2],...]) VALUES (v1, v2, ...);
    //   Multi-row: INSERT INTO … VALUES (…),(…);
    // Column list may use bracket, dq, or plain identifiers.
    static const std::regex insert_regex(
        R"REGEX(INSERT\s+INTO\s+(?:(?:\[([^\]]+)\]|"([^"]+)"|(\w+))\.)?(?:\[([^\]]+)\]|"([^"]+)"|(\w+))\s*(?:\(([^)]*)\))?\s+VALUES\s*(.+?)\s*;?\s*$)REGEX",
        std::regex_constants::icase);
    std::smatch match;

    if (!std::regex_search(sql, match, insert_regex)) {
        addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                 "Could not parse INSERT statement",
                 "line " + std::to_string(line_number));
        stats.failed_records++;
        return false;
    }

    // groups 4/5/6 = table name (bracket / dq / plain)
    std::string table_name = match[4].matched ? match[4].str()
                           : match[5].matched ? match[5].str()
                           : match[6].str();

    if (!shouldImportTable(table_name, options)) {
        stats.skipped_records++;
        return true;
    }

    // Column list from the explicit column clause (group 7)
    std::vector<std::string> col_list;
    if (match[7].matched && !match[7].str().empty()) {
        std::string col_str = match[7].str();
        // Split at top-level commas
        std::istringstream css(col_str);
        std::string col;
        while (std::getline(css, col, ',')) {
            col = unquoteIdentifier(col);
            col.erase(0, col.find_first_not_of(" \t"));
            if (!col.empty()) {
                size_t last = col.find_last_not_of(" \t");
                if (last != std::string::npos) col = col.substr(0, last + 1);
            }
            if (!col.empty()) col_list.push_back(col);
        }
    } else if (schemas_.count(table_name)) {
        col_list = schemas_[table_name].columns;
    }

    // VALUES payload (group 8)
    std::string values_payload = match[8].str();

    // Build effective schema
    TableSchema eff_schema;
    eff_schema.name = table_name;
    if (schemas_.count(table_name)) eff_schema = schemas_[table_name];
    if (!col_list.empty()) eff_schema.columns = col_list;

    // Parse tuple list: (v1,...),(v2,...), ...
    size_t pos = 0;
    while (pos < values_payload.size()) {
        // Skip whitespace and commas between tuples
        while (pos < values_payload.size() &&
               (values_payload[pos] == ' ' || values_payload[pos] == '\t' ||
                values_payload[pos] == ',' || values_payload[pos] == '\r' ||
                values_payload[pos] == '\n')) {
            ++pos;
        }
        if (pos >= values_payload.size()) break;
        if (values_payload[pos] != '(') { ++pos; continue; }

        // Find matching ')' for this tuple
        size_t tuple_start = pos + 1;
        int dep = 1;
        bool in_str = false;
        char str_c = '\0';
        size_t k = pos + 1;
        while (k < values_payload.size() && dep > 0) {
            char c = values_payload[k];
            if (in_str) {
                if (c == str_c) {
                    if (c == '\'' && k + 1 < values_payload.size() &&
                        values_payload[k + 1] == '\'') {
                        ++k; // '' escape
                    } else {
                        in_str = false;
                    }
                }
            } else if (c == '\'' || c == '"') {
                in_str = true; str_c = c;
            } else if (c == '(') {
                ++dep;
            } else if (c == ')') {
                --dep;
            }
            ++k;
        }
        size_t tuple_end = k - 1;
        if (dep != 0) break; // unbalanced – abort

        std::string tuple_str = values_payload.substr(tuple_start, tuple_end - tuple_start);
        std::vector<std::string> values = parseInsertValues(tuple_str);

        // Delta / incremental import check
        if (!options.delta_hash_file.empty()) {
            uint64_t h = computeRowHash(tuple_str, values,
                                        options.delta_key_columns,
                                        eff_schema.columns);
            if (delta_hashes.count(h)) {
                stats.skipped_records++;
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
                           " columns, expected " +
                           std::to_string(eff_schema.columns.size());
            err.location = "table " + table_name + ", line " +
                           std::to_string(line_number);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back(err.message);
            stats.failed_records++;
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table_name}, {"status", "failed"}}, 1.0);
        } else {
            json entity = convertRowToEntity(eff_schema, values);
            THEMIS_DEBUG("SQL Server INSERT entity: {}", entity.dump());
            if (options.streaming_row_callback) {
                if (!options.streaming_row_callback(table_name, entity)) {
                    cancelled_ = true;
                }
            }
            stats.imported_records++;
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table_name}, {"status", "imported"}}, 1.0);
            if (cancelled_) return true;
        }

        pos = k;
    }

    return true;
}

std::string MSSQLImporter::mapMSSQLTypeToThemis(const std::string& mssql_type,
                                                 const ImportOptions& options) const {
    // User-configurable overrides take priority
    auto it = options.type_overrides.find(mssql_type);
    if (it != options.type_overrides.end()) return it->second;

    // Normalise: strip precision/scale and trailing modifiers
    // e.g. "NVARCHAR(255)" -> "nvarchar", "DECIMAL(10,2)" -> "decimal"
    std::string base_type = mssql_type;
    size_t paren = base_type.find('(');
    if (paren != std::string::npos) base_type = base_type.substr(0, paren);
    std::string lower = toLower(base_type);
    {
        size_t l = lower.find_last_not_of(" \t");
        if (l != std::string::npos) lower = lower.substr(0, l + 1);
    }

    // Exact matches (T-SQL specific and common types)

    // Exact integer types
    if (lower == "bigint")          return "integer";
    if (lower == "int")             return "integer";
    if (lower == "integer")         return "integer";
    if (lower == "smallint")        return "integer";
    if (lower == "tinyint")         return "integer";

    // Exact floating-point types
    if (lower == "float")           return "double";
    if (lower == "real")            return "float";

    // Exact decimal types
    if (lower == "decimal")         return "double";
    if (lower == "numeric")         return "double";
    if (lower == "money")           return "double";
    if (lower == "smallmoney")      return "double";

    // Exact boolean
    if (lower == "bit")             return "boolean";

    // Exact string types
    if (lower == "char")            return "string";
    if (lower == "varchar")         return "string";
    if (lower == "text")            return "string";
    if (lower == "nchar")           return "string";
    if (lower == "nvarchar")        return "string";
    if (lower == "ntext")           return "string";
    if (lower == "xml")             return "string";
    if (lower == "sysname")         return "string";

    // Exact binary/blob types
    if (lower == "binary")          return "binary";
    if (lower == "varbinary")       return "binary";
    if (lower == "image")           return "binary";
    if (lower == "rowversion")      return "binary";
    if (lower == "timestamp")       return "binary";  // SQL Server TIMESTAMP is rowversion

    // Exact date/time types
    if (lower == "date")            return "date";
    if (lower == "time")            return "time";
    if (lower == "datetime")        return "datetime";
    if (lower == "datetime2")       return "datetime";
    if (lower == "smalldatetime")   return "datetime";
    if (lower == "datetimeoffset")  return "datetime";

    // Exact UUID / GUID type
    if (lower == "uniqueidentifier") return "uuid";

    // Exact hierarchical / spatial types → string representation
    if (lower == "hierarchyid")     return "string";
    if (lower == "geography")       return "string";
    if (lower == "geometry")        return "string";
    if (lower == "sql_variant")     return "string";

    // Prefix-based fallbacks
    if (lower.find("int")    != std::string::npos) return "integer";
    if (lower.find("char")   != std::string::npos) return "string";
    if (lower.find("text")   != std::string::npos) return "string";
    if (lower.find("binary") != std::string::npos) return "binary";
    if (lower.find("float")  != std::string::npos) return "double";
    if (lower.find("decimal")!= std::string::npos) return "double";
    if (lower.find("numeric")!= std::string::npos) return "double";
    if (lower.find("money")  != std::string::npos) return "double";
    if (lower.find("date")   != std::string::npos) return "datetime";
    if (lower.find("time")   != std::string::npos) return "datetime";
    if (lower.find("bit")    != std::string::npos) return "boolean";

    return "string"; // Default: treat unknown types as strings
}

bool MSSQLImporter::shouldImportTable(const std::string& table_name,
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

json MSSQLImporter::convertRowToEntity(const TableSchema& schema,
                                       const std::vector<std::string>& values) {
    json entity;
    entity["_type"] = schema.name;

    for (size_t i = 0; i < values.size() && i < schema.columns.size(); ++i) {
        const std::string& col  = schema.columns[i];
        const std::string& val  = values[i];

        // Determine the ThemisDB logical type for this column
        auto type_it = schema.column_types.find(col);
        const std::string col_type = (type_it != schema.column_types.end())
                                     ? type_it->second : "string";
        const std::string mapped_type = mapMSSQLTypeToThemis(col_type, ImportOptions{});

        if (val.empty()) {
            entity[col] = nullptr;
        } else if (mapped_type == "integer") {
            try { entity[col] = std::stoll(val); }
            catch (...) { entity[col] = val; }
        } else if (mapped_type == "double" || mapped_type == "float") {
            try { entity[col] = std::stod(val); }
            catch (...) { entity[col] = val; }
        } else if (mapped_type == "boolean") {
            // T-SQL BIT: 0 or 1 (or TRUE/FALSE)
            if (val == "1" || toLower(val) == "true")  entity[col] = true;
            else if (val == "0" || toLower(val) == "false") entity[col] = false;
            else entity[col] = val;
        } else {
            entity[col] = val;
        }
    }

    return entity;
}

std::vector<std::string> MSSQLImporter::parseInsertValues(
    const std::string& values_clause) const {
    // Handles:
    //   - NULL keyword -> null sentinel (empty string)
    //   - Single-quoted strings with '' escape for embedded quotes
    //   - N'unicode string' literals (N prefix stripped)
    //   - Numeric literals (including negative)
    //   - Function calls like GETDATE(), CONVERT(datetime,…)
    std::vector<std::string> result;
    size_t i = 0;
    const size_t n = values_clause.size();

    auto skipWs = [&]() {
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                         values_clause[i] == '\r' || values_clause[i] == '\n')) ++i;
    };

    while (i < n) {
        skipWs();
        if (i >= n) break;

        char c = values_clause[i];

        // N'unicode string' or 'string'
        if ((c == 'N' || c == 'n') && i + 1 < n && values_clause[i + 1] == '\'') {
            ++i; // skip the N prefix
            c = values_clause[i];
        }

        if (c == '\'') {
            ++i;
            std::string val;
            while (i < n) {
                char sc = values_clause[i];
                if (sc == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
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
        } else {
            // Unquoted: NULL, number, function call, 0x hex literal
            size_t start = i;
            int dep = 0;
            while (i < n) {
                char tc = values_clause[i];
                if (tc == '(') { ++dep; ++i; }
                else if (tc == ')') {
                    if (dep > 0) { --dep; ++i; }
                    else break;
                } else if (tc == '\'' && dep > 0) {
                    ++i;
                    while (i < n) {
                        char sc = values_clause[i];
                        if (sc == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                            i += 2;
                        } else if (sc == '\'') {
                            ++i; break;
                        } else {
                            ++i;
                        }
                    }
                } else if (tc == ',' && dep == 0) {
                    break;
                } else {
                    ++i;
                }
            }
            std::string token = values_clause.substr(start, i - start);
            {
                size_t f = token.find_first_not_of(" \t\r\n");
                size_t l = token.find_last_not_of(" \t\r\n");
                if (f == std::string::npos) token.clear();
                else token = token.substr(f, l - f + 1);
            }
            // NULL -> empty string sentinel
            std::string upper_tok;
            for (char ch : token)
                upper_tok += static_cast<char>(
                    std::toupper(static_cast<unsigned char>(ch)));
            if (upper_tok == "NULL") token.clear();
            result.push_back(token);
        }

        skipWs();
        if (i < n && values_clause[i] == ',') ++i;
    }
    return result;
}

std::string MSSQLImporter::unquoteIdentifier(const std::string& s) {
    std::string t = s;
    {
        size_t f = t.find_first_not_of(" \t\r\n");
        size_t l = t.find_last_not_of(" \t\r\n");
        if (f == std::string::npos) return "";
        t = t.substr(f, l - f + 1);
    }
    // Strip square brackets
    if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
        return t.substr(1, t.size() - 2);
    }
    // Strip double quotes
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        return t.substr(1, t.size() - 2);
    }
    return t;
}

std::string MSSQLImporter::stripBlockComments(const std::string& sql) {
    std::string result;
    result.reserve(sql.size());
    size_t i = 0;
    while (i < sql.size()) {
        if (i + 1 < sql.size() && sql[i] == '/' && sql[i + 1] == '*') {
            i += 2;
            while (i + 1 < sql.size() && !(sql[i] == '*' && sql[i + 1] == '/')) {
                ++i;
            }
            i += 2;
            result += ' ';
        } else {
            result += sql[i];
            ++i;
        }
    }
    return result;
}

void MSSQLImporter::addError(ImportStats& stats, ImportErrorCode code,
                              ImportErrorSeverity severity, const std::string& message,
                              const std::string& location) const {
    ImportError err;
    err.code     = code;
    err.severity = severity;
    err.message  = message;
    err.location = location;
    stats.structured_errors.push_back(err);
    if (severity == ImportErrorSeverity::ERROR ||
        severity == ImportErrorSeverity::CRITICAL) {
        stats.errors.push_back(message);
    }
}

void MSSQLImporter::emitMetric(const ImportOptions& options,
                                const std::string& metric,
                                const std::map<std::string, std::string>& labels,
                                double value) const {
    if (options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void MSSQLImporter::emitSpan(const ImportOptions& options,
                              const std::string& operation,
                              const std::map<std::string, std::string>& attributes,
                              double duration_seconds) const {
    if (options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void MSSQLImporter::reportProgress(ProgressCallback& callback,
                                   const std::string& stage,
                                   size_t current, size_t total) {
    if (callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// Delta / incremental import helpers
// ============================================================================

uint64_t MSSQLImporter::computeRowHash(const std::string& tuple_str,
                                       const std::vector<std::string>& values,
                                       const std::vector<std::string>& key_columns,
                                       const std::vector<std::string>& schema_columns) {
    // FNV-1a 64-bit hash (same algorithm as MySQL importer for consistency)
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME  = 1099511628211ULL;

    auto fnv1a = [&](const std::string& s) {
        uint64_t h = FNV_OFFSET;
        for (unsigned char c : s) {
            h ^= c;
            h *= FNV_PRIME;
        }
        return h;
    };

    if (key_columns.empty()) {
        // Hash the full tuple
        return fnv1a(tuple_str);
    }

    // Hash only the specified key columns
    uint64_t h = FNV_OFFSET;
    for (const auto& key : key_columns) {
        auto it = std::find(schema_columns.begin(), schema_columns.end(), key);
        if (it != schema_columns.end()) {
            size_t idx = static_cast<size_t>(
                std::distance(schema_columns.begin(), it));
            if (idx < values.size()) {
                for (unsigned char c : values[idx]) {
                    h ^= c;
                    h *= FNV_PRIME;
                }
            }
        }
        // Separator between key columns
        h ^= static_cast<unsigned char>('\0');
        h *= FNV_PRIME;
    }
    return h;
}

std::unordered_set<uint64_t> MSSQLImporter::loadDeltaHashes(
    const std::string& delta_hash_file) {
    std::unordered_set<uint64_t> hashes;
    std::ifstream f(delta_hash_file);
    if (!f) return hashes;
    uint64_t h;
    while (f >> h) hashes.insert(h);
    return hashes;
}

void MSSQLImporter::saveDeltaHashes(const std::string& delta_hash_file,
                                    const std::unordered_set<uint64_t>& hashes) {
    std::ofstream f(delta_hash_file, std::ios::trunc);
    if (!f) return;
    for (uint64_t h : hashes) f << h << '\n';
}

// ============================================================================
// Plugin implementation
// ============================================================================

MSSQLImporterPlugin::MSSQLImporterPlugin()
    : importer_(std::make_unique<MSSQLImporter>()) {
}

plugins::PluginCapabilities MSSQLImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    caps.thread_safe        = false;
    return caps;
}

bool MSSQLImporterPlugin::initialize(const char* config_json) {
    if (!importer_) return false;
    return importer_->initialize(config_json ? config_json : "{}");
}

void MSSQLImporterPlugin::shutdown() {
    if (importer_) importer_->cancel();
}

} // namespace importers
} // namespace themis
