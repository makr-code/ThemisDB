/*
 * ThemisDB | File: tensor_fingerprint_graph.cpp | Version: 1.0.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 89/100 | Lines: 269
 * Open Issues: TODOs=1, Stubs=5, Gaps=8, Unimpl=0, Mock=1, Sim=1, Debt=0
 * Gap Correlation: internal=8 | external_v3=54 | delta=46 | status=divergent
 * External Severity (v3): C=12, H=40, M=2
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file tensor/tensor_fingerprint_graph.cpp
 * @brief TensorFingerprintGraph implementation.
 *
 * findSimilar() uses TT-domain cosine similarity via
 * TensorTrainDecomposer::innerProduct().  findSimilarByFingerprint()
 * retains the compact first-core fingerprint approximation.
 */

#include "tensor/tensor_fingerprint_graph.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
#include <shared_mutex>
#include <utility>

namespace themis {
namespace tensor {

// ============================================================================
// ExactSimilarity bridge (stub #276)
// ============================================================================

namespace {
    static std::mutex s_exact_sim_fn_mutex;
    static TensorFingerprintGraph::ExactSimilarityFn s_exact_sim_fn;
} // namespace

void TensorFingerprintGraph::setExactSimilarityFn(ExactSimilarityFn fn) {
    std::lock_guard<std::mutex> lock(s_exact_sim_fn_mutex);
    s_exact_sim_fn = std::move(fn);
}

void TensorFingerprintGraph::clearExactSimilarityFn() {
    std::lock_guard<std::mutex> lock(s_exact_sim_fn_mutex);
    s_exact_sim_fn = nullptr;
}

static TensorFingerprintGraph::ExactSimilarityFn getExactSimilarityFn() {
    std::lock_guard<std::mutex> lock(s_exact_sim_fn_mutex);
    return s_exact_sim_fn;
}

// ============================================================================
// Static helpers
// ============================================================================

std::vector<float>
TensorFingerprintGraph::columnMeans(const std::vector<float>& data,
                                     std::size_t n_rows,
                                     std::size_t n_cols) {
    if (data.empty() || n_rows == 0 || n_cols == 0) return {};

    std::vector<float> means(n_cols, 0.0f);
    for (std::size_t r = 0; r < n_rows; ++r) {
        for (std::size_t c = 0; c < n_cols; ++c) {
            means[c] += data[r * n_cols + c];
        }
    }
    const float inv = 1.0f / static_cast<float>(n_rows);
    for (float& v : means) v *= inv;
    return means;
}

float TensorFingerprintGraph::cosineSimilarity(const std::vector<float>& a,
                                                const std::vector<float>& b) noexcept {
    if (a.size() != b.size() || a.empty()) return 0.0f;

    float dot   = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot    += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    if (norm_a < 1e-12f || norm_b < 1e-12f) return 0.0f;
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

// ============================================================================
// addAdapter()
// ============================================================================

bool TensorFingerprintGraph::addAdapter(const std::string&        adapter_key,
                                         const storage::TTTrain&   train,
                                         const std::string&        domain,
                                         const std::string&        base_model_id,
                                         const std::string&        tenant_id) {
    if (train.cores.empty()) return false;

    const auto& G0 = train.cores[0];
    // G0 layout: r_left × n × r_right, stored row-major.
    // For the fingerprint we treat G0.data as a (r_left * n) × r_right matrix
    // and take column means along r_right.
    const std::size_t n_rows = static_cast<std::size_t>(G0.r_left) * G0.n;
    const std::size_t n_cols = G0.r_right;

    std::vector<float> fp = columnMeans(G0.data, n_rows, n_cols);
    if (fp.empty()) {
        // Fallback: use raw data directly as fingerprint.
        fp = G0.data;
    }

    // Frobenius norm of the first core data.
    float norm = 0.0f;
    for (float v : G0.data) norm += v * v;
    norm = std::sqrt(norm);

    FingerprintEntry entry;
    entry.adapter_key    = adapter_key;
    entry.domain         = domain;
    entry.base_model_id  = base_model_id;
    entry.tenant_id      = tenant_id;
    entry.fingerprint    = std::move(fp);
    entry.first_core_norm = norm;
    entry.exact_train    = train;

    {
        std::unique_lock lock(mutex_);
        entries_[adapter_key] = std::move(entry);
        trains_[adapter_key] = train;
    }
    {
        std::unique_lock slock(stats_mutex_);
        stats_.total_adapters = entries_.size();
    }
    return true;
}

// ============================================================================
// removeAdapter()
// ============================================================================

bool TensorFingerprintGraph::removeAdapter(const std::string& adapter_key) {
    std::unique_lock lock(mutex_);
    const bool removed = entries_.erase(adapter_key) > 0;
    trains_.erase(adapter_key);
    if (removed) {
        std::unique_lock slock(stats_mutex_);
        stats_.total_adapters = entries_.size();
    }
    return removed;
}

// ============================================================================
// findSimilarByFingerprint()
// ============================================================================

std::vector<SimilarityResult>
TensorFingerprintGraph::findSimilarByFingerprint(
        const std::vector<float>& fingerprint,
        std::size_t               k,
        const std::string&        tenant_id) const {

    if (fingerprint.empty() || k == 0) return {};

    std::vector<SimilarityResult> results;

    {
        std::shared_lock lock(mutex_);
        results.reserve(entries_.size());

        for (const auto& [key, ent] : entries_) {
            if (!tenant_id.empty() && ent.tenant_id != tenant_id) continue;
            if (ent.fingerprint.empty()) continue;

            // Pad shorter fingerprint with zeros if dimensions differ.
            float sim = 0.0f;
            if (ent.fingerprint.size() == fingerprint.size()) {
                sim = cosineSimilarity(fingerprint, ent.fingerprint);
            } else {
                // Align to min length — treats missing dims as zero.
                const std::size_t len = std::min(fingerprint.size(),
                                                  ent.fingerprint.size());
                std::vector<float> qa(fingerprint.begin(),
                                       fingerprint.begin() + len);
                std::vector<float> qb(ent.fingerprint.begin(),
                                       ent.fingerprint.begin() + len);
                sim = cosineSimilarity(qa, qb);
            }

            SimilarityResult sr;
            sr.adapter_key    = ent.adapter_key;
            sr.domain         = ent.domain;
            sr.base_model_id  = ent.base_model_id;
            sr.score          = sim;
            results.push_back(std::move(sr));
        }
    }

    // Sort descending by score.
    std::sort(results.begin(), results.end(),
              [](const SimilarityResult& a, const SimilarityResult& b) {
                  return a.score > b.score;
              });
    if (results.size() > k) results.resize(k);

    {
        std::unique_lock slock(stats_mutex_);
        ++stats_.total_query_calls;
        stats_.total_comparisons += entries_.size();
    }

    return results;
}

// ============================================================================
// findSimilar()
// ============================================================================

std::vector<SimilarityResult>
TensorFingerprintGraph::findSimilar(const std::string& query_key,
                                     std::size_t        k) const {
    if (k == 0) return {};

    storage::TTTrain query_train;
    std::vector<std::pair<std::string, FingerprintEntry>> candidates;
    {
        std::shared_lock lock(mutex_);
        const auto it_train = trains_.find(query_key);
        if (it_train == trains_.end()) return {};
        query_train = it_train->second;
        candidates.reserve(entries_.size() > 0 ? entries_.size() - 1 : 0);
        for (const auto& [key, entry] : entries_) {
            if (key == query_key) continue;
            candidates.emplace_back(key, entry);
        }
    }

    const double query_self_ip =
        storage::TensorTrainDecomposer::innerProduct(query_train, query_train);
    if (!std::isfinite(query_self_ip) || query_self_ip <= 0.0) return {};

    std::vector<SimilarityResult> results;
    results.reserve(candidates.size());
    std::size_t comparisons = 0;
    {
        std::shared_lock lock(mutex_);
        for (const auto& [key, entry] : candidates) {
            const auto it_train = trains_.find(key);
            if (it_train == trains_.end()) continue;
            const auto& other_train = it_train->second;

            const double other_self_ip =
                storage::TensorTrainDecomposer::innerProduct(other_train, other_train);
            if (!std::isfinite(other_self_ip) || other_self_ip <= 0.0) {
                continue;
            }

            const double cross_ip =
                storage::TensorTrainDecomposer::innerProduct(query_train, other_train);
            const double denom = std::sqrt(query_self_ip * other_self_ip);
            if (!std::isfinite(cross_ip) || !std::isfinite(denom) || denom <= 0.0) {
                continue;
            }

            double score = cross_ip / denom;
            score = std::clamp(score, -1.0, 1.0);

            SimilarityResult sr;
            sr.adapter_key = entry.adapter_key;
            sr.domain = entry.domain;
            sr.base_model_id = entry.base_model_id;
            sr.score = static_cast<float>(score);
            results.push_back(std::move(sr));
            ++comparisons;
        }
    }

    std::sort(results.begin(), results.end(),
              [](const SimilarityResult& a, const SimilarityResult& b) {
                  return a.score > b.score;
              });
    if (results.size() > k) results.resize(k);

    {
        std::unique_lock slock(stats_mutex_);
        ++stats_.total_query_calls;
        stats_.total_comparisons += comparisons;
    }

    return results;
}

// ============================================================================
// entry() / size() / adapterKeys() / stats()
// ============================================================================

std::optional<FingerprintEntry>
TensorFingerprintGraph::entry(const std::string& adapter_key) const {
    std::shared_lock lock(mutex_);
    auto it = entries_.find(adapter_key);
    if (it == entries_.end()) return std::nullopt;
    return it->second;
}

std::size_t TensorFingerprintGraph::size() const noexcept {
    std::shared_lock lock(mutex_);
    return entries_.size();
}

std::vector<std::string> TensorFingerprintGraph::adapterKeys() const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(entries_.size());
    for (const auto& [k, _] : entries_) keys.push_back(k);
    std::sort(keys.begin(), keys.end());
    return keys;
}

TensorFingerprintGraph::GraphStats
TensorFingerprintGraph::stats() const noexcept {
    std::shared_lock lock(stats_mutex_);
    return stats_;
}

void TensorFingerprintGraph::setExactSimilarityFn(ExactSimilarityFn fn) {
    std::unique_lock lock(exact_similarity_fn_mutex_);
    exact_similarity_fn_ = std::move(fn);
}

void TensorFingerprintGraph::clearExactSimilarityFn() {
    std::unique_lock lock(exact_similarity_fn_mutex_);
    exact_similarity_fn_ = nullptr;
}

} // namespace tensor
} // namespace themis
