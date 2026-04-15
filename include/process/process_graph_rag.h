/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            process_graph_rag.h                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:12:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     328                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f7a272409  2026-03-12  feat(process): add ProcessLinker, ProcessGraphRag, and mo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_graph_rag.h
 * Module:  include/process/
 * Purpose: Graph-RAG engine for German administrative proceedings
 *          (Verwaltungsvorgänge).  Bridges the process execution graph with
 *          the KnowledgeGraphRetriever to produce LLM-ready context.
 */

#pragma once

#include "index/process_graph.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "rag/knowledge_graph_retriever.h"
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// ProcessRagConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Tuning parameters for Graph-RAG retrieval over process instances.
 */
struct ProcessRagConfig {
    int    max_subgraph_depth{3};       ///< BFS depth from the current node(s)
    int    max_similar_cases{5};        ///< Max similar historical cases to include
    bool   include_attachments{true};   ///< Include attached documents in context
    bool   include_history{true};       ///< Include token traversal history
    bool   include_missing_docs{true};  ///< Check for missing required documents
    bool   include_compliance{true};    ///< Include compliance tags and rules
    float  similarity_threshold{0.7f};  ///< Minimum similarity score for similar cases
    size_t max_prompt_tokens{3000};     ///< Approximate token budget for the LLM prompt
    std::string language{"de"};         ///< Output language: "de" (German) or "en"
    bool   use_ppr{false};              ///< Use Personalized PageRank instead of BFS
};

// ─────────────────────────────────────────────────────────────────────────────
// PprConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Tuning parameters for Personalized PageRank (PPR) subgraph scoring.
 *
 * PPR replaces BFS for multi-hop query resolution when
 * @c ProcessRagConfig::use_ppr is true.
 *
 * Scientific basis: Gutierrez et al. (2024). *HippoRAG: Neurobiologically
 * Inspired Long-Term Memory for Large Language Models.* NeurIPS 2024.
 */
struct PprConfig {
    float damping{0.85f};             ///< Damping factor α (standard PageRank default)
    int   max_iterations{50};         ///< Maximum power-iteration steps
    float convergence_epsilon{1e-6f}; ///< Stop when ||r_new - r_old||_1 < epsilon
    int   top_k_nodes{20};            ///< Return only the top-k nodes by PPR score
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessRagContext
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Full retrieval context built from the process graph for LLM
 *        consumption.
 *
 * Produced by @c ProcessGraphRag::retrieve() and
 * @c ProcessGraphRag::retrieveForNode().
 */
struct ProcessRagContext {
    std::string instance_id;
    std::string query;

    // Current process state ──────────────────────────────────────────────
    std::string process_definition_id;
    std::string process_name;
    std::string current_state;           ///< "RUNNING", "SUSPENDED", etc.
    std::vector<std::string> active_nodes;   ///< Node IDs currently holding tokens
    std::vector<std::string> visited_nodes;  ///< All nodes visited so far

    // Relevant subgraph ──────────────────────────────────────────────────
    /// JSON object with keys "nodes" (array) and "edges" (array) – the
    /// portion of the process model graph relevant to the query.
    nlohmann::json subgraph;

    // Attached data objects ──────────────────────────────────────────────
    std::vector<nlohmann::json> attachments; ///< Attached documents / metadata

    // Similar historical cases ───────────────────────────────────────────
    std::vector<nlohmann::json> similar_cases;

    // Missing documents ──────────────────────────────────────────────────
    std::vector<std::string> missing_documents;

    // Compliance context ─────────────────────────────────────────────────
    std::vector<std::string> compliance_tags;

    // LLM prompt ─────────────────────────────────────────────────────────
    std::string llm_prompt;

    // Relevance scores (node_id → score) ─────────────────────────────────
    std::unordered_map<std::string, float> node_scores;
    float overall_relevance{0.f};
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessGraphRag
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Core Graph-RAG engine for Verwaltungsvorgänge.
 *
 * Orchestrates retrieval from the process execution engine
 * (@c ProcessGraphManager), the process model store (@c ProcessModelManager),
 * and the attachment layer (@c ProcessLinker) to produce a rich context
 * object that can be forwarded to a large language model.
 *
 * Typical usage:
 * @code
 *   ProcessGraphRag rag(db, engine, models, linker);
 *   auto ctx = rag.retrieve("inst-42", "Was fehlt noch für den Bauantrag?");
 *   // ctx.llm_prompt is ready to send to the LLM
 * @endcode
 */
class ProcessGraphRag {
public:
    ProcessGraphRag(
        RocksDBWrapper&       db,
        ProcessGraphManager&  engine,   ///< Execution engine (instance/token queries)
        ProcessModelManager&  models,   ///< Model definitions
        ProcessLinker&        linker    ///< Attachment layer
    );

    // ── Core Graph-RAG retrieval ──────────────────────────────────────────

    /**
     * @brief Build the full retrieval context for a process instance and a
     *        free-text query.
     *
     * Orchestration sequence:
     * 1. Load instance state and active tokens.
     * 2. Extract subgraph around active nodes.
     * 3. Collect attachments.
     * 4. Check for missing required documents.
     * 5. Find similar historical cases.
     * 6. Assemble the LLM prompt.
     */
    [[nodiscard]] ProcessRagContext retrieve(
        std::string_view        instance_id,
        std::string_view        query,
        const ProcessRagConfig& config = {}
    ) const;

    /**
     * @brief Build retrieval context focused on a specific node within an
     *        instance.
     *
     * Used when the LLM needs to answer questions about a particular task
     * step (e.g., "Which documents are required at Schritt 3?").
     */
    [[nodiscard]] ProcessRagContext retrieveForNode(
        std::string_view        instance_id,
        std::string_view        node_id,
        std::string_view        query,
        const ProcessRagConfig& config = {}
    ) const;

    // ── Knowledge graph population ─────────────────────────────────────────

    /**
     * @brief Container for KGNode / KGEdge objects derived from a process
     *        model or instance.
     */
    struct ProcessKnowledgeGraph {
        std::vector<rag::kg::KGNode> nodes;
        std::vector<rag::kg::KGEdge> edges;
    };

    /**
     * @brief Convert a stored process model definition into a
     *        @c KnowledgeGraph-compatible node/edge set.
     *
     * Process nodes become @c KGNode objects (type @c CONCEPT).
     * Sequence flows become @c KGEdge objects (relation @c CAUSES or
     * @c RELATED_TO).
     */
    [[nodiscard]] ProcessKnowledgeGraph buildKnowledgeGraph(
        std::string_view model_id
    ) const;

    /**
     * @brief Build a @c KnowledgeGraph from a live process instance.
     *
     * Includes instance state nodes (active tasks, variables) and attachment
     * nodes so the LLM can traverse relationships.
     */
    [[nodiscard]] ProcessKnowledgeGraph buildInstanceKnowledgeGraph(
        std::string_view        instance_id,
        const ProcessRagConfig& config = {}
    ) const;

    // ── Subgraph extraction ────────────────────────────────────────────────

    /**
     * @brief Extract the subgraph around @p seed_node_ids using BFS up to
     *        @p max_depth hops.
     *
     * @return JSON with keys @c "nodes" (array of node objects) and
     *         @c "edges" (array of edge objects).
     */
    [[nodiscard]] nlohmann::json extractSubgraph(
        std::string_view             model_id,
        const std::vector<std::string>& seed_node_ids,
        int                          max_depth = 2
    ) const;

    /**
     * @brief Compute Personalized PageRank (PPR) scores for all nodes in a
     *        process model graph, seeded from @p seed_node_ids.
     *
     * PPR handles multi-hop queries better than BFS by propagating relevance
     * across the entire graph with exponential distance decay.
     *
     * Uses power iteration:
     * @code
     *   r = α * A^T * r + (1-α) * personalization
     * @endcode
     * Terminates when ||r_new − r_old||₁ < @c PprConfig::convergence_epsilon
     * or after @c PprConfig::max_iterations steps.
     *
     * @param normalized_graph  Process model normalized graph JSON
     *                          ({@c "nodes", @c "edges"}).
     * @param seed_node_ids     Starting nodes (personalisation vector is
     *                          uniform over these nodes).
     * @param config            PPR tuning parameters.
     * @return Top-k (node_id, ppr_score) pairs sorted descending by score.
     */
    [[nodiscard]] std::vector<std::pair<std::string, float>> computePpr(
        const nlohmann::json&          normalized_graph,
        const std::vector<std::string>& seed_node_ids,
        const PprConfig&               config = {}
    ) const;

    // ── LLM prompt building ────────────────────────────────────────────────

    /**
     * @brief Build a German (or English) system prompt for an LLM agent
     *        managing a Verwaltungsvorgang.
     *
     * The prompt includes: process state, active tasks, attached documents,
     * missing documents, compliance tags, SLA status, subgraph description,
     * and similar historical cases.
     */
    [[nodiscard]] std::string buildAdminProcessingPrompt(
        const ProcessRagContext& ctx
    ) const;

    /**
     * @brief Build a query-specific user prompt (e.g. for "Was fehlt noch?").
     */
    [[nodiscard]] std::string buildQueryPrompt(
        const ProcessRagContext& ctx
    ) const;

    // ── Administrative proceeding helpers ─────────────────────────────────

    /**
     * @brief Summarise a Verwaltungsvorgang for an officer or a case-management UI.
     *
     * @return JSON:
     * @code{.json}
     * {
     *   "instance_id": "…",
     *   "process_name": "…",
     *   "state": "RUNNING",
     *   "current_tasks": ["…"],
     *   "progress_pct": 42.5,
     *   "missing_documents": ["Bauzeichnung"],
     *   "compliance_status": "ok",
     *   "sla_status": "on_time",
     *   "attachments_count": 3,
     *   "variables": { … }
     * }
     * @endcode
     */
    [[nodiscard]] nlohmann::json summarizeVerwaltungsvorgang(
        std::string_view instance_id
    ) const;

    /**
     * @brief Check compliance of a process instance against its model.
     */
    struct ComplianceCheckResult {
        bool is_compliant{true};
        std::vector<std::string> violations;  ///< Violated regulation strings
        std::vector<std::string> warnings;    ///< Potential issues
        float compliance_score{1.f};          ///< 0.0 – 1.0
    };
    [[nodiscard]] ComplianceCheckResult checkCompliance(
        std::string_view instance_id
    ) const;

    /**
     * @brief Find similar past Verwaltungsvorgänge (for precedent / reference).
     *
     * Uses cosine similarity over stored instance embeddings
     * (key prefix @c proc:inst_emb:<id>) if available, and falls back to
     * variable-based Jaccard similarity otherwise.
     */
    struct SimilarCase {
        std::string instance_id;
        std::string process_definition_id;
        std::string name;
        float       similarity{0.f};
        std::string outcome;           ///< "COMPLETED", "FAILED", etc.
        nlohmann::json key_variables;
    };
    [[nodiscard]] std::vector<SimilarCase> findSimilarCases(
        std::string_view instance_id,
        int              k             = 5,
        float            min_similarity = 0.6f
    ) const;

private:
    RocksDBWrapper&       db_;
    ProcessGraphManager&  engine_;
    ProcessModelManager&  models_;
    ProcessLinker&        linker_;

    /// Score how relevant @p node_doc is to @p query given @p active_nodes.
    float scoreNodeRelevance_(
        const nlohmann::json&          node_doc,
        std::string_view               query,
        const std::vector<std::string>& active_nodes
    ) const;

    /// Assemble the final LLM prompt string from context and config.
    std::string assemblePrompt_(
        const ProcessRagContext& ctx,
        const ProcessRagConfig&  config
    ) const;
};

} // namespace process
} // namespace themis
