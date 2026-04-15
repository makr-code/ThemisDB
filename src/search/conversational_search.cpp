/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            conversational_search.cpp                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-15 04:19:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9a7d418c01  2026-04-11  feat(search): Phase 5 interfaces — ConversationalSearch, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
