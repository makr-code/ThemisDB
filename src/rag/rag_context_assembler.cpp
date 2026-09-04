/**
 * @file rag_context_assembler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/rag_context_assembler.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <limits>

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
    if (static_cast<int>(content.size()) <= max_chars) {
      return content;
    }

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

// Thread-Safe: all parameters are const/local; no mutable state modification.
// Complexity: O(n log n) where n = chunks.size()
//   - Line 101-113: sort by relevance descending = O(n log n)
//   - Line 119-143: greedy fill with token estimation = O(n)
// Failure modes handled:
//   - Empty chunks: returns valid empty AssembledContext (lines 87-94)
//   - Zero budget: returns valid empty AssembledContext with response reservation
//   - All chunks over-budget: greedy strategy ensures at least response tokens reserved
//   - Integer overflow in token counting: prevented by budget bounds enforcement

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

    spdlog::info(
        "RAGContextAssembler::assemble start: input_chunks={} query_chars={} model_ctx={} context_budget={} response_budget={}",
        chunks.size(),
        query.size(),
        config_.model_context_tokens,
        budget.available_context_tokens,
        budget.reserved_response_tokens);

    AssembledContext result;
    result.tokens_remaining_for_response = budget.reserved_response_tokens;

    if (!budget.hasContextBudget() || chunks.empty()) {
        spdlog::info(
            "RAGContextAssembler::assemble short-circuit: has_context_budget={} input_chunks={} response_tokens_remaining={}",
            budget.hasContextBudget(),
            chunks.size(),
            result.tokens_remaining_for_response);
        return result;
    }

    // ── Step 2: Sort chunks by relevance descending ─────────────────────────
    // Complexity: O(n log n) where n = chunks.size()
    // Deterministic tie-breaking ensures consistent output:
    //   1. Primary: relevance_score descending (most relevant first)
    //   2. Secondary: chunk_id ascending (stable ordering for same-score chunks)
    //   3. Tertiary: source ascending (deterministic when ID also matches)
    //   4. Quaternary: content ascending (absolute determinism for edge cases)
    // This ensures that identical input always produces identical output,
    // enabling reliable reproducibility and testing.
    std::vector<const RetrievedChunk*> ordered = {};

    ordered.reserve(chunks.size());
    for (const auto& c : chunks) {
      ordered.push_back(&c);
    }

    std::sort(ordered.begin(), ordered.end(),
        [](const RetrievedChunk* a, const RetrievedChunk* b) {
            // Primary sort: relevance descending (highest first)
            if (a->relevance_score != b->relevance_score) {
                return a->relevance_score > b->relevance_score;
            }
            // Secondary: chunk_id ascending (deterministic tie-breaking)
            if (a->chunk_id != b->chunk_id) {
                return a->chunk_id < b->chunk_id;
            }
            // Tertiary: source ascending (further deterministic ordering)
            if (a->source != b->source) {
                return a->source < b->source;
            }
            // Quaternary: content ascending (absolute determinism)
            return a->content < b->content;
        });

    // ── Step 3 & 4: Greedy fill with optional truncation ────────────────────
    // Complexity: O(n) single pass through sorted chunks
    // Strategy: fill context budget greedily in relevance order
    // Failure modes handled:
    //   - Chunk fits fully: add to result, continue
    //   - Chunk partially fits & allow_partial_chunk=true: truncate + add + stop
    //   - Chunk too large & allow_partial_chunk=false: skip chunk, continue
    //   - All chunks exhausted: result has as many chunks as fit
    // Response budget invariant: always reserved via budget.responseBudgetAfterContext()
    size_t remaining = budget.available_context_tokens;
    result.chunks_used.reserve(ordered.size());

    for (size_t i = 0; i <static_cast<int>(ordered.size()) && remaining > 0u; ++i) {
        const RetrievedChunk& chunk     = *ordered[i];
        const size_t          chunk_tok = estimateTokens(chunk.content);

        if (chunk_tok <= remaining) {
            // Chunk fits in full: add to result and deduct from remaining budget
            result.chunks_used.push_back(chunk);
            result.tokens_used += chunk_tok;
            remaining          -= chunk_tok;
        } else if (config_.allow_partial_chunk) {
            // Chunk is too large — truncate it to the remaining char budget
            // and consume the remainder of context allocation
            const size_t max_chars = tokensToChars(remaining);
            if (max_chars > static_cast<int>(config_.truncation_marker.size())) {
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

    spdlog::info(
        "RAGContextAssembler::assemble complete: used_chunks={} tokens_used={} truncated={} response_tokens_remaining={}",
        result.chunks_used.size(),
        result.tokens_used,
        result.was_truncated,
        result.tokens_remaining_for_response);

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
    constexpr size_t kIntMaxAsSizeT =
        static_cast<size_t>(std::numeric_limits<int>::max());
    const size_t clamped = std::min(budget.reserved_response_tokens, kIntMaxAsSizeT);
    int computed = static_cast<int>(clamped);
    if (computed <= 0) {
      computed = 1;
    }

    if (user_max > 0) {
        return std::min(computed, user_max);
    }
    return computed;
}

} // namespace themis::rag

