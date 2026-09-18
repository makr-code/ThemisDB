/**
 * @file training_data_iterator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "exporters/jsonl_llm_exporter.h"
#include "storage/rocksdb_wrapper.h"
#include "llm/adapter_registry.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace themis {
namespace llm {

/// Training data iterator with zero-copy streaming from RocksDB
/// Extends JSONLLMExporter for data export functionality
class TrainingDataIterator {
public:
    /**
     * @brief TBD: Describe ~TrainingDataIterator.
     * @return Return value.
     */
    virtual ~TrainingDataIterator() = default;
    /// Configuration for training data iteration
    struct Config {
        // Batch configuration
        size_t batch_size = 32;
        size_t prefetch_batches = 2;
        
        // Streaming configuration
        bool enable_zero_copy = true;
        bool shuffle_data = true;
        uint64_t random_seed = 42;
        
        // Quality filtering (reuse from JSONLLMExporter)
        exporters::JSONLLLMConfig::QualityFilter quality_filter;
        
        // Multi-model enrichment
        bool enable_graph_context = false;
        bool enable_vector_similarity = false;
        bool enable_relational_joins = false;
        
        // Progress reporting
        std::function<void(size_t processed, size_t total)> progress_callback;
        size_t progress_interval = 100;
    };
    
    /// Training sample - Single training instance
    struct TrainingSample {
        std::string instruction;     // Instruction/prompt
        std::string input;           // Optional input context
        std::string output;          // Expected output/completion
        double weight = 1.0;         // Sample importance weight
        std::map<std::string, std::string> metadata;  // Additional metadata
        
        // Multi-model enrichment
        std::optional<std::string> graph_context;     // Graph relationships
        std::optional<std::string> vector_context;    // Similar documents
        std::optional<std::string> relational_context; // Joined data
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        /**
         * @brief TBD: Describe fromJson.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static TrainingSample fromJson(const nlohmann::json& j);
    };
    
    /// Training batch - Multiple samples for batch training
    struct TrainingBatch {
        std::vector<TrainingSample> samples;
        size_t batch_id = 0;
        size_t total_batches = 0;
        
        /**
         * @brief TBD: Describe getTotalTokens.
         * @return Return value.
         */
        size_t getTotalTokens() const;  // Estimate total tokens
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief TBD: Describe TrainingDataIterator.
     * @param[in] db Input parameter.
     * @param[in] exporter Input parameter.
     * @return Return value.
     */
    explicit TrainingDataIterator(
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<exporters::JSONLLLMExporter> exporter
    );

    /**
     * @brief TBD: Describe TrainingDataIterator.
     * @param[in] db Input parameter.
     * @param[in] exporter Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TrainingDataIterator(
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<exporters::JSONLLLMExporter> exporter,
        Config config
    );
    
    // Iterator Operations
    
    /**
     * @brief Initialize iterator with AQL query @param aql_query AQL query to select training data @param metadata Adapter metadata for configuration @return true if initialization successful
     * @param[in] aql_query Input parameter.
     * @param[in] metadata Input parameter.
     * @return True on success.
     */
    bool initialize(const std::string& aql_query, const AdapterMetadata& metadata);
    
    /// Check if more batches available
    bool hasNext() const;
    
    /**
     * @brief Get next batch of training samples @return Next batch or nullopt if no more data
     * @return Return value.
     */
    std::optional<TrainingBatch> getNextBatch();
    
    /// Reset iterator to beginning
    void reset();
    
    /// Get total number of samples
    size_t getTotalSamples() const { return total_samples_; }
    
    /// Get total number of batches
    size_t getTotalBatches() const { return total_batches_; }
    
    /// Get current position
    size_t getCurrentPosition() const { return current_position_; }
    
    // Statistics
    
    struct IteratorStats {
        size_t total_samples = 0;
        size_t processed_samples = 0;
        size_t skipped_samples = 0;
        size_t total_batches = 0;
        size_t processed_batches = 0;
        double avg_samples_per_batch = 0.0;
        double avg_tokens_per_sample = 0.0;
        size_t total_tokens = 0;
        std::chrono::milliseconds iteration_time{0};
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    IteratorStats getStats() const { return stats_; }
    
    /// Get current configuration
    const Config& getConfig() const { return config_; }
    
    /// Update configuration (only before initialization)
    void setConfig(const Config& config);
    
private:
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<exporters::JSONLLLMExporter> exporter_;
    Config config_;
    
    // Iterator state
    bool initialized_ = false;
    size_t total_samples_ = 0;
    size_t total_batches_ = 0;
    size_t current_position_ = 0;
    size_t current_batch_id_ = 0;
    
    // Data buffer
    std::vector<TrainingSample> sample_buffer_;
    std::vector<size_t> sample_indices_;  // For shuffling
    
    // Statistics
    IteratorStats stats_;
    std::chrono::steady_clock::time_point start_time_;
    
    /**
     * @brief Internal helpers
     * @param[in] aql_query Input parameter.
     * @return True on success.
     */
    bool loadSamples(const std::string& aql_query);
    /**
     * @brief TBD: Describe convertToTrainingSample.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    TrainingSample convertToTrainingSample(const BaseEntity& entity);
    /**
     * @brief TBD: Describe shuffleSamples.
     */
    void shuffleSamples();
    /**
     * @brief TBD: Describe enrichSampleWithGraphContext.
     * @param[in,out] sample Input/output parameter.
     */
    void enrichSampleWithGraphContext(TrainingSample& sample);
    /**
     * @brief TBD: Describe enrichSampleWithVectorContext.
     * @param[in,out] sample Input/output parameter.
     */
    void enrichSampleWithVectorContext(TrainingSample& sample);
    /**
     * @brief TBD: Describe enrichSampleWithRelationalContext.
     * @param[in,out] sample Input/output parameter.
     */
    void enrichSampleWithRelationalContext(TrainingSample& sample);
    /**
     * @brief TBD: Describe passesQualityFilter.
     * @param[in] sample Input parameter.
     * @return True on success.
     */
    bool passesQualityFilter(const TrainingSample& sample) const;
};

/// Training query builder - Helper for constructing AQL queries
class TrainingQueryBuilder {
public:
    TrainingQueryBuilder() = default;
    
    /// Set base FROM clause
    TrainingQueryBuilder& from(const std::string& collection);
    
    /// Add WHERE condition
    TrainingQueryBuilder& where(const std::string& condition);
    
    /// Add GRAPH_CONTEXT enrichment
    TrainingQueryBuilder& withGraphContext(
        const std::vector<std::string>& relationships,
        int max_depth = 2
    );
    
    /// Add VECTOR_SIMILARITY enrichment
    TrainingQueryBuilder& withVectorSimilarity(
        const std::string& embedding_field,
        double threshold = 0.8,
        size_t top_k = 5
    );
    
    /// Add LIMIT clause
    TrainingQueryBuilder& limit(size_t max_samples);
    
    /// Build final AQL query
    std::string build() const;
    
private:
    std::string from_clause_;
    std::vector<std::string> where_conditions_;
    std::string graph_context_;
    std::string vector_similarity_;
    std::optional<size_t> limit_;
};

} // namespace llm
} // namespace themis
