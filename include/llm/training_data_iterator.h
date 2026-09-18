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

class TrainingDataIterator {
public:
    /**
     * @brief Training Data Iterator.
     * @return Return value.
     */
    virtual ~TrainingDataIterator() = default;
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
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static TrainingSample fromJson(const nlohmann::json& j);
    };
    
    struct TrainingBatch {
        std::vector<TrainingSample> samples;
        size_t batch_id = 0;
        size_t total_batches = 0;
        
        /**
         * @brief Get Total Tokens.
         * @return Return value.
         */
        size_t getTotalTokens() const;  // Estimate total tokens
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Training Data Iterator.
     * @param[in] db Input parameter.
     * @param[in] exporter Input parameter.
     * @return Return value.
     */
    explicit TrainingDataIterator(
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<exporters::JSONLLLMExporter> exporter
    );

    /**
     * @brief Training Data Iterator.
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
     * @brief Initialize.
     * @param[in] aql_query Input parameter.
     * @param[in] metadata Input parameter.
     * @return True when the operation succeeds.
     */
    bool initialize(const std::string& aql_query, const AdapterMetadata& metadata);
    
    /**
     * @brief Has Next.
     * @return True when the operation succeeds.
     */
    bool hasNext() const;
    
    /**
     * @brief Get Next Batch.
     * @return Return value.
     */
    std::optional<TrainingBatch> getNextBatch();
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    size_t getTotalSamples() const { return total_samples_; }
    
    size_t getTotalBatches() const { return total_batches_; }
    
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
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    IteratorStats getStats() const { return stats_; }
    
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
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
    
    // Internal helpers
    /**
     * @brief Load Samples.
     * @param[in] aql_query Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadSamples(const std::string& aql_query);
    /**
     * @brief Convert To Training Sample.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    TrainingSample convertToTrainingSample(const BaseEntity& entity);
    /**
     * @brief Shuffle Samples.
     */
    void shuffleSamples();
    /**
     * @brief Enrich Sample With Graph Context.
     * @param[in,out] sample Input/output parameter.
     */
    void enrichSampleWithGraphContext(TrainingSample& sample);
    /**
     * @brief Enrich Sample With Vector Context.
     * @param[in,out] sample Input/output parameter.
     */
    void enrichSampleWithVectorContext(TrainingSample& sample);
    /**
     * @brief Enrich Sample With Relational Context.
     * @param[in,out] sample Input/output parameter.
     */
    void enrichSampleWithRelationalContext(TrainingSample& sample);
    /**
     * @brief Passes Quality Filter.
     * @param[in] sample Input parameter.
     * @return True when the operation succeeds.
     */
    bool passesQualityFilter(const TrainingSample& sample) const;
};

class TrainingQueryBuilder {
public:
    TrainingQueryBuilder() = default;
    
    /**
     * @brief From.
     * @param[in] collection Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& from(const std::string& collection);
    
    /**
     * @brief Where.
     * @param[in] condition Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& where(const std::string& condition);
    
    TrainingQueryBuilder& withGraphContext(
        const std::vector<std::string>& relationships,
        int max_depth = 2
    );
    
    TrainingQueryBuilder& withVectorSimilarity(
        const std::string& embedding_field,
        double threshold = 0.8,
        size_t top_k = 5
    );
    
    /**
     * @brief Limit.
     * @param[in] max_samples Input parameter.
     * @return Return value.
     */
    TrainingQueryBuilder& limit(size_t max_samples);
    
    /**
     * @brief Build.
     * @return Return value.
     */
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
