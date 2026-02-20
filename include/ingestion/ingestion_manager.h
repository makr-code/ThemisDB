#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

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
    int    max_attempts      = 3;     ///< Maximum total attempts (1 = no retry)
    double initial_delay_ms  = 500.0; ///< First back-off delay (ms)
    double backoff_factor    = 2.0;   ///< Exponential multiplier per attempt
    double max_delay_ms      = 30000.0; ///< Cap on per-attempt delay (ms)
    int    timeout_ms        = 30000; ///< Per-request timeout (ms)

    RetryConfig() = default;
};

/**
 * @brief Lightweight observability metrics collected during ingestion
 */
struct IngestionMetrics {
    size_t retry_count      = 0;  ///< Total retries across all requests
    size_t timeout_count    = 0;  ///< Requests that timed out
    size_t error_count      = 0;  ///< Total individual errors encountered
    double throughput_docs_per_sec = 0.0; ///< Documents / second

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
    std::string error_message;        ///< Primary error (for backward compatibility)
    std::vector<IngestionError> errors; ///< Structured error log
    IngestionMetrics metrics;           ///< Observability counters
    
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
 * @brief Ingestion report for all sources
 */
struct IngestionReport {
    std::unordered_map<std::string, IngestionStats> source_stats;
    size_t total_documents = 0;
    size_t total_failures = 0;
    double total_time_seconds = 0.0;
    
    IngestionReport() = default;
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

} // namespace ingestion
} // namespace themis
