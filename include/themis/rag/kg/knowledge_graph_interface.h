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
    virtual ~IKnowledgeGraph() = default;

    virtual std::optional<KGNode> findNode(const std::string& node_id) const = 0;
    virtual std::optional<KGNode> findNodeByName(const std::string& text) const = 0;
    virtual size_t nodeCount() const = 0;
    virtual std::unordered_set<std::string> neighbours(
        const std::string& start_id,
        size_t max_depth = 1,
        double min_edge_weight = 0.0,
        size_t max_nodes = 4096) const = 0;
    virtual std::vector<KGEdge> outEdges(const std::string& node_id) const = 0;
};

    // Adapter factory: create an `IKnowledgeGraph` view for a concrete
    // `KnowledgeGraph` instance. Definition lives in src/rag/kg/knowledge_graph_adapter.cpp
    std::shared_ptr<IKnowledgeGraph> makeIKnowledgeGraph(const KnowledgeGraph& kg);

} // namespace themis::rag::kg
