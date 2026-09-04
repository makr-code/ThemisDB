/**
 * @file cross_lingual_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/cross_lingual_search.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

CrossLingualSearch::CrossLingualSearch(VectorIndexManager* vec_index)
    : CrossLingualSearch(vec_index, Config{}) {}

CrossLingualSearch::CrossLingualSearch(VectorIndexManager* vec_index,
                                       const Config& config)
    : vec_index_(vec_index), config_(config) {
    if (config_.k == 0) {
        throw std::invalid_argument("CrossLingualSearch: k must be > 0");
    }
    if (config_.candidates == 0) {
        throw std::invalid_argument("CrossLingualSearch: candidates must be > 0");
    }
    if (config_.rrf_k <= 0.0) {
        throw std::invalid_argument("CrossLingualSearch: rrf_k must be > 0");
    }
    // Clamp to hard resource limits
    config_.k          = std::min(config_.k,          config_.max_k);
    config_.candidates = std::min(config_.candidates, config_.max_candidates);
}

// ============================================================================
// setLanguageMap
// ============================================================================

void CrossLingualSearch::setLanguageMap(
    std::unordered_map<std::string, std::string> lang_map) {
    lang_map_ = std::move(lang_map);
}

// ============================================================================
// search
// ============================================================================

std::vector<CrossLingualSearch::Result> CrossLingualSearch::search(
    const std::vector<float>& query_embedding,
    const std::vector<LanguageHint>& language_hints) const {

    if (query_embedding.empty()) return {};

    auto scored = executeKnn(query_embedding);
    return applyHintsAndFinalize(std::move(scored), language_hints);
}

// ============================================================================
// searchMultiEmbedding
// ============================================================================

std::vector<CrossLingualSearch::Result> CrossLingualSearch::searchMultiEmbedding(
    const std::vector<EmbeddingQuery>& queries,
    const std::vector<LanguageHint>& language_hints) const {

    if (queries.empty()) return {};

    // Collect non-empty query ranked lists and their weights
    std::vector<std::vector<std::pair<std::string, double>>> ranked_lists;
    std::vector<double> weights = {};

    ranked_lists.reserve(queries.size());
    weights.reserve(queries.size());

    for (const auto& q : queries) {
        if (q.embedding.empty()) {
            THEMIS_DEBUG("CrossLingualSearch: skipping empty embedding in multi-embedding query");
            continue;
        }
        auto list = executeKnn(q.embedding);
        if (!list.empty()) {
            ranked_lists.push_back(std::move(list));
            weights.push_back(q.weight > 0.0 ? q.weight : 1.0);
        }
    }

    if (ranked_lists.empty()) return {};

    // Weighted Reciprocal Rank Fusion
    std::unordered_map<std::string, double> rrf_scores = {};

    for (size_t li = 0; li < ranked_lists.size(); ++li) {
        const double w = weights[li];
        const auto& list = ranked_lists[li];
        for (size_t rank = 0; rank < list.size(); ++rank) {
            const std::string& doc_id = list[rank].first;
            rrf_scores[doc_id] += w / (config_.rrf_k + static_cast<double>(rank + 1));
        }
    }

    // Collect fused scores into a sorted list
    std::vector<std::pair<std::string, double>> merged;
    merged.reserve(rrf_scores.size());
    for (auto& [doc_id, score] : rrf_scores) {
        merged.emplace_back(doc_id, score);
    }
    std::sort(merged.begin(), merged.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    THEMIS_DEBUG("CrossLingualSearch::searchMultiEmbedding: {} lists fused -> {} candidates",
                 ranked_lists.size(),static_cast<int>(merged.size()));

    return applyHintsAndFinalize(std::move(merged), language_hints);
}

// ============================================================================
// executeKnn (private)
// ============================================================================

std::vector<std::pair<std::string, double>> CrossLingualSearch::executeKnn(
    const std::vector<float>& embedding) const {

    if (!vec_index_) {
        THEMIS_DEBUG("CrossLingualSearch: vector index is null, skipping kNN");
        return {};
    }

    auto [st, knn_results] = vec_index_->searchKnn(embedding, config_.candidates);
    if (!st.ok) {
        THEMIS_WARN("CrossLingualSearch: kNN search failed: {}", st.message);
        return {};
    }

    std::vector<std::pair<std::string, double>> scored;
    scored.reserve(knn_results.size());
    for (const auto& r : knn_results) {
        // Convert distance (smaller-is-better) to similarity (larger-is-better)
        double sim = 1.0 / (1.0 + static_cast<double>(r.distance));
        scored.emplace_back(r.pk, sim);
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    return scored;
}

// ============================================================================
// applyHintsAndFinalize (private)
// ============================================================================

std::vector<CrossLingualSearch::Result> CrossLingualSearch::applyHintsAndFinalize(
    std::vector<std::pair<std::string, double>> scored,
    const std::vector<LanguageHint>& hints) const {

    // Build language-code → boost map for O(1) lookup
    std::unordered_map<std::string, double> boost_map = {};

    for (const auto& hint : hints) {
        if (!hint.language_code.empty() && hint.boost > 0.0) {
            boost_map[hint.language_code] = hint.boost;
        }
    }

    std::vector<Result> results = {};

    results.reserve(scored.size());

    for (auto& [doc_id, score] : scored) {
        // Language annotation from the language map
        std::string lang = {};
        auto lang_it = lang_map_.find(doc_id);
        if (lang_it != lang_map_.end()) {
            lang = lang_it->second;
        }

        // Apply language boost when available
        if (!lang.empty()) {
            auto boost_it = boost_map.find(lang);
            if (boost_it != boost_map.end()) {
                score *= boost_it->second;
            }
        }

        // Score threshold filter
        if (score < config_.score_threshold) {
            continue;
        }

        Result r;
        r.document_id = doc_id;
        r.score       = score;
        r.language    = std::move(lang);
        results.push_back(std::move(r));
    }

    // Re-sort after boost adjustments (boosts may change relative order)
    std::sort(results.begin(), results.end(),
              [](const Result& a, const Result& b) { return a.score > b.score; });

    if (static_cast<int>(results.size()) > config_.k) {
        results.resize(config_.k);
    }

    THEMIS_DEBUG("CrossLingualSearch: {} results (k={})",static_cast<int>(results.size()), config_.k);
    return results;
}

} // namespace themis
