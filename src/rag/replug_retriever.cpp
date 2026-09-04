/**
 * @file replug_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/replug_retriever.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis::rag {

// ============================================================
// Internal helpers
// ============================================================
namespace {

/// Tokenise a string into a set of lowercase words for Jaccard similarity.
std::unordered_set<std::string> tokenise(const std::string& text) {
    std::unordered_set<std::string> tokens;
    std::istringstream ss(text);
    std::string word;
    while (ss >> word) {
        // Lower-case and strip trailing punctuation.
        std::string lower;
        lower.reserve(word.size());
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                lower += static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (!lower.empty()) {
            tokens.insert(std::move(lower));
        }
    }
    return tokens;
}

/// Compute Jaccard similarity between two token sets.
double jaccardSimilarity(const std::unordered_set<std::string>& a,
                         const std::unordered_set<std::string>& b) {
    if (a.empty() && b.empty()) {
        return 1.0;
    }
    size_t intersection = 0;
    for (const auto& tok : a) {
        if (b.count(tok)) {
            ++intersection;
        }
    }
    const size_t union_size = a.size() + b.size() - intersection;
    return union_size == 0 ? 0.0
                           : static_cast<double>(intersection) /
                                 static_cast<double>(union_size);
}

} // anonymous namespace

// ============================================================
// HeuristicLLMScorer
// ============================================================

double HeuristicLLMScorer::score(const std::string& query,
                                  const std::string& document) const {
    if (query.empty() || document.empty()) {
        return 0.0;
    }
    const auto q_tokens = tokenise(query);
    const auto d_tokens = tokenise(document);
    return jaccardSimilarity(q_tokens, d_tokens);
}

// ============================================================
// ReplugRetriever — construction & config
// ============================================================

ReplugRetriever::ReplugRetriever()
    : config_{}
    , scorer_(std::make_shared<HeuristicLLMScorer>()) {}

ReplugRetriever::ReplugRetriever(const ReplugConfig&         config,
                                  std::shared_ptr<ILLMScorer> scorer)
    : ReplugRetriever() {
    setConfig(config);
    setScorer(std::move(scorer));
}

void ReplugRetriever::validateConfig(const ReplugConfig& config) {
    if (config.llm_weight < 0.0 || config.llm_weight > 1.0) {
        throw std::invalid_argument(
            "ReplugRetriever: llm_weight must be in [0, 1]");
    }
    if (config.temperature <= 0.0) {
        throw std::invalid_argument(
            "ReplugRetriever: temperature must be > 0");
    }
    if (config.weight_update_lr < 0.0) {
        throw std::invalid_argument(
            "ReplugRetriever: weight_update_lr must be >= 0");
    }
    if (config.min_retrieval_score < 0.0) {
        throw std::invalid_argument(
            "ReplugRetriever: min_retrieval_score must be >= 0");
    }
}

const ReplugConfig& ReplugRetriever::getConfig() const {
    return config_;
}

void ReplugRetriever::setConfig(const ReplugConfig& config) {
    validateConfig(config);
    config_ = config;
}

void ReplugRetriever::setScorer(std::shared_ptr<ILLMScorer> scorer) {
    scorer_ = scorer ? std::move(scorer)
                     : std::make_shared<HeuristicLLMScorer>();
}

std::string ReplugRetriever::scorerName() const {
    return scorer_ ? scorer_->name() : "none";
}

// ============================================================
// Static math helpers
// ============================================================

void ReplugRetriever::applySoftmax(std::vector<double>& scores,
                                    double temperature) {
    if (scores.empty()) {
        return;
    }
    // Numerically stable softmax: subtract max before exp.
    const double max_val =
        *std::max_element(scores.begin(), scores.end());
    double sum = 0.0;
    for (auto& s : scores) {
        s = std::exp((s - max_val) / temperature);
        sum += s;
    }
    if (sum > 0.0) {
        for (auto& s : scores) {
            s /= sum;
        }
    }
}

void ReplugRetriever::normalise(std::vector<double>& scores) {
    if (scores.empty()) {
        return;
    }
    const auto [lo_it, hi_it] =
        std::minmax_element(scores.begin(), scores.end());
    const double lo    = *lo_it;
    const double hi    = *hi_it;
    const double range = hi - lo;
    if (std::abs(range) < std::numeric_limits<double>::epsilon()) {
        // All identical: map to 1 when value > 0, else 0.
        const double uniform = (hi > 0.0) ? 1.0 : 0.0;
        for (auto& s : scores) {
            s = uniform;
        }
    } else {
        for (auto& s : scores) {
            s = (s - lo) / range;
        }
    }
}

std::vector<double> ReplugRetriever::computeKLGradients(
    const std::vector<double>& retrieval_probs,
    const std::vector<double>& llm_probs) {
    // KL(llm || retrieval) gradient wrt retrieval_probs[i]:
    //   ∂KL/∂p_i = -llm_i / p_i   (for p_i > 0)
    // We return -gradient so that adding it to the weight increases
    // retrieval weight for documents with high llm/retrieval ratio.
    const size_t n = retrieval_probs.size();
    std::vector<double> grad(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        const double p_r = retrieval_probs[i];
        const double p_l = llm_probs[i];
        if (p_r > 1e-12) {
            // Positive gradient ⟹ weight should increase.
            grad[i] = p_l / p_r - 1.0;
        }
    }
    return grad;
}

// ============================================================
// fuse()
// ============================================================

std::vector<double> ReplugRetriever::computeLLMScores(
    const std::string&                           query,
    const std::vector<judge::RetrievedDocument>& candidates) const {
    std::vector<double> raw = {};

    raw.reserve(candidates.size());
    for (const auto& doc : candidates) {
        raw.push_back(scorer_->score(query, doc.content));
    }
    return raw;
}

ReplugFusionResult ReplugRetriever::fuse(
    const std::string&                           query,
    const std::vector<judge::RetrievedDocument>& candidates) const {
    ReplugFusionResult result;
    result.scorer_name     = scorerName();
    result.total_candidates = candidates.size();

    if (candidates.empty()) {
        return result;
    }

    // ── Step 1: filter by min_retrieval_score ──────────────────────────────
    std::vector<judge::RetrievedDocument> filtered = {};

    filtered.reserve(candidates.size());
    for (const auto& doc : candidates) {
        if (doc.similarity_score >= config_.min_retrieval_score) {
            filtered.push_back(doc);
        }
    }
    if (filtered.empty()) {
        return result;
    }

    // ── Step 2: collect and normalise retrieval scores ─────────────────────
    std::vector<double> ret_scores = {};

    ret_scores.reserve(filtered.size());
    for (const auto& doc : filtered) {
        // Apply per-document learned weight (REPLUG-LSR).
        const double w = [this, &doc]() -> double {
            auto it = weights_.find(doc.id);
            return it != weights_.end() ? it->second : 1.0;
        }();
        ret_scores.push_back(doc.similarity_score * w);
    }
    normalise(ret_scores); // ret_probs ∈ [0,1]

    // Convert to a probability distribution (softmax with τ=1 after normalise
    // already gives values in [0,1]; we use a simple normalised vector).
    std::vector<double> ret_probs = ret_scores;
    {
        double sum = std::accumulate(ret_probs.begin(), ret_probs.end(), 0.0);
        if (sum > 0.0) {
            for (auto& p : ret_probs) { p /= sum; }
        }
    }

    // ── Step 3: LLM scores ────────────────────────────────────────────────
    std::vector<double> llm_raw = computeLLMScores(query, filtered);

    // ── Step 4: softmax with temperature ─────────────────────────────────
    std::vector<double> llm_probs = llm_raw;
    applySoftmax(llm_probs, config_.temperature);

    // Normalise raw LLM scores for display in the breakdown.
    normalise(llm_raw);

    // ── Step 5: KL gradients (for weight update) ──────────────────────────
    std::vector<double> kl_grads = computeKLGradients(ret_probs, llm_probs);

    // ── Step 6: interpolate ────────────────────────────────────────────────
    const double lam = config_.llm_weight;
    std::vector<double> fused = {};

    fused.reserve(filtered.size());
    for (size_t i = 0; i < filtered.size(); ++i) {
        fused.push_back((1.0 - lam) * ret_scores[i] + lam * llm_probs[i]);
    }

    // ── Step 7: sort descending ────────────────────────────────────────────
    std::vector<size_t> indices(filtered.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::stable_sort(indices.begin(), indices.end(),
                     [&fused](size_t a, size_t b) {
                         return fused[a] > fused[b];
                     });

    // ── Step 8: apply top_k and build output ─────────────────────────────
    const size_t limit =
        (config_.top_k == 0 || config_.top_k >= indices.size())
            ? indices.size()
            : config_.top_k;

    result.documents.reserve(limit);
    result.scores.reserve(limit);

    for (size_t rank = 0; rank < limit; ++rank) {
        const size_t i = indices[rank];
        judge::RetrievedDocument doc = filtered[i];
        doc.similarity_score         = fused[i];
        result.documents.push_back(std::move(doc));

        ReplugScore sc;
        sc.document_id    = filtered[i].id;
        sc.retrieval_score = ret_scores[i];
        sc.llm_score       = llm_raw[i];
        sc.fused_score     = fused[i];
        sc.kl_gradient     = kl_grads[i];
        result.scores.push_back(sc);
    }

    THEMIS_DEBUG("ReplugRetriever::fuse: candidates={} filtered={} returned={}",
                 candidates.size(), filtered.size(), result.documents.size());

    return result;
}

// ============================================================
// Weight update (REPLUG-LSR)
// ============================================================

void ReplugRetriever::updateRetrieverWeights(
    const ReplugFusionResult& result) {
    if (!config_.enable_weight_update) {
        return;
    }
    const double lr = config_.weight_update_lr;
    for (const auto& sc : result.scores) {
        double& w = weights_[sc.document_id];
        if (std::abs(w) < std::numeric_limits<double>::epsilon()) {
            w = 1.0; // Initialise to neutral weight on first encounter.
        }
        w += lr * sc.kl_gradient;
        w  = std::clamp(w, 0.0, 1.0); // Keep weight in valid range.
    }
}

void ReplugRetriever::resetWeights() {
    weights_.clear();
}

double ReplugRetriever::getWeight(const std::string& document_id) const {
    auto it = weights_.find(document_id);
    return it != weights_.end() ? it->second : 1.0;
}

// ============================================================
// Factory
// ============================================================

ReplugRetriever ReplugRetrieverFactory::createBalanced(
    std::shared_ptr<ILLMScorer> scorer,
    size_t top_k) {
    ReplugConfig cfg;
    cfg.llm_weight  = 0.5;
    cfg.top_k       = top_k;
    cfg.temperature = 1.0;
    return ReplugRetriever(cfg, std::move(scorer));
}

ReplugRetriever ReplugRetrieverFactory::createLLMDominant(
    std::shared_ptr<ILLMScorer> scorer,
    size_t top_k) {
    ReplugConfig cfg;
    cfg.llm_weight  = 0.8;
    cfg.top_k       = top_k;
    cfg.temperature = 0.5; // Sharper LLM distribution.
    return ReplugRetriever(cfg, std::move(scorer));
}

ReplugRetriever ReplugRetrieverFactory::createRetrievalDominant(
    std::shared_ptr<ILLMScorer> scorer,
    size_t top_k) {
    ReplugConfig cfg;
    cfg.llm_weight  = 0.2;
    cfg.top_k       = top_k;
    cfg.temperature = 2.0; // Flatter LLM distribution.
    return ReplugRetriever(cfg, std::move(scorer));
}

ReplugRetriever ReplugRetrieverFactory::createLSR(
    std::shared_ptr<ILLMScorer> scorer,
    size_t top_k) {
    ReplugConfig cfg;
    cfg.llm_weight          = 0.5;
    cfg.top_k               = top_k;
    cfg.enable_weight_update = true;
    cfg.weight_update_lr     = 0.01;
    return ReplugRetriever(cfg, std::move(scorer));
}

} // namespace themis::rag
