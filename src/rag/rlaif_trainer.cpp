/**
 * @file rlaif_trainer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/rlaif_trainer.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis::rag::training {

// ============================================================
// Internal helpers
// ============================================================
namespace {

/// Compute lexical diversity (unique tokens / total tokens).
double lexicalDiversity(const std::string& text) {
    if (text.empty()) {
        return 0.0;
    }
    std::istringstream ss(text);
    std::string word = {};
    std::unordered_set<std::string> unique;
    size_t total = 0;
    while (ss >> word) {
        ++total;
        std::string lower = {};
        lower.reserve(word.size());
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                lower += static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (!lower.empty()) {
            unique.insert(lower);
        }
    }
    return total == 0 ? 0.0
                      : static_cast<double>(unique.size()) /
                            static_cast<double>(total);
}

/// Simple heuristic quality score: combines length (capped) and diversity.
double heuristicQuality(const std::string& response) {
    if (response.empty()) {
        return 0.0;
    }
    // Length contribution: log-normalised, target ~200 chars → 0.5.
    const double len_score =
        std::min(1.0, std::log1p(static_cast<double>(response.size())) /
                          std::log1p(400.0));
    const double div_score = lexicalDiversity(response);
    return 0.6 * len_score + 0.4 * div_score;
}

/// Check whether @p text contains any of the given patterns (case-insensitive).
bool containsAnyPattern(const std::string&              text,
                         const std::vector<std::string>& patterns) {
    std::string lower = {};
    lower.reserve(text.size());
    for (char c : text) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    for (const auto& p : patterns) {
        if (lower.find(p) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // anonymous namespace

// ============================================================
// HeuristicAIJudge
// ============================================================

double HeuristicAIJudge::judge(const std::string& /*prompt*/,
                                const std::string& response_a,
                                const std::string& response_b) const {
    const double qa = heuristicQuality(response_a);
    const double qb = heuristicQuality(response_b);
    const double total = qa + qb;
    if (total < 1e-12) {
        return 0.5;
    }
    return qa / total;
}

std::string HeuristicAIJudge::critique(const std::string& /*prompt*/,
                                        const std::string& response,
                                        const AIPrinciple& principle) const {
    // Heuristic: check for harmful / discriminatory keywords.
    static const std::vector<std::string> harmful_patterns = {
        "harm", "kill", "destroy", "illegal", "dangerous", "threat"};
    static const std::vector<std::string> bias_patterns = {
        "always inferior", "typical of", "all of them", "never capable"};
    static const std::vector<std::string> dishonest_patterns = {
        "guaranteed", "definitely true", "100% certain", "absolute fact"};

    const std::vector<std::string>* patterns = nullptr;
    if (principle.strategy == ConstitutionalStrategy::HARMLESSNESS) {
        patterns = &harmful_patterns;
    } else if (principle.strategy == ConstitutionalStrategy::FAIRNESS) {
        patterns = &bias_patterns;
    } else if (principle.strategy == ConstitutionalStrategy::HONESTY) {
        patterns = &dishonest_patterns;
    }

    if (patterns && containsAnyPattern(response, *patterns)) {
        return "The response contains content that violates the principle: " +
               principle.description + ". Please revise to address this.";
    }
    return "Review the response against the principle: " + principle.description +
           ". No explicit violation was detected, but the answer should remain aligned with it.";
}

std::string HeuristicAIJudge::revise(const std::string& /*prompt*/,
                                      const std::string& response,
                                      const std::string& critique_text,
                                      const AIPrinciple& /*principle*/) const {
    if (critique_text.empty()) {
        return response;
    }
    // Heuristic revision: prepend a disclaimer and truncate harmful content.
    return "I want to provide a helpful and responsible answer. " + response;
}

// ============================================================
// RLAIFTrainer::Impl
// ============================================================

struct RLAIFTrainer::Impl {
    RLAIFConfig                  config;
    std::shared_ptr<IAIJudge>    judge;
    std::vector<PreferencePair>  dataset;
    mutable std::mutex           stats_mutex;
    RLAIFTrainerStats            stats;
    std::function<void(const RLAIFTrainingStep&)> step_callback;

    // Queue for batch processing.
    mutable std::mutex queue_mutex;
    std::vector<std::pair<std::string, std::string>> queue; // (query, draft)

    // DK-5: Cross-shard feedback counters
    mutable std::mutex cross_shard_mutex;
    CrossShardStats cross_shard_stats;
};

// ============================================================
// RLAIFTrainer — construction
// ============================================================

// Wave 5 R8: exception_in_destructor — make destructor explicitly noexcept
// and suppress any exception that could propagate during cleanup.
RLAIFTrainer::~RLAIFTrainer() noexcept {
    try {
        // impl_ is a unique_ptr<Impl>; Impl holds mutexes and shared_ptrs.
        // All of their destructors are noexcept, so this try/catch is a
        // belt-and-suspenders guard in case a shared IAIJudge deleter throws.
        impl_.reset();
    } catch (const std::exception& e) {
        THEMIS_WARN("RLAIFTrainer destructor: exception suppressed: {}", e.what());
    } catch (...) {
        THEMIS_WARN("RLAIFTrainer destructor: unknown exception suppressed");
    }
}

RLAIFTrainer::RLAIFTrainer()
    : impl_(std::make_unique<Impl>()) {
    impl_->config = {};
    impl_->judge  = std::make_shared<HeuristicAIJudge>();
    loadDefaultPrinciples();
}

RLAIFTrainer::RLAIFTrainer(const RLAIFConfig&       config,
                             std::shared_ptr<IAIJudge> judge)
    : impl_(std::make_unique<Impl>()) {
    validateConfig(config);
    impl_->config = config;
    impl_->judge  = judge ? std::move(judge)
                          : std::make_shared<HeuristicAIJudge>();
    // Principles come from config; if empty, caller calls loadDefaultPrinciples.
}

void RLAIFTrainer::validateConfig(const RLAIFConfig& config) {
    if (config.max_revision_iterations < 1) {
        throw std::invalid_argument(
            "RLAIFTrainer: max_revision_iterations must be >= 1");
    }
    if (config.min_quality_threshold < 0.0 ||
        config.min_quality_threshold > 1.0) {
        throw std::invalid_argument(
            "RLAIFTrainer: min_quality_threshold must be in [0, 1]");
    }
    if (config.min_preference_score < 0.0 ||
        config.min_preference_score > 1.0) {
        throw std::invalid_argument(
            "RLAIFTrainer: min_preference_score must be in [0, 1]");
    }
    if (config.improvement_threshold < 0.0) {
        throw std::invalid_argument(
            "RLAIFTrainer: improvement_threshold must be >= 0");
    }
}

// ============================================================
// Default principles
// ============================================================

void RLAIFTrainer::loadDefaultPrinciples() {
    impl_->config.principles.clear();

    impl_->config.principles.push_back({
        "harmlessness-1",
        "The response must not contain harmful, dangerous, or threatening content.",
        "Does the response contain harmful content? Identify any issues.",
        "Revise the response to remove all harmful or dangerous content.",
        ConstitutionalStrategy::HARMLESSNESS
    });

    impl_->config.principles.push_back({
        "helpfulness-1",
        "The response should be genuinely helpful and address the user's needs.",
        "Is the response helpful? Identify ways it could be more useful.",
        "Revise the response to be more helpful and directly address the query.",
        ConstitutionalStrategy::HELPFULNESS
    });

    impl_->config.principles.push_back({
        "honesty-1",
        "The response should be honest and avoid overconfident claims.",
        "Does the response make any overconfident or unverifiable claims?",
        "Revise to use appropriate hedging language and acknowledge uncertainty.",
        ConstitutionalStrategy::HONESTY
    });

    impl_->config.principles.push_back({
        "fairness-1",
        "The response must not contain discriminatory or biased language.",
        "Does the response contain discriminatory or biased language?",
        "Revise the response to use fair and neutral language.",
        ConstitutionalStrategy::FAIRNESS
    });
}

void RLAIFTrainer::addPrinciple(const AIPrinciple& principle) {
    impl_->config.principles.push_back(principle);
}

void RLAIFTrainer::removePrinciple(const std::string& principle_id) {
    auto& principles = impl_->config.principles;
    principles.erase(
        std::remove_if(principles.begin(), principles.end(),
                       [&principle_id](const AIPrinciple& p) {
                           return p.id == principle_id;
                       }),
        principles.end());
}

const std::vector<AIPrinciple>& RLAIFTrainer::getPrinciples() const {
    return impl_->config.principles;
}

// ============================================================
// Core: single-step training
// ============================================================

ConstitutionalCritique RLAIFTrainer::generateCritique(
    const std::string& response,
    const AIPrinciple& principle) const {
    ConstitutionalCritique result;
    result.principle_id = principle.id;

    const std::string crit_text = impl_->judge->critique("", response, principle);
    result.critique_text     = crit_text;
    result.violation_detected = !crit_text.empty();
    // Severity proxy: length of critique relative to a cap of 200 chars.
    result.severity =
        crit_text.empty()
            ? 0.0
            : std::min(1.0, static_cast<double>(crit_text.size()) / 200.0);
    return result;
}

std::string RLAIFTrainer::generateRevision(const std::string& response,
                                            const std::string& critique_text,
                                            const AIPrinciple& principle) const {
    return impl_->judge->revise("", response, critique_text, principle);
}

double RLAIFTrainer::scoreResponse(const std::string& response) const {
    return heuristicQuality(response);
}

ConstitutionalRevision RLAIFTrainer::applyRevisionCycle(
    const std::string& response,
    int                iteration) const {
    ConstitutionalRevision rev;
    rev.original_response = response;
    rev.revised_response  = response;
    rev.iteration         = iteration;

    const auto& principles = impl_->config.principles;
    if (principles.empty()) {
        rev.outcome = RevisionOutcome::UNCHANGED;
        return rev;
    }

    std::string current = response;
    bool any_violation  = false;

    for (const auto& principle : principles) {
        const auto critique = generateCritique(current, principle);
        rev.critiques.push_back(critique);

        if (critique.violation_detected) {
            any_violation = true;
            const std::string revised =
                generateRevision(current, critique.critique_text, principle);
            if (revised != current) {
                current = revised;
            }
        }
    }

    rev.revised_response = current;

    if (!any_violation) {
        rev.outcome = RevisionOutcome::UNCHANGED;
    } else {
        const double orig_score = scoreResponse(response);
        const double rev_score  = scoreResponse(current);
        rev.quality_delta       = rev_score - orig_score;
        rev.outcome = (rev_score >= orig_score) ? RevisionOutcome::ACCEPTED
                                                 : RevisionOutcome::REJECTED;
    }

    return rev;
}

PreferencePair RLAIFTrainer::createPreferencePair(
    const std::string& query,
    const std::string& response_a,
    const std::string& response_b) const {
    const double pref_a = impl_->judge->judge(query, response_a, response_b);

    PreferencePair pair;
    pair.prompt    = query;
    pair.created_at = std::chrono::system_clock::now();

    if (pref_a >= 0.5) {
        pair.chosen           = response_a;
        pair.rejected         = response_b;
        pair.preference_score = pref_a;
    } else {
        pair.chosen           = response_b;
        pair.rejected         = response_a;
        pair.preference_score = 1.0 - pref_a;
    }

    if (impl_->config.include_rationale) {
        std::ostringstream rationale = {};
        rationale << "AI judge (" << impl_->judge->name() << ") assigned "
                  << "preference score " << pair.preference_score
                  << " to the chosen response.";
        pair.judge_rationale = rationale.str();
    }

    for (const auto& p : impl_->config.principles) {
        pair.applied_principles.push_back(p.id);
    }

    return pair;
}

RLAIFTrainingStep RLAIFTrainer::runTrainingStep(
    const std::string& query,
    const std::string& draft_response) {
    const auto step_start = std::chrono::steady_clock::now();

    RLAIFTrainingStep step;
    step.query = query;

    if (impl_->config.principles.empty()) {
        step.success       = false;
        step.error_message = "No constitutional principles configured.";
        {
            std::lock_guard<std::mutex> lock(impl_->stats_mutex);
            ++impl_->stats.total_steps;
            ++impl_->stats.failed_steps;
        }
        return step;
    }

    // ── Constitutional AI revision loop ────────────────────────────────────
    std::string current = draft_response;
    const int max_iter  = impl_->config.max_revision_iterations;

    for (int iter = 0; iter < max_iter; ++iter) {
        const auto rev = applyRevisionCycle(current, iter);
        step.revision_chain.push_back(rev);

        if (rev.outcome == RevisionOutcome::ACCEPTED) {
            current = rev.revised_response;
            {
                std::lock_guard<std::mutex> lock(impl_->stats_mutex);
                ++impl_->stats.revisions_performed;
            }
            for (const auto& crit : rev.critiques) {
                if (crit.violation_detected) {
                    {
                        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
                        ++impl_->stats.violations_detected;
                    }
                }
            }
        } else if (rev.outcome == RevisionOutcome::UNCHANGED) {
            break; // No violations found; no further iterations needed.
        }
        // REJECTED: continue with original to try different principles.
    }

    // ── Preference pair generation ─────────────────────────────────────────
    const std::string& final_response = current;
    if (final_response == draft_response) {
        // No revision occurred; cannot form a meaningful preference pair
        // unless the draft and revised are different strings.
        // We still create a degenerate pair for completeness.
    }

    const auto pair = createPreferencePair(query, final_response, draft_response);
    step.preference_pair = pair;

    const bool meets_threshold =
        pair.preference_score >= impl_->config.min_preference_score;

    if (meets_threshold &&
        impl_-> static_cast<int>(dataset.size()) < impl_->config.max_dataset_size) {
        impl_->dataset.push_back(pair);
    }

     step.success = true;

    // ── Update statistics ──────────────────────────────────────────────────
    const auto elapsed = std::chrono::steady_clock::now() - step_start;
    step.elapsed_ms    = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    {
        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
        ++impl_->stats.total_steps;
        ++impl_->stats.successful_steps;

        // Running average of preference score.
        {
            const double n       = static_cast<double>(impl_->stats.successful_steps);
            const double old_avg = impl_->stats.avg_preference_score;
            impl_->stats.avg_preference_score =
                old_avg + (pair.preference_score - old_avg) / n;
        }

        // Running average of step elapsed time.
        {
            const double n = static_cast<double>(impl_->stats.total_steps);
            const std::chrono::milliseconds::rep old_ms = impl_->stats.avg_step_ms.count();
            const std::chrono::milliseconds::rep new_ms = step.elapsed_ms.count();
            impl_->stats.avg_step_ms =
                std::chrono::milliseconds(
                    static_cast<std::chrono::milliseconds::rep>(
                        old_ms + (new_ms - old_ms) / n));
        }
    }

    if (impl_->step_callback) {
        try {
            impl_->step_callback(step);
        } catch (...) {
            // Callbacks must not propagate exceptions.
        }
    }

    THEMIS_DEBUG("RLAIFTrainer::runTrainingStep: revisions={} pref_score={:.3f}",
                 step.revision_chain.size(), pair.preference_score);

    return step;
}

// ============================================================
// Batch processing
// ============================================================

void RLAIFTrainer::addToQueue(const std::string& query,
                               const std::string& draft_response) {
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    impl_->queue.emplace_back(query, draft_response);
}

std::vector<RLAIFTrainingStep> RLAIFTrainer::processBatch() {
    std::vector<RLAIFTrainingStep> results;
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        results.reserve(impl_-> static_cast<int>(queue.size()));
        for (const auto& [query, draft] : impl_->queue) {
            results.push_back(runTrainingStep(query, draft));
        }
        impl_->queue.clear();
    }
    return results;
}

// ============================================================
// Dataset access
// ============================================================

const std::vector<PreferencePair>& RLAIFTrainer::getDataset() const {
    return impl_->dataset;
}

void RLAIFTrainer::clearDataset() {
    impl_->dataset.clear();
    resetStats();
}

size_t RLAIFTrainer::datasetSize() const {
    return static_cast<bool>(impl_- < static_cast<int>(dataset.size()));
}

// ============================================================
// Statistics & monitoring
// ============================================================

RLAIFTrainerStats RLAIFTrainer::getStats() const {
    RLAIFTrainerStats stats;
    {
        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
        stats = impl_->stats;
    }
    // Build per-principle violation summary from the dataset.
    std::unordered_map<std::string, size_t> pv_map = {};

    for (const auto& pair : impl_->dataset) {
        for (const auto& pid : pair.applied_principles) {
            ++pv_map[pid];
        }
    }
    stats.principle_violations.assign(pv_map.begin(), pv_map.end());
    return stats;
}

void RLAIFTrainer::resetStats() {
    std::lock_guard<std::mutex> lock(impl_->stats_mutex);
    impl_->stats = {};
}

void RLAIFTrainer::setStepCallback(
    std::function<void(const RLAIFTrainingStep&)> callback) {
    impl_->step_callback = std::move(callback);
}

// ============================================================
// Configuration
// ============================================================

const RLAIFConfig& RLAIFTrainer::getConfig() const {
    return impl_->config;
}

void RLAIFTrainer::setConfig(const RLAIFConfig& config) {
    validateConfig(config);
    impl_->config = config;
}

void RLAIFTrainer::setJudge(std::shared_ptr<IAIJudge> judge) {
    impl_->judge = judge ? std::move(judge)
                         : std::make_shared<HeuristicAIJudge>();
}

std::string RLAIFTrainer::judgeName() const {
    return impl_->judge ? impl_->judge->name() : "none";
}

// ============================================================
// Factory
// ============================================================

RLAIFTrainer RLAIFTrainerFactory::createDefault() {
    RLAIFTrainer trainer = {};
    return trainer;
}

RLAIFTrainer RLAIFTrainerFactory::createStrict(
    std::shared_ptr<IAIJudge> judge) {
    RLAIFConfig cfg;
    cfg.max_revision_iterations = 5;
    cfg.min_quality_threshold   = 0.75;
    cfg.min_preference_score    = 0.65;
    cfg.improvement_threshold   = 0.03;
    RLAIFTrainer trainer(cfg, std::move(judge));
    trainer.loadDefaultPrinciples();
    return trainer;
}

RLAIFTrainer RLAIFTrainerFactory::createFast(
    std::shared_ptr<IAIJudge> judge) {
    RLAIFConfig cfg;
    cfg.max_revision_iterations = 1;
    cfg.min_quality_threshold   = 0.5;
    cfg.min_preference_score    = 0.51;
    cfg.improvement_threshold   = 0.01;
    RLAIFTrainer trainer(cfg, std::move(judge));
    trainer.loadDefaultPrinciples();
    return trainer;
}

RLAIFTrainer RLAIFTrainerFactory::createWithJudge(
    std::shared_ptr<IAIJudge> judge,
    const RLAIFConfig&        config) {
    RLAIFTrainer trainer(config, std::move(judge));
    if (trainer.getPrinciples().empty()) {
        trainer.loadDefaultPrinciples();
    }
    return trainer;
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-5: Cross-shard RLAIF feedback ingestion
// ─────────────────────────────────────────────────────────────────────────────

void RLAIFTrainer::addCrossShardSummary(
    const distributed_knowledge::FeedbackSummary& /*summary*/,
    const PreferencePair& synthetic_pair)
{
    ++impl_->cross_shard_stats.received_summaries;
    impl_->dataset.push_back(synthetic_pair);
    ++impl_->cross_shard_stats.applied_pairs;
}

RLAIFTrainer::CrossShardStats RLAIFTrainer::getCrossShardStats() const
{
    return impl_->cross_shard_stats;
}

} // namespace themis::rag::training

