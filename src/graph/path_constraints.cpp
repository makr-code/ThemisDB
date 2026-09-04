/**
 * @file path_constraints.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "graph/path_constraints.h"

#include <algorithm>
#include <cctype>
#include <queue>
#include <sstream>
#include <unordered_map>

#include "index/graph_index.h"
#include "utils/error_registry.h"

namespace themis {
namespace graph {

namespace {

struct ErrorRegistry {
    enum class ErrorCode { VALIDATION_FAILED, INVALID_STATE, NOT_FOUND };
};

inline errors::ErrorCode mapErrorCode(ErrorRegistry::ErrorCode code) {
    switch (code) {
        case ErrorRegistry::ErrorCode::VALIDATION_FAILED:
            return errors::ErrorCode::ERR_QUERY_INVALID_INPUT;
        case ErrorRegistry::ErrorCode::INVALID_STATE:
            return errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED;
        case ErrorRegistry::ErrorCode::NOT_FOUND:
            return errors::ErrorCode::ERR_GRAPH_PATH_NOT_FOUND;
    }
    return errors::ErrorCode::ERR_UNKNOWN;
}

inline tl::unexpected<Error> makeError(ErrorRegistry::ErrorCode code, std::string message) {
    return tl::unexpected<Error>(Error(mapErrorCode(code), std::move(message)));
}

} // namespace

PathConstraints::PathConstraints(GraphIndexManager *graph_mgr) : graph_mgr_(graph_mgr) {}

// ── Security helpers ─────────────────────────────────────────────────────────

bool PathConstraints::isValidIdentifier(std::string_view s) noexcept {
    if (s.empty() || s.size() > MAX_ID_LENGTH) {
        return false;
    }
    // Reject null bytes — they can cause string-comparison bypass via early
    // termination in underlying C-string APIs.
    return s.find('\0') == std::string_view::npos;
}

bool PathConstraints::isValidFieldName(std::string_view s) noexcept {
    if (s.empty() || s.size() > MAX_FIELD_NAME_LENGTH) {
        return false;
    }
    for (char ch : s) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}

void PathConstraints::setGraphManager(GraphIndexManager *graph_mgr) {
    graph_mgr_ = graph_mgr;
}

void PathConstraints::addMinLength([[maybe_unused]] int min_length) {
    constraints_.emplace_back(ConstraintType::MIN_LENGTH, min_length);
}

void PathConstraints::addMaxLength([[maybe_unused]] int max_length) {
    constraints_.emplace_back(ConstraintType::MAX_LENGTH, max_length);
}

void PathConstraints::addForbiddenNode(std::string_view node_id) {
    if (!isValidIdentifier(node_id)) {
        return;
    }
    forbidden_nodes_.insert(std::string(node_id));
    constraints_.emplace_back(ConstraintType::FORBIDDEN_NODE, std::string(node_id));
}

void PathConstraints::addRequiredNode(std::string_view node_id) {
    if (!isValidIdentifier(node_id)) {
        return;
    }
    required_nodes_.insert(std::string(node_id));
    constraints_.emplace_back(ConstraintType::REQUIRED_NODE, std::string(node_id));
}

void PathConstraints::addForbiddenEdge(std::string_view edge_id) {
    if (!isValidIdentifier(edge_id)) {
        return;
    }
    forbidden_edges_.insert(std::string(edge_id));
    constraints_.emplace_back(ConstraintType::FORBIDDEN_EDGE, std::string(edge_id));
}

void PathConstraints::addRequiredEdge(std::string_view edge_id) {
    if (!isValidIdentifier(edge_id)) {
        return;
    }
    required_edges_.insert(std::string(edge_id));
    constraints_.emplace_back(ConstraintType::REQUIRED_EDGE, std::string(edge_id));
}

void PathConstraints::addEdgePropertyConstraint(std::string_view field_name, std::string_view expected_value) {
    if (!isValidFieldName(field_name)) {
        return;
    }
    if (expected_value.size() > MAX_FIELD_VALUE_LENGTH) {
        return;
    }
    if (expected_value.find('\0') != std::string_view::npos) {
        return;
    }
    Constraint c(ConstraintType::EDGE_PROPERTY, std::string(field_name), std::string(expected_value));
    constraints_.push_back(std::move(c));
}

void PathConstraints::addNodePropertyConstraint(std::string_view field_name, std::string_view expected_value) {
    if (!isValidFieldName(field_name)) {
        return;
    }
    if (expected_value.size() > MAX_FIELD_VALUE_LENGTH) {
        return;
    }
    if (expected_value.find('\0') != std::string_view::npos) {
        return;
    }
    Constraint c(ConstraintType::NODE_PROPERTY, std::string(field_name), std::string(expected_value));
    constraints_.push_back(std::move(c));
}

void PathConstraints::addMaxWeight([[maybe_unused]] double max_weight) {
    constraints_.emplace_back(ConstraintType::MAX_WEIGHT, max_weight);
}

void PathConstraints::addMinWeight([[maybe_unused]] double min_weight) {
    constraints_.emplace_back(ConstraintType::MIN_WEIGHT, min_weight);
}

void PathConstraints::requireAcyclic() {
    constraints_.emplace_back(ConstraintType::NO_CYCLES);
}

void PathConstraints::requireUniqueNodes() {
    constraints_.emplace_back(ConstraintType::UNIQUE_NODES);
}

void PathConstraints::requireUniqueEdges() {
    constraints_.emplace_back(ConstraintType::UNIQUE_EDGES);
}

void PathConstraints::addCustomPredicate(std::function<bool(const std::vector<std::string> &)> predicate) {
    Constraint c(ConstraintType::CUSTOM_PREDICATE);
    c.predicate = std::move(predicate);
    constraints_.emplace_back(std::move(c));
}

Result<bool> PathConstraints::validatePath(const std::vector<std::string> &nodes,
                                           const std::vector<std::string> &edges) const {
    // Empty path validation
    if (nodes.empty()) {
        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED, "Path validation failed: empty node list");
    }

    // Check all constraints
    for (const auto &constraint : constraints_) {
        switch (constraint.type) {
            case ConstraintType::MIN_LENGTH:
                if (constraint.int_value) {
                    const int limit = *constraint.int_value;
                    // Guard against negative values: a negative int cast to
                    // size_t becomes SIZE_MAX, causing every path to fail.
                    // Treat a negative limit as "no minimum restriction".
                    if (limit >= 0 && nodes.size() < static_cast<size_t>(limit)) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path too short: " + std::to_string(nodes.size()) + " < "
                                             + std::to_string(limit));
                    }
                }
                break;

            case ConstraintType::MAX_LENGTH:
                if (constraint.int_value) {
                    const int limit = *constraint.int_value;
                    // A negative limit would wrap to SIZE_MAX, making this
                    // constraint a no-op; treat it as unlimited instead.
                    if (limit >= 0 && nodes.size() > static_cast<size_t>(limit)) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path too long: " + std::to_string(nodes.size()) + " > "
                                             + std::to_string(limit));
                    }
                }
                break;

            case ConstraintType::FORBIDDEN_NODE:
                if (constraint.string_value) {
                    for (const auto &node : nodes) {
                        if (node == *constraint.string_value) {
                            return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                             "Path contains forbidden node: " + *constraint.string_value);
                        }
                    }
                }
                break;

            case ConstraintType::REQUIRED_NODE:
                if (constraint.string_value) {
                    bool found = false;
                    for (const auto &node : nodes) {
                        if (node == *constraint.string_value) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path missing required node: " + *constraint.string_value);
                    }
                }
                break;

            case ConstraintType::FORBIDDEN_EDGE:
                if (constraint.string_value) {
                    for (const auto &edge : edges) {
                        if (edge == *constraint.string_value) {
                            return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                             "Path contains forbidden edge: " + *constraint.string_value);
                        }
                    }
                }
                break;

            case ConstraintType::REQUIRED_EDGE:
                if (constraint.string_value) {
                    bool found = false;
                    for (const auto &edge : edges) {
                        if (edge == *constraint.string_value) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path missing required edge: " + *constraint.string_value);
                    }
                }
                break;

            case ConstraintType::NO_CYCLES:
            [[fallthrough]];\n            case ConstraintType::UNIQUE_NODES: {
                std::unordered_set<std::string> seen;
                for (const auto &node : nodes) {
                    if (seen.count(node) > 0) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path contains duplicate node: " + node);
                    }
                    seen.insert(node);
                }
                break;
            }

            case ConstraintType::UNIQUE_EDGES: {
                std::unordered_set<std::string> seen;
                for (const auto &edge : edges) {
                    if (seen.count(edge) > 0) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path contains duplicate edge: " + edge);
                    }
                    seen.insert(edge);
                }
                break;
            }

            case ConstraintType::CUSTOM_PREDICATE:
                if (constraint.predicate) {
                    if (!(*constraint.predicate)(nodes)) {
                        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                         "Path failed custom predicate validation");
                    }
                }
                break;

            case ConstraintType::NODE_PROPERTY:
                // Validate that every node in the path satisfies the property constraint.
                if (graph_mgr_ && constraint.property_key.has_value() && constraint.string_value.has_value()) {
                    const std::string &key      = *constraint.property_key;
                    const std::string &expected = *constraint.string_value;
                    for (const auto &node_id : nodes) {
                        auto field_val = graph_mgr_->getNodeField(node_id, key);
                        if (!field_val.has_value() || *field_val != expected) {
                            return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                             "Node '" + node_id + "' field '" + key
                                                 + "' does not match expected value '" + expected + "'");
                        }
                    }
                }
                break;

            case ConstraintType::EDGE_PROPERTY:
                // Validate that every edge in the path satisfies the property constraint.
                if (graph_mgr_ && constraint.property_key.has_value() && constraint.string_value.has_value()) {
                    const std::string &key      = *constraint.property_key;
                    const std::string &expected = *constraint.string_value;
                    for (const auto &edge_id : edges) {
                        auto field_val = graph_mgr_->getEdgeField(edge_id, key);
                        if (!field_val.has_value() || *field_val != expected) {
                            return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                             "Edge '" + edge_id + "' field '" + key
                                                 + "' does not match expected value '" + expected + "'");
                        }
                    }
                }
                break;

            case ConstraintType::MAX_WEIGHT:
            [[fallthrough]];\n            case ConstraintType::MIN_WEIGHT:
                // Weight validation requires the accumulated path cost which is stored
                // in PathResult::cost, not available here. Callers (findConstrainedPaths)
                // enforce weight constraints using the PathResult cost directly.
                break;
        }
    }

    return true;
}

Result<std::vector<PathConstraints::PathResult>>
PathConstraints::findConstrainedPaths(std::string_view start_node, std::string_view end_node, int max_results) const {
    // Check if GraphIndexManager is set
    if (!graph_mgr_) {
        return makeError(ErrorRegistry::ErrorCode::INVALID_STATE,
                         "GraphIndexManager not set. Call setGraphManager() first.");
    }

    // Validate start/end node identifiers
    if (!isValidIdentifier(start_node)) {
        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED, "Invalid start node identifier");
    }
    if (!isValidIdentifier(end_node)) {
        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED, "Invalid end node identifier");
    }

    // Clamp max_results to a safe upper bound to prevent memory exhaustion.
    if (max_results <= 0) {
        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED, "max_results must be positive");
    }
    if (max_results > MAX_RESULTS_LIMIT) {
        max_results = MAX_RESULTS_LIMIT;
    }

    // Extract constraint values for efficient access
    int min_length            = 0;
    int max_length            = -1; // -1 means unlimited
    bool require_unique_nodes = false;
    bool require_unique_edges = false;
    bool require_acyclic      = false;
    double max_weight         = -1.0; // -1 means unlimited
    double min_weight         = -1.0; // -1 means no minimum

    for (const auto &constraint : constraints_) {
        switch (constraint.type) {
            case ConstraintType::MIN_LENGTH:
                if (constraint.int_value) {
                    min_length = *constraint.int_value;
                }
                break;
            case ConstraintType::MAX_LENGTH:
                if (constraint.int_value) {
                    max_length = *constraint.int_value;
                }
                break;
            case ConstraintType::UNIQUE_NODES:
                require_unique_nodes = true;
                break;
            case ConstraintType::UNIQUE_EDGES:
                require_unique_edges = true;
                break;
            case ConstraintType::NO_CYCLES:
                require_acyclic = true;
                break;
            case ConstraintType::MAX_WEIGHT:
                if (constraint.double_value) {
                    max_weight = *constraint.double_value;
                }
                break;
            case ConstraintType::MIN_WEIGHT:
                if (constraint.double_value) {
                    min_weight = *constraint.double_value;
                }
                break;
            default:
                break;
        }
    }

    // Validate constraint compatibility
    if (min_length > 0 && max_length > 0 && min_length > max_length) {
        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED, "MIN_LENGTH (" + std::to_string(min_length)
                                                                          + ") cannot be greater than MAX_LENGTH ("
                                                                          + std::to_string(max_length) + ")");
    }

    // Check for contradictory constraints
    for (const auto &node : required_nodes_) {
        if (forbidden_nodes_.count(node) > 0) {
            return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                             "Node '" + node + "' is both required and forbidden");
        }
    }

    for (const auto &edge : required_edges_) {
        if (forbidden_edges_.count(edge) > 0) {
            return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                             "Edge '" + edge + "' is both required and forbidden");
        }
    }

    // Storage for results
    std::vector<PathResult> results;

    // Path state for BFS
    struct PathState {
        std::vector<std::string> nodes;
        std::vector<std::string> edges;
        std::unordered_set<std::string> visited_nodes;
        std::unordered_set<std::string> visited_edges;
        double cost;
    };

    // BFS queue
    std::queue<PathState> queue;

    // Initialize with start node
    PathState initial;
    initial.nodes.push_back(std::string(start_node));
    initial.visited_nodes.insert(std::string(start_node));
    initial.cost = 0.0;
    queue.push(std::move(initial));

    // BFS traversal
    while (!queue.empty() && static_cast<int>(results.size()) < max_results) {
        PathState current = std::move(queue.front());
        queue.pop();

        const std::string &current_node = current.nodes.back();

        // Check if we reached the target
        if (current_node == end_node) {
            // Weight check: reject paths below min_weight threshold
            if (min_weight >= 0.0 && current.cost < min_weight) {
                continue; // Path too light – don't accept
            }
            // Validate path against all other constraints
            auto validation = validatePath(current.nodes, current.edges);

            if (validation.has_value() && *validation) {
                // All constraints satisfied
                PathResult result;
                result.nodes                     = current.nodes;
                result.edges                     = current.edges;
                result.cost                      = current.cost;
                result.satisfies_all_constraints = true;
                results.push_back(std::move(result));
            }
            continue; // Don't explore further from target node
        }

        // Check max length constraint (early termination)
        if (max_length > 0 && static_cast<int>(current.nodes.size()) >= max_length) {
            continue; // Path already at max length
        }

        // Get neighbors
        auto [status, adjacency] = graph_mgr_->outAdjacency(current_node);
        if (!status.ok) {
            continue; // Skip nodes with no neighbors or errors
        }

        // Explore each neighbor
        for (const auto &adj : adjacency) {
            const std::string &next_node = adj.targetPk;
            const std::string &edge_id   = adj.edgeId;

            // Check forbidden node constraint
            if (forbidden_nodes_.count(next_node) > 0) {
                continue;
            }

            // Check forbidden edge constraint
            if (forbidden_edges_.count(edge_id) > 0) {
                continue;
            }

            // Check EDGE_PROPERTY constraints: prune edges that don't satisfy
            // the required field value early, before adding to the BFS queue.
            bool edge_property_ok = true;
            for (const auto &c : constraints_) {
                if (c.type == ConstraintType::EDGE_PROPERTY && c.property_key.has_value()
                    && c.string_value.has_value()) {
                    auto field_val = graph_mgr_->getEdgeField(edge_id, *c.property_key);
                    if (!field_val.has_value() || *field_val != *c.string_value) {
                        edge_property_ok = false;
                        break;
                    }
                }
            }
            if (!edge_property_ok) {
                continue;
            }

            // Check NODE_PROPERTY constraints: prune next_node if it doesn't
            // satisfy all required node-field values.
            bool node_property_ok = true;
            for (const auto &c : constraints_) {
                if (c.type == ConstraintType::NODE_PROPERTY && c.property_key.has_value()
                    && c.string_value.has_value()) {
                    auto field_val = graph_mgr_->getNodeField(next_node, *c.property_key);
                    if (!field_val.has_value() || *field_val != *c.string_value) {
                        node_property_ok = false;
                        break;
                    }
                }
            }
            if (!node_property_ok) {
                continue;
            }

            // Check unique nodes constraint
            if (require_unique_nodes && current.visited_nodes.count(next_node) > 0) {
                continue;
            }

            // Check acyclic constraint (same as unique nodes)
            if (require_acyclic && current.visited_nodes.count(next_node) > 0) {
                continue;
            }

            // Check unique edges constraint
            if (require_unique_edges && current.visited_edges.count(edge_id) > 0) {
                continue;
            }

            // Create new path state
            PathState next_state;
            next_state.nodes = current.nodes;
            next_state.nodes.push_back(next_node);
            next_state.edges = current.edges;
            next_state.edges.push_back(edge_id);
            next_state.visited_nodes = current.visited_nodes;
            next_state.visited_nodes.insert(next_node);
            next_state.visited_edges = current.visited_edges;
            next_state.visited_edges.insert(edge_id);

            // Get edge weight for cost calculation
            double edge_weight = graph_mgr_->getEdgeWeight("", edge_id, "_weight");
            next_state.cost    = current.cost + edge_weight;

            // Prune states that already exceed the max_weight budget
            if (max_weight >= 0.0 && next_state.cost > max_weight) {
                continue;
            }

            queue.push(std::move(next_state));
        }
    }

    // If no paths found
    if (results.empty()) {
        return makeError(ErrorRegistry::ErrorCode::NOT_FOUND, "No paths found from '" + std::string(start_node)
                                                                  + "' to '" + std::string(end_node)
                                                                  + "' satisfying all constraints");
    }

    // Sort results by cost (shortest paths first)
    std::sort(results.begin(), results.end(), [](const PathResult &a, const PathResult &b) { return a.cost < b.cost; });

    // Limit to max_results
    if (static_cast<int>(results.size()) > max_results) {
        results.resize(max_results);
    }

    return results;
}

void PathConstraints::clearConstraints() {
    constraints_.clear();
    forbidden_nodes_.clear();
    required_nodes_.clear();
    forbidden_edges_.clear();
    required_edges_.clear();
    ontology_ = nullptr;
    last_violations_.clear();
}

std::string PathConstraints::describeConstraints() const {
    std::ostringstream oss;
    oss << "Path Constraints (" << constraints_.size() << " total):\n";

    for (const auto &constraint : constraints_) {
        oss << "  - ";
        switch (constraint.type) {
            case ConstraintType::MIN_LENGTH:
                oss << "Minimum length: " << *constraint.int_value;
                break;
            case ConstraintType::MAX_LENGTH:
                oss << "Maximum length: " << *constraint.int_value;
                break;
            case ConstraintType::FORBIDDEN_NODE:
                oss << "Forbidden node: " << *constraint.string_value;
                break;
            case ConstraintType::REQUIRED_NODE:
                oss << "Required node: " << *constraint.string_value;
                break;
            case ConstraintType::FORBIDDEN_EDGE:
                oss << "Forbidden edge: " << *constraint.string_value;
                break;
            case ConstraintType::REQUIRED_EDGE:
                oss << "Required edge: " << *constraint.string_value;
                break;
            case ConstraintType::NO_CYCLES:
                oss << "No cycles allowed";
                break;
            case ConstraintType::UNIQUE_NODES:
                oss << "Unique nodes required";
                break;
            case ConstraintType::UNIQUE_EDGES:
                oss << "Unique edges required";
                break;
            case ConstraintType::CUSTOM_PREDICATE:
                oss << "Custom predicate";
                break;
            case ConstraintType::NODE_PROPERTY:
                if (constraint.property_key.has_value() && constraint.string_value.has_value()) {
                    oss << "Node property: " << *constraint.property_key << " = " << *constraint.string_value;
                } else {
                    oss << "Node property constraint";
                }
                break;
            case ConstraintType::EDGE_PROPERTY:
                if (constraint.property_key.has_value() && constraint.string_value.has_value()) {
                    oss << "Edge property: " << *constraint.property_key << " = " << *constraint.string_value;
                } else {
                    oss << "Edge property constraint";
                }
                break;
            case ConstraintType::MAX_WEIGHT:
                if (constraint.double_value.has_value()) {
                    oss << "Maximum path weight: " << *constraint.double_value;
                } else {
                    oss << "Maximum weight constraint";
                }
                break;
            case ConstraintType::MIN_WEIGHT:
                if (constraint.double_value.has_value()) {
                    oss << "Minimum path weight: " << *constraint.double_value;
                } else {
                    oss << "Minimum weight constraint";
                }
                break;
        }
        oss << "\n";
    }

    return oss.str();
}

// ============================================================================
// Semantic constraint methods
// ============================================================================

void PathConstraints::addSemanticConstraint(const OntologyManager *ontology, OntologyManager::Ruleset ruleset) {
    ontology_         = ontology;
    ontology_ruleset_ = ruleset;
}

std::vector<PathConstraints::ConstraintViolation>
PathConstraints::validateSemanticPath(const PathResult &result) const {
    std::vector<ConstraintViolation> violations;
    if (!ontology_ || !graph_mgr_) {
        return violations;
    }

    // Each edge in result.edges connects result.nodes[i] to result.nodes[i+1].
    const std::size_t edge_count = result.edges.size();
    for (std::size_t i = 0; i < edge_count; ++i) {
        const std::string &edge_id  = result.edges[i];
        const std::string &src_node = (i < result.nodes.size()) ? result.nodes[i] : "";
        const std::string &tgt_node = (i + 1 < result.nodes.size()) ? result.nodes[i + 1] : "";

        // Fetch node class from the graph ("_class" field; default "")
        std::string src_class, tgt_class, edge_type;
        {
            auto sc = graph_mgr_->getNodeField(src_node, "_class");
            if (sc) {
                src_class = *sc;
            }
            auto tc = graph_mgr_->getNodeField(tgt_node, "_class");
            if (tc) {
                tgt_class = *tc;
            }
            auto et = graph_mgr_->getEdgeField(edge_id, "type");
            if (et) {
                edge_type = *et;
            }
        }

        // If class or type are unknown, skip (graceful degradation)
        if (src_class.empty() || tgt_class.empty() || edge_type.empty()) {
            continue;
        }

        if (!ontology_->isEdgeTypeAllowed(src_class, tgt_class, edge_type)) {
            ConstraintViolation v;
            v.edge_id      = edge_id;
            v.source_class = src_class;
            v.target_class = tgt_class;
            v.edge_type    = edge_type;
            v.description
                = "Edge type '" + edge_type + "' not permitted between '" + src_class + "' and '" + tgt_class + "'";
            violations.push_back(std::move(v));
        }
    }
    last_violations_ = violations;
    return violations;
}

} // namespace graph
} // namespace themis
