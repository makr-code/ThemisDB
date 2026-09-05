/**
 * @file retrieval_metrics.cc
 * @brief Implementation of the layered retrieval evaluation metrics (EPIC 2 Phase 2).
 *
 * Implements all computation functions declared in `retrieval_metrics.h`:
 *
 * - Retrieval quality: Recall\@k, Precision\@k, NDCG\@k, MRR, candidate
 *   reduction ratio.
 * - Evidence quality: coverage rate, precision, multi-hop support score.
 * - Provenance quality: fidelity, source attribution completeness, trust
 *   signal correctness.
 * - Compression / tensor: compression ratio, approximation loss, redundancy
 *   elimination, residual error, rank growth rate.
 * - LLM answer quality: faithfulness, hallucination rate, groundedness,
 *   answer support density.
 * - Distributed efficiency: shard fan-out, bytes per query, summary-first
 *   selectivity, selective exact load.
 * - Tensor-graph runtime: artifact freshness, delta lag, residual error, rank
 *   growth, rebuild frequency, exact fallback frequency, summary-first false-
 *   negative rate, graph-verified finalization pass rate.
 *
 * ## Error contract
 *
 * Every compute function validates its preconditions and throws @ref MetricError
 * on violation. Silent numeric failures (NaN propagation, division by zero) are
 * prevented by explicit guards throughout.
 *
 * @see include/retrieval_metrics.h
 * @see EVALUATION_FRAMEWORK.md
 */

#include "../include/retrieval_metrics.h"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace evaluation {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// @brief Guard helper that always throws a MetricError.
[[noreturn]] void throwMetric(MetricErrorKind kind, std::string_view msg) {
    throw MetricError(kind, msg);
}

/// @brief Validate that a double value is finite; throw on NaN or ±Inf.
void requireFinite(double v, std::string_view name) {
    if (!std::isfinite(v)) {
        throwMetric(MetricErrorKind::NonFiniteInput,
                    std::string("Non-finite value for ") + std::string(name));
    }
}

/// @brief Validate a probability in [0, 1].
void requireProbability(double v, std::string_view name) {
    requireFinite(v, name);
    if (v < 0.0 || v > 1.0) {
        throwMetric(MetricErrorKind::InvalidRange,
                    std::string(name) + " must be in [0, 1]");
    }
}

/// @brief Build a set from a string vector; throw on duplicates.
[[nodiscard]] std::unordered_set<std::string> toSetChecked(
    const std::vector<std::string>& v, std::string_view ctx)
{
    std::unordered_set<std::string> s = {};

    s.reserve(v.size());
    for (const auto& id : v) {
        if (!s.insert(id).second) {
            throwMetric(MetricErrorKind::DuplicateEntries,
                        std::string("Duplicate id in ") + std::string(ctx) + ": " + id);
        }
    }
    return s;
}

/// @brief Compute DCG for a relevance gain vector up to depth k.
[[nodiscard]] double dcg(const std::vector<double>& gains, std::size_t k) {
    double result = 0.0;
    for (std::size_t i = 0; i < k && i < gains.size(); ++i) {
        result += gains[i] / std::log2(static_cast<double>(i) + 2.0);
    }
    return result;
}

} // namespace

// ============================================================================
// § 1  Retrieval quality
// ============================================================================

RetrievalQualityMetrics computeRetrievalQuality(
    const std::vector<RankedResult>& ranked,
    const std::vector<std::string>&  ground_truth,
    std::size_t                      k,
    std::size_t                      total_candidates)
{
    if (ground_truth.empty()) {
        throwMetric(MetricErrorKind::EmptyGroundTruth,
                    "ground_truth must not be empty");
    }
    if (k == 0 || k > ranked.size()) {
        throwMetric(MetricErrorKind::InvalidK,
                    "k must be in [1, ranked.size()]; k=" + std::to_string(k) +
                    " ranked.size()=" + std::to_string(ranked.size()));
    }

    // Check for duplicate ids in ranked list.
    {
        std::unordered_set<std::string> seen = {};

        seen.reserve(ranked.size());
        for (const auto& r : ranked) {
            if (!seen.insert(r.id).second) {
                throwMetric(MetricErrorKind::DuplicateEntries,
                            "Duplicate id in ranked list: " + r.id);
            }
        }
    }

    const auto gtSet = toSetChecked(ground_truth, "ground_truth");
    const std::size_t gt_size = gtSet.size();

    // --- Recall@k and Precision@k ---
    std::size_t hits = 0;
    for (std::size_t i = 0; i < k; ++i) {
        if (gtSet.count(ranked[i].id)) {
          ++hits;
        }
    }
    const double recall_at_k    = static_cast<double>(hits) / static_cast<double>(gt_size);
    const double precision_at_k = static_cast<double>(hits) / static_cast<double>(k);

    // --- NDCG@k ---
    // Binary relevance: gain = 1.0 if in ground truth, 0.0 otherwise.
    std::vector<double> gains(k);
    for (std::size_t i = 0; i < k; ++i) {
        gains[i] = gtSet.count(ranked[i].id) ? 1.0 : 0.0;
    }
    const double actual_dcg = dcg(gains, k);

    // Ideal DCG: place all relevant items first.
    const std::size_t ideal_hits = std::min(k, gt_size);
    std::vector<double> ideal_gains(k, 0.0);
    for (std::size_t i = 0; i < ideal_hits; ++i) {
      ideal_gains[i] = 1.0;
    }
    const double ideal_dcg_val = dcg(ideal_gains, k);

    const double ndcg = (ideal_dcg_val > 0.0) ? (actual_dcg / ideal_dcg_val) : 0.0;

    // --- MRR ---
    double mrr = 0.0;
    for (std::size_t i = 0; i < ranked.size(); ++i) {
        if (gtSet.count(ranked[i].id)) {
            mrr = 1.0 / static_cast<double>(i + 1);
            break;
        }
    }

    // --- Candidate reduction ratio ---
    double reduction = 0.0;
    if (total_candidates > 0 && total_candidates >= k) {
        reduction = 1.0 - static_cast<double>(k) / static_cast<double>(total_candidates);
    }

    RetrievalQualityMetrics result;
    result.recall_at_k             = recall_at_k;
    result.precision_at_k          = precision_at_k;
    result.ndcg_at_k               = ndcg;
    result.mrr                     = mrr;
    result.candidate_reduction_ratio = reduction;
    return result;
}

// ============================================================================
// § 2  Evidence quality
// ============================================================================

EvidenceQualityMetrics computeEvidenceQuality(
    const std::vector<std::string>& returned_evidence_ids,
    const std::vector<std::string>& required_evidence_ids,
    const std::vector<int>&         hop_chain_lengths)
{
    if (required_evidence_ids.empty()) {
        throwMetric(MetricErrorKind::EmptyGroundTruth,
                    "required_evidence_ids must not be empty");
    }

    const auto required_set = toSetChecked(required_evidence_ids, "required_evidence_ids");
    const auto returned_set = toSetChecked(returned_evidence_ids, "returned_evidence_ids");

    // Coverage: fraction of required items that were returned.
    std::size_t covered = 0;
    for (const auto& id : required_set) {
        if (returned_set.count(id)) {
          ++covered;
        }
    }
    const double coverage = static_cast<double>(covered) /
                            static_cast<double>(required_set.size());

    // Precision: fraction of returned items that are relevant.
    double precision = 0.0;
    if (!returned_set.empty()) {
        std::size_t relevant = 0;
        for (const auto& id : returned_set) {
            if (required_set.count(id)) {
              ++relevant;
            }
        }
        precision = static_cast<double>(relevant) / static_cast<double>(returned_set.size());
    }

    // Multi-hop support score: mean normalized hop chain length in [0, 1].
    // A single-hop chain has length 1 → score contribution 1/max_hops.
    // We cap at max_hops = 5 per the retrieval spec.
    double multi_hop_support = 0.0;
    if (!hop_chain_lengths.empty()) {
        constexpr int kMaxHops = 5;
        double sum = 0.0;
        for (int h : hop_chain_lengths) {
            if (h < 0) {
                throwMetric(MetricErrorKind::InvalidRange,
                            "hop_chain_length must be >= 0");
            }
            sum += std::min(h, kMaxHops) / static_cast<double>(kMaxHops);
        }
        multi_hop_support = sum / static_cast<double>(hop_chain_lengths.size());
    }

    EvidenceQualityMetrics result;
    result.coverage_rate      = coverage;
    result.evidence_precision = precision;
    result.multi_hop_support  = multi_hop_support;
    return result;
}

// ============================================================================
// § 3  Provenance quality
// ============================================================================

ProvenanceQualityMetrics computeProvenanceQuality(
    const std::vector<ProvenanceAssertion>& returned,
    const std::vector<ProvenanceAssertion>& ground_truth)
{
    if (ground_truth.empty()) {
        throwMetric(MetricErrorKind::MissingGroundTruthLabels,
                    "ground_truth provenance assertions must not be empty");
    }

    // Validate confidence values.
    for (const auto& a : returned) {
        requireProbability(a.confidence, "returned provenance confidence");
    }
    for (const auto& a : ground_truth) {
        requireProbability(a.confidence, "ground_truth provenance confidence");
    }

    // Build a lookup: (claim_id, source_id) → true for ground truth.
    struct PairHash {
        std::size_t operator()(const std::pair<std::string, std::string>& p) const noexcept {
            std::size_t h = std::hash<std::string>{}(p.first);
            h ^= std::hash<std::string>{}(p.second) + 0x9e3779b9ULL + (h << 6) + (h >> 2);
            return h;
        }
    };
    std::unordered_set<std::pair<std::string, std::string>, PairHash> gt_pairs;
    for (const auto& a : ground_truth) {
        gt_pairs.emplace(a.claim_id, a.source_id);
    }

    // Fidelity: fraction of returned assertions that match ground truth.
    std::size_t matched = 0;
    for (const auto& a : returned) {
        if (gt_pairs.count({a.claim_id, a.source_id})) ++matched;
    }
    const double fidelity = returned.empty()
        ? 0.0
        : static_cast<double>(matched) / static_cast<double>(returned.size());

    // Source attribution completeness: fraction of GT claims covered.
    std::unordered_set<std::string> gt_claims = {};

    for (const auto& a : ground_truth) {
      gt_claims.insert(a.claim_id);
    }

    std::unordered_set<std::string> covered_claims = {};

    for (const auto& a : returned) {
        if (gt_claims.count(a.claim_id)) {
          covered_claims.insert(a.claim_id);
        }
    }
    const double completeness = static_cast<double>(covered_claims.size()) /
                                static_cast<double>(gt_claims.size());

    // Trust signal correctness: fraction of assertions with confidence
    // within ±0.10 of the ground-truth for the same (claim, source) pair.
    std::unordered_map<std::pair<std::string, std::string>, double, PairHash> gt_confidence;
    for (const auto& a : ground_truth) {
        gt_confidence[{a.claim_id, a.source_id}] = a.confidence;
    }

    std::size_t trust_correct = 0;
    std::size_t trust_total   = 0;
    for (const auto& a : returned) {
        auto it = gt_confidence.find({a.claim_id, a.source_id});
        if (it != gt_confidence.end()) {
            ++trust_total;
            if (std::abs(a.confidence - it->second) <= 0.10) {
              ++trust_correct;
            }
        }
    }
    const double trust_correctness = (trust_total > 0)
        ? static_cast<double>(trust_correct) / static_cast<double>(trust_total)
        : 0.0;

    ProvenanceQualityMetrics result;
    result.fidelity_score                = fidelity;
    result.source_attribution_completeness = completeness;
    result.trust_signal_correctness      = trust_correctness;
    return result;
}

// ============================================================================
// § 4  Compression / tensor metrics
// ============================================================================

CompressionMetrics computeCompressionMetrics(
    std::size_t                original_size_bytes,
    std::size_t                compressed_size_bytes,
    const std::vector<double>& approximation_errors,
    const std::vector<int>&    rank_samples)
{
    if (original_size_bytes == 0) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "original_size_bytes must be > 0");
    }
    if (compressed_size_bytes == 0 || compressed_size_bytes > original_size_bytes) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "compressed_size_bytes must be in (0, original_size_bytes]");
    }
    if (approximation_errors.empty()) {
        throwMetric(MetricErrorKind::EmptyGroundTruth,
                    "approximation_errors must not be empty");
    }
    for (auto e : approximation_errors) {
        requireFinite(e, "approximation_error");
        if (e < 0.0) {
            throwMetric(MetricErrorKind::InvalidRange,
                        "approximation_error must be >= 0");
        }
    }

    const double ratio = static_cast<double>(original_size_bytes) /
                         static_cast<double>(compressed_size_bytes);

    const double mean_error = std::accumulate(
        approximation_errors.begin(), approximation_errors.end(), 0.0) /
        static_cast<double>(approximation_errors.size());

    // Redundancy elimination: estimated from compression ratio.
    const double redundancy = 1.0 - (1.0 / ratio);

    // Residual error: max of the approximation error vector (worst-case).
    const double residual = *std::max_element(
        approximation_errors.begin(), approximation_errors.end());

    // Rank growth rate: linear regression slope of rank over epoch index.
    double rank_growth = 0.0;
    if (rank_samples.size() >= 2) {
        const std::size_t n = rank_samples.size();
        double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            const double x = static_cast<double>(i);
            const double y = static_cast<double>(rank_samples[i]);
            sum_x  += x;
            sum_y  += y;
            sum_xy += x * y;
            sum_xx += x * x;
        }
        const double denom = static_cast<double>(n) * sum_xx - sum_x * sum_x;
        if (std::abs(denom) > 1e-12) {
            rank_growth = (static_cast<double>(n) * sum_xy - sum_x * sum_y) / denom;
        }
    }

    CompressionMetrics result;
    result.compression_ratio      = ratio;
    result.approximation_loss     = mean_error;
    result.redundancy_elimination = redundancy;
    result.residual_error         = residual;
    result.rank_growth_rate       = rank_growth;
    return result;
}

// ============================================================================
// § 5  LLM answer quality
// ============================================================================

LlmAnswerQualityMetrics computeLlmAnswerQuality(
    uint32_t supported_claims,
    uint32_t total_claims,
    uint32_t evidence_tokens,
    uint32_t prompt_token_count)
{
    if (total_claims == 0) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "total_claims must be > 0");
    }
    if (supported_claims > total_claims) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "supported_claims must be <= total_claims");
    }

    const double faithfulness   = static_cast<double>(supported_claims) /
                                  static_cast<double>(total_claims);
    const double hallucination  = 1.0 - faithfulness;
    // Groundedness: geometric mean of faithfulness and coverage signal.
    // When evidence_tokens == 0 we use faithfulness directly.
    double groundedness = faithfulness;
    double support_density = 0.0;
    if (evidence_tokens > 0 && prompt_token_count > 0 && prompt_token_count >= evidence_tokens) {
        support_density = static_cast<double>(evidence_tokens) /
                          static_cast<double>(prompt_token_count);
        groundedness = std::sqrt(faithfulness * support_density);
    }

    LlmAnswerQualityMetrics result;
    result.faithfulness_score     = faithfulness;
    result.hallucination_rate     = hallucination;
    result.groundedness_score     = groundedness;
    result.answer_support_density = support_density;
    result.prompt_token_count     = prompt_token_count;
    return result;
}

// ============================================================================
// § 6  Distributed efficiency
// ============================================================================

DistributedEfficiencyMetrics computeDistributedEfficiency(
    const std::vector<uint32_t>& per_query_shard_counts,
    const std::vector<double>&   per_query_bytes,
    const std::vector<uint32_t>& summary_skipped_shards,
    uint32_t                     total_shards)
{
    if (per_query_shard_counts.empty()) {
        throwMetric(MetricErrorKind::EmptyGroundTruth,
                    "per_query_shard_counts must not be empty");
    }
    if (per_query_shard_counts.size() != per_query_bytes.size()) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "per_query_shard_counts and per_query_bytes must have equal length");
    }
    if (per_query_shard_counts.size() != summary_skipped_shards.size()) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "per_query_shard_counts and summary_skipped_shards must have equal length");
    }
    if (total_shards == 0) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "total_shards must be > 0");
    }

    const auto n = static_cast<double>(per_query_shard_counts.size());

    double sum_shards  = 0.0;
    double sum_bytes   = 0.0;
    double sum_skipped = 0.0;
    for (std::size_t i = 0; i < per_query_shard_counts.size(); ++i) {
        requireFinite(per_query_bytes[i], "per_query_bytes");
        sum_shards  += per_query_shard_counts[i];
        sum_bytes   += per_query_bytes[i];
        sum_skipped += summary_skipped_shards[i];
    }

    const double mean_shards  = sum_shards / n;
    const double mean_bytes   = sum_bytes  / n;
    const double mean_skipped = sum_skipped / n;

    // Summary-first selectivity: fraction of shards skipped on average.
    const double selectivity = mean_skipped / static_cast<double>(total_shards);

    // Selective exact load: fraction of queries that contacted fewer shards
    // than total (i.e., did NOT fan out to all shards).
    std::size_t selective_count = 0;
    for (auto c : per_query_shard_counts) {
        if (c < total_shards) {
          ++selective_count;
        }
    }
    const double selective_load = static_cast<double>(selective_count) / n;

    DistributedEfficiencyMetrics result;
    result.shard_fan_out             = mean_shards;
    result.bytes_per_query           = mean_bytes;
    result.summary_first_selectivity = std::min(selectivity, 1.0);
    result.selective_exact_load      = selective_load;
    result.total_queries             = static_cast<uint32_t>(per_query_shard_counts.size());
    return result;
}

// ============================================================================
// § 7  Tensor-graph runtime metrics
// ============================================================================

TensorGraphRuntimeMetrics computeTensorGraphRuntimeMetrics(
    const std::vector<TensorGraphSnapshot>& snapshots,
    double                                  max_residual_error)
{
    if (snapshots.empty()) {
        throwMetric(MetricErrorKind::EmptyGroundTruth,
                    "snapshots must not be empty");
    }

    requireFinite(max_residual_error, "max_residual_error");
    if (max_residual_error < 0.0) {
        throwMetric(MetricErrorKind::InvalidRange,
                    "max_residual_error must be >= 0");
    }

    const auto n = static_cast<double>(snapshots.size());

    double sum_age           = 0.0;
    double sum_delta         = 0.0;
    double sum_residual      = 0.0;
    double rank_growth_count = 0.0;
    double rebuild_count     = 0.0;
    double fallback_count    = 0.0;

    double summary_first_total   = 0.0;
    double summary_fn_unrecovered = 0.0;

    double graph_eligible = 0.0;
    double graph_passed   = 0.0;

    for (const auto& s : snapshots) {
        requireFinite(s.residual_error, "snapshot residual_error");
        if (s.isResidualUnsafe(max_residual_error)) {
            throwMetric(
                MetricErrorKind::ResidualTooHighForPlanner,
                "snapshot residual_error exceeds max_residual_error");
        }

        sum_age      += static_cast<double>(s.artifact_age_ms);
        sum_delta    += static_cast<double>(s.delta_lag);
        sum_residual += s.residual_error;

        if (s.rank_cap_limit > 0 && s.rank_cap_used >= s.rank_cap_limit) {
            rank_growth_count += 1.0;
        }
        if (s.rebuild_triggered) {
          rebuild_count += 1.0;
        }
        if (s.exact_fallback_used) {
          fallback_count += 1.0;
        }

        if (s.summary_first_routing_used) {
            summary_first_total += 1.0;
            if (s.isUnrecoveredFalseNegative()) {
                summary_fn_unrecovered += 1.0;
            }
        }

        // Graph finalization pass rate: denominator counts queries that
        // *could* have reached the graph stage (ANN+Tensor+Graph path).
        if (s.graph_finalization_passed || s.exact_fallback_used) {
            graph_eligible += 1.0;
            if (s.graph_finalization_passed) {
              graph_passed += 1.0;
            }
        }
    }

    TensorGraphRuntimeMetrics result;
    result.mean_artifact_age_ms   = sum_age      / n;
    result.mean_delta_lag         = sum_delta    / n;
    result.mean_residual_error    = sum_residual / n;
    result.rank_growth_fraction   = rank_growth_count / n;
    result.rebuild_frequency      = rebuild_count     / n;
    result.exact_fallback_frequency = fallback_count  / n;
    result.summary_first_false_negative_rate = (summary_first_total > 0.0)
        ? summary_fn_unrecovered / summary_first_total
        : 0.0;
    result.graph_verified_finalization_pass_rate = (graph_eligible > 0.0)
        ? graph_passed / graph_eligible
        : 0.0;

    return result;
}

// ============================================================================
// § 8  MetricCollector
// ============================================================================

void MetricCollector::recordSnapshot(TensorGraphSnapshot snapshot) {
    snapshots_.push_back(std::move(snapshot));
}

void MetricCollector::recordShardQuery(
    uint32_t shard_count, double bytes, uint32_t skipped)
{
    shard_counts_.push_back(shard_count);
    shard_bytes_.push_back(bytes);
    shard_skipped_.push_back(skipped);
}

std::size_t MetricCollector::snapshotCount() const noexcept {
    return snapshots_.size();
}

TensorGraphRuntimeMetrics MetricCollector::summarizeTensorGraph(
    double max_residual_error) const
{
    return computeTensorGraphRuntimeMetrics(snapshots_, max_residual_error);
}

DistributedEfficiencyMetrics MetricCollector::summarizeDistributed(
    uint32_t total_shards) const
{
    return computeDistributedEfficiency(
        shard_counts_, shard_bytes_, shard_skipped_, total_shards);
}

void MetricCollector::reset() noexcept {
    snapshots_.clear();
    shard_counts_.clear();
    shard_bytes_.clear();
    shard_skipped_.clear();
}

} // namespace evaluation
} // namespace themis
