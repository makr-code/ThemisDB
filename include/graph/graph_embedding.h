/*
 * ThemisDB | File: graph_embedding.h | Version: 0.1.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 126
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4486 feat(graph): add GraphEmbed... (2026-04-09)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file graph_embedding.h
 * @brief ANN/GNN graph embedding interface for ThemisDB.
 *
 * Provides `IGraphEmbeddingProvider` and supporting types for training and
 * querying node embeddings using algorithms such as Node2Vec, GraphSAGE,
 * GAT, GCN, LINE, DeepWalk, or a custom LLM-based embedder.
 *
 * Typical usage:
 * @code
 *   GraphEmbeddingConfig cfg;
 *   cfg.algorithm  = EmbeddingAlgorithm::NODE2VEC;
 *   cfg.dimensions = 128;
 *   auto stats = provider.train(cfg);
 *   auto emb   = provider.getEmbedding("node_42");
 *   auto knn   = provider.findSimilarNodes("node_42", 10);
 * @endcode
 */

#include <string>
#include <vector>
#include <utility>

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// EmbeddingAlgorithm
// ---------------------------------------------------------------------------

/// @brief Graph embedding algorithm selector.
enum class EmbeddingAlgorithm {
    NODE2VEC,
    GRAPHSAGE,
    GAT,
    GCN,
    LINE,
    DEEPWALK,
    CUSTOM_LLM,  ///< Use an LLM to generate node-description embeddings.
};

// ---------------------------------------------------------------------------
// GraphEmbeddingConfig
// ---------------------------------------------------------------------------

/// @brief Configuration for training a graph embedding model.
struct GraphEmbeddingConfig {
    EmbeddingAlgorithm algorithm   = EmbeddingAlgorithm::NODE2VEC;
    size_t dimensions              = 128;
    int    walk_length             = 80;    ///< Random walk length (Node2Vec/DeepWalk).
    int    num_walks               = 10;    ///< Number of walks per node.
    float  p                       = 1.0f;  ///< Return parameter (Node2Vec).
    float  q                       = 1.0f;  ///< In-out parameter (Node2Vec).
    int    epochs                  = 10;
    bool   use_edge_features       = false;
    std::string custom_model_path;          ///< Path to model weights (CUSTOM_LLM).
};

// ---------------------------------------------------------------------------
// NodeEmbedding
// ---------------------------------------------------------------------------

/// @brief Embedding vector for a single graph node.
struct NodeEmbedding {
    std::string node_id;
    std::vector<float> embedding;
    double training_loss = 0.0;
};

// ---------------------------------------------------------------------------
// GraphEmbeddingStats
// ---------------------------------------------------------------------------

/// @brief Summary statistics returned after training completes.
struct GraphEmbeddingStats {
    size_t nodes_embedded    = 0;
    double training_time_ms  = 0.0;
    double final_loss        = 0.0;
    EmbeddingAlgorithm algorithm{EmbeddingAlgorithm::NODE2VEC};
};

// ---------------------------------------------------------------------------
// IGraphEmbeddingProvider
// ---------------------------------------------------------------------------

/**
 * @brief Interface for graph embedding training and nearest-node queries.
 */
class IGraphEmbeddingProvider {
public:
    virtual ~IGraphEmbeddingProvider() = default;

    /// @brief Train the embedding model over the current graph.
    [[nodiscard]] virtual GraphEmbeddingStats train(const GraphEmbeddingConfig& config) = 0;

    /// @brief Retrieve the embedding vector for a single node.
    [[nodiscard]] virtual NodeEmbedding getEmbedding(const std::string& node_id) = 0;

    /// @brief Retrieve embedding vectors for multiple nodes.
    [[nodiscard]] virtual std::vector<NodeEmbedding> getEmbeddings(
        const std::vector<std::string>& node_ids) = 0;

    /**
     * @brief Find the k nearest nodes to the given node in embedding space.
     * @return Pairs of (node_id, cosine_similarity) sorted descending by similarity.
     */
    [[nodiscard]] virtual std::vector<std::pair<std::string, float>> findSimilarNodes(
        const std::string& node_id, size_t k) = 0;

    /// @brief Return true if a model has been successfully trained.
    [[nodiscard]] virtual bool isModelTrained() const = 0;
};

} // namespace graph
} // namespace themis
