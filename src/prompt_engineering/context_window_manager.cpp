/**
 * @file context_window_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

// -------------------------------------------------------------------------
// Ethics discourse: per-thesis budget selection  (§9.1)
// -------------------------------------------------------------------------

std::vector<ThesisInjection> ContextWindowBudgetManager::selectThesesForRound(
    const ::themis::plugins::ethics::PhilosophyProfile& profile,
    int round_number,
    const std::string& round_role,
    int available_tokens) const {

    using Thesis = ::themis::plugins::ethics::PhilosophyThesis;

    if (profile.typed_theses.empty()) return {};

    // ── Step 1: partition into active / inactive for this round ──────────────
    // A thesis is active when:
    //   • activation_rounds is empty (all rounds active), OR
    //   • round_number is in activation_rounds
    std::vector<const Thesis*> active;
    std::vector<const Thesis*> inactive;

    for (const auto& t : profile.typed_theses) {
        bool is_active = t.activation_rounds.empty() ||
            (std::find(t.activation_rounds.begin(),
                       t.activation_rounds.end(),
                       round_number) != t.activation_rounds.end());
        if (is_active) {
          active.push_back(&t);
        }
        else           inactive.push_back(&t);
    }

    // ── Step 2: sort active theses by round_role weight descending ───────────
    // Theses without a weight for this role receive a neutral 0.5 priority.
    std::stable_sort(active.begin(), active.end(),
        [&round_role](const Thesis* a, const Thesis* b) {
            auto get_w = [&round_role](const Thesis* t) -> float {
                auto it = t->round_role_weights.find(round_role);
                return (it != t->round_role_weights.end()) ? it->second : 0.5f;
            };
            return get_w(a) > get_w(b);
        });

    // ── Step 3: greedy selection up to available_tokens ──────────────────────
    std::vector<ThesisInjection> result;
    result.reserve(profile.typed_theses.size());

    int remaining = available_tokens;

    for (const Thesis* t : active) {
        // The text we would inject: prefer description, fallback to name
        const std::string full_text = t->description.empty() ? t->name : t->description;
        int full_tokens = static_cast<int>(countTokens(full_text));

        // Apply per-thesis cap (token_budget == -1 → no cap)
        int capped_tokens = (t->token_budget >= 0)
            ? std::min(full_tokens, t->token_budget)
            : full_tokens;

        if (capped_tokens <= remaining) {
            // Full injection
            ThesisInjection inj;
            inj.thesis_id  = t->thesis_id;
            inj.text       = full_text;
            inj.is_full    = true;
            inj.tokens_used = capped_tokens;
            remaining      -= capped_tokens;
            result.push_back(std::move(inj));
        } else {
            // Downgrade to headline
            const std::string headline = "[" + t->thesis_id + ": " + t->name + "]";
            ThesisInjection inj;
            inj.thesis_id   = t->thesis_id;
            inj.text        = headline;
            inj.is_full     = false;
            inj.tokens_used = static_cast<int>(countTokens(headline));
            result.push_back(std::move(inj));
        }
    }

    // ── Step 4: all inactive theses → headline ────────────────────────────────
    for (const Thesis* t : inactive) {
        const std::string headline = "[" + t->thesis_id + ": " + t->name + "]";
        ThesisInjection inj;
        inj.thesis_id   = t->thesis_id;
        inj.text        = headline;
        inj.is_full     = false;
        inj.tokens_used = static_cast<int>(countTokens(headline));
        result.push_back(std::move(inj));
    }

    return result;
}

} // namespace prompt_engineering
} // namespace themis

