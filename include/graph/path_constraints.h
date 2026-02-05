#pragma once

#include "utils/expected.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_set>

namespace themis {

// Forward declaration
class GraphIndexManager;

namespace graph {

/**
 * @brief Path Constraints for advanced graph traversal queries
 * 
 * Provides constraint-based path finding in graphs, allowing users to specify
 * complex requirements for paths including:
 * - Path length constraints (min/max)
 * - Node property requirements
 * - Edge property requirements
 * - Forbidden/required nodes and edges
 * - Path uniqueness constraints
 * - Custom validation predicates
 * 
 * This is a stub implementation for GAP-006. Future implementations will
 * integrate with the graph query optimizer for efficient execution.
 * 
 * @note This is a placeholder implementation. Real algorithms to be added in future releases.
 */
class PathConstraints {
public:
    /**
     * @brief Constraint types for path validation
     */
    enum class ConstraintType {
        MIN_LENGTH,           // Minimum path length
        MAX_LENGTH,           // Maximum path length
        NODE_PROPERTY,        // Node must have specific property
        EDGE_PROPERTY,        // Edge must have specific property
        FORBIDDEN_NODE,       // Path cannot include this node
        REQUIRED_NODE,        // Path must include this node
        FORBIDDEN_EDGE,       // Path cannot include this edge
        REQUIRED_EDGE,        // Path must include this edge
        NO_CYCLES,           // Path must be acyclic
        UNIQUE_NODES,        // All nodes in path must be unique
        UNIQUE_EDGES,        // All edges in path must be unique
        CUSTOM_PREDICATE     // Custom validation function
    };

    /**
     * @brief Represents a single path constraint
     */
    struct Constraint {
        ConstraintType type;
        std::optional<int> int_value;
        std::optional<std::string> string_value;
        std::optional<std::function<bool(const std::vector<std::string>&)>> predicate;

        Constraint(ConstraintType t) : type(t) {}
        Constraint(ConstraintType t, int value) : type(t), int_value(value) {}
        Constraint(ConstraintType t, std::string value) : type(t), string_value(std::move(value)) {}
    };

    /**
     * @brief Result of a constrained path query
     */
    struct PathResult {
        std::vector<std::string> nodes;
        std::vector<std::string> edges;
        double cost = 0.0;
        bool satisfies_all_constraints = false;
        std::vector<std::string> violated_constraints;
    };

    PathConstraints() = default;
    
    /**
     * @brief Construct with GraphIndexManager for path finding
     */
    explicit PathConstraints(GraphIndexManager* graph_mgr);
    
    /**
     * @brief Set GraphIndexManager for path finding operations
     */
    void setGraphManager(GraphIndexManager* graph_mgr);

    /**
     * @brief Add a minimum path length constraint
     */
    void addMinLength(int min_length);

    /**
     * @brief Add a maximum path length constraint
     */
    void addMaxLength(int max_length);

    /**
     * @brief Add a forbidden node constraint
     */
    void addForbiddenNode(std::string_view node_id);

    /**
     * @brief Add a required node constraint
     */
    void addRequiredNode(std::string_view node_id);

    /**
     * @brief Add a forbidden edge constraint
     */
    void addForbiddenEdge(std::string_view edge_id);

    /**
     * @brief Add a required edge constraint
     */
    void addRequiredEdge(std::string_view edge_id);

    /**
     * @brief Require path to be acyclic
     */
    void requireAcyclic();

    /**
     * @brief Require all nodes in path to be unique
     */
    void requireUniqueNodes();

    /**
     * @brief Require all edges in path to be unique
     */
    void requireUniqueEdges();

    /**
     * @brief Add a custom validation predicate
     */
    void addCustomPredicate(std::function<bool(const std::vector<std::string>&)> predicate);

    /**
     * @brief Validate a path against all constraints
     * 
     * @note Stub implementation - always returns false with appropriate message
     */
    Result<bool> validatePath(const std::vector<std::string>& nodes, 
                              const std::vector<std::string>& edges) const;

    /**
     * @brief Find paths between two nodes that satisfy all constraints
     * 
     * @note Stub implementation - returns error indicating not yet implemented
     */
    Result<std::vector<PathResult>> findConstrainedPaths(
        std::string_view start_node,
        std::string_view end_node,
        int max_results = 10
    ) const;

    /**
     * @brief Get all active constraints
     */
    const std::vector<Constraint>& getConstraints() const { return constraints_; }

    /**
     * @brief Clear all constraints
     */
    void clearConstraints();

    /**
     * @brief Get human-readable description of constraints
     */
    std::string describeConstraints() const;

private:
    std::vector<Constraint> constraints_;
    std::unordered_set<std::string> forbidden_nodes_;
    std::unordered_set<std::string> required_nodes_;
    std::unordered_set<std::string> forbidden_edges_;
    std::unordered_set<std::string> required_edges_;
    GraphIndexManager* graph_mgr_ = nullptr;
};

} // namespace graph
} // namespace themis
