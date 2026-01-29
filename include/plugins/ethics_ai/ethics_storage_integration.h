#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace themis {

// Forward declarations for ThemisDB storage managers
class RocksDBWrapper;
class VectorIndexManager;
class GraphIndexManager;

namespace plugins {
namespace ethics {

/**
 * @brief Graph Storage Integration for Ethics AI
 * 
 * Manages ethical argument relationships in the graph database:
 * - Argument chains (supports/counters relationships)
 * - Philosophy school relationships
 * - Debate participation graphs
 * - Influence networks
 */
class EthicsGraphStorage {
public:
    explicit EthicsGraphStorage(std::shared_ptr<GraphIndexManager> graph_manager);
    ~EthicsGraphStorage() = default;
    
    /**
     * @brief Store argument as graph node
     * @param argument The ethical argument
     * @return Status indicating success/failure
     */
    Status storeArgumentNode(const EthicalArgument& argument);
    
    /**
     * @brief Create edge between arguments
     * @param from_id Source argument ID
     * @param to_id Target argument ID
     * @param relationship_type "supports", "counters", "rebuts", "synthesizes"
     * @param weight Relationship strength (0.0-1.0)
     * @return Status indicating success/failure
     */
    Status createArgumentEdge(
        const std::string& from_id,
        const std::string& to_id,
        const std::string& relationship_type,
        double weight = 1.0
    );
    
    /**
     * @brief Traverse argument chain using graph algorithms
     * @param start_id Starting argument ID
     * @param max_depth Maximum traversal depth
     * @param direction "forward", "backward", "both"
     * @param algorithm "BFS", "DFS", "shortest_path"
     * @return List of argument IDs in traversal order
     */
    std::variant<std::vector<std::string>, Status> traverseArgumentChain(
        const std::string& start_id,
        size_t max_depth,
        const std::string& direction,
        const std::string& algorithm = "BFS"
    );
    
    /**
     * @brief Find paths between two arguments
     * @param start_id Starting argument ID
     * @param end_id Ending argument ID
     * @param max_paths Maximum number of paths to return
     * @return List of paths (each path is a list of argument IDs)
     */
    std::variant<std::vector<std::vector<std::string>>, Status> findArgumentPaths(
        const std::string& start_id,
        const std::string& end_id,
        size_t max_paths = 5
    );
    
    /**
     * @brief Get all arguments that support a given argument
     * @param argument_id Argument ID
     * @return List of supporting argument IDs
     */
    std::variant<std::vector<std::string>, Status> getSupportingArguments(
        const std::string& argument_id
    );
    
    /**
     * @brief Get all arguments that counter a given argument
     * @param argument_id Argument ID
     * @return List of countering argument IDs
     */
    std::variant<std::vector<std::string>, Status> getCounteringArguments(
        const std::string& argument_id
    );
    
    /**
     * @brief Calculate PageRank for arguments (influence score)
     * @param max_iterations Maximum PageRank iterations
     * @param damping_factor Damping factor (default: 0.85)
     * @return Map of argument_id -> PageRank score
     */
    std::variant<std::map<std::string, double>, Status> calculateArgumentInfluence(
        size_t max_iterations = 20,
        double damping_factor = 0.85
    );
    
private:
    std::shared_ptr<GraphIndexManager> graph_manager_;
};

/**
 * @brief Relational Storage Integration for Ethics AI
 * 
 * Manages structured ethical data in relational tables:
 * - Arguments metadata (structured queries)
 * - Decisions and outcomes
 * - Philosophy profile metadata
 * - Evaluation metrics
 * - Debate sessions
 */
class EthicsRelationalStorage {
public:
    explicit EthicsRelationalStorage(std::shared_ptr<RocksDBWrapper> storage);
    ~EthicsRelationalStorage() = default;
    
    /**
     * @brief Initialize relational schema for ethics data
     * @return Status indicating success/failure
     */
    Status initializeSchema();
    
    /**
     * @brief Store argument in relational format
     * @param argument The ethical argument
     * @return Status indicating success/failure
     */
    Status storeArgument(const EthicalArgument& argument);
    
    /**
     * @brief Query arguments using SQL-like conditions
     * @param philosophy_school Filter by philosophy (empty = all)
     * @param argument_types Filter by types (empty = all)
     * @param min_strength Minimum argument strength
     * @param limit Maximum results
     * @param order_by Order by field ("created_at", "strength")
     * @return List of matching arguments
     */
    std::variant<std::vector<EthicalArgument>, Status> queryArguments(
        const std::string& philosophy_school = "",
        const std::vector<ArgumentType>& argument_types = {},
        ArgumentStrength min_strength = ArgumentStrength::WEAK,
        size_t limit = 100,
        const std::string& order_by = "created_at"
    );
    
    /**
     * @brief Store decision with full metadata
     * @param decision The ethical decision
     * @return Status indicating success/failure
     */
    Status storeDecision(const EthicalDecision& decision);
    
    /**
     * @brief Query decisions by criteria
     * @param category Dilemma category filter
     * @param min_confidence Minimum confidence score
     * @param min_consensus Minimum consensus level
     * @param limit Maximum results
     * @return List of matching decisions
     */
    std::variant<std::vector<EthicalDecision>, Status> queryDecisions(
        const std::string& category = "",
        double min_confidence = 0.0,
        double min_consensus = 0.0,
        size_t limit = 100
    );
    
    /**
     * @brief Store evaluation result
     * @param decision_id Associated decision ID
     * @param evaluation Evaluation result
     * @return Status indicating success/failure
     */
    Status storeEvaluation(
        const std::string& decision_id,
        const EthicsEvaluationResult& evaluation
    );
    
    /**
     * @brief Get evaluation for a decision
     * @param decision_id Decision ID
     * @return Evaluation result or error
     */
    std::variant<EthicsEvaluationResult, Status> getEvaluation(
        const std::string& decision_id
    );
    
    /**
     * @brief Get aggregate statistics
     * @param philosophy_school Optional philosophy filter
     * @return Map of statistic_name -> value
     */
    std::variant<std::map<std::string, double>, Status> getStatistics(
        const std::string& philosophy_school = ""
    );
    
private:
    std::shared_ptr<RocksDBWrapper> storage_;
    
    // Helper methods for schema management
    Status createArgumentsTable();
    Status createDecisionsTable();
    Status createEvaluationsTable();
    Status createDebatesTable();
};

/**
 * @brief Vector Storage Integration for Ethics AI
 * 
 * Manages semantic embeddings for ethical reasoning:
 * - Argument content embeddings
 * - Decision text embeddings
 * - Dilemma description embeddings
 * - Semantic similarity search
 */
class EthicsVectorStorage {
public:
    explicit EthicsVectorStorage(std::shared_ptr<VectorIndexManager> vector_manager);
    ~EthicsVectorStorage() = default;
    
    /**
     * @brief Store argument with semantic embedding
     * @param argument The ethical argument
     * @param embedding Vector embedding of argument content
     * @return Status indicating success/failure
     */
    Status storeArgumentEmbedding(
        const EthicalArgument& argument,
        const std::vector<float>& embedding
    );
    
    /**
     * @brief Search for semantically similar arguments
     * @param query_embedding Query vector
     * @param philosophy_school Optional philosophy filter
     * @param top_k Number of results to return
     * @param min_similarity Minimum similarity threshold
     * @return List of (argument_id, similarity_score) pairs
     */
    std::variant<std::vector<std::pair<std::string, double>>, Status> 
    searchSimilarArguments(
        const std::vector<float>& query_embedding,
        const std::string& philosophy_school = "",
        size_t top_k = 20,
        double min_similarity = 0.65
    );
    
    /**
     * @brief Store decision embedding
     * @param decision The ethical decision
     * @param embedding Vector embedding of decision text
     * @return Status indicating success/failure
     */
    Status storeDecisionEmbedding(
        const EthicalDecision& decision,
        const std::vector<float>& embedding
    );
    
    /**
     * @brief Search for similar decisions
     * @param query_embedding Query vector
     * @param category Optional category filter
     * @param top_k Number of results to return
     * @return List of (decision_id, similarity_score) pairs
     */
    std::variant<std::vector<std::pair<std::string, double>>, Status> 
    searchSimilarDecisions(
        const std::vector<float>& query_embedding,
        const std::string& category = "",
        size_t top_k = 10
    );
    
    /**
     * @brief Find arguments with similar philosophical stance
     * @param reference_id Reference argument ID
     * @param top_k Number of results
     * @return List of similar argument IDs with scores
     */
    std::variant<std::vector<std::pair<std::string, double>>, Status> 
    findSimilarStances(
        const std::string& reference_id,
        size_t top_k = 10
    );
    
    /**
     * @brief Cluster arguments by semantic similarity
     * @param num_clusters Number of clusters to create
     * @param philosophy_school Optional filter
     * @return Map of cluster_id -> list of argument IDs
     */
    std::variant<std::map<size_t, std::vector<std::string>>, Status> 
    clusterArguments(
        size_t num_clusters,
        const std::string& philosophy_school = ""
    );
    
    /**
     * @brief Get embedding statistics
     * @return Map of statistic_name -> value
     */
    std::map<std::string, double> getEmbeddingStatistics() const;
    
private:
    std::shared_ptr<VectorIndexManager> vector_manager_;
    std::string argument_index_name_ = "ethics_arguments";
    std::string decision_index_name_ = "ethics_decisions";
};

/**
 * @brief Unified Storage Manager for Ethics AI
 * 
 * Coordinates all storage backends for the Ethics AI Plugin,
 * similar to how geospatial and timeseries features integrate.
 */
class EthicsStorageManager {
public:
    EthicsStorageManager(
        std::shared_ptr<GraphIndexManager> graph_manager,
        std::shared_ptr<RocksDBWrapper> relational_storage,
        std::shared_ptr<VectorIndexManager> vector_manager
    );
    ~EthicsStorageManager() = default;
    
    /**
     * @brief Initialize all storage backends
     * @return Status indicating success/failure
     */
    Status initialize();
    
    /**
     * @brief Store argument across all relevant backends
     * @param argument The ethical argument
     * @param embedding Optional embedding (if available)
     * @return Status indicating success/failure
     */
    Status storeArgumentMultiModel(
        const EthicalArgument& argument,
        const std::optional<std::vector<float>>& embedding = std::nullopt
    );
    
    /**
     * @brief Store decision across all relevant backends
     * @param decision The ethical decision
     * @param embedding Optional embedding
     * @return Status indicating success/failure
     */
    Status storeDecisionMultiModel(
        const EthicalDecision& decision,
        const std::optional<std::vector<float>>& embedding = std::nullopt
    );
    
    /**
     * @brief Execute complex query across multiple backends
     * @param query Query specification (JSON format)
     * @return Query results
     */
    std::variant<nlohmann::json, Status> executeComplexQuery(
        const nlohmann::json& query
    );
    
    // Accessors for individual storage managers
    EthicsGraphStorage& graph() { return *graph_storage_; }
    EthicsRelationalStorage& relational() { return *relational_storage_; }
    EthicsVectorStorage& vector() { return *vector_storage_; }
    
private:
    std::unique_ptr<EthicsGraphStorage> graph_storage_;
    std::unique_ptr<EthicsRelationalStorage> relational_storage_;
    std::unique_ptr<EthicsVectorStorage> vector_storage_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
