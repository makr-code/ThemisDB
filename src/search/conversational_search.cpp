/**
 * @file conversational_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/conversational_search.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

ConversationalSearch::ConversationalSearch(HybridSearch* hybrid_search)
    : ConversationalSearch(hybrid_search, Config{}) {}

ConversationalSearch::ConversationalSearch(HybridSearch* hybrid_search,
                                           const Config& config)
    : hybrid_search_(hybrid_search), config_(config) {
    if (config_.max_history == 0) {
        throw std::invalid_argument(
            "ConversationalSearch: max_history must be > 0");
    }
}

// ============================================================================
// search
// ============================================================================

std::vector<HybridSearch::Result> ConversationalSearch::search(
    const std::string& query) {

    if (query.empty()) {
        return {};
    }

    const std::string enriched = reformulate(query);

    std::vector<HybridSearch::Result> results;
    if (hybrid_search_) {
        try {
            results = hybrid_search_->search(enriched);
        } catch (const std::exception& e) {
            THEMIS_ERROR("ConversationalSearch: search failed: {}", e.what());
        } catch (...) {
            THEMIS_ERROR("ConversationalSearch: search failed with unknown error");
        }
    }

    // Append turn to history, evicting oldest entry when limit is reached
    if (history_.size() >= config_.max_history) {
        history_.pop_front();
    }
    Turn turn;
    turn.query              = query;
    turn.reformulated_query = enriched;
    turn.results            = results;
    history_.push_back(std::move(turn));

    return results;
}

// ============================================================================
// reformulate
// ============================================================================

std::string ConversationalSearch::reformulate(const std::string& query) const {
    if (config_.context_window == 0 || history_.empty()) {
        return query;
    }

    // Collect the most recent context_window queries from history
    const size_t window = std::min(config_.context_window, history_.size());
    const size_t start  = history_.size() - window;

    std::string enriched;
    for (size_t i = start; i < history_.size(); ++i) {
        if (!history_[i].query.empty()) {
            enriched += history_[i].query;
            enriched += config_.context_separator;
        }
    }
    enriched += query;
    return enriched;
}

// ============================================================================
// clearHistory
// ============================================================================

void ConversationalSearch::clearHistory() {
    history_.clear();
}

// ============================================================================
// setConfig
// ============================================================================

void ConversationalSearch::setConfig(const Config& config) {
    if (config.max_history == 0) {
        throw std::invalid_argument(
            "ConversationalSearch: max_history must be > 0");
    }
    config_ = config;
}

} // namespace themis

