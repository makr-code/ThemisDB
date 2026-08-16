/**
 * @file process_agentic_rag.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_agentic_rag.cpp
 * Module:  src/process/
 * Purpose: AgenticRAG integration for iterative process Q&A.
 */

#include "process/process_agentic_rag.h"
#include <stdexcept>
#include "utils/logger.h"

#include <chrono>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace process {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ProcessAgenticRag::ProcessAgenticRag(ProcessGraphRag& rag)
    : rag_(rag), config_{}
{}

ProcessAgenticRag::ProcessAgenticRag(
    ProcessGraphRag& rag, const ProcessAgenticConfig& config)
    : rag_(rag), config_(config)
{}

// ---------------------------------------------------------------------------
// encodeContext – ProcessRagContext → vector<RetrievedDocument>
// ---------------------------------------------------------------------------

std::vector<rag::judge::RetrievedDocument>
ProcessAgenticRag::encodeContext(const ProcessRagContext& ctx)
{
    std::vector<rag::judge::RetrievedDocument> docs;

    // Encode the main LLM prompt as a document.
    if (!ctx.llm_prompt.empty()) {
        rag::judge::RetrievedDocument d;
        // NOLINT(clang-analyzer-core.uninitialized.Assign) - Safe string concatenation
        d.id               = "proc_prompt:" + ctx.instance_id;
        d.content          = ctx.llm_prompt;
        d.similarity_score = 1.0;
        d.metadata["type"] = "llm_prompt";
        d.metadata["instance_id"] = ctx.instance_id;
        docs.push_back(std::move(d));
    }

    // Encode the subgraph as a document.
    if (!ctx.subgraph.empty()) {
        rag::judge::RetrievedDocument d;
        // NOLINT(clang-analyzer-core.uninitialized.Assign) - Safe string concatenation
        d.id               = "proc_subgraph:" + ctx.instance_id;
        d.content          = ctx.subgraph.dump(2);  // NOLINT(clang-analyzer-core.uninitialized.Assign)
        d.similarity_score = 0.9;
        d.metadata["type"] = "subgraph";
        d.metadata["instance_id"] = ctx.instance_id;
        d.metadata["process_name"] = ctx.process_name;
        docs.push_back(std::move(d));
    }

    // Encode attachments.
    size_t att_idx = 0;
    for (const auto& att : ctx.attachments) {
        rag::judge::RetrievedDocument d;
        // NOLINT(clang-analyzer-core.uninitialized.Assign) - Safe string concatenation and std::to_string
        d.id               = "proc_att:" + ctx.instance_id + ":" +
                             std::to_string(att_idx++);
        d.content          = att.dump(2);  // NOLINT(clang-analyzer-core.uninitialized.Assign)
        d.similarity_score = 0.8;
        d.metadata["type"] = "attachment";
        d.metadata["instance_id"] = ctx.instance_id;
        docs.push_back(std::move(d));
    }

    // Encode similar cases.
    size_t sc_idx = 0;
    for (const auto& sc : ctx.similar_cases) {
        rag::judge::RetrievedDocument d;
        // NOLINT(clang-analyzer-core.uninitialized.Assign) - Safe string concatenation and std::to_string
        d.id               = "proc_case:" + ctx.instance_id + ":" +
                             std::to_string(sc_idx++);
        d.content          = sc.dump(2);  // NOLINT(clang-analyzer-core.uninitialized.Assign)
        d.similarity_score = 0.7;
        d.metadata["type"] = "similar_case";
        docs.push_back(std::move(d));
    }

    // Encode missing documents list.
    if (!ctx.missing_documents.empty()) {
        std::ostringstream oss;
        for (const auto& md : ctx.missing_documents) {
            oss << "- " << md << '\n';
        }
        rag::judge::RetrievedDocument d;
        // NOLINT(clang-analyzer-core.uninitialized.Assign) - Safe string concatenation
        d.id               = "proc_missing:" + ctx.instance_id;
        d.content          = oss.str();  // NOLINT(clang-analyzer-core.uninitialized.Assign)
        d.similarity_score = 0.85;
        d.metadata["type"] = "missing_documents";
        d.metadata["instance_id"] = ctx.instance_id;
        docs.push_back(std::move(d));
    }

    return docs;
}

// ---------------------------------------------------------------------------
// mergeDocuments – incorporate extra docs back into context
// ---------------------------------------------------------------------------

ProcessRagContext ProcessAgenticRag::mergeDocuments(
    ProcessRagContext ctx,
    const std::vector<rag::judge::RetrievedDocument>& extra_docs)
{
    // Build a set of existing attachment IDs for O(log n) lookup
    // instead of O(n) linear search per doc (avoiding O(n²) complexity)
    std::unordered_set<std::string> existing_ids;
    for (const auto& existing : ctx.attachments) {
        if (existing.contains("_id")) {
            try {
                std::string id = existing["_id"].get<std::string>();
                existing_ids.insert(std::move(id));
            } catch (const nlohmann::json::exception&) {
                // Skip malformed entries
                continue;
            }
        }
    }

    for (const auto& doc : extra_docs) {
        auto type_it = doc.metadata.find("type");
        if (type_it == doc.metadata.end()) continue;

        const std::string& t = type_it->second;
        if (t == "attachment" || t == "similar_case") {
            // Avoid duplicates by ID using O(1) set lookup instead of O(n) search
            if (existing_ids.find(doc.id) != existing_ids.end()) {
                continue;
            }

            try {
                nlohmann::json parsed = nlohmann::json::parse(doc.content);
                ctx.attachments.push_back(std::move(parsed));
                existing_ids.insert(doc.id);  // Track newly added ID
            } catch (const nlohmann::json::exception& e) {
                // Fallback: store as-is with metadata
                SPDLOG_WARN("[process] Failed to parse attachment '{}': {}", doc.id, e.what());
                ctx.attachments.push_back(nlohmann::json{{"_id", doc.id},
                                                          {"content", doc.content}});
                existing_ids.insert(doc.id);
            } catch (const std::exception& e) {
                // Catch other exceptions (memory, etc.)
                SPDLOG_ERROR("[process] Unexpected error merging document '{}': {}", doc.id, e.what());
            }
        }
        // Subgraph and prompt replacements are not merged to avoid clobbering
        // the structured data.
    }
    return ctx;
}

// ---------------------------------------------------------------------------
// runLoop – core agentic loop
// ---------------------------------------------------------------------------

ProcessAgenticResult ProcessAgenticRag::runLoop(
    std::string_view  instance_id,
    std::string_view  question,
    ProcessRagContext initial_ctx)
{
    const auto wall_start = std::chrono::steady_clock::now();

    // Build the AgenticRAG config from ProcessAgenticConfig.
    rag::agentic::AgenticRAGConfig agentic_cfg;
    agentic_cfg.max_iterations        = config_.max_iterations;
    agentic_cfg.quality_threshold     = config_.quality_threshold;
    agentic_cfg.faithfulness_threshold = config_.faithfulness_threshold;
    agentic_cfg.max_total_documents   = config_.max_total_documents;
    agentic_cfg.accumulate_documents  = true;

    rag::agentic::AgenticRAG agent(agentic_cfg);

    // Encode the initial context.
    auto initial_docs = encodeContext(initial_ctx);

    // Build the retrieval callback: on subsequent iterations, re-retrieve
    // with the reformulated query and return newly-seen documents.
    const std::string inst_str(instance_id);
    const ProcessRagConfig& rag_cfg = config_.rag_config;

    auto retrieval_fn =
        [this, &inst_str, &rag_cfg](
            const std::string& query,
            const std::vector<std::string>& seen_ids)
        -> std::vector<rag::judge::RetrievedDocument>
    {
        ProcessRagConfig cfg = rag_cfg;
        cfg.max_prompt_tokens = 2000; // tighter budget on follow-up rounds
        ProcessRagContext ctx = rag_.retrieve(inst_str, query, cfg);
        auto docs = encodeContext(ctx);

        // Filter out already-seen documents.
        std::vector<rag::judge::RetrievedDocument> fresh;
        for (auto& d : docs) {
            bool already_seen = false;
            for (const auto& sid : seen_ids) {
                if (d.id == sid) { already_seen = true; break; }
            }
            if (!already_seen) fresh.push_back(std::move(d));
        }
        return fresh;
    };

    // Run the agentic loop.
    auto agentic_result = agent.run(
        std::string(question),
        std::move(initial_docs),
        retrieval_fn
    );

    // Assemble the final ProcessAgenticResult.
    ProcessAgenticResult result;
    result.quality_satisfied = agentic_result.quality_satisfied;
    result.total_iterations  = agentic_result.total_iterations;
    result.total_elapsed_ms  = agentic_result.total_elapsed_ms;

    // Merge any extra documents back into the initial context.
    result.final_context = mergeDocuments(
        std::move(initial_ctx), agentic_result.final_documents);

    // Build the final LLM prompt from the enriched context.
    result.llm_prompt = rag_.buildAdminProcessingPrompt(result.final_context);

    // Populate iteration history.
    for (const auto& iter : agentic_result.iterations) {
        ProcessAgenticResult::IterationSummary s;
        s.iteration          = iter.iteration;
        s.query_used         = iter.query_used;
        s.documents_retrieved = iter.documents.size();
        s.overall_score      = iter.evaluation.overall_score;
        result.iteration_history.push_back(std::move(s));
    }

    const auto wall_end = std::chrono::steady_clock::now();
    SPDLOG_INFO(
        "[process] agentic Q&A for '{}': {} iterations, quality_satisfied={}, "
        "elapsed={}ms",
        instance_id,
        result.total_iterations,
        result.quality_satisfied,
        std::chrono::duration_cast<std::chrono::milliseconds>(
            wall_end - wall_start).count()
    );

    return result;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ProcessAgenticResult ProcessAgenticRag::iterativeQuery(
    std::string_view instance_id,
    std::string_view question)
{
    auto initial_ctx = rag_.retrieve(
        instance_id, question, config_.rag_config);
    return runLoop(instance_id, question, std::move(initial_ctx));
}

ProcessAgenticResult ProcessAgenticRag::iterativeQueryForNode(
    std::string_view instance_id,
    std::string_view node_id,
    std::string_view question)
{
    auto initial_ctx = rag_.retrieveForNode(
        instance_id, node_id, question, config_.rag_config);
    return runLoop(instance_id, question, std::move(initial_ctx));
}

} // namespace process
} // namespace themis

