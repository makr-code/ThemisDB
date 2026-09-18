/**
 * @file explain_plan.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <string>
#include <vector>
#include <map>

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// GraphPlanNodeType
// ---------------------------------------------------------------------------

enum class GraphPlanNodeType {
    VERTEX_SCAN,
    EDGE_SCAN,
    INDEX_LOOKUP,
    BFS_TRAVERSAL,
    DFS_TRAVERSAL,
    SHORTEST_PATH,
    FILTER,
    PROJECTION,
    AGGREGATE,
    SORT,
    LIMIT,
    HASH_JOIN,
    NESTED_LOOP_JOIN,
};

// ---------------------------------------------------------------------------
// GraphPlanNode
// ---------------------------------------------------------------------------

struct GraphPlanNode {
    std::string node_id;
    GraphPlanNodeType type;
    std::string description;
    std::vector<std::string> child_node_ids;
    std::map<std::string, std::string> properties;
    double estimated_cost = 0.0;
    double estimated_rows = 0.0;
    double actual_ms      = -1.0;  ///< -1 = not executed (EXPLAIN only)
    size_t actual_rows    = 0;
};

// ---------------------------------------------------------------------------
// GraphExplainPlan
// ---------------------------------------------------------------------------

struct GraphExplainPlan {
    std::string query;
    std::string plan_id;
    std::vector<GraphPlanNode> nodes;
    std::string root_node_id;
    double total_estimated_cost = 0.0;
    double total_actual_ms      = -1.0;
    bool is_analyzed            = false;

    /**
     * @brief To Dot.
     * @return Return value.
     */
    std::string toDot() const;

    /**
     * @brief To Json.
     * @return Return value.
     */
    std::string toJson() const;
};

// ---------------------------------------------------------------------------
// IGraphExplainProvider
// ---------------------------------------------------------------------------

class IGraphExplainProvider {
public:
    /**
     * @brief IGraph Explain Provider.
     * @return Return value.
     */
    virtual ~IGraphExplainProvider() = default;

    virtual GraphExplainPlan explain(
        const std::string& query,
        const std::map<std::string, std::string>& params = {}) = 0;

    virtual GraphExplainPlan explainAnalyze(
        const std::string& query,
        const std::map<std::string, std::string>& params = {}) = 0;
};

} // namespace graph
} // namespace themis
