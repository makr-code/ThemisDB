/**
 * @file batch_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Batch sampling strategy
 */
enum class SamplingStrategy {
    SEQUENTIAL,      // Sequential order
    RANDOM,          // Random sampling
    STRATIFIED,      // Stratified sampling (balanced by category)
    CURRICULUM,      // Curriculum learning (easy to hard)
    WEIGHTED         // Weighted sampling by quality
};

/**
 * @brief Batch configuration
 */
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
    
    nlohmann::json toJSON() const;
    static BatchConfig fromJSON(const nlohmann::json& j);
};

/**
 * @brief A batch of training examples
 */
struct TrainingBatch {
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
    
    nlohmann::json toJSON() const;
};

/**
 * @brief Statistics about batch generation
 */
struct BatchStatistics {
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
    
    nlohmann::json toJSON() const;
};

/**
 * @brief Batch generator with prefetching and multi-model enrichment
 * 
 * Generates batches of training data from TrainingDataIterator with:
 * - Zero-copy data access from RocksDB
 * - Prefetching for performance
 * - Multiple sampling strategies
 * - Quality filtering
 * - Multi-model enrichment (Graph + Vector + Relational)
 */
class BatchGenerator {
public:
    /**
     * @brief Constructor
     * @param data_iterator Source of training data
     * @param config Batch generation configuration
     */
    BatchGenerator(
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const BatchConfig& config
    );
    
    ~BatchGenerator();
    
    /**
     * @brief Get next batch of training examples
     * @return Batch or nullopt if no more data
     */
    std::optional<TrainingBatch> nextBatch();
    
    /**
     * @brief Reset to beginning of data
     * @param reshuffle Whether to reshuffle data
     */
    void reset(bool reshuffle = true);
    
    /**
     * @brief Get total number of batches for one epoch
     */
    int getBatchCount() const;
    
    /**
     * @brief Get current batch index
     */
    int getCurrentBatchIndex() const;
    
    /**
     * @brief Check if there are more batches
     */
    bool hasNextBatch() const;
    
    /**
     * @brief Get batch generation statistics
     */
    BatchStatistics getStatistics() const;
    
    /**
     * @brief Set a filter function for examples
     * @param filter Function that returns true if example should be included
     */
    void setFilter(std::function<bool(const TrainingSample&)> filter);
    
    /**
     * @brief Enable/disable prefetching
     */
    void setPrefetchEnabled(bool enabled);
    
private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Prefetch next batch in background
    void prefetchNextBatch();
    
    // Apply sampling strategy
    std::vector<size_t> sampleIndices(size_t total_count, size_t batch_size);
    
    // Tokenize batch
    void tokenizeBatch(TrainingBatch& batch);
    
    // Apply padding
    void applyPadding(TrainingBatch& batch);
};

/**
 * @brief Factory for creating batch generators
 */
class BatchGeneratorFactory {
public:
    /**
     * @brief Create batch generator with default configuration
     */
    static std::unique_ptr<BatchGenerator> create(
        std::shared_ptr<TrainingDataIterator> data_iterator
    );
    
    /**
     * @brief Create batch generator with custom configuration
     */
    static std::unique_ptr<BatchGenerator> create(
        std::shared_ptr<TrainingDataIterator> data_iterator,
        const BatchConfig& config
    );
};

} // namespace themis::llm

