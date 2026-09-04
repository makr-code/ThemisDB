/**
 * @file hybrid_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/hybrid_retriever.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

/// Normalise scores in-place to [0, 1].  No-op when all values are equal.
void normaliseScores(std::vector<double>& scores) {
    if (scores.empty()) {
      return;
    }

    const auto [min_it, max_it] =
        std::minmax_element(scores.begin(), scores.end());
    const double lo = *min_it;
    const double hi = *max_it;
    const double range = hi - lo;

    if (std::abs(range) < std::numeric_limits<double>::epsilon()) {
        // All identical: map to 1 when score > 0, else 0.
        const double uniform = (hi > 0.0) ? 1.0 : 0.0;
        for (auto& s : scores) { s = uniform; }
    } else {
        for (auto& s : scores) { s = (s - lo) / range; }
    }
}

double cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty()) {
        return 0.0;
    }

    const size_t dim = std::min(a.size(), b.size());
    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;
    for (size_t i = 0; i < dim; ++i) {
        const double av = static_cast<double>(a[i]);
        const double bv = static_cast<double>(b[i]);
        dot += av * bv;
        norm_a += av * av;
        norm_b += bv * bv;
    }

    if (norm_a <= 0.0 || norm_b <= 0.0) {
        return 0.0;
    }
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// HybridRetriever – construction & config
// ---------------------------------------------------------------------------

HybridRetriever::HybridRetriever()
    : config_{} {}

HybridRetriever::HybridRetriever(const HybridRetrieverConfig& config)
    : config_(config) {
    validateConfig(config);
}

void HybridRetriever::validateConfig(const HybridRetrieverConfig& config) {
    if (config.bm25_weight < 0.0) {
        throw std::invalid_argument(
            "HybridRetriever: bm25_weight must be >= 0");
    }
    if (config.vector_weight < 0.0) {
        throw std::invalid_argument(
            "HybridRetriever: vector_weight must be >= 0");
    }
    if (config.rrf_k <= 0.0) {
        throw std::invalid_argument(
            "HybridRetriever: rrf_k must be > 0");
    }
}

const HybridRetrieverConfig& HybridRetriever::getConfig() const {
    return config_;
}

void HybridRetriever::setConfig(const HybridRetrieverConfig& config) {
    validateConfig(config);
    config_ = config;
}

void HybridRetriever::setVectorizer(std::shared_ptr<IVectorizer> vectorizer) {
    vectorizer_ = std::move(vectorizer);
}

std::shared_ptr<IVectorizer> HybridRetriever::getVectorizer() const {
    return vectorizer_;
}

// ---------------------------------------------------------------------------
// HybridRetriever::fuse – public entry point
// ---------------------------------------------------------------------------

HybridFusionResult HybridRetriever::fuse(
    const std::vector<judge::RetrievedDocument>& bm25_candidates,
    const std::vector<judge::RetrievedDocument>& vector_candidates
) const {
    THEMIS_DEBUG("HybridRetriever::fuse: bm25={}, vector={}, use_rrf={}, "
                 "bm25_w={:.2f}, vec_w={:.2f}, rrf_k={:.0f}, top_k={}",
                 bm25_candidates.size(), vector_candidates.size(),
                 config_.use_rrf, config_.bm25_weight, config_.vector_weight,
                 config_.rrf_k, config_.top_k);

    if (config_.use_rrf) {
        return fuseRRF(bm25_candidates, vector_candidates);
    }
    return fuseLinear(bm25_candidates, vector_candidates);
}

HybridFusionResult HybridRetriever::retrieveWithVectorizer(
    const std::string& query,
    const std::vector<judge::RetrievedDocument>& bm25_candidates
) const {
    if (query.empty()) {
        throw std::invalid_argument("HybridRetriever::retrieveWithVectorizer: query must not be empty");
    }
    if (!vectorizer_) {
        throw std::runtime_error("HybridRetriever::retrieveWithVectorizer: no vectorizer configured");
    }
    if (!vectorizer_->isInitialized()) {
        throw std::runtime_error("HybridRetriever::retrieveWithVectorizer: vectorizer is not initialized");
    }
    if (bm25_candidates.empty()) {
        return fuse({}, {});
    }

    const auto query_embedding = vectorizer_->encodeQuery(query);
    std::vector<judge::RetrievedDocument> vector_candidates;
    vector_candidates.reserve(bm25_candidates.size());

    for (const auto& doc : bm25_candidates) {
        auto dense_doc = doc;
        if (doc.content.empty()) {
            dense_doc.similarity_score = 0.0;
        } else {
            const auto passage_embedding = vectorizer_->encodePassage(doc.content);
            dense_doc.similarity_score = cosineSimilarity(query_embedding, passage_embedding);
        }
        vector_candidates.push_back(std::move(dense_doc));
    }

    std::stable_sort(
        vector_candidates.begin(),
        vector_candidates.end(),
        [](const judge::RetrievedDocument& lhs, const judge::RetrievedDocument& rhs) {
            return lhs.similarity_score > rhs.similarity_score;
        });

    return fuse(bm25_candidates, vector_candidates);
}

// ---------------------------------------------------------------------------
// RRF fusion
// ---------------------------------------------------------------------------

HybridFusionResult HybridRetriever::fuseRRF(
    const std::vector<judge::RetrievedDocument>& bm25_candidates,
    const std::vector<judge::RetrievedDocument>& vector_candidates
) const {
    // Build a map: document_id -> accumulated score data.
    struct DocData {
        judge::RetrievedDocument doc;
        HybridScore              score;
    };
    std::unordered_map<std::string, DocData> doc_map;
    doc_map.reserve(bm25_candidates.size() + vector_candidates.size());

    // Process BM25 list in the order provided (assumed sorted descending).
    for (size_t i = 0; i < bm25_candidates.size(); ++i) {
        const auto& src = bm25_candidates[i];
        auto& entry = doc_map[src.id];
        if (entry.doc.id.empty()) {
            entry.doc = src;
        }
        entry.score.document_id = src.id;
        entry.score.bm25_score  = src.similarity_score;
        entry.score.bm25_rank   = static_cast<int>(i + 1);

        const double rrf = 1.0 / (config_.rrf_k + static_cast<double>(i + 1));
        entry.score.hybrid_score += config_.bm25_weight * rrf;
    }

    // Process vector list.
    for (size_t i = 0; i < vector_candidates.size(); ++i) {
        const auto& src = vector_candidates[i];
        auto& entry = doc_map[src.id];
        if (entry.doc.id.empty()) {
            entry.doc = src;
        }
        entry.score.document_id  = src.id;
        entry.score.vector_score = src.similarity_score;
        entry.score.vector_rank  = static_cast<int>(i + 1);

        const double rrf = 1.0 / (config_.rrf_k + static_cast<double>(i + 1));
        entry.score.hybrid_score += config_.vector_weight * rrf;
    }

    // Collect and sort by descending hybrid score.
    std::vector<DocData> entries;
    entries.reserve(doc_map.size());
    for (auto& [_, data] : doc_map) {
        entries.push_back(std::move(data));
    }
    std::stable_sort(entries.begin(), entries.end(),
        [](const DocData& a, const DocData& b) {
            return a.score.hybrid_score > b.score.hybrid_score;
        });

    // Apply top_k truncation.
    if (config_.top_k > 0 && entries.size() > config_.top_k) {
        entries.resize(config_.top_k);
    }

    // Build result.
    HybridFusionResult result;
    result.total_candidates = doc_map.size();
    result.used_rrf         = true;
    result.documents.reserve(entries.size());
    result.scores.reserve(entries.size());

    for (auto& e : entries) {
        e.doc.similarity_score = e.score.hybrid_score;
        result.documents.push_back(std::move(e.doc));
        result.scores.push_back(std::move(e.score));
    }

    THEMIS_INFO("HybridRetriever (RRF): {} candidates → {} results",
                result.total_candidates, result.documents.size());
    return result;
}

// ---------------------------------------------------------------------------
// Linear combination fusion
// ---------------------------------------------------------------------------

HybridFusionResult HybridRetriever::fuseLinear(
    const std::vector<judge::RetrievedDocument>& bm25_candidates,
    const std::vector<judge::RetrievedDocument>& vector_candidates
) const {
    struct DocData {
        judge::RetrievedDocument doc;
        HybridScore              score;
    };
    std::unordered_map<std::string, DocData> doc_map;
    doc_map.reserve(bm25_candidates.size() + vector_candidates.size());

    // Collect raw BM25 scores for optional normalisation.
    std::vector<double> bm25_raw;
    bm25_raw.reserve(bm25_candidates.size());
    for (const auto& src : bm25_candidates) {
        bm25_raw.push_back(src.similarity_score);
    }
    if (config_.normalize_scores) { normaliseScores(bm25_raw); }

    std::vector<double> vec_raw;
    vec_raw.reserve(vector_candidates.size());
    for (const auto& src : vector_candidates) {
        vec_raw.push_back(src.similarity_score);
    }
    if (config_.normalize_scores) { normaliseScores(vec_raw); }

    for (size_t i = 0; i < bm25_candidates.size(); ++i) {
        const auto& src = bm25_candidates[i];
        auto& entry = doc_map[src.id];
        if (entry.doc.id.empty()) { entry.doc = src; }
        entry.score.document_id = src.id;
        entry.score.bm25_score  = src.similarity_score;
        entry.score.bm25_rank   = static_cast<int>(i + 1);
        entry.score.hybrid_score += config_.bm25_weight * bm25_raw[i];
    }

    for (size_t i = 0; i < vector_candidates.size(); ++i) {
        const auto& src = vector_candidates[i];
        auto& entry = doc_map[src.id];
        if (entry.doc.id.empty()) { entry.doc = src; }
        entry.score.document_id  = src.id;
        entry.score.vector_score = src.similarity_score;
        entry.score.vector_rank  = static_cast<int>(i + 1);
        entry.score.hybrid_score += config_.vector_weight * vec_raw[i];
    }

    std::vector<DocData> entries;
    entries.reserve(doc_map.size());
    for (auto& [_, data] : doc_map) {
        entries.push_back(std::move(data));
    }
    std::stable_sort(entries.begin(), entries.end(),
        [](const DocData& a, const DocData& b) {
            return a.score.hybrid_score > b.score.hybrid_score;
        });

    if (config_.top_k > 0 && entries.size() > config_.top_k) {
        entries.resize(config_.top_k);
    }

    HybridFusionResult result;
    result.total_candidates = doc_map.size();
    result.used_rrf         = false;
    result.documents.reserve(entries.size());
    result.scores.reserve(entries.size());

    for (auto& e : entries) {
        e.doc.similarity_score = e.score.hybrid_score;
        result.documents.push_back(std::move(e.doc));
        result.scores.push_back(std::move(e.score));
    }

    THEMIS_INFO("HybridRetriever (Linear): {} candidates → {} results",
                result.total_candidates, result.documents.size());
    return result;
}

// ---------------------------------------------------------------------------
// HybridRetrieverFactory
// ---------------------------------------------------------------------------

HybridRetriever HybridRetrieverFactory::createBalanced([[maybe_unused]] size_t top_k) {
    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.5;
    cfg.vector_weight = 0.5;
    cfg.use_rrf       = true;
    cfg.rrf_k         = 60.0;
    cfg.top_k         = top_k;
    return HybridRetriever(cfg);
}

HybridRetriever HybridRetrieverFactory::createSemanticFocused([[maybe_unused]] size_t top_k) {
    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.3;
    cfg.vector_weight = 0.7;
    cfg.use_rrf       = true;
    cfg.rrf_k         = 60.0;
    cfg.top_k         = top_k;
    return HybridRetriever(cfg);
}

HybridRetriever HybridRetrieverFactory::createKeywordFocused([[maybe_unused]] size_t top_k) {
    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.7;
    cfg.vector_weight = 0.3;
    cfg.use_rrf       = true;
    cfg.rrf_k         = 60.0;
    cfg.top_k         = top_k;
    return HybridRetriever(cfg);
}

} // namespace themis::rag
