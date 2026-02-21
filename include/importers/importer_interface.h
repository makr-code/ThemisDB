/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            importer_interface.h                               ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     574                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <future>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Import error severity levels
 */
enum class ImportErrorSeverity {
    INFO,     ///< Informational (e.g., skipped duplicate)
    WARNING,  ///< Non-fatal issue (e.g., unknown type, using default)
    ERROR,    ///< Row/record-level failure (import continues if continue_on_error)
    CRITICAL  ///< Fatal failure that stops the import
};

/**
 * @brief Structured import error codes
 *
 * Ranges:
 *   0       – success
 *   100-199 – I/O and file errors
 *   200-299 – SQL parsing errors
 *   300-399 – Schema mapping errors
 *   400-499 – Data conversion errors
 *   500-599 – Validation / policy errors
 *   900-999 – Generic / unknown errors
 */
enum class ImportErrorCode : uint32_t {
    // Success
    SUCCESS = 0,

    // I/O errors (100-199)
    FILE_NOT_FOUND       = 100,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,

    // SQL parsing errors (200-299)
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    PARSE_COPY_HEADER    = 202,
    PARSE_COPY_ROW       = 203,
    STATEMENT_TOO_LARGE  = 204,
    ROW_TOO_LARGE        = 205,

    // Schema mapping errors (300-399)
    UNKNOWN_TABLE        = 300,
    COLUMN_COUNT_MISMATCH = 301,

    // Data conversion errors (400-499)
    TYPE_CONVERSION      = 400,
    UNKNOWN_PG_TYPE      = 401,
    VALUE_OUT_OF_RANGE   = 402,

    // Validation / policy errors (500-599)
    DRY_RUN_ONLY         = 500,
    TABLE_EXCLUDED       = 501,
    INVALID_UTF8         = 502,
    PERMISSION_DENIED    = 503,  ///< Caller's permission_check callback returned false

    // SQL parsing errors – extended range (206)
    BINARY_COPY_FORMAT   = 206,  ///< Binary (non-text) COPY data detected; unsupported

    // Generic errors (900-999)
    UNKNOWN              = 900
};

/**
 * @brief Structured import error entry
 */
struct ImportError {
    ImportErrorCode   code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string       message;
    std::string       location;  ///< e.g. "line 42" or "table users, row 7"

    json toJson() const {
        return json{
            {"code",     static_cast<uint32_t>(code)},
            {"severity", static_cast<int>(severity)},
            {"message",  message},
            {"location", location}
        };
    }
};

/**
 * @brief Import Statistics
 */
struct ImportStats {
    size_t total_records = 0;
    size_t imported_records = 0;
    size_t failed_records = 0;
    size_t skipped_records = 0;
    size_t quarantined_records = 0;  ///< Rows written to the quarantine file
    
    size_t tables_processed = 0;
    size_t schemas_processed = 0;
    size_t custom_types_processed = 0;  ///< CREATE TYPE statements parsed (enum / composite)
    
    double elapsed_seconds = 0.0;

    // Dump-mode flags (set from dump header comments)
    bool is_schema_only = false;  ///< true when pg_dump --schema-only header detected
    bool is_data_only   = false;  ///< true when pg_dump --data-only header detected
    
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;  ///< Machine-readable error list
    
    json toJson() const {
        json err_arr = json::array();
        for (const auto& e : structured_errors) {
            err_arr.push_back(e.toJson());
        }
        return json{
            {"total_records", total_records},
            {"imported_records", imported_records},
            {"failed_records", failed_records},
            {"skipped_records", skipped_records},
            {"quarantined_records", quarantined_records},
            {"tables_processed", tables_processed},
            {"schemas_processed", schemas_processed},
            {"custom_types_processed", custom_types_processed},
            {"elapsed_seconds", elapsed_seconds},
            {"is_schema_only", is_schema_only},
            {"is_data_only", is_data_only},
            {"warnings", warnings},
            {"errors", errors},
            {"structured_errors", err_arr}
        };
    }
};

/**
 * @brief Progress Callback
 */
using ProgressCallback = std::function<void(const std::string& stage, size_t current, size_t total)>;

/**
 * @brief Distributed Tracing / OpenTelemetry Span Callback.
 *
 * Called by the importer at the start and end of every major operation
 * (table schema parse, COPY block, INSERT batch) so callers can record
 * OpenTelemetry spans, Jaeger spans, or any other distributed-tracing entry
 * without the importer having a hard dependency on a tracing library.
 *
 * @param operation       Short name for the span, e.g. "parse_table", "copy_block"
 * @param attributes      Key/value span attributes, e.g. {"table": "users", "rows": "150"}
 * @param duration_seconds Wall-clock duration of the operation in seconds (>= 0)
 *
 * Standard operation names emitted:
 *   "import_total"      – wraps the entire importData() call
 *   "parse_table"       – one CREATE TABLE statement parsed; attr: "table"
 *   "copy_block"        – one COPY … FROM stdin block processed; attrs: "table", "rows"
 *   "insert_batch"      – one INSERT INTO statement processed; attr: "table"
 *   "alter_column"      – one ALTER TABLE ADD COLUMN processed; attrs: "table", "column"
 *
 * Example wiring to OpenTelemetry:
 * @code
 *   auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("themisdb");
 *   opts.tracing_callback = [&tracer](const std::string& op,
 *                                      const std::map<std::string,std::string>& attrs,
 *                                      double dur) {
 *       auto span = tracer->StartSpan(op);
 *       for (auto& [k, v] : attrs) span->SetAttribute(k, v);
 *       span->SetAttribute("duration_seconds", dur);
 *       span->End();
 *   };
 * @endcode
 */
using SpanCallback = std::function<void(
    const std::string& operation,
    const std::map<std::string, std::string>& attributes,
    double duration_seconds
)>;

/**
 * @brief Metrics Callback for Prometheus / OpenTelemetry integration.
 *
 * Called by the importer at key points so callers can wire any metrics backend
 * without the importer having a hard dependency on a specific library.
 *
 * Standard metric names emitted:
 *   "themisdb_import_rows_total"     labels: table, status ("imported"|"failed"|"skipped")
 *   "themisdb_import_duration_seconds" labels: table
 *   "themisdb_import_errors_total"   labels: table, code (ImportErrorCode as uint32 string)
 *   "themisdb_import_tables_total"   labels: (none)
 *
 * Example wiring to PrometheusMetrics:
 * @code
 *   auto& prom = PrometheusMetrics::instance();
 *   opts.metrics_callback = [&](const std::string& metric,
 *                               const std::map<std::string,std::string>& labels,
 *                               double value) {
 *       prom.addToCounter(metric, static_cast<int64_t>(value), labels);
 *   };
 * @endcode
 */
using MetricsCallback = std::function<void(
    const std::string& metric,
    const std::map<std::string, std::string>& labels,
    double value
)>;

/**
 * @brief Permission Check Callback for ACL / policy enforcement.
 *
 * Called by the importer at the start of every `importData()` /
 * `importDataAsync()` call so the caller can enforce its own access-control
 * policy without the importer having a hard dependency on any specific
 * security framework.
 *
 * Return `true` to allow the import; `false` to deny it.  On denial the
 * importer records a structured `PERMISSION_DENIED` (code 503) error and
 * returns an empty `ImportStats` immediately.
 *
 * @param resource  The resource being accessed, e.g. "import"
 * @param action    The action being performed, e.g. "write"
 *
 * Example wiring to RBAC:
 * @code
 *   opts.permission_check = [&rbac, user_id](const std::string& resource,
 *                                             const std::string& action) {
 *       return rbac.hasPermission(user_id, resource, action);
 *   };
 * @endcode
 */
using PermissionCheckCallback = std::function<bool(
    const std::string& resource,
    const std::string& action
)>;


struct ImportOptions {
    // General
    bool dry_run = false;                    // Don't actually import, just validate
    bool continue_on_error = true;           // Continue importing on row errors
    size_t batch_size = 1000;                // Records per batch
    
    // Schema mapping
    bool auto_create_schema = true;          // Auto-create missing entity types
    std::string default_namespace = "imported"; // Namespace for imported entities
    
    // Data handling
    bool preserve_ids = false;               // Try to preserve original IDs
    bool update_existing = false;            // Update if entity exists
    bool skip_duplicates = true;             // Skip duplicate records
    
    // Filtering
    std::vector<std::string> include_tables; // Only import these tables (empty = all)
    std::vector<std::string> exclude_tables; // Exclude these tables
    std::vector<std::string> include_schemas; // Only import these schemas
    
    // Transformations
    std::map<std::string, std::string> column_mappings; // Old column -> new attribute
    std::map<std::string, std::string> table_mappings;  // Old table -> new entity type
    std::map<std::string, std::string> type_overrides;  // PG type -> ThemisDB type (user-configurable)

    // Input validation / safety limits
    size_t max_row_size_bytes = 0;        // 0 = unlimited; rows exceeding this are rejected
    size_t max_statement_size_bytes = 0;  // 0 = unlimited; SQL statements exceeding this are skipped
    bool enforce_utf8 = false;            // Reject rows/statements containing invalid UTF-8 sequences

    // Checkpoint / resume support
    std::string checkpoint_file;          // Path to checkpoint JSON file (empty = disabled)

    // Observability: optional metrics emission callback (Prometheus / OTel / custom)
    MetricsCallback metrics_callback;     // Called at row/error/table/duration events

    // Observability: optional distributed tracing / OpenTelemetry span callback
    // Called at start+end of major operations; see SpanCallback documentation above.
    SpanCallback tracing_callback;

    // Access control: optional permission-check callback
    // Called once at import start with ("import", "write"); deny → PERMISSION_DENIED error.
    PermissionCheckCallback permission_check;

    // Quarantine: rows that fail data conversion are appended to this file as JSON-L.
    // Each line is: { "table": ..., "row": ..., "error": { "code": ..., "message": ... } }
    // Empty string = disabled (default).
    std::string quarantine_file;

    // Delta / incremental import: skip rows whose 64-bit FNV-1a content hash is already
    // present in delta_hash_file.  After import the file is updated with new hashes.
    // Empty string = disabled (default).
    std::string delta_hash_file;

    // Columns to use as the delta key for hash computation.
    // Each entry is a column name; the hash is computed over the concatenation of
    // their values (separated by a non-printable field separator).
    // If empty, the entire raw row string is hashed instead.
    std::vector<std::string> delta_key_columns;
    
    json toJson() const {
        return json{
            {"dry_run", dry_run},
            {"continue_on_error", continue_on_error},
            {"batch_size", batch_size},
            {"auto_create_schema", auto_create_schema},
            {"default_namespace", default_namespace},
            {"preserve_ids", preserve_ids},
            {"update_existing", update_existing},
            {"skip_duplicates", skip_duplicates},
            {"include_tables", include_tables},
            {"exclude_tables", exclude_tables},
            {"include_schemas", include_schemas},
            {"max_row_size_bytes", max_row_size_bytes},
            {"max_statement_size_bytes", max_statement_size_bytes},
            {"enforce_utf8", enforce_utf8},
            {"checkpoint_file", checkpoint_file},
            {"quarantine_file", quarantine_file},
            {"delta_hash_file", delta_hash_file},
            {"delta_key_columns", delta_key_columns}
        };
    }
};

// ============================================================================
// Async import API
// ============================================================================

/**
 * @brief Async import status
 */
enum class ImportStatus {
    PENDING,    ///< Job submitted, not yet started
    RUNNING,    ///< Import in progress
    COMPLETED,  ///< Import finished successfully (or with skipped/failed rows)
    CANCELLED,  ///< Cancelled via cancel()
    FAILED      ///< Fatal error stopped the import
};

/**
 * @brief Live handle for an in-progress or completed async import.
 *
 * Returned by `IImporter::importDataAsync()`.  All fields are thread-safe:
 * the importer worker thread writes to the atomic counters while the caller
 * or HTTP handler thread reads them.
 */
struct ImportHandle {
    std::string id;           ///< Unique job ID (UUID-like string)

    // Live progress – updated by the worker thread
    std::atomic<size_t> current_records{0};    ///< Records processed so far
    std::atomic<size_t> total_records{0};      ///< Estimated total (0 = unknown)
    std::atomic<bool>   running{false};

    // Human-readable current stage, e.g. "parsing", "copying table users"
    std::string         stage;
    mutable std::mutex  stage_mutex;

    // Final result – available once running == false
    std::shared_future<ImportStats> future;

    // Start / end timestamps (epoch milliseconds)
    int64_t started_at_ms  = 0;
    int64_t finished_at_ms = 0;

    ImportHandle() = default;
    // Non-copyable (contains mutexes and atomics)
    ImportHandle(const ImportHandle&) = delete;
    ImportHandle& operator=(const ImportHandle&) = delete;

    ImportStatus getStatus() const {
        if (running.load()) return ImportStatus::RUNNING;
        if (!future.valid()) return ImportStatus::PENDING;
        using fs = std::future_status;
        if (future.wait_for(std::chrono::seconds(0)) != fs::ready) return ImportStatus::RUNNING;
        return ImportStatus::COMPLETED;
    }

    std::string getStage() const {
        std::lock_guard<std::mutex> lk(stage_mutex);
        return stage;
    }

    void setStage(const std::string& s) {
        std::lock_guard<std::mutex> lk(stage_mutex);
        stage = s;
    }

    json toJson() const {
        std::string st;
        switch (getStatus()) {
            case ImportStatus::PENDING:    st = "pending";    break;
            case ImportStatus::RUNNING:    st = "running";    break;
            case ImportStatus::COMPLETED:  st = "completed";  break;
            case ImportStatus::CANCELLED:  st = "cancelled";  break;
            case ImportStatus::FAILED:     st = "failed";     break;
        }
        json j{
            {"id",              id},
            {"status",          st},
            {"stage",           getStage()},
            {"current_records", current_records.load()},
            {"total_records",   total_records.load()},
            {"started_at_ms",   started_at_ms},
            {"finished_at_ms",  finished_at_ms}
        };
        if (getStatus() == ImportStatus::COMPLETED &&
            future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                // shared_future::get() may be called multiple times safely;
                // it returns a const reference to the stored value.
                j["stats"] = future.get().toJson();
            } catch (...) {}
        }
        return j;
    }
};

/**
 * @brief Thread-safe registry of active and recently completed import jobs.
 *
 * Holds shared_ptr<ImportHandle> entries keyed by job ID.  Jobs are kept in
 * the registry after completion so status queries can retrieve final stats.
 */
class ImportJobRegistry {
public:
    void add(std::shared_ptr<ImportHandle> handle) {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_[handle->id] = std::move(handle);
    }

    std::shared_ptr<ImportHandle> get(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = jobs_.find(id);
        return (it != jobs_.end()) ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<ImportHandle>> all() const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<std::shared_ptr<ImportHandle>> out;
        out.reserve(jobs_.size());
        for (auto& [k, v] : jobs_) out.push_back(v);
        return out;
    }

    void remove(const std::string& id) {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_.erase(id);
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<ImportHandle>> jobs_;
};

/**
 * @brief Base Importer Interface
 * 
 * All importers (PostgreSQL, MySQL, CSV, etc.) implement this interface.
 */
class IImporter {
public:
    virtual ~IImporter() = default;
    
    /**
     * @brief Get importer name
     */
    virtual const char* getName() const = 0;
    
    /**
     * @brief Get supported source types
     * @return List of supported types (e.g., "postgresql", "mysql", "csv")
     */
    virtual std::vector<std::string> getSupportedTypes() const = 0;
    
    /**
     * @brief Initialize importer with configuration
     * @param config Configuration JSON
     * @return true if initialized successfully
     */
    virtual bool initialize(const std::string& config) = 0;
    
    /**
     * @brief Validate source before import
     * @param source_path Path to source (file, directory, connection string)
     * @param errors Output: validation errors
     * @return true if source is valid
     */
    virtual bool validateSource(const std::string& source_path, std::vector<std::string>& errors) = 0;
    
    /**
     * @brief Import data from source (synchronous)
     * @param source_path Path to source
     * @param options Import options
     * @param progress_callback Optional progress callback
     * @return Import statistics
     */
    virtual ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) = 0;

    /**
     * @brief Import data from source (asynchronous)
     *
     * Launches a background thread and returns an `ImportHandle` immediately.
     * Callers can poll `handle->getStatus()` and read live progress from
     * `handle->current_records`.  When `getStatus() == COMPLETED` the full
     * `ImportStats` is available via `handle->future.get()`.
     *
     * **Lifetime requirement**: The `IImporter` instance must outlive all
     * pending `ImportHandle` futures.  In practice, hold the importer via a
     * `shared_ptr<IImporter>` whose lifetime is at least as long as the handle
     * (e.g., store both in the same owning object or `ImportJobRegistry`).
     *
     * @param source_path Path to source
     * @param options Import options
     * @return Shared handle; call `cancel()` then inspect `future` when done.
     */
    virtual std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) = 0;
    
    /**
     * @brief Cancel ongoing import
     */
    virtual void cancel() = 0;
    
    /**
     * @brief Get schema information from source
     * @param source_path Path to source
     * @return Schema as JSON
     */
    virtual json getSourceSchema(const std::string& source_path) = 0;
};

} // namespace importers
} // namespace themis
