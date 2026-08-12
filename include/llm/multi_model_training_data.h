/**
 * @file multi_model_training_data.h
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

/**
 * @brief Graph context enrichment result
 */
struct GraphContext {
    virtual ~GraphContext() = default;
    std::vector<std::string> related_nodes;
    std::vector<std::string> relationship_types;
    std::vector<std::string> paths;
    int depth = 0;
    
    std::string toString() const;
    nlohmann::json toJSON() const;
};

/**
 * @brief Vector similarity enrichment result
 */
struct VectorSimilarity {
    std::vector<std::string> similar_documents;
    std::vector<float> similarity_scores;
    std::vector<std::string> document_texts;
    
    std::string toString() const;
    nlohmann::json toJSON() const;
};

/**
 * @brief Relational join enrichment result
 */
struct RelationalJoin {
    std::vector<std::string> joined_fields;
    std::vector<std::string> joined_values;
    std::string join_type;
    
    std::string toString() const;
    nlohmann::json toJSON() const;
};

/**
 * @brief Enriched training example with multi-model data
 */
struct EnrichedTrainingExample {
    TrainingSample base_example;
    
    // Multi-model enrichments
    std::optional<GraphContext> graph_context;
    std::optional<VectorSimilarity> vector_similarity;
    std::optional<RelationalJoin> relational_join;
    
    /**
     * @brief Combine all enrichments into a single context string
     * @param format Format template (e.g., "Graph: {graph}\nVector: {vector}")
     */
    std::string getCombinedContext(const std::string& format = "default") const;
    
    /**
     * @brief Get enriched instruction with context
     */
    std::string getEnrichedInstruction() const;
    
    nlohmann::json toJSON() const;
};

/**
 * @brief Configuration for multi-model enrichment
 */
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
    
    nlohmann::json toJSON() const;
    static MultiModelEnrichmentConfig fromJSON(const nlohmann::json& j);
};

/**
 * @brief Statistics about multi-model enrichment
 */
struct EnrichmentStatistics {
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
    
    nlohmann::json toJSON() const;
};

/**
 * @brief Multi-model training data fusion engine
 * 
 * Enriches training examples with data from:
 * - Graph database (relationships, paths, connected nodes)
 * - Vector database (similar documents, embeddings)
 * - Relational database (joined data, foreign keys)
 * 
 * This creates richer training examples by combining multiple data sources.
 */
class MultiModelTrainingData {
public:
    /**
     * @brief Constructor
     * @param base_iterator Base training data iterator
     * @param config Enrichment configuration
     */
    MultiModelTrainingData(
        std::shared_ptr<TrainingDataIterator> base_iterator,
        const MultiModelEnrichmentConfig& config
    );
    
    ~MultiModelTrainingData();
    
    /**
     * @brief Get next enriched training example
     * @return Enriched example or nullopt if no more data
     */
    std::optional<EnrichedTrainingExample> nextExample();
    
    /**
     * @brief Get batch of enriched examples
     * @param batch_size Number of examples to return
     * @return Vector of enriched examples
     */
    std::vector<EnrichedTrainingExample> nextBatch(int batch_size);
    
    /**
     * @brief Reset to beginning
     */
    void reset();
    
    /**
     * @brief Get enrichment statistics
     */
    EnrichmentStatistics getStatistics() const;
    
    /**
     * @brief Set graph context provider
     */
    void setGraphProvider(std::shared_ptr<GraphContextProvider> provider);
    
    /**
     * @brief Set vector similarity provider
     */
    void setVectorProvider(std::shared_ptr<VectorSimilarityProvider> provider);
    
    /**
     * @brief Set relational join provider
     */
    void setRelationalProvider(std::shared_ptr<RelationalJoinProvider> provider);
    
private:
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Enrich single example
    EnrichedTrainingExample enrichExample(const TrainingSample& example);
    
    // Individual enrichment methods
    std::optional<GraphContext> enrichWithGraph(const TrainingSample& example);
    std::optional<VectorSimilarity> enrichWithVector(const TrainingSample& example);
    std::optional<RelationalJoin> enrichWithRelational(const TrainingSample& example);
};

/**
 * @brief Graph context provider interface
 */
class GraphContextProvider {
public:
    virtual ~GraphContextProvider() = default;
    
    /**
     * @brief Get graph context for a document/entity
     * @param entity_id ID of the entity to get context for
     * @param relationships Types of relationships to traverse
     * @param max_depth Maximum depth to traverse
     * @return Graph context
     */
    virtual GraphContext getContext(
        const std::string& entity_id,
        const std::vector<std::string>& relationships,
        int max_depth
    ) = 0;
};

/**
 * @brief Vector similarity provider interface
 */
class VectorSimilarityProvider {
public:
    virtual ~VectorSimilarityProvider() = default;
    
    /**
     * @brief Find similar documents using vector similarity
     * @param query_embedding Query embedding vector
     * @param threshold Minimum similarity threshold
     * @param top_k Number of results to return
     * @return Vector similarity results
     */
    virtual VectorSimilarity findSimilar(
        const std::vector<float>& query_embedding,
        float threshold,
        int top_k
    ) = 0;
};

/**
 * @brief Relational join provider interface
 */
class RelationalJoinProvider {
public:
    virtual ~RelationalJoinProvider() = default;
    
    /**
     * @brief Perform relational join
     * @param base_table Base table name
     * @param join_tables Tables to join
     * @param join_type Type of join (INNER, LEFT, etc.)
     * @param key Join key
     * @return Relational join results
     */
    virtual RelationalJoin performJoin(
        const std::string& base_table,
        const std::vector<std::string>& join_tables,
        const std::string& join_type,
        const std::string& key
    ) = 0;
};

/**
 * @brief Factory for creating multi-model training data
 */
class MultiModelTrainingDataFactory {
public:
    /**
     * @brief Create with default configuration
     */
    static std::unique_ptr<MultiModelTrainingData> create(
        std::shared_ptr<TrainingDataIterator> base_iterator
    );
    
    /**
     * @brief Create with custom configuration
     */
    static std::unique_ptr<MultiModelTrainingData> create(
        std::shared_ptr<TrainingDataIterator> base_iterator,
        const MultiModelEnrichmentConfig& config
    );
};

} // namespace themis::llm
