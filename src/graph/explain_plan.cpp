/**
 * @file explain_plan.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "graph/explain_plan.h"

#include <sstream>

namespace themis {
namespace graph {
namespace {

const char* nodeTypeToString(GraphPlanNodeType type) {
    switch (type) {
        case GraphPlanNodeType::VERTEX_SCAN: return "VERTEX_SCAN";
        case GraphPlanNodeType::EDGE_SCAN: return "EDGE_SCAN";
        case GraphPlanNodeType::INDEX_LOOKUP: return "INDEX_LOOKUP";
        case GraphPlanNodeType::BFS_TRAVERSAL: return "BFS_TRAVERSAL";
        case GraphPlanNodeType::DFS_TRAVERSAL: return "DFS_TRAVERSAL";
        case GraphPlanNodeType::SHORTEST_PATH: return "SHORTEST_PATH";
        case GraphPlanNodeType::FILTER: return "FILTER";
        case GraphPlanNodeType::PROJECTION: return "PROJECTION";
        case GraphPlanNodeType::AGGREGATE: return "AGGREGATE";
        case GraphPlanNodeType::SORT: return "SORT";
        case GraphPlanNodeType::LIMIT: return "LIMIT";
        case GraphPlanNodeType::HASH_JOIN: return "HASH_JOIN";
        case GraphPlanNodeType::NESTED_LOOP_JOIN: return "NESTED_LOOP_JOIN";
    }
    return "UNKNOWN";
}

std::string escapeJson(const std::string& value) {
    std::string out = {};
    out.reserve(value.size());
    for (char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

} // namespace

std::string GraphExplainPlan::toDot() const {
    if (nodes.empty()) {
        return {};
    }

    std::ostringstream out = {};
    out << "digraph GraphExplainPlan {\n";
    out << "  label=\"" << escapeJson(plan_id) << "\";\n";

    for (const auto& node : nodes) {
        out << "  \"" << escapeJson(node.node_id) << "\" [label=\""
            << nodeTypeToString(node.type) << "\\n"
            << escapeJson(node.description) << "\"];\n";

        for (const auto& child_id : node.child_node_ids) {
            out << "  \"" << escapeJson(node.node_id) << "\" -> \""
                << escapeJson(child_id) << "\";\n";
        }
    }

    out << "}\n";
    return out.str();
}

std::string GraphExplainPlan::toJson() const {
    if (nodes.empty()) {
        return {};
    }

    std::ostringstream out = {};
    out << "{";
    out << "\"query\":\"" << escapeJson(query) << "\",";
    out << "\"plan_id\":\"" << escapeJson(plan_id) << "\",";
    out << "\"root_node_id\":\"" << escapeJson(root_node_id) << "\",";
    out << "\"total_estimated_cost\":" << total_estimated_cost << ",";
    out << "\"total_actual_ms\":" << total_actual_ms << ",";
    out << "\"is_analyzed\":" << (is_analyzed ? "true" : "false") << ",";
    out << "\"nodes\":[";

    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        out << "{";
        out << "\"node_id\":\"" << escapeJson(node.node_id) << "\",";
        out << "\"type\":\"" << nodeTypeToString(node.type) << "\",";
        out << "\"description\":\"" << escapeJson(node.description) << "\",";
        out << "\"estimated_cost\":" << node.estimated_cost << ",";
        out << "\"estimated_rows\":" << node.estimated_rows << ",";
        out << "\"actual_ms\":" << node.actual_ms << ",";
        out << "\"actual_rows\":" << node.actual_rows << ",";

        out << "\"child_node_ids\":[";
        for (size_t j = 0; j < node.child_node_ids.size(); ++j) {
            out << "\"" << escapeJson(node.child_node_ids[j]) << "\"";
            if (j + 1 < node.child_node_ids.size()) {
                out << ",";
            }
        }
        out << "],";

        out << "\"properties\":{";
        size_t property_idx = 0;
        for (const auto& [key, value] : node.properties) {
            out << "\"" << escapeJson(key) << "\":\"" << escapeJson(value) << "\"";
            if (++property_idx < node.properties.size()) {
                out << ",";
            }
        }
        out << "}";

        out << "}";
        if (i + 1 < nodes.size()) {
            out << ",";
        }
    }

    out << "]}";
    return out.str();
}

} // namespace graph
} // namespace themis
