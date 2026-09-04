/**
 * @file prompt_ab_experiment.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_ab_experiment.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// variantToString / stringToVariant
// ============================================================================

std::string variantToString(ExperimentVariant v) {
    return (v == ExperimentVariant::TREATMENT) ? "treatment" : "control";
}

std::optional<ExperimentVariant> stringToVariant(const std::string& s) {
    if (s == "treatment") { return ExperimentVariant::TREATMENT; }
    if (s == "control")   { return ExperimentVariant::CONTROL;   }
    return std::nullopt;
}

// ============================================================================
// statusToString
// ============================================================================

std::string statusToString(ExperimentStatus s) {
    switch (s) {
        case ExperimentStatus::RUNNING:           return "running";
        case ExperimentStatus::WINNER_CONTROL:    return "winner_control";
        case ExperimentStatus::WINNER_TREATMENT:  return "winner_treatment";
        case ExperimentStatus::INCONCLUSIVE:      return "inconclusive";
        case ExperimentStatus::COMPLETED:         return "completed";
    }
    return "unknown";
}

// ============================================================================
// PromptExperiment
// ============================================================================

nlohmann::json PromptExperiment::toJson() const {
    return {
        {"experiment_id",        experiment_id},
        {"template_id",          template_id},
        {"control_version_id",   control_version_id},
        {"treatment_version_id", treatment_version_id},
        {"split_pct",            split_pct},
        {"min_samples",          min_samples},
        {"confidence_level",     confidence_level},
        {"status",               statusToString(status)},
        {"created_at",           static_cast<std::int64_t>(
             std::chrono::system_clock::to_time_t(created_at))},
        {"stopped_at",           static_cast<std::int64_t>(
             std::chrono::system_clock::to_time_t(stopped_at))}
    };
}

PromptExperiment PromptExperiment::fromJson(const nlohmann::json& j) {
    PromptExperiment e;
    e.experiment_id        = j.value("experiment_id",        std::string{});
    e.template_id          = j.value("template_id",          std::string{});
    e.control_version_id   = j.value("control_version_id",   std::string{});
    e.treatment_version_id = j.value("treatment_version_id", std::string{});
    e.split_pct            = j.value("split_pct",            50);
    e.min_samples          = j.value("min_samples",          std::size_t{200});
    e.confidence_level     = j.value("confidence_level",     0.95);
    // status
    const std::string st   = j.value("status", std::string{"running"});
    if      (st == "winner_control")   { e.status = ExperimentStatus::WINNER_CONTROL;   }
    else if (st == "winner_treatment") { e.status = ExperimentStatus::WINNER_TREATMENT; }
    else if (st == "inconclusive")     { e.status = ExperimentStatus::INCONCLUSIVE;     }
    else if (st == "completed")        { e.status = ExperimentStatus::COMPLETED;        }
    else                               { e.status = ExperimentStatus::RUNNING;          }
    return e;
}

// ============================================================================
// ExperimentOutcome
// ============================================================================

nlohmann::json ExperimentOutcome::toJson() const {
    return {
        {"experiment_id", experiment_id},
        {"variant",       variantToString(variant)},
        {"score",         score},
        {"request_id",    request_id},
        {"timestamp",     static_cast<std::int64_t>(
             std::chrono::system_clock::to_time_t(timestamp))}
    };
}

// ============================================================================
// ExperimentSummary
// ============================================================================

nlohmann::json ExperimentSummary::toJson() const {
    return {
        {"experiment",         experiment.toJson()},
        {"control_samples",    control_samples},
        {"treatment_samples",  treatment_samples},
        {"mean_control_score", mean_control_score},
        {"mean_treatment_score", mean_treatment_score},
        {"delta_pct",          delta_pct},
        {"p_value",            p_value},
        {"significant",        significant},
        {"winner_version_id",  winner_version_id}
    };
}

// ============================================================================
// PromptABExperimentFramework — statics
// ============================================================================

// MurmurHash3-32 (public domain, Austin Appleby).
// Seed chosen to give an even bucket distribution for short strings.
static constexpr std::uint32_t kMurmurSeed = 0x9747b28cu;

std::uint32_t PromptABExperimentFramework::murmur3_32(
        const std::string& key) noexcept {
    const auto* data    = reinterpret_cast<const std::uint8_t*>(key.data());
    const std::size_t n = key.size();
    const std::size_t nblocks = n / 4;

    std::uint32_t h1 = kMurmurSeed;
    const std::uint32_t c1 = 0xcc9e2d51u;
    const std::uint32_t c2 = 0x1b873593u;

    const auto* blocks =
        reinterpret_cast<const std::uint32_t*>(data);
    for (std::size_t i = 0; i < nblocks; ++i) {
        std::uint32_t k1 = {};
        std::memcpy(&k1, blocks + i, sizeof(k1));
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64u;
    }

    const auto* tail = data + nblocks * 4;
    std::uint32_t k1 = 0;
    switch (n & 3) {
        case 3: k1 ^= static_cast<std::uint32_t>(tail[2]) << 16; [[fallthrough]];
        case 2: k1 ^= static_cast<std::uint32_t>(tail[1]) <<  8; [[fallthrough]];
        case 1: k1 ^= static_cast<std::uint32_t>(tail[0]);
                k1 *= c1;
                k1  = (k1 << 15) | (k1 >> 17);
                k1 *= c2;
                h1 ^= k1;
    }

    h1 ^= static_cast<std::uint32_t>(n);
    h1 ^= h1 >> 16;  h1 *= 0x85ebca6bu;
    h1 ^= h1 >> 13;  h1 *= 0xc2b2ae35u;
    h1 ^= h1 >> 16;
    return h1;
}

// Student-t CDF rational approximation (Abramowitz & Stegun 26.7.8).
// Returns P(T ≤ t) for t ≥ 0 with `df` degrees of freedom.
double PromptABExperimentFramework::tDistCdf(double t, double df) noexcept {
    if (df <= 0.0 || std::isnan(t)) { return 0.5; }
    const double x    = df / (df + t * t);
    // Regularised incomplete Beta I_x(a,b) with a = df/2, b = 0.5.
    // Use the continued-fraction expansion (Lentz) for I_x.
    const double a    = df / 2.0;
    const double b    = 0.5;
    // Maximum depth for continued-fraction convergence.
    constexpr int kMaxIter = 200;
    constexpr double kEps  = 1e-12;

    // betai via continued-fraction (for x < (a+1)/(a+b+2) use direct CF).
    // We always satisfy x = df/(df+t^2) ≤ 1, and since t≥0 we use the
    // standard direction.
    const double lnBeta =
        std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b);
    const double front  = std::exp(std::log(x) * a +
                                    std::log(1.0 - x) * b - lnBeta) / a;

    // Lentz continued-fraction for betacf(a, b, x).
    double f  = 1.0;
    double C  = f;
    double D  = 1.0 - (a + b) * x / (a + 1.0);
    if (std::abs(D) < kEps) { D = kEps; }
    D = 1.0 / D;
    f = C * D;
    for (int m = 1; m <= kMaxIter; ++m) {
        const double dm  = static_cast<double>(m);
        // Even step.
        double num = dm * (b - dm) * x /
                     ((a + 2.0 * dm - 1.0) * (a + 2.0 * dm));
        D = 1.0 + num * D;
        if (std::abs(D) < kEps) { D = kEps; }
        C = 1.0 + num / C;
        if (std::abs(C) < kEps) { C = kEps; }
        D = 1.0 / D;
        f *= C * D;
        // Odd step.
        num = -(a + dm) * (a + b + dm) * x /
              ((a + 2.0 * dm) * (a + 2.0 * dm + 1.0));
        D = 1.0 + num * D;
        if (std::abs(D) < kEps) { D = kEps; }
        C = 1.0 + num / C;
        if (std::abs(C) < kEps) { C = kEps; }
        D = 1.0 / D;
        const double delta = C * D;
        f *= delta;
        if (std::abs(delta - 1.0) < kEps) { break; }
    }
    const double betai = front * f;

    // For t ≥ 0: P(T ≤ t) = 1 − betai/2.
    return 1.0 - betai / 2.0;
}

double PromptABExperimentFramework::welchPValue(
        const std::vector<double>& a,
        const std::vector<double>& b) noexcept {
    const std::size_t na = a.size();
    const std::size_t nb = b.size();
    if (na < 2 || nb < 2) { return 1.0; }

    const double mean_a =
        std::accumulate(a.begin(), a.end(), 0.0) / static_cast<double>(na);
    const double mean_b =
        std::accumulate(b.begin(), b.end(), 0.0) / static_cast<double>(nb);

    auto var = [](const std::vector<double>& v, double mean,
                  std::size_t n) -> double {
        double s = 0.0;
        for (double x : v) { s += (x - mean) * (x - mean); }
        return s / static_cast<double>(n - 1);
    };

    const double va = var(a, mean_a, na);
    const double vb = var(b, mean_b, nb);

    const double dna = static_cast<double>(na);
    const double dnb = static_cast<double>(nb);

    const double se2 = va / dna + vb / dnb;
    if (se2 <= 0.0) { return 1.0; }
    const double se = std::sqrt(se2);

    const double t = std::abs(mean_a - mean_b) / se;

    // Welch-Satterthwaite degrees of freedom.
    const double df_num   = se2 * se2;
    const double df_denom = (va / dna) * (va / dna) / (dna - 1.0) +
                             (vb / dnb) * (vb / dnb) / (dnb - 1.0);
    if (df_denom <= 0.0) { return 1.0; }
    const double df = df_num / df_denom;

    // Two-tailed p-value.
    const double cdf_val = tDistCdf(t, df);
    return 2.0 * (1.0 - cdf_val);
}

// Unique ID generator (sequential suffix for determinism in tests).
std::string PromptABExperimentFramework::generateId() {
    static std::atomic<std::uint64_t> counter{1};
    std::ostringstream ss = {};
    ss << "exp-" << counter.fetch_add(1, std::memory_order_relaxed);
    return ss.str();
}

// ============================================================================
// Lifecycle
// ============================================================================

std::string PromptABExperimentFramework::create(PromptExperiment exp) {
    if (exp.experiment_id.empty()) {
        exp.experiment_id = generateId();
    }
    if (exp.split_pct < 0)   { exp.split_pct = 0;   }
    if (exp.split_pct > 100) { exp.split_pct = 100;  }
    exp.status     = ExperimentStatus::RUNNING;
    exp.created_at = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(mutex_);
    const std::string id = exp.experiment_id;
    experiments_[id] = std::move(exp);
    scores_[id];  // default-construct ScoreStore
    return id;
}

bool PromptABExperimentFramework::stop(const std::string& experiment_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(experiment_id);
    if (it == experiments_.end()) { return false; }
    if (it->second.status == ExperimentStatus::RUNNING) {
        it->second.status     = ExperimentStatus::INCONCLUSIVE;
        it->second.stopped_at = std::chrono::system_clock::now();
    }
    return true;
}

std::optional<PromptExperiment> PromptABExperimentFramework::getExperiment(
        const std::string& experiment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(experiment_id);
    if (it == experiments_.end()) { return std::nullopt; }
    return it->second;
}

std::vector<PromptExperiment> PromptABExperimentFramework::listExperiments() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PromptExperiment> out = {};

    out.reserve(experiments_.size());
    for (const auto& [id, exp] : experiments_) {
        out.push_back(exp);
    }
    return out;
}

// ============================================================================
// Variant assignment
// ============================================================================

ExperimentVariant PromptABExperimentFramework::assignVariant(
        const ExperimentContext& context) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(context.experiment_id);
    if (it == experiments_.end() ||
        it->second.status != ExperimentStatus::RUNNING) {
        return ExperimentVariant::CONTROL;
    }
    const int split_pct = it->second.split_pct;
    const std::uint32_t h = murmur3_32(context.request_id);
    return (static_cast<int>(h % 100) < split_pct)
           ? ExperimentVariant::TREATMENT
           : ExperimentVariant::CONTROL;
}

// ============================================================================
// Outcome recording
// ============================================================================

bool PromptABExperimentFramework::recordOutcome(
        const std::string& experiment_id,
        ExperimentVariant  variant,
        double             score,
        const std::string& request_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(experiment_id);
    if (it == experiments_.end()) {
        return false;
    }

    const auto status = it->second.status;
    if (status == ExperimentStatus::INCONCLUSIVE ||
        status == ExperimentStatus::COMPLETED) {
        return false;
    }

    auto& store = scores_[experiment_id];

    ExperimentOutcome outcome;
    outcome.experiment_id = experiment_id;
    outcome.variant       = variant;
    outcome.score         = score;
    outcome.request_id    = request_id;
    outcome.timestamp     = std::chrono::system_clock::now();
    store.outcomes.push_back(outcome);

    if (variant == ExperimentVariant::CONTROL) {
        store.control.push_back(score);
    } else {
        store.treatment.push_back(score);
    }

    // Auto-check significance only while the experiment is still actively
    // collecting evidence. Late-arriving observations after a winner was
    // declared are still retained for summaries and audit trails.
    const std::size_t min = it->second.min_samples;
    if (status == ExperimentStatus::RUNNING &&
        static_cast<int>(store.control.size()) >= min &&
        static_cast<int>(store.treatment.size()) >= min) {
        checkSignificanceLocked(experiment_id);
    }

    return true;
}

// ============================================================================
// Significance testing
// ============================================================================

bool PromptABExperimentFramework::checkSignificanceLocked(
        const std::string& experiment_id) {
    // Caller must hold mutex_.
    auto eit = experiments_.find(experiment_id);
    if (eit == experiments_.end() ||
        eit->second.status != ExperimentStatus::RUNNING) {
        return false;
    }
    const auto& store = scores_.at(experiment_id);
    if (static_cast<int>(store.control.size()) < 2 || static_cast<int>(store.treatment.size()) < 2) {
        return false;
    }

    const double p = welchPValue(store.control, store.treatment);
    const double alpha = 1.0 - eit->second.confidence_level;

    if (p >= alpha) { return false; }

    // Determine winner.
    const double mean_c =
        std::accumulate(store.control.begin(),
                        store.control.end(), 0.0) /
        static_cast<double>(store.control.size());
    const double mean_t =
        std::accumulate(store.treatment.begin(),
                        store.treatment.end(), 0.0) /
        static_cast<double>(store.treatment.size());

    const ExperimentVariant winner =
        (mean_t >= mean_c) ? ExperimentVariant::TREATMENT
                           : ExperimentVariant::CONTROL;

    eit->second.status     =
        (winner == ExperimentVariant::TREATMENT)
        ? ExperimentStatus::WINNER_TREATMENT
        : ExperimentStatus::WINNER_CONTROL;
    eit->second.stopped_at = std::chrono::system_clock::now();

    // Fire winner callback (outside the critical path; capture by value).
    if ([[maybe_unused]] winner_callback_) {
        const std::string wid =
            (winner == ExperimentVariant::TREATMENT)
            ? eit->second.treatment_version_id
            : eit->second.control_version_id;
        const std::string eid  = experiment_id;
        const auto cb          = winner_callback_;
        // Invoke inline; callers must not call back into the framework
        // from within the callback to avoid deadlock.
        try { cb(eid, winner, wid); } catch (...) {}
    }

    return true;
}

bool PromptABExperimentFramework::checkSignificance(
        const std::string& experiment_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkSignificanceLocked(experiment_id);
}

// ============================================================================
// Results
// ============================================================================

std::string PromptABExperimentFramework::promoteWinner(
        const std::string& experiment_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(experiment_id);
    if (it == experiments_.end()) { return {}; }
    const auto& exp = it->second;
    if (exp.status == ExperimentStatus::WINNER_TREATMENT) {
        it->second.status = ExperimentStatus::COMPLETED;
        return exp.treatment_version_id;
    }
    if (exp.status == ExperimentStatus::WINNER_CONTROL) {
        it->second.status = ExperimentStatus::COMPLETED;
        return exp.control_version_id;
    }
    return {};
}

ExperimentStatus PromptABExperimentFramework::getStatus(
        const std::string& experiment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = experiments_.find(experiment_id);
    if (it == experiments_.end()) { return ExperimentStatus::INCONCLUSIVE; }
    return it->second.status;
}

std::optional<ExperimentSummary> PromptABExperimentFramework::getSummary(
        const std::string& experiment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto eit = experiments_.find(experiment_id);
    if (eit == experiments_.end()) { return std::nullopt; }

    ExperimentSummary s;
    s.experiment = eit->second;

    auto sit = scores_.find(experiment_id);
    if (sit != scores_.end()) {
        const auto& store = sit->second;
        s.control_samples   = store.control.size();
        s.treatment_samples = store.treatment.size();

        if (!store.control.empty()) {
            s.mean_control_score =
                std::accumulate(store.control.begin(),
                                store.control.end(), 0.0) /
                static_cast<double>(store.control.size());
        }
        if (!store.treatment.empty()) {
            s.mean_treatment_score =
                std::accumulate(store.treatment.begin(),
                                store.treatment.end(), 0.0) /
                static_cast<double>(store.treatment.size());
        }
        if (s.mean_control_score > 0.0) {
            s.delta_pct =
                (s.mean_treatment_score - s.mean_control_score) /
                s.mean_control_score * 100.0;
        }

        if (static_cast<int>(store.control.size()) >= 2 && static_cast<int>(store.treatment.size()) >= 2) {
            s.p_value    = welchPValue(store.control, store.treatment);
            const double alpha = 1.0 - eit->second.confidence_level;
            s.significant = (s.p_value < alpha);
        }
    }

    const auto st = eit->second.status;
    if (st == ExperimentStatus::WINNER_TREATMENT) {
        s.winner_version_id = eit->second.treatment_version_id;
    } else if (st == ExperimentStatus::WINNER_CONTROL ||
               st == ExperimentStatus::COMPLETED) {
        if (st == ExperimentStatus::WINNER_CONTROL) {
            s.winner_version_id = eit->second.control_version_id;
        } else if (s.mean_treatment_score > s.mean_control_score) {
            s.winner_version_id = eit->second.treatment_version_id;
        } else if (s.control_samples > 0 || s.treatment_samples > 0) {
            s.winner_version_id = eit->second.control_version_id;
        }
    }
    return s;
}

std::vector<ExperimentOutcome> PromptABExperimentFramework::getOutcomes(
        const std::string& experiment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = scores_.find(experiment_id);
    if (it == scores_.end()) { return {}; }
    return it->second.outcomes;
}

// ============================================================================
// Callbacks
// ============================================================================

void PromptABExperimentFramework::setWinnerCallback([[maybe_unused]] WinnerCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    winner_callback_ = std::move([[maybe_unused]] cb);
}

// ============================================================================
// SimplePromptABFramework (IPromptABFramework)
// ============================================================================

uint32_t SimplePromptABFramework::fnv1a32(const std::string& user_id,
                                           const std::string& key) noexcept {
    // FNV-1a-32 over (user_id + NUL + key)
    constexpr uint32_t FNV_OFFSET = 2166136261;
    constexpr uint32_t FNV_PRIME  = 16777619;
    uint32_t hash = FNV_OFFSET;
    for (unsigned char c : user_id) {
        hash ^= c;
        hash *= FNV_PRIME;
    }
    hash ^= 0;
    hash *= FNV_PRIME;
    for (unsigned char c : key) {
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
}

void SimplePromptABFramework::registerExperiment(ExperimentDescriptor descriptor) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& e : experiments_) {
        if (e.key == descriptor.key) {
            throw std::invalid_argument(
                "SimplePromptABFramework: experiment key '" +
                descriptor.key + "' is already registered");
        }
    }
    experiments_.push_back(std::move(descriptor));
}

bool SimplePromptABFramework::deactivate(const ExperimentKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& e : experiments_) {
        if (e.key == key && e.active) {
            e.active = false;
            return true;
        }
    }
    return false;
}

ABVariant SimplePromptABFramework::assignVariant(const UserId&        user_id,
                                                  const ExperimentKey& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& exp : experiments_) {
        if (exp.key != key || !exp.active || exp.variants.empty()) {
            continue;
        }
        if (static_cast<int>(exp.variants.size()) == 1) {
            return exp.variants[0];
        }
        // Use FNV-1a hash modulo 100 for traffic split.
        const uint32_t bucket = fnv1a32(user_id, key) % 100;
        double cumulative = 0.0;
        for (const auto& v : exp.variants) {
            cumulative += v.trafficWeight * 100.0;
            if (static_cast<double>(bucket) < cumulative) {
                return v;
            }
        }
        // Fallback: last variant.
        return exp.variants.back();
    }
    // No active experiment found — return a default "control" variant.
    return ABVariant{"control", nullptr, 1.0};
}

std::vector<ExperimentDescriptor> SimplePromptABFramework::listExperiments() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return experiments_;
}

} // namespace prompt_engineering
} // namespace themis


