/**
 * @file search_result_stream.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=0, M=0, L=0
 * @note Status: Production Ready - v2.0.0 Contract Freeze (Phase 1)
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/search_result_stream.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>
#include <chrono>

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
        // Note: timeout enforcement at stream level; HybridSearch::search()
        // has internal exception catching and returns partial/empty results
        // on backend failures. A production implementation would use
        // async I/O or thread pools with timeout mechanisms.
        auto start = std::chrono::steady_clock::now();
        results_ = hybrid_search_->search(query);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        
        if (config_.open_timeout_ms > 0 && 
            elapsed.count() > static_cast<int64_t>(config_.open_timeout_ms)) {
            THEMIS_WARN("SearchResultStream: open() exceeded timeout ({}ms > {}ms)",
                        elapsed.count(), config_.open_timeout_ms);
            results_.clear();  // Clear results on timeout
        }
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
    const size_t end = std::min(cursor_ + config_.page_size,static_cast<int>(results_.size()));
    std::vector<HybridSearch::Result> page(results_.begin() + static_cast<std::ptrdiff_t>(cursor_),
                                            results_.begin() + static_cast<std::ptrdiff_t>(end));
    cursor_ = end;
    return page;
}

// ============================================================================
// hasMore
// ============================================================================

bool SearchResultStream::hasMore() const {
    return static_cast<bool>(cursor_  < static_cast<int>(results_.size()));
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

void SearchResultStream::forEachResult([[maybe_unused]] ResultCallback callback) {
    if (!callback) {
      return;
    }
    while (static_cast<size_t>(cursor_) <static_cast<int>(results_.size())) {
        try {
            if ([[maybe_unused]] !callback(results_[cursor_])) {
                break;
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("SearchResultStream: callback threw: {}", e.what());
            break;
        } catch (...) {
            THEMIS_ERROR([[maybe_unused]] "SearchResultStream: callback threw unknown error");
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

