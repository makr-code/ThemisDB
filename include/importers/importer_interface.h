/**
 * @file importer_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef ERROR
#undef ERROR
#endif

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
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

enum class ImportErrorSeverity {
    INFO,     ///< Informational (e.g., skipped duplicate)
    WARNING,  ///< Non-fatal issue (e.g., unknown type, using default)
    ERROR,    ///< Row/record-level failure (import continues if continue_on_error)
    CRITICAL  ///< Fatal failure that stops the import
};

enum class ImportErrorCode : uint32_t {
    // Success
    SUCCESS = 0,

    // I/O errors (100-199)
    FILE_NOT_FOUND       = 100,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,
    NOT_A_SQLITE_DUMP    = 104,  ///< File does not appear to be a SQLite dump
    IMPORT_CONNECTOR_UNAVAILABLE = 105,  ///< Source connector (DB, file, S3, Kafka) unreachable; connection pool exhausted

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
    IMPORT_SCHEMA_MISMATCH = 302,  ///< Schema cache invalidated; schema changed or connection lost

    // Data conversion errors (400-499)
    TYPE_CONVERSION      = 400,
    UNKNOWN_PG_TYPE      = 401,
    VALUE_OUT_OF_RANGE   = 402,

    // Validation / policy errors (500-599)
    DRY_RUN_ONLY             = 500,
    TABLE_EXCLUDED           = 501,
    INVALID_UTF8             = 502,
    PERMISSION_DENIED        = 503,  ///< Caller's permission_check callback returned false
    SCHEMA_VALIDATION_FAILED = 504,  ///< Row value does not match the auto-detected schema type

    // SQL parsing errors – extended range (206)
    BINARY_COPY_FORMAT   = 206,  ///< Binary (non-text) COPY data detected; unsupported

    // Timeout / deadline errors (110-119)
    DEADLINE_EXCEEDED    = 110,  ///< Import operation exceeded configured import_timeout_ms

    // Conflict resolution errors (600-699)
    CONFLICT_ERROR       = 600,  ///< ERROR strategy triggered on key conflict

    // Generic errors (900-999)
    UNKNOWN              = 900
};

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

// ============================================================================
// PHASE-3-ERROR-HANDLING: Connector Capability Fallback Chain
// ============================================================================

enum class ConnectorCapability {
    BASIC_IMPORT,           ///< All connectors support basic row import
    CDC_SUPPORT,            ///< Change Data Capture (fallback: polling)
    SCHEMA_INFERENCE,       ///< Auto-detect schema (fallback: sampling → ALL_TEXT)
    TRANSACTION_SUPPORT,    ///< ACID transactions (fallback: checkpointing)
    BATCH_OPTIMIZATION      ///< Bulk operations (fallback: single-row)
};

struct CapabilityCheckResult {
    bool supported = {};

    std::string fallback_path;

    float performance_delta;

    std::string audit_message;

    json toJson() const {
        return json{
            {"supported",        supported},
            {"fallback_path",    fallback_path},
            {"performance_delta", performance_delta},
            {"audit_message",    audit_message}
        };
    }
};

enum class ConflictStrategy {
    OVERWRITE,  ///< Replace existing entity with incoming (default)
    SKIP,       ///< Keep existing entity, discard incoming duplicate
    MERGE,      ///< Field-level merge; incoming wins unless field is protected
    ERROR       ///< Abort the batch on the first conflict
};

// ============================================================================
// Entity Linking / MDM configuration (used by ImportOptions)
// ============================================================================

struct CollectionMatchingConfig {
    std::vector<std::string> primary_key_fields;

    std::vector<std::string> unique_fields;

    std::map<std::string, std::string> field_algorithms;

    std::map<std::string, double> field_weights;

    double semantic_threshold = 0.85;
};

struct EntityLinkingConfig {
    bool enabled = false;

    int strategy = 0; // DETERMINISTIC_FIRST

    double deterministic_threshold = 1.0;
    double semantic_threshold      = 0.85;

    int resolution_policy = 4; // RICHEST_MERGE

    bool auto_resolve_conflicts = false;

    bool create_reverse_links = true;

    std::vector<std::string> protected_fields;

    std::map<std::string, CollectionMatchingConfig> collection_configs;

    json toJson() const {
        return json{
            {"enabled",                  enabled},
            {"strategy",                 strategy},
            {"deterministic_threshold",  deterministic_threshold},
            {"semantic_threshold",       semantic_threshold},
            {"resolution_policy",        resolution_policy},
            {"auto_resolve_conflicts",   auto_resolve_conflicts},
            {"create_reverse_links",     create_reverse_links},
            {"protected_fields",         protected_fields}
        };
    }
};

struct ImportStats {
    size_t total_records = 0;
    size_t imported_records = 0;
    size_t failed_records = 0;
    size_t skipped_records = 0;
    size_t quarantined_records = 0;  ///< Rows written to the quarantine file

    // Conflict resolution counters
    size_t conflicts_skipped     = 0;  ///< Rows skipped due to SKIP strategy
    size_t conflicts_overwritten = 0;  ///< Rows overwritten due to OVERWRITE strategy
    size_t conflicts_merged      = 0;  ///< Rows merged due to MERGE strategy
    
    size_t tables_processed = 0;
    size_t schemas_processed = 0;
    size_t custom_types_processed = 0;  ///< CREATE TYPE statements parsed (enum / composite)
    size_t foreign_keys_preserved = 0;  ///< Foreign key constraints extracted and preserved (v2.0)
    size_t relationships_processed = 0; ///< Foreign key constraints mapped to graph relationships
    size_t indexes_processed = 0;       ///< CREATE INDEX statements parsed
    
    double elapsed_seconds = 0.0;

    // Dump-mode flags (set from dump header comments)
    bool is_schema_only = false;  ///< true when pg_dump --schema-only header detected
    bool is_data_only   = false;  ///< true when pg_dump --data-only header detected
    
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;  ///< Machine-readable error list

    // MDM / Entity-linking counters (populated when entity_linking is enabled)
    size_t entities_linked    = 0;  ///< Entity links created by the MDM phase
    size_t golden_records     = 0;  ///< Golden records produced by the MDM phase
    size_t mdm_reviews_needed = 0;  ///< Entities queued for manual review

    json sample_entities = json::array();

    // Backwards-compatibility aliases (legacy test and plugin code may use
    // the older field names). These are kept as separate counters to avoid
    // changing existing initialization behaviour; callers are encouraged to
    // use the canonical names above. They will be synchronized by the
    // importer where necessary.
    // Deprecated: use `imported_records` instead of `rows_imported`.
    size_t rows_imported = 0;
    // Deprecated: use `skipped_records` instead of `rows_skipped`.
    size_t rows_skipped = 0;
    // Deprecated: use `quarantined_records` instead of `rows_quarantined`.
    size_t rows_quarantined = 0;
    
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
            {"conflicts_skipped", conflicts_skipped},
            {"conflicts_overwritten", conflicts_overwritten},
            {"conflicts_merged", conflicts_merged},
            {"tables_processed", tables_processed},
            {"schemas_processed", schemas_processed},
            {"custom_types_processed", custom_types_processed},
            {"foreign_keys_preserved", foreign_keys_preserved},
            {"relationships_processed", relationships_processed},
            {"indexes_processed", indexes_processed},
            {"elapsed_seconds", elapsed_seconds},
            {"is_schema_only", is_schema_only},
            {"is_data_only", is_data_only},
            {"warnings", warnings},
            {"errors", errors},
            {"structured_errors", err_arr},
            {"entities_linked",    entities_linked},
            {"golden_records",     golden_records},
            {"mdm_reviews_needed", mdm_reviews_needed}
        };
    }
};

using ProgressCallback = std::function<void(const std::string& stage, size_t current, size_t total)>;

using RowCallback = std::function<bool(const std::string& table_name, const json& entity)>;

using SpanCallback = std::function<void(
    const std::string& operation,
    const std::map<std::string, std::string>& attributes,
    double duration_seconds
)>;

using MetricsCallback = std::function<void(
    const std::string& metric,
    const std::map<std::string, std::string>& labels,
    double value
)>;

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

    // Streaming row callback for large-dataset imports.
    // When set, each converted entity is delivered to this callback immediately
    // after it is produced, without buffering all rows in memory first.
    // Return false from the callback to abort the import early.
    // Used internally by importDataStreaming(); can also be set directly on
    // ImportOptions passed to importData() for the same effect.
    RowCallback streaming_row_callback;

    // -------------------------------------------------------------------------
    // Conflict resolution
    // -------------------------------------------------------------------------

    ConflictStrategy conflict_strategy = ConflictStrategy::OVERWRITE;

    std::vector<std::string> conflict_key_columns;

    std::vector<std::string> protected_fields;

    int merge_depth = 1;

    // -------------------------------------------------------------------------
    // Schema auto-detection and validation
    // -------------------------------------------------------------------------

    bool validate_schema = false;

    size_t schema_sample_rows = 100;

    // -------------------------------------------------------------------------
    // v2.0: Foreign Key Preservation
    // -------------------------------------------------------------------------

    bool preserve_foreign_keys = true;
    // Foreign Key / Relationship preservation (v2.0)
    // -------------------------------------------------------------------------

    bool preserve_relationships = true;

    bool validate_references = false;

    std::string relationship_mapping_mode = "auto";
    // Entity linking / Master Data Management (MDM)
    // -------------------------------------------------------------------------

    EntityLinkingConfig entity_linking;

    // -------------------------------------------------------------------------
    // I1: Connection / operation timeout enforcement (Phase 4 hardening)
    // -------------------------------------------------------------------------

    uint32_t import_timeout_ms = 0;   ///< 0 = disabled

    // Backwards-compatibility alias: older code/tests used `deadline_ms`.
    // Keep a separate alias field to avoid ABI churn; importer code may
    // synchronize these values at call sites.
    uint32_t deadline_ms = 0;

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
            {"delta_key_columns", delta_key_columns},
            {"conflict_strategy", static_cast<int>(conflict_strategy)},
            {"conflict_key_columns", conflict_key_columns},
            {"protected_fields", protected_fields},
            {"merge_depth", merge_depth},
            {"validate_schema", validate_schema},
            {"schema_sample_rows", schema_sample_rows},
            {"preserve_foreign_keys", preserve_foreign_keys},
            {"preserve_relationships", preserve_relationships},
            {"validate_references", validate_references},
            {"relationship_mapping_mode", relationship_mapping_mode},
            {"entity_linking", entity_linking.toJson()},
            {"import_timeout_ms", import_timeout_ms}
        };
    }
};

// ============================================================================
// Async import API
// ============================================================================

enum class ImportStatus {
    PENDING,    ///< Job submitted, not yet started
    RUNNING,    ///< Import in progress
    COMPLETED,  ///< Import finished successfully (or with skipped/failed rows)
    CANCELLED,  ///< Cancelled via cancel()
    FAILED      ///< Fatal error stopped the import
};

struct ImportHandle {
    std::string id;           ///< Unique job ID (UUID-like string)
    std::string source_path;  ///< Source file path used for this job (v2.0)

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
        if (running.load()) {
          return ImportStatus::RUNNING;
        }
        if (!future.valid()) {
          return ImportStatus::PENDING;
        }
        using fs = std::future_status;
        if (future.wait_for(std::chrono::seconds(0)) != fs::ready) {
          return ImportStatus::RUNNING;
        }
        return ImportStatus::COMPLETED;
    }

    std::string getStage() const {
        /**
         * @brief Lk.
         * @param[in] stage_mutex Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(stage_mutex);
        return stage;
    }

    /**
     * @brief Set Stage.
     * @param[in] s Input parameter.
     * @details Calls: lk().
     */
    void setStage(const std::string& s) {
        std::lock_guard<std::mutex> lk(stage_mutex);
        stage = s;
    }

    json toJson() const {
        std::string st = {};
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

class ImportJobRegistry {
public:
    /**
     * @brief Add.
     * @param[in] handle Input parameter.
     * @details Calls: lk(), std::move().
     */
    void add(std::shared_ptr<ImportHandle> handle) {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_[handle->id] = std::move(handle);
    }

    std::shared_ptr<ImportHandle> get(const std::string& id) const {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = jobs_.find(id);
        return (it != jobs_.end()) ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<ImportHandle>> all() const {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<std::shared_ptr<ImportHandle>> out;
        out.reserve(jobs_.size());
        for (auto& [k, v] : jobs_) {
          out.push_back(v);
        }
        return out;
    }

    std::optional<json> getJsonSnapshot(const std::string& id) const {
        auto handle = get(id);
        if (!handle) {
            return std::nullopt;
        }
        return handle->toJson();
    }

    std::optional<std::pair<bool, json>> getRunningAndJsonSnapshot(
        const std::string& id) const {
        auto handle = get(id);
        if (!handle) {
            return std::nullopt;
        }
        return std::make_pair(handle->running.load(), handle->toJson());
    }

    std::optional<std::string> getSourcePathSnapshot(const std::string& id) const {
        auto handle = get(id);
        if (!handle) {
            return std::nullopt;
        }
        return handle->source_path;
    }

    std::vector<json> allJsonSnapshots() const {
        auto handles = all();
        std::vector<json> out = {};

        out.reserve(handles.size());
        for (const auto& handle : handles) {
            out.push_back(handle->toJson());
        }
        return out;
    }

    /**
     * @brief Remove.
     * @param[in] id Input parameter.
     * @details Calls: lk(), erase().
     */
    void remove(const std::string& id) {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_.erase(id);
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<ImportHandle>> jobs_;
};

class IImporter {
public:
    /**
     * @brief IImporter.
     * @return Return value.
     */
    virtual ~IImporter() = default;
    
    [[nodiscard]] virtual const char* getName() const = 0;
    
    [[nodiscard]] virtual std::vector<std::string> getSupportedTypes() const = 0;
    
    [[nodiscard]] virtual bool initialize(const std::string& config) = 0;
    
    [[nodiscard]] virtual bool validateSource(const std::string& source_path, std::vector<std::string>& errors) = 0;
    
    [[nodiscard]] virtual ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) = 0;

    /**
     * @brief Import Data Streaming.
     * @param[in] source_path Path to the source.
     * @param[in] options Input parameter.
     * @param[in] row_callback Input parameter.
     * @return Return value.
     * @details Calls: std::move(), importData().
     */
    virtual ImportStats importDataStreaming(
        const std::string& source_path,
        const ImportOptions& options,
        RowCallback row_callback
    ) {
        ImportOptions streaming_opts = options;
        streaming_opts.streaming_row_callback = std::move(row_callback);
        return importData(source_path, streaming_opts, nullptr);
    }

    [[nodiscard]] virtual std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) = 0;
    
    /**
     * @brief Cancel.
     */
    virtual void cancel() = 0;
    
    [[nodiscard]] virtual json getSourceSchema(const std::string& source_path) = 0;
};

} // namespace importers
} // namespace themis
