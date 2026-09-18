#pragma once

#include "rag/knowledge_graph_retriever.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis::rag::kg {

class IKnowledgeGraph {
public:
    /**
     * @brief TBD: Describe ~IKnowledgeGraph.
     * @return Return value.
     */
    virtual ~IKnowledgeGraph() = default;

    /**
     * @brief TBD: Describe findNode.
     * @param[in] node_id Input parameter.
     * @return Return value.
     */
    virtual std::optional<KGNode> findNode(const std::string& node_id) const = 0;
    /**
     * @brief TBD: Describe findNodeByName.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    virtual std::optional<KGNode> findNodeByName(const std::string& text) const = 0;
    /**
     * @brief TBD: Describe nodeCount.
     * @return Return value.
     */
    virtual size_t nodeCount() const = 0;
    virtual std::unordered_set<std::string> neighbours(
        const std::string& start_id,
        size_t max_depth = 1,
        double min_edge_weight = 0.0,
        size_t max_nodes = 4096) const = 0;
    /**
     * @brief TBD: Describe outEdges.
     * @param[in] node_id Input parameter.
     * @return Return value.
     */
    virtual std::vector<KGEdge> outEdges(const std::string& node_id) const = 0;
};

    /**
     * @brief Adapter factory: create an `IKnowledgeGraph` view for a concrete `KnowledgeGraph` instance.
     * @param[in] kg Input parameter.
     * @return Return value.
     * @details Definition lives in src/rag/kg/knowledge_graph_adapter.cpp
     */
    std::shared_ptr<IKnowledgeGraph> makeIKnowledgeGraph(const KnowledgeGraph& kg);

} // namespace themis::rag::kg
