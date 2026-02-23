/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_manager.h                                ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1012                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c8bd4be58  2026-02-22  Add withApiSource() to IngestionBuilder for cursor/offset... ║
    • 4699a5a4d  2026-02-22  audit(ingestion): add quarantine_retry_success_total Prom... ║
    • 57ca95f7c  2026-02-22  feat(ingestion): per-document quarantine retry with expon... ║
    • 8798208c4  2026-02-22  feat(ingestion): implement cursor-based pagination with o... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>

namespace themis {
namespace ingestion {

/**
 * @brief Source type enumeration
 */
enum class SourceType {
    HUGGINGFACE,    ///< HuggingFace datasets
    FILESYSTEM,     ///< Local file system (PDF, DOCX, etc.)
    API,            ///< REST/SOAP API (future)
    DATABASE        ///< Legacy database exports (future)
};

/**
 * @brief Pagination mode for API source connectors
 *
 * Controls how the connector advances through pages of results.
 *
 * - `OFFSET`: Numeric offset/limit style – each page request carries an
 *   integer offset that advances by the number of items received.
 *   Example URL: `?offset=100&limit=50`
 *
 * - `CURSOR`: Opaque cursor token style – the first page request carries no
 *   cursor; each subsequent request uses the cursor token returned in the
 *   previous response.  Pagination stops when the response contains no cursor.
 *   Example URL: `?cursor=abc123&limit=50`
 *
 * Configured via `SourceConfig::options["pagination_mode"]` with values
 * `"offset"` (default) or `"cursor"`.
 */
enum class PaginationMode {
    OFFSET,  ///< Numeric offset/limit pagination (default)
    CURSOR   ///< Opaque cursor-token pagination
};

/**
 * @brief Structured error codes for ingestion operations
 */
enum class IngestionErrorCode {
    OK = 0,

    // Source / configuration errors (1000-1099)
    SOURCE_NOT_FOUND        = 1000,
    SOURCE_UNAVAILABLE      = 1001,
    SOURCE_NOT_CONFIGURED   = 1002,
    SOURCE_DISABLED         = 1003,
    CONNECTOR_INIT_FAILED   = 1004,
    CONNECTOR_NOT_SUPPORTED = 1005,

    // Network / HTTP errors (1100-1199)
    HTTP_REQUEST_FAILED     = 1100,
    HTTP_UNAUTHORIZED       = 1101,
    HTTP_NOT_FOUND          = 1102,
    HTTP_RATE_LIMITED       = 1103,
    HTTP_SERVER_ERROR       = 1104,
    HTTP_TIMEOUT            = 1105,

    // File / IO errors (1200-1299)
    FILE_NOT_FOUND          = 1200,
    FILE_READ_ERROR         = 1201,
    FILE_FORMAT_UNSUPPORTED = 1202,
    FILE_ENCODING_ERROR     = 1203,

    // Processing errors (1300-1399)
    PROCESSING_FAILED       = 1300,
    PARSING_FAILED          = 1301,
    EXTRACTION_FAILED       = 1302,
    OCR_FAILED              = 1303,

    // Retry/quota errors (1400-1499)
    RETRY_EXHAUSTED         = 1400,
    QUOTA_EXCEEDED          = 1401,

    // Internal errors (1900-1999)
    INTERNAL_ERROR          = 1900,
    UNKNOWN_ERROR           = 1999
};

/**
 * @brief Error severity classification
 */
enum class IngestionErrorSeverity {
    INFO,       ///< Informational – not a failure
    WARNING,    ///< Degraded operation, non-fatal
    ERROR,      ///< Single item/source failure
    FATAL       ///< Whole ingestion run aborted
};

/**
 * @brief Structured ingestion error
 */
struct IngestionError {
    IngestionErrorCode   code     = IngestionErrorCode::OK;
    IngestionErrorSeverity severity = IngestionErrorSeverity::ERROR;
    std::string          message;         ///< Human-readable message
    std::string          source_id;       ///< Originating source (if known)
    std::string          details;         ///< Technical detail for logs

    IngestionError() = default;

    IngestionError(IngestionErrorCode c, IngestionErrorSeverity sev,
                   const std::string& msg, const std::string& sid = "",
                   const std::string& det = "")
        : code(c), severity(sev), message(msg), source_id(sid), details(det) {}

    bool isOk()      const { return code == IngestionErrorCode::OK; }
    bool isFatal()   const { return severity == IngestionErrorSeverity::FATAL; }
    bool isWarning() const { return severity == IngestionErrorSeverity::WARNING; }

    /** @brief Returns true for codes that should trigger a retry */
    bool isRetryable() const {
        return code == IngestionErrorCode::HTTP_TIMEOUT
            || code == IngestionErrorCode::HTTP_SERVER_ERROR
            || code == IngestionErrorCode::HTTP_RATE_LIMITED;
    }
};

/**
 * @brief Retry / back-off configuration for connectors
 */
struct RetryConfig {
    int    max_attempts           = 3;     ///< Maximum total attempts (1 = no retry)
    double initial_delay_ms       = 500.0; ///< First back-off delay (ms)
    double backoff_factor         = 2.0;   ///< Exponential multiplier per attempt
    double max_delay_ms           = 30000.0; ///< Cap on per-attempt delay (ms)
    int    timeout_ms             = 30000; ///< Per-request timeout (ms)
    int    max_quarantine_retries = 5;     ///< Max per-document quarantine retry attempts

    RetryConfig() = default;
};

/**
 * @brief Per-source rate-limit configuration
 *
 * Controls the maximum throughput for a single source connector.
 * The token-bucket algorithm is used: tokens refill at `requests_per_second`
 * and each request consumes one token.  When the bucket is empty the connector
 * waits until a token is available (blocking back-pressure).
 */
struct RateLimitConfig {
    double  requests_per_second = 0.0;   ///< 0 = unlimited; token-bucket refill rate
    size_t  max_bytes_per_hour  = 0;     ///< 0 = unlimited
    bool    enabled             = false; ///< Must be true to activate

    RateLimitConfig() = default;
};

/**
 * @brief Source preview – a lightweight sample of documents from a source
 *
 * Returned by `IngestionManager::previewSource()`. Contains the first
 * up-to `max_documents` items without writing them to any collection.
 */
struct SourcePreview {
    std::string source_id;                 ///< Source that was sampled
    std::vector<std::string> documents;    ///< Extracted document contents
    size_t total_available = 0;            ///< Total documents available in source
    bool truncated = false;               ///< true if there are more docs than returned

    SourcePreview() = default;
};

/**
 * @brief Lightweight observability metrics collected during ingestion
 */
struct IngestionMetrics {
    size_t retry_count      = 0;  ///< Total retries across all requests
    size_t timeout_count    = 0;  ///< Requests that timed out
    size_t error_count      = 0;  ///< Total individual errors encountered
    double throughput_docs_per_sec = 0.0; ///< Documents / second
    size_t quota_violations = 0;  ///< Times rate/byte-quota was exceeded

    IngestionMetrics() = default;
};

/**
 * @brief Source configuration for data ingestion
 */
struct SourceConfig {
    std::string source_id;           ///< Unique identifier for this source
    SourceType type;                 ///< Type of data source
    std::string location;            ///< Location (URL, path, etc.)
    int priority = 5;                ///< Priority (higher = more important, 1-10)
    bool enabled = true;             ///< Whether this source is enabled
    std::unordered_map<std::string, std::string> options;  ///< Additional options
};

/**
 * @brief Ingestion progress callback
 */
using ProgressCallback = std::function<void(const std::string& source_id, 
                                            size_t processed, 
                                            size_t total,
                                            const std::string& status)>;

/**
 * @brief Ingestion statistics
 */
struct IngestionStats {
    size_t documents_processed = 0;
    size_t documents_failed = 0;
    size_t bytes_processed = 0;
    double elapsed_seconds = 0.0;
    std::string error_message;          ///< Primary error (for backward compatibility)
    std::vector<IngestionError> errors; ///< Structured error log
    IngestionMetrics metrics;           ///< Observability counters
    std::string correlation_id;         ///< Unique run ID for distributed tracing
    
    IngestionStats() = default;

    /** @brief Record a structured error and update backward-compat field */
    void addError(IngestionErrorCode code,
                  IngestionErrorSeverity severity,
                  const std::string& message,
                  const std::string& source_id = "",
                  const std::string& details   = "") {
        IngestionError err{code, severity, message, source_id, details};
        errors.push_back(err);
        metrics.error_count++;
        if (error_message.empty() && err.severity >= IngestionErrorSeverity::ERROR) {
            error_message = message;
        }
    }
};

/**
 * @brief Quarantine entry for a failed item
 *
 * Records an item (file path or URL) that could not be ingested after all
 * retries. Quarantined items can be re-processed via the admin API.
 */
struct QuarantineEntry {
    std::string item_path;         ///< File path or URL of the failed item
    std::string source_id;         ///< Source that produced this entry
    IngestionErrorCode error_code  = IngestionErrorCode::UNKNOWN_ERROR;
    std::string error_message;     ///< Last error message
    size_t retry_count = 0;        ///< Number of retry attempts made
    std::chrono::system_clock::time_point timestamp; ///< When quarantined
    std::string raw_payload;       ///< Serialized document content (for per-doc retry)
    bool permanently_failed = false; ///< True when max_quarantine_retries exceeded

    QuarantineEntry() : timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Ingestion report for all sources
 */
struct IngestionReport {
    std::unordered_map<std::string, IngestionStats> source_stats;
    size_t total_documents = 0;
    size_t total_failures = 0;
    double total_time_seconds = 0.0;
    std::vector<QuarantineEntry> quarantine; ///< Items quarantined during this run
    bool dry_run = false;                    ///< True if run in dry-run mode
    size_t quarantine_retry_successes = 0;   ///< Cumulative successful quarantine retries

    IngestionReport() = default;
};

/**
 * @brief Persistent checkpoint for a single ingestion source
 *
 * Checkpoints allow incremental ingestion: when the same source is ingested
 * again, already-processed documents are skipped and processing resumes from
 * the stored offset.
 */
struct IngestionCheckpoint {
    std::string source_id;
    size_t processed_count = 0;  ///< Documents successfully processed so far
    size_t byte_offset     = 0;  ///< Byte-level offset (for streaming sources)
    std::string cursor;          ///< Opaque pagination cursor (API / HF streaming)
    std::string timestamp;       ///< ISO-8601 timestamp of the last write

    IngestionCheckpoint() = default;
};

/**
 * @brief File-based checkpoint store for incremental ingestion
 *
 * Checkpoints are written as simple key=value text files under the configured
 * directory, one file per source.  The store is fully thread-safe.
 *
 * File name format: `<checkpoint_dir>/<source_id>.checkpoint`
 *
 * Example:
 * @code
 * CheckpointStore store("/var/lib/themis/checkpoints");
 * IngestionCheckpoint cp;
 * if (store.read("hf_legal", cp)) {
 *     std::cout << "Resuming from doc " << cp.processed_count << '\n';
 * }
 * store.write({"hf_legal", 5000, 0, "", "2026-02-20T16:00:00Z"});
 * @endcode
 */
class CheckpointStore {
public:
    /**
     * @brief Construct a store rooted at the given directory
     * @param checkpoint_dir Directory where checkpoint files are persisted.
     *        The directory must already exist (this class does not create it).
     */
    explicit CheckpointStore(const std::string& checkpoint_dir);

    /**
     * @brief Write (or overwrite) a checkpoint to disk
     * @return true on success
     */
    bool write(const IngestionCheckpoint& cp);

    /**
     * @brief Read the checkpoint for a source
     * @param source_id Source whose checkpoint to read
     * @param out       Populated on success
     * @return true if a checkpoint exists and was read successfully
     */
    bool read(const std::string& source_id, IngestionCheckpoint& out) const;

    /**
     * @brief Delete the checkpoint file for a source
     * @return true if the file existed and was removed
     */
    bool clear(const std::string& source_id);

    /**
     * @brief Check whether a checkpoint exists for a source
     */
    bool exists(const std::string& source_id) const;

private:
    std::string checkpointPath(const std::string& source_id) const;

    std::string dir_;
    mutable std::mutex mutex_;
};

/**
 * @brief Forward declaration of connector interface
 */
class ISourceConnector;

/**
 * @brief Unified multi-source ingestion manager
 * 
 * Coordinates ingestion from multiple data sources (HuggingFace, filesystem, APIs, etc.)
 * and stores processed documents in ThemisDB for training.
 * 
 * Example usage:
 * @code
 * IngestionManager mgr(db);
 * mgr.registerSource({
 *     .source_id = "huggingface_legal",
 *     .type = SourceType::HUGGINGFACE,
 *     .location = "lexlms/ger_legal_data",
 *     .priority = 5
 * });
 * auto report = mgr.ingestAll();
 * @endcode
 */
class IngestionManager {
public:
    /**
     * @brief Construct ingestion manager
     * @param db_connection Database connection string or handle
     */
    explicit IngestionManager(const std::string& db_connection);
    
    ~IngestionManager();
    
    // Delete copy constructor and assignment
    IngestionManager(const IngestionManager&) = delete;
    IngestionManager& operator=(const IngestionManager&) = delete;
    
    /**
     * @brief Register a data source for ingestion
     * @param config Source configuration
     * @return true if registration successful
     */
    bool registerSource(const SourceConfig& config);
    
    /**
     * @brief Remove a registered source
     * @param source_id Source identifier
     * @return true if source was found and removed
     */
    bool unregisterSource(const std::string& source_id);
    
    /**
     * @brief Ingest data from a specific source
     * @param source_id Source identifier
     * @param progress_callback Optional progress callback
     * @return Ingestion statistics
     */
    IngestionStats ingestSource(const std::string& source_id,
                                ProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Ingest data from all registered sources
     * @param progress_callback Optional progress callback
     * @return Overall ingestion report
     */
    IngestionReport ingestAll(ProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Get list of registered sources
     * @return Vector of source configurations
     */
    std::vector<SourceConfig> getRegisteredSources() const;
    
    /**
     * @brief Set target collection for ingested documents
     * @param collection_name Name of target collection
     */
    void setTargetCollection(const std::string& collection_name);
    
    /**
     * @brief Enable/disable parallel ingestion
     * @param enabled Whether to enable parallel processing
     * @param max_threads Maximum number of threads (0 = auto)
     */
    void setParallelProcessing(bool enabled, size_t max_threads = 0);

    /**
     * @brief Configure retry behaviour for connectors
     * @param config Retry and timeout settings
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Enable dry-run mode (scan only, no actual insertion)
     *
     * In dry-run mode `ingestAll()` / `ingestSource()` scan sources and count
     * documents but do not write anything to the target collection.
     * @param enabled true to enable dry-run mode
     */
    void setDryRun(bool enabled);

    /**
     * @brief Check whether dry-run mode is active
     */
    bool isDryRun() const;

    /**
     * @brief Get all quarantined items accumulated across ingestion runs
     * @return Vector of quarantine entries
     */
    std::vector<QuarantineEntry> getQuarantineItems() const;

    /**
     * @brief Remove a specific item from the quarantine list
     * @param item_path Path or URL of the quarantined item
     * @return true if the item was found and removed
     */
    bool dismissQuarantineItem(const std::string& item_path);

    /**
     * @brief Clear the entire quarantine list
     */
    void clearQuarantine();

    /**
     * @brief Update an existing quarantine entry in-place
     *
     * Finds the entry whose `item_path` matches `updated.item_path` and
     * replaces it with the supplied value.  Used by the retry mechanism to
     * persist incremented retry counts and permanently_failed flags.
     *
     * @param updated Entry with the new field values
     * @return true if a matching entry was found and updated
     */
    bool updateQuarantineEntry(const QuarantineEntry& updated);

    /**
     * @brief Directly inject an entry into the quarantine list
     *
     * Allows callers (e.g. external importers, unit tests) to queue a
     * document for retry without going through a full ingestion run.
     *
     * @param entry The entry to add
     */
    void addToQuarantine(QuarantineEntry entry);

    /**
     * @brief Return the current retry / back-off configuration
     */
    RetryConfig getRetryConfig() const;

    /**
     * @brief Return the cumulative count of successful quarantine retries
     *
     * Counts every call to `IngestionAdminApi::retryQuarantineItem()` that
     * resulted in a successful re-write and removal from quarantine.
     */
    size_t getQuarantineRetrySuccessCount() const;

    /**
     * @brief Increment the quarantine retry success counter by one
     *
     * Called by `IngestionAdminApi::retryQuarantineItem()` on each successful
     * per-document retry.  Exposed as a public method so that external callers
     * (e.g. admin REST handlers) can maintain accurate metrics when invoking
     * the retry logic directly.
     */
    void incrementQuarantineRetrySuccess();

    /**
     * @brief Configure per-source rate limiting
     * @param config Rate-limit settings (requests/sec, bytes/hour)
     *
     * When enabled, connectors are throttled to the configured rate.
     * Excess requests are delayed (blocking back-pressure).
     * Byte-hour quota violations emit a `QUOTA_EXCEEDED` error.
     */
    void setRateLimitConfig(const RateLimitConfig& config);

    /**
     * @brief Preview a source – fetch the first N documents without writing
     *
     * Useful for validating source configuration, inspecting content quality,
     * or estimating ingestion size before committing to a full run.
     *
     * @param source_id     Source to preview
     * @param max_documents Maximum number of document contents to return
     *                      (capped at 100 to avoid memory exhaustion)
     * @return SourcePreview containing sample documents and total count
     */
    SourcePreview previewSource(const std::string& source_id,
                                size_t max_documents = 5) const;

    /**
     * @brief Disable a registered source
     *
     * Disabled sources are skipped in future `ingestAll()` calls.
     * @param source_id Source to disable
     * @return true if source was found and disabled
     */
    bool pauseSource(const std::string& source_id);

    /**
     * @brief Re-enable a previously disabled source
     * @param source_id Source to re-enable
     * @return true if source was found and re-enabled
     */
    bool resumeSource(const std::string& source_id);

    // ── Checkpoint / incremental ingestion ───────────────────────────────────

    /**
     * @brief Configure the directory used for persistent checkpoints
     *
     * When set, `ingestSource()` and `ingestAll()` automatically read/write
     * checkpoint files so that runs can be resumed after interruption.
     * The directory must already exist.
     *
     * @param checkpoint_dir Absolute path to an existing directory
     */
    void setCheckpointDir(const std::string& checkpoint_dir);

    /**
     * @brief Enable or disable incremental (resume-on-restart) mode
     *
     * When enabled and a checkpoint directory has been set, ingestion of a
     * source skips documents up to the stored `processed_count` offset and
     * resumes from there.  A new checkpoint is written after each successful
     * source run.
     *
     * @param enabled true to enable incremental mode
     */
    void enableIncrementalMode(bool enabled);

    /**
     * @brief Check whether incremental mode is active
     */
    bool isIncrementalMode() const;

    /**
     * @brief Read the current checkpoint for a source (if any)
     *
     * @param source_id Source to query
     * @param out       Populated on success
     * @return true if a checkpoint exists and was read
     */
    bool getCheckpoint(const std::string& source_id,
                       IngestionCheckpoint& out) const;

    /**
     * @brief Delete the stored checkpoint for a source
     *
     * Call this to force a full re-ingest on the next run.
     * @return true if a checkpoint existed and was removed
     */
    bool clearCheckpoint(const std::string& source_id);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Interface for source connectors
 * 
 * Base interface for implementing custom data source connectors
 */
class ISourceConnector {
public:
    virtual ~ISourceConnector() = default;
    
    /**
     * @brief Initialize the connector
     * @param config Source configuration
     * @return true if initialization successful
     */
    virtual bool initialize(const SourceConfig& config) = 0;
    
    /**
     * @brief Check if source is available
     * @return true if source can be accessed
     */
    virtual bool isAvailable() const = 0;
    
    /**
     * @brief Get total number of documents available
     * @return Document count (0 if unknown)
     */
    virtual size_t getDocumentCount() const = 0;
    
    /**
     * @brief Ingest documents from source
     * @param target_collection Target collection name
     * @param progress_callback Progress callback
     * @return Ingestion statistics
     */
    virtual IngestionStats ingest(const std::string& target_collection,
                                  ProgressCallback progress_callback) = 0;
};

// ============================================================================
// IngestionMetricsExporter
// ============================================================================

/**
 * @brief Prometheus-compatible text-format metrics exporter for the ingestion module.
 *
 * Converts an `IngestionReport` to the Prometheus text exposition format
 * (https://prometheus.io/docs/instrumenting/exposition_formats/).
 *
 * Usage (without requiring prometheus-cpp at the call site):
 * @code
 * IngestionMetricsExporter exporter;
 * exporter.setPrefix("themis_ingestion");
 * std::string prom_text = exporter.exportText(report);
 * // Write prom_text to HTTP /metrics endpoint
 * @endcode
 */
class IngestionMetricsExporter {
public:
    IngestionMetricsExporter() = default;

    /**
     * @brief Set the metric name prefix (default: "themis_ingestion")
     */
    void setPrefix(const std::string& prefix) { prefix_ = prefix; }

    /**
     * @brief Export an IngestionReport as Prometheus text
     * @param report The report to export
     * @return Prometheus text exposition format string
     */
    std::string exportText(const IngestionReport& report) const;

    /**
     * @brief Export a single IngestionStats as Prometheus text
     *
     * Labels included: `source_id`, `source_type` (if provided).
     * Error breakdown by code emitted as `errors_by_code_total`.
     *
     * @param stats       The stats to export
     * @param source_id   Label value for the source_id label
     * @param source_type Optional label value for the source_type label
     *                    (e.g., "FILESYSTEM", "HUGGINGFACE")
     * @return Prometheus text exposition format string
     */
    std::string exportText(const IngestionStats& stats,
                           const std::string& source_id,
                           const std::string& source_type = "") const;

private:
    std::string prefix_ = "themis_ingestion";
};

// ============================================================================
// IngestionBuilder – fluent API for configuring IngestionManager
// ============================================================================

/**
 * @brief Fluent builder for constructing and configuring an IngestionManager.
 *
 * Provides a convenient, chainable API to register sources and configure
 * behaviour before running ingestion.
 *
 * Example usage:
 * @code
 * auto mgr = IngestionBuilder("my_db")
 *     .withHuggingFaceSource("hf_legal", "lexlms/ger_legal_data",
 *                             {{"split","train"},{"streaming","true"}})
 *     .withFilesystemSource("custom_docs", "/mnt/documents",
 *                            {{"recursive","true"}})
 *     .withRetryConfig({.max_attempts=5, .initial_delay_ms=200.0})
 *     .withParallelProcessing(true, 4)
 *     .withTargetCollection("legal_documents")
 *     .build();
 *
 * auto report = mgr->ingestAll();
 * @endcode
 */
class IngestionBuilder {
public:
    /**
     * @brief Construct builder targeting the specified database connection
     * @param db_connection Database connection string or handle
     */
    explicit IngestionBuilder(const std::string& db_connection);

    ~IngestionBuilder();

    // Non-copyable, movable
    IngestionBuilder(const IngestionBuilder&) = delete;
    IngestionBuilder& operator=(const IngestionBuilder&) = delete;
    IngestionBuilder(IngestionBuilder&&) noexcept;
    IngestionBuilder& operator=(IngestionBuilder&&) noexcept;

    /**
     * @brief Register a HuggingFace dataset source
     * @param source_id Unique source identifier
     * @param dataset   Dataset name (e.g., "lexlms/ger_legal_data")
     * @param options   Optional key/value options (split, streaming, token, …)
     * @param priority  Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withHuggingFaceSource(
        const std::string& source_id,
        const std::string& dataset,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a filesystem source
     * @param source_id Unique source identifier
     * @param path      Filesystem path (file or directory)
     * @param options   Optional key/value options (recursive, format, …)
     * @param priority  Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withFilesystemSource(
        const std::string& source_id,
        const std::string& path,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a REST API source (supports cursor and offset/limit pagination)
     *
     * Registers a `GenericApiConnector` source.  Pagination behaviour is
     * controlled through the `options` map:
     *
     * | Key                     | Description                                        | Default       |
     * |-------------------------|----------------------------------------------------|---------------|
     * | `api_key`               | Bearer token for `Authorization: Bearer <token>`   | (none)        |
     * | `page_size`             | Items per page                                     | `100`         |
     * | `pagination_mode`       | `"offset"` (numeric) or `"cursor"` (opaque token)  | `"offset"`    |
     * | `cursor_param`          | Query-parameter name for the cursor / offset       | `"offset"`    |
     * | `cursor_response_field` | JSON key in response containing the next cursor    | `"next_cursor"`|
     * | `text_field`            | JSON key whose value is the document text          | `"text"`      |
     * | `max_pages`             | Maximum pages to fetch (0 = unlimited)             | `0`           |
     *
     * @param source_id Unique source identifier
     * @param endpoint  Base URL of the REST API endpoint
     * @param options   Optional key/value options (see table above)
     * @param priority  Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withApiSource(
        const std::string& source_id,
        const std::string& endpoint,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Set retry configuration
     * @return *this for chaining
     */
    IngestionBuilder& withRetryConfig(const RetryConfig& config);

    /**
     * @brief Set rate-limit configuration
     * @return *this for chaining
     */
    IngestionBuilder& withRateLimitConfig(const RateLimitConfig& config);

    /**
     * @brief Enable parallel processing
     * @param enabled     Whether to use parallel ingestion
     * @param max_threads Max concurrent sources (0 = hardware_concurrency)
     * @return *this for chaining
     */
    IngestionBuilder& withParallelProcessing(bool enabled,
                                              size_t max_threads = 0);

    /**
     * @brief Set the target collection name
     * @return *this for chaining
     */
    IngestionBuilder& withTargetCollection(const std::string& collection);

    /**
     * @brief Enable dry-run mode (no actual writes)
     * @return *this for chaining
     */
    IngestionBuilder& withDryRun(bool enabled = true);

    /**
     * @brief Build and return the configured IngestionManager
     * @return Unique pointer to the fully configured manager
     */
    std::unique_ptr<IngestionManager> build();

private:
    struct Opts {
        std::string db_connection;
        std::vector<SourceConfig> sources;
        RetryConfig retry_config;
        RateLimitConfig rate_limit_config;
        bool parallel_enabled = false;
        size_t max_threads = 0;
        std::string target_collection = "legal_documents";
        bool dry_run = false;
    };
    std::unique_ptr<Opts> opts_;
};

// ============================================================================
// IngestionAdminApi – in-process operator / admin control layer
// ============================================================================

/**
 * @brief Source status snapshot for admin queries
 */
struct SourceStatus {
    std::string source_id;
    SourceType  type;
    bool        enabled  = true;
    bool        available = false;  ///< Result of isAvailable() poll
    size_t      doc_count = 0;     ///< Last known document count (0 = unknown)
    std::chrono::system_clock::time_point last_run; ///< Last successful run time
    bool        has_last_run = false;

    SourceStatus() = default;
};

/**
 * @brief Admin / Operator API for the ingestion module
 *
 * Wraps an existing `IngestionManager` and provides operator-level
 * status, control, and quarantine management without requiring an HTTP
 * server.  An HTTP layer (e.g. crow, cpp-httplib) can expose these
 * methods as REST endpoints (Q3 3.2 final step).
 *
 * Example usage:
 * @code
 * IngestionManager mgr("db");
 * mgr.registerSource({...});
 *
 * IngestionAdminApi admin(mgr);
 * auto statuses = admin.listSources();
 * admin.startSource("my_source");
 * auto q = admin.listQuarantine();
 * admin.retryQuarantineItem("path/to/file.pdf");
 * @endcode
 */
class IngestionAdminApi {
public:
    /**
     * @brief Construct admin API around an existing IngestionManager
     * @param manager Reference to the manager to control (must outlive this object)
     */
    explicit IngestionAdminApi(IngestionManager& manager);
    ~IngestionAdminApi() = default;

    // Non-copyable
    IngestionAdminApi(const IngestionAdminApi&) = delete;
    IngestionAdminApi& operator=(const IngestionAdminApi&) = delete;

    // ── Source management ──────────────────────────────────────────────────

    /**
     * @brief List all registered sources with availability and document counts
     */
    std::vector<SourceStatus> listSources() const;

    /**
     * @brief Trigger an immediate ingestion run for a single source
     * @return Ingestion statistics from the run
     */
    IngestionStats startSource(const std::string& source_id);

    /**
     * @brief Disable a source so it is skipped in future `ingestAll()` runs
     * @return true if source was found and disabled
     */
    bool pauseSource(const std::string& source_id);

    /**
     * @brief Re-enable a previously paused source
     * @return true if source was found and re-enabled
     */
    bool resumeSource(const std::string& source_id);

    // ── Quarantine management ──────────────────────────────────────────────

    /**
     * @brief List all items currently in quarantine
     */
    std::vector<QuarantineEntry> listQuarantine() const;

    /**
     * @brief Retry a single quarantined item with exponential back-off
     *
     * When the entry has a `raw_payload`, the document is written directly
     * without re-running the whole source.  On each failed attempt the
     * back-off delay is doubled (capped at `RetryConfig::max_delay_ms`).
     * If `retry_count` reaches `RetryConfig::max_quarantine_retries` the
     * entry is marked `permanently_failed` and excluded from future retries.
     *
     * If `raw_payload` is empty (legacy entry) the method falls back to
     * re-running the originating source from the last checkpoint.
     *
     * @param item_path The quarantined item path/URL
     * @return true if the item was successfully re-ingested and removed from
     *         quarantine; false if not found, permanently failed, or all
     *         retry attempts were exhausted
     */
    bool retryQuarantineItem(const std::string& item_path);

    /**
     * @brief Retry all quarantined items that are not permanently failed
     *
     * Iterates the quarantine list and calls `retryQuarantineItem()` for
     * every entry whose `permanently_failed` flag is false.
     *
     * @return Number of items that were successfully re-ingested
     */
    size_t retryAllQuarantine();

    /**
     * @brief Dismiss (permanently delete) a quarantined item
     * @return true if the item was found and removed
     */
    bool dismissQuarantineItem(const std::string& item_path);

    // ── Health ──────────────────────────────────────────────────────────────

    /**
     * @brief Get a JSON-like health summary string
     *
     * Returns a compact JSON object with ingestion module health indicators:
     * - `status`:           "healthy" | "degraded" | "unhealthy"
     * - `registered_sources`: total registered source count
     * - `enabled_sources`:    enabled source count
     * - `quarantine_size`:    number of quarantined items
     *
     * @return JSON string (UTF-8, no trailing newline)
     */
    std::string healthJson() const;

private:
    IngestionManager& mgr_;
};

} // namespace ingestion
} // namespace themis
