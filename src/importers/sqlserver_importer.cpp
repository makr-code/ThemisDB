/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sqlserver_importer.cpp                             ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23 12:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 IN PROGRESS                                  ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     900                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • initial  2026-02-23  Add SQL Server importer (Issue #1845)      ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/sqlserver_importer.h"
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
    for (auto& c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

/**
 * @brief Trim leading and trailing whitespace from a string.
 */
static std::string trim(const std::string& s) {
    size_t f = s.find_first_not_of(" \t\r\n");
    if (f == std::string::npos) return "";
    size_t l = s.find_last_not_of(" \t\r\n");
    return s.substr(f, l - f + 1);
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

SQLServerImporter::SQLServerImporter() = default;

SQLServerImporter::~SQLServerImporter() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> SQLServerImporter::getSupportedTypes() const {
    return {"sqlserver", "mssql", "tsql"};
}

bool SQLServerImporter::initialize(const std::string& /*config*/) {
    cancelled_ = false;
    schemas_.clear();
    THEMIS_INFO("SQL Server Importer initialized");
    return true;
}

bool SQLServerImporter::validateSource(const std::string& source_path,
                                       std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }

    // Check for SQL Server indicators in the first 100 lines.
    std::string line;
    bool found_sqlserver = false;
    int lines_checked = 0;
    while (std::getline(file, line) && lines_checked < 100) {
        std::string lower_line = toLower(line);
        if (lower_line.find("sql server") != std::string::npos ||
            lower_line.find("ssms")       != std::string::npos ||
            lower_line.find("sqlcmd")     != std::string::npos ||
            line.find("SET ANSI_NULLS")         != std::string::npos ||
            line.find("SET QUOTED_IDENTIFIER")  != std::string::npos ||
            line.find("CREATE TABLE [")         != std::string::npos) {
            found_sqlserver = true;
            break;
        }
        lines_checked++;
    }

    if (!found_sqlserver) {
        errors.push_back("File does not appear to be a SQL Server T-SQL dump");
        return false;
    }

    THEMIS_INFO("SQL Server source validation successful: {}", source_path);
    return true;
}

ImportStats SQLServerImporter::importData(
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

    // Parse dump file
    if (!parseDumpFile(source_path, options, stats, progress_callback)) {
        if (stats.structured_errors.empty()) {
            addError(stats, ImportErrorCode::FILE_READ_FAILED,
                     ImportErrorSeverity::CRITICAL, "Failed to parse dump file");
        }
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

std::shared_ptr<ImportHandle> SQLServerImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "sqlserver-import-" + std::to_string(ms) + "-" +
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

void SQLServerImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("SQL Server import cancelled");
}

json SQLServerImporter::getSourceSchema(const std::string& source_path) {
    schemas_.clear();

    std::ifstream file(source_path);
    if (!file) {
        return json::array();
    }

    std::string line;
    std::string current_batch;

    while (std::getline(file, line)) {
        // Trim trailing carriage-return
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Check for GO batch separator
        std::string trimmed = trim(line);
        std::string upper_trimmed = trimmed;
        for (auto& c : upper_trimmed)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if (upper_trimmed == "GO") {
            // Process accumulated batch
            if (current_batch.find("CREATE TABLE") != std::string::npos ||
                current_batch.find("create table") != std::string::npos) {
                TableSchema schema;
                if (parseCreateTable(current_batch, schema)) {
                    schemas_[schema.name] = schema;
                }
            }
            current_batch.clear();
            continue;
        }

        // Skip comments
        if (trimmed.size() >= 2 && trimmed[0] == '-' && trimmed[1] == '-') continue;
        if (trimmed.empty()) continue;

        current_batch += line + "\n";
    }

    // Process any remaining batch
    if (!current_batch.empty() &&
        (current_batch.find("CREATE TABLE") != std::string::npos ||
         current_batch.find("create table") != std::string::npos)) {
        TableSchema schema;
        if (parseCreateTable(current_batch, schema)) {
            schemas_[schema.name] = schema;
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

bool SQLServerImporter::parseDumpFile(const std::string& file_path, const ImportOptions& options,
                                       ImportStats& stats, ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    // Detect SQL Server dump header in the first 100 lines.
    {
        std::string hdr_line;
        bool found_header = false;
        int hdr_lines = 0;
        while (std::getline(file, hdr_line) && hdr_lines < 100) {
            std::string lower_line = toLower(hdr_line);
            if (lower_line.find("sql server") != std::string::npos ||
                lower_line.find("ssms")       != std::string::npos ||
                lower_line.find("sqlcmd")     != std::string::npos ||
                hdr_line.find("SET ANSI_NULLS")        != std::string::npos ||
                hdr_line.find("SET QUOTED_IDENTIFIER") != std::string::npos ||
                hdr_line.find("CREATE TABLE [")        != std::string::npos) {
                found_header = true;
                break;
            }
            hdr_lines++;
        }
        if (!found_header) {
            addError(stats, ImportErrorCode::NOT_A_PG_DUMP, ImportErrorSeverity::CRITICAL,
                     "File does not appear to be a SQL Server T-SQL dump");
            return false;
        }
        // Rewind to process the full file
        file.clear();
        file.seekg(0);
    }

    // Per-line read limit (default 64 MB, honoring max_statement_size_bytes)
    const size_t line_read_limit = options.max_statement_size_bytes > 0
                                   ? options.max_statement_size_bytes
                                   : 64 * 1024 * 1024ULL;

    std::string line;
    std::string current_batch;
    size_t line_number = 0;
    size_t batch_row_count = 0;
    bool line_truncated = false;

    while (streamReadLine(file, line, line_read_limit, line_truncated) && !cancelled_) {
        line_number++;

        // Strip trailing carriage-return
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line_truncated) {
            addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "Line too long (> " + std::to_string(line_read_limit) + " bytes); truncated",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back("Line truncated at " + std::to_string(line_number));
            current_batch.clear();
            if (!options.continue_on_error) return false;
            continue;
        }

        // Check for GO batch separator (case-insensitive, possibly trailing whitespace)
        {
            std::string tline = trim(line);
            std::string upper_tline = tline;
            for (auto& c : upper_tline)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (upper_tline == "GO") {
                // Flush accumulated batch
                if (!current_batch.empty()) {
                    // Statement-size guard
                    if (options.max_statement_size_bytes > 0 &&
                        current_batch.size() > options.max_statement_size_bytes) {
                        addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                                 ImportErrorSeverity::WARNING,
                                 "SQL batch exceeds max_statement_size_bytes",
                                 "line " + std::to_string(line_number));
                        stats.warnings.push_back("Batch too large near line " +
                                                 std::to_string(line_number));
                        current_batch.clear();
                        if (!options.continue_on_error) return false;
                        continue;
                    }

                    // Determine first non-empty, non-comment keyword prefix
                    std::string prefix;
                    {
                        std::istringstream bss(current_batch);
                        std::string bline;
                        while (std::getline(bss, bline)) {
                            std::string bt = trim(bline);
                            if (bt.empty()) continue;
                            if (bt.size() >= 2 && bt[0] == '-' && bt[1] == '-') continue;
                            prefix = bt.substr(0, std::min(bt.size(), size_t(30)));
                            break;
                        }
                    }
                    std::string upper_prefix = prefix;
                    for (auto& c : upper_prefix)
                        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

                    if (upper_prefix.find("CREATE TABLE") != std::string::npos) {
                        auto t0 = std::chrono::steady_clock::now();
                        TableSchema schema;
                        if (parseCreateTable(current_batch, schema)) {
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
                    } else if (upper_prefix.find("INSERT") != std::string::npos) {
                        // INSERT batch may contain multiple INSERT statements separated by newlines
                        std::istringstream bss(current_batch);
                        std::string stmt;
                        std::string stmt_acc;
                        while (std::getline(bss, stmt)) {
                            std::string st = trim(stmt);
                            if (st.empty()) continue;
                            if (st.size() >= 2 && st[0] == '-' && st[1] == '-') continue;
                            stmt_acc += stmt + "\n";
                            // Each INSERT ends with a semicolon or we treat each line
                            // that starts with INSERT as its own statement
                            std::string st_upper = st.substr(0, std::min(st.size(), size_t(10)));
                            for (auto& c : st_upper)
                                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                            if (st_upper.find("INSERT") == 0) {
                                // Flush previous accumulation and start new one
                                // (already accumulated above)
                            }
                        }
                        // Process the whole batch as a single INSERT statement
                        stats.total_records++;
                        if (!options.dry_run) {
                            auto t0 = std::chrono::steady_clock::now();
                            // Process each INSERT line in the batch
                            std::istringstream ins_ss(current_batch);
                            std::string ins_line;
                            std::string ins_stmt;
                            while (std::getline(ins_ss, ins_line)) {
                                std::string it = trim(ins_line);
                                if (it.empty()) continue;
                                if (it.size() >= 2 && it[0] == '-' && it[1] == '-') continue;
                                std::string it_upper = it.substr(0, std::min(it.size(), size_t(10)));
                                for (auto& c : it_upper)
                                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                                if (it_upper.find("INSERT") == 0 && !ins_stmt.empty()) {
                                    parseInsert(ins_stmt, options, stats, line_number);
                                    ins_stmt.clear();
                                }
                                ins_stmt += ins_line + " ";
                            }
                            if (!ins_stmt.empty()) {
                                parseInsert(ins_stmt, options, stats, line_number);
                            }
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
                    // SET ANSI_NULLS, SET QUOTED_IDENTIFIER, USE, PRINT, ALTER TABLE
                    // ADD CONSTRAINT, SET IDENTITY_INSERT are silently ignored.

                    current_batch.clear();
                }
                continue;
            }
        }

        // Skip standalone comment lines and empty lines
        {
            std::string tline = trim(line);
            if (tline.empty()) continue;
            if (tline.size() >= 2 && tline[0] == '-' && tline[1] == '-') continue;
        }

        current_batch += line + "\n";
    }

    // Process any remaining batch (no trailing GO)
    if (!current_batch.empty() && !cancelled_) {
        std::string prefix;
        {
            std::istringstream bss(current_batch);
            std::string bline;
            while (std::getline(bss, bline)) {
                std::string bt = trim(bline);
                if (bt.empty()) continue;
                if (bt.size() >= 2 && bt[0] == '-' && bt[1] == '-') continue;
                prefix = bt.substr(0, std::min(bt.size(), size_t(30)));
                break;
            }
        }
        std::string upper_prefix = prefix;
        for (auto& c : upper_prefix)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if (upper_prefix.find("CREATE TABLE") != std::string::npos) {
            TableSchema schema;
            if (parseCreateTable(current_batch, schema)) {
                if (shouldImportTable(schema.name, options)) {
                    schemas_[schema.name] = schema;
                    stats.tables_processed++;
                }
            }
        } else if (upper_prefix.find("INSERT") != std::string::npos) {
            stats.total_records++;
            if (!options.dry_run) {
                std::istringstream ins_ss(current_batch);
                std::string ins_line;
                std::string ins_stmt;
                while (std::getline(ins_ss, ins_line)) {
                    std::string it = trim(ins_line);
                    if (it.empty()) continue;
                    if (it.size() >= 2 && it[0] == '-' && it[1] == '-') continue;
                    std::string it_upper = it.substr(0, std::min(it.size(), size_t(10)));
                    for (auto& c : it_upper)
                        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    if (it_upper.find("INSERT") == 0 && !ins_stmt.empty()) {
                        parseInsert(ins_stmt, options, stats, line_number);
                        ins_stmt.clear();
                    }
                    ins_stmt += ins_line + " ";
                }
                if (!ins_stmt.empty()) {
                    parseInsert(ins_stmt, options, stats, line_number);
                }
            }
        }
    }

    return !cancelled_;
}

bool SQLServerImporter::parseCreateTable(const std::string& sql, TableSchema& schema) {
    // Match: CREATE TABLE [schema].[table] ( or CREATE TABLE [table] (
    // SQL Server uses square-bracket-quoted identifiers.
    std::regex table_regex(
        R"(CREATE\s+TABLE\s+(?:\[([^\]]+)\]\.)?(?:\[([^\]]+)\]|(\w+))\s*\()",
        std::regex_constants::icase);
    std::smatch match;

    if (!std::regex_search(sql, match, table_regex)) {
        return false;
    }

    schema.schema = match[1].matched ? match[1].str() : "";
    schema.name   = match[2].matched ? match[2].str()
                  : match[3].matched ? match[3].str() : "";

    if (schema.name.empty()) return false;

    // Find the outer parentheses that wrap the column definitions.
    size_t open_pos = sql.find('(', match.position());
    if (open_pos == std::string::npos) return false;

    // Find matching closing paren using a simple depth counter (respects quotes)
    int depth = 0;
    bool in_string = false;
    char str_char = '\0';
    size_t close_pos = std::string::npos;
    for (size_t k = open_pos; k < sql.size(); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == '\'' && k + 1 < sql.size() && sql[k + 1] == '\'') {
                ++k;  // '' escape inside string
                continue;
            }
            if (c == str_char) in_string = false;
        } else if (c == '\'') {
            in_string = true;
            str_char  = c;
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
        std::string cur;
        for (size_t i = 0; i < cols_str.size(); ++i) {
            char c = cols_str[i];
            if (inq) {
                cur += c;
                if (c == '\'' && i + 1 < cols_str.size() && cols_str[i + 1] == '\'') {
                    cur += cols_str[++i];  // '' escape
                } else if (c == qc) {
                    inq = false;
                }
            } else if (c == '\'' || c == '"') {
                inq = true; qc = c; cur += c;
            } else if (c == '[') {
                // Square-bracket-quoted identifier – not a quote but skip depth tracking
                cur += c;
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
        // Trim whitespace
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

        // Build upper-case prefix for constraint detection
        std::string upper_def;
        for (size_t i = 0; i < col_def.size() && i < 30; ++i)
            upper_def += static_cast<char>(std::toupper(static_cast<unsigned char>(col_def[i])));

        // Skip table-level constraints
        if (upper_def.find("PRIMARY")   != std::string::npos ||
            upper_def.find("UNIQUE")    != std::string::npos ||
            upper_def.find("CONSTRAINT")!= std::string::npos ||
            upper_def.find("CHECK")     != std::string::npos ||
            upper_def.find("FOREIGN")   != std::string::npos ||
            upper_def.find("INDEX")     != std::string::npos) {
            continue;
        }

        // Column definition: [col_name] [type] options...
        // Extract column name (square-bracket-quoted or plain identifier)
        std::string col_name;
        size_t type_start = 0;
        if (!col_def.empty() && col_def[0] == '[') {
            size_t end_bracket = col_def.find(']', 1);
            if (end_bracket == std::string::npos) continue;
            col_name   = col_def.substr(1, end_bracket - 1);
            type_start = end_bracket + 1;
        } else {
            size_t sp = col_def.find_first_of(" \t");
            if (sp == std::string::npos) continue;
            col_name   = col_def.substr(0, sp);
            type_start = sp;
        }

        if (col_name.empty()) continue;

        // Skip leading whitespace after name
        while (type_start < col_def.size() &&
               (col_def[type_start] == ' ' || col_def[type_start] == '\t')) {
            ++type_start;
        }

        // Extract type token (may include parenthesised size; may be bracket-quoted)
        std::string col_type;
        size_t k = type_start;
        int tdep = 0;
        bool in_bracket = false;
        while (k < col_def.size()) {
            char c = col_def[k];
            if (in_bracket) {
                if (c == ']') { in_bracket = false; col_type += c; }
                else { col_type += c; }
            } else if (c == '[') {
                in_bracket = true; col_type += c;
            } else if (c == '(') {
                ++tdep; col_type += c;
            } else if (c == ')') {
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

        // Strip square brackets from type name if present (e.g. [int] -> int)
        col_type = unquoteIdentifier(col_type);

        schema.columns.push_back(col_name);
        schema.column_types[col_name] = col_type;
    }

    return !schema.name.empty();
}

bool SQLServerImporter::parseInsert(const std::string& sql, const ImportOptions& options,
                                     ImportStats& stats, size_t line_number) {
    // SQL Server INSERT:
    //   INSERT INTO [schema].[table] [(col1,...)] VALUES (v1,...);
    //   INSERT INTO [table] VALUES (v1,...);
    // N'string' prefix for Unicode strings is handled in parseInsertValues.
    std::regex insert_regex(
        R"(INSERT\s+INTO\s+(?:\[([^\]]+)\]\.)?(?:\[([^\]]+)\]|(\w+))\s*(?:\(([^)]*)\))?\s+VALUES\s*(.+?)\s*;?\s*$)",
        std::regex_constants::icase);
    std::smatch match;

    if (!std::regex_search(sql, match, insert_regex)) {
        addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                 "Could not parse INSERT statement",
                 "line " + std::to_string(line_number));
        stats.failed_records++;
        return false;
    }

    // group 2 = bracket table name, group 3 = plain table name
    std::string table_name = match[2].matched ? match[2].str() : match[3].str();

    if (!shouldImportTable(table_name, options)) {
        stats.skipped_records++;
        return true;
    }

    // Resolve column list
    std::vector<std::string> col_list;
    if (match[4].matched && !match[4].str().empty()) {
        std::istringstream css(match[4].str());
        std::string col;
        while (std::getline(css, col, ',')) {
            col = unquoteIdentifier(col);
            col.erase(0, col.find_first_not_of(" \t"));
            if (!col.empty() && col.back() != ' ')
                col.erase(col.find_last_not_of(" \t") + 1);
            if (!col.empty()) col_list.push_back(col);
        }
    } else if (schemas_.count(table_name)) {
        col_list = schemas_[table_name].columns;
    }

    // The VALUES payload: may be a single tuple or comma-separated tuples
    std::string values_payload = match[5].str();

    // Build effective schema for convertRowToEntity
    TableSchema eff_schema;
    eff_schema.name = table_name;
    if (schemas_.count(table_name)) eff_schema = schemas_[table_name];
    if (!col_list.empty()) eff_schema.columns = col_list;

    // Parse multi-row tuple list: (v1,v2,...),(v3,v4,...), ...
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
        if (values_payload[pos] != '(') {
            ++pos;
            continue;
        }

        // Find matching ')' for this tuple
        size_t tuple_start = pos + 1;
        int dep = 1;
        bool in_str = false;
        char sq = '\0';
        size_t k = pos + 1;
        while (k < values_payload.size() && dep > 0) {
            char c = values_payload[k];
            if (in_str) {
                if (c == '\'' && k + 1 < values_payload.size() && values_payload[k + 1] == '\'') {
                    ++k;  // '' escape
                } else if (c == sq) {
                    in_str = false;
                }
            } else if (c == '\'') {
                in_str = true; sq = c;
            } else if (c == '(') {
                ++dep;
            } else if (c == ')') {
                --dep;
            }
            ++k;
        }
        size_t tuple_end = k - 1;
        if (dep != 0) break;

        std::string tuple_str = values_payload.substr(tuple_start, tuple_end - tuple_start);
        std::vector<std::string> values = parseInsertValues(tuple_str);

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
        } else {
            json entity = convertRowToEntity(eff_schema, values);
            THEMIS_DEBUG("SQL Server INSERT entity: {}", entity.dump());
            stats.imported_records++;
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table_name}, {"status", "imported"}}, 1.0);
        }

        pos = k;
    }

    return true;
}

std::string SQLServerImporter::mapSQLServerTypeToThemis(const std::string& sqlserver_type,
                                                         const ImportOptions& options) const {
    // User-configurable overrides take priority
    auto it = options.type_overrides.find(sqlserver_type);
    if (it != options.type_overrides.end()) return it->second;

    // Normalise: strip size specifier, e.g. "nvarchar(255)" -> "nvarchar"
    std::string base_type = sqlserver_type;
    size_t paren = base_type.find('(');
    if (paren != std::string::npos) base_type = base_type.substr(0, paren);
    std::string lower = toLower(base_type);

    // Integer types
    if (lower == "int")       return "integer";
    if (lower == "integer")   return "integer";
    if (lower == "bigint")    return "long";
    if (lower == "smallint")  return "integer";
    if (lower == "tinyint")   return "integer";

    // Boolean
    if (lower == "bit")       return "boolean";

    // Floating-point / numeric types
    if (lower == "decimal")    return "double";
    if (lower == "numeric")    return "double";
    if (lower == "money")      return "double";
    if (lower == "smallmoney") return "double";
    if (lower == "float")      return "float";
    if (lower == "real")       return "float";

    // String types
    if (lower == "char")      return "string";
    if (lower == "nchar")     return "string";
    if (lower == "varchar")   return "string";
    if (lower == "nvarchar")  return "string";
    if (lower == "text")      return "string";
    if (lower == "ntext")     return "string";

    // Binary types
    if (lower == "binary")    return "binary";
    if (lower == "varbinary") return "binary";
    if (lower == "image")     return "binary";

    // Date / time types
    if (lower == "date")            return "date";
    if (lower == "time")            return "time";
    if (lower == "datetime")        return "datetime";
    if (lower == "datetime2")       return "datetime";
    if (lower == "smalldatetime")   return "datetime";
    if (lower == "datetimeoffset")  return "datetime";

    // Special types
    if (lower == "uniqueidentifier") return "string";
    if (lower == "xml")              return "string";
    if (lower == "hierarchyid")      return "string";
    if (lower == "sql_variant")      return "string";

    // Spatial types
    if (lower == "geography") return "geo";
    if (lower == "geometry")  return "geo";

    // Versioning / binary types
    if (lower == "rowversion") return "binary";
    if (lower == "timestamp")  return "binary";

    // JSON (SQL Server 2022+)
    if (lower == "json")       return "json";

    // Prefix-based fallbacks
    if (lower.find("int")   != std::string::npos) return "integer";
    if (lower.find("char")  != std::string::npos) return "string";
    if (lower.find("text")  != std::string::npos) return "string";
    if (lower.find("date")  != std::string::npos) return "datetime";
    if (lower.find("time")  != std::string::npos) return "datetime";
    if (lower.find("binary")!= std::string::npos) return "binary";

    return "string";  // Default: treat unknown types as strings
}

bool SQLServerImporter::shouldImportTable(const std::string& table_name,
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

json SQLServerImporter::convertRowToEntity(const TableSchema& schema,
                                            const std::vector<std::string>& values) {
    json entity;
    entity["_type"] = schema.name;

    for (size_t i = 0; i < values.size() && i < schema.columns.size(); ++i) {
        entity[schema.columns[i]] = values[i];
    }

    return entity;
}

std::vector<std::string> SQLServerImporter::parseInsertValues(
    const std::string& values_clause) const {
    // Parse a single tuple's contents (the part inside the outer parentheses).
    // Handles:
    //   - NULL keyword -> empty string sentinel
    //   - Single-quoted strings: 'value' or N'value' (Unicode prefix)
    //   - '' escape inside strings (SQL standard)
    //   - Numeric literals
    std::vector<std::string> result;
    size_t i = 0;
    size_t n = values_clause.size();

    auto skipWs = [&]() {
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                         values_clause[i] == '\r' || values_clause[i] == '\n')) ++i;
    };

    while (i < n) {
        skipWs();
        if (i >= n) break;

        char c = values_clause[i];

        // N'string' Unicode prefix – skip the N and fall through to string handling
        if ((c == 'N' || c == 'n') && i + 1 < n && values_clause[i + 1] == '\'') {
            ++i;
            c = values_clause[i];
        }

        if (c == '\'') {
            // Single-quoted string with SQL Server '' escape (no backslash escaping)
            ++i;
            std::string val;
            while (i < n) {
                char sc = values_clause[i];
                if (sc == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                    // '' -> single quote
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
            // Unquoted token: NULL, number, etc.
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
                if (f == std::string::npos) token.clear();
                else token = token.substr(f, l - f + 1);
            }
            // NULL -> empty string sentinel
            std::string upper_tok;
            for (char ch : token)
                upper_tok += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (upper_tok == "NULL") token.clear();
            result.push_back(token);
        }

        skipWs();
        if (i < n && values_clause[i] == ',') ++i;
    }

    return result;
}

std::string SQLServerImporter::unquoteIdentifier(const std::string& s) {
    std::string t = s;
    // Trim surrounding whitespace
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
    // Strip double-quotes (ANSI SQL identifier)
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        return t.substr(1, t.size() - 2);
    }
    return t;
}

void SQLServerImporter::addError(ImportStats& stats, ImportErrorCode code,
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

void SQLServerImporter::emitMetric(const ImportOptions& options,
                                    const std::string& metric,
                                    const std::map<std::string, std::string>& labels,
                                    double value) const {
    if (options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void SQLServerImporter::emitSpan(const ImportOptions& options,
                                  const std::string& operation,
                                  const std::map<std::string, std::string>& attributes,
                                  double duration_seconds) const {
    if (options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void SQLServerImporter::reportProgress(ProgressCallback& callback, const std::string& stage,
                                        size_t current, size_t total) {
    if (callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// Plugin implementation
// ============================================================================

SQLServerImporterPlugin::SQLServerImporterPlugin()
    : importer_(std::make_unique<SQLServerImporter>()) {
}

plugins::PluginCapabilities SQLServerImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    caps.thread_safe        = false;
    return caps;
}

bool SQLServerImporterPlugin::initialize(const char* config_json) {
    if (!importer_) return false;
    return importer_->initialize(config_json ? config_json : "{}");
}

void SQLServerImporterPlugin::shutdown() {
    if (importer_) importer_->cancel();
}

} // namespace importers
} // namespace themis

// ============================================================================
// Plugin Entry Points
// ============================================================================
