#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

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
    std::string error_message;
    
    IngestionStats() = default;
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
