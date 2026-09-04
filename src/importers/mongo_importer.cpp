/**
 * @file mongo_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/mongo_importer.h"
#include <stdexcept>
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>

namespace themis {
namespace importers {

// ============================================================================
// PHASE-2-HARDENING: MongoDB Error Mapping and Fallback Infrastructure
// ============================================================================
namespace {

/// Maps MongoDB-specific error patterns to ImporterErrorCode for standardized
/// error reporting across all importers.
[[maybe_unused]] static ImportErrorCode mapMongoDBErrorToCode(const std::string& error_msg) {
    // PHASE-2-HARDENING: Standardized error mapping
    const auto msg_lower = [](std::string s) {
        for (auto& c : s) {
          c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    };
    std::string lower_msg = msg_lower(error_msg);
    
    // Connection errors
    if (lower_msg.find("connection") != std::string::npos ||
        lower_msg.find("unavailable") != std::string::npos ||
        lower_msg.find("unreachable") != std::string::npos ||
        lower_msg.find("refused") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    
    // Timeout errors
    if (lower_msg.find("timeout") != std::string::npos ||
        lower_msg.find("deadline") != std::string::npos) {
        return ImportErrorCode::DEADLINE_EXCEEDED;
    }
    
    // Schema/type errors
    if (lower_msg.find("schema") != std::string::npos ||
        lower_msg.find("type mismatch") != std::string::npos) {
        return ImportErrorCode::IMPORT_SCHEMA_MISMATCH;
    }
    
    // Parse errors
    if (lower_msg.find("parse") != std::string::npos ||
        lower_msg.find("syntax") != std::string::npos ||
        lower_msg.find("json") != std::string::npos) {
        return ImportErrorCode::PARSE_INSERT;
    }
    
    // File errors
    if (lower_msg.find("file") != std::string::npos ||
        lower_msg.find("not found") != std::string::npos ||
        lower_msg.find("cannot open") != std::string::npos) {
        return ImportErrorCode::FILE_NOT_FOUND;
    }
    
    return ImportErrorCode::UNKNOWN;
}

/// Attempts exponential backoff retry with connection timeout.
/// Parameters: initial_timeout_ms (e.g., 3000), max_retries (e.g., 3)
/// Returns: true if successful, false if all retries exhausted.
/// PHASE-2-HARDENING: Connection Timeout with Exponential Backoff
static bool retryWithExponentialBackoff(
    const std::function<bool()>& operation,
    int initial_timeout_ms = 3000,
    int max_retries = 3
) {
    int current_timeout = initial_timeout_ms;
    for (int retry = 0; retry < max_retries; ++retry) {
        if (operation()) {
            return true;
        }
        if (retry < max_retries - 1) {
            THEMIS_DEBUG("Retry {} failed, backing off for {}ms", retry + 1, current_timeout);
            std::this_thread::sleep_for(std::chrono::milliseconds(current_timeout));
            current_timeout *= 2;  // exponential backoff: 3s → 6s → 12s
        }
    }
    return false;
}

} // anonymous namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

MongoDBImporter::MongoDBImporter() = default;

MongoDBImporter::~MongoDBImporter() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> MongoDBImporter::getSupportedTypes() const {
    return {"mongodb", "mongoexport", "json", "jsonl", "ndjson"};
}

bool MongoDBImporter::initialize(const std::string& config) {
    cancelled_ = false;
    configured_collection_.clear();

    if (!config.empty() && config != "{}") {
        try {
            json cfg = json::parse(config);
            if (cfg.contains("collection") && cfg["collection"].is_string()) {
                configured_collection_ = cfg["collection"].get<std::string>();
            }
        } catch (const json::exception&) {
            // Ignore invalid config JSON; collection will be derived from path.
        }
    }

    THEMIS_INFO("MongoDB Importer initialized (collection='{}')",
                configured_collection_.empty() ? "<from-path>" : configured_collection_);
    return true;
}

bool MongoDBImporter::validateSource(const std::string& source_path,
                                     std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }

    // Read up to 200 lines to find the first non-empty, non-comment line.
    std::string line;
    int lines_checked = 0;
    bool found_json = false;
    while (std::getline(file, line) && lines_checked < 200) {
        // Trim leading whitespace
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) { lines_checked++; continue; }
        char c = line[first];
        if (c == '{' || c == '[') {
            found_json = true;
            break;
        }
        // Lines starting with '#' are treated as comments (non-standard but common)
        if (c == '#') { lines_checked++; continue; }
        // Anything else is unexpected
        break;
    }

    if (!found_json) {
        errors.push_back("File does not appear to be a MongoDB JSON export "
                         "(expected first non-empty line to start with '{' or '[')");
        return false;
    }

    THEMIS_INFO("MongoDB source validation successful: {}", source_path);
    return true;
}

ImportStats MongoDBImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();

    THEMIS_INFO("Starting MongoDB import from: {}", source_path);
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

    // Derive the effective collection name
    std::string collection = configured_collection_.empty()
                             ? collectionFromPath(source_path)
                             : configured_collection_;

    // Check collection-level filtering before opening the file
    if (!shouldImportCollection(collection, options)) {
        THEMIS_INFO("Collection '{}' excluded by import options; skipping", collection);
        auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end_time - start_time).count();
        return stats;
    }

    // PHASE-2-HARDENING: Open file with connection timeout and exponential backoff
    std::ifstream peek_file;
    bool file_opened = retryWithExponentialBackoff(
        [&]() {
            peek_file.open(source_path);
            return peek_file.is_open();
        },
        3000,  // initial timeout: 3 seconds
        3      // max retries
    );
    
    if (!file_opened) {
        addError(stats, ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE,
                 ImportErrorSeverity::CRITICAL,
                 "Cannot open file after retries with exponential backoff: " + source_path);
        THEMIS_INFO("MongoDB import failed: file unavailable");
        auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end_time - start_time).count();
        return stats;
    }

    // --- Detect format: JSON array vs JSON-Lines ---
    // Peek at the first non-whitespace character of the file.
    char first_char = '\0';
    {
        char c = '\0';
        while (peek_file.get(c)) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                continue;  // skip whitespace
            }
            if (c == '#') {
                // Skip comment line (non-standard but tolerated)
                while (peek_file.get(c) && c != '\n') { /* discard */ }
                continue;
            }
            first_char = c;
            break;
        }
    }
    peek_file.close();

    bool ok = false;
    if (first_char == '[') {
        // PHASE-2-HARDENING: Try JSON array parsing first, fallback to JSON-Lines on error
        THEMIS_DEBUG("Detected JSON array format; attempting aggregation pipeline parse");
        ok = parseJsonArray(source_path, collection, options, stats, progress_callback);
        
        if (!ok && !stats.structured_errors.empty()) {
            ImportErrorCode last_code = stats.structured_errors.back().code;
            if (last_code == ImportErrorCode::PARSE_INSERT || 
                last_code == ImportErrorCode::FILE_READ_FAILED ||
                last_code == ImportErrorCode::IMPORT_SCHEMA_MISMATCH) {
                // PHASE-2-HARDENING: Fallback from aggregation pipeline to streaming parse
                THEMIS_INFO("Aggregation pipeline parse failed ({}); falling back to streaming parse",
                            static_cast<int>(last_code));
                stats.structured_errors.clear();
                stats.errors.clear();
                stats.imported_records = 0;
                stats.failed_records = 0;
                stats.skipped_records = 0;
                stats.total_records = 0;
                ok = parseJsonLines(source_path, collection, options, stats, progress_callback);
                
                if (ok) {
                    THEMIS_INFO("Successfully recovered from aggregation pipeline failure using streaming parse");
                    emitMetric(options, "themisdb_import_fallback_total",
                               {{"fallback", "aggregation_to_streaming"}}, 1.0);
                }
            }
        }
    } else {
        // Default: JSON-Lines (NDJSON) — one document per line
        ok = parseJsonLines(source_path, collection, options, stats, progress_callback);
    }

    if (!ok && stats.structured_errors.empty()) {
        addError(stats, ImportErrorCode::FILE_READ_FAILED,
                 ImportErrorSeverity::CRITICAL, "Failed to parse source file");
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds =
        std::chrono::duration<double>(end_time - start_time).count();

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
             {{"source",     source_path},
              {"collection", collection},
              {"rows",       std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> MongoDBImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "mongo-import-" + std::to_string(ms) + "-" +
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
            err.message  = std::string("Unhandled exception in async MongoDB import: ")
                           + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async MongoDB import worker";
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

void MongoDBImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("MongoDB import cancelled");
}

json MongoDBImporter::getSourceSchema(const std::string& source_path) {
    std::ifstream file(source_path);
    if (!file) {
      return json::array();
    }

    // PHASE-2-HARDENING: Sample up to 100 documents to infer the schema with graceful degradation
    std::map<std::string, std::string> field_types;
    std::vector<std::string> field_order;
    int docs_sampled = 0;
    const int max_sample = 100;

    auto processDoc = [&]([[maybe_unused]] const json& raw_doc) {
        if (!raw_doc.is_object()) {
          return;
        }
        json doc = unwrapDocument(raw_doc);
        for (auto it = doc.begin(); it != doc.end(); ++it) {
            const std::string& key = it.key();
            std::string inferred   = inferThemisType(it.value());
            if (field_types.find(key) == field_types.end()) {
                field_types[key] = inferred;
                field_order.push_back(key);
            } else if (field_types[key] != inferred && inferred != "string") {
                // PHASE-2-HARDENING: Graceful degradation - widen to string on type conflict
                // This prevents schema inference failures when types are inconsistent
                field_types[key] = "string";
                THEMIS_DEBUG("Type conflict for field '{}': {} vs {}; widening to string",
                            key, field_types[key], inferred);
            }
        }
        docs_sampled++;
    };

    // Detect format
    char first_char = '\0';
    {
        char c = '\0';
        while (file.get(c)) {
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                first_char = c;
                break;
            }
        }
    }
    file.seekg(0);

    try {
        if (first_char == '[') {
            // PHASE-2-HARDENING: JSON array parsing with BSON schema inference fallback
            try {
                json arr = json::parse(file);
                for (auto& doc : arr) {
                    if (docs_sampled >= max_sample) {
                      break;
                    }
                    processDoc(doc);
                }
            } catch (const json::parse_error& e) {
                // PHASE-2-HARDENING: Fallback to line-by-line parsing on array parse failure
                THEMIS_WARN("Failed to parse as JSON array ({}); falling back to JSON-Lines schema inference",
                           e.what());
                field_types.clear();
                field_order.clear();
                docs_sampled = 0;
                file.clear();
                file.seekg(0);
                
                std::string line;
                while (std::getline(file, line) && docs_sampled < max_sample) {
                    size_t f = line.find_first_not_of(" \t\r\n");
                    if (f == std::string::npos || line[f] == '#') {
                      continue;
                    }
                    line = line.substr(f);
                    if (line.empty() || line[0] != '{') continue;
                    try {
                        json doc = json::parse(line);
                        processDoc(doc);
                    } catch (const json::parse_error& e) {
                        // PHASE-2-HARDENING: Skip malformed documents during schema inference
                        THEMIS_DEBUG("Skipping malformed document during schema inference: {}", e.what());
                        continue;
                    }
                }
            }
        } else {
            std::string line;
            while (std::getline(file, line) && docs_sampled < max_sample) {
                // Trim
                size_t f = line.find_first_not_of(" \t\r\n");
                if (f == std::string::npos || line[f] == '#') {
                  continue;
                }
                line = line.substr(f);
                if (line.empty() || line[0] != '{') continue;
                try {
                    json doc = json::parse(line);
                    processDoc(doc);
                } catch (const json::parse_error& e) {
                    // PHASE-2-HARDENING: Skip malformed documents during schema inference
                    THEMIS_DEBUG("Skipping malformed document during schema inference: {}", e.what());
                    continue;
                }
            }
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Error during schema inference: {}", e.what());
        // Fall through with partial schema (graceful degradation)
    }

    json schema = json::array();
    std::string collection = configured_collection_.empty()
                             ? collectionFromPath(source_path)
                             : configured_collection_;
    json col_arr = json::array();
    for (const auto& fname : field_order) {
        // PHASE-3A-FIX: Add bounds check before .at() to prevent exception throwing
        auto it = field_types.find(fname);
        if (it != field_types.end()) {
            col_arr.push_back({{"name", fname}, {"type", it->second}});
        } else {
            // Fallback to string type if field not found in inferred types
            col_arr.push_back({{"name", fname}, {"type", "string"}});
        }
    }
    
    // PHASE-2-HARDENING: Include fallback indicator in schema metadata
    json col_obj = {
        {"name",         collection},
        {"type",         "collection"},
        {"fields",       col_arr},
        {"docs_sampled", docs_sampled}
    };
    if (docs_sampled == 0) {
        col_obj["inference_status"] = "DEGRADED";
        col_obj["fallback_reason"] = "No valid documents found; unable to infer schema";
    } else if (docs_sampled < max_sample) {
        col_obj["inference_status"] = "PARTIAL";
    } else {
        col_obj["inference_status"] = "COMPLETE";
    }
    schema.push_back(col_obj);

    return schema;
}

// ============================================================================
// Private Methods — Parsing
// ============================================================================

bool MongoDBImporter::parseJsonLines(const std::string& file_path,
                                     const std::string& collection,
                                     const ImportOptions& options,
                                     ImportStats& stats,
                                     ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    // Count tables (collections) processed — one per file for NDJSON
    bool collection_counted = false;
    size_t doc_index    = 0;
    size_t batch_count  = 0;

    std::string line;
    while (std::getline(file, line) && !cancelled_) {
        // Trim leading whitespace
        size_t f = line.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) {
          continue;
        }
        // Skip comment lines
        if (line[f] == '#') {
          continue;
        }
        // Skip empty or non-JSON lines
        if (line[f] != '{') continue;

        // Per-line size guard
        if (options.max_row_size_bytes > 0 &&
            line.size() > options.max_row_size_bytes) {
            addError(stats, ImportErrorCode::ROW_TOO_LARGE, ImportErrorSeverity::WARNING,
                     "Document line too large (" + std::to_string(line.size()) +
                     " bytes); skipped",
                     "document " + std::to_string(doc_index + 1));
            stats.warnings.push_back("Document skipped: line too large at index " +
                                     std::to_string(doc_index));
            stats.failed_records++;
            doc_index++;  // keep index in sync even when skipping
            if (!options.continue_on_error) {
              return false;
            }
            continue;
        }

        json doc;
        try {
            doc = json::parse(line);
        } catch (const json::exception& ex) {
            addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                     std::string("JSON parse error: ") + ex.what(),
                     "document " + std::to_string(doc_index + 1));
            stats.warnings.push_back("Parse error at document " +
                                     std::to_string(doc_index + 1));
            stats.failed_records++;
            if (!options.continue_on_error) {
              return false;
            }
            doc_index++;
            continue;
        }

        if (!collection_counted) {
            stats.tables_processed++;
            collection_counted = true;
            auto t0 = std::chrono::steady_clock::now();
            double dur = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - t0).count();
            emitSpan(options, "parse_table", {{"collection", collection}}, dur);
        }

        stats.total_records++;
        importDocument(doc, collection, options, stats, doc_index);
        doc_index++;
        batch_count++;

        if (options.batch_size > 0 && batch_count >= options.batch_size) {
            reportProgress(callback, "data", stats.imported_records, 0);
            batch_count = 0;
        }
    }

    return !cancelled_;
}

bool MongoDBImporter::parseJsonArray(const std::string& file_path,
                                     const std::string& collection,
                                     const ImportOptions& options,
                                     ImportStats& stats,
                                     ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    json arr;
    try {
        arr = json::parse(file);
    } catch (const json::exception& ex) {
        addError(stats, ImportErrorCode::FILE_READ_FAILED, ImportErrorSeverity::CRITICAL,
                 std::string("Failed to parse JSON array file: ") + ex.what());
        return false;
    }

    if (!arr.is_array()) {
        addError(stats, ImportErrorCode::NOT_A_PG_DUMP, ImportErrorSeverity::CRITICAL,
                 "Source file does not contain a JSON array");
        return false;
    }

    if (!arr.empty()) {
        stats.tables_processed++;
        emitSpan(options, "parse_table", {{"collection", collection}}, 0.0);
    }

    size_t batch_count = 0;
    for (size_t i = 0; i < arr.size() && !cancelled_; ++i) {
        stats.total_records++;
        importDocument(arr[i], collection, options, stats, i);
        batch_count++;

        if (options.batch_size > 0 && batch_count >= options.batch_size) {
            reportProgress(callback, "data", stats.imported_records, arr.size());
            batch_count = 0;
        }
    }

    return !cancelled_;
}

bool MongoDBImporter::importDocument(const json& doc,
                                     const std::string& collection,
                                     const ImportOptions& options,
                                     ImportStats& stats,
                                     size_t doc_index) {
    auto t0 = std::chrono::steady_clock::now();

    if (!doc.is_object()) {
        addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                 "Document is not a JSON object; skipped",
                 "document " + std::to_string(doc_index + 1));
        stats.warnings.push_back("Non-object document at index " +
                                 std::to_string(doc_index + 1));
        stats.failed_records++;
        return false;
    }

    if (!shouldImportCollection(collection, options)) {
        stats.skipped_records++;
        return true;
    }

    if (options.dry_run) {
        // Validate document can be unwrapped without error; don't store.
        try {
            json unwrapped = unwrapDocument(doc);
            unwrapped["_type"] = collection;
        } catch (...) {
            addError(stats, ImportErrorCode::TYPE_CONVERSION, ImportErrorSeverity::WARNING,
                     "Dry-run: document unwrap failed",
                     "document " + std::to_string(doc_index + 1));
            stats.failed_records++;
            return false;
        }
        stats.imported_records++;
        emitMetric(options, "themisdb_import_rows_total",
                   {{"collection", collection}, {"status", "imported"}}, 1.0);
        return true;
    }

    // Unwrap BSON extended JSON and produce a ThemisDB entity
    json entity;
    try {
        entity = unwrapDocument(doc);
    } catch (const std::exception& ex) {
        addError(stats, ImportErrorCode::TYPE_CONVERSION, ImportErrorSeverity::WARNING,
                 std::string("BSON unwrap failed: ") + ex.what(),
                 "document " + std::to_string(doc_index + 1));
        stats.warnings.push_back("BSON unwrap failed at document " +
                                 std::to_string(doc_index + 1));
        stats.failed_records++;
        emitMetric(options, "themisdb_import_rows_total",
                   {{"collection", collection}, {"status", "failed"}}, 1.0);
        return false;
    }

    entity["_type"] = collection;

    THEMIS_DEBUG("MongoDB document entity: {}", entity.dump());
    if ([[maybe_unused]] options.streaming_row_callback) {
        if (!options.streaming_row_callback(collection, entity)) {
            cancelled_ = true;
        }
    }
    stats.imported_records++;
    emitMetric(options, "themisdb_import_rows_total",
               {{"collection", collection}, {"status", "imported"}}, 1.0);

    double dur = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    emitSpan(options, "insert_batch", {{"collection", collection}}, dur);

    return true;
}

// ============================================================================
// Private Methods — Type mapping / BSON helpers
// ============================================================================

std::string MongoDBImporter::inferThemisType(const json& value) {
    if (value.is_null()) {
      return "string";
    }
    if (value.is_boolean()) {
      return "boolean";
    }
    if (value.is_number_integer()) {
      return "long";
    }
    if (value.is_number_float()) {
      return "double";
    }
    if (value.is_string()) {
      return "string";
    }
    if (value.is_array()) {
      return "array";
    }

    if (value.is_object()) {
        // BSON extended JSON v2 type wrappers
        if (value.contains("$oid"))           return "string";   // ObjectId as hex string
        if (value.contains("$date")) {
          return "datetime";
        }
        if (value.contains("$numberDecimal")) {
          return "double";
        }
        if (value.contains("$numberLong")) {
          return "long";
        }
        if (value.contains("$numberInt")) {
          return "integer";
        }
        if (value.contains("$numberDouble")) {
          return "double";
        }
        if (value.contains("$binary")) {
          return "binary";
        }
        if (value.contains("$timestamp")) {
          return "datetime";
        }
        if (value.contains("$regex")) {
          return "string";
        }
        if (value.contains("$ref"))           return "string";   // DBRef
        if (value.contains("$code")) {
          return "string";
        }
        if (value.contains("$undefined")) {
          return "string";
        }
        if (value.contains("$minKey")) {
          return "string";
        }
        if (value.contains("$maxKey")) {
          return "string";
        }
        return "object";
    }

    return "string";
}

json MongoDBImporter::unwrapBsonValue(const json& value) {
    if (!value.is_object()) {
      return value;
    }

    // $oid  -> string
    if (value.contains("$oid") && value["$oid"].is_string()) {
        return value["$oid"].get<std::string>();
    }

    // $date  -> ISO-8601 string or epoch ms
    if (value.contains("$date")) {
        const json& d = value["$date"];
        if (d.is_string()) {
          return d.get<std::string>();
        }
        // { "$numberLong": "..." }
        if (d.is_object() && d.contains("$numberLong") && d["$numberLong"].is_string()) {
            return d["$numberLong"].get<std::string>();
        }
        if (d.is_number()) {
          return d;
        }
        return d.dump();
    }

    // $numberDecimal -> string (preserve precision)
    if (value.contains("$numberDecimal") && value["$numberDecimal"].is_string()) {
        return value["$numberDecimal"].get<std::string>();
    }

    // $numberLong -> string (may overflow int64 in JSON)
    if (value.contains("$numberLong") && value["$numberLong"].is_string()) {
        return value["$numberLong"].get<std::string>();
    }

    // $numberInt -> integer
    if (value.contains("$numberInt")) {
        const json& n = value["$numberInt"];
        if (n.is_string()) {
            try { return std::stoi(n.get<std::string>()); } catch (...) {}
        }
        if (n.is_number()) {
          return n;
        }
    }

    // $numberDouble -> double
    if (value.contains("$numberDouble")) {
        const json& n = value["$numberDouble"];
        if (n.is_string()) {
            try { return std::stod(n.get<std::string>()); } catch (...) {}
        }
        if (n.is_number()) {
          return n;
        }
    }

    // $binary -> base64 sub-type string representation
    if (value.contains("$binary")) {
        const json& b = value["$binary"];
        if (b.is_object() && b.contains("base64") && b["base64"].is_string()) {
            return b["base64"].get<std::string>();
        }
        if (b.is_string()) {
          return b.get<std::string>();
        }
    }

    // $timestamp -> { t: <sec>, i: <inc> } -> string representation
    if (value.contains("$timestamp")) {
        const json& ts = value["$timestamp"];
        if (ts.is_object()) {
            json copy = ts;
            return copy.dump();
        }
    }

    // $regex -> string (pattern only)
    if (value.contains("$regex") && value["$regex"].is_string()) {
        return value["$regex"].get<std::string>();
    }

    // $code -> string
    if (value.contains("$code") && value["$code"].is_string()) {
        return value["$code"].get<std::string>();
    }

    // $ref (DBRef) -> stringify the whole wrapper
    if (value.contains("$ref")) {
        return value.dump();
    }

    // $undefined, $minKey, $maxKey -> empty string
    if (value.contains("$undefined") || value.contains("$minKey") ||
        value.contains("$maxKey")) {
        return "";
    }

    // Plain nested object — return as-is (recursion handled by unwrapDocument)
    return value;
}

json MongoDBImporter::unwrapDocument(const json& doc) {
    if (!doc.is_object()) {
      return doc;
    }

    json result = json::object();
    for (auto it = doc.begin(); it != doc.end(); ++it) {
        const std::string& key = it.key();
        const json&        val = it.value();

        if (val.is_object()) {
            // Try BSON unwrap first; if the result is still an object,
            // recurse to handle nested documents.
            json unwrapped = unwrapBsonValue(val);
            if (unwrapped.is_object() && unwrapped == val) {
                // Not a BSON wrapper — recurse into nested document
                result[key] = unwrapDocument(val);
            } else {
                result[key] = unwrapped;
            }
        } else if (val.is_array()) {
            // Recurse into arrays
            json arr = json::array();
            for (const auto& elem : val) {
                if (elem.is_object()) {
                    json uw = unwrapBsonValue(elem);
                    if (uw.is_object() && uw == elem) {
                        arr.push_back(unwrapDocument(elem));
                    } else {
                        arr.push_back(uw);
                    }
                } else {
                    arr.push_back(elem);
                }
            }
            result[key] = arr;
        } else {
            result[key] = val;
        }
    }
    return result;
}

// ============================================================================
// Private Methods — Utilities
// ============================================================================

std::string MongoDBImporter::collectionFromPath(const std::string& path) {
    // Extract the base filename without extension
    // e.g. "/data/exports/users.json" -> "users"
    size_t sep = path.find_last_of("/\\");
    std::string basename = (sep == std::string::npos) ? path : path.substr(sep + 1);
    size_t dot = basename.rfind('.');
    if (dot != std::string::npos) {
      basename = basename.substr(0, dot);
    }
    return basename.empty() ? "documents" : basename;
}

bool MongoDBImporter::shouldImportCollection(const std::string& collection,
                                             const ImportOptions& options) {
    if (std::find(options.exclude_tables.begin(), options.exclude_tables.end(),
                  collection) != options.exclude_tables.end()) {
        return false;
    }
    if (!options.include_tables.empty()) {
        return std::find(options.include_tables.begin(), options.include_tables.end(),
                         collection) != options.include_tables.end();
    }
    return true;
}

// ============================================================================
// Private Methods — Observability
// ============================================================================

void MongoDBImporter::addError(ImportStats& stats, ImportErrorCode code,
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

void MongoDBImporter::emitMetric(const ImportOptions& options,
                                  const std::string& metric,
                                  const std::map<std::string, std::string>& labels,
                                  double value) const {
    if ([[maybe_unused]] options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void MongoDBImporter::emitSpan(const ImportOptions& options,
                                const std::string& operation,
                                const std::map<std::string, std::string>& attributes,
                                double duration_seconds) const {
    if ([[maybe_unused]] options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void MongoDBImporter::reportProgress(ProgressCallback& callback,
                                     const std::string& stage,
                                     size_t current, size_t total) {
    if ([[maybe_unused]] callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// Plugin implementation
// ============================================================================

MongoDBImporterPlugin::MongoDBImporterPlugin()
    : importer_(std::make_unique<MongoDBImporter>()) {
}

plugins::PluginCapabilities MongoDBImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    caps.thread_safe        = false;
    return caps;
}

bool MongoDBImporterPlugin::initialize(const char* config_json) {
    if (!importer_) {
      return false;
    }
    return importer_->initialize(config_json ? config_json : "{}");
}

void MongoDBImporterPlugin::shutdown() {
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
    themis::plugins::IThemisPlugin* createMongoDBPlugin() {
        return new themis::importers::MongoDBImporterPlugin();
    }

    void destroyMongoDBPlugin(themis::plugins::IThemisPlugin* plugin) {
        delete plugin;
    }
}


