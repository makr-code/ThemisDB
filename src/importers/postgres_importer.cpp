/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            postgres_importer.cpp                              ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   78.0/100                                       ║
    • Total Lines:     1442                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/postgres_importer.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <unordered_set>
#include <cinttypes>

namespace themis {
namespace importers {

// ============================================================================
// File-level helpers
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
    std::string current;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (in_string) {
            current += c;
            if (c == '\'') {
                // PostgreSQL '' escape: two consecutive single-quotes inside a string
                if (i + 1 < s.size() && s[i + 1] == '\'') {
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
    if (!current.empty()) result.push_back(current);
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
    for (size_t k = open_pos; k < sql.size(); ++k) {
        char c = sql[k];
        if (in_string) {
            if (c == '\'' && k + 1 < sql.size() && sql[k + 1] == '\'') {
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
            if (depth == 0) return k;
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
static bool streamReadLine(std::istream& file,
                           std::string& line,
                           size_t max_bytes,
                           bool& truncated) {
    truncated = false;
    line.clear();

    if (max_bytes == 0) {
        // Unlimited – plain std::getline
        if (!std::getline(file, line)) return false;
        return true;
    }

    // Character-by-character read respecting the cap
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
            // Cap exceeded – drain to the next newline without storing
            truncated = true;
            while (file.get(c) && c != '\n') { /* discard */ }
            break;
        }
    }

    return got_any;
}


PostgreSQLImporter::PostgreSQLImporter() {
}

PostgreSQLImporter::~PostgreSQLImporter() {
    cancel();
}

std::vector<std::string> PostgreSQLImporter::getSupportedTypes() const {
    return {"postgresql", "postgres", "pg_dump"};
}

bool PostgreSQLImporter::initialize(const std::string& config) {
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
    std::string line;
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

json PostgreSQLImporter::getSourceSchema(const std::string& source_path) {
    schemas_.clear();
    
    std::ifstream file(source_path);
    if (!file) {
        return json::array();
    }
    
    std::string line;
    std::string current_sql;
    
    while (std::getline(file, line)) {
        // Skip comments
        if (line.empty() || line[0] == '-') continue;
        
        current_sql += line + " ";
        
        // Complete statement?
        if (line.find(';') != std::string::npos) {
            if (current_sql.find("CREATE TABLE") != std::string::npos) {
                TableSchema schema;
                if (parseCreateTable(current_sql, schema)) {
                    schemas_[schema.name] = schema;
                }
            }
            current_sql.clear();
        }
    }
    
    // Convert to JSON
    json result = json::array();
    for (const auto& [name, schema] : schemas_) {
        json table_json = {
            {"name", schema.name},
            {"schema", schema.schema},
            {"columns", schema.columns},
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
        std::string hdr_line;
        int hdr_lines = 0;
        bool hdr_trunc = false;
        std::streampos after_header = 0;
        while (streamReadLine(file, hdr_line, 4096, hdr_trunc) && hdr_lines < 50) {
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
    std::unordered_set<uint64_t> delta_hashes;
    if (!options.delta_hash_file.empty()) {
        delta_hashes = loadDeltaHashes(options.delta_hash_file);
        THEMIS_INFO("Delta import: loaded {} known hashes from {}", delta_hashes.size(),
                    options.delta_hash_file);
    }

    // --- Checkpoint / resume support ---
    std::streampos resume_offset = 0;
    if (!options.checkpoint_file.empty()) {
        ImportStats dummy;
        if (loadCheckpoint(options.checkpoint_file, resume_offset, dummy)) {
            THEMIS_INFO("Resuming import from byte offset {}", static_cast<long>(resume_offset));
            file.seekg(resume_offset);
            // Carry accumulated counts from the checkpoint
            stats.imported_records = dummy.imported_records;
            stats.failed_records   = dummy.failed_records;
            stats.skipped_records  = dummy.skipped_records;
            stats.total_records    = dummy.total_records;
            stats.tables_processed = dummy.tables_processed;
        }
    }

    std::string line;
    std::string current_sql;
    size_t line_number = 0;
    size_t batch_row_count = 0;

    // Per-line read limit: cap single-line allocation to max_statement_size_bytes
    // (or a safe default of 64 MB) so a crafted dump with no newlines cannot OOM.
    const size_t line_read_limit = options.max_statement_size_bytes > 0
                                   ? options.max_statement_size_bytes
                                   : 64 * 1024 * 1024ULL;  // 64 MB default cap

    bool line_truncated = false;
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

        // Skip blank lines and SQL comments
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
            continue;
        }
        
        current_sql += line + " ";

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
        
        // Complete statement?
        if (line.find(';') != std::string::npos) {
            // Parse different statement types
            if (current_sql.find("CREATE TABLE") != std::string::npos ||
                current_sql.find("CREATE SCHEMA") != std::string::npos) {
                auto t0 = std::chrono::steady_clock::now();
                TableSchema schema;
                if (parseCreateTable(current_sql, schema)) {
                    if (shouldImportTable(schema.name, options)) {
                        schemas_[schema.name] = schema;
                        stats.tables_processed++;
                        THEMIS_DEBUG("Parsed table schema: {}", schema.name);
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
                std::regex enum_regex(
                    R"(CREATE TYPE\s+(?:\w+\.)?(\w+)\s+AS\s+ENUM)",
                    std::regex_constants::icase);
                std::regex comp_regex(
                    R"(CREATE TYPE\s+(?:\w+\.)?(\w+)\s+AS\s*\()",
                    std::regex_constants::icase);
                std::smatch tm;
                if (std::regex_search(current_sql, tm, enum_regex)) {
                    custom_type_map_[tm[1].str()] = "string";
                    stats.custom_types_processed++;
                    THEMIS_DEBUG("Registered enum type: {} -> string", tm[1].str());
                } else if (std::regex_search(current_sql, tm, comp_regex)) {
                    custom_type_map_[tm[1].str()] = "object";
                    stats.custom_types_processed++;
                    THEMIS_DEBUG("Registered composite type: {} -> object", tm[1].str());
                }
            }
            // ALTER TABLE ... ADD COLUMN – update cached schema so subsequent COPY
            // and INSERT statements see the new column.
            else if (current_sql.find("ALTER TABLE") != std::string::npos &&
                     current_sql.find("ADD COLUMN") != std::string::npos) {
                // Pattern: ALTER TABLE [ONLY] [schema.]table ADD COLUMN name type
                std::regex alter_regex(
                    R"(ALTER TABLE\s+(?:ONLY\s+)?(?:\w+\.)?(\w+)\s+ADD COLUMN\s+(\w+)\s+(\S+))",
                    std::regex_constants::icase);
                std::smatch am;
                if (std::regex_search(current_sql, am, alter_regex)) {
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
            else if (current_sql.find("INSERT INTO") != std::string::npos) {
                stats.total_records++;
                if (!options.dry_run) {
                    auto t0 = std::chrono::steady_clock::now();
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
                std::regex copy_regex(
                    R"(COPY\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+FROM\s+stdin)",
                    std::regex_constants::icase);
                std::smatch match;
                if (std::regex_search(current_sql, match, copy_regex)) {
                    std::string table_name = match[1].str();
                    std::vector<std::string> col_list;
                    if (match[2].matched && !match[2].str().empty()) {
                        std::istringstream css(match[2].str());
                        std::string col;
                        while (std::getline(css, col, ',')) {
                            col.erase(0, col.find_first_not_of(" \t"));
                            col.erase(col.find_last_not_of(" \t") + 1);
                            if (!col.empty()) col_list.push_back(col);
                        }
                    }
                    size_t before_copy = stats.imported_records;
                    auto t0 = std::chrono::steady_clock::now();
                    if (!options.dry_run) {
                        parseCopy(file, table_name, col_list, options, stats, delta_hashes);
                    } else {
                        // skip COPY data block in dry-run mode using bounded reader
                        std::string skip_line;
                        bool skip_trunc = false;
                        while (streamReadLine(file, skip_line, line_read_limit, skip_trunc)) {
                            if (skip_line == "\\." || skip_line.rfind("\\.", 0) == 0) break;
                            stats.total_records++;
                        }
                    }
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

            // Checkpoint after each batch
            if (!options.checkpoint_file.empty() &&
                options.batch_size > 0 &&
                batch_row_count >= options.batch_size) {
                std::streampos current_pos = file.tellg();
                saveCheckpoint(options.checkpoint_file, current_pos, stats);
                batch_row_count = 0;
                reportProgress(callback, "data", stats.imported_records, 0);
            }
        }
    }

    // Final checkpoint on clean completion
    if (!options.checkpoint_file.empty() && !cancelled_) {
        saveCheckpoint(options.checkpoint_file, file.tellg(), stats);
    }

    // Save updated delta hashes
    if (!options.delta_hash_file.empty() && !delta_hashes.empty()) {
        saveDeltaHashes(options.delta_hash_file, delta_hashes);
    }
    
    return !cancelled_;
}

bool PostgreSQLImporter::parseCreateTable(const std::string& sql, TableSchema& schema) {
    // Regex-based parsing for CREATE TABLE statements.
    // Handles schema-qualified names: CREATE TABLE [schema.]table (...)
    std::regex table_regex(R"(CREATE TABLE\s+(?:(\w+)\.)?(\w+)\s*\()");
    std::smatch match;
    
    if (std::regex_search(sql, match, table_regex)) {
        if (match.size() > 2) {
            schema.schema = match[1].str();
            schema.name = match[2].str();
        } else {
            schema.name = match[1].str();
        }

        // Find the first '(' after the table name and its matching ')'.
        // Using findMatchingParen() instead of find_last_of(')') so that nested
        // parens inside column defaults and constraints are handled correctly.
        size_t start = sql.find('(', match.position());
        if (start == std::string::npos) return !schema.name.empty();
        size_t end = findMatchingParen(sql, start);
        if (end == std::string::npos) return !schema.name.empty();

        std::string columns_str = sql.substr(start + 1, end - start - 1);

        // Split using a paren+quote-aware splitter so that commas inside
        // DEFAULT expressions, CHECK constraints, and type arguments are
        // not treated as column separators.
        std::vector<std::string> column_defs = splitTopLevelCommas(columns_str);

        for (auto& column_def : column_defs) {
                // Trim whitespace
                column_def.erase(0, column_def.find_first_not_of(" \t\n\r"));
                column_def.erase(column_def.find_last_not_of(" \t\n\r") + 1);
                
                if (column_def.empty()) continue;

                // Skip table-level constraints
                if (column_def.find("CONSTRAINT") != std::string::npos ||
                    column_def.find("PRIMARY KEY") != std::string::npos ||
                    column_def.find("FOREIGN KEY") != std::string::npos ||
                    column_def.find("UNIQUE") != std::string::npos ||
                    column_def.find("CHECK") != std::string::npos) {
                    continue;
                }
                
                // Extract column name and type
                std::istringstream col_ss(column_def);
                std::string col_name, col_type;
                col_ss >> col_name >> col_type;
                
                // Strip surrounding quotes from column name
                if (!col_name.empty() && col_name.front() == '"') {
                    col_name = col_name.substr(1, col_name.size() - 2);
                }

                if (!col_name.empty() && !col_type.empty()) {
                    schema.columns.push_back(col_name);
                    schema.column_types[col_name] = col_type;
                }
        }
        
        return !schema.name.empty();
    }
    
    return false;
}

bool PostgreSQLImporter::parseInsert(const std::string& sql, const ImportOptions& options,
                                      ImportStats& stats, size_t line_number) {
    // Extract table name: INSERT INTO [schema.]table [(col1,...)] VALUES (...)
    std::regex insert_regex(R"(INSERT INTO\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+VALUES\s*\((.+)\)\s*;?\s*$)",
                            std::regex_constants::icase);
    std::smatch match;
    
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
    std::vector<std::string> col_list;
    if (match[2].matched && !match[2].str().empty()) {
        std::istringstream css(match[2].str());
        std::string col;
        while (std::getline(css, col, ',')) {
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            if (!col.empty()) col_list.push_back(col);
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
    if (schemas_.count(table_name)) eff_schema = schemas_[table_name];
    if (!col_list.empty()) eff_schema.columns = col_list;

    json entity = convertRowToEntity(eff_schema, values);
    THEMIS_DEBUG("INSERT entity: {}", entity.dump());

    stats.imported_records++;
    return true;
}

bool PostgreSQLImporter::parseCopy(std::ifstream& file, const std::string& table_name,
                                    const std::vector<std::string>& columns,
                                    const ImportOptions& options, ImportStats& stats,
                                    std::unordered_set<uint64_t>& delta_hashes) {
    if (!shouldImportTable(table_name, options)) {
        // Skip until end marker – use bounded reader so the skip itself is safe
        std::string line;
        bool trunc = false;
        const size_t skip_limit = options.max_row_size_bytes > 0
                                  ? options.max_row_size_bytes * 2
                                  : 64 * 1024 * 1024ULL;
        while (streamReadLine(file, line, skip_limit, trunc)) {
            if (line == "\\." || line.rfind("\\.", 0) == 0) break;
            stats.skipped_records++;
        }
        return true;
    }

    // Resolve effective column list from schema or provided list
    TableSchema eff_schema;
    if (schemas_.count(table_name)) eff_schema = schemas_[table_name];
    if (!columns.empty()) eff_schema.columns = columns;
    eff_schema.name = table_name;

    // Per-row read limit: cap single-row allocation to max_row_size_bytes
    // (or a safe default of 64 MB) to prevent OOM from adversarial dumps.
    const size_t row_read_limit = options.max_row_size_bytes > 0
                                  ? options.max_row_size_bytes
                                  : 64 * 1024 * 1024ULL;  // 64 MB default cap

    std::string line;
    size_t row_num = 0;
    bool first_data_line = true;
    bool row_truncated = false;
    while (streamReadLine(file, line, row_read_limit, row_truncated) && !cancelled_) {
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
            writeQuarantineRow(options.quarantine_file, table_name,
                               "<truncated at " + std::to_string(row_read_limit) + " bytes>", err);
            if (!options.continue_on_error) { stats.failed_records++; return false; }
            stats.failed_records++;
            stats.quarantined_records++;
            continue;
        }

        // --- Binary COPY format detection (first data line) ---
        // PostgreSQL binary COPY starts with the signature: "PGCOPY\n\xff\r\n\0"
        if (first_data_line) {
            first_data_line = false;
            if (line.size() >= 6 && line.compare(0, 6, "PGCOPY") == 0) {
                addError(stats, ImportErrorCode::BINARY_COPY_FORMAT,
                         ImportErrorSeverity::ERROR,
                         "Binary COPY format detected for table '" + table_name +
                         "'; only text-format COPY is supported. "
                         "Re-export the dump without --format=binary.",
                         "table " + table_name);
                stats.errors.push_back("Binary COPY unsupported in table " + table_name);
                if (!options.continue_on_error) return false;
                // Skip remaining lines of this COPY block using the bounded reader
                bool skip_trunc = false;
                while (streamReadLine(file, line, row_read_limit, skip_trunc)) {
                    if (line == "\\." || line.rfind("\\.", 0) == 0) break;
                }
                return true;
            }
        }
        
        row_num++;
        stats.total_records++;

        // Row-size guard
        if (options.max_row_size_bytes > 0 && line.size() > options.max_row_size_bytes) {
            ImportError err;
            err.code     = ImportErrorCode::ROW_TOO_LARGE;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "COPY row exceeds max_row_size_bytes (" +
                           std::to_string(options.max_row_size_bytes) + ")";
            err.location = "table " + table_name + ", row " + std::to_string(row_num);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back("Row too large in table " + table_name +
                                     " row " + std::to_string(row_num));
            writeQuarantineRow(options.quarantine_file, table_name, line, err);
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
            writeQuarantineRow(options.quarantine_file, table_name, line, err);
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

        if (!eff_schema.columns.empty() && values.size() != eff_schema.columns.size()) {
            ImportError err;
            err.code     = ImportErrorCode::COLUMN_COUNT_MISMATCH;
            err.severity = ImportErrorSeverity::WARNING;
            err.message  = "COPY row has " + std::to_string(values.size()) +
                           " columns, expected " + std::to_string(eff_schema.columns.size());
            err.location = "table " + table_name + ", row " + std::to_string(row_num);
            stats.structured_errors.push_back(err);
            stats.warnings.push_back("Column count mismatch in table " + table_name +
                                     " row " + std::to_string(row_num));
            writeQuarantineRow(options.quarantine_file, table_name, line, err);
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

        stats.imported_records++;
        emitMetric(options, "themisdb_import_rows_total",
                   {{"table", table_name}, {"status", "imported"}}, 1.0);
    }
    
    return true;
}

std::vector<std::string> PostgreSQLImporter::parseCopyRow(const std::string& line) const {
    // PostgreSQL COPY text format: columns separated by TAB.
    // Special sequences: \N = SQL NULL, \t = tab, \n = newline, \r = CR, \\ = backslash.
    std::vector<std::string> result;
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
    std::string out;
    out.reserve(val.size());
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
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
    size_t i = 0;
    const size_t n = values_clause.size();

    while (i < n) {
        // Skip leading whitespace
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t')) ++i;
        if (i >= n) break;

        if (values_clause[i] == '\'') {
            // Quoted string
            ++i;
            std::string val;
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
            while (i < n && values_clause[i] != ',' && values_clause[i] != ')') ++i;
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
    auto it = options.type_overrides.find(pg_type);
    if (it != options.type_overrides.end()) {
        return it->second;
    }

    std::string lower_type = pg_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);

    // Also check lowercase override
    it = options.type_overrides.find(lower_type);
    if (it != options.type_overrides.end()) {
        return it->second;
    }

    // Check custom types discovered from CREATE TYPE statements in the dump.
    // Check both the original and lowercased form of the type name.
    auto ct = custom_type_map_.find(pg_type);
    if (ct != custom_type_map_.end()) return ct->second;
    ct = custom_type_map_.find(lower_type);
    if (ct != custom_type_map_.end()) return ct->second;

    // Array types
    if (lower_type.back() == ']' || lower_type.find("[]") != std::string::npos ||
        lower_type.rfind("array", 0) == 0) {
        return "array";
    }

    // Exact / prefix matches for PostgreSQL built-in types
    if (lower_type == "bigserial" || lower_type == "bigint" || lower_type == "int8") return "long";
    if (lower_type == "smallint" || lower_type == "int2" || lower_type == "smallserial") return "integer";
    if (lower_type == "integer" || lower_type == "int" || lower_type == "int4" ||
        lower_type == "serial" || lower_type == "serial4") return "integer";
    if (lower_type == "real" || lower_type == "float4") return "float";
    if (lower_type == "double precision" || lower_type == "float8") return "double";
    if (lower_type == "numeric" || lower_type == "decimal") return "double";
    if (lower_type == "money") return "double";
    if (lower_type == "boolean" || lower_type == "bool") return "boolean";
    if (lower_type == "text" || lower_type == "name") return "string";
    if (lower_type == "uuid") return "string";
    if (lower_type == "inet" || lower_type == "cidr" || lower_type == "macaddr" ||
        lower_type == "macaddr8") return "string";
    if (lower_type == "xml") return "string";
    if (lower_type == "bytea") return "binary";
    if (lower_type == "json" || lower_type == "jsonb") return "json";
    if (lower_type == "interval") return "string";
    if (lower_type == "point" || lower_type == "line" || lower_type == "lseg" ||
        lower_type == "box" || lower_type == "path" || lower_type == "polygon" ||
        lower_type == "circle") return "geo";
    if (lower_type == "tsvector" || lower_type == "tsquery") return "string";
    if (lower_type == "oid" || lower_type == "xid" || lower_type == "cid") return "integer";

    // Prefix-based fallbacks
    if (lower_type.find("int") != std::string::npos) return "integer";
    if (lower_type.find("serial") != std::string::npos) return "integer";
    if (lower_type.find("float") != std::string::npos) return "double";
    if (lower_type.find("char") != std::string::npos) return "string";
    if (lower_type.find("varchar") != std::string::npos) return "string";
    if (lower_type.find("timestamp") != std::string::npos) return "datetime";
    if (lower_type.find("date") != std::string::npos) return "date";
    if (lower_type.find("time") != std::string::npos) return "time";
    if (lower_type.find("json") != std::string::npos) return "json";

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
    
    for (size_t i = 0; i < values.size() && i < schema.columns.size(); i++) {
        entity[schema.columns[i]] = values[i];
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

void PostgreSQLImporter::emitMetric(const ImportOptions& options,
                                     const std::string& metric,
                                     const std::map<std::string, std::string>& labels,
                                     double value) const {
    if (options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void PostgreSQLImporter::emitSpan(const ImportOptions& options,
                                   const std::string& operation,
                                   const std::map<std::string, std::string>& attributes,
                                   double duration_seconds) const {
    if (options.tracing_callback) {
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
    const size_t len  = s.size();
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
        if (extra == 1 && codepoint < 0x80)   return false;
        if (extra == 2 && codepoint < 0x800)  return false;
        if (extra == 3 && codepoint < 0x10000) return false;
        // Reject surrogates (U+D800–U+DFFF)
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return false;
        // Reject values above U+10FFFF
        if (codepoint > 0x10FFFF) return false;
        i += 1 + extra;
    }
    return true;
}

bool PostgreSQLImporter::loadCheckpoint(const std::string& checkpoint_file,
                                         std::streampos& offset,
                                         ImportStats& accumulated_stats) const {
    std::ifstream f(checkpoint_file);
    if (!f) return false;

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
    if (callback) {
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
    if (quarantine_file.empty()) return;
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
        return fnv1a64(raw_row.data(), raw_row.size());
    }
    // Hash only the key column values, separated by a non-printable sentinel
    static constexpr char kDeltaHashFieldSep = '\x01';
    std::string key_data;
    for (const auto& kc : key_columns) {
        auto it = std::find(schema_columns.begin(), schema_columns.end(), kc);
        if (it != schema_columns.end()) {
            size_t idx = static_cast<size_t>(it - schema_columns.begin());
            if (idx < values.size()) {
                key_data += values[idx];
            }
        }
        key_data += kDeltaHashFieldSep;
    }
    return fnv1a64(key_data.data(), key_data.size());
}

std::unordered_set<uint64_t> PostgreSQLImporter::loadDeltaHashes(const std::string& delta_hash_file) {
    std::unordered_set<uint64_t> hashes;
    std::ifstream f(delta_hash_file);
    if (!f) return hashes;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        try {
            hashes.insert(std::stoull(line, nullptr, 16));
        } catch (...) {}
    }
    return hashes;
}

void PostgreSQLImporter::saveDeltaHashes(const std::string& delta_hash_file,
                                          const std::unordered_set<uint64_t>& hashes) {
    std::ofstream f(delta_hash_file, std::ios::trunc);
    if (!f) return;
    for (uint64_t h : hashes) {
        // Write as 16-character zero-padded hex
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
        f << buf << "\n";
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

