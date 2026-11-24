/**
 * @file tensor_fingerprint_graph.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/tensor_fingerprint_graph.h"
#include "tensor/tensor_error_handling.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <mutex>
#include <numeric>
#include <shared_mutex>
#include <stdexcept>
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

/**
 * @brief Set Exact Similarity Fn.
 * @param[in] fn Input parameter.
 * @details Calls: lock(), std::move().
 */
void TensorFingerprintGraph::setExactSimilarityFn(ExactSimilarityFn fn) {
    std::lock_guard<std::mutex> lock(s_exact_sim_fn_mutex);
    s_exact_sim_fn = std::move(fn);
}

/**
 * @brief Clear Exact Similarity Fn.
 * @details Calls: lock().
 */
void TensorFingerprintGraph::clearExactSimilarityFn() {
    std::lock_guard<std::mutex> lock(s_exact_sim_fn_mutex);
    s_exact_sim_fn = nullptr;
}

/**
 * @brief Get Exact Similarity Fn.
 * @return Return value.
 * @details Calls: lock().
 */
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
    for (float& v : means) {
      v *= inv;
    }
    return means;
}

float TensorFingerprintGraph::cosineSimilarity(const std::vector<float>& a,
                                                const std::vector<float>& b) noexcept {
    if (a.size() != b.size() || a.empty()) {
      return 0.0f;
    }

    float dot   = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    for (std::size_t i = 0; i  < a.size(); ++i) {
        dot    += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    if (norm_a < 1e-12f || norm_b < 1e-12f) {
      return 0.0f;
    }
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

float TensorFingerprintGraph::cosineSimilarityZeroPadded(
        const std::vector<float>& a,
        float                     a_sq_norm,
        const std::vector<float>& b,
        float                     b_sq_norm) noexcept {
    if (a.empty() || b.empty()) {
      return 0.0f;
    }
    if (a_sq_norm < 1e-12f || b_sq_norm < 1e-12f) {
      return 0.0f;
    }

    const std::size_t overlap = std::min(a.size(), b.size());
    float dot = 0.0f;
    for (std::size_t i = 0; i < overlap; ++i) {
        dot += a[i] * b[i];
    }

    return dot / (std::sqrt(a_sq_norm) * std::sqrt(b_sq_norm));
}

// ============================================================================
// addAdapter()
// ============================================================================

bool TensorFingerprintGraph::isTrainValid(const storage::TTTrain& train) noexcept {
    if (train.cores.empty() || train.mode_sizes.size() != train.cores.size()) {
      return false;
    }

    std::size_t previous_rank = 1;
    for (std::size_t i = 0; i < train.cores.size(); ++i) {
        const auto& core = train.cores[i];
        const auto max_size = std::numeric_limits<std::size_t>::max();
        if (core.n == 0 || core.r_left == 0 || core.r_right == 0 ||
            core.r_left != previous_rank || core.n != train.mode_sizes[i] ||
            core.n > max_size / core.r_left ||
            core.r_right > max_size / (core.r_left * core.n) ||
            core.data.size() != core.numElements() ||
            !std::all_of(core.data.begin(), core.data.end(),
                         [](float value) { return std::isfinite(value); })) {
            return false;
        }
        previous_rank = core.r_right;
    }
    return previous_rank == 1;
}

bool TensorFingerprintGraph::replaceTenantDomainEntries(
    const std::string& tenant_id, const std::string& domain,
    const TensorFingerprintGraph& validated) {
    if (this == &validated || tenant_id.empty() || domain.empty()) {
        return false;
    }

    std::shared_lock validated_lock(validated.mutex_);
    for (const auto& [key, entry] : validated.entries_) {
        if (entry.tenant_id != tenant_id || entry.domain != domain ||
            entry.adapter_key != key || !isTrainValid(entry.exact_train) ||
            !validated.trains_.contains(key) ||
            !validated.train_self_ip_.contains(key)) {
            return false;
        }
    }

    std::unique_lock lock(mutex_);
    auto next_entries = entries_;
    auto next_trains = trains_;
    auto next_self_ips = train_self_ip_;
    for (auto it = next_entries.begin(); it != next_entries.end();) {
        if (it->second.tenant_id == tenant_id && it->second.domain == domain) {
            next_trains.erase(it->first);
            next_self_ips.erase(it->first);
            it = next_entries.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& [key, entry] : validated.entries_) {
        const auto existing = next_entries.find(key);
        if (existing != next_entries.end()) {
            return false;
        }
        next_entries.emplace(key, entry);
        next_trains.emplace(key, validated.trains_.at(key));
        next_self_ips.emplace(key, validated.train_self_ip_.at(key));
    }

    entries_.swap(next_entries);
    trains_.swap(next_trains);
    train_self_ip_.swap(next_self_ips);
    {
        std::unique_lock stats_lock(stats_mutex_);
        stats_.total_adapters = entries_.size();
    }
    return true;
}

bool TensorFingerprintGraph::addAdapter(const std::string&        adapter_key,
                                         const storage::TTTrain&   train,
                                         const std::string&        domain,
                                         const std::string&        base_model_id,
                                         const std::string&        tenant_id) {
    if (!isTrainValid(train)) {
        return false;
    }

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

    float fp_sq_norm = 0.0f;
    for (float v : fp) {
      fp_sq_norm += v * v;
    }

    // Frobenius norm of the first core data.
    float norm = 0.0f;
    for (float v : G0.data) {
      norm += v * v;
    }
    norm = std::sqrt(norm);

    FingerprintEntry entry;
    entry.adapter_key    = adapter_key;
    entry.domain         = domain;
    entry.base_model_id  = base_model_id;
    entry.tenant_id      = tenant_id;
    entry.fingerprint    = std::move(fp);
    entry.fingerprint_sq_norm = fp_sq_norm;
    entry.first_core_norm = norm;
    entry.exact_train    = train;

    const double train_self_ip =
        storage::TensorTrainDecomposer::innerProduct(train, train);

    {
        std::unique_lock lock(mutex_);
        entries_[adapter_key] = std::move(entry);
        trains_[adapter_key] = train;
        train_self_ip_[adapter_key] = train_self_ip;
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

/**
 * @brief Remove Adapter.
 * @param[in] adapter_key Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), erase(), slock(), size().
 */
bool TensorFingerprintGraph::removeAdapter(const std::string& adapter_key) {
    std::unique_lock lock(mutex_);
    const bool removed = entries_.erase(adapter_key) > 0;
    trains_.erase(adapter_key);
    train_self_ip_.erase(adapter_key);
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
    float query_sq_norm = 0.0f;
    for (float v : fingerprint) {
      query_sq_norm += v * v;
    }
    std::size_t comparisons = 0;

    {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(mutex_);
        results.reserve(entries_.size());

        for (const auto& [key, ent] : entries_) {
            if (!tenant_id.empty() && ent.tenant_id != tenant_id) {
              continue;
            }
            if (ent.fingerprint.empty()) {
              continue;
            }

            const float sim = cosineSimilarityZeroPadded(
                fingerprint,
                query_sq_norm,
                ent.fingerprint,
                ent.fingerprint_sq_norm);
            ++comparisons;

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
    if (results.size() > k) {
      results.resize(k);
    }

    {
        /**
         * @brief Slock.
         * @param[in] stats_mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock slock(stats_mutex_);
        ++stats_.total_query_calls;
        stats_.total_comparisons += comparisons;
    }

    return results;
}

// ============================================================================
// findSimilar()
// ============================================================================

std::vector<SimilarityResult>
TensorFingerprintGraph::findSimilar(const std::string& query_key,
                                     std::size_t        k) const {
    return findSimilar(query_key, k, query::RequestContext{}, nullptr);
}

std::vector<SimilarityResult>
TensorFingerprintGraph::findSimilarForTenant(
        const std::string& query_key, const std::string& tenant_id, std::size_t k) const {
    if (tenant_id.empty()) {
        throw std::invalid_argument("tensor fingerprint tenant is empty");
    }
    return findSimilarImpl(query_key, k, query::RequestContext{}, nullptr, &tenant_id);
}

std::optional<TenantSimilaritySnapshot>
TensorFingerprintGraph::snapshotSimilarForTenant(
        const std::string& query_key, const std::string& tenant_id, std::size_t k) const {
    if (tenant_id.empty() || k == 0) {
        throw std::invalid_argument("tensor fingerprint snapshot requires tenant and positive k");
    }
    std::optional<TenantSimilaritySnapshot> snapshot;
    auto results = findSimilarImpl(query_key, k, query::RequestContext{}, nullptr,
                                   &tenant_id, &snapshot);
    if (snapshot) {
        snapshot->candidates = std::move(results);
    }
    return snapshot;
}

std::vector<SimilarityResult>
TensorFingerprintGraph::findSimilar(
        const std::string& query_key,
        std::size_t k,
        const query::RequestContext& request,
        query::StopReason* stop_reason) const {
    return findSimilarImpl(query_key, k, request, stop_reason, nullptr);
}

std::vector<SimilarityResult>
TensorFingerprintGraph::findSimilarImpl(
        const std::string& query_key,
        std::size_t k,
        const query::RequestContext& request,
        query::StopReason* stop_reason,
        const std::string* expected_tenant,
        std::optional<TenantSimilaritySnapshot>* snapshot) const {
    if (stop_reason) {
        *stop_reason = query::StopReason::None;
    }
    const auto stopIfRequested = [&]() {
        const auto reason = request.stopReason();
        if (reason == query::StopReason::None) {
            return false;
        }
        if (stop_reason) {
            *stop_reason = reason;
        }
        return true;
    };

    if (stopIfRequested() || k == 0) return {};

    storage::TTTrain query_train;
    std::vector<std::pair<std::string, FingerprintEntry>> candidates;
    double query_self_ip = 0.0;
    {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(mutex_);
        const auto it_train = trains_.find(query_key);
        const auto it_query = entries_.find(query_key);
        if (it_train == trains_.end() || it_query == entries_.end()) return {};
        if (expected_tenant && it_query->second.tenant_id != *expected_tenant) {
            throw std::invalid_argument("tensor fingerprint query tenant mismatch");
        }
        if (snapshot) {
            snapshot->emplace(TenantSimilaritySnapshot{
                it_query->second.domain, it_query->second.base_model_id, {}});
        }
        query_train = it_train->second;

        if (stopIfRequested()) return {};
        const auto it_query_self = train_self_ip_.find(query_key);
        if (it_query_self != train_self_ip_.end()) {
            query_self_ip = it_query_self->second;
        } else {
            query_self_ip =
                storage::TensorTrainDecomposer::innerProduct(query_train, query_train);
        }
        if (stopIfRequested()) return {};

        candidates.reserve(entries_.size() > 0 ? entries_.size() - 1 : 0);
        for (const auto& [key, entry] : entries_) {
            if (stopIfRequested()) return {};
            if (key == query_key || entry.tenant_id != it_query->second.tenant_id) {
              continue;
            }
            candidates.emplace_back(key, entry);
            if (stopIfRequested()) return {};
        }
    }
    if (stopIfRequested()) return {};
    if (!std::isfinite(query_self_ip) || query_self_ip <= 0.0) {
        // CRITICAL-1: Emit diagnostic before silent return
        emitFingerprintDiagnostic(
            "TENSOR-9510",
            "Invalid query self inner product (NaN/Inf/≤0)",
            query_key);
        return {};
    }

    const auto exact_similarity_fn = getExactSimilarityFn();

    std::vector<SimilarityResult> results = {};

    results.reserve(candidates.size());
    std::size_t comparisons = 0;
    if (exact_similarity_fn) {
        for (const auto& [key, entry] : candidates) {
            if (stopIfRequested()) return {};
            double score = 0.0;
            try {
                score = static_cast<double>(exact_similarity_fn(query_key, key));
            } catch (...) {
                // CRITICAL-1: Emit diagnostic for exception in similarity computation
                emitFingerprintDiagnostic(
                    "TENSOR-9511",
                    "Exception in exact similarity computation",
                    key);
                continue;
            }
            if (stopIfRequested()) return {};
            if (!std::isfinite(score)) {
                // CRITICAL-1: Emit diagnostic for invalid score
                emitFingerprintDiagnostic(
                    "TENSOR-9512",
                    "Computed similarity score is NaN/Inf",
                    key);
                continue;
            }
            score = std::clamp(score, -1.0, 1.0);

            SimilarityResult sr;
            sr.adapter_key = entry.adapter_key;
            sr.domain = entry.domain;
            sr.base_model_id = entry.base_model_id;
            sr.score = static_cast<float>(score);
            results.push_back(std::move(sr));
            ++comparisons;
        }
    } else {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(mutex_);
        for (const auto& [key, entry] : candidates) {
            if (stopIfRequested()) return {};
            const auto current_entry = entries_.find(key);
            if (current_entry == entries_.end() ||
                current_entry->second.tenant_id != entry.tenant_id ||
                current_entry->second.adapter_key != entry.adapter_key ||
                current_entry->second.domain != entry.domain ||
                current_entry->second.base_model_id != entry.base_model_id) {
                continue;
            }
            const auto it_train = trains_.find(key);
            if (it_train == trains_.end()) {
                // CRITICAL-1: Emit diagnostic when referenced train not found
                emitFingerprintDiagnostic(
                    "TENSOR-9513",
                    "Referenced tensor train entry not found",
                    key);
                continue;
            }
            const auto& other_train = it_train->second;

            double other_self_ip = 0.0;
            const auto it_other_self = train_self_ip_.find(key);
            if (it_other_self != train_self_ip_.end()) {
                other_self_ip = it_other_self->second;
            } else {
                other_self_ip =
                    storage::TensorTrainDecomposer::innerProduct(other_train, other_train);
            }
            if (stopIfRequested()) return {};

            if (!std::isfinite(other_self_ip) || other_self_ip <= 0.0) {
                // CRITICAL-1: Emit diagnostic for invalid other self inner product
                emitFingerprintDiagnostic(
                    "TENSOR-9510",
                    "Invalid other self inner product (NaN/Inf/≤0)",
                    key);
                continue;
            }

            const double cross_ip =
                storage::TensorTrainDecomposer::innerProduct(query_train, other_train);
            if (stopIfRequested()) return {};
            const double denom = std::sqrt(query_self_ip * other_self_ip);
            if (!std::isfinite(cross_ip) || !std::isfinite(denom) || denom <= 0.0) {
                // CRITICAL-1: Emit diagnostic for invalid cross product or denominator
                emitFingerprintDiagnostic(
                    "TENSOR-9514",
                    "Cross inner-product computation failed (NaN/Inf/denom≤0)",
                    key);
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

    if (stopIfRequested()) return {};
    std::sort(results.begin(), results.end(),
              [](const SimilarityResult& a, const SimilarityResult& b) {
                  return a.score > b.score;
              });
    if (stopIfRequested()) return {};
    if (results.size() > k) {
      results.resize(k);
    }
    if (stopIfRequested()) return {};

    {
        /**
         * @brief Slock.
         * @param[in] stats_mutex_ Input parameter.
         * @return Return value.
         */
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    auto it = entries_.find(adapter_key);
    if (it == entries_.end()) {
      return std::nullopt;
    }
    return it->second;
}

std::size_t TensorFingerprintGraph::size() const noexcept {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    return entries_.size();
}

std::vector<std::string> TensorFingerprintGraph::adapterKeys() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    std::vector<std::string> keys = {};

    keys.reserve(entries_.size());
    for (const auto& [k, _] : entries_) {
      keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

TensorFingerprintGraph::GraphStats
TensorFingerprintGraph::stats() const noexcept {
    /**
     * @brief Lock.
     * @param[in] stats_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(stats_mutex_);
    return stats_;
}

} // namespace tensor
} // namespace themis
