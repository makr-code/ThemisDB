/**
 * @file knowledge_graph_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include "graph/knowledge_graph_reasoner.h"
// Forward-declare IKnowledgeGraph to avoid a circular include. The full
// definition lives in themis/rag/kg/knowledge_graph_interface.h.
namespace themis::rag::kg { class IKnowledgeGraph; }

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis::rag::kg {

// ─────────────────────────────────────────────────────────────────────────────
// Supporting enumerations
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Coarse category of a named entity.
 */
enum class EntityType {
    PERSON,        ///< A human individual
    ORGANIZATION,  ///< Company, institute, government body
    LOCATION,      ///< Geographical location, place
    CONCEPT,       ///< Abstract idea, algorithm, topic
    PRODUCT,       ///< Software, hardware, brand
    EVENT,         ///< Named event (conference, release, …)
    OTHER          ///< Catch-all for unclassified entities
};

/**
 * @brief Directed relation type between two knowledge-graph nodes.
 */
enum class RelationType {
    IS_A,        ///< Subtype / instance-of relationship
    HAS_PART,    ///< Meronymy (X has-part Y)
    RELATED_TO,  ///< Generic semantic relatedness
    CAUSES,      ///< Causal link (X causes Y)
    MENTIONS,    ///< Document-level mention link
    SYNONYM_OF,  ///< Lexical synonymy
    DEFINED_BY   ///< X is defined / described by Y
};

// ─────────────────────────────────────────────────────────────────────────────
// Data structures
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A named entity extracted from text.
 */
struct Entity {
    std::string  text;          ///< Surface form as it appears in the text
    EntityType   type;          ///< Coarse semantic category
    double       confidence;    ///< Extraction confidence in [0, 1]
    size_t       start_offset;  ///< Byte offset of first character in source text
    size_t       end_offset;    ///< Byte offset one past the last character
};

/**
 * @brief A node in the knowledge graph representing a canonical entity.
 */
struct KGNode {
    std::string              id;             ///< Unique stable identifier
    std::string              canonical_name; ///< Preferred display name
    std::vector<std::string> aliases;        ///< Alternative surface forms
    EntityType               type;           ///< Entity type
    /// Arbitrary key-value metadata (e.g. description, source, external URI).
    std::unordered_map<std::string, std::string> properties;
};

/**
 * @brief A directed, weighted edge in the knowledge graph.
 */
struct KGEdge {
    std::string  from_id;  ///< Source node identifier
    std::string  to_id;    ///< Target node identifier
    RelationType relation; ///< Semantic relation type
    double       weight;   ///< Edge weight / confidence in [0, 1]
};

/**
 * @brief Result of linking a single extracted entity to a KG node.
 */
struct EntityLinkingMatch {
    Entity      entity;          ///< The extracted surface entity
    std::string node_id;         ///< Matched KG node identifier (empty if no match)
    double      linking_score;   ///< Similarity score used for the link [0, 1]
    bool        is_linked;       ///< True when a match was found
};

/**
 * @brief A retrieved document augmented with entity-linking information.
 */
struct KGAugmentedDocument {
    judge::RetrievedDocument     document;          ///< Original retrieved document
    std::vector<EntityLinkingMatch> entity_links;   ///< Entities found in this document
    double                       kg_boost;          ///< Score boost from graph traversal
    double                       final_score;       ///< Fused relevance score
};

/**
 * @brief Complete result of one KG-augmented retrieval pass.
 */
struct KGRetrievalResult {
    /// Augmented and re-scored candidate documents, sorted by final_score DESC.
    std::vector<KGAugmentedDocument> documents;

    /// Entity links found in the query.
    std::vector<EntityLinkingMatch> query_entity_links;

    /// KG node IDs visited during graph traversal.
    std::unordered_set<std::string> visited_nodes;

    /// Fraction of query entities successfully linked to KG nodes.
    double entity_linking_coverage = 0.0;

    /// Wall-clock time for the entire augmentation pass (milliseconds).
    double elapsed_ms = 0.0;

    // ── Reasoning-chain fields (populated when a KnowledgeGraphReasoner is set) ──

    /// Inference chains derived during this retrieval pass (one per linked query entity).
    std::vector<graph::InferenceChain> inference_chains;

    /// True when at least one inference chain produced derived triples.
    bool has_reasoning = false;

    /// Time spent on KnowledgeGraphReasoner::infer() calls (ms).
    double reasoning_elapsed_ms = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for the entity linker.
 */
struct EntityLinkerConfig {
    /// Minimum token length to be considered a candidate entity (chars).
    size_t min_entity_length = 3;

    /// Minimum linking score for a match to be accepted [0, 1].
    double min_linking_score = 0.5;

    /// Maximum number of entities to extract per text block.
    size_t max_entities_per_text = 20;

    /// When true, also extract single capitalised words as potential entities
    /// (in addition to multi-word capitalised sequences).
    bool extract_single_word_entities = true;
};

/**
 * @brief Configuration for KnowledgeGraphRetriever.
 */
struct KGRetrieverConfig {
    /// Maximum BFS hops from a query-linked node during graph traversal.
    size_t max_traversal_depth = 2;

    /// Minimum edge weight to follow during traversal [0, 1].
    double min_edge_weight = 0.3;

    /// Weight given to the KG-derived score when fusing with the original
    /// vector-search score.  Final score = orig * (1 - kg_score_weight)
    ///                                   + kg_score * kg_score_weight.
    double kg_score_weight = 0.3;

    /// Maximum number of unique nodes to visit per query (circuit-breaker).
    size_t max_nodes_visited = 256;

    /// Entity linker configuration.
    EntityLinkerConfig linker_config;

    // ── KnowledgeGraphReasoner integration ───────────────────────────────────

    /// Maximum inference hops when a KnowledgeGraphReasoner is attached.
    /// Set to 0 to disable reasoning even when a reasoner is registered.
    int max_inference_hops = 5;

    /// When true, the serialised InferenceChain for each query entity is
    /// stored in `RetrievedDocument::metadata["reasoning_chain"]`.
    bool attach_reasoning_chain_to_metadata = true;

    /// Timeout budget for reasoning per query entity (ms).  Inference that
    /// exceeds this budget is skipped gracefully (fallback to direct KG query).
    double reasoning_timeout_ms = 200.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeGraph
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory knowledge graph.
 *
 * Stores nodes and directed edges and exposes BFS neighbourhood queries.
 *
 * Usage:
 * @code
 *   KnowledgeGraph g;
 *   g.addNode({"n1", "HNSW", {}, EntityType::CONCEPT});
 *   g.addNode({"n2", "ANN Search", {}, EntityType::CONCEPT});
 *   g.addEdge({"n1", "n2", RelationType::RELATED_TO, 0.8});
 *
 *   auto neighbours = g.neighbours("n1", 1, 0.5);
 * @endcode
 */
class KnowledgeGraph {
public:
    KnowledgeGraph();
    ~KnowledgeGraph();

    KnowledgeGraph(const KnowledgeGraph&)            = delete;
    KnowledgeGraph& operator=(const KnowledgeGraph&) = delete;
    KnowledgeGraph(KnowledgeGraph&&);
    KnowledgeGraph& operator=(KnowledgeGraph&&);

    // ── Node management ────────────────────────────────────────────────────

    /**
     * @brief Add or replace a node.  Thread-safe.
     * @param node The node to insert.
     */
    void addNode(KGNode node);

    /**
     * @brief Remove a node and all edges incident to it.  Thread-safe.
     * @param node_id Identifier of the node to remove.
     * @return true if the node existed.
     */
    bool removeNode(const std::string& node_id);

    /**
     * @brief Look up a node by identifier.  Thread-safe.
     * @param node_id Identifier.
     * @return Pointer to the node, or nullptr if not found.
     */
    const KGNode* findNode(const std::string& node_id) const;

    /**
     * @brief Find a node whose canonical_name or aliases match @p text
     *        (case-insensitive normalised comparison).  Thread-safe.
     * @param text Surface form to search.
     * @return Pointer to the best-matching node, or nullptr.
     */
    const KGNode* findNodeByName(const std::string& text) const;

    /**
     * @brief Return the number of nodes.
     */
    size_t nodeCount() const;

    // ── Edge management ────────────────────────────────────────────────────

    /**
     * @brief Add a directed edge.  Thread-safe.
     * @param edge Edge to insert.
     */
    void addEdge(KGEdge edge);

    /**
     * @brief Remove a specific directed edge.  Thread-safe.
     * @return true if the edge existed.
     */
    bool removeEdge(const std::string& from_id, const std::string& to_id,
                    RelationType relation);

    /**
     * @brief Return the number of edges.
     */
    size_t edgeCount() const;

    // ── Traversal ──────────────────────────────────────────────────────────

    /**
     * @brief BFS neighbourhood starting from @p start_id.
     *
     * Returns all nodes reachable within @p max_depth hops over edges whose
     * weight is at least @p min_edge_weight.
     *
     * @param start_id       Starting node identifier.
     * @param max_depth      Maximum BFS depth (1 = direct neighbours only).
     * @param min_edge_weight Minimum edge weight to follow.
     * @param max_nodes      Maximum number of nodes to visit (DoS guard, GAP-010).
     *                       The BFS terminates once this many nodes have been
     *                       enqueued, preventing unbounded traversal of dense
     *                       graphs.  Defaults to 4096.
     * @return               Set of reachable node IDs (excluding start_id).
     */
    std::unordered_set<std::string> neighbours(
        const std::string& start_id,
        size_t             max_depth      = 1,
        double             min_edge_weight = 0.0,
        size_t             max_nodes      = 4096) const;

    /**
     * @brief Return all outgoing edges from @p node_id.  Thread-safe.
     */
    std::vector<KGEdge> outEdges(const std::string& node_id) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// EntityLinker
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Extracts named entities from text and links them to KG nodes.
 *
 * Entity extraction uses two complementary strategies:
 *  1. **Capitalisation heuristic**: sequences of capitalised words are
 *     treated as entity candidates.
 *  2. **Dictionary lookup**: nodes registered in the KnowledgeGraph are
 *     matched by their canonical names and aliases via normalised string
 *     comparison (case-fold + whitespace collapse).
 *
 * Linking uses normalised string similarity: exact matches score 1.0,
 * prefix matches are penalised proportionally to the length difference.
 */
class EntityLinker {
public:
    /**
     * @brief Construct with a reference to the knowledge graph and config.
     * @param graph  Knowledge graph to use for dictionary-based linking.
     * @param config Linker configuration.
     */
    explicit EntityLinker(const KnowledgeGraph&   graph,
                          const EntityLinkerConfig& config = {});

    explicit EntityLinker(std::shared_ptr<IKnowledgeGraph> graph,
                          const EntityLinkerConfig& config = {});

    /**
     * @brief Extract entities from @p text and link them to KG nodes.
     * @param text Source text.
     * @return     Linking matches, one per extracted entity candidate.
     */
    std::vector<EntityLinkingMatch> link(const std::string& text) const;

private:
    // When constructed from a concrete KnowledgeGraph, `raw_graph_` points
    // to it. When constructed from an interface, `graph_iface_` is used.
    const KnowledgeGraph*                 raw_graph_ = nullptr;
    std::shared_ptr<IKnowledgeGraph>      graph_iface_;
    EntityLinkerConfig     config_;

    /// Extract raw entity candidates (surface spans) from text.
    std::vector<Entity> extract(const std::string& text) const;

    /// Compute normalised string similarity between two strings.
    static double similarity(const std::string& a, const std::string& b);

    /// Case-fold and collapse whitespace for comparison.
    static std::string normalise(const std::string& s);
};

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeGraphRetriever
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Augments a set of retrieved documents using a knowledge graph.
 *
 * The retriever:
 *  1. Links query entities to KG nodes.
 *  2. Performs BFS traversal from linked nodes to collect a neighbourhood.
 *  3. For each candidate document, links its entities to the same KG.
 *  4. Computes a KG boost score = overlap between document entity neighbourhood
 *     and query neighbourhood (normalised Jaccard).
 *  5. Fuses the original similarity score with the KG boost using
 *     @c KGRetrieverConfig::kg_score_weight.
 *  6. Returns documents sorted by final fused score.
 *
 * Performance targets:
 *  - <10 ms for 100 candidates with graph of 1 000 nodes / 5 000 edges.
 *  - Linear in |candidates| × average entity count per document.
 */
class KnowledgeGraphRetriever {
public:
    /**
     * @brief Construct with a shared knowledge graph and configuration.
     * @param graph  Knowledge graph (must outlive this retriever).
     * @param config Retrieval configuration.
     */
    explicit KnowledgeGraphRetriever(const KnowledgeGraph&   graph,
                                     const KGRetrieverConfig& config = {});

    explicit KnowledgeGraphRetriever(std::shared_ptr<IKnowledgeGraph> graph,
                                     const KGRetrieverConfig& config = {});

    ~KnowledgeGraphRetriever();

    KnowledgeGraphRetriever(const KnowledgeGraphRetriever&)            = delete;
    KnowledgeGraphRetriever& operator=(const KnowledgeGraphRetriever&) = delete;
    KnowledgeGraphRetriever(KnowledgeGraphRetriever&&)                 = default;
    KnowledgeGraphRetriever& operator=(KnowledgeGraphRetriever&&)      = default;

    /**
     * @brief Augment @p candidates with knowledge graph signal.
     *
     * @param query      Original user query.
     * @param candidates Initial retrieval results (e.g. from vector search).
     * @return           KG-augmented and re-ranked documents.
     */
    KGRetrievalResult retrieve(
        const std::string&                           query,
        const std::vector<judge::RetrievedDocument>& candidates) const;

    /**
     * @brief Access current configuration.
     */
    const KGRetrieverConfig& getConfig() const;

    /**
     * @brief Replace configuration.
     */
    void setConfig(const KGRetrieverConfig& config);

    // ── KnowledgeGraphReasoner integration ────────────────────────────────────

    /**
     * @brief Attach a KnowledgeGraphReasoner for multi-hop inference.
     *
     * When set, `retrieve()` calls `KnowledgeGraphReasoner::infer()` for each
     * linked query entity and includes the resulting `InferenceChain` in the
     * `KGRetrievalResult::inference_chains` list.  When
     * `KGRetrieverConfig::attach_reasoning_chain_to_metadata` is true, the
     * chain is also serialised into
     * `RetrievedDocument::metadata["reasoning_chain"]`.
     *
     * Pass `nullptr` to detach the current reasoner.
     *
     * @param reasoner  Pointer to a fully configured `KnowledgeGraphReasoner`
     *                  (must outlive this retriever or be detached first).
     */
    void setReasoner(graph::KnowledgeGraphReasoner* reasoner);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory helpers for common KG retriever configurations.
 */
class KnowledgeGraphRetrieverFactory {
public:
    /**
     * @brief Shallow retriever: 1-hop traversal, low KG weight (0.2).
     * Best for: quick augmentation with minimal latency overhead.
     */
    static std::unique_ptr<KnowledgeGraphRetriever> createShallow(
        const KnowledgeGraph& graph);

    /**
     * @brief Balanced retriever: 2-hop traversal, moderate KG weight (0.3).
     * Best for: general-purpose knowledge-graph augmented RAG.
     */
    static std::unique_ptr<KnowledgeGraphRetriever> createBalanced(
        const KnowledgeGraph& graph);

    static std::unique_ptr<KnowledgeGraphRetriever> createShallow(
        std::shared_ptr<IKnowledgeGraph> graph);

    static std::unique_ptr<KnowledgeGraphRetriever> createBalanced(
        std::shared_ptr<IKnowledgeGraph> graph);

    static std::unique_ptr<KnowledgeGraphRetriever> createDeep(
        std::shared_ptr<IKnowledgeGraph> graph);

    /**
     * @brief Deep retriever: 3-hop traversal, higher KG weight (0.45).
     * Best for: entity-rich corpora where graph signal is highly informative.
     */
    static std::unique_ptr<KnowledgeGraphRetriever> createDeep(
        const KnowledgeGraph& graph);
};

} // namespace themis::rag::kg
