/**
 * @file importer_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: importer_interface.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 891
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3081 feat(importers): S3-compati... (2026-03-12) | #3014 [importers] Add SQLite impo... (2026-03-12) | #2813 [importers] Implement confl... (2026-03-12) | #2594 [importers] Streaming impor... (2026-03-12) | #3774 feat(importers): PostgreSQL... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
 * @brief Conflict resolution strategy for import jobs.
 *
 * Controls what happens when the same conflict key (see
 * ImportOptions::conflict_key_columns) is encountered more than once during an
 * import session.
 *
 * | Strategy  | Behaviour on conflict                                         |
 * |-----------|---------------------------------------------------------------|
 * | OVERWRITE | Discard the previously seen entity; use the incoming one      |
 * | SKIP      | Keep the previously seen entity; discard the incoming one     |
 * | MERGE     | Merge fields from both entities; incoming fields win unless   |
 * |           | listed in ImportOptions::protected_fields                     |
 * | ERROR     | Treat the conflict as a fatal error; abort the batch          |
 *
 * Default is OVERWRITE for backward compatibility.
 */
enum class ConflictStrategy {
    OVERWRITE,  ///< Replace existing entity with incoming (default)
    SKIP,       ///< Keep existing entity, discard incoming duplicate
    MERGE,      ///< Field-level merge; incoming wins unless field is protected
    ERROR       ///< Abort the batch on the first conflict
};

// ============================================================================
// Entity Linking / MDM configuration (used by ImportOptions)
// ============================================================================

/**
 * @brief Per-collection semantic matching settings for the MDM pipeline.
 */
struct CollectionMatchingConfig {
    /// Field names to use for deterministic (exact-key) matching.
    std::vector<std::string> primary_key_fields;

    /// Field names with unique constraints (secondary deterministic matching).
    std::vector<std::string> unique_fields;

    /// Semantic matching algorithm per field: "jaro_winkler", "levenshtein",
    /// "soundex", "email", "phone".
    std::map<std::string, std::string> field_algorithms;

    /// Per-field weight for the semantic matching score (0.0–1.0).
    std::map<std::string, double> field_weights;

    /// Minimum overall semantic confidence to accept a match (default: 0.85).
    double semantic_threshold = 0.85;
};

/**
 * @brief Configuration for the MDM entity-linking phase of an import.
 *
 * When @c enabled is true, the importer runs an MDM workflow after the
 * standard import phase to match, link, and deduplicate incoming entities
 * against existing ones in ThemisDB.
 *
 * The strategy and thresholds can be overridden per collection via
 * @c collection_configs.
 */
struct EntityLinkingConfig {
    /// When false the MDM workflow is completely bypassed (default).
    bool enabled = false;

    /// Matching strategy: 0 = DETERMINISTIC_FIRST, 1 = SEMANTIC_FIRST,
    /// 2 = WEIGHTED_ENSEMBLE.  Stored as int to avoid pulling in
    /// entity_matcher.h from this header.
    int strategy = 0; // DETERMINISTIC_FIRST

    double deterministic_threshold = 1.0;
    double semantic_threshold      = 0.85;

    /// Resolution policy: 0–5 maps to ResolutionPolicy enum values.
    int resolution_policy = 4; // RICHEST_MERGE

    /// Automatically resolve conflicts without queuing for manual review.
    bool auto_resolve_conflicts = false;

    /// Create reverse links (target → source) in addition to forward links.
    bool create_reverse_links = true;

    /// Fields that are never overwritten during golden-record creation.
    std::vector<std::string> protected_fields;

    /// Per-collection overrides for matching algorithm and thresholds.
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

/**
 * @brief Import Statistics
 */
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

    /// Optional sample of imported entities (used by MDM post-processing).
    /// Populated by the importer when entity_linking.enabled is true and
    /// the batch fits within the configured sample limit.
    json sample_entities = json::array();
    
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

/**
 * @brief Progress Callback
 */
using ProgressCallback = std::function<void(const std::string& stage, size_t current, size_t total)>;

/**
 * @brief Streaming Row Callback for memory-efficient large-dataset imports.
 *
 * Invoked by the importer for each converted entity as it is produced from the
 * source file, enabling callers to process rows one-by-one without waiting for
 * the entire dataset to be read into memory.
 *
 * @param table_name  Name of the source table the entity belongs to.
 * @param entity      Converted row as a JSON object (field-name → value).
 *
 * @return `true`  to continue the import; `false` to abort immediately.
 *
 * Thread-safety: the callback is invoked from the same thread that calls
 * `importDataStreaming()`.  Implementations that share state with other
 * threads must provide their own synchronisation.
 *
 * Example – stream rows directly to a sink without buffering:
 * @code
 *   ImportOptions opts;
 *   auto stats = importer.importDataStreaming(path, opts,
 *       [&sink](const std::string& table, const json& row) -> bool {
 *           return sink.write(table, row);  // false on sink error → abort
 *       });
 * @endcode
 */
using RowCallback = std::function<bool(const std::string& table_name, const json& entity)>;

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

    /// Strategy to apply when the same conflict key appears more than once
    /// during an import session.  Default: OVERWRITE (backward compatible).
    ConflictStrategy conflict_strategy = ConflictStrategy::OVERWRITE;

    /// Columns whose values form the conflict detection key.
    /// If empty, no in-session conflict detection is performed.
    /// Example: {"id"} or {"tenant_id", "user_id"}
    std::vector<std::string> conflict_key_columns;

    /// Fields that the MERGE strategy must not overwrite with incoming values.
    /// Ignored by SKIP, OVERWRITE, and ERROR strategies.
    std::vector<std::string> protected_fields;

    /// Recursion depth for the MERGE strategy.
    /// 1  = top-level fields only (default, nested objects replaced entirely).
    /// -1 = deep recursive merge for all nested JSON objects.
    /// N  = merge up to N levels deep.
    int merge_depth = 1;

    // -------------------------------------------------------------------------
    // Schema auto-detection and validation
    // -------------------------------------------------------------------------

    /// When true, the importer samples the first schema_sample_rows data rows
    /// to auto-detect column types, then validates every row against the
    /// detected schema during import.  Type mismatches are recorded as
    /// SCHEMA_VALIDATION_FAILED (code 504) WARNING-severity structured errors.
    /// Rows with type mismatches are still imported; validation failures are
    /// non-fatal by design and do not count as failed_records.
    bool validate_schema = false;

    /// Number of data rows to sample for schema type inference.
    /// Only used when validate_schema is true.  Defaults to 100.
    /// Setting this to 0 effectively disables schema detection (no rows are
    /// sampled → no schema is built → per-row validation is skipped).
    size_t schema_sample_rows = 100;

    // -------------------------------------------------------------------------
    // v2.0: Foreign Key Preservation
    // -------------------------------------------------------------------------

    /// When true (default), the importer parses and preserves FOREIGN KEY
    /// constraints from the dump.  Extracted FK metadata is:
    ///   - stored in the per-table schema (getSourceSchema returns "foreign_keys")
    ///   - counted in ImportStats::foreign_keys_preserved
    ///   - embedded in entity JSON as "_foreign_keys" array when present
    ///
    /// Setting this to false restores v1.x behaviour (FKs silently skipped).
    bool preserve_foreign_keys = true;
    // Foreign Key / Relationship preservation (v2.0)
    // -------------------------------------------------------------------------

    /// When true, Foreign Key constraints are extracted and preserved as
    /// ThemisDB graph relationships during import.  Default: true.
    bool preserve_relationships = true;

    /// When true, all FK references are validated before data import starts.
    /// Missing target tables produce structured UNKNOWN_TABLE errors.  Default: false.
    bool validate_references = false;

    /// How FK constraints are mapped to graph edges.
    ///   "auto"   – detect cardinality automatically (default)
    ///   "manual" – no automatic mapping; user configures via API
    ///   "skip"   – do not create graph edges for FKs
    std::string relationship_mapping_mode = "auto";
    // Entity linking / Master Data Management (MDM)
    // -------------------------------------------------------------------------

    /// When entity_linking.enabled is true, a post-import MDM workflow
    /// matches, links, and deduplicates the imported entities against
    /// existing ThemisDB records using the configured strategy.
    EntityLinkingConfig entity_linking;

    // -------------------------------------------------------------------------
    // I1: Connection / operation timeout enforcement (Phase 4 hardening)
    // -------------------------------------------------------------------------

    /// Maximum milliseconds allowed for the entire importData() call.
    /// When exceeded the import is aborted with a DEADLINE_EXCEEDED (110) error
    /// and a structured audit event is emitted via THEMIS_WARN.
    /// Set to 0 (default) to disable the timeout guard.
    ///
    /// Typical values:
    ///   connection_timeout_ms = 5000   (file open / header validation)
    ///   query_timeout_ms      = 30000  (full dump processing)
    ///   fetch_timeout_ms      = 10000  (per result-set fetch window)
    ///
    /// All three share the single import_timeout_ms budget in file-based importers.
    /// Live-connection importers may honour them individually.
    uint32_t import_timeout_ms = 0;   ///< 0 = disabled

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
        std::vector<json> out;
        out.reserve(handles.size());
        for (const auto& handle : handles) {
            out.push_back(handle->toJson());
        }
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
    [[nodiscard]] virtual const char* getName() const = 0;
    
    /**
     * @brief Get supported source types
     * @return List of supported types (e.g., "postgresql", "mysql", "csv")
     */
    [[nodiscard]] virtual std::vector<std::string> getSupportedTypes() const = 0;
    
    /**
     * @brief Initialize importer with configuration
     * @param config Configuration JSON
     * @return true if initialized successfully
     */
    [[nodiscard]] virtual bool initialize(const std::string& config) = 0;
    
    /**
     * @brief Validate source before import
     * @param source_path Path to source (file, directory, connection string)
     * @param errors Output: validation errors
     * @return true if source is valid
     */
    [[nodiscard]] virtual bool validateSource(const std::string& source_path, std::vector<std::string>& errors) = 0;
    
    /**
     * @brief Import data from source (synchronous)
     * @param source_path Path to source
     * @param options Import options
     * @param progress_callback Optional progress callback
     * @return Import statistics
     */
    [[nodiscard]] virtual ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) = 0;

    /**
     * @brief Import data from source with per-row streaming callback.
     *
     * Reads the source file in a single pass, delivering each converted entity
     * to @p row_callback immediately after it is produced.  No rows are held in
     * memory between callback invocations, making this suitable for datasets
     * that would otherwise exhaust available RAM.
     *
     * The callback signature is:
     * @code
     *   bool callback(const std::string& table_name, const json& entity);
     * @endcode
     * Return `true` to continue; `false` to abort the import early (the method
     * will return with whatever @c ImportStats have been accumulated so far).
     *
     * All other @p options (filtering, type overrides, UTF-8 enforcement, etc.)
     * are applied in the same way as `importData()`.
     *
     * The default implementation stores the callback in a copy of @p options
     * and delegates to `importData()`.  Derived classes may override for
     * connector-specific streaming optimisations.
     *
     * @param source_path  Path to source file / connection string.
     * @param options      Import options (streaming_row_callback is overwritten).
     * @param row_callback Callback invoked for every successfully converted row.
     * @return             Accumulated import statistics.
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
    [[nodiscard]] virtual std::shared_ptr<ImportHandle> importDataAsync(
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
    [[nodiscard]] virtual json getSourceSchema(const std::string& source_path) = 0;
};

} // namespace importers
} // namespace themis
