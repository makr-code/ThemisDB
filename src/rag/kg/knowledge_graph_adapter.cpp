#include "themis/rag/kg/knowledge_graph_interface.h"
#include "rag/knowledge_graph_retriever.h"

namespace themis::rag::kg {

class KnowledgeGraphAdapter : public IKnowledgeGraph {
public:
    explicit KnowledgeGraphAdapter(const KnowledgeGraph& kg) : kg_(kg) {}

    std::optional<KGNode> findNode(const std::string& node_id) const override {
        const KGNode* p = kg_.findNode(node_id);
        if (!p) return std::nullopt;
        return *p;
    }

    std::optional<KGNode> findNodeByName(const std::string& text) const override {
        const KGNode* p = kg_.findNodeByName(text);
        if (!p) return std::nullopt;
        return *p;
    }

    size_t nodeCount() const override { return kg_.nodeCount(); }

    std::unordered_set<std::string> neighbours(
        const std::string& start_id,
        size_t max_depth = 1,
        double min_edge_weight = 0.0,
        size_t max_nodes = 4096) const override
    {
        return kg_.neighbours(start_id, max_depth, min_edge_weight, max_nodes);
    }

    std::vector<KGEdge> outEdges(const std::string& node_id) const override {
        return kg_.outEdges(node_id);
    }

private:
    const KnowledgeGraph& kg_;
};

std::shared_ptr<IKnowledgeGraph> makeIKnowledgeGraph(const KnowledgeGraph& kg) {
    return std::make_shared<KnowledgeGraphAdapter>(kg);
}

} // namespace themis::rag::kg
