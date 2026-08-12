/**
 * @file path_constraints.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/expected.h"
#include "graph/ontology_manager.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_set>
#include <memory>

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
 * - Weight constraints (min/max total path weight)
 * - Custom validation predicates
 * 
 * Implemented using BFS traversal with constraint validation during graph exploration.
 * Integrates with GraphIndexManager for graph operations and GraphQueryOptimizer
 * for query planning and cost estimation.
 * All constraint types are fully implemented, including node and edge property
 * constraints validated via GraphIndexManager::getNodeField / getEdgeField.
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
        CUSTOM_PREDICATE,    // Custom validation function
        MAX_WEIGHT,          // Total path weight must not exceed threshold
        MIN_WEIGHT           // Total path weight must meet minimum threshold
    };

    /**
     * @brief Represents a single path constraint
     */
    struct Constraint {
        ConstraintType type;
        std::optional<int> int_value;
        std::optional<std::string> string_value;
        std::optional<std::function<bool(const std::vector<std::string>&)>> predicate;
        /// Property key for NODE_PROPERTY / EDGE_PROPERTY constraints.
        /// string_value holds the expected property value.
        std::optional<std::string> property_key;
        /// Threshold value for MAX_WEIGHT / MIN_WEIGHT constraints.
        std::optional<double> double_value;

        Constraint(ConstraintType t) : type(t) {}
        Constraint(ConstraintType t, int value) : type(t), int_value(value) {}
        Constraint(ConstraintType t, std::string value) : type(t), string_value(std::move(value)) {}
        /// Constructor for property constraints: type, property key, expected value
        Constraint(ConstraintType t, std::string key, std::string value)
            : type(t), string_value(std::move(value)), property_key(std::move(key)) {}
        /// Constructor for weight constraints: type, threshold
        Constraint(ConstraintType t, double threshold) : type(t), double_value(threshold) {}
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
     * @brief Require every edge in the path to carry a specific field value.
     *
     * During `findConstrainedPaths` each candidate edge is looked up in the
     * graph store and its @p field_name field is compared to @p expected_value
     * (case-sensitive string comparison). Edges that do not match are pruned.
     *
     * Example – only follow edges whose type field equals "follows":
     * @code
     *   constraints.addEdgePropertyConstraint("type", "follows");
     * @endcode
     *
     * @param field_name   Name of the edge field to check (e.g. "type", "_weight").
     * @param expected_value Expected string value for the field.
     */
    void addEdgePropertyConstraint(std::string_view field_name, std::string_view expected_value);

    /**
     * @brief Require every vertex in the path to carry a specific field value.
     *
     * During `findConstrainedPaths` each candidate next-node is looked up in the
     * graph store and its @p field_name field is compared to @p expected_value
     * (case-sensitive string comparison). Nodes that do not match are pruned.
     *
     * Example – only traverse nodes whose "country" field equals "USA":
     * @code
     *   constraints.addNodePropertyConstraint("country", "USA");
     * @endcode
     *
     * @param field_name     Name of the node field to check (e.g. "type", "country").
     * @param expected_value Expected string value for the field.
     */
    void addNodePropertyConstraint(std::string_view field_name, std::string_view expected_value);

    /**
     * @brief Require the total accumulated path weight to be at most @p max_weight.
     *
     * Edge weights are read from the "_weight" field of each edge entity (default 1.0
     * when the field is absent). Candidate states whose accumulated cost already exceeds
     * @p max_weight are pruned during BFS traversal, and complete paths are rejected
     * in `validatePath` when their `PathResult::cost` exceeds the threshold.
     *
     * @param max_weight Maximum allowable total path weight (inclusive).
     */
    void addMaxWeight(double max_weight);

    /**
     * @brief Require the total accumulated path weight to be at least @p min_weight.
     *
     * Paths whose total edge weight falls below @p min_weight are rejected during
     * final validation (the constraint cannot be used for BFS pruning because the
     * cost only increases monotonically).
     *
     * @param min_weight Minimum required total path weight (inclusive).
     */
    void addMinWeight(double min_weight);

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
     * Checks all active constraints and returns true if the path satisfies all of them.
     * Supports: MIN_LENGTH, MAX_LENGTH, FORBIDDEN_NODE, REQUIRED_NODE, FORBIDDEN_EDGE,
     * REQUIRED_EDGE, NO_CYCLES, UNIQUE_NODES, UNIQUE_EDGES, EDGE_PROPERTY,
     * NODE_PROPERTY, MAX_WEIGHT, MIN_WEIGHT, and CUSTOM_PREDICATE.
     * 
     * For EDGE_PROPERTY and NODE_PROPERTY constraints, the GraphIndexManager must be
     * set (via constructor or `setGraphManager`) so entities can be fetched.
     * 
     * @param nodes Vector of node IDs in the path
     * @param edges Vector of edge IDs in the path
     * @return Result containing true if path is valid, or error with violation details
     */
    Result<bool> validatePath(const std::vector<std::string>& nodes, 
                              const std::vector<std::string>& edges) const;

    /**
     * @brief Find paths between two nodes that satisfy all constraints
     * 
     * Uses BFS traversal to explore the graph and find paths that satisfy all active
     * constraints. Constraints are validated both during traversal (for efficiency)
     * and after path completion (for correctness). Results are sorted by path cost.
     * 
     * @param start_node Starting node ID
     * @param end_node Target node ID
     * @param max_results Maximum number of paths to return (default: 10)
     * @return Result containing vector of PathResult objects, sorted by cost on success,
     *         or an error describing the failure condition.
     * @note Possible error codes include:
     *       - ErrorRegistry::ErrorCode::INVALID_STATE if GraphIndexManager is not set
     *       - ErrorRegistry::ErrorCode::VALIDATION_FAILED if constraints are contradictory
     *       - ErrorRegistry::ErrorCode::NOT_FOUND if no paths satisfy all constraints
     */
    Result<std::vector<PathResult>> findConstrainedPaths(
        std::string_view start_node,
        std::string_view end_node,
        int max_results = 10
    ) const;

    /**
     * @brief Represents a single semantic constraint violation.
     *
     * Returned by `validateSemanticPath()` when an edge in the discovered path
     * violates the ontology-declared relationship axioms.
     */
    struct ConstraintViolation {
        /// The edge identifier that triggered the violation.
        std::string edge_id;
        /// Concept class of the source node of the violating edge.
        std::string source_class;
        /// Concept class of the target node of the violating edge.
        std::string target_class;
        /// The edge type that was found (not permitted by the ontology).
        std::string edge_type;
        /// Human-readable description of the violation.
        std::string description;
    };

    /**
     * @brief Attach an ontology-based semantic constraint to this path query.
     *
     * After this call, every edge explored during `findConstrainedPaths` is
     * validated against @p ontology using the prune-first strategy: edges
     * whose type is not permitted for the (sourceClass, targetClass) pair
     * are pruned at the BFS frontier level, avoiding generation of invalid
     * path candidates.
     *
     * `validateSemanticPath()` can be used post-discovery for a full
     * structural check.
     *
     * @param ontology Non-owning pointer to a built `OntologyManager`.
     *                 Must outlive this `PathConstraints` object.
     * @param ruleset  `STRICT` (default) rejects violating paths; `WARN`
     *                 records violations but keeps the path.
     */
    void addSemanticConstraint(const OntologyManager* ontology,
                               OntologyManager::Ruleset ruleset = OntologyManager::Ruleset::Strict);

    /**
     * @brief Validate a discovered path against all attached ontology constraints.
     *
     * Iterates over every edge in @p result and calls
     * `ontology->isEdgeTypeAllowed(srcClass, tgtClass, edgeType)`.  The
     * node-class lookup uses GraphIndexManager::getNodeField("_class") and
     * defaults to the empty string (unconstrained) when the field is absent.
     *
     * @param result   A path result previously returned by `findConstrainedPaths`.
     * @return Vector of violations; empty means the path is semantically valid.
     */
    std::vector<ConstraintViolation> validateSemanticPath(const PathResult& result) const;

    /**
     * @brief Return all violations recorded during the last `findConstrainedPaths` call.
     */
    const std::vector<ConstraintViolation>& lastViolations() const noexcept {
        return last_violations_;
    }

    /**
     * @brief Clear all constraints
     */
    void clearConstraints();

    /**
     * @brief Get human-readable description of constraints
     */
    std::string describeConstraints() const;

    /**
     * @brief Access configured constraints for optimization/planning.
     */
    [[nodiscard]] const std::vector<Constraint>& getConstraints() const noexcept {
        return constraints_;
    }

    // ── Security constants ──────────────────────────────────────────────────
    /// Maximum allowed byte length for a node or edge identifier.
    static constexpr size_t MAX_ID_LENGTH = 1024;
    /// Maximum allowed byte length for a property field name.
    static constexpr size_t MAX_FIELD_NAME_LENGTH = 256;
    /// Maximum allowed byte length for a property expected value.
    static constexpr size_t MAX_FIELD_VALUE_LENGTH = 4096;
    /// Upper bound on max_results accepted by findConstrainedPaths.
    static constexpr int MAX_RESULTS_LIMIT = 10000;

private:
    std::vector<Constraint> constraints_;
    std::unordered_set<std::string> forbidden_nodes_;
    std::unordered_set<std::string> required_nodes_;
    std::unordered_set<std::string> forbidden_edges_;
    std::unordered_set<std::string> required_edges_;
    GraphIndexManager* graph_mgr_ = nullptr;

    // ── Semantic constraint state ───────────────────────────────────────────
    const OntologyManager* ontology_ = nullptr;
    OntologyManager::Ruleset ontology_ruleset_ = OntologyManager::Ruleset::Strict;
    mutable std::vector<ConstraintViolation> last_violations_;

    /**
     * @brief Validate a node or edge identifier supplied as user input.
     *
     * Accepts identifiers that are non-empty, do not contain null bytes, and
     * do not exceed MAX_ID_LENGTH bytes.  Returns true when the identifier is
     * safe to use as a constraint value.
     */
    static bool isValidIdentifier(std::string_view s) noexcept;

    /**
     * @brief Validate a property field name supplied as user input.
     *
     * In addition to the identifier checks, field names must consist solely of
     * alphanumeric characters, underscores, hyphens, or dots
     * (pattern: [A-Za-z0-9_.\-]+) and must not exceed MAX_FIELD_NAME_LENGTH
     * bytes.  This prevents injection of arbitrary storage keys.
     */
    static bool isValidFieldName(std::string_view s) noexcept;
};

} // namespace graph
} // namespace themis

