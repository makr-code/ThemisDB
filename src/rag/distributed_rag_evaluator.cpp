/**
 * @file distributed_rag_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=11, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/distributed_rag_evaluator.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <future>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>

namespace themis::rag::distributed {

// ============================================================================
// Internal implementation
// ============================================================================

struct DistributedRAGEvaluator::Impl {
    std::vector<JudgeWorkerConfig>    workers;
    DistributedEvaluatorConfig        config;
    std::atomic<uint64_t>             total_evaluations{0};
    mutable std::mutex                config_mutex;

    explicit Impl(std::vector<JudgeWorkerConfig> w, DistributedEvaluatorConfig c)
        : workers(std::move(w)), config(std::move(c)) {}
};

// ============================================================================
// Construction / destruction
// ============================================================================

DistributedRAGEvaluator::DistributedRAGEvaluator(
    std::vector<JudgeWorkerConfig>    workers,
    const DistributedEvaluatorConfig& config
)
    : impl_(std::make_unique<Impl>(std::move(workers), config))
{
    if (impl_->workers.empty()) {
        throw std::invalid_argument(
            "DistributedRAGEvaluator: at least one judge worker must be configured");
    }
    THEMIS_INFO("DistributedRAGEvaluator initialised with {} judge(s), "
                "aggregation={}",
                impl_->workers.size(),
                static_cast<int>(config.aggregation));
}

DistributedRAGEvaluator::~DistributedRAGEvaluator() = default;

// ============================================================================
// evaluate()
// ============================================================================

std::pair<judge::EvaluationResult, DistributedEvaluationMeta>
DistributedRAGEvaluator::evaluate(const judge::EvaluationInput& input)
{
    const auto wall_start = std::chrono::steady_clock::now();

    const size_t n = impl_->workers.size();
    const size_t max_parallel = (impl_->config.max_parallel_judges == 0)
                                 ? n
                                 : impl_->config.max_parallel_judges;

    // Snapshot config fields.  Only `aggregation` is mutable after construction
    // (via setAggregationStrategy), so only that field is read under the mutex.
    AggregationStrategy agg_strategy;
    const auto          timeout        = impl_->config.per_judge_timeout;
    const bool          skip_failed    = impl_->config.skip_failed_judges;
    const size_t        min_ok_judges  = impl_->config.min_successful_judges;
    {
        std::lock_guard<std::mutex> lk(impl_->config_mutex);
        agg_strategy = impl_->config.aggregation;
    }

    // Build judges as shared_ptrs so timed-out lambdas don't access freed memory.
    std::vector<std::shared_ptr<judge::RAGJudge>> judges;
    judges.reserve(n);
    for (const auto& w : impl_->workers) {
        judges.push_back(std::make_shared<judge::RAGJudge>(w.judge_config));
    }

    // Copy the input into shared storage so lambdas that outlive evaluate()
    // (due to timeout handling) still have valid access.
    auto shared_input = std::make_shared<judge::EvaluationInput>(input);

    // Semaphore-like control for max_parallel.
    // Use shared_ptr so lambdas that run past the evaluate() return keep
    // valid references (only possible when per_judge_timeout > 0).
    struct SemState {
        std::atomic<size_t>     running{0};
        std::mutex              mtx = {};
        std::condition_variable cv = {};
    };
    auto sem = std::make_shared<SemState>();

    std::vector<std::future<judge::EvaluationResult>> futures;
    futures.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        // Acquire a parallelism slot before launching.
        {
            std::unique_lock<std::mutex> lk(sem->mtx);
            sem->cv.wait(lk, [&sem, max_parallel] {
                return sem->running.load() < max_parallel;
            });
            ++sem->running;
        }

        auto judge_ptr = judges[i];  // shared ownership — safe across timeouts
        
        // Skip null judges
        if (!judge_ptr) {
            THEMIS_WARN("DistributedRAGEvaluator: Skipping null judge at index {}", i);
            std::lock_guard<std::mutex> lk(sem->mtx);
            --sem->running;
            continue;
        }
        
        futures.push_back(
            std::async(std::launch::async,
                       [judge_ptr, shared_input, sem]() {
                           auto result = judge_ptr->evaluate(*shared_input);
                           {
                               std::lock_guard<std::mutex> lk(sem->mtx);
                               --sem->running;
                           }
                           sem->cv.notify_one();
                           return result;
                       }));
    }

    // Collect results
    DistributedEvaluationMeta meta;
    meta.individual_results.reserve(n);

    std::vector<judge::EvaluationResult> successful_results;
    std::vector<double>                  successful_weights;
    successful_results.reserve(n);
    successful_weights.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        bool ok = false;
        judge::EvaluationResult res{};

        try {
            if (timeout.count() > 0) {
                const auto status = futures[i].wait_for(timeout);
                if (status == std::future_status::ready) {
                    res = futures[i].get();
                    ok  = true;
                } else {
                    THEMIS_WARN("DistributedRAGEvaluator: judge '{}' timed out",
                                impl_->workers[i].judge_id);
                }
            } else {
                // Wave 5 R1: blocking_no_timeout — apply a 30 s default so
                // callers that omit per_judge_timeout cannot hang indefinitely.
                constexpr auto kEvalTimeout = std::chrono::seconds(30);
                const auto status = futures[i].wait_for(kEvalTimeout);
                if (status == std::future_status::ready) {
                    res = futures[i].get();
                    ok  = true;
                } else {
                    THEMIS_WARN(
                        "DistributedRAGEvaluator: judge '{}' timed out after 30 s "
                        "(no per_judge_timeout set), using fallback empty result",
                        impl_->workers[i].judge_id);
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("DistributedRAGEvaluator: judge '{}' threw: {}",
                        impl_->workers[i].judge_id, e.what());
        }

        if (ok) {
            successful_results.push_back(res);
            successful_weights.push_back(impl_->workers[i].weight);
            ++meta.successful_judges;
        } else {
            ++meta.failed_judges;
            if (!skip_failed) {
                throw std::runtime_error(
                    "DistributedRAGEvaluator: judge '" +
                    impl_->workers[i].judge_id + "' failed and skip_failed_judges=false");
            }
        }

        meta.individual_results.push_back(res);
    }

    if (meta.successful_judges < min_ok_judges) {
        throw std::runtime_error(
            "DistributedRAGEvaluator: only " +
            std::to_string(meta.successful_judges) +
            " judge(s) succeeded; need at least " +
            std::to_string(min_ok_judges));
    }

    // Aggregate (pass strategy explicitly to avoid re-reading config under lock)
    auto aggregated = aggregateResults(successful_results, successful_weights,
                                       agg_strategy);

    // Compute agreement
    meta.inter_judge_agreement = computeAgreement(successful_results);

    const auto wall_end = std::chrono::steady_clock::now();
    meta.total_elapsed  = std::chrono::duration_cast<std::chrono::milliseconds>(
        wall_end - wall_start);

    ++impl_->total_evaluations;

    THEMIS_DEBUG("DistributedRAGEvaluator: aggregated overall_score={:.3f}, "
                 "agreement={:.3f}, elapsed={}ms",
                 aggregated.overall_score,
                 meta.inter_judge_agreement,
                 meta.total_elapsed.count());

    return {aggregated, meta};
}

// ============================================================================
// batchEvaluate()
// ============================================================================

std::vector<std::pair<judge::EvaluationResult, DistributedEvaluationMeta>>
DistributedRAGEvaluator::batchEvaluate(
    const std::vector<judge::EvaluationInput>& inputs)
{
    std::vector<std::pair<judge::EvaluationResult, DistributedEvaluationMeta>> results;
    results.reserve(inputs.size());

    for (const auto& input : inputs) {
        results.push_back(evaluate(input));
    }

    return results;
}

// ============================================================================
// Configuration / accessors
// ============================================================================

void DistributedRAGEvaluator::setAggregationStrategy(AggregationStrategy strategy)
{
    std::lock_guard<std::mutex> lk(impl_->config_mutex);
    impl_->config.aggregation = strategy;
}

DistributedEvaluatorConfig DistributedRAGEvaluator::getConfig() const
{
    std::lock_guard<std::mutex> lk(impl_->config_mutex);
    return impl_->config;
}

size_t DistributedRAGEvaluator::judgeCount() const
{
    return impl_->workers.size();
}

uint64_t DistributedRAGEvaluator::totalEvaluations() const
{
    return impl_->total_evaluations.load();
}

// ============================================================================
// aggregateResults() — private
// ============================================================================

judge::EvaluationResult DistributedRAGEvaluator::aggregateResults(
    const std::vector<judge::EvaluationResult>& results,
    const std::vector<double>&                  weights,
    AggregationStrategy                         strategy) const
{
    if (results.empty()) {
        return judge::EvaluationResult{};
    }
    if (static_cast<int>(results.size()) == 1) {
        return results[0];
    }

    // ── BEST_OF_N ─────────────────────────────────────────────────────────
    if (strategy == AggregationStrategy::BEST_OF_N) {
        return *std::max_element(
            results.begin(), results.end(),
            [](const judge::EvaluationResult& a,
               const judge::EvaluationResult& b) {
                return a.overall_score < b.overall_score;
            });
    }

    // ── MAJORITY_VOTING ────────────────────────────────────────────────────
    if (strategy == AggregationStrategy::MAJORITY_VOTING) {
        // Each dimension is binarised at 0.5; majority determines the aggregate.
        judge::EvaluationResult out{};
        const size_t n = results.size();
        const size_t            half    = n / 2;

        auto majorityScore = [&](auto field_fn) -> double {
            size_t pass = 0;
            for (const auto& r : results) {
                if (field_fn(r) >= 0.5) { ++pass; }
            }
            return (pass > half) ? 1.0 : 0.0;
        };

        out.faithfulness_score    = majorityScore([](const auto& r) { return r.faithfulness_score; });
        out.relevance_score       = majorityScore([](const auto& r) { return r.relevance_score; });
        out.completeness_score    = majorityScore([](const auto& r) { return r.completeness_score; });
        out.coherence_score       = majorityScore([](const auto& r) { return r.coherence_score; });
        out.overall_score         = majorityScore([](const auto& r) { return r.overall_score; });
        out.judge_model           = "distributed-majority";
        return out;
    }

    // ── MEAN / WEIGHTED_MEAN ───────────────────────────────────────────────
    const bool use_weights = (strategy == AggregationStrategy::WEIGHTED_MEAN);

    double total_w = 0.0;
    for (size_t i = 0; i < results.size(); ++i) {
        total_w += use_weights ? weights[i] : 1.0;
    }
    if (total_w < std::numeric_limits<double>::epsilon()) { total_w = 1.0; }

    judge::EvaluationResult out{};
    out.judge_model = "distributed-aggregate";

    for (size_t i = 0; i < results.size(); ++i) {
        const double w = (use_weights ? weights[i] : 1.0) / total_w;
        const auto&  r = results[i];

        out.faithfulness_score         += w * r.faithfulness_score;
        out.relevance_score            += w * r.relevance_score;
        out.completeness_score         += w * r.completeness_score;
        out.coherence_score            += w * r.coherence_score;
        out.ethical_compliance_score   += w * r.ethical_compliance_score;
        out.overall_score              += w * r.overall_score;
        out.confidence                 += w * r.confidence;
    }

    // Boolean fields: majority vote (true if > half of judges agree)
    const size_t half = results.size() / 2;
    {
        size_t autonomy_true = 0, moral_true = 0, citation_true = 0, quality_true = 0;
        for (const auto& r : results) {
            if (r.respects_human_autonomy)  { ++autonomy_true; }
            if (r.shows_moral_diversity)    { ++moral_true; }
            if (r.has_ethical_citations)    { ++citation_true; }
            if (r.passed_quality_threshold) { ++quality_true; }
        }
        out.respects_human_autonomy  = autonomy_true  > half;
        out.shows_moral_diversity    = moral_true    > half;
        out.has_ethical_citations    = citation_true > half;
        out.passed_quality_threshold = quality_true  > half;
    }

    // Use the first successful result's claims and explanation as a
    // representative sample (full merging would require LLM post-processing).
    out.verified_claims   = results[0].verified_claims;
    out.unverified_claims = results[0].unverified_claims;
    out.explanation       = results[0].explanation;

    return out;
}

// ============================================================================
// computeAgreement() — static helper
// ============================================================================

double DistributedRAGEvaluator::computeAgreement(
    const std::vector<judge::EvaluationResult>& results)
{
    if (static_cast<int>(results.size()) <= 1) { return 1.0; }

    // Agreement = 1 - normalised standard deviation of overall_score
    double sum  = 0.0;
    double sum2 = 0.0;
    for (const auto& r : results) {
        sum  += r.overall_score;
        sum2 += r.overall_score * r.overall_score;
    }
    const double n    = static_cast<double>(results.size());
    const double mean = sum / n;
    const double var  = std::max(0.0, sum2 / n - mean * mean);
    const double std_dev = std::sqrt(var);

    // Scores are in [0, 1], so max std_dev ≈ 0.5.  Map to agreement [0, 1].
    return std::max(0.0, 1.0 - 2.0 * std_dev);
}

// ============================================================================
// DistributedEvaluatorFactory
// ============================================================================

std::unique_ptr<DistributedRAGEvaluator>
DistributedEvaluatorFactory::createHomogeneous(
    size_t                judge_count,
    judge::EvaluationMode mode,
    AggregationStrategy   aggregation)
{
    if (judge_count == 0) {
        throw std::invalid_argument(
            "DistributedEvaluatorFactory::createHomogeneous: judge_count must be >= 1");
    }

    judge::RAGJudgeConfig jcfg;
    jcfg.mode = mode;

    std::vector<JudgeWorkerConfig> workers;
    workers.reserve(judge_count);
    for (size_t i = 0; i < judge_count; ++i) {
        JudgeWorkerConfig w;
        w.judge_id     = "judge-" + std::to_string(i);
        w.judge_config = jcfg;
        w.weight       = 1.0;
        workers.push_back(std::move(w));
    }

    DistributedEvaluatorConfig cfg;
    cfg.aggregation = aggregation;

    return std::make_unique<DistributedRAGEvaluator>(std::move(workers), cfg);
}

std::unique_ptr<DistributedRAGEvaluator>
DistributedEvaluatorFactory::createFastThorough()
{
    judge::RAGJudgeConfig fast_cfg;
    fast_cfg.mode = judge::EvaluationMode::FAST;

    judge::RAGJudgeConfig thorough_cfg;
    thorough_cfg.mode = judge::EvaluationMode::THOROUGH;

    std::vector<JudgeWorkerConfig> workers;

    JudgeWorkerConfig w_fast;
    w_fast.judge_id     = "judge-fast";
    w_fast.judge_config = fast_cfg;
    w_fast.weight       = 0.4;
    workers.push_back(std::move(w_fast));

    JudgeWorkerConfig w_thorough;
    w_thorough.judge_id     = "judge-thorough";
    w_thorough.judge_config = thorough_cfg;
    w_thorough.weight       = 0.6;
    workers.push_back(std::move(w_thorough));

    DistributedEvaluatorConfig cfg;
    cfg.aggregation = AggregationStrategy::WEIGHTED_MEAN;

    return std::make_unique<DistributedRAGEvaluator>(std::move(workers), cfg);
}

} // namespace themis::rag::distributed

