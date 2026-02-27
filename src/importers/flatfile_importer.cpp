/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            flatfile_importer.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-27                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/flatfile_importer.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <cctype>

namespace themis {
namespace importers {

// ============================================================================
// File-level helpers
// ============================================================================

/**
 * @brief Memory-bounded line reader (mirrors pattern from other importers).
 *
 * Reads the next newline-terminated line from @p file with a hard per-line
 * byte cap of @p max_bytes (0 = unlimited).  When the cap is exceeded the
 * remaining bytes of the current line are discarded and @p truncated is set
 * to true.  Returns false only when EOF is reached before any bytes are read.
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

// ============================================================================
// Constructor / Destructor
// ============================================================================

FlatFileImporter::FlatFileImporter() = default;

FlatFileImporter::~FlatFileImporter() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> FlatFileImporter::getSupportedTypes() const {
    return {"csv", "tsv", "jsonl", "ndjson"};
}

bool FlatFileImporter::initialize(const std::string& config) {
    cancelled_ = false;
    format_    = FlatFileFormat::AUTO;
    delimiter_ = ',';
    quote_char_ = '"';
    has_header_ = true;
    table_name_.clear();

    if (!config.empty() && config != "{}") {
        try {
            auto cfg = json::parse(config);

            if (cfg.contains("format")) {
                std::string fmt = cfg["format"].get<std::string>();
                if (fmt == "csv")        format_ = FlatFileFormat::CSV;
                else if (fmt == "tsv")   format_ = FlatFileFormat::TSV;
                else if (fmt == "jsonl" ||
                         fmt == "ndjson") format_ = FlatFileFormat::JSONL;
            }

            if (cfg.contains("delimiter")) {
                std::string d = cfg["delimiter"].get<std::string>();
                if (!d.empty()) delimiter_ = d[0];
            }

            if (cfg.contains("quote_char")) {
                std::string q = cfg["quote_char"].get<std::string>();
                if (!q.empty()) quote_char_ = q[0];
            }

            if (cfg.contains("has_header")) {
                has_header_ = cfg["has_header"].get<bool>();
            }

            if (cfg.contains("table_name")) {
                table_name_ = cfg["table_name"].get<std::string>();
            }
        } catch (const std::exception& e) {
            THEMIS_INFO("FlatFile Importer: invalid config JSON: {}", e.what());
            return false;
        }
    }

    THEMIS_INFO("FlatFile Importer initialized");
    return true;
}

bool FlatFileImporter::validateSource(const std::string& source_path,
                                      std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }

    FlatFileFormat fmt = effectiveFormat(source_path);
    if (fmt == FlatFileFormat::AUTO) {
        errors.push_back(
            "Unknown file format; use .csv, .tsv, .jsonl, or .ndjson extension "
            "or set format in config");
        return false;
    }

    // For JSONL, verify that the first non-empty line parses as a JSON object.
    if (fmt == FlatFileFormat::JSONL) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '\r') continue;
            // Strip trailing \r
            if (!line.empty() && line.back() == '\r') line.pop_back();
            try {
                auto j = json::parse(line);
                if (!j.is_object()) {
                    errors.push_back("First record is not a JSON object");
                    return false;
                }
            } catch (...) {
                errors.push_back("First record is not valid JSON: " + line);
                return false;
            }
            break;
        }
    }

    THEMIS_INFO("FlatFile source validation successful: {}", source_path);
    return true;
}

ImportStats FlatFileImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();

    THEMIS_INFO("Starting flat-file import from: {}", source_path);

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

    // Resolve format and logical table name.
    FlatFileFormat fmt = effectiveFormat(source_path);
    if (fmt == FlatFileFormat::AUTO) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Cannot determine file format for: " + source_path);
        return stats;
    }

    std::string table = table_name_.empty()
                            ? filenameStem(source_path)
                            : table_name_;
    // Apply table_mappings if present.
    {
        auto it = options.table_mappings.find(table);
        if (it != options.table_mappings.end()) table = it->second;
    }

    bool ok = false;
    if (fmt == FlatFileFormat::JSONL) {
        ok = importJsonlFile(source_path, table, options, stats,
                             progress_callback);
    } else {
        ok = importCsvFile(source_path, fmt, table, options, stats,
                           progress_callback);
    }

    if (!ok && stats.structured_errors.empty()) {
        addError(stats, ImportErrorCode::FILE_READ_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Failed to read flat-file: " + source_path);
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds =
        std::chrono::duration<double>(end_time - start_time).count();

    THEMIS_INFO("Import completed: {} records imported, {} failed, {} skipped "
                "in {:.2f}s",
                stats.imported_records, stats.failed_records,
                stats.skipped_records, stats.elapsed_seconds);

    // Prometheus / OTel metrics
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "imported"}},
               static_cast<double>(stats.imported_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "failed"}},
               static_cast<double>(stats.failed_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"status", "skipped"}},
               static_cast<double>(stats.skipped_records));
    emitMetric(options, "themisdb_import_tables_total", {},
               static_cast<double>(stats.tables_processed));
    emitMetric(options, "themisdb_import_duration_seconds", {},
               stats.elapsed_seconds);
    for (const auto& e : stats.structured_errors) {
        emitMetric(options, "themisdb_import_errors_total",
                   {{"code", std::to_string(static_cast<uint32_t>(e.code))}},
                   1.0);
    }

    emitSpan(options, "import_total",
             {{"source",  source_path},
              {"tables",  std::to_string(stats.tables_processed)},
              {"rows",    std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> FlatFileImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "flatfile-import-" + std::to_string(ms) + "-" +
                     std::to_string(
                         reinterpret_cast<uintptr_t>(handle.get()) & 0xFFFF);
    }
    handle->started_at_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    handle->running.store(true);
    handle->setStage("pending");

    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    std::weak_ptr<ImportHandle> weak_handle = handle;
    ProgressCallback progress_cb =
        [weak_handle](const std::string& stage, size_t current, size_t total) {
            if (auto h = weak_handle.lock()) {
                h->current_records.store(current);
                h->total_records.store(total);
                h->setStage(stage);
            }
        };

    std::thread([this, source_path, options, progress_cb, handle,
                 promise]() mutable {
        ImportStats stats;
        try {
            stats = this->importData(source_path, options, progress_cb);
        } catch (const std::exception& e) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = std::string(
                "Unhandled exception in async flat-file import: ") + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async flat-file import worker";
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        }
        handle->running.store(false);
        handle->setStage("completed");
        handle->finished_at_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        promise->set_value(std::move(stats));
    }).detach();

    return handle;
}

void FlatFileImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("FlatFile import cancelled");
}

json FlatFileImporter::getSourceSchema(const std::string& source_path) {
    FlatFileFormat fmt = effectiveFormat(source_path);
    std::string table  = table_name_.empty()
                             ? filenameStem(source_path)
                             : table_name_;

    json result = json::array();

    if (fmt == FlatFileFormat::CSV || fmt == FlatFileFormat::TSV) {
        char delim = (fmt == FlatFileFormat::TSV) ? '\t' : delimiter_;

        std::ifstream file(source_path);
        if (!file) return result;

        std::string header_line;
        if (!std::getline(file, header_line)) return result;

        // Strip trailing \r
        if (!header_line.empty() && header_line.back() == '\r')
            header_line.pop_back();

        if (!has_header_) {
            // Generate synthetic column names: col_0, col_1, ...
            auto fields = parseCsvRow(header_line, delim, quote_char_);
            json cols   = json::array();
            json types  = json::object();
            for (size_t i = 0; i < fields.size(); ++i) {
                std::string cname = "col_" + std::to_string(i);
                cols.push_back(cname);
                types[cname] = "string";
            }
            result.push_back({{"name", table},
                              {"columns", cols},
                              {"column_types", types},
                              {"primary_keys", json::array()}});
        } else {
            auto cols_vec = parseCsvRow(header_line, delim, quote_char_);
            json cols     = json::array();
            json types    = json::object();
            for (const auto& c : cols_vec) {
                cols.push_back(c);
                types[c] = "string";
            }
            result.push_back({{"name", table},
                              {"columns", cols},
                              {"column_types", types},
                              {"primary_keys", json::array()}});
        }
    } else if (fmt == FlatFileFormat::JSONL) {
        std::ifstream file(source_path);
        if (!file) return result;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line == "\r") continue;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            try {
                auto obj = json::parse(line);
                if (!obj.is_object()) continue;
                json cols  = json::array();
                json types = json::object();
                for (auto& [key, val] : obj.items()) {
                    cols.push_back(key);
                    if (val.is_number_integer())     types[key] = "integer";
                    else if (val.is_number_float())  types[key] = "double";
                    else if (val.is_boolean())       types[key] = "boolean";
                    else                             types[key] = "string";
                }
                result.push_back({{"name", table},
                                  {"columns", cols},
                                  {"column_types", types},
                                  {"primary_keys", json::array()}});
                break;
            } catch (...) {}
        }
    }

    return result;
}

// ============================================================================
// Static helpers
// ============================================================================

FlatFileFormat FlatFileImporter::detectFormat(const std::string& path) {
    // Extract extension (last '.' segment, lower-cased)
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return FlatFileFormat::AUTO;

    std::string ext = path.substr(dot + 1);
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (ext == "csv")                    return FlatFileFormat::CSV;
    if (ext == "tsv")                    return FlatFileFormat::TSV;
    if (ext == "jsonl" || ext == "ndjson") return FlatFileFormat::JSONL;
    return FlatFileFormat::AUTO;
}

FlatFileFormat FlatFileImporter::effectiveFormat(const std::string& path) const {
    if (format_ != FlatFileFormat::AUTO) return format_;
    return detectFormat(path);
}

std::string FlatFileImporter::filenameStem(const std::string& path) {
    // Extract basename
    size_t slash = path.rfind('/');
    if (slash == std::string::npos) slash = path.rfind('\\');
    std::string base = (slash != std::string::npos)
                           ? path.substr(slash + 1)
                           : path;

    // Strip extension
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    return base.empty() ? "data" : base;
}

// ============================================================================
// CSV row parser
// ============================================================================

std::vector<std::string> FlatFileImporter::parseCsvRow(const std::string& line,
                                                        char delim,
                                                        char quote) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    size_t n = line.size();

    for (size_t i = 0; i < n; ++i) {
        char c = line[i];

        if (in_quotes) {
            if (c == quote) {
                // Peek ahead for doubled-quote escape
                if (i + 1 < n && line[i + 1] == quote) {
                    field += quote;
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == quote) {
                in_quotes = true;
            } else if (c == delim) {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.push_back(field);
    return fields;
}

// ============================================================================
// CSV / TSV import
// ============================================================================

bool FlatFileImporter::importCsvFile(const std::string& path,
                                     FlatFileFormat fmt,
                                     const std::string& table,
                                     const ImportOptions& options,
                                     ImportStats& stats,
                                     ProgressCallback& cb) {
    std::ifstream file(path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + path);
        return false;
    }

    // Check table filter
    if (!shouldImportTable(table, options)) {
        addError(stats, ImportErrorCode::TABLE_EXCLUDED,
                 ImportErrorSeverity::INFO,
                 "Table excluded by filter: " + table);
        stats.skipped_records++;
        return true;
    }

    const char delim = (fmt == FlatFileFormat::TSV) ? '\t' : delimiter_;
    const size_t row_limit  = options.max_row_size_bytes;
    const size_t line_limit = row_limit > 0 ? row_limit : 64 * 1024 * 1024ULL;

    std::vector<std::string> columns;
    size_t line_number = 0;

    // ---- Read header row (if present) ----
    if (has_header_) {
        std::string header_line;
        bool truncated = false;
        if (!streamReadLine(file, header_line, line_limit, truncated)) {
            addError(stats, ImportErrorCode::FILE_READ_FAILED,
                     ImportErrorSeverity::CRITICAL,
                     "Empty file (expected header row): " + path);
            return false;
        }
        ++line_number;
        if (!header_line.empty() && header_line.back() == '\r')
            header_line.pop_back();

        columns = parseCsvRow(header_line, delim, quote_char_);

        // Apply column_mappings
        for (auto& col : columns) {
            auto it = options.column_mappings.find(col);
            if (it != options.column_mappings.end()) col = it->second;
        }
    }

    stats.tables_processed++;
    reportProgress(cb, "importing table " + table, 0, 0);

    emitSpan(options, "parse_table",
             {{"table", table}}, 0.0);

    // ---- Read data rows ----
    std::string line;
    bool line_truncated = false;
    size_t row_index    = 0;

    while (streamReadLine(file, line, line_limit, line_truncated) &&
           !cancelled_) {
        ++line_number;
        ++row_index;

        if (line_truncated) {
            addError(stats, ImportErrorCode::ROW_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "Row too long (> " + std::to_string(row_limit) +
                         " bytes); skipped",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back(
                "Row truncated at line " + std::to_string(line_number));
            stats.failed_records++;
            stats.total_records++;
            if (!options.continue_on_error) return false;
            continue;
        }

        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip blank lines
        if (line.empty()) continue;

        stats.total_records++;

        // If no header was read, generate synthetic column names on first row
        if (columns.empty()) {
            auto first_fields = parseCsvRow(line, delim, quote_char_);
            for (size_t i = 0; i < first_fields.size(); ++i)
                columns.push_back("col_" + std::to_string(i));
        }

        // UTF-8 validation
        if (options.enforce_utf8 && !isValidUtf8(line)) {
            addError(stats, ImportErrorCode::INVALID_UTF8,
                     ImportErrorSeverity::ERROR,
                     "Invalid UTF-8 in row",
                     "line " + std::to_string(line_number));
            stats.failed_records++;
            emitMetric(options, "themisdb_import_errors_total",
                       {{"code", std::to_string(
                             static_cast<uint32_t>(ImportErrorCode::INVALID_UTF8))}},
                       1.0);
            if (!options.continue_on_error) return false;
            continue;
        }

        auto fields = parseCsvRow(line, delim, quote_char_);

        // Column count mismatch – pad with empty or truncate
        while (fields.size() < columns.size()) fields.emplace_back();
        if (fields.size() > columns.size()) fields.resize(columns.size());

        // Build entity JSON
        json entity = json::object();
        for (size_t i = 0; i < columns.size(); ++i) {
            entity[columns[i]] = fields[i];
        }

        // Dry-run: count but don't invoke streaming callback
        if (options.dry_run) {
            addError(stats, ImportErrorCode::DRY_RUN_ONLY,
                     ImportErrorSeverity::INFO,
                     "dry-run: row not written",
                     "line " + std::to_string(line_number));
            stats.imported_records++;
            continue;
        }

        // Streaming callback
        if (options.streaming_row_callback) {
            if (!options.streaming_row_callback(table, entity)) {
                THEMIS_INFO("Streaming callback aborted import at line {}",
                            line_number);
                return true;
            }
        }

        stats.imported_records++;

        if (row_index % options.batch_size == 0) {
            reportProgress(cb, "importing table " + table,
                           stats.imported_records, 0);
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table}, {"status", "imported"}},
                       static_cast<double>(options.batch_size));
        }
    }

    return true;
}

// ============================================================================
// JSONL import
// ============================================================================

bool FlatFileImporter::importJsonlFile(const std::string& path,
                                        const std::string& table,
                                        const ImportOptions& options,
                                        ImportStats& stats,
                                        ProgressCallback& cb) {
    std::ifstream file(path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + path);
        return false;
    }

    // Check table filter
    if (!shouldImportTable(table, options)) {
        addError(stats, ImportErrorCode::TABLE_EXCLUDED,
                 ImportErrorSeverity::INFO,
                 "Table excluded by filter: " + table);
        stats.skipped_records++;
        return true;
    }

    const size_t row_limit  = options.max_row_size_bytes;
    const size_t line_limit = row_limit > 0 ? row_limit : 64 * 1024 * 1024ULL;

    stats.tables_processed++;
    reportProgress(cb, "importing table " + table, 0, 0);
    emitSpan(options, "parse_table", {{"table", table}}, 0.0);

    std::string line;
    bool line_truncated = false;
    size_t line_number  = 0;
    size_t row_index    = 0;

    while (streamReadLine(file, line, line_limit, line_truncated) &&
           !cancelled_) {
        ++line_number;

        if (line_truncated) {
            addError(stats, ImportErrorCode::ROW_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "Row too long (> " + std::to_string(row_limit) +
                         " bytes); skipped",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back(
                "Row truncated at line " + std::to_string(line_number));
            stats.failed_records++;
            stats.total_records++;
            if (!options.continue_on_error) return false;
            continue;
        }

        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Skip blank lines
        if (line.empty()) continue;

        stats.total_records++;
        ++row_index;

        // UTF-8 validation
        if (options.enforce_utf8 && !isValidUtf8(line)) {
            addError(stats, ImportErrorCode::INVALID_UTF8,
                     ImportErrorSeverity::ERROR,
                     "Invalid UTF-8 in row",
                     "line " + std::to_string(line_number));
            stats.failed_records++;
            if (!options.continue_on_error) return false;
            continue;
        }

        json entity;
        try {
            entity = json::parse(line);
        } catch (const std::exception& e) {
            addError(stats, ImportErrorCode::PARSE_INSERT,
                     ImportErrorSeverity::ERROR,
                     std::string("JSON parse error: ") + e.what(),
                     "line " + std::to_string(line_number));
            stats.errors.push_back("JSON parse error at line " +
                                   std::to_string(line_number));
            stats.failed_records++;
            if (!options.continue_on_error) return false;
            continue;
        }

        if (!entity.is_object()) {
            addError(stats, ImportErrorCode::PARSE_INSERT,
                     ImportErrorSeverity::ERROR,
                     "JSONL line is not a JSON object",
                     "line " + std::to_string(line_number));
            stats.failed_records++;
            if (!options.continue_on_error) return false;
            continue;
        }

        // Apply column_mappings: rename keys
        if (!options.column_mappings.empty()) {
            json remapped = json::object();
            for (auto& [key, val] : entity.items()) {
                auto it = options.column_mappings.find(key);
                remapped[it != options.column_mappings.end() ? it->second : key]
                    = val;
            }
            entity = std::move(remapped);
        }

        // Dry-run
        if (options.dry_run) {
            addError(stats, ImportErrorCode::DRY_RUN_ONLY,
                     ImportErrorSeverity::INFO,
                     "dry-run: row not written",
                     "line " + std::to_string(line_number));
            stats.imported_records++;
            continue;
        }

        // Streaming callback
        if (options.streaming_row_callback) {
            if (!options.streaming_row_callback(table, entity)) {
                THEMIS_INFO("Streaming callback aborted import at line {}",
                            line_number);
                return true;
            }
        }

        stats.imported_records++;

        if (row_index % options.batch_size == 0) {
            reportProgress(cb, "importing table " + table,
                           stats.imported_records, 0);
            emitMetric(options, "themisdb_import_rows_total",
                       {{"table", table}, {"status", "imported"}},
                       static_cast<double>(options.batch_size));
        }
    }

    return true;
}

// ============================================================================
// Utility helpers
// ============================================================================

bool FlatFileImporter::shouldImportTable(const std::string& table_name,
                                          const ImportOptions& options) const {
    // Check include list
    if (!options.include_tables.empty()) {
        bool found = false;
        for (const auto& t : options.include_tables) {
            if (t == table_name) { found = true; break; }
        }
        if (!found) return false;
    }

    // Check exclude list
    for (const auto& t : options.exclude_tables) {
        if (t == table_name) return false;
    }

    return true;
}

void FlatFileImporter::addError(ImportStats& stats, ImportErrorCode code,
                                 ImportErrorSeverity severity,
                                 const std::string& message,
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

void FlatFileImporter::emitMetric(const ImportOptions& options,
                                   const std::string& metric,
                                   const std::map<std::string, std::string>& labels,
                                   double value) const {
    if (options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void FlatFileImporter::emitSpan(const ImportOptions& options,
                                 const std::string& operation,
                                 const std::map<std::string, std::string>& attributes,
                                 double duration_seconds) const {
    if (options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void FlatFileImporter::reportProgress(ProgressCallback& callback,
                                       const std::string& stage,
                                       size_t current, size_t total) {
    if (callback) {
        callback(stage, current, total);
    }
}

bool FlatFileImporter::isValidUtf8(const std::string& s) {
    const unsigned char* bytes =
        reinterpret_cast<const unsigned char*>(s.data());
    size_t n = s.size();
    for (size_t i = 0; i < n;) {
        unsigned char b = bytes[i];
        size_t trail = 0;
        if (b < 0x80) {
            ++i; continue;
        } else if ((b & 0xE0) == 0xC0) {
            trail = 1;
        } else if ((b & 0xF0) == 0xE0) {
            trail = 2;
        } else if ((b & 0xF8) == 0xF0) {
            trail = 3;
        } else {
            return false;
        }
        ++i;
        for (size_t j = 0; j < trail; ++j, ++i) {
            if (i >= n || (bytes[i] & 0xC0) != 0x80) return false;
        }
    }
    return true;
}

// ============================================================================
// Plugin implementation
// ============================================================================

FlatFileImporterPlugin::FlatFileImporterPlugin()
    : importer_(std::make_unique<FlatFileImporter>()) {}

plugins::PluginCapabilities FlatFileImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    caps.thread_safe        = false;
    return caps;
}

bool FlatFileImporterPlugin::initialize(const char* config_json) {
    if (!importer_) return false;
    return importer_->initialize(config_json ? config_json : "{}");
}

void FlatFileImporterPlugin::shutdown() {
    if (importer_) importer_->cancel();
}

} // namespace importers
} // namespace themis

// ============================================================================
// Plugin Entry Points
// ============================================================================
