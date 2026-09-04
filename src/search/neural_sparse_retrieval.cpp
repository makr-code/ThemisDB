/**
 * @file neural_sparse_retrieval.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/neural_sparse_retrieval.h"
#include "utils/logger.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

NeuralSparseRetrieval::NeuralSparseRetrieval(const Config& config)
    : config_(config) {
    if (config_.k == 0) {
        throw std::invalid_argument("NeuralSparseRetrieval: k must be > 0");
    }
    if (config_.max_terms_per_doc == 0) {
        throw std::invalid_argument(
            "NeuralSparseRetrieval: max_terms_per_doc must be > 0");
    }
    if (config_.score_threshold < 0.0f) {
        throw std::invalid_argument(
            "NeuralSparseRetrieval: score_threshold must be >= 0");
    }
    THEMIS_DEBUG("NeuralSparseRetrieval initialized (k={}, max_terms={}, threshold={:.3f})",
                 config_.k, config_.max_terms_per_doc,
                 static_cast<double>(config_.score_threshold));
}

// ============================================================================
// Encoder management
// ============================================================================

void NeuralSparseRetrieval::setEncoder(SparseEncoderBackend encoder) {
    encoder_ = std::move(encoder);
    THEMIS_DEBUG("NeuralSparseRetrieval: encoder {}", encoder_ ? "attached" : "removed");
}

// ============================================================================
// sanitize (private static)
// ============================================================================

SparseVector NeuralSparseRetrieval::sanitize(const SparseVector& raw, size_t max_terms) {
    // Clamp negatives, collect (weight, term) pairs for truncation
    std::vector<std::pair<float, std::string>> pairs;
    pairs.reserve(raw.size());
    for (const auto& [term, weight] : raw) {
        if (weight > 0.0f) {
            pairs.emplace_back(weight, term);
        }
    }

    // Keep only the top max_terms by weight
    if (pairs.size() > max_terms) {
        std::partial_sort(pairs.begin(),
                          pairs.begin() + static_cast<std::ptrdiff_t>(max_terms),
                          pairs.end(),
                          [](const auto& a, const auto& b) { return a.first > b.first; });
        pairs.resize(max_terms);
    }

    SparseVector result;
    result.reserve(pairs.size());
    for (auto& [w, t] : pairs) {
        result.emplace(std::move(t), w);
    }
    return result;
}

// ============================================================================
// insertVector / eraseFromIndex (private helpers)
// ============================================================================

void NeuralSparseRetrieval::insertVector(const std::string& doc_id,
                                          const SparseVector& vec) {
    for (const auto& [term, weight] : vec) {
        inverted_index_[term].emplace_back(doc_id, weight);
    }
    forward_index_[doc_id] = vec;
}

void NeuralSparseRetrieval::eraseFromIndex(const std::string& doc_id,
                                            const SparseVector& vec) {
    for (const auto& [term, weight] : vec) {
        auto it = inverted_index_.find(term);
        if (it == inverted_index_.end()) {
          continue;
        }
        auto& posting = it->second;
        posting.erase(std::remove_if(posting.begin(), posting.end(),
                                     [&]([[maybe_unused]] const auto& p) { return p.first == doc_id; }),
                      posting.end());
        if (posting.empty()) {
            inverted_index_.erase(it);
        }
    }
}

// ============================================================================
// addDocument
// ============================================================================

void NeuralSparseRetrieval::addDocument(const std::string& doc_id,
                                         const SparseVector& sparse_vec) {
    if (doc_id.empty()) {
        THEMIS_WARN("NeuralSparseRetrieval::addDocument: empty doc_id, skipping");
        return;
    }

    // Remove existing entry if present
    auto fwd_it = forward_index_.find(doc_id);
    if (fwd_it != forward_index_.end()) {
        eraseFromIndex(doc_id, fwd_it->second);
        forward_index_.erase(fwd_it);
    }

    SparseVector sanitized = sanitize(sparse_vec, config_.max_terms_per_doc);
    if (sanitized.empty()) {
        THEMIS_DEBUG("NeuralSparseRetrieval::addDocument: doc='{}' has empty "
                     "sparse vector after sanitization", doc_id);
        return;
    }

    insertVector(doc_id, sanitized);
    THEMIS_DEBUG("NeuralSparseRetrieval::addDocument: doc='{}' indexed with {} terms",
                 doc_id, sanitized.size());
}

// ============================================================================
// addDocumentText
// ============================================================================

void NeuralSparseRetrieval::addDocumentText(const std::string& doc_id,
                                             const std::string& text) {
    if (!encoder_) {
        THEMIS_WARN("NeuralSparseRetrieval::addDocumentText: no encoder set, "
                    "doc='{}' not indexed", doc_id);
        return;
    }
    SparseVector vec = encoder_(text);
    addDocument(doc_id, vec);
}

// ============================================================================
// removeDocument
// ============================================================================

void NeuralSparseRetrieval::removeDocument(const std::string& doc_id) {
    auto fwd_it = forward_index_.find(doc_id);
    if (fwd_it == forward_index_.end()) {
      return;
    }
    eraseFromIndex(doc_id, fwd_it->second);
    forward_index_.erase(fwd_it);
    THEMIS_DEBUG("NeuralSparseRetrieval::removeDocument: doc='{}' removed", doc_id);
}

// ============================================================================
// clear
// ============================================================================

void NeuralSparseRetrieval::clear() {
    inverted_index_.clear();
    forward_index_.clear();
    THEMIS_DEBUG("NeuralSparseRetrieval::clear: index cleared");
}

// ============================================================================
// size
// ============================================================================

size_t NeuralSparseRetrieval::size() const {
    return forward_index_.size();
}

// ============================================================================
// normalizeScores (public static)
// ============================================================================

void NeuralSparseRetrieval::normalizeScores(std::vector<Result>& results) {
    if (results.empty()) {
      return;
    }

    float min_s = std::numeric_limits<float>::max();
    float max_s = std::numeric_limits<float>::lowest();
    for (const auto& r : results) {
        min_s = std::min(min_s, r.score);
        max_s = std::max(max_s, r.score);
    }

    const float range = max_s - min_s;
    if (range > 0.0f) {
        for (auto& r : results) {
            r.score = (r.score - min_s) / range;
        }
    } else {
        const float normalized = (max_s > 0.0f) ? 1.0f : 0.0f;
        for (auto& r : results) {
            r.score = normalized;
        }
    }
}

// ============================================================================
// search
// ============================================================================

std::vector<NeuralSparseRetrieval::Result>
NeuralSparseRetrieval::search(const SparseVector& query_vec, size_t k) const {
    const size_t top_k = (k > 0) ? k : config_.k;

    if (query_vec.empty() || forward_index_.empty()) {
        return {};
    }

    // Accumulate dot-product scores across all shared terms
    std::unordered_map<std::string, float> accum;
    accum.reserve(forward_index_.size());

    for (const auto& [term, q_weight] : query_vec) {
        if (q_weight <= 0.0f) {
          continue;
        }

        auto inv_it = inverted_index_.find(term);
        if (inv_it == inverted_index_.end()) {
          continue;
        }

        for (const auto& [doc_id, d_weight] : inv_it->second) {
            accum[doc_id] += q_weight * d_weight;
        }
    }

    // Build result list, applying score threshold
    std::vector<Result> results;
    results.reserve(accum.size());
    for (auto& [doc_id, raw_score] : accum) {
        if (raw_score < config_.score_threshold) {
          continue;
        }
        Result r;
        r.document_id = doc_id;
        r.raw_score   = raw_score;
        r.score       = raw_score;
        results.push_back(std::move(r));
    }

    // Sort by score descending
    std::sort(results.begin(), results.end(),
              [](const Result& a, const Result& b) {
                  return a.raw_score > b.raw_score;
              });

    if (results.size() > top_k) {
        results.resize(top_k);
    }

    if (config_.normalize_scores) {
        normalizeScores(results);
    }

    THEMIS_INFO("NeuralSparseRetrieval::search: query_terms={} -> {} results",
                query_vec.size(), results.size());

    return results;
}

// ============================================================================
// searchText
// ============================================================================

std::vector<NeuralSparseRetrieval::Result>
NeuralSparseRetrieval::searchText(const std::string& query_text, size_t k) const {
    if (!encoder_) {
        THEMIS_WARN("NeuralSparseRetrieval::searchText: no encoder set, "
                    "returning empty results");
        return {};
    }
    try {
        SparseVector query_vec = encoder_(query_text);
        return search(query_vec, k);
    } catch (const std::exception& e) {
        THEMIS_ERROR("NeuralSparseRetrieval::searchText: encoder threw: {}", e.what());
        return {};
    } catch (...) {
        THEMIS_ERROR("NeuralSparseRetrieval::searchText: encoder threw unknown exception");
        return {};
    }
}

} // namespace themis


