/**
 * @file gnn_embeddings.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/property_graph.h"
#include "index/vector_index.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

namespace themis {


class GNNEmbeddingManager {
public:
    struct Status {
        bool ok = true;
        std::string message;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    struct EmbeddingInfo {
        std::string entity_id;       // Node PK or Edge ID
        std::string entity_type;     // "node" or "edge"
        std::string graph_id;
        std::string model_name;      // e.g., "gcn_v1", "graphsage_v2"
        int64_t timestamp;           // Generation timestamp
        std::vector<float> embedding;
    };

    struct SimilarityResult {
        std::string entity_id;
        float similarity;            // Cosine similarity (higher = more similar)
        std::string entity_type;
        std::string graph_id;
    };

    /**
     * @brief GNNEmbedding Manager.
     * @param[in,out] db Input/output parameter.
     * @param[in,out] pgm Input/output parameter.
     * @param[in,out] vim Input/output parameter.
     * @return Return value.
     */
    explicit GNNEmbeddingManager(
        RocksDBWrapper& db,
        PropertyGraphManager& pgm,
        VectorIndexManager& vim
    );

    // ===== Node Embedding Generation =====

    Status generateNodeEmbeddings(
        std::string_view graph_id,
        std::string_view label,
        std::string_view model_name,
        const std::vector<std::string>& feature_fields = {}
    );

    Status updateNodeEmbedding(
        std::string_view node_pk,
        std::string_view graph_id,
        std::string_view model_name,
        const std::vector<std::string>& feature_fields = {}
    );

    // ===== Edge Embedding Generation =====

    Status generateEdgeEmbeddings(
        std::string_view graph_id,
        std::string_view edge_type,
        std::string_view model_name,
        const std::vector<std::string>& feature_fields = {}
    );

    Status updateEdgeEmbedding(
        std::string_view edge_id,
        std::string_view graph_id,
        std::string_view model_name,
        const std::vector<std::string>& feature_fields = {}
    );

    // ===== Graph-Level Embeddings =====

    std::pair<Status, std::vector<float>> generateGraphEmbedding(
        std::string_view graph_id,
        std::string_view model_name,
        std::string_view aggregation_method = "mean"
    );

    // ===== Embedding Retrieval =====

    std::pair<Status, EmbeddingInfo> getNodeEmbedding(
        std::string_view node_pk,
        std::string_view graph_id,
        std::string_view model_name
    ) const;

    std::pair<Status, EmbeddingInfo> getEdgeEmbedding(
        std::string_view edge_id,
        std::string_view graph_id,
        std::string_view model_name
    ) const;

    // ===== Similarity Search =====

    std::pair<Status, std::vector<SimilarityResult>> findSimilarNodes(
        std::string_view node_pk,
        std::string_view graph_id,
        int k,
        std::string_view model_name
    ) const;

    std::pair<Status, std::vector<SimilarityResult>> findSimilarEdges(
        std::string_view edge_id,
        std::string_view graph_id,
        int k,
        std::string_view model_name
    ) const;

    // ===== Model Management =====

    enum class AggregationStrategy {
        MEAN_POOLING,      // Average neighbor features (default)
        MAX_POOLING,       // Max pooling across neighbors
        SUM_POOLING,       // Sum neighbor features
        ATTENTION          // Weighted aggregation (simplified)
    };

    Status registerModel(
        std::string_view model_name,
        std::string_view model_type,
        int embedding_dim,
        std::string_view config = "{}"
    );

    std::pair<Status, std::vector<std::string>> listModels() const;

    struct ModelInfo {
        std::string name = {};
        std::string type;
        int embedding_dim;
        std::string config;
        int64_t registered_at;
        AggregationStrategy aggregation = AggregationStrategy::MEAN_POOLING;
    };
    std::pair<Status, ModelInfo> getModelInfo(std::string_view model_name) const;

    /**
     * @brief Set Aggregation Strategy.
     * @param[in] model_name Name of the model.
     * @param[in] strategy Input parameter.
     * @return Return value.
     */
    Status setAggregationStrategy(
        std::string_view model_name,
        AggregationStrategy strategy
    );

    // ===== Batch Operations =====

    Status generateNodeEmbeddingsBatch(
        const std::vector<std::string>& node_pks,
        std::string_view graph_id,
        std::string_view model_name,
        size_t batch_size = 32
    );

    Status generateEdgeEmbeddingsBatch(
        const std::vector<std::string>& edge_ids,
        std::string_view graph_id,
        std::string_view model_name,
        size_t batch_size = 32
    );

    // ===== Statistics =====

    struct EmbeddingStats {
        size_t total_node_embeddings = 0;
        size_t total_edge_embeddings;
        std::unordered_map<std::string, size_t> embeddings_per_model;
        std::unordered_map<std::string, size_t> embeddings_per_graph;
    };

    std::pair<Status, EmbeddingStats> getStats() const;

private:
    RocksDBWrapper& db_;
    PropertyGraphManager& pgm_;
    VectorIndexManager& vim_;

    // Model registry
    std::unordered_map<std::string, ModelInfo> models_;

    /**
     * @brief Helper: Extract feature vector from entity fields
     * @param[in] entity Input parameter.
     * @param[in] feature_fields Input parameter.
     * @return Return value.
     */
    std::vector<float> extractFeatures_(
        const BaseEntity& entity,
        const std::vector<std::string>& feature_fields
    ) const;

    /**
     * @brief Helper: Build embedding key
     * @param[in] entity_type Input parameter.
     * @param[in] graph_id Identifier of the graph.
     * @param[in] entity_id Identifier of the entity.
     * @param[in] model_name Name of the model.
     * @return Return value.
     */
    std::string makeEmbeddingKey_(
        std::string_view entity_type,  // "node" or "edge"
        std::string_view graph_id,
        std::string_view entity_id,
        std::string_view model_name
    ) const;

    // Helper: Parse embedding key
    struct EmbeddingKeyParts {
        std::string entity_type;
        std::string graph_id;
        std::string entity_id;
        std::string model_name;
    };
    /**
     * @brief Parse Embedding Key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::optional<EmbeddingKeyParts> parseEmbeddingKey_(std::string_view key) const;

    // Helper: Compute embedding using registered model
    // For MVP: Simple feature-based embedding (mean pooling)
    // For production: Call external GNN model via Python bridge or native inference
    std::pair<Status, std::vector<float>> computeEmbedding_(
        std::string_view model_name,
        const std::vector<float>& features,
        const std::vector<std::string>& neighbor_ids,  // For GNN context
        std::string_view graph_id
    ) const;

    // Helper: Get neighbors for GNN aggregation
    std::vector<std::string> getNeighbors_(
        std::string_view node_pk,
        std::string_view graph_id,
        int hop_count = 1
    ) const;
};

} // namespace themis
