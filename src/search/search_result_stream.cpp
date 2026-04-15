/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            search_result_stream.cpp                           ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:14:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     158                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9a7d418c01  2026-04-11  feat(search): Phase 5 interfaces — ConversationalSearch, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "search/search_result_stream.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

SearchResultStream::SearchResultStream(HybridSearch* hybrid_search)
    : SearchResultStream(hybrid_search, Config{}) {}

SearchResultStream::SearchResultStream(HybridSearch* hybrid_search,
                                       const Config& config)
    : hybrid_search_(hybrid_search), config_(config) {
    if (config_.total_k == 0) {
        throw std::invalid_argument(
            "SearchResultStream: total_k must be > 0");
    }
    if (config_.page_size == 0) {
        throw std::invalid_argument(
            "SearchResultStream: page_size must be > 0");
    }
}

// ============================================================================
// open
// ============================================================================

void SearchResultStream::open(const std::string& query,
                               const std::vector<float>& /*vector_query*/) {
    close();

    if (!hybrid_search_ || query.empty()) {
        return;
    }

    // Temporarily override k to fetch total_k results, restoring it afterwards
    HybridSearch::Config hs_cfg = hybrid_search_->getConfig();
    const size_t saved_k = hs_cfg.k;
    hs_cfg.k = config_.total_k;
    hybrid_search_->setConfig(hs_cfg);

    try {
        results_ = hybrid_search_->search(query);
    } catch (const std::exception& e) {
        THEMIS_ERROR("SearchResultStream: open failed: {}", e.what());
        results_.clear();
    } catch (...) {
        THEMIS_ERROR("SearchResultStream: open failed with unknown error");
        results_.clear();
    }

    // Always restore saved k
    hs_cfg.k = saved_k;
    hybrid_search_->setConfig(hs_cfg);
    cursor_ = 0;
}

// ============================================================================
// nextPage
// ============================================================================

std::vector<HybridSearch::Result> SearchResultStream::nextPage() {
    if (!hasMore()) {
        return {};
    }
    const size_t end = std::min(cursor_ + config_.page_size, results_.size());
    std::vector<HybridSearch::Result> page(results_.begin() + static_cast<std::ptrdiff_t>(cursor_),
                                            results_.begin() + static_cast<std::ptrdiff_t>(end));
    cursor_ = end;
    return page;
}

// ============================================================================
// hasMore
// ============================================================================

bool SearchResultStream::hasMore() const {
    return cursor_ < results_.size();
}

// ============================================================================
// reset / close
// ============================================================================

void SearchResultStream::reset() {
    cursor_ = 0;
}

void SearchResultStream::close() {
    results_.clear();
    cursor_ = 0;
}

// ============================================================================
// forEachResult
// ============================================================================

void SearchResultStream::forEachResult(ResultCallback callback) {
    if (!callback) return;
    while (cursor_ < results_.size()) {
        try {
            if (!callback(results_[cursor_])) {
                break;
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("SearchResultStream: callback threw: {}", e.what());
            break;
        } catch (...) {
            THEMIS_ERROR("SearchResultStream: callback threw unknown error");
            break;
        }
        ++cursor_;
    }
}

// ============================================================================
// setConfig
// ============================================================================

void SearchResultStream::setConfig(const Config& config) {
    if (config.total_k == 0) {
        throw std::invalid_argument(
            "SearchResultStream: total_k must be > 0");
    }
    if (config.page_size == 0) {
        throw std::invalid_argument(
            "SearchResultStream: page_size must be > 0");
    }
    config_ = config;
}

} // namespace themis
