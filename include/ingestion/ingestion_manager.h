/**
 * @file ingestion_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=9; TODO=1, Stub=1, Unimpl=0, Mock=6, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
#include <utility>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>
#include "ingestion/semantic_validator.h"
#include "ingestion/inference_backend.h"
#include "ingestion/ingestion_quality_judge.h"
#include "ingestion/workflow_engine.h"

namespace themis {
namespace ingestion {

/**
 * @brief Source type enumeration
 */
enum class SourceType {
    HUGGINGFACE,     ///< HuggingFace datasets
    FILESYSTEM,      ///< Local file system (PDF, DOCX, etc.)
    API,             ///< REST/SOAP API (future)
    DATABASE,        ///< Legacy database exports (future)
    KAFKA,           ///< Apache Kafka consumer (librdkafka)
    OBJECT_STORAGE,  ///< S3 / GCS / Azure Blob object storage
    WEB_CRAWLER,     ///< HTTP web crawler and XML sitemap source
    CDC,             ///< Change-Data-Capture source for live database streams
    PLUGIN           ///< Third-party plugin-supplied source connector
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
    SCHEMA_VALIDATION_FAILED = 1304,

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
    /// Path to a CA bundle file (PEM) for TLS certificate verification.
    /// Empty string means use the default system CA bundle (recommended for
    /// most deployments).  Set to an explicit path when the server uses a
    /// private or self-signed CA that is not in the system bundle.
    /// @note CURLOPT_SSL_VERIFYPEER is always enabled; this field only
    ///       overrides which CA store is used for verification.
    std::string ca_bundle_path;           ///< CA bundle path; empty = system default

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
    size_t schema_violations = 0; ///< Documents rejected or warned by schema validation

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
 * @brief Return a copy of an options map with sensitive values redacted
 *
 * Keys whose names suggest they contain authentication credentials
 * (`api_key`, `token`, `oauth_client_secret`, `oauth_access_token`,
 * `oauth_refresh_token`, `password`, `secret`, `client_secret`,
 * `credentials`, `auth_token`) have their values replaced with `"***"`.
 * All other key-value pairs are copied unchanged.
 *
 * Use this helper whenever an options map is included in log messages or
 * error details to prevent accidental credential exposure in log output.
 *
 * @param options  Source options map (as stored in `SourceConfig::options`)
 * @return Copy of the map with sensitive values masked
 */
inline std::unordered_map<std::string, std::string> sanitizeOptions(
    const std::unordered_map<std::string, std::string>& options)
{
    static const char* const kSensitiveKeys[] = {
        "api_key", "token", "oauth_client_secret", "oauth_access_token",
        "oauth_refresh_token", "password", "secret", "client_secret",
        "credentials", "auth_token"
    };
    auto result = options;
    for (const char* key : kSensitiveKeys) {
        auto it = result.find(key);
        if (it != result.end() && !it->second.empty()) {
            it->second = "***";
        }
    }
    return result;
}

// ============================================================================
// Per-source schema validation
// ============================================================================

/**
 * @brief Expected type for a schema field
 */
enum class SchemaFieldType {
    ANY,     ///< No type constraint
    STRING,  ///< Must be a JSON string value
    NUMBER,  ///< Must be a JSON numeric value
    BOOLEAN, ///< Must be a JSON boolean value
    ARRAY,   ///< Must be a JSON array
    OBJECT   ///< Must be a JSON object
};

/**
 * @brief Validation rule for a single named field within a JSON document
 */
struct SchemaFieldRule {
    bool           required        = false;          ///< Field must be present
    SchemaFieldType expected_type  = SchemaFieldType::ANY; ///< Expected JSON value type
    size_t         min_length      = 0;              ///< Minimum string length (0 = no limit)
    size_t         max_length      = 0;              ///< Maximum string length (0 = no limit)
    std::string    pattern;                          ///< Regex pattern the string value must match (empty = no check)

    SchemaFieldRule() = default;
};

/**
 * @brief Per-source schema configuration for document validation
 *
 * When a `SchemaConfig` is registered for a source, every document produced
 * by that source is validated before it is written to the target collection.
 * Documents that fail validation are counted as failures and added to the
 * quarantine queue when `reject_invalid` is `true`.
 *
 * Two complementary validation layers are provided:
 *
 * 1. **Content-level**: `min_content_length`, `max_content_length`, and
 *    `required_content_pattern` check the raw document text without
 *    JSON parsing.  These apply to all document formats.
 *
 * 2. **Field-level**: `fields` maps field names to `SchemaFieldRule` objects.
 *    These checks are performed when the document can be parsed as JSON.
 *    Non-JSON documents skip field-level validation silently.
 *
 * Example – require a non-empty "text" field of at least 10 characters:
 * @code
 * SchemaConfig sc;
 * sc.min_content_length = 10;
 * sc.fields["text"] = {.required = true, .expected_type = SchemaFieldType::STRING,
 *                      .min_length = 10};
 * mgr.setSchemaConfig("my_source", sc);
 * @endcode
 */
struct SchemaConfig {
    std::unordered_map<std::string, SchemaFieldRule> fields; ///< Per-field rules (JSON docs)
    size_t min_content_length = 0;   ///< Minimum raw document length in bytes (0 = no limit)
    size_t max_content_length = 0;   ///< Maximum raw document length in bytes (0 = no limit)
    std::string required_content_pattern; ///< Regex the full document text must match (empty = no check)
    bool reject_invalid = true;      ///< If true, invalid docs are failed/quarantined; if false, only a warning is recorded

    SchemaConfig() = default;

    /** @brief Returns true when at least one validation rule is set */
    bool isEnabled() const {
        return min_content_length > 0
            || max_content_length > 0
            || !required_content_pattern.empty()
            || !fields.empty();
    }
};

/**
 * @brief A single schema violation message
 */
struct DocumentValidationViolation {
    std::string field;    ///< Field name, or empty for document-level violations
    std::string message;  ///< Human-readable description of the violation

    DocumentValidationViolation() = default;
    DocumentValidationViolation(const std::string& f, const std::string& m)
        : field(f), message(m) {}
};

/**
 * @brief Result of validating a document against a SchemaConfig
 */
struct DocumentValidationResult {
    bool is_valid = true;
    std::vector<DocumentValidationViolation> violations;

    DocumentValidationResult() = default;

    /** @brief Add a violation and mark the result invalid */
    void addViolation(const std::string& field, const std::string& message) {
        is_valid = false;
        violations.push_back({field, message});
    }

    /** @brief Return all violation messages joined by "; " */
    std::string summary() const {
        std::string out;
        for (const auto& v : violations) {
            if (!out.empty()) {
              out += "; ";
            }
            if (!v.field.empty()) { out += v.field; out += ": "; }
            out += v.message;
        }
        return out;
    }
};

/**
 * @brief Callback invoked by connectors for each document before writing.
 *
 * Receives the raw document content and returns a `DocumentValidationResult`.
 * When the result is invalid and the schema's `reject_invalid` flag is set,
 * the connector must count the document as failed and not increment
 * `documents_processed`.
 */
using DocumentValidatorFn = std::function<DocumentValidationResult(const std::string& content)>;

/**
 * @brief Function type for injecting a mock HTTP GET response in tests.
 *
 * Returns `{status_code, response_body}`.  When injected via
 * `GenericApiConnector::setHttpGetForTesting()`,
 * `HuggingFaceConnector::setHttpGetForTesting()`, or
 * `IngestionManager::setApiHttpGetForTesting()`, this function is called
 * instead of a real libcurl request.  Intended for unit tests only.
 */
using ApiHttpGetFn =
    std::function<std::pair<int, std::string>(const std::string& url,
                                              const std::string& auth)>;

/**
 * @brief Function type for injecting a mock HTTP POST response in tests.
 *
 * Returns `{status_code, response_body}`.  When injected via
 * `GenericApiConnector::setHttpPostForTesting()` or
 * `HuggingFaceConnector::setHttpPostForTesting()`, this function is called
 * for OAuth token endpoint POST requests instead of a real libcurl request.
 * Intended for unit tests only.
 */
using ApiHttpPostFn =
    std::function<std::pair<int, std::string>(const std::string& url,
                                              const std::string& body)>;

/**
 * @brief Function type for injecting a mock document write in unit tests.
 *
 * Called by `IngestionAdminApi::retryQuarantineItem()` when a per-document
 * retry is performed.  The function receives the source identifier and the
 * raw serialized payload and should return `true` on a successful write or
 * `false` to simulate a write failure so the retry / permanently-failed
 * logic can be exercised.
 *
 * Inject via `IngestionManager::setDocumentWriteForTesting()`.
 * When no function is installed the retry always succeeds (same as before).
 */
using DocumentWriteFn =
    std::function<bool(const std::string& source_id,
                       const std::string& payload)>;

/**
 * @brief OAuth 2.0 token refresh configuration for ingestion connectors
 *
 * When configured, connectors automatically refresh an expired access token
 * using the stored refresh token (RFC 6749 §6) upon receiving HTTP 401.
 * The refreshed access token is cached in `access_token` and used for all
 * subsequent requests within the same ingestion run.
 *
 * Set via `GenericApiConnector::setOAuthConfig()` or
 * `HuggingFaceConnector::setOAuthConfig()`.
 *
 * Supported `SourceConfig::options` keys (alternative to calling setOAuthConfig):
 * | Key                    | Description                                |
 * |------------------------|--------------------------------------------|
 * | `oauth_token_endpoint` | Token endpoint URL                         |
 * | `oauth_client_id`      | OAuth client ID                            |
 * | `oauth_client_secret`  | OAuth client secret                        |
 * | `oauth_refresh_token`  | Refresh token obtained during initial auth |
 * | `oauth_access_token`   | Initial access token (optional)            |
 */
struct OAuthConfig {
    std::string token_endpoint;  ///< Token endpoint URL (e.g. https://auth.example.com/token)
    std::string client_id;       ///< OAuth client ID
    std::string client_secret;   ///< OAuth client secret (empty for public clients)
    std::string refresh_token;   ///< Refresh token for RFC 6749 §6 token refresh
    std::string access_token;    ///< Current access token (updated automatically on refresh)

    OAuthConfig() = default;

    /** @brief Returns true when a token refresh can be attempted */
    bool isRefreshable() const {
        return !token_endpoint.empty() && !refresh_token.empty();
    }
};

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

// ============================================================================
// Ingestion lineage tracking (Issue #1901)
// ============================================================================

/**
 * @brief Status of a single lineage record
 */
enum class LineageStatus {
    SUCCESS,     ///< Document written to the target collection
    FAILED,      ///< Document processing failed (counted in documents_failed)
    QUARANTINED, ///< Document added to the quarantine queue
    DRY_RUN      ///< Run was in dry-run mode – no actual write occurred
};

/**
 * @brief Lineage record for a single ingestion batch (one source run)
 *
 * Captures the provenance metadata for every ingestion run so that each
 * batch of documents can be traced back to its source, connector, timestamp,
 * and the transformations applied during intake.
 *
 * One record is created per call to `IngestionManager::ingestSource()` when
 * lineage tracking is enabled.  Additionally, one record with
 * `status = LineageStatus::QUARANTINED` is appended for each item that ends
 * up in the quarantine queue during the same run.
 *
 * Records are stored in an in-memory `IngestionLineageStore` and can be
 * retrieved via `IngestionManager::getLineageRecords()`.
 *
 * @see IngestionLineageStore
 * @see IngestionManager::enableLineageTracking
 */
struct IngestionLineageRecord {
    std::string run_correlation_id;  ///< Links to `IngestionStats::correlation_id`
    std::string source_id;           ///< Source that produced the documents
    std::string connector_type;      ///< Connector label, e.g. "FILESYSTEM", "KAFKA"
    std::string connector_version;   ///< Semantic version of the connector (e.g. "1.0.0")
    std::string doc_id;              ///< Document identifier: item_path for quarantine records;
                                     ///<   "batch:<N>" for successful batch runs
    std::string ingested_at;         ///< ISO-8601 UTC timestamp of the ingestion moment
    size_t      bytes    = 0;        ///< Total bytes processed in this record
    size_t      doc_count = 0;       ///< Number of documents covered by this record (≥1)
    std::vector<std::string> transformation_steps; ///< Applied transformations, e.g.
                                     ///<   {"schema_validation", "mime_detection", "rate_limiting"}
    LineageStatus status = LineageStatus::SUCCESS; ///< Outcome of the ingestion attempt

    IngestionLineageRecord() = default;
};

/**
 * @brief Thread-safe in-memory store for ingestion lineage records
 *
 * Holds `IngestionLineageRecord` objects produced during ingestion runs.
 * The store is owned by the `IngestionManager` and populated automatically
 * when lineage tracking is enabled via `IngestionManager::enableLineageTracking()`.
 *
 * Querying is always available even if tracking is disabled; in that case the
 * store simply remains empty.
 */
class IngestionLineageStore {
public:
    IngestionLineageStore() = default;

    /// Append a lineage record (thread-safe).
    void record(IngestionLineageRecord r) {
        std::lock_guard<std::mutex> lk(mutex_);
        records_.push_back(std::move(r));
    }

    /// Return all records whose `source_id` matches (thread-safe).
    std::vector<IngestionLineageRecord> getBySource(const std::string& source_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<IngestionLineageRecord> out;
        for (const auto& r : records_) {
            if (r.source_id == source_id) {
              out.push_back(r);
            }
        }
        return out;
    }

    /// Return all records whose `run_correlation_id` matches (thread-safe).
    std::vector<IngestionLineageRecord> getByCorrelationId(const std::string& run_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<IngestionLineageRecord> out;
        for (const auto& r : records_) {
            if (r.run_correlation_id == run_id) {
              out.push_back(r);
            }
        }
        return out;
    }

    /// Return a copy of all stored records (thread-safe).
    std::vector<IngestionLineageRecord> getAll() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return records_;
    }

    /// Remove all records (thread-safe).
    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        records_.clear();
    }

    /// Number of records currently stored (thread-safe).
    size_t size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return records_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<IngestionLineageRecord> records_;
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
 * @brief Factory function type for plugin-based source connectors.
 *
 * A `ConnectorFactory` is a zero-argument callable that constructs and
 * returns a new heap-allocated `ISourceConnector` instance.  It is
 * registered with an `IngestionManager` via `registerConnectorPlugin()`.
 *
 * Example:
 * @code
 * mgr.registerConnectorPlugin("my_source", []() {
 *     return std::make_unique<MyCustomConnector>();
 * });
 * @endcode
 */
using ConnectorFactory = std::function<std::unique_ptr<ISourceConnector>()>;

// ============================================================================
// Legal ingestion pipeline configuration
// ============================================================================

/**
 * @brief Configuration for the LLM-driven legal text ingestion pipeline.
 *
 * When registered for a source via `IngestionManager::setLegalIngestionConfig()`,
 * each document ingested from that source is run through the semantic extraction
 * pipeline (deontic extraction + semantic validation + reference validation).
 * The results are accessible via `IngestionManager::getLastLegalExtractionResult()`.
 *
 * Example:
 * @code
 * LegalIngestionConfig cfg;
 * cfg.enabled                = true;
 * cfg.confidence_threshold   = 0.75;
 * cfg.validate_references    = true;
 * mgr.setLegalIngestionConfig("bimschg_source", cfg);
 * @endcode
 */
struct LegalIngestionConfig {
    bool   enabled                = false; ///< Enable semantic extraction pipeline
    double confidence_threshold   = 0.75;  ///< Minimum deontic extraction confidence
    bool   validate_references    = true;  ///< Run AgenticReferenceValidator on each doc
    bool   require_section_struct = false; ///< Reject docs without § section structure
    bool   flag_low_confidence    = true;  ///< Add warning step to lineage when confidence low

    LegalIngestionConfig() = default;

    /** @brief Returns true when the pipeline is active */
    bool isEnabled() const { return enabled; }
};

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
     * @brief Update the configuration of an already-registered source at runtime
     *
     * Atomically replaces the stored `SourceConfig` for `source_id` with
     * `new_config`.  The change takes effect on the next call to
     * `ingestSource()` or `ingestAll()` – no restart required.
     *
     * The `new_config.source_id` field is ignored; the source is always
     * identified by the `source_id` parameter so callers cannot accidentally
     * reassign an entry to a different key.
     *
     * @param source_id  Identifier of the source to update
     * @param new_config New configuration to apply
     * @return true  if the source was found and its configuration replaced
     * @return false if no source with `source_id` is registered
     */
    bool reconfigureSource(const std::string& source_id,
                           const SourceConfig& new_config);

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
     * @brief Register a per-source schema that is applied before each write.
     *
     * When a schema is set for `source_id`, every document produced by that
     * source is validated via `DocumentValidatorFn` before being written to
     * the target collection.  Documents that fail validation are counted as
     * failures; when `SchemaConfig::reject_invalid` is `true` (the default)
     * they are also added to the quarantine queue.
     *
     * Call with a default-constructed `SchemaConfig` (or `SchemaConfig{}`
     * with `isEnabled() == false`) to remove validation for a source.
     *
     * @param source_id Source whose documents the schema applies to
     * @param config    Schema rules to enforce
     */
    void setSchemaConfig(const std::string& source_id, const SchemaConfig& config);

    /**
     * @brief Retrieve the schema configuration registered for a source
     *
     * @param source_id Source identifier
     * @param out       Populated with the stored config on success
     * @return true if a schema config exists for `source_id`
     */
    bool getSchemaConfig(const std::string& source_id, SchemaConfig& out) const;

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
     * @brief Return the currently installed document write function (may be empty)
     *
     * Used internally by `IngestionAdminApi::retryQuarantineItem()` to obtain
     * the injectable write function without exposing the Impl directly.
     */
    DocumentWriteFn getDocumentWriteFn() const;

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

    /**
     * @brief Inject a mock HTTP GET function for all HTTP-based connectors (testing only)
     *
     * When set, every `GenericApiConnector` and `HuggingFaceConnector` created
     * by this manager will have the supplied function installed via
     * `setHttpGetForTesting()` before its first use.  Pass an empty
     * `ApiHttpGetFn{}` to restore real HTTP.
     */
    void setApiHttpGetForTesting(ApiHttpGetFn fn);

    /**
     * @brief Inject a mock document write function for quarantine retry tests (testing only)
     *
     * When set, `IngestionAdminApi::retryQuarantineItem()` calls this function
     * instead of assuming that every write succeeds.  This allows tests to
     * exercise both the success and failure code paths of the quarantine retry
     * loop.  Pass an empty `DocumentWriteFn{}` to restore the default
     * behaviour (always succeed).
     */
    void setDocumentWriteForTesting(DocumentWriteFn fn);

    // ── Plugin connector registry ───────────────────────────────────────────

    /**
     * @brief Register a third-party connector factory under a plugin name.
     *
     * The factory is stored in this manager's plugin registry.  Once
     * registered, a source with `type == SourceType::PLUGIN` and
     * `options["plugin_name"] == plugin_name` will be backed by a connector
     * created by invoking `factory()`.
     *
     * Re-registering an existing name overwrites the previous factory.
     *
     * @param plugin_name Unique name identifying the plugin (e.g. "my_plugin")
     * @param factory     Zero-argument callable returning a new connector
     */
    void registerConnectorPlugin(const std::string& plugin_name,
                                  ConnectorFactory factory);

    /**
     * @brief Remove a previously registered plugin factory.
     *
     * @param plugin_name Name of the plugin to remove
     * @return true if the plugin existed and was removed
     */
    bool unregisterConnectorPlugin(const std::string& plugin_name);

    /**
     * @brief List the names of all registered plugin connectors.
     *
     * @return Sorted vector of plugin names
     */
    std::vector<std::string> listConnectorPlugins() const;

    // ── Lineage tracking (Issue #1901) ──────────────────────────────────────

    /**
     * @brief Enable or disable end-to-end ingestion lineage tracking.
     *
     * When enabled, each call to `ingestSource()` appends one or more
     * `IngestionLineageRecord` objects to the internal `IngestionLineageStore`:
     *
     * - **Successful batch**: one record with `status = SUCCESS` covering all
     *   documents written in that run.
     * - **Each quarantined item**: one record with `status = QUARANTINED`
     *   carrying the item's path as `doc_id`.
     * - **Dry-run**: one record with `status = DRY_RUN`; no writes occurred.
     *
     * Lineage tracking is **disabled** by default to avoid overhead when not
     * needed.
     *
     * @param enabled true to enable, false to disable
     */
    void enableLineageTracking(bool enabled);

    /**
     * @brief Return whether lineage tracking is currently active.
     */
    bool isLineageTrackingEnabled() const;

    /**
     * @brief Return all lineage records for a specific source.
     *
     * @param source_id Source to query
     * @return Vector of lineage records (may be empty)
     */
    std::vector<IngestionLineageRecord> getLineageRecords(
        const std::string& source_id) const;

    /**
     * @brief Return all lineage records associated with an ingestion run.
     *
     * @param run_correlation_id Correlation ID from `IngestionStats::correlation_id`
     * @return Vector of lineage records (may be empty)
     */
    std::vector<IngestionLineageRecord> getLineageRecordsByRun(
        const std::string& run_correlation_id) const;

    /**
     * @brief Return all lineage records accumulated since the last clear.
     */
    std::vector<IngestionLineageRecord> getAllLineageRecords() const;

    /**
     * @brief Remove all stored lineage records.
     */
    void clearLineageRecords();

    // ── Legal ingestion pipeline (LLM-driven semantic extraction) ───────────

    /**
     * @brief Register a legal ingestion configuration for a source.
     *
     * When enabled, documents ingested from @p source_id are run through the
     * semantic extraction pipeline (deontic extraction + semantic validation +
     * optional reference validation).  The pipeline results are recorded in
     * the ingestion lineage as transformation steps.
     *
     * @param source_id  Source to configure
     * @param config     Legal ingestion configuration
     */
    void setLegalIngestionConfig(const std::string& source_id,
                                  const LegalIngestionConfig& config);

    /**
     * @brief Retrieve the legal ingestion configuration for a source.
     *
     * @param source_id  Source identifier
     * @param out        Populated with the stored config on success
     * @return true if a legal ingestion config exists for @p source_id
     */
    bool getLegalIngestionConfig(const std::string& source_id,
                                  LegalIngestionConfig& out) const;

    /**
     * @brief Run the legal extraction pipeline on a single document text.
     *
     * Applies deontic extraction, semantic validation, and (when
     * `LegalIngestionConfig::validate_references` is true) reference
     * validation to the supplied text.  The result is independent of any
     * registered source.
     *
     * Useful for ad-hoc extraction or testing the pipeline without
     * triggering a full ingestion run.
     *
     * @param document_id  Identifier for the document (used in result)
     * @param text         Raw legal document text
     * @param config       Pipeline configuration to apply
     * @return             LegalExtractionResult with provisions and quality scores
     */
    LegalExtractionResult runLegalExtraction(
        const std::string& document_id,
        const std::string& text,
        const LegalIngestionConfig& config) const;

    // ── AI backend injection (SoC / DIP) ─────────────────────────────────────

    /**
     * @brief Inject a text-generation backend into the ingestion pipeline.
     *
     * When set, `runLegalExtraction()` builds a `LegalLlmAdapter` backed by
     * this backend and injects it into the `SemanticValidator` so deontic
     * extraction uses LLM inference instead of regex.
     *
     * The ingestion module never sees a concrete LLM class — it only depends
     * on `ITextGenerationBackend` (defined in `ingestion/inference_backend.h`).
     * The `LlmIngestionBridge` (in `llm/`) is the only binding between the
     * two modules and is provided by wiring code (main / server bootstrap).
     *
     * Passing `nullptr` (or not calling this method) resets to the default
     * `NullTextGenerationBackend`, which falls back to regex extraction.
     *
     * Thread-safety: the backend pointer is stored once at startup; concurrent
     * calls to `runLegalExtraction()` are safe as long as the backend itself
     * is thread-safe (required by `ITextGenerationBackend` contract).
     *
     * Example (wiring code):
     * @code
     * #include "llm/llm_ingestion_bridge.h"
     * auto bridge = std::make_shared<LlmIngestionBridge>();
     * mgr.setTextGenerationBackend(bridge);
     * @endcode
     *
     * @param backend  Shared pointer to any `ITextGenerationBackend`.
     *                 Pass `nullptr` to fall back to `NullTextGenerationBackend`.
     */
    void setTextGenerationBackend(
        std::shared_ptr<ITextGenerationBackend> backend);

    /**
     * @brief Return the currently configured text-generation backend.
     *
     * Never returns null: if no backend has been set the result is a
     * `NullTextGenerationBackend`.
     */
    std::shared_ptr<ITextGenerationBackend> getTextGenerationBackend() const;

    /**
     * @brief Inject the workflow orchestration engine.
     *
     * When a `WorkflowEngine` is set, calls to `ingestFile()` route through
     * the YAML-driven pipeline (Stage 1–5 as described in ARCHITECTURE.md)
     * instead of the legacy `FileSystemIngester` / `runLegalExtraction()` path.
     *
     * The legacy path remains fully functional when no `WorkflowEngine` is
     * set (backward compatibility).
     *
     * Passing `nullptr` disables the workflow engine and reverts to the legacy
     * path.
     *
     * Thread-safety: the engine pointer is stored once at startup; concurrent
     * calls to `ingestFile()` are safe as long as the engine itself is
     * thread-safe (guaranteed by `WorkflowEngine`).
     *
     * @param engine  Shared pointer to a configured `WorkflowEngine`.
     *                Pass `nullptr` to revert to legacy mode.
     */
    void setWorkflowEngine(std::shared_ptr<::themis::ingestion::WorkflowEngine> engine);

    /**
     * @brief Return the currently configured workflow engine, or nullptr.
     */
    std::shared_ptr<::themis::ingestion::WorkflowEngine> getWorkflowEngine() const;

    // ---- LLM-as-judge re-ingestion quality control (v2.1) -----------------

    /**
     * @brief Attach a `ReIngestionController` for runtime quality control.
     *
     * When set, every call to `ingestFile()` (or the equivalent workflow-
     * engine path) is wrapped in the quality-judge feedback loop:
     *
     *   1. Run WorkflowEngine on the document.
     *   2. Evaluate extraction quality via the injected IngestionQualityJudge.
     *   3. If quality fails and attempts remain, re-run with targeted hints.
     *   4. Persist the best-quality extraction context.
     *
     * Pass `nullptr` to disable the quality-control loop and fall back to a
     * single-pass ingestion (legacy behaviour).
     *
     * @param controller  Configured `ReIngestionController` instance, or nullptr.
     */
    void setReIngestionController(std::shared_ptr<ReIngestionController> controller);

    /**
     * @brief Return the active `ReIngestionController`, or nullptr when unset.
     */
    std::shared_ptr<ReIngestionController> getReIngestionController() const;

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
    [[nodiscard]] virtual bool initialize(const SourceConfig& config) = 0;
    
    /**
     * @brief Check if source is available
     * @return true if source can be accessed
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;
    
    /**
     * @brief Get total number of documents available
     * @return Document count (0 if unknown)
     */
    [[nodiscard]] virtual size_t getDocumentCount() const = 0;
    
    /**
     * @brief Ingest documents from source
     * @param target_collection Target collection name
     * @param progress_callback Progress callback
     * @return Ingestion statistics
     */
    [[nodiscard]] virtual IngestionStats ingest(const std::string& target_collection,
                                  ProgressCallback progress_callback) = 0;

    /**
     * @brief Inject a per-document validator that is called before writing.
     *
     * When set, the connector calls `validator(content)` for each document
     * before counting it as processed.  If the result is invalid and the
     * source's `SchemaConfig::reject_invalid` flag is true, the document is
     * counted as failed instead of processed.
     *
     * The default implementation is a no-op (validators are opt-in per
     * connector).  Concrete connectors that support field-level document
     * validation should override this method.
     *
     * @param validator Callback to invoke per document; empty fn = disable
     */
    virtual void setDocumentValidator([[maybe_unused]] DocumentValidatorFn validator) {
        // default: no-op
    }
};

// ============================================================================
// ConnectorPluginRegistry
// ============================================================================

/**
 * @brief Thread-safe registry for third-party source connector factories.
 *
 * `ConnectorPluginRegistry` maps string plugin names to `ConnectorFactory`
 * callables.  An `IngestionManager` holds one instance; sources registered
 * with `SourceType::PLUGIN` look up their factory here at ingestion time.
 *
 * Example – register a custom connector and ingest:
 * @code
 * mgr.registerConnectorPlugin("csv_reader", []() {
 *     return std::make_unique<CsvSourceConnector>();
 * });
 *
 * mgr.registerSource({
 *     .source_id = "sales_data",
 *     .type      = SourceType::PLUGIN,
 *     .location  = "/data/sales.csv",
 *     .options   = {{"plugin_name", "csv_reader"}}
 * });
 *
 * auto report = mgr.ingestAll();
 * @endcode
 */
class ConnectorPluginRegistry {
public:
    ConnectorPluginRegistry() = default;

    /**
     * @brief Register a factory under the given plugin name.
     *
     * Re-registering an existing name silently replaces the previous factory.
     *
     * @param plugin_name Non-empty identifier for the plugin
     * @param factory     Zero-argument callable returning a new connector
     */
    void registerFactory(const std::string& plugin_name, ConnectorFactory factory);

    /**
     * @brief Remove the factory registered under @p plugin_name.
     *
     * @return true if the name was registered and has been removed
     */
    bool unregisterFactory(const std::string& plugin_name);

    /**
     * @brief Check whether a factory is registered for @p plugin_name.
     */
    bool isRegistered(const std::string& plugin_name) const;

    /**
     * @brief Invoke the factory for @p plugin_name and return a new connector.
     *
     * @return New connector instance, or nullptr if the name is not registered
     *         or the factory returns nullptr.
     */
    std::unique_ptr<ISourceConnector> create(const std::string& plugin_name) const;

    /**
     * @brief Return a sorted list of all registered plugin names.
     */
    std::vector<std::string> listPlugins() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ConnectorFactory> factories_;
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
     * @brief Register a Kafka consumer source
     *
     * Registers a `KafkaConnector` source.  Behaviour is controlled through
     * the `options` map:
     *
     * | Key                   | Description                                    | Default            |
     * |-----------------------|------------------------------------------------|--------------------|
     * | `consumer_group`      | Consumer group ID                              | `themis-ingestion` |
     * | `message_format`      | `"json"` (default) or `"avro"`                 | `json`             |
     * | `text_field`          | JSON key whose value is the document text      | `text`             |
     * | `schema_registry_url` | Avro Schema Registry URL (avro format only)    | (none)             |
     * | `poll_timeout_ms`     | Per-poll timeout in milliseconds               | `1000`             |
     * | `max_messages`        | Maximum messages per run (0 = unlimited)       | `0`                |
     * | `session_timeout_ms`  | Consumer session timeout in milliseconds       | `10000`            |
     * | `security_protocol`   | Security protocol                              | `plaintext`        |
     * | `auto_offset_reset`   | Start offset when no committed offset: `"earliest"`, `"latest"`, `"none"` | `earliest` |
     *
     * @param source_id Unique source identifier
     * @param brokers   Comma-separated broker list (e.g. `"host:9092"`)
     * @param topic     Kafka topic name to subscribe to
     * @param options   Optional key/value options (see table above)
     * @param priority  Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withKafkaSource(
        const std::string& source_id,
        const std::string& brokers,
        const std::string& topic,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register an object-storage source (S3, GCS, or Azure Blob).
     *
     * Supported `options` keys — see `ObjectStorageConnector` documentation
     * for the full table.  Key options:
     * | Key        | Description                        | Default |
     * |------------|------------------------------------|---------|
     * | `provider` | `"s3"`, `"gcs"`, or `"azure"`      | `s3`    |
     * | `prefix`   | Object key prefix filter           | (none)  |
     * | `max_keys` | Max objects to process (0=all)     | `0`     |
     *
     * @param source_id  Unique source identifier
     * @param bucket     Bucket or container name
     * @param options    Optional key/value options
     * @param priority   Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withObjectStorageSource(
        const std::string& source_id,
        const std::string& bucket,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a JDBC-compatible database source
     *
     * Registers a `DatabaseConnector` source backed by ODBC.  Behaviour is
     * controlled through the `options` map:
     *
     * | Key            | Description                                           | Default              |
     * |----------------|-------------------------------------------------------|----------------------|
     * | `query`        | SQL SELECT statement to execute                       | `SELECT * FROM <table>` |
     * | `table`        | Table name used when `query` is not set               | (required if no query) |
     * | `text_columns` | Comma-separated column names to use as document text  | (all columns as JSON) |
     * | `batch_size`   | Rows per iteration                                    | `500`                |
     * | `max_rows`     | Maximum total rows (0 = unlimited)                    | `0`                  |
     * | `username`     | Database user (never logged)                          | (from DSN)           |
     * | `password`     | Database password (never logged)                      | (from DSN)           |
     * | `driver`       | ODBC driver name override                             | (inferred from URL)  |
     * | `timeout_s`    | Login and query timeout in seconds                    | `30`                 |
     *
     * @param source_id   Unique source identifier
     * @param jdbc_url    JDBC connection URL (e.g. `jdbc:postgresql://host:5432/db`)
     * @param options     Optional key/value options (see table above)
     * @param priority    Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withDatabaseSource(
        const std::string& source_id,
        const std::string& jdbc_url,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a web crawler and sitemap source
     *
     * Registers a `WebCrawlerConnector` source.  Behaviour is controlled
     * through the `options` map:
     *
     * | Key                | Description                                          | Default   |
     * |--------------------|------------------------------------------------------|-----------|
     * | `max_depth`        | Maximum crawl depth (0 = seed URL only)              | `3`       |
     * | `max_pages`        | Maximum pages to crawl (0 = unlimited)               | `0`       |
     * | `user_agent`       | HTTP User-Agent header value                         | `ThemisDB-Crawler/1.0` |
     * | `follow_sitemaps`  | Parse XML sitemap at /sitemap.xml automatically      | `true`    |
     * | `respect_robots`   | Honour robots.txt disallow rules                     | `true`    |
     * | `same_domain_only` | Follow only URLs on the same domain as the seed      | `true`    |
     *
     * @param source_id  Unique source identifier
     * @param seed_url   Starting URL (or sitemap URL when `follow_sitemaps=true`)
     * @param options    Optional key/value options (see table above)
     * @param priority   Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withWebCrawlerSource(
        const std::string& source_id,
        const std::string& seed_url,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a CDC (Change-Data-Capture) source for live database streams
     *
     * Registers a `CdcConnector` source that consumes change events from a live
     * database replication stream (PostgreSQL logical replication, MySQL binlog,
     * or compatible CDC source) and ingests each event as a document.
     *
     * Supported `options` keys:
     * | Key               | Description                                              | Default              |
     * |-------------------|----------------------------------------------------------|----------------------|
     * | `slot_name`       | Replication slot name (PostgreSQL) or equivalent         | `themis_cdc`         |
     * | `table_filter`    | Comma-separated table names to capture (empty = all)     | (all tables)         |
     * | `operations`      | Comma-separated ops to capture: `INSERT,UPDATE,DELETE`   | `INSERT,UPDATE,DELETE` |
     * | `text_columns`    | Comma-separated columns to use as document text          | (full event JSON)    |
     * | `batch_size`      | Events per fetch batch                                   | `500`                |
     * | `max_events`      | Maximum events to consume (0 = unlimited)                | `0`                  |
     * | `poll_timeout_ms` | Poll timeout waiting for new events (milliseconds)       | `1000`               |
     * | `from_lsn`        | Start from this LSN / binlog position (empty = start)    | (from beginning)     |
     *
     * @param source_id      Unique source identifier
     * @param connection_url Database connection URL
     *                       (e.g. `postgresql://host:5432/db`)
     * @param options        Optional key/value options (see table above)
     * @param priority       Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withCdcSource(
        const std::string& source_id,
        const std::string& connection_url,
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a plugin-backed source connector.
     *
     * The source will be driven by a connector instance produced by the
     * factory previously registered under @p plugin_name (via
     * `withConnectorPlugin()` or `IngestionManager::registerConnectorPlugin()`).
     *
     * @param source_id   Unique source identifier
     * @param plugin_name Name of the registered plugin factory
     * @param location    Optional location string passed to the connector
     *                    via `SourceConfig::location`
     * @param options     Optional key/value options forwarded to the connector
     * @param priority    Source priority (default 5)
     * @return *this for chaining
     */
    IngestionBuilder& withPluginSource(
        const std::string& source_id,
        const std::string& plugin_name,
        const std::string& location = "",
        std::unordered_map<std::string, std::string> options = {},
        int priority = 5);

    /**
     * @brief Register a connector factory with the builder.
     *
     * The factory is transferred to the `IngestionManager` produced by
     * `build()`.  Call this method once per plugin before `withPluginSource()`.
     *
     * @param plugin_name Non-empty identifier for the plugin
     * @param factory     Zero-argument callable returning a new connector
     * @return *this for chaining
     */
    IngestionBuilder& withConnectorPlugin(const std::string& plugin_name,
                                           ConnectorFactory factory);

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
     * @brief Register a per-source schema validation configuration.
     *
     * Equivalent to calling `IngestionManager::setSchemaConfig(source_id, config)`
     * after `build()`.  Multiple calls with different `source_id` values are
     * supported; they accumulate independently.
     *
     * @param source_id Source whose documents the schema should validate
     * @param config    Schema rules to enforce before each write
     * @return *this for chaining
     */
    IngestionBuilder& withSchemaValidation(const std::string& source_id,
                                           const SchemaConfig& config);

    /**
     * @brief Register a legal ingestion pipeline configuration for a source.
     *
     * Equivalent to calling `IngestionManager::setLegalIngestionConfig(source_id, config)`
     * after `build()`.
     *
     * @param source_id Source whose documents the legal pipeline should process
     * @param config    Legal ingestion configuration
     * @return *this for chaining
     */
    IngestionBuilder& withLegalIngestionConfig(const std::string& source_id,
                                                const LegalIngestionConfig& config);

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
        std::unordered_map<std::string, SchemaConfig> schema_configs;
        std::unordered_map<std::string, ConnectorFactory> plugin_factories;
        std::unordered_map<std::string, LegalIngestionConfig> legal_ingestion_configs;
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

    /**
     * @brief Update the configuration of an already-registered source at runtime
     *
     * Delegates to `IngestionManager::reconfigureSource()`.  The change takes
     * effect on the next ingestion run – no restart required.
     *
     * @param source_id  Identifier of the source to update
     * @param new_config New configuration to apply
     * @return true  if the source was found and its configuration replaced
     * @return false if no source with `source_id` is registered
     */
    bool reconfigureSource(const std::string& source_id,
                           const SourceConfig& new_config);

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

