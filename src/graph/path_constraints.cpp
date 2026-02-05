#include "graph/path_constraints.h"
#include "core/error_registry.h"
#include <sstream>

namespace themis {
namespace graph {

void PathConstraints::addMinLength(int min_length) {
    constraints_.emplace_back(ConstraintType::MIN_LENGTH, min_length);
}

void PathConstraints::addMaxLength(int max_length) {
    constraints_.emplace_back(ConstraintType::MAX_LENGTH, max_length);
}

void PathConstraints::addForbiddenNode(std::string_view node_id) {
    forbidden_nodes_.insert(std::string(node_id));
    constraints_.emplace_back(ConstraintType::FORBIDDEN_NODE, std::string(node_id));
}

void PathConstraints::addRequiredNode(std::string_view node_id) {
    required_nodes_.insert(std::string(node_id));
    constraints_.emplace_back(ConstraintType::REQUIRED_NODE, std::string(node_id));
}

void PathConstraints::addForbiddenEdge(std::string_view edge_id) {
    forbidden_edges_.insert(std::string(edge_id));
    constraints_.emplace_back(ConstraintType::FORBIDDEN_EDGE, std::string(edge_id));
}

void PathConstraints::addRequiredEdge(std::string_view edge_id) {
    required_edges_.insert(std::string(edge_id));
    constraints_.emplace_back(ConstraintType::REQUIRED_EDGE, std::string(edge_id));
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

void PathConstraints::addCustomPredicate(std::function<bool(const std::vector<std::string>&)> predicate) {
    Constraint c(ConstraintType::CUSTOM_PREDICATE);
    c.predicate = std::move(predicate);
    constraints_.emplace_back(std::move(c));
}

Result<bool> PathConstraints::validatePath(
    const std::vector<std::string>& nodes,
    const std::vector<std::string>& edges) const {
    
    // Stub implementation - basic validation only
    if (nodes.empty()) {
        return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED, 
                        "Path validation failed: empty node list");
    }

    // Check basic constraints
    for (const auto& constraint : constraints_) {
        switch (constraint.type) {
            case ConstraintType::MIN_LENGTH:
                if (constraint.int_value && nodes.size() < static_cast<size_t>(*constraint.int_value)) {
                    return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                   "Path too short: " + std::to_string(nodes.size()) + 
                                   " < " + std::to_string(*constraint.int_value));
                }
                break;
            case ConstraintType::MAX_LENGTH:
                if (constraint.int_value && nodes.size() > static_cast<size_t>(*constraint.int_value)) {
                    return makeError(ErrorRegistry::ErrorCode::VALIDATION_FAILED,
                                   "Path too long: " + std::to_string(nodes.size()) + 
                                   " > " + std::to_string(*constraint.int_value));
                }
                break;
            default:
                // Other constraints not yet implemented in stub
                break;
        }
    }

    return true;
}

Result<std::vector<PathConstraints::PathResult>> PathConstraints::findConstrainedPaths(
    std::string_view start_node,
    std::string_view end_node,
    int max_results) const {
    
    // Stub implementation
    return makeError(ErrorRegistry::ErrorCode::NOT_IMPLEMENTED,
                    "PathConstraints::findConstrainedPaths is not yet implemented. "
                    "This is a stub for GAP-006. Full implementation coming in future release.");
}

void PathConstraints::clearConstraints() {
    constraints_.clear();
    forbidden_nodes_.clear();
    required_nodes_.clear();
    forbidden_edges_.clear();
    required_edges_.clear();
}

std::string PathConstraints::describeConstraints() const {
    std::ostringstream oss;
    oss << "Path Constraints (" << constraints_.size() << " total):\n";
    
    for (const auto& constraint : constraints_) {
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
        }
        oss << "\n";
    }
    
    return oss.str();
}

} // namespace graph
} // namespace themis
