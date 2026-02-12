#pragma once

#include "ingestion_manager.h"
#include <string>
#include <memory>

namespace themis {
namespace ingestion {

/**
 * @brief HuggingFace dataset connector
 * 
 * Downloads datasets from HuggingFace Hub via REST API and ingests them
 * into ThemisDB. Supports streaming for large datasets.
 * 
 * Example usage:
 * @code
 * HuggingFaceConnector connector;
 * SourceConfig config{
 *     .source_id = "hf_legal",
 *     .type = SourceType::HUGGINGFACE,
 *     .location = "lexlms/ger_legal_data",
 *     .options = {{"split", "train"}, {"streaming", "true"}}
 * };
 * connector.initialize(config);
 * auto stats = connector.ingest("legal_documents", nullptr);
 * @endcode
 */
class HuggingFaceConnector : public ISourceConnector {
public:
    /**
     * @brief Construct HuggingFace connector
     */
    HuggingFaceConnector();
    
    ~HuggingFaceConnector() override;
    
    // Delete copy
    HuggingFaceConnector(const HuggingFaceConnector&) = delete;
    HuggingFaceConnector& operator=(const HuggingFaceConnector&) = delete;
    
    /**
     * @brief Initialize connector with configuration
     * @param config Source configuration with:
     *        - location: dataset name (e.g., "lexlms/ger_legal_data")
     *        - options["split"]: dataset split (train/test/validation)
     *        - options["streaming"]: "true" for streaming mode
     *        - options["token"]: HuggingFace API token (optional)
     *        - options["revision"]: dataset revision/branch (optional)
     * @return true if initialization successful
     */
    bool initialize(const SourceConfig& config) override;
    
    /**
     * @brief Check if HuggingFace Hub is accessible
     * @return true if dataset can be accessed
     */
    bool isAvailable() const override;
    
    /**
     * @brief Get total number of documents in dataset
     * @return Document count (may require pre-loading metadata)
     */
    size_t getDocumentCount() const override;
    
    /**
     * @brief Ingest documents from HuggingFace dataset
     * @param target_collection Target collection in ThemisDB
     * @param progress_callback Optional progress callback
     * @return Ingestion statistics
     */
    IngestionStats ingest(const std::string& target_collection,
                         ProgressCallback progress_callback) override;
    
    /**
     * @brief Set API token for authenticated access
     * @param token HuggingFace API token
     */
    void setApiToken(const std::string& token);
    
    /**
     * @brief Set batch size for ingestion
     * @param batch_size Number of documents to process per batch
     */
    void setBatchSize(size_t batch_size);
    
    /**
     * @brief Enable/disable streaming mode
     * @param enabled Whether to use streaming (recommended for large datasets)
     */
    void setStreamingMode(bool enabled);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
