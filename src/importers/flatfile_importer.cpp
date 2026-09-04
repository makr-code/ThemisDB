/**
 * @file flatfile_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=9, M=19, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/flatfile_importer.h"
#include <stdexcept>
#include "importers/schema_validator.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <cctype>

#ifdef ARROW_ENABLED
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#endif

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
static bool streamReadLineFlat(std::istream& file,
                            std::string& line,
                            size_t max_bytes,
                            bool& truncated) {
    truncated = false;
    line.clear();

    if (max_bytes == 0) {
        if (!std::getline(file, line)) {
          return false;
        }
        return true;
    }

    char c = '\0';
    size_t count = 0;
    bool got_any = false;

    while (file.get(c)) {
        got_any = true;
        if (c == '\n') {
          break;
        }
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
    return {"csv", "tsv", "jsonl", "ndjson", "parquet"};
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
                if (fmt == "csv") {
                  format_ = FlatFileFormat::CSV;
                }
                else if (fmt == "tsv")   format_ = FlatFileFormat::TSV;
                else if (fmt == "jsonl" ||
                         fmt == "ndjson") format_ = FlatFileFormat::JSONL;
                else if (fmt == "parquet") format_ = FlatFileFormat::PARQUET;
            }

            if (cfg.contains("delimiter")) {
                std::string d = cfg["delimiter"].get<std::string>();
                if (!d.empty()) {
                  delimiter_ = d[0];
                }
            }

            if (cfg.contains("quote_char")) {
                std::string q = cfg["quote_char"].get<std::string>();
                if (!q.empty()) {
                  quote_char_ = q[0];
                }
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
            "Unknown file format; use .csv, .tsv, .jsonl, .ndjson, or .parquet "
            "extension or set format in config");
        return false;
    }

    // For JSONL, verify that the first non-empty line parses as a JSON object.
    if (fmt == FlatFileFormat::JSONL) {
        std::string line = {};
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '\r') {
              continue;
            }
            // Strip trailing \r
            if (!line.empty() && line.back() == '\r') {
              line.pop_back();
            }
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

    // For Parquet, verify magic bytes and (when Arrow is available) open schema.
    if (fmt == FlatFileFormat::PARQUET) {
        // Parquet files start and end with the 4-byte magic "PAR1".
        // Check the header magic bytes without requiring Arrow.
        char magic[4] = {0, 0, 0, 0};
        file.read(magic, 4);
        if (file.gcount() < 4 ||
            magic[0] != 'P' || magic[1] != 'A' ||
            magic[2] != 'R' || magic[3] != '1') {
            errors.push_back(
                "Not a valid Parquet file (missing PAR1 magic bytes): " +
                source_path);
            return false;
        }
#ifdef ARROW_ENABLED
        // Open with Arrow Parquet reader to validate the full file footer.
        auto open_result = arrow::io::ReadableFile::Open(source_path);
        if (!open_result.ok()) {
            errors.push_back("Cannot open Parquet file via Arrow: " +
                             open_result.status().ToString());
            return false;
        }
        std::shared_ptr<arrow::io::ReadableFile> infile = open_result.ValueOrDie();

        auto reader_result = parquet::arrow::OpenFile(
            infile, arrow::default_memory_pool());
        if (!reader_result.ok()) {
            errors.push_back("Invalid Parquet file (Arrow): " +
                             reader_result.status().ToString());
            return false;
        }
        std::unique_ptr<parquet::arrow::FileReader> reader =
            std::move(reader_result).ValueOrDie();
        std::shared_ptr<arrow::Schema> schema;
        auto schema_status = reader->GetSchema(&schema);
        if (!schema_status.ok() || !schema) {
            errors.push_back("Cannot read Parquet schema: " +
                             (schema_status.ok() ? "null schema"
                                                 : schema_status.ToString()));
            return false;
        }
#endif // ARROW_ENABLED
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
        if (it != options.table_mappings.end()) {
          table = it->second;
        }
    }

    bool ok = false;
    if (fmt == FlatFileFormat::JSONL) {
        ok = importJsonlFile(source_path, table, options, stats,
                             progress_callback);
    } else if (fmt == FlatFileFormat::PARQUET) {
        ok = importParquetFile(source_path, table, options, stats,
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
        if (!file) {
          return result;
        }

        std::string header_line = {};
        if (!std::getline(file, header_line)) {
          return result;
        }

        // Strip trailing \r
        if (!header_line.empty() && header_line.back() == '\r')
            header_line.pop_back();

        std::vector<std::string> cols_vec = {};

        if (!has_header_) {
            // Generate synthetic column names: col_0, col_1, ...
            auto fields = parseCsvRow(header_line, delim, quote_char_);
            cols_vec.reserve(fields.size());
            for (size_t i = 0; i < fields.size(); ++i)
                cols_vec.push_back("col_" + std::to_string(i));
            // Re-wind: treat the first "header" line as a data row too
            file.clear();
            file.seekg(0);
        } else {
            cols_vec = parseCsvRow(header_line, delim, quote_char_);
        }

        // Sample up to 100 data rows to auto-detect column types
        SchemaAutoDetector detector;
        static constexpr size_t kSampleLimit = 100;
        size_t sampled = 0;
        std::string data_line = {};
        while (sampled < kSampleLimit && std::getline(file, data_line)) {
            if (!data_line.empty() && data_line.back() == '\r')
                data_line.pop_back();
            if (data_line.empty()) {
              continue;
            }
            auto fields = parseCsvRow(data_line, delim, quote_char_);
            while (fields.size() < cols_vec.size()) {
              fields.emplace_back();
            }
            if (fields.size() > cols_vec.size()) {
              fields.resize(cols_vec.size());
            }
            detector.feedRow(cols_vec, fields);
            ++sampled;
        }

        DetectedSchema schema = detector.getSchema(table);
        // Ensure all columns are represented (e.g. when no data rows exist)
        for (const auto& col : cols_vec) {
            if (schema.column_types.find(col) == schema.column_types.end())
                schema.column_types[col] = DetectedFieldType::STRING;
        }
        schema.columns = cols_vec;

        result.push_back(SchemaAutoDetector::schemaToJson(schema));

    } else if (fmt == FlatFileFormat::JSONL) {
        std::ifstream file(source_path);
        if (!file) {
          return result;
        }

        // Use SchemaAutoDetector to aggregate types across sample rows
        SchemaAutoDetector detector;
        static constexpr size_t kSampleLimit = 100;
        size_t sampled = 0;
        std::vector<std::string> first_cols;

        std::string line = {};
        while (sampled < kSampleLimit && std::getline(file, line)) {
            if (line.empty() || line == "\r") {
              continue;
            }
            if (!line.empty() && line.back() == '\r') {
              line.pop_back();
            }
            if (line.empty()) {
              continue;
            }
            try {
                auto obj = json::parse(line);
                if (!obj.is_object()) {
                  continue;
                }

                std::vector<std::string> cols;
                std::vector<std::string> vals = {};

                cols.reserve(obj.size());
                vals.reserve(obj.size());
                for (auto& [key, val] : obj.items()) {
                    cols.push_back(key);
                    if (val.is_null()) {
                      vals.emplace_back();
                    }
                    else if (val.is_boolean())        vals.push_back(val.get<bool>() ? "true" : "false");
                    else if (val.is_number_integer()) vals.push_back(std::to_string(val.get<int64_t>()));
                    else if (val.is_number_float())   vals.push_back(std::to_string(val.get<double>()));
                    else if (val.is_string())         vals.push_back(val.get<std::string>());
                    else                              vals.push_back(val.dump());
                }

                if (sampled == 0) {
                  first_cols = cols;
                }
                detector.feedRow(cols, vals);
                ++sampled;
            } catch (...) {}
        }

        if (sampled > 0) {
            DetectedSchema schema = detector.getSchema(table);
            // Preserve column order from first row
            if (!first_cols.empty()) {
              schema.columns = first_cols;
            }
            result.push_back(SchemaAutoDetector::schemaToJson(schema));
        }
    } else if (fmt == FlatFileFormat::PARQUET) {
#ifdef ARROW_ENABLED
        auto open_result = arrow::io::ReadableFile::Open(source_path);
        if (!open_result.ok())
            return result;
        std::shared_ptr<arrow::io::ReadableFile> infile = open_result.ValueOrDie();

        auto reader_result = parquet::arrow::OpenFile(
            infile, arrow::default_memory_pool());
        if (!reader_result.ok())
            return result;
        std::unique_ptr<parquet::arrow::FileReader> reader =
            std::move(reader_result).ValueOrDie();

        std::shared_ptr<arrow::Schema> schema = {};

        if (!reader->GetSchema(&schema).ok() || !schema) {
          return result;
        }

        DetectedSchema detected;
        detected.table_name = table;
        detected.columns.reserve(static_cast<size_t>(schema->num_fields()));
        for (int i = 0; i < schema->num_fields(); ++i) {
            const auto& field = schema->field(i);
            detected.columns.push_back(field->name());
            auto type_id = field->type()->id();
            DetectedFieldType ft = DetectedFieldType::STRING;
            if (type_id == arrow::Type::BOOL) {
                ft = DetectedFieldType::BOOLEAN;
            } else if (type_id == arrow::Type::INT8   ||
                       type_id == arrow::Type::INT16  ||
                       type_id == arrow::Type::INT32  ||
                       type_id == arrow::Type::INT64  ||
                       type_id == arrow::Type::UINT8  ||
                       type_id == arrow::Type::UINT16 ||
                       type_id == arrow::Type::UINT32 ||
                       type_id == arrow::Type::UINT64) {
                ft = DetectedFieldType::INTEGER;
            } else if (type_id == arrow::Type::FLOAT ||
                       type_id == arrow::Type::DOUBLE) {
                ft = DetectedFieldType::DOUBLE;
            }
            detected.column_types[field->name()] = ft;
        }
        result.push_back(SchemaAutoDetector::schemaToJson(detected));
#endif // ARROW_ENABLED
    }

    return result;
}

// ============================================================================
// Static helpers
// ============================================================================

FlatFileFormat FlatFileImporter::detectFormat(const std::string& path) {
    // Extract extension (last '.' segment, lower-cased)
    auto dot = path.rfind('.');
    if (dot == std::string::npos) {
      return FlatFileFormat::AUTO;
    }

    std::string ext = path.substr(dot + 1);
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (ext == "csv") {
      return FlatFileFormat::CSV;
    }
    if (ext == "tsv") {
      return FlatFileFormat::TSV;
    }
    if (ext == "jsonl" || ext == "ndjson") {
      return FlatFileFormat::JSONL;
    }
    if (ext == "parquet") {
      return FlatFileFormat::PARQUET;
    }
    return FlatFileFormat::AUTO;
}

FlatFileFormat FlatFileImporter::effectiveFormat(const std::string& path) const {
    if (format_ != FlatFileFormat::AUTO) {
      return format_;
    }
    return detectFormat(path);
}

std::string FlatFileImporter::filenameStem(const std::string& path) {
    // Extract basename
    size_t slash = path.rfind('/');
    if (slash == std::string::npos) {
      slash = path.rfind('\\');
    }
    std::string base = (slash != std::string::npos)
                           ? path.substr(slash + 1)
                           : path;

    // Strip extension
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) {
      base = base.substr(0, dot);
    }

    return base.empty() ? "data" : base;
}

// ============================================================================
// CSV row parser
// ============================================================================

std::vector<std::string> FlatFileImporter::parseCsvRow(const std::string& line,
                                                        char delim,
                                                        char quote) {
    std::vector<std::string> fields;
    std::string field = {};
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
// CSV / TSV schema detection
// ============================================================================

DetectedSchema FlatFileImporter::detectCsvSchema(
    std::ifstream& file,
    std::streampos data_start_pos,
    const std::vector<std::string>& columns,
    char delim,
    size_t line_limit,
    size_t sample_limit,
    const std::string& table)
{
    SchemaAutoDetector detector;

    size_t sampled = 0;
    std::string sample_line = {};
    bool sample_truncated = false;

    while (sampled < sample_limit &&
           streamReadLineFlat(file, sample_line, line_limit, sample_truncated)) {
        if (sample_truncated || sample_line.empty()) {
          continue;
        }
        if (!sample_line.empty() && sample_line.back() == '\r')
            sample_line.pop_back();
        if (sample_line.empty()) {
          continue;
        }

        auto fields = parseCsvRow(sample_line, delim, quote_char_);
        while (fields.size() < columns.size()) {
          fields.emplace_back();
        }
        if (fields.size() > columns.size()) {
          fields.resize(columns.size());
        }

        detector.feedRow(columns, fields);
        ++sampled;
    }

    // Seek back so the main import loop starts from the first data row
    file.clear();
    file.seekg(data_start_pos);

    return detector.getSchema(table);
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
    const size_t line_limit = row_limit > 0 ? row_limit : 64 * 1024 * 1024;

    std::vector<std::string> columns;
    size_t line_number = 0;

    // ---- Read header row (if present) ----
    if (has_header_) {
        std::string header_line = {};
        bool truncated = false;
        if (!streamReadLineFlat(file, header_line, line_limit, truncated)) {
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
            if (it != options.column_mappings.end()) {
              col = it->second;
            }
        }
    }

    stats.tables_processed++;
    reportProgress(cb, "importing table " + table, 0, 0);

    emitSpan(options, "parse_table",
             {{"table", table}}, 0.0);

    // ---- Schema auto-detection (optional pre-pass) ----
    // When validate_schema is enabled and we already know the column names,
    // sample the first N data rows to detect types, then seek back.
    // schema_sample_rows == 0 disables detection (nothing to sample).
    bool schema_validation_active = options.validate_schema &&
                                    !columns.empty() &&
                                    options.schema_sample_rows > 0;
    DetectedSchema detected_schema = {};
    if (schema_validation_active) {
        auto data_start = file.tellg();
        detected_schema = detectCsvSchema(
            file, data_start, columns, delim,
            line_limit,
            options.schema_sample_rows,
            table);
        THEMIS_INFO("Schema auto-detected for '{}': {} columns", table,
                    detected_schema.columns.size());
    }

    // ---- Read data rows ----
    std::string line = {};
    bool line_truncated = false;
    size_t row_index    = 0;

    while (streamReadLineFlat(file, line, line_limit, line_truncated) &&
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
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }

        // Skip blank lines
        if (line.empty()) {
          continue;
        }

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
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        auto fields = parseCsvRow(line, delim, quote_char_);

        // Column count mismatch – pad with empty or truncate
        while (fields.size() < columns.size()) {
          fields.emplace_back();
        }
        if (fields.size() > columns.size()) {
          fields.resize(columns.size());
        }

        // Schema validation (type-mismatch warnings)
        if (schema_validation_active) {
            auto val_errors = SchemaAutoDetector::validateRow(
                columns, fields, detected_schema);
            for (const auto& ve : val_errors) {
                addError(stats, ImportErrorCode::SCHEMA_VALIDATION_FAILED,
                         ImportErrorSeverity::WARNING,
                         ve.message,
                         "line " + std::to_string(line_number));
                stats.warnings.push_back(ve.message);
                emitMetric(options, "themisdb_import_errors_total",
                           {{"code", std::to_string(static_cast<uint32_t>(
                                 ImportErrorCode::SCHEMA_VALIDATION_FAILED))}},
                           1.0);
            }
        }

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
        if ([[maybe_unused]] options.streaming_row_callback) {
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
    const size_t line_limit = row_limit > 0 ? row_limit : 64 * 1024 * 1024;

    stats.tables_processed++;
    reportProgress(cb, "importing table " + table, 0, 0);
    emitSpan(options, "parse_table", {{"table", table}}, 0.0);

    // ---- Schema auto-detection pre-pass (optional) ----
    // Sample up to schema_sample_rows to detect types; then seek back to start.
    // schema_sample_rows == 0 disables detection (nothing to sample).
    bool schema_validation_active = options.validate_schema &&
                                    options.schema_sample_rows > 0;
    DetectedSchema jsonl_schema = {};
    if (schema_validation_active) {
        SchemaAutoDetector detector;
        auto data_start = file.tellg();
        size_t sampled  = 0;
        std::vector<std::string> first_cols;

        std::string sline = {};
        bool strunc = false;
        while (sampled < options.schema_sample_rows &&
               streamReadLineFlat(file, sline, line_limit, strunc)) {
            if (strunc || sline.empty()) {
              continue;
            }
            if (!sline.empty() && sline.back() == '\r') {
              sline.pop_back();
            }
            if (sline.empty()) {
              continue;
            }
            try {
                auto obj = json::parse(sline);
                if (!obj.is_object()) {
                  continue;
                }
                std::vector<std::string> cols, vals;
                cols.reserve(obj.size());
                vals.reserve(obj.size());
                for (auto& [key, val] : obj.items()) {
                    cols.push_back(key);
                    if (val.is_null()) {
                      vals.emplace_back();
                    }
                    else if (val.is_boolean())        vals.push_back(val.get<bool>() ? "true" : "false");
                    else if (val.is_number_integer()) vals.push_back(std::to_string(val.get<int64_t>()));
                    else if (val.is_number_float())   vals.push_back(std::to_string(val.get<double>()));
                    else if (val.is_string())         vals.push_back(val.get<std::string>());
                    else                              vals.push_back(val.dump());
                }
                // Apply column_mappings so the detected schema matches the
                // names that will be used during the actual import pass.
                if (!options.column_mappings.empty()) {
                    for (auto& col : cols) {
                        auto it = options.column_mappings.find(col);
                        if (it != options.column_mappings.end()) {
                          col = it->second;
                        }
                    }
                }
                if (sampled == 0) {
                  first_cols = cols;
                }
                detector.feedRow(cols, vals);
                ++sampled;
            } catch (...) {}
        }
        file.clear();
        file.seekg(data_start);
        jsonl_schema = detector.getSchema(table);
        if (!first_cols.empty()) {
          jsonl_schema.columns = first_cols;
        }
        THEMIS_INFO("Schema auto-detected for JSONL '{}': {} columns",
                    table, jsonl_schema.columns.size());
    }

    std::string line = {};
    bool line_truncated = false;
    size_t line_number  = 0;
    size_t row_index    = 0;

    while (streamReadLineFlat(file, line, line_limit, line_truncated) &&
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
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }

        // Skip blank lines
        if (line.empty()) {
          continue;
        }

        stats.total_records++;
        ++row_index;

        // UTF-8 validation
        if (options.enforce_utf8 && !isValidUtf8(line)) {
            addError(stats, ImportErrorCode::INVALID_UTF8,
                     ImportErrorSeverity::ERROR,
                     "Invalid UTF-8 in row",
                     "line " + std::to_string(line_number));
            stats.failed_records++;
            if (!options.continue_on_error) {
              return false;
            }
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
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        if (!entity.is_object()) {
            addError(stats, ImportErrorCode::PARSE_INSERT,
                     ImportErrorSeverity::ERROR,
                     "JSONL line is not a JSON object",
                     "line " + std::to_string(line_number));
            stats.failed_records++;
            if (!options.continue_on_error) {
              return false;
            }
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

        // Schema validation (type-mismatch warnings for JSONL)
        if (schema_validation_active && !jsonl_schema.columns.empty()) {
            std::vector<std::string> cols, vals;
            cols.reserve(entity.size());
            vals.reserve(entity.size());
            for (auto& [key, val] : entity.items()) {
                cols.push_back(key);
                if (val.is_null()) {
                  vals.emplace_back();
                }
                else if (val.is_boolean())        vals.push_back(val.get<bool>() ? "true" : "false");
                else if (val.is_number_integer()) vals.push_back(std::to_string(val.get<int64_t>()));
                else if (val.is_number_float())   vals.push_back(std::to_string(val.get<double>()));
                else if (val.is_string())         vals.push_back(val.get<std::string>());
                else                              vals.push_back(val.dump());
            }
            auto val_errors = SchemaAutoDetector::validateRow(cols, vals, jsonl_schema);
            for (const auto& ve : val_errors) {
                addError(stats, ImportErrorCode::SCHEMA_VALIDATION_FAILED,
                         ImportErrorSeverity::WARNING,
                         ve.message,
                         "line " + std::to_string(line_number));
                stats.warnings.push_back(ve.message);
                emitMetric(options, "themisdb_import_errors_total",
                           {{"code", std::to_string(static_cast<uint32_t>(
                                 ImportErrorCode::SCHEMA_VALIDATION_FAILED))}},
                           1.0);
            }
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
        if ([[maybe_unused]] options.streaming_row_callback) {
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
// Parquet import
// ============================================================================

bool FlatFileImporter::importParquetFile(const std::string& path,
                                          const std::string& table,
                                          const ImportOptions& options,
                                          ImportStats& stats,
                                          ProgressCallback& cb) {
#ifdef ARROW_ENABLED
    // ---- Check table filter ----
    if (!shouldImportTable(table, options)) {
        addError(stats, ImportErrorCode::TABLE_EXCLUDED,
                 ImportErrorSeverity::INFO,
                 "Table excluded by filter: " + table);
        stats.skipped_records++;
        return true;
    }

    // ---- Open file ----
    auto open_result = arrow::io::ReadableFile::Open(path);
    if (!open_result.ok()) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Cannot open Parquet file: " + open_result.status().ToString());
        return false;
    }
    std::shared_ptr<arrow::io::ReadableFile> infile = open_result.ValueOrDie();

    // ---- Open Parquet reader ----
    auto reader_result = parquet::arrow::OpenFile(
        infile, arrow::default_memory_pool());
    if (!reader_result.ok()) {
        addError(stats, ImportErrorCode::FILE_READ_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Failed to open Parquet reader: " + reader_result.status().ToString());
        return false;
    }
    std::unique_ptr<parquet::arrow::FileReader> reader =
        std::move(reader_result).ValueOrDie();

    // ---- Read full table ----
    std::shared_ptr<arrow::Table> arrow_table;
    auto read_status = reader->ReadTable(&arrow_table);
    if (!read_status.ok() || !arrow_table) {
        addError(stats, ImportErrorCode::FILE_READ_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Failed to read Parquet table: " +
                     (read_status.ok() ? "null table" : read_status.ToString()));
        return false;
    }

    // ---- Build column list (with optional remapping) ----
    auto schema = arrow_table->schema();
    std::vector<std::string> columns;
    columns.reserve(static_cast<size_t>(schema->num_fields()));
    for (int i = 0; i < schema->num_fields(); ++i) {
        std::string col = schema->field(i)->name();
        auto it = options.column_mappings.find(col);
        if (it != options.column_mappings.end()) {
          col = it->second;
        }
        columns.push_back(std::move(col));
    }

    // ---- Build schema from Arrow types (for validation) ----
    DetectedSchema detected_schema;
    detected_schema.table_name = table;
    detected_schema.columns    = columns;
    for (int i = 0; i < schema->num_fields(); ++i) {
        auto type_id = schema->field(i)->type()->id();
        DetectedFieldType ft = DetectedFieldType::STRING;
        if (type_id == arrow::Type::BOOL) {
            ft = DetectedFieldType::BOOLEAN;
        } else if (type_id == arrow::Type::INT8   ||
                   type_id == arrow::Type::INT16  ||
                   type_id == arrow::Type::INT32  ||
                   type_id == arrow::Type::INT64  ||
                   type_id == arrow::Type::UINT8  ||
                   type_id == arrow::Type::UINT16 ||
                   type_id == arrow::Type::UINT32 ||
                   type_id == arrow::Type::UINT64) {
            ft = DetectedFieldType::INTEGER;
        } else if (type_id == arrow::Type::FLOAT ||
                   type_id == arrow::Type::DOUBLE) {
            ft = DetectedFieldType::DOUBLE;
        }
        // Defensive bounds check to prevent potential out-of-bounds access
        if (static_cast<size_t>(i) < columns.size()) {
            detected_schema.column_types[columns[static_cast<size_t>(i)]] = ft;
        } else {
            THEMIS_WARN("Column index {} exceeds columns.size() = {}", i, columns.size());
        }
    }
    bool schema_validation_active =
        options.validate_schema && options.schema_sample_rows > 0;

    stats.tables_processed++;
    int64_t total_rows = arrow_table->num_rows();
    reportProgress(cb, "importing table " + table, 0,
                   static_cast<size_t>(total_rows));
    emitSpan(options, "parse_table", {{"table", table}}, 0.0);

    THEMIS_INFO("Parquet schema auto-detected for '{}': {} columns, {} rows",
                table, columns.size(), total_rows);

    // ---- Iterate batches ----
    arrow::TableBatchReader batch_reader(*arrow_table);
    std::shared_ptr<arrow::RecordBatch> batch;
    size_t row_index = 0;

    while (batch_reader.ReadNext(&batch).ok() && batch && !cancelled_) {
        int64_t batch_rows = batch->num_rows();
        int     num_cols   = batch->num_columns();

        for (int64_t r = 0; r < batch_rows && !cancelled_; ++r) {
            ++row_index;
            stats.total_records++;

            json entity = json::object();
            std::vector<std::string> row_values;
            row_values.reserve(static_cast<size_t>(num_cols));

            for (int c = 0; c < num_cols; ++c) {
                const auto& col_arr  = batch->column(c);
                // Defensive bounds check for column name lookup
                if (static_cast<size_t>(c) >= columns.size()) {
                    THEMIS_WARN("Column index {} exceeds columns.size() = {}", c, columns.size());
                    continue;
                }
                const std::string& col_name =
                    columns[static_cast<size_t>(c)];

                if (col_arr->IsNull(r)) {
                    entity[col_name] = nullptr;
                    row_values.emplace_back();
                    continue;
                }

                auto type_id = col_arr->type_id();
                std::string sv = {};

                if (type_id == arrow::Type::BOOL) {
                    auto arr = std::static_pointer_cast<arrow::BooleanArray>(
                        col_arr);
                    bool bval = arr->Value(r);
                    sv        = bval ? "true" : "false";
                    entity[col_name] = bval;
                } else if (type_id == arrow::Type::INT8) {
                    auto arr = std::static_pointer_cast<arrow::Int8Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::INT16) {
                    auto arr = std::static_pointer_cast<arrow::Int16Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::INT32) {
                    auto arr = std::static_pointer_cast<arrow::Int32Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::INT64) {
                    auto arr = std::static_pointer_cast<arrow::Int64Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::UINT8) {
                    auto arr = std::static_pointer_cast<arrow::UInt8Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::UINT16) {
                    auto arr = std::static_pointer_cast<arrow::UInt16Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::UINT32) {
                    auto arr = std::static_pointer_cast<arrow::UInt32Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::UINT64) {
                    auto arr = std::static_pointer_cast<arrow::UInt64Array>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::FLOAT) {
                    auto arr = std::static_pointer_cast<arrow::FloatArray>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else if (type_id == arrow::Type::DOUBLE) {
                    auto arr = std::static_pointer_cast<arrow::DoubleArray>(col_arr);
                    sv = std::to_string(arr->Value(r));
                    entity[col_name] = sv;
                } else {
                    // STRING, LARGE_STRING, BINARY, DATE, TIMESTAMP, etc.
                    // Use GetScalar().ToString() as the universal fallback.
                    auto scalar_result = col_arr->GetScalar(r);
                    if (scalar_result.ok()) {
                        sv = (*scalar_result)->ToString();
                    }
                    entity[col_name] = sv;
                }
                row_values.push_back(sv);
            }

            // ---- Schema validation (type-mismatch warnings) ----
            if (schema_validation_active) {
                auto val_errors = SchemaAutoDetector::validateRow(
                    columns, row_values, detected_schema);
                for (const auto& ve : val_errors) {
                    addError(stats, ImportErrorCode::SCHEMA_VALIDATION_FAILED,
                             ImportErrorSeverity::WARNING, ve.message,
                             "row " + std::to_string(row_index));
                    stats.warnings.push_back(ve.message);
                    emitMetric(
                        options, "themisdb_import_errors_total",
                        {{"code", std::to_string(static_cast<uint32_t>(
                              ImportErrorCode::SCHEMA_VALIDATION_FAILED))}},
                        1.0);
                }
            }

            // ---- Dry-run ----
            if (options.dry_run) {
                addError(stats, ImportErrorCode::DRY_RUN_ONLY,
                         ImportErrorSeverity::INFO,
                         "dry-run: row not written",
                         "row " + std::to_string(row_index));
                stats.imported_records++;
                continue;
            }

            // ---- Streaming callback ----
            if ([[maybe_unused]] options.streaming_row_callback) {
                if (!options.streaming_row_callback(table, entity)) {
                    THEMIS_INFO(
                        "Streaming callback aborted Parquet import at row {}",
                        row_index);
                    return true;
                }
            }

            stats.imported_records++;

            if (row_index % options.batch_size == 0) {
                reportProgress(cb, "importing table " + table,
                               stats.imported_records,
                               static_cast<size_t>(total_rows));
                emitMetric(options, "themisdb_import_rows_total",
                           {{"table", table}, {"status", "imported"}},
                           static_cast<double>(options.batch_size));
            }
        }
    }

    return true;
#else
    (void)path;
    (void)table;
    (void)options;
    (void)cb;
    addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
             ImportErrorSeverity::CRITICAL,
             "Parquet import requires Apache Arrow "
             "(rebuild ThemisDB with -DARROW_ENABLED=1 / vcpkg arrow feature)");
    return false;
#endif // ARROW_ENABLED
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
        if (!found) {
          return false;
        }
    }

    // Check exclude list
    for (const auto& t : options.exclude_tables) {
        if (t == table_name) {
          return false;
        }
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
    if ([[maybe_unused]] options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void FlatFileImporter::emitSpan(const ImportOptions& options,
                                 const std::string& operation,
                                 const std::map<std::string, std::string>& attributes,
                                 double duration_seconds) const {
    if ([[maybe_unused]] options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void FlatFileImporter::reportProgress(ProgressCallback& callback,
                                       const std::string& stage,
                                       size_t current, size_t total) {
    if ([[maybe_unused]] callback) {
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
            if (i >= n || (bytes[i] & 0xC0) != 0x80) {
              return false;
            }
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
    if (!importer_) {
      return false;
    }
    return importer_->initialize(config_json ? config_json : "{}");
}

void FlatFileImporterPlugin::shutdown() {
    if (importer_) {
      importer_->cancel();
    }
}

} // namespace importers
} // namespace themis

// ============================================================================
// Plugin Entry Points
// ============================================================================

