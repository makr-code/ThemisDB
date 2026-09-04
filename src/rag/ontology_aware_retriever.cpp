/**
 * @file ontology_aware_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "rag/ontology_aware_retriever.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Convert kg::RelationType to a string name used by OntologyManager axioms.
std::string relationTypeName(kg::RelationType rel) {
    switch (rel) {
        case kg::RelationType::IS_A:       return "IS_A";
        case kg::RelationType::HAS_PART:   return "HAS_PART";
        case kg::RelationType::RELATED_TO: return "RELATED_TO";
        case kg::RelationType::CAUSES:     return "CAUSES";
        case kg::RelationType::MENTIONS:   return "MENTIONS";
        case kg::RelationType::SYNONYM_OF: return "SYNONYM_OF";
        case kg::RelationType::DEFINED_BY: return "DEFINED_BY";
    }
    return "RELATED_TO";
}

/// Convert EntityType to an ontology concept name used for axiom lookup.
std::string entityTypeName(kg::EntityType type) {
    switch (type) {
        case kg::EntityType::PERSON:       return "Person";
        case kg::EntityType::ORGANIZATION: return "Organization";
        case kg::EntityType::LOCATION:     return "Location";
        case kg::EntityType::CONCEPT:      return "Concept";
        case kg::EntityType::PRODUCT:      return "Product";
        case kg::EntityType::EVENT:        return "Event";
        case kg::EntityType::OTHER:        return "Other";
    }
    return "Other";
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// OntologyAwareRetriever — life-cycle
// ─────────────────────────────────────────────────────────────────────────────

OntologyAwareRetriever::OntologyAwareRetriever(
    const kg::KnowledgeGraph&      graph,
    const graph::OntologyManager&  ontology,
    const OntologyRetrieverConfig& config)
    : raw_graph_(&graph)
    , graph_iface_(kg::makeIKnowledgeGraph(graph))
    , ontology_(ontology)
    , config_(config)
{}

OntologyAwareRetriever::OntologyAwareRetriever(
    std::shared_ptr<kg::IKnowledgeGraph> graph,
    const graph::OntologyManager&       ontology,
    const OntologyRetrieverConfig&      config)
    : raw_graph_(nullptr)
    , graph_iface_(std::move(graph))
    , ontology_(ontology)
    , config_(config)
{}

OntologyAwareRetriever::~OntologyAwareRetriever() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

const OntologyRetrieverConfig& OntologyAwareRetriever::config() const noexcept {
    return config_;
}

void OntologyAwareRetriever::setConfig(const OntologyRetrieverConfig& config) {
    config_ = config;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::unordered_set<std::string>
OntologyAwareRetriever::expandConcept(const std::string& concept_id) const {
    std::unordered_set<std::string> expanded;
    expanded.insert(concept_id);

    if (!ontology_.isBuilt()) {
        return expanded;
    }

    // BFS over parent chain until max_superclass_expansion ancestors found.
    std::vector<std::string> queue = {concept_id};
    std::unordered_set<std::string> visited = {concept_id};

    while (!queue.empty() &&
           static_cast<int>(expanded.size()) < config_.max_superclass_expansion + 1) {
        const std::string current = queue.back();
        queue.pop_back();

        const auto* node = ontology_.getConcept(current);
        if (!node) {
          continue;
        }

        for (const auto& parent : node->parents) {
            if (visited.insert(parent).second) {
                expanded.insert(parent);
                queue.push_back(parent);
            }
        }
    }

    return expanded;
}

bool OntologyAwareRetriever::isEdgeAllowed(
    const std::string& from_type,
    const std::string& to_type,
    const std::string& edge_type_name) const
{
    if (!config_.filter_by_allowed_edge_types || !ontology_.isBuilt()) {
        return true;
    }
    // Graceful degradation: if either concept is unknown, allow the edge.
    if (!ontology_.hasConcept(from_type) || !ontology_.hasConcept(to_type)) {
        return true;
    }
    return ontology_.isEdgeTypeAllowed(from_type, to_type, edge_type_name);
}

// ─────────────────────────────────────────────────────────────────────────────
// OntologyAwareRetriever::retrieve
// ─────────────────────────────────────────────────────────────────────────────

OntologyRetrievalResult OntologyAwareRetriever::retrieve(
    const std::string&                           query,
    const std::vector<judge::RetrievedDocument>& candidates) const
{
    const auto t0 = std::chrono::steady_clock::now();

    OntologyRetrievalResult result;

    // ── Step 1: run the base KG retriever ────────────────────────────────────
    kg::KGRetrieverConfig kg_cfg;
    kg_cfg.max_traversal_depth = config_.max_traversal_depth;
    kg_cfg.kg_score_weight     = config_.kg_score_weight;
    kg_cfg.linker_config       = config_.linker_config;

    kg::KGRetrievalResult base_result;
    if (raw_graph_) {
        kg::KnowledgeGraphRetriever base_retriever(*raw_graph_, kg_cfg);
        base_result = base_retriever.retrieve(query, candidates);
    } else if (graph_iface_) {
        // When an IKnowledgeGraph is provided, construct a retriever that
        // operates over the interface implementation.
        kg::KnowledgeGraphRetriever base_retriever(graph_iface_, kg_cfg);
        base_result = base_retriever.retrieve(query, candidates);
    }

    result.documents              = std::move(base_result.documents);
    result.query_entity_links     = std::move(base_result.query_entity_links);
    result.visited_nodes          = std::move(base_result.visited_nodes);
    result.entity_linking_coverage = base_result.entity_linking_coverage;

    // ── Step 2: ontology expansion of linked concepts ────────────────────────
    for (const auto& link : result.query_entity_links) {
        if (!link.is_linked) {
          continue;
        }

        // Map entity type to ontology concept name for expansion.
        const std::string concept_name = entityTypeName(link.entity.type);
        auto superclasses = expandConcept(concept_name);
        for (auto& sc : superclasses) {
            result.expanded_concepts.insert(std::move(sc));
        }
    }

    // ── Step 3: re-score documents applying ontology-filtered edge signal ────
    // For each document, check whether its entity links point to nodes
    // reachable via ontology-allowed edges from the query node set.
    // Documents whose edges are disallowed by the ontology get a penalty.
    if (config_.filter_by_allowed_edge_types && ontology_.isBuilt()) {
        for (auto& aug_doc : result.documents) {
            double penalty = 0.0;
            for (const auto& doc_link : aug_doc.entity_links) {
                if (!doc_link.is_linked) {
                  continue;
                }
                // Get all outgoing edges from this node and check axioms.
                std::vector<kg::KGEdge> out_edges = {};

                if (graph_iface_) {
                    out_edges = graph_iface_->outEdges(doc_link.node_id);
                } else if (raw_graph_) {
                    out_edges = raw_graph_->outEdges(doc_link.node_id);
                }
                for (const auto& edge : out_edges) {
                    const kg::KGNode* src_node = nullptr;
                    const kg::KGNode* tgt_node = nullptr;
                    if (graph_iface_) {
                        auto s = graph_iface_->findNode(edge.from_id);
                        auto t = graph_iface_->findNode(edge.to_id);
                        if (s) {
                          src_node = &(*s);
                        }
                        if (t) {
                          tgt_node = &(*t);
                        }
                    } else if (raw_graph_) {
                        auto src = raw_graph_->findNode(edge.from_id);
                        auto tgt = raw_graph_->findNode(edge.to_id);
                        if (src) {
                          src_node = src;
                        }
                        if (tgt) {
                          tgt_node = tgt;
                        }
                    }
                    if (!src_node || !tgt_node) {
                      continue;
                    }

                    const std::string src_type = entityTypeName(src_node->type);
                    const std::string tgt_type = entityTypeName(tgt_node->type);
                    const std::string rel_name = relationTypeName(edge.relation);

                    if (!isEdgeAllowed(src_type, tgt_type, rel_name)) {
                        // Edge not sanctioned by ontology — apply a small penalty.
                        penalty += 0.05 * edge.weight;
                    }
                }
            }
            // Clamp score to [0, 1].
            aug_doc.final_score = std::max(0.0, aug_doc.final_score - penalty);
        }

        // Re-sort after score adjustment.
        std::sort(result.documents.begin(), result.documents.end(),
                  [](const kg::KGAugmentedDocument& a,
                     const kg::KGAugmentedDocument& b) {
                      return a.final_score > b.final_score;
                  });
    }

    const auto t1 = std::chrono::steady_clock::now();
    result.elapsed_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

/*static*/
std::unique_ptr<OntologyAwareRetriever>
OntologyAwareRetrieverFactory::createShallow(
    const kg::KnowledgeGraph&     graph,
    const graph::OntologyManager& ontology)
{
    OntologyRetrieverConfig cfg;
    cfg.max_traversal_depth        = 1;
    cfg.kg_score_weight            = 0.2;
    cfg.filter_by_allowed_edge_types = false;
    return std::make_unique<OntologyAwareRetriever>(graph, ontology, cfg);
}

std::unique_ptr<OntologyAwareRetriever>
OntologyAwareRetrieverFactory::createShallow(
    std::shared_ptr<kg::IKnowledgeGraph> graph,
    const graph::OntologyManager&        ontology)
{
    OntologyRetrieverConfig cfg;
    cfg.max_traversal_depth        = 1;
    cfg.kg_score_weight            = 0.2;
    cfg.filter_by_allowed_edge_types = false;
    return std::make_unique<OntologyAwareRetriever>(std::move(graph), ontology, cfg);
}

/*static*/
std::unique_ptr<OntologyAwareRetriever>
OntologyAwareRetrieverFactory::createBalanced(
    const kg::KnowledgeGraph&     graph,
    const graph::OntologyManager& ontology)
{
    OntologyRetrieverConfig cfg;
    cfg.max_traversal_depth        = 2;
    cfg.kg_score_weight            = 0.3;
    cfg.filter_by_allowed_edge_types = true;
    return std::make_unique<OntologyAwareRetriever>(graph, ontology, cfg);
}

std::unique_ptr<OntologyAwareRetriever>
OntologyAwareRetrieverFactory::createBalanced(
    std::shared_ptr<kg::IKnowledgeGraph> graph,
    const graph::OntologyManager&        ontology)
{
    OntologyRetrieverConfig cfg;
    cfg.max_traversal_depth        = 2;
    cfg.kg_score_weight            = 0.3;
    cfg.filter_by_allowed_edge_types = true;
    return std::make_unique<OntologyAwareRetriever>(std::move(graph), ontology, cfg);
}

/*static*/
std::unique_ptr<OntologyAwareRetriever>
OntologyAwareRetrieverFactory::createDeep(
    const kg::KnowledgeGraph&     graph,
    const graph::OntologyManager& ontology)
{
    OntologyRetrieverConfig cfg;
    cfg.max_traversal_depth        = 3;
    cfg.kg_score_weight            = 0.45;
    cfg.max_superclass_expansion   = 20;
    cfg.filter_by_allowed_edge_types = true;
    return std::make_unique<OntologyAwareRetriever>(graph, ontology, cfg);
}

std::unique_ptr<OntologyAwareRetriever>
OntologyAwareRetrieverFactory::createDeep(
    std::shared_ptr<kg::IKnowledgeGraph> graph,
    const graph::OntologyManager&        ontology)
{
    OntologyRetrieverConfig cfg;
    cfg.max_traversal_depth        = 3;
    cfg.kg_score_weight            = 0.45;
    cfg.max_superclass_expansion   = 20;
    cfg.filter_by_allowed_edge_types = true;
    return std::make_unique<OntologyAwareRetriever>(std::move(graph), ontology, cfg);
}

} // namespace themis::rag
