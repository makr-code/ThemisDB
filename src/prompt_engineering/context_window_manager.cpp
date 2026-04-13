/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            context_window_manager.cpp                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:28:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6314fa0fc6  2026-03-21  feat(prompt_engineering): implement ContextWindowBudgetMa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file context_window_manager.cpp
 * @brief Implementation of ContextWindowBudgetManager.
 */

#include "prompt_engineering/context_window_manager.h"

#include <algorithm>
#include <numeric>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// ContextWindowBudgetManager
// ============================================================================

ContextWindowBudgetManager::ContextWindowBudgetManager(
    const ModelTokenBudget& budget,
    std::shared_ptr<ITokenCounter> counter)
    : budget_(budget),
      counter_(counter ? std::move(counter)
                       : std::make_shared<CharDivisionCounter>()) {}

// -------------------------------------------------------------------------
// Configuration
// -------------------------------------------------------------------------

void ContextWindowBudgetManager::setModel(const ModelTokenBudget& budget) {
    budget_ = budget;
}

const ModelTokenBudget& ContextWindowBudgetManager::getModel() const noexcept {
    return budget_;
}

void ContextWindowBudgetManager::setTokenCounter(
    std::shared_ptr<ITokenCounter> counter) {
    if (counter) {
        counter_ = std::move(counter);
    }
}

// -------------------------------------------------------------------------
// Token counting
// -------------------------------------------------------------------------

size_t ContextWindowBudgetManager::countTokens(const std::string& text) const {
    return counter_->count(text);
}

// -------------------------------------------------------------------------
// Private helpers
// -------------------------------------------------------------------------

size_t ContextWindowBudgetManager::countChunkTokens(
    const std::vector<RetrievedChunk>& chunks) const {
    size_t total = 0;
    for (const auto& chunk : chunks) {
        total += counter_->count(chunk.content);
    }
    return total;
}

// -------------------------------------------------------------------------
// Chunk selection
// -------------------------------------------------------------------------

std::vector<RetrievedChunk> ContextWindowBudgetManager::fitChunksInBudget(
    const std::vector<RetrievedChunk>& chunks,
    size_t available_tokens) const {

    std::vector<RetrievedChunk> selected;
    size_t used = 0;

    for (const auto& chunk : chunks) {
        size_t chunk_tokens = counter_->count(chunk.content);
        if (used + chunk_tokens > available_tokens && !selected.empty()) {
            break;  // budget exhausted
        }
        used += chunk_tokens;
        selected.push_back(chunk);
    }

    return selected;
}

std::vector<RetrievedChunk> ContextWindowBudgetManager::fitChunksInBudget(
    const std::vector<RetrievedChunk>& chunks,
    const std::string& system_prompt,
    const std::string& query) const {

    const size_t prompt_budget   = budget_.promptBudget();
    const size_t system_tokens   = counter_->count(system_prompt);
    const size_t query_tokens    = counter_->count(query);
    const size_t overhead        = system_tokens + query_tokens;

    const size_t context_budget =
        (overhead < prompt_budget) ? (prompt_budget - overhead) : 0;

    return fitChunksInBudget(chunks, context_budget);
}

// -------------------------------------------------------------------------
// Budget computation
// -------------------------------------------------------------------------

BudgetAllocation ContextWindowBudgetManager::computeBudget(
    const std::string& system_prompt,
    const std::string& query,
    const std::vector<RetrievedChunk>& chunks) const {

    BudgetAllocation alloc;
    alloc.system_tokens  = counter_->count(system_prompt);
    alloc.query_tokens   = counter_->count(query);
    alloc.context_tokens = countChunkTokens(chunks);
    alloc.total_tokens   = alloc.system_tokens + alloc.query_tokens + alloc.context_tokens;
    alloc.budget_tokens  = budget_.promptBudget();
    alloc.utilization    = (alloc.budget_tokens > 0)
                               ? (static_cast<double>(alloc.total_tokens) /
                                  static_cast<double>(alloc.budget_tokens))
                               : 0.0;

    if (utilization_cb_) {
        utilization_cb_(alloc.utilization);
    }

    return alloc;
}

BudgetAllocation ContextWindowBudgetManager::computeAndCheck(
    const std::string& system_prompt,
    const std::string& query,
    const std::vector<RetrievedChunk>& chunks) const {

    auto alloc = computeBudget(system_prompt, query, chunks);
    if (!alloc.fits()) {
        throw PromptBudgetExceededError(
            alloc.total_tokens,
            alloc.budget_tokens,
            budget_.model_name);
    }
    return alloc;
}

// -------------------------------------------------------------------------
// Metrics
// -------------------------------------------------------------------------

void ContextWindowBudgetManager::setUtilizationCallback(
    std::function<void(double)> cb) {
    utilization_cb_ = std::move(cb);
}

} // namespace prompt_engineering
} // namespace themis
