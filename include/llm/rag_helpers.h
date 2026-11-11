#pragma once

#include "llm/llm_plugin_interface.h"
#include "rag/rag_candidate.h"

#include <cstddef>
#include <utility>

namespace themis::llm {

/**
 * @brief Prepare one budgeted, sanitized generation request for any LLM backend.
 * @param context Supplied evidence, optional template and response reservation.
 * @param request Generation settings; prompt takes precedence over context.query.
 * @param model_context_tokens Loaded model's actual window; zero uses the shared default.
 * @return Owned request with formatted evidence, bounded max_tokens and assembly metadata.
 * @throws std::invalid_argument for invalid budgets, unsupported modes, blocked
 * query/system/template, or a budget that cannot accommodate supplied evidence.
 * @note Performs no retrieval or inference. Budgeting uses the existing character
 * heuristic, not a tokenizer-exact guarantee. Safe for concurrent independent calls.
 */
[[nodiscard]] InferenceRequest prepareRAGRequest(
    const RAGContext& context, const InferenceRequest& request,
    std::size_t model_context_tokens);

/**
 * @brief Convert owned, model-independent evidence to the existing LLM contract.
 * @param candidates Materialized evidence; no retrieval or inference is performed.
 * @return Documents preserving identity, collection and retrieval provenance.
 * @note Pure boundary conversion. Does not change the LLM plugin API/ABI.
 */
[[nodiscard]] inline std::vector<RAGContext::Document> toRAGDocuments(
    const std::vector<rag::RAGCandidate>& candidates) {
    std::vector<RAGContext::Document> documents;
    documents.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        RAGContext::Document document;
        document.content = candidate.content;
        document.source = candidate.doc_id;
        document.relevance_score = candidate.score;
        document.metadata = {
            {"id", candidate.doc_id},
            {"collection", candidate.source_namespace},
            {"provenance", candidate.provenance_tags}
        };
        documents.push_back(std::move(document));
    }
    return documents;
}

} // namespace themis::llm
