/**
 * @file learning_to_rank.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/learning_to_rank.h"
#include "utils/logger.h"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

LearningToRank::LearningToRank(const Config& config) : config_(config) {
    if (config_.learning_rate <= 0.0) {
        throw std::invalid_argument("LearningToRank: learning_rate must be > 0");
    }
    if (config_.max_click_buffer == 0) {
        throw std::invalid_argument("LearningToRank: max_click_buffer must be > 0");
    }
    if (config_.regularization < 0.0) {
        throw std::invalid_argument("LearningToRank: regularization must be >= 0");
    }
    // Initialize weights proportionally; sum = 1.0
    // bm25 and vector are primary signals (each 0.3), rrf is secondary (0.2),
    // click and popularity are behavioral signals (0.1 each), recency last (0.1)
    weights_.bm25_score    = 0.3;
    weights_.vector_score  = 0.3;
    weights_.rrf_score     = 0.2;
    weights_.recency       = 0.1;
    weights_.click_count   = 0.1;
    weights_.popularity    = 0.0; // starts at zero; grows from training
}

// ============================================================================
// Re-ranking
// ============================================================================

double LearningToRank::score(const RankingFeatures& f) const {
    return dot(weights_, f);
}

std::vector<RankedResult> LearningToRank::rerank(std::vector<RankedResult> candidates) const {
    for (auto& c : candidates) {
        c.final_score = score(c.features);
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const RankedResult& a, const RankedResult& b) {
                  return a.final_score > b.final_score;
              });
    return candidates;
}

std::vector<RankedResult> LearningToRank::rerankWithVariant(
    std::vector<RankedResult> candidates,
    const std::string& variant_name) const {

    auto it = variants_.find(variant_name);
    if (it == variants_.end()) {
        THEMIS_DEBUG("LearningToRank::rerankWithVariant: variant '{}' not found, using default",
                     variant_name);
        return rerank(std::move(candidates));
    }

    const auto& variant_scorer = it->second.scorer;
    for (auto& c : candidates) {
        c.final_score = variant_scorer(c.features);
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const RankedResult& a, const RankedResult& b) {
                  return a.final_score > b.final_score;
              });
    return candidates;
}

// ============================================================================
// Training
// ============================================================================

void LearningToRank::recordClick(const ClickEvent& event) {
    if (static_cast<int>(clicks_.size()) >= config_.max_click_buffer) {
        // Evict oldest event when buffer is full
        clicks_.erase(clicks_.begin());
    }
    clicks_.push_back(event);
}

size_t LearningToRank::train() {
    if (clicks_.empty()) {
      return 0;
    }

    const size_t n_events = clicks_.size();

    // Pairwise gradient descent using position-based implicit feedback:
    // For each click event, the clicked document should score higher than
    // documents at rank positions 0..(position-1).
    // Since we don't have per-document features at training time (only query+pk),
    // we use proxy features: clicked document gets unit positive features,
    // documents ranked above it get uniform negative gradient.
    for (const auto& ev : clicks_) {
        if (ev.result_position == 0) continue; // clicked at top, no pairwise loss

        // Positive example: clicked doc (heuristic features)
        RankingFeatures pos_features;
        pos_features.click_count   = 1.0;
        pos_features.popularity    = 1.0;
        pos_features.bm25_score    = 0.5;
        pos_features.vector_score  = 0.5;
        pos_features.rrf_score     = 0.5;
        pos_features.recency       = 0.5;

        // Negative example: average document ranked above the click
        RankingFeatures neg_features;
        neg_features.click_count   = 0.0;
        neg_features.popularity    = 0.0;
        neg_features.bm25_score    = 1.0 / static_cast<double>(ev.result_position + 1);
        neg_features.vector_score  = 1.0 / static_cast<double>(ev.result_position + 1);
        neg_features.rrf_score     = 1.0 / static_cast<double>(ev.result_position + 1);
        neg_features.recency       = 0.5;

        // Pairwise hinge gradient
        double score_pos = dot(weights_, pos_features);
        double score_neg = dot(weights_, neg_features);
        if (score_pos - score_neg < 1.0) {
            // Update: w += lr * (pos - neg)
            auto g = gradient(pos_features, neg_features);
            weights_ = addScaled(weights_, g, config_.learning_rate);
        }
    }

    // L2 regularization
    if (config_.regularization > 0.0) {
        weights_ = regularize(weights_, config_.regularization * config_.learning_rate);
    }

    THEMIS_DEBUG("LearningToRank::train(): processed {} click events", n_events);
    clicks_.clear();
    return n_events;
}

// ============================================================================
// A/B Testing
// ============================================================================

void LearningToRank::registerVariant(const Variant& variant) {
    variants_[variant.name] = variant;
    THEMIS_DEBUG("LearningToRank: registered variant '{}'", variant.name);
}

std::string LearningToRank::selectVariant(const std::string& request_key) const {
    if (variants_.empty()) return {};

    // Deterministic hash of the request key → consistent assignment
    size_t hash = std::hash<std::string>{}(request_key);
    double fraction = static_cast<double>(hash % 1000) / 1000.0;

    double cumulative = 0.0;
    for (const auto& [name, variant] : variants_) {
        cumulative += variant.traffic_fraction;
        if (fraction < cumulative) {
          return name;
        }
    }
    return {}; // default model
}

// ============================================================================
// Static helpers
// ============================================================================

double LearningToRank::dot(const RankingFeatures& w, const RankingFeatures& f) {
    return w.bm25_score    * f.bm25_score
         + w.vector_score  * f.vector_score
         + w.rrf_score     * f.rrf_score
         + w.recency       * f.recency
         + w.click_count   * f.click_count
         + w.popularity    * f.popularity;
}

RankingFeatures LearningToRank::gradient(const RankingFeatures& pos,
                                          const RankingFeatures& neg) {
    RankingFeatures g;
    g.bm25_score   = pos.bm25_score   - neg.bm25_score;
    g.vector_score = pos.vector_score - neg.vector_score;
    g.rrf_score    = pos.rrf_score    - neg.rrf_score;
    g.recency      = pos.recency      - neg.recency;
    g.click_count  = pos.click_count  - neg.click_count;
    g.popularity   = pos.popularity   - neg.popularity;
    return g;
}

RankingFeatures LearningToRank::addScaled(const RankingFeatures& w,
                                           const RankingFeatures& g,
                                           double lr) {
    RankingFeatures result;
    result.bm25_score   = w.bm25_score   + lr * g.bm25_score;
    result.vector_score = w.vector_score + lr * g.vector_score;
    result.rrf_score    = w.rrf_score    + lr * g.rrf_score;
    result.recency      = w.recency      + lr * g.recency;
    result.click_count  = w.click_count  + lr * g.click_count;
    result.popularity   = w.popularity   + lr * g.popularity;
    return result;
}

RankingFeatures LearningToRank::regularize(const RankingFeatures& w, double decay) {
    RankingFeatures result;
    result.bm25_score   = w.bm25_score   * (1.0 - decay);
    result.vector_score = w.vector_score * (1.0 - decay);
    result.rrf_score    = w.rrf_score    * (1.0 - decay);
    result.recency      = w.recency      * (1.0 - decay);
    result.click_count  = w.click_count  * (1.0 - decay);
    result.popularity   = w.popularity   * (1.0 - decay);
    return result;
}

} // namespace themis
