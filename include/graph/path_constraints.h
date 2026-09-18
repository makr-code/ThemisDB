/**
 * @file path_constraints.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PathConstraints {
public:
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

    struct Constraint {
        ConstraintType type;
        std::optional<int> int_value;
        std::optional<std::string> string_value;
        std::optional<std::function<bool(const std::vector<std::string>&)>> predicate;
        std::optional<std::string> property_key;
        std::optional<double> double_value;

        Constraint(ConstraintType t) : type(t) {}
        Constraint(ConstraintType t, int value) : type(t), int_value(value) {}
        Constraint(ConstraintType t, std::string value) : type(t), string_value(std::move(value)) {}
        Constraint(ConstraintType t, std::string key, std::string value)
            : type(t), string_value(std::move(value)), property_key(std::move(key)) {}
        Constraint(ConstraintType t, double threshold) : type(t), double_value(threshold) {}
    };

    struct PathResult {
        std::vector<std::string> nodes;
        std::vector<std::string> edges;
        double cost = 0.0;
        bool satisfies_all_constraints = false;
        std::vector<std::string> violated_constraints;
    };

    PathConstraints() = default;
    
    /**
     * @brief Path Constraints.
     * @param[in,out] graph_mgr Input/output parameter.
     * @return Return value.
     */
    explicit PathConstraints(GraphIndexManager* graph_mgr);
    
    /**
     * @brief Set Graph Manager.
     * @param[in,out] graph_mgr Input/output parameter.
     */
    void setGraphManager(GraphIndexManager* graph_mgr);

    /**
     * @brief Add Min Length.
     * @param[in] min_length Input parameter.
     */
    void addMinLength(int min_length);

    /**
     * @brief Add Max Length.
     * @param[in] max_length Input parameter.
     */
    void addMaxLength(int max_length);

    /**
     * @brief Add Forbidden Node.
     * @param[in] node_id Identifier of the node.
     */
    void addForbiddenNode(std::string_view node_id);

    /**
     * @brief Add Required Node.
     * @param[in] node_id Identifier of the node.
     */
    void addRequiredNode(std::string_view node_id);

    /**
     * @brief Add Forbidden Edge.
     * @param[in] edge_id Identifier of the edge.
     */
    void addForbiddenEdge(std::string_view edge_id);

    /**
     * @brief Add Required Edge.
     * @param[in] edge_id Identifier of the edge.
     */
    void addRequiredEdge(std::string_view edge_id);

    /**
     * @brief Add Edge Property Constraint.
     * @param[in] field_name Name of the field.
     * @param[in] expected_value Input parameter.
     */
    void addEdgePropertyConstraint(std::string_view field_name, std::string_view expected_value);

    /**
     * @brief Add Node Property Constraint.
     * @param[in] field_name Name of the field.
     * @param[in] expected_value Input parameter.
     */
    void addNodePropertyConstraint(std::string_view field_name, std::string_view expected_value);

    /**
     * @brief Add Max Weight.
     * @param[in] max_weight Input parameter.
     */
    void addMaxWeight(double max_weight);

    /**
     * @brief Add Min Weight.
     * @param[in] min_weight Input parameter.
     */
    void addMinWeight(double min_weight);

    /**
     * @brief Require Acyclic.
     */
    void requireAcyclic();

    /**
     * @brief Require Unique Nodes.
     */
    void requireUniqueNodes();

    /**
     * @brief Require Unique Edges.
     */
    void requireUniqueEdges();

    void addCustomPredicate(std::function<bool(const std::vector<std::string>&)> predicate);

    /**
     * @brief Validate Path.
     * @param[in] nodes Input parameter.
     * @param[in] edges Input parameter.
     * @return Return value.
     */
    Result<bool> validatePath(const std::vector<std::string>& nodes, 
                              const std::vector<std::string>& edges) const;

    Result<std::vector<PathResult>> findConstrainedPaths(
        std::string_view start_node,
        std::string_view end_node,
        int max_results = 10
    ) const;

    struct ConstraintViolation {
        std::string edge_id;
        std::string source_class;
        std::string target_class;
        std::string edge_type;
        std::string description;
    };

    void addSemanticConstraint(const OntologyManager* ontology,
                               OntologyManager::Ruleset ruleset = OntologyManager::Ruleset::Strict);

    /**
     * @brief Validate Semantic Path.
     * @param[in] result Input parameter.
     * @return Return value.
     */
    std::vector<ConstraintViolation> validateSemanticPath(const PathResult& result) const;

    const std::vector<ConstraintViolation>& lastViolations() const noexcept {
        return last_violations_;
    }

    /**
     * @brief Clear Constraints.
     */
    void clearConstraints();

    /**
     * @brief Describe Constraints.
     * @return Return value.
     */
    std::string describeConstraints() const;

    [[nodiscard]] const std::vector<Constraint>& getConstraints() const noexcept {
        return constraints_;
    }

    // ── Security constants ──────────────────────────────────────────────────
    static constexpr size_t MAX_ID_LENGTH = 1024;
    static constexpr size_t MAX_FIELD_NAME_LENGTH = 256;
    static constexpr size_t MAX_FIELD_VALUE_LENGTH = 4096;
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
     * @brief Is Valid Identifier.
     * @param[in] s Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isValidIdentifier(std::string_view s) noexcept;

    /**
     * @brief Is Valid Field Name.
     * @param[in] s Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isValidFieldName(std::string_view s) noexcept;
};

} // namespace graph
} // namespace themis

