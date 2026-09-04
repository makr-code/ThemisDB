/**
 * @file query_plan_visualizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.25
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Query plan visualization implementation
// Provides EXPLAIN / EXPLAIN ANALYZE output in text, JSON, and DOT formats.

#include "query/query_plan_visualizer.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>

namespace themis {
namespace query {

// Maximum recursion depth for plan tree rendering (guards against deep/cyclic trees).
static constexpr int kMaxPlanDepth = 128;

// ============================================================================
// Helpers
// ============================================================================

std::string QueryPlanVisualizer::planNodeTypeName(PlanNodeType type) {
    switch (type) {
        case PlanNodeType::SeqScan:           return "SeqScan";
        case PlanNodeType::IndexScan:         return "IndexScan";
        case PlanNodeType::Filter:            return "Filter";
        case PlanNodeType::Sort:              return "Sort";
        case PlanNodeType::Limit:             return "Limit";
        case PlanNodeType::Return:            return "Return";
        case PlanNodeType::Aggregate:         return "Aggregate";
        case PlanNodeType::HashJoin:          return "HashJoin";
        case PlanNodeType::NestedLoopJoin:    return "NestedLoopJoin";
        case PlanNodeType::GraphTraversal:    return "GraphTraversal";
        case PlanNodeType::VectorSearch:      return "VectorSearch";
        case PlanNodeType::SpatialFilter:     return "SpatialFilter";
        case PlanNodeType::CTE:               return "CTE";
        case PlanNodeType::Subquery:          return "Subquery";
        case PlanNodeType::TensorContraction: return "TensorContraction";
        case PlanNodeType::LLMGenerate:       return "LLMGenerate";
        default:                              return "Unknown";
    }
}

// ============================================================================
// Plan construction
// ============================================================================

QueryPlanNode QueryPlanVisualizer::buildPlan(const ConjunctiveQuery& query,
                                              const QueryOptimizer::Plan& plan) {
    // Outermost node: Return. Use the most-selective count as the final output estimate.
    QueryPlanNode return_node = {};
    return_node.type = PlanNodeType::Return;
    return_node.description = "Return";
    return_node.estimated_rows = plan.details.empty() ? 0 : plan.details.front().estimatedCount;
    return_node.estimated_cost = 10.0;

    // Build filter chain in REVERSE predicate order so that the most-selective
    // predicate (smallest estimatedCount, orderedPredicates[0]) ends up deepest
    // in the tree (closest to the scan).  Data flows bottom-up:
    //   Scan → Filter[most-selective] → … → Filter[least-selective] → Return
    //
    // orderedPredicates is sorted ascending: [0] = most selective, [static_cast<int>(n - 1)] = least.
    // Iterating from [static_cast<int>(n - 1)] down to [0] builds the chain in the correct display
    // order (top = least-selective child of Return, bottom = most-selective
    // parent of Scan).
    const size_t n = plan.orderedPredicates.size();
    // Use the least-selective predicate's count as a proxy for the scan size.
    const size_t proxy_scan_rows =
        plan.details.empty() ? 0 : plan.details.back().estimatedCount;

    QueryPlanNode* current_parent = &return_node;

    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        const auto& pred = plan.orderedPredicates[static_cast<size_t>(i)];

        // Selectivity: fraction of proxy_scan_rows that pass this predicate.
        const double selectivity =
            (proxy_scan_rows > 0 && static_cast<size_t>(i) < plan.details.size())
                ? std::min(1.0, static_cast<double>(plan.details[static_cast<size_t>(i)].estimatedCount) /
                                    static_cast<double>(proxy_scan_rows))
                : 1.0;

        auto filter_node = std::make_shared<QueryPlanNode>();
        filter_node->type = PlanNodeType::Filter;
        filter_node->description = pred.column + " == " + pred.value;
        filter_node->selectivity = std::min(1.0, std::max(0.0, selectivity));
        filter_node->estimated_rows = static_cast<size_t>(i) < plan.details.size()
            ? plan.details[static_cast<size_t>(i)].estimatedCount : 0;
        // Heuristic: each filter contributes proportionally to total cost
        filter_node->estimated_cost = 50.0 + 10.0 * static_cast<double>(i);
        filter_node->attributes.push_back(pred.column);

        current_parent->children.push_back(filter_node);
        current_parent = filter_node.get();
    }

    // Leaf node: IndexScan or SeqScan
    auto scan_node = std::make_shared<QueryPlanNode>();
    const bool has_index = !plan.orderedPredicates.empty();
    if (has_index) {
        scan_node->type = PlanNodeType::IndexScan;
        scan_node->description = "IndexScan on " + query.table;
        // Use first ordered predicate column as the index reference
        scan_node->index_name = plan.orderedPredicates.front().column + "_idx";
    } else {
        scan_node->type = PlanNodeType::SeqScan;
        scan_node->description = "SeqScan on " + query.table;
    }
    scan_node->estimated_rows = plan.details.empty() ? 0
        : plan.details.front().estimatedCount;
    scan_node->estimated_cost = 200.0;

    current_parent->children.push_back(scan_node);

    // Accumulate total cost bottom-up (simple sum for estimation)
    double total_cost = 0.0;
    std::function<void(QueryPlanNode&)> accumulate_cost = [&]([[maybe_unused]] QueryPlanNode& n) {
        for (auto& child : n.children) {
            accumulate_cost(*child);
            total_cost += child->estimated_cost;
        }
    };
    accumulate_cost(return_node);
    return_node.estimated_cost += total_cost;

    return return_node;
}

// ============================================================================
// Text rendering (EXPLAIN style)
// ============================================================================

void QueryPlanVisualizer::toTextImpl(const QueryPlanNode& node, bool analyze,
                                      std::string& out, int depth) {
    if (depth > kMaxPlanDepth) {
        out += std::string(4, ' ') + "<max depth exceeded>\n";
        return;
    }

    // Indent
    for (int i = 0; i < depth; ++i) {
        out += (i < depth - 1) ? "    " : "  -> ";
    }

    // Operator name + description
    out += planNodeTypeName(node.type);
    if (!node.description.empty() &&
        node.description != planNodeTypeName(node.type)) {
        out += "  (" + node.description + ")";
    }

    // Cost / rows
    std::ostringstream ann = {};
    ann << std::fixed << std::setprecision(2);
    ann << "  (cost=" << node.estimated_cost
        << " rows=" << node.estimated_rows;

    if (node.index_name.has_value()) {
        ann << " index=" << *node.index_name;
    }
    if (node.selectivity < 1.0) {
        ann << " selectivity=" << node.selectivity;
    }
    ann << ")";

    if (analyze && node.actual_time_ms >= 0.0) {
        ann << "  (actual time=" << node.actual_time_ms << "ms"
            << " rows=" << node.actual_rows << ")";
    }
    out += ann.str();
    out += "\n";

    // Additional attributes
    for (const auto& attr : node.attributes) {
        out += std::string(static_cast<std::size_t>(depth + 1) * 4, ' ');
        out += attr + "\n";
    }

    // Recurse
    for (const auto& child : node.children) {
        toTextImpl(*child, analyze, out, depth + 1);
    }
}

std::string QueryPlanVisualizer::toText(const QueryPlanNode& root, bool analyze) {
    std::string out = {};
    out.reserve(4096);  // Pre-allocate for typical query plan output (5-20KB)
    out += analyze ? "EXPLAIN ANALYZE\n" : "EXPLAIN\n";
    out += std::string(60, '-') + "\n";
    toTextImpl(root, analyze, out, 0);
    out += std::string(60, '-') + "\n";
 
    // Summary line
    std::ostringstream summary = {};
    summary << std::fixed << std::setprecision(2);
    summary << "Estimated total cost: " << root.estimated_cost
            << "  Estimated rows: " << root.estimated_rows << "\n";
    out += summary.str();
    return out;
}

// ============================================================================
// JSON rendering
// ============================================================================

nlohmann::json QueryPlanVisualizer::toJSONImpl(const QueryPlanNode& node, bool analyze) {
    return toJSONImpl(node, analyze, 0);
}

nlohmann::json QueryPlanVisualizer::toJSONImpl(const QueryPlanNode& node, bool analyze, int depth) {
    if (depth > kMaxPlanDepth) {
        nlohmann::json j;
        j["error"] = "max depth exceeded";
        return j;
    }

    nlohmann::json j;
    j["type"] = planNodeTypeName(node.type);
    j["description"] = node.description;
    j["estimated_cost"] = node.estimated_cost;
    j["estimated_rows"] = node.estimated_rows;

    if (node.index_name.has_value()) {
        j["index"] = *node.index_name;
    }
    if (node.selectivity < 1.0) {
        j["selectivity"] = node.selectivity;
    }
    if (!node.attributes.empty()) {
        j["attributes"] = node.attributes;
    }
    if (analyze) {
        j["actual_time_ms"] = node.actual_time_ms;
        j["actual_rows"] = node.actual_rows;
    }

    nlohmann::json children_arr = nlohmann::json::array();
    for (const auto& child : node.children) {
        children_arr.push_back(toJSONImpl(*child, analyze, depth + 1));
    }
    j["children"] = children_arr;
    return j;
}

nlohmann::json QueryPlanVisualizer::toJSON(const QueryPlanNode& root, bool analyze) {
    nlohmann::json j;
    j["mode"] = analyze ? "EXPLAIN ANALYZE" : "EXPLAIN";
    j["plan"] = toJSONImpl(root, analyze);
    return j;
}

// ============================================================================
// DOT rendering (Graphviz)
// ============================================================================

// Escape characters that would produce invalid or misleading DOT quoted strings:
//   " and \ need escaping per DOT syntax.
//   \n, \r, \t are invalid bare bytes inside a quoted string; replace with
//   their DOT escape sequences (\\n etc.) which Graphviz renders as printable.
static std::string dotEscape(const std::string& s) {
    std::string result = {};
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

void QueryPlanVisualizer::toDOTImpl(const QueryPlanNode& node, int& id_counter,
                                     std::string& nodes_out, std::string& edges_out) {
    toDOTImpl(node, id_counter, nodes_out, edges_out, 0);
}

void QueryPlanVisualizer::toDOTImpl(const QueryPlanNode& node, int& id_counter,
                                     std::string& nodes_out, std::string& edges_out, int depth) {
    if (depth > kMaxPlanDepth) {
        int my_id = id_counter++;
        nodes_out += "  n" + std::to_string(my_id) +
                     " [label=\"<max depth exceeded>\" shape=diamond];\n";
        return;
    }

    int my_id = id_counter++;

    // Node shape: box for scan/join, ellipse for filter/sort
    const char* shape = "box";
    if (node.type == PlanNodeType::Filter ||
        node.type == PlanNodeType::Sort ||
        node.type == PlanNodeType::Limit) {
        shape = "ellipse";
    }

    std::ostringstream label = {};
    label << planNodeTypeName(node.type);
    if (!node.description.empty() &&
        node.description != planNodeTypeName(node.type)) {
        label << "\\n" << dotEscape(node.description);
    }
    label << "\\ncost=" << static_cast<long long>(node.estimated_cost)
          << " rows=" << node.estimated_rows;
    if (node.index_name.has_value()) {
        label << "\\nidx=" << dotEscape(*node.index_name);
    }

    nodes_out += "  n" + std::to_string(my_id) + " [label=\"" + label.str()
              + "\" shape=" + shape + "];\n";

    for (const auto& child : node.children) {
        int child_id = id_counter;
        toDOTImpl(*child, id_counter, nodes_out, edges_out, depth + 1);
        edges_out += "  n" + std::to_string(my_id)
                  + " -> n" + std::to_string(child_id) + ";\n";
    }
}

std::string QueryPlanVisualizer::toDOT(const QueryPlanNode& root) {
    std::string nodes_out = {};
    std::string edges_out = {};
    nodes_out.reserve(8192);  // Pre-allocate for node definitions
    edges_out.reserve(4096);  // Pre-allocate for edge definitions
    int counter = 0;
    toDOTImpl(root, counter, nodes_out, edges_out);
 
    std::string dot = {};
    dot.reserve(12288);  // Pre-allocate for complete DOT output
    dot += "digraph QueryPlan {\n";
    dot += "  rankdir=TB;\n";
    dot += "  node [fontname=Helvetica fontsize=10];\n";
    dot += nodes_out;
    dot += edges_out;
    dot += "}\n";
    return dot;
}

} // namespace query
} // namespace themis

