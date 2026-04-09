/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_embedding.h                                  ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     340                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • (initial)  2026-04-09  feat(graph): add GNN/ANN embedding interface ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "index/graph_index.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>

namespace themis {
namespace graph {

/**
 * @brief Graph embedding algorithm selection.
 *
 * DE: Algorithmus-Auswahl für Graph-Embeddings.
 */
enum class EmbeddingAlgorithm {
    NODE2VEC,   ///< Node2Vec — biased random walks + Word2Vec (Grover & Leskovec, 2016)
    DEEPWALK,   ///< DeepWalk — unbiased random walks + Word2Vec (Perozzi et al., 2014)
    GNN_SAGE,   ///< GraphSAGE — inductive GNN aggregation (Hamilton et al., 2017)
    GNN_GCN,    ///< Graph Convolutional Network — spectral convolution (Kipf & Welling, 2017)
    LINE        ///< LINE — first/second-order proximity (Tang et al., 2015)
};

/**
 * @brief Configuration for graph embedding training.
 *
 * All fields are validated in GraphEmbedding::train(); invalid values cause
 * an std::invalid_argument exception to be thrown before any computation begins.
 */
struct EmbeddingConfig {
    /// Algorithm to use for generating embeddings.
    EmbeddingAlgorithm algorithm = EmbeddingAlgorithm::NODE2VEC;

    /// Output embedding dimensionality. Must be in [2, 4096].
    uint32_t dimensions = 128;

    /// Random-walk length per starting node (Node2Vec / DeepWalk).
    /// Must be >= 1.
    uint32_t walk_length = 80;

    /// Number of random walks originating from each node.
    /// Must be >= 1.
    uint32_t num_walks = 10;

    /// Node2Vec return parameter p (controls likelihood of returning to previous node).
    /// Higher values make walks more DFS-like. Must be > 0.
    double return_param_p = 1.0;

    /// Node2Vec in-out parameter q (controls exploration vs. exploitation).
    /// q < 1 → BFS-like (local), q > 1 → DFS-like (global). Must be > 0.
    double in_out_param_q = 1.0;

    /// Skip-gram window size for Word2Vec optimisation step.
    /// Must be in [1, 20].
    uint32_t window_size = 5;

    /// Number of training epochs for the Word2Vec / GNN optimiser.
    /// Must be >= 1.
    uint32_t num_epochs = 5;

    /// Learning rate for the optimiser. Must be in (0, 1].
    double learning_rate = 0.025;

    /// Optional graph_id scope — only nodes belonging to this graph are embedded.
    /// Empty string means all nodes in the GraphIndexManager.
    std::string graph_id;
};

/**
 * @brief Result of a link-prediction query for a single candidate edge.
 */
struct LinkPrediction {
    /// Source vertex identifier.
    std::string from_vertex;
    /// Candidate target vertex identifier.
    std::string to_vertex;
    /// Estimated probability that an edge from→to should exist. Range [0, 1].
    double probability{0.0};
    /// Cosine similarity between the two node embeddings.
    double similarity{0.0};
};

/**
 * @brief Result of a single-node classification query.
 */
struct NodeClassification {
    /// Node identifier.
    std::string node_id;
    /// Predicted class label (user-defined category string).
    std::string predicted_label;
    /// Confidence in the prediction. Range [0, 1].
    double confidence{0.0};
};

/**
 * @brief Training status reported after a completed or failed training run.
 */
struct EmbeddingTrainingResult {
    /// True if training completed without errors.
    bool success{false};
    /// Number of nodes embedded.
    size_t nodes_embedded{0};
    /// Final training loss (algorithm-specific; lower = better).
    double final_loss{0.0};
    /// Wall-clock training duration in milliseconds.
    int64_t training_duration_ms{0};
    /// Human-readable status or error message.
    std::string message;
};

/**
 * @brief Abstract interface for graph embedding providers.
 *
 * Implementors provide node embedding generation, link prediction, and
 * node classification over a property graph managed by GraphIndexManager.
 *
 * Thread-safety: train() is not thread-safe. getNodeEmbedding(),
 * predictLinks(), and classifyNode() are safe to call concurrently after
 * training has completed (isReady() returns true).
 */
class IGraphEmbeddingProvider {
public:
    virtual ~IGraphEmbeddingProvider() = default;

    /**
     * @brief Train embeddings for all nodes in the configured graph.
     *
     * Must be called before any getNodeEmbedding() / predictLinks() /
     * classifyNode() calls.  Throws std::invalid_argument for invalid config.
     *
     * @param config Training configuration.
     * @return Training result including success flag and diagnostics.
     */
    virtual EmbeddingTrainingResult train(const EmbeddingConfig& config) = 0;

    /**
     * @brief Retrieve the embedding vector for a node.
     *
     * @param node_id Vertex primary key.
     * @return Float vector of length dimensions(), or empty if node unknown.
     */
    virtual std::vector<float> getNodeEmbedding(const std::string& node_id) const = 0;

    /**
     * @brief Predict the top-k most likely edges from a given node.
     *
     * Uses the cosine similarity between node embeddings to score candidates.
     * Existing edges are excluded from the result.
     *
     * @param node_id Source vertex primary key.
     * @param k       Maximum number of candidate edges to return.
     * @return Vector of LinkPrediction structs sorted by probability descending.
     */
    virtual std::vector<LinkPrediction> predictLinks(
        const std::string& node_id,
        int k) const = 0;

    /**
     * @brief Classify a node using a pre-trained label mapping.
     *
     * The label mapping is a callback supplied at construction that maps
     * an embedding vector to a (label, confidence) pair.
     *
     * @param node_id    Vertex primary key.
     * @param model_hint Optional hint string forwarded to the classifier.
     * @return NodeClassification result (predicted_label = "" when unavailable).
     */
    virtual NodeClassification classifyNode(
        const std::string& node_id,
        const std::string& model_hint = "") const = 0;

    /**
     * @brief Compute the cosine similarity between two node embeddings.
     *
     * @return Similarity in [-1, 1], or 0.0 if either node is unknown.
     */
    virtual double similarity(
        const std::string& node_a,
        const std::string& node_b) const = 0;

    /// @return True if training has been completed successfully.
    virtual bool isReady() const = 0;

    /// @return Number of nodes for which embeddings are available.
    virtual size_t nodeCount() const = 0;

    /// @return Dimensionality of each embedding vector (0 before training).
    virtual uint32_t dimensions() const = 0;

    /// @return A copy of the config used for the last successful training run.
    virtual EmbeddingConfig config() const = 0;
};

/**
 * @brief Callable type for user-supplied node classifiers.
 *
 * The function receives the embedding vector and an optional model hint,
 * and returns a (label, confidence) pair.
 */
using NodeClassifierFn = std::function<
    std::pair<std::string, double>(
        const std::vector<float>& embedding,
        const std::string& model_hint)>;

/**
 * @brief Concrete graph embedding implementation.
 *
 * Implements Node2Vec and DeepWalk via random-walk generation and
 * a skip-gram negative-sampling optimiser.  GNN_SAGE, GNN_GCN, and LINE
 * fall back to the Node2Vec walk-based approach with adjusted walk parameters.
 * For production GNN inference, provide a custom IGraphEmbeddingProvider.
 *
 * Usage:
 * @code
 *   GraphEmbedding emb(graph_manager);
 *   EmbeddingConfig cfg;
 *   cfg.algorithm  = EmbeddingAlgorithm::NODE2VEC;
 *   cfg.dimensions = 64;
 *   cfg.graph_id   = "social";
 *
 *   auto result = emb.train(cfg);
 *   if (result.success) {
 *       auto vec = emb.getNodeEmbedding("user_A");
 *       auto links = emb.predictLinks("user_A", 10);
 *   }
 * @endcode
 */
class GraphEmbedding : public IGraphEmbeddingProvider {
public:
    /**
     * @brief Construct with a GraphIndexManager.
     *
     * @param graph_mgr   Graph to embed. Must outlive this object.
     * @param classifier  Optional classifier callback for classifyNode().
     */
    explicit GraphEmbedding(
        GraphIndexManager& graph_mgr,
        NodeClassifierFn classifier = nullptr);

    ~GraphEmbedding() override = default;

    // Non-copyable, movable
    GraphEmbedding(const GraphEmbedding&) = delete;
    GraphEmbedding& operator=(const GraphEmbedding&) = delete;
    GraphEmbedding(GraphEmbedding&&) noexcept = default;
    GraphEmbedding& operator=(GraphEmbedding&&) noexcept = default;

    EmbeddingTrainingResult train(const EmbeddingConfig& config) override;
    std::vector<float> getNodeEmbedding(const std::string& node_id) const override;
    std::vector<LinkPrediction> predictLinks(const std::string& node_id, int k) const override;
    NodeClassification classifyNode(
        const std::string& node_id,
        const std::string& model_hint = "") const override;
    double similarity(const std::string& node_a, const std::string& node_b) const override;
    bool isReady() const override;
    size_t nodeCount() const override;
    uint32_t dimensions() const override;
    EmbeddingConfig config() const override;

    // ── Static helpers ──────────────────────────────────────────────────────

    /**
     * @brief Compute cosine similarity between two float vectors.
     *
     * Returns 0.0 when either vector is empty or their dimensions differ.
     */
    static double cosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b) noexcept;

    /**
     * @brief Compute dot-product similarity between two float vectors.
     */
    static double dotProduct(
        const std::vector<float>& a,
        const std::vector<float>& b) noexcept;

    /**
     * @brief Compute negative Euclidean distance (higher = more similar).
     */
    static double negativeEuclidean(
        const std::vector<float>& a,
        const std::vector<float>& b) noexcept;

private:
    GraphIndexManager& graph_mgr_;
    NodeClassifierFn classifier_;

    bool ready_{false};
    EmbeddingConfig config_;

    /// node_id → embedding vector
    std::unordered_map<std::string, std::vector<float>> embeddings_;
    /// Ordered node list (index → node_id, for walk generation)
    std::vector<std::string> node_index_;
    /// node_id → integer index
    std::unordered_map<std::string, size_t> node_to_idx_;

    // ── Internal helpers ────────────────────────────────────────────────────

    /// Validate config; throws std::invalid_argument on bad values.
    static void validateConfig(const EmbeddingConfig& cfg);

    /// Load all nodes in scope (respects config_.graph_id).
    std::vector<std::string> loadNodes() const;

    /// Generate one random walk of length walk_length starting from node_idx.
    std::vector<size_t> randomWalk(size_t start_idx, uint32_t length) const;

    /// Node2Vec biased random walk.
    std::vector<size_t> node2vecWalk(
        size_t start_idx,
        uint32_t length,
        double p,
        double q) const;

    /// Skip-gram negative-sampling optimiser (updates embeddings_ in place).
    void optimiseSkipGram(
        const std::vector<std::vector<size_t>>& walks,
        uint32_t window_size,
        uint32_t num_epochs,
        double learning_rate);
};

} // namespace graph
} // namespace themis
