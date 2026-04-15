/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_context_assembler.cpp                          ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:50:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 01a86c4f10  2026-04-07  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file rag_context_assembler.cpp
 * @brief Budget-aware context assembler for RAG inference.
 *
 * Implements the "Greedy Fill with Response Guard" strategy defined in
 * include/rag/rag_context_assembler.h.
 */

#include "rag/rag_context_assembler.h"

#include <algorithm>
#include <cmath>

namespace themis::rag {

using ::themis::llm::estimateTokens;
using ::themis::llm::ContextWindowBudget;
using ::themis::llm::tokensToChars;

// ---------------------------------------------------------------------------
// Constructor / configuration
// ---------------------------------------------------------------------------

RAGContextAssembler::RAGContextAssembler(const RAGContextAssemblerConfig& cfg)
    : config_(cfg) {}

const RAGContextAssemblerConfig& RAGContextAssembler::getConfig() const
{
    return config_;
}

void RAGContextAssembler::setConfig(const RAGContextAssemblerConfig& cfg)
{
    config_ = cfg;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string RAGContextAssembler::truncateContent(const std::string& content,
                                                  size_t             max_chars) const
{
    if (content.size() <= max_chars) return content;

    const std::string& marker = config_.truncation_marker;
    if (max_chars <= marker.size()) {
        // No room even for the marker — return marker only (capped to max_chars).
        return marker.substr(0, max_chars);
    }
    return content.substr(0, max_chars - marker.size()) + marker;
}

// ---------------------------------------------------------------------------
// Core assembly
// ---------------------------------------------------------------------------

AssembledContext RAGContextAssembler::assemble(
    const std::vector<RetrievedChunk>& chunks,
    const std::string&                 system_prompt,
    const std::string&                 query) const
{
    // ── Step 1: Compute token budget ────────────────────────────────────────
    const ContextWindowBudget budget = ContextWindowBudget::compute(
        config_.model_context_tokens,
        system_prompt,
        query,
        config_.min_response_tokens);

    AssembledContext result;
    result.tokens_remaining_for_response = budget.reserved_response_tokens;

    if (!budget.hasContextBudget() || chunks.empty()) {
        return result;
    }

    // ── Step 2: Sort chunks by relevance descending ─────────────────────────
    std::vector<const RetrievedChunk*> ordered;
    ordered.reserve(chunks.size());
    for (const auto& c : chunks) ordered.push_back(&c);

    std::stable_sort(ordered.begin(), ordered.end(),
        [](const RetrievedChunk* a, const RetrievedChunk* b) {
            return a->relevance_score > b->relevance_score;
        });

    // ── Step 3 & 4: Greedy fill with optional truncation ────────────────────
    size_t remaining = budget.available_context_tokens;

    for (size_t i = 0; i < ordered.size() && remaining > 0u; ++i) {
        const RetrievedChunk& chunk     = *ordered[i];
        const size_t          chunk_tok = estimateTokens(chunk.content);

        if (chunk_tok <= remaining) {
            // Chunk fits in full.
            result.chunks_used.push_back(chunk);
            result.tokens_used += chunk_tok;
            remaining          -= chunk_tok;
        } else if (config_.allow_partial_chunk) {
            // Chunk is too large — truncate it to the remaining char budget.
            const size_t max_chars = tokensToChars(remaining);
            if (max_chars > config_.truncation_marker.size()) {
                RetrievedChunk truncated = chunk;
                truncated.content        = truncateContent(chunk.content, max_chars);
                result.chunks_used.push_back(std::move(truncated));
                result.tokens_used   += remaining; // consumed all remaining budget
                remaining             = 0u;
                result.was_truncated  = true;
            }
            break; // budget exhausted
        } else {
            break; // drop over-budget chunk
        }
    }

    // ── Step 5: Re-compute response budget after context fill ────────────────
    result.tokens_remaining_for_response =
        budget.responseBudgetAfterContext(result.tokens_used);

    return result;
}

// ---------------------------------------------------------------------------
// computeMaxTokens
// ---------------------------------------------------------------------------

int RAGContextAssembler::computeMaxTokens(
    const ContextWindowBudget& budget,
    int                        user_max)
{
    // Tokens available for the response = reserved_response_tokens (minimum).
    int computed = static_cast<int>(budget.reserved_response_tokens);
    if (computed <= 0) computed = 1;

    if (user_max > 0) {
        return std::min(computed, user_max);
    }
    return computed;
}

} // namespace themis::rag
