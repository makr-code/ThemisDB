/**
 * @file ontology_aware_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "rag/knowledge_graph_retriever.h"
#include "themis/rag/kg/knowledge_graph_interface.h"
#include "graph/ontology_manager.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// OntologyAwareRetriever
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for OntologyAwareRetriever.
 */
struct OntologyRetrieverConfig {
    /// Maximum BFS hops in the knowledge graph (passed to KGRetrieverConfig).
    std::size_t max_traversal_depth = 2;

    /// KG score fusion weight [0, 1].
    double kg_score_weight = 0.3;

    /// Maximum concept-expansion ancestors to include per entity concept.
    /// Bounds the number of superclass IDs added to the alias lookup set.
    std::size_t max_superclass_expansion = 10;

    /// If true, filter KG traversal to only edges allowed by the ontology's
    /// axioms (via OntologyManager::allowedEdgeTypes).  When false, all edges
    /// with sufficient weight are followed (same as plain KGRetriever).
    bool filter_by_allowed_edge_types = true;

    /// Entity linker configuration forwarded to the underlying KGRetriever.
    kg::EntityLinkerConfig linker_config;
};

/**
 * @brief Augmented retrieval result from OntologyAwareRetriever.
 */
struct OntologyRetrievalResult {
    /// Re-scored candidate documents, sorted by final_score DESC.
    std::vector<kg::KGAugmentedDocument> documents;

    /// Query entity links (same as KGRetrievalResult::query_entity_links).
    std::vector<kg::EntityLinkingMatch> query_entity_links;

    /// Concept IDs found for query entities after ontology expansion.
    std::unordered_set<std::string> expanded_concepts;

    /// KG node IDs visited during traversal.
    std::unordered_set<std::string> visited_nodes;

    /// Fraction of query entities that matched a KG node.
    double entity_linking_coverage = 0.0;

    /// Wall-clock time for the full pass (ms).
    double elapsed_ms = 0.0;
};

/**
 * @brief Knowledge-graph retriever with ontology-guided entity expansion.
 *
 * @par Overview
 * Wraps `kg::KnowledgeGraphRetriever` and adds two ontology-driven steps:
 *  1. **Entity expansion**: each linked entity concept is expanded via
 *     `OntologyManager::isA()` to include its superclass chain up to
 *     `max_superclass_expansion` ancestors.  The expanded concept set is
 *     used to widen the KG neighbourhood query.
 *  2. **Edge-type filtering** (optional): KG BFS only follows edges whose
 *     relation type is permitted by `OntologyManager::allowedEdgeTypes()`
 *     for the current source and target node types.  Edges whose type is
 *     unknown to the ontology are passed through (graceful degradation).
 *
 * When the ontology is not loaded or a concept is unknown the retriever
 * degrades gracefully to standard KG retrieval without throwing.
 *
 * @par Thread safety
 * Retrieve is const and thread-safe for concurrent calls once constructed.
 *
 * @par Usage example
 * @code{.cpp}
 *   kg::KnowledgeGraph graph;
 *   // ... populate graph ...
 *
 *   themis::graph::OntologyManager ontology;
 *   ontology.loadFromJsonString(R"({"concepts":[{"id":"Entity"},
 *     {"id":"Person","parents":["Entity"]}],"axioms":[]})");
 *   ontology.build();
 *
 *   OntologyAwareRetriever retriever(graph, ontology);
 *   auto result = retriever.retrieve(query, candidates);
 * @endcode
 */
class OntologyAwareRetriever {
public:
    /**
     * @brief Construct with a knowledge graph, ontology, and optional config.
     *
     * @param graph    Knowledge graph (must outlive this retriever).
     * @param ontology OntologyManager (must outlive this retriever).
     * @param config   Retrieval configuration.
     */
    explicit OntologyAwareRetriever(
        const kg::KnowledgeGraph&        graph,
        const graph::OntologyManager&    ontology,
        const OntologyRetrieverConfig&   config = {});

    /**
     * @brief Construct from an abstract knowledge-graph interface.
     *
     * Prefer this constructor when callers only have an `IKnowledgeGraph`
     * implementation (runtime factories / adapters). The retriever will use
     * the provided interface methods for traversal and linking.
     */
    explicit OntologyAwareRetriever(
        std::shared_ptr<kg::IKnowledgeGraph> graph,
        const graph::OntologyManager&       ontology,
        const OntologyRetrieverConfig&      config = {});

    ~OntologyAwareRetriever();

    OntologyAwareRetriever(const OntologyAwareRetriever&)            = delete;
    OntologyAwareRetriever& operator=(const OntologyAwareRetriever&) = delete;
    OntologyAwareRetriever(OntologyAwareRetriever&&)                 = default;
    OntologyAwareRetriever& operator=(OntologyAwareRetriever&&)      = default;

    /**
     * @brief Augment @p candidates using ontology-aware KG signal.
     *
     * @param query      Original user query text.
     * @param candidates Initial retrieval results.
     * @return           Ontology-augmented and re-ranked documents.
     */
    [[nodiscard]] OntologyRetrievalResult retrieve(
        const std::string&                               query,
        const std::vector<judge::RetrievedDocument>&     candidates) const;

    /**
     * @brief Return the current configuration.
     */
    const OntologyRetrieverConfig& config() const noexcept;

    /**
     * @brief Replace configuration.
     */
    void setConfig(const OntologyRetrieverConfig& config);

private:
    /// Expand a concept ID to its ancestor chain (up to max_superclass_expansion).
    [[nodiscard]] std::unordered_set<std::string> expandConcept(
        const std::string& concept_id) const;

    /// Test whether an edge relation type is allowed between two node type names.
    /// Returns true when either class is unknown (graceful degradation).
    [[nodiscard]] bool isEdgeAllowed(
        const std::string& from_type,
        const std::string& to_type,
        const std::string& edge_type_name) const;

    // If constructed from a concrete KnowledgeGraph, `raw_graph_` points
    // to it and `graph_iface_` may be empty. If constructed from an
    // IKnowledgeGraph, `graph_iface_` is used for traversal.
    const kg::KnowledgeGraph*                 raw_graph_ = nullptr;
    std::shared_ptr<kg::IKnowledgeGraph>      graph_iface_;
    const graph::OntologyManager&             ontology_;
    OntologyRetrieverConfig       config_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory helpers for common OntologyAwareRetriever configurations.
 */
class OntologyAwareRetrieverFactory {
public:
    /**
     * @brief Shallow: 1-hop, low KG weight (0.2), edge-type filtering disabled.
     */
    static std::unique_ptr<OntologyAwareRetriever> createShallow(
        const kg::KnowledgeGraph&     graph,
        const graph::OntologyManager& ontology);

    static std::unique_ptr<OntologyAwareRetriever> createShallow(
        std::shared_ptr<kg::IKnowledgeGraph> graph,
        const graph::OntologyManager&        ontology);

    /**
     * @brief Balanced: 2-hop, moderate KG weight (0.3), edge-type filtering enabled.
     */
    static std::unique_ptr<OntologyAwareRetriever> createBalanced(
        const kg::KnowledgeGraph&     graph,
        const graph::OntologyManager& ontology);

    static std::unique_ptr<OntologyAwareRetriever> createBalanced(
        std::shared_ptr<kg::IKnowledgeGraph> graph,
        const graph::OntologyManager&        ontology);

    /**
     * @brief Deep: 3-hop, higher KG weight (0.45), edge-type filtering enabled.
     */
    static std::unique_ptr<OntologyAwareRetriever> createDeep(
        const kg::KnowledgeGraph&     graph,
        const graph::OntologyManager& ontology);

    static std::unique_ptr<OntologyAwareRetriever> createDeep(
        std::shared_ptr<kg::IKnowledgeGraph> graph,
        const graph::OntologyManager&        ontology);
};

} // namespace themis::rag
