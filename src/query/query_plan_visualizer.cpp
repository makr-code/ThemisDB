// Query plan visualization implementation
// Provides EXPLAIN / EXPLAIN ANALYZE output in text, JSON, and DOT formats.

#include "query/query_plan_visualizer.h"

#include <sstream>
#include <iomanip>

namespace themis {
namespace query {

// ============================================================================
// Helpers
// ============================================================================

std::string QueryPlanVisualizer::planNodeTypeName(PlanNodeType type) {
    switch (type) {
        case PlanNodeType::SeqScan:         return "SeqScan";
        case PlanNodeType::IndexScan:       return "IndexScan";
        case PlanNodeType::Filter:          return "Filter";
        case PlanNodeType::Sort:            return "Sort";
        case PlanNodeType::Limit:           return "Limit";
        case PlanNodeType::Return:          return "Return";
        case PlanNodeType::Aggregate:       return "Aggregate";
        case PlanNodeType::HashJoin:        return "HashJoin";
        case PlanNodeType::NestedLoopJoin:  return "NestedLoopJoin";
        case PlanNodeType::GraphTraversal:  return "GraphTraversal";
        case PlanNodeType::VectorSearch:    return "VectorSearch";
        case PlanNodeType::SpatialFilter:   return "SpatialFilter";
        case PlanNodeType::CTE:             return "CTE";
        case PlanNodeType::Subquery:        return "Subquery";
        default:                            return "Unknown";
    }
}

// ============================================================================
// Plan construction
// ============================================================================

QueryPlanNode QueryPlanVisualizer::buildPlan(const ConjunctiveQuery& query,
                                              const QueryOptimizer::Plan& plan) {
    // Outermost node: Return
    QueryPlanNode return_node;
    return_node.type = PlanNodeType::Return;
    return_node.description = "Return";
    return_node.estimated_rows = plan.details.empty() ? 0 : plan.details.front().estimatedCount;
    return_node.estimated_cost = 10.0;

    // If there are ordered predicates, build a filter chain leading down to a scan.
    // The first predicate (most selective) is closest to the scan; each subsequent
    // predicate wraps the previous one as a parent Filter node.
    QueryPlanNode* current_parent = &return_node;

    for (size_t i = 0; i < plan.orderedPredicates.size(); ++i) {
        const auto& pred = plan.orderedPredicates[i];
        const double selectivity = (plan.details.size() > i && plan.details[i].estimatedCount > 0)
            ? (static_cast<double>(plan.details[i].estimatedCount) /
               std::max<size_t>(1, plan.details.empty() ? 1 : plan.details.front().estimatedCount))
            : 1.0;

        auto filter_node = std::make_shared<QueryPlanNode>();
        filter_node->type = PlanNodeType::Filter;
        filter_node->description = pred.column + " == " + pred.value;
        filter_node->selectivity = std::min(1.0, std::max(0.0, selectivity));
        filter_node->estimated_rows = plan.details.size() > i
            ? plan.details[i].estimatedCount : 0;
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
    std::function<void(QueryPlanNode&)> accumulate_cost = [&](QueryPlanNode& n) {
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
    std::ostringstream ann;
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
        for (int i = 0; i <= depth; ++i) out += "    ";
        out += attr + "\n";
    }

    // Recurse
    for (const auto& child : node.children) {
        toTextImpl(*child, analyze, out, depth + 1);
    }
}

std::string QueryPlanVisualizer::toText(const QueryPlanNode& root, bool analyze) {
    std::string out;
    out += analyze ? "EXPLAIN ANALYZE\n" : "EXPLAIN\n";
    out += std::string(60, '-') + "\n";
    toTextImpl(root, analyze, out, 0);
    out += std::string(60, '-') + "\n";

    // Summary line
    std::ostringstream summary;
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
        children_arr.push_back(toJSONImpl(*child, analyze));
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

// Escape double-quotes and backslashes in a DOT label string.
static std::string dotEscape(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        if (c == '"' || c == '\\') result += '\\';
        result += c;
    }
    return result;
}

void QueryPlanVisualizer::toDOTImpl(const QueryPlanNode& node, int& id_counter,
                                     std::string& nodes_out, std::string& edges_out) {
    int my_id = id_counter++;

    // Node shape: box for scan/join, ellipse for filter/sort
    const char* shape = "box";
    if (node.type == PlanNodeType::Filter ||
        node.type == PlanNodeType::Sort ||
        node.type == PlanNodeType::Limit) {
        shape = "ellipse";
    }

    std::ostringstream label;
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
        toDOTImpl(*child, id_counter, nodes_out, edges_out);
        edges_out += "  n" + std::to_string(my_id)
                  + " -> n" + std::to_string(child_id) + ";\n";
    }
}

std::string QueryPlanVisualizer::toDOT(const QueryPlanNode& root) {
    std::string nodes_out;
    std::string edges_out;
    int counter = 0;
    toDOTImpl(root, counter, nodes_out, edges_out);

    std::string dot;
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
