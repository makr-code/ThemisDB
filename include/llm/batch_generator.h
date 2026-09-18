/**
 * @file batch_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2025 ThemisDB
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include "training_data_iterator.h"

namespace themis::llm {

enum class SamplingStrategy {
    SEQUENTIAL,      // Sequential order
    RANDOM,          // Random sampling
    STRATIFIED,      // Stratified sampling (balanced by category)
    CURRICULUM,      // Curriculum learning (easy to hard)
    WEIGHTED         // Weighted sampling by quality
};

struct BatchConfig {
    int batch_size = 4;
    int max_sequence_length = 2048;
    bool shuffle = true;
    SamplingStrategy strategy = SamplingStrategy::RANDOM;
    
    // Prefetching
    bool enable_prefetch = true;
    int prefetch_batches = 2;
    
    // Padding
    bool pad_to_max_length = false;
    std::string padding_side = "right";  // "left" or "right"
    
    // Filtering
    std::optional<float> min_quality_score;
    std::optional<int> min_length;
    std::optional<int> max_length;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static BatchConfig fromJSON(const nlohmann::json& j);
};

struct TrainingBatch {
    /**
     * @brief Training Batch.
     * @return Return value.
     */
    virtual ~TrainingBatch() = default;
    std::vector<TrainingSample> examples;
    
    // Tokenized data (prepared for model input)
    std::vector<std::vector<int>> input_ids;
    std::vector<std::vector<int>> attention_mask;
    std::vector<std::vector<int>> labels;
    
    // Batch metadata
    int batch_id = 0;
    int num_tokens = 0;
    float avg_quality_score = 0.0f;
    
    // Multi-model enrichment metadata
    std::vector<std::string> graph_contexts;
    std::vector<std::string> vector_similarities;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

struct BatchStatistics {
    /**
     * @brief Batch Statistics.
     * @return Return value.
     */
    virtual ~BatchStatistics() = default;
    int total_batches = 0;
    int total_examples = 0;
    int total_tokens = 0;
    int filtered_examples = 0;
    
    // Quality distribution
    float min_quality = 0.0f;
    float max_quality = 0.0f;
    float avg_quality = 0.0f;
    
    // Length distribution
    int min_length = 0;
    int max_length = 0;
    float avg_length = 0.0f;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

class BatchGenerator {
public:
    BatchGenerator(
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const BatchConfig& config
    );
    
    ~BatchGenerator();
    
    /**
     * @brief Next Batch.
     * @return Return value.
     */
    std::optional<TrainingBatch> nextBatch();
    
    void reset(bool reshuffle = true);
    
    /**
     * @brief Get Batch Count.
     * @return Return value.
     */
    int getBatchCount() const;
    
    /**
     * @brief Get Current Batch Index.
     * @return Return value.
     */
    int getCurrentBatchIndex() const;
    
    /**
     * @brief Has Next Batch.
     * @return True when the operation succeeds.
     */
    bool hasNextBatch() const;
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    BatchStatistics getStatistics() const;
    
    void setFilter(std::function<bool(const TrainingSample&)> filter);
    
    /**
     * @brief Set Prefetch Enabled.
     * @param[in] enabled Input parameter.
     */
    void setPrefetchEnabled(bool enabled);
    
private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Prefetch next batch in background
     */
    void prefetchNextBatch();
    
    // Apply sampling strategy
    /**
     * @brief Sample Indices.
     * @param[in] total_count Input parameter.
     * @param[in] batch_size Input parameter.
     * @return Return value.
     */
    std::vector<size_t> sampleIndices(size_t total_count, size_t batch_size);
    
    // Tokenize batch
    /**
     * @brief Tokenize Batch.
     * @param[in,out] batch Input/output parameter.
     */
    void tokenizeBatch(TrainingBatch& batch);
    
    // Apply padding
    /**
     * @brief Apply Padding.
     * @param[in,out] batch Input/output parameter.
     */
    void applyPadding(TrainingBatch& batch);
};

class BatchGeneratorFactory {
public:
    /**
     * @brief Create.
     * @param[in] data_iterator Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<BatchGenerator> create(
        std::shared_ptr<TrainingDataIterator> data_iterator
    );
    
    /**
     * @brief Create.
     * @param[in] data_iterator Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<BatchGenerator> create(
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const BatchConfig& config
    );
};

} // namespace themis::llm

