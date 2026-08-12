/**
 * @file process_agentic_rag.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_agentic_rag.h
 * Module:  include/process/
 * Purpose: AgenticRAG integration for iterative process question answering.
 *
 * Bridges the process-specific Graph-RAG engine (ProcessGraphRag) with the
 * generic AgenticRAG loop (rag::agentic::AgenticRAG) so that an LLM agent
 * can iteratively refine its understanding of a Verwaltungsvorgang
 * (administrative proceeding) by issuing follow-up retrieval queries.
 *
 * ## Design
 *
 * ProcessAgenticRag acts as a façade:
 *
 * 1.  On the first call it invokes ProcessGraphRag::retrieve() to obtain an
 *     initial context (subgraph, attachments, compliance tags).
 * 2.  The context is encoded as a vector of RetrievedDocument objects and
 *     passed to AgenticRAG::run().
 * 3.  The AgenticRAG loop evaluates quality, detects knowledge gaps, and
 *     calls back into the process retrieval layer when gaps remain.
 * 4.  The final consolidated documents are re-assembled into an enriched
 *     ProcessRagContext and a ready-to-send LLM prompt.
 *
 * ## Thread safety
 *
 * One ProcessAgenticRag instance must not be shared across threads.
 * Create one instance per concurrent request.
 */

#include "process/process_graph_rag.h"
#include "rag/agentic_rag.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace process {

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Tuning parameters for ProcessAgenticRag.
 */
struct ProcessAgenticConfig {
    /// Maximum number of retrieval–evaluate–detect–reformulate cycles.
    size_t max_iterations{4};

    /// Minimum overall quality score to consider the loop successful.
    double quality_threshold{0.75};

    /// Minimum faithfulness score required.
    double faithfulness_threshold{0.80};

    /// Maximum accumulated documents across all iterations.
    size_t max_total_documents{40};

    /// ProcessRagConfig forwarded to ProcessGraphRag::retrieve() on each
    /// iteration.
    ProcessRagConfig rag_config;
};

// ---------------------------------------------------------------------------
// Result
// ---------------------------------------------------------------------------

/**
 * @brief Result of an iterative process question-answering session.
 */
struct ProcessAgenticResult {
    /// Enriched retrieval context from the final iteration.
    ProcessRagContext final_context;

    /// Ready-to-send LLM prompt (built from final_context).
    std::string llm_prompt;

    /// True when quality_threshold was satisfied.
    bool quality_satisfied{false};

    /// Number of retrieval iterations executed.
    size_t total_iterations{0};

    /// Total elapsed wall-clock time.
    std::chrono::milliseconds total_elapsed_ms{0};

    /// Per-iteration summaries (query used, document count, overall score).
    struct IterationSummary {
        size_t      iteration{0};
        std::string query_used;
        size_t      documents_retrieved{0};
        double      overall_score{0.0};
    };
    std::vector<IterationSummary> iteration_history;
};

// ---------------------------------------------------------------------------
// ProcessAgenticRag
// ---------------------------------------------------------------------------

/**
 * @brief Iterative agentic Q&A engine for process instances.
 *
 * Wraps ProcessGraphRag and AgenticRAG to provide multi-hop question
 * answering over running Verwaltungsvorgänge.
 *
 * Typical usage:
 * @code
 *   ProcessAgenticConfig cfg;
 *   cfg.max_iterations = 3;
 *
 *   ProcessAgenticRag agent(graph_rag, cfg);
 *   auto result = agent.iterativeQuery("inst-42", "Was fehlt noch für den Bauantrag?");
 *   // result.llm_prompt is ready to send to the LLM
 * @endcode
 */
class ProcessAgenticRag {
public:
    /**
     * @brief Construct with default configuration.
     * @param rag  ProcessGraphRag reference (must outlive this object).
     */
    explicit ProcessAgenticRag(ProcessGraphRag& rag);

    /**
     * @brief Construct with custom configuration.
     * @param rag     ProcessGraphRag reference.
     * @param config  Tuning parameters.
     */
    ProcessAgenticRag(ProcessGraphRag& rag, const ProcessAgenticConfig& config);

    ~ProcessAgenticRag() = default;

    // Non-copyable
    ProcessAgenticRag(const ProcessAgenticRag&)            = delete;
    ProcessAgenticRag& operator=(const ProcessAgenticRag&) = delete;

    // -----------------------------------------------------------------------
    // Primary entry point
    // -----------------------------------------------------------------------

    /**
     * @brief Run the iterative agentic Q&A loop for a process instance.
     *
     * The loop:
     * 1. Retrieves an initial process context via ProcessGraphRag::retrieve().
     * 2. Encodes the context as RetrievedDocument objects.
     * 3. Runs the AgenticRAG loop to refine coverage.
     * 4. Returns the final enriched context and LLM prompt.
     *
     * @param instance_id  Process instance ID.
     * @param question     Natural-language question (DE or EN).
     * @return             ProcessAgenticResult with the final LLM prompt.
     */
    [[nodiscard]] ProcessAgenticResult iterativeQuery(
        std::string_view instance_id,
        std::string_view question
    );

    /**
     * @brief Run for a specific node within a process instance.
     *
     * Like iterativeQuery() but seeds the initial retrieval from
     * ProcessGraphRag::retrieveForNode().
     *
     * @param instance_id  Process instance ID.
     * @param node_id      Seed node ID.
     * @param question     Natural-language question.
     */
    [[nodiscard]] ProcessAgenticResult iterativeQueryForNode(
        std::string_view instance_id,
        std::string_view node_id,
        std::string_view question
    );

    // -----------------------------------------------------------------------
    // Configuration access
    // -----------------------------------------------------------------------

    const ProcessAgenticConfig& config() const { return config_; }
    void setConfig(const ProcessAgenticConfig& cfg) { config_ = cfg; }

private:
    ProcessGraphRag&     rag_;
    ProcessAgenticConfig config_;

    /// Encode a ProcessRagContext into a vector of RetrievedDocument.
    static std::vector<rag::judge::RetrievedDocument> encodeContext(
        const ProcessRagContext& ctx
    );

    /// Merge additional documents back into a ProcessRagContext.
    static ProcessRagContext mergeDocuments(
        ProcessRagContext ctx,
        const std::vector<rag::judge::RetrievedDocument>& extra_docs
    );

    /// Run the full agentic loop given the initial context.
    ProcessAgenticResult runLoop(
        std::string_view  instance_id,
        std::string_view  question,
        ProcessRagContext initial_ctx
    );
};

} // namespace process
} // namespace themis
