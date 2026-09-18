/**
 * @file multi_model_training_data.h
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
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "training_data_iterator.h"

namespace themis::llm {

// Forward declarations
class GraphContextProvider;
class VectorSimilarityProvider;
class RelationalJoinProvider;

struct GraphContext {
    /**
     * @brief Graph Context.
     * @return Return value.
     */
    virtual ~GraphContext() = default;
    std::vector<std::string> related_nodes;
    std::vector<std::string> relationship_types;
    std::vector<std::string> paths;
    int depth = 0;
    
    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

struct VectorSimilarity {
    std::vector<std::string> similar_documents;
    std::vector<float> similarity_scores;
    std::vector<std::string> document_texts;
    
    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

struct RelationalJoin {
    std::vector<std::string> joined_fields;
    std::vector<std::string> joined_values;
    std::string join_type;
    
    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

struct EnrichedTrainingExample {
    TrainingSample base_example;
    
    // Multi-model enrichments
    std::optional<GraphContext> graph_context;
    std::optional<VectorSimilarity> vector_similarity;
    std::optional<RelationalJoin> relational_join;
    
    std::string getCombinedContext(const std::string& format = "default") const;
    
    /**
     * @brief Get Enriched Instruction.
     * @return Return value.
     */
    std::string getEnrichedInstruction() const;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

struct MultiModelEnrichmentConfig {
    // Graph enrichment
    bool enable_graph = false;
    std::vector<std::string> graph_relationships;
    int graph_max_depth = 2;
    
    // Vector enrichment
    bool enable_vector = false;
    std::string vector_field = "embedding";
    float vector_threshold = 0.8f;
    int vector_top_k = 5;
    
    // Relational enrichment
    bool enable_relational = false;
    std::vector<std::string> join_tables;
    std::string join_type = "LEFT";
    
    // Context formatting
    std::string context_format = "default";
    int max_context_length = 512;
    
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
    static MultiModelEnrichmentConfig fromJSON(const nlohmann::json& j);
};

struct EnrichmentStatistics {
    /**
     * @brief Enrichment Statistics.
     * @return Return value.
     */
    virtual ~EnrichmentStatistics() = default;
    int total_examples = 0;
    int graph_enriched = 0;
    int vector_enriched = 0;
    int relational_enriched = 0;
    
    // Performance metrics
    double avg_graph_time_ms = 0.0;
    double avg_vector_time_ms = 0.0;
    double avg_relational_time_ms = 0.0;
    
    // Context statistics
    float avg_context_length = 0.0f;
    int max_context_length = 0;
    
    /**
     * @brief To JSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
};

class MultiModelTrainingData {
public:
    MultiModelTrainingData(
        std::shared_ptr<TrainingDataIterator> base_iterator,
        const MultiModelEnrichmentConfig& config
    );
    
    ~MultiModelTrainingData();
    
    /**
     * @brief Next Example.
     * @return Return value.
     */
    std::optional<EnrichedTrainingExample> nextExample();
    
    /**
     * @brief Next Batch.
     * @param[in] batch_size Input parameter.
     * @return Return value.
     */
    std::vector<EnrichedTrainingExample> nextBatch(int batch_size);
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    EnrichmentStatistics getStatistics() const;
    
    /**
     * @brief Set Graph Provider.
     * @param[in] provider Input parameter.
     */
    void setGraphProvider(std::shared_ptr<GraphContextProvider> provider);
    
    /**
     * @brief Set Vector Provider.
     * @param[in] provider Input parameter.
     */
    void setVectorProvider(std::shared_ptr<VectorSimilarityProvider> provider);
    
    /**
     * @brief Set Relational Provider.
     * @param[in] provider Input parameter.
     */
    void setRelationalProvider(std::shared_ptr<RelationalJoinProvider> provider);
    
private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Enrich single example
    /**
     * @brief Enrich Example.
     * @param[in] example Input parameter.
     * @return Return value.
     */
    EnrichedTrainingExample enrichExample(const TrainingSample& example);
    
    // Individual enrichment methods
    /**
     * @brief Enrich With Graph.
     * @param[in] example Input parameter.
     * @return Return value.
     */
    std::optional<GraphContext> enrichWithGraph(const TrainingSample& example);
    /**
     * @brief Enrich With Vector.
     * @param[in] example Input parameter.
     * @return Return value.
     */
    std::optional<VectorSimilarity> enrichWithVector(const TrainingSample& example);
    /**
     * @brief Enrich With Relational.
     * @param[in] example Input parameter.
     * @return Return value.
     */
    std::optional<RelationalJoin> enrichWithRelational(const TrainingSample& example);
};

class GraphContextProvider {
public:
    /**
     * @brief Graph Context Provider.
     * @return Return value.
     */
    virtual ~GraphContextProvider() = default;
    
    /**
     * @brief Get Context.
     * @param[in] entity_id Identifier of the entity.
     * @param[in] relationships Input parameter.
     * @param[in] max_depth Input parameter.
     * @return Return value.
     */
    virtual GraphContext getContext(
        const std::string& entity_id,
        const std::vector<std::string>& relationships,
        int max_depth
    ) = 0;
};

class VectorSimilarityProvider {
public:
    /**
     * @brief Vector Similarity Provider.
     * @return Return value.
     */
    virtual ~VectorSimilarityProvider() = default;
    
    /**
     * @brief Find Similar.
     * @param[in] query_embedding Input parameter.
     * @param[in] threshold Input parameter.
     * @param[in] top_k Input parameter.
     * @return Return value.
     */
    virtual VectorSimilarity findSimilar(
        const std::vector<float>& query_embedding,
        float threshold,
        int top_k
    ) = 0;
};

class RelationalJoinProvider {
public:
    /**
     * @brief Relational Join Provider.
     * @return Return value.
     */
    virtual ~RelationalJoinProvider() = default;
    
    /**
     * @brief Perform Join.
     * @param[in] base_table Input parameter.
     * @param[in] join_tables Input parameter.
     * @param[in] join_type Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    virtual RelationalJoin performJoin(
        const std::string& base_table,
        const std::vector<std::string>& join_tables,
        const std::string& join_type,
        const std::string& key
    ) = 0;
};

class MultiModelTrainingDataFactory {
public:
    /**
     * @brief Create.
     * @param[in] base_iterator Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<MultiModelTrainingData> create(
        std::shared_ptr<TrainingDataIterator> base_iterator
    );
    
    /**
     * @brief Create.
     * @param[in] base_iterator Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<MultiModelTrainingData> create(
        std::shared_ptr<TrainingDataIterator> base_iterator,
        const MultiModelEnrichmentConfig& config
    );
};

} // namespace themis::llm
