/**
 * @file federated_distillation_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "distributed_knowledge/federated_distillation_coordinator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Compute Gaussian DP noise sigma from (ε, δ, sensitivity).
/// σ = sensitivity · sqrt(2 · ln(1.25/δ)) / ε
double gaussianSigma(double epsilon, double delta, double sensitivity) {
    return sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;
}

/// Clamp to [lo, hi].
template <typename T> T clamp(T val, T lo, T hi) {
    return val < lo ? lo : (val > hi ? hi : val);
}

/// Normalise a non-negative vector to sum 1; no-op when sum == 0.
void normalise(std::vector<double> &v) {
    double sum = 0.0;
    for (auto x : v) {
        sum += x;
    }
    if (sum > 0.0) {
        for (auto &x : v) {
            x /= sum;
        }
    }
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

FederatedDistillationCoordinator::FederatedDistillationCoordinator(DistillationConfig cfg) : config_(std::move(cfg)) {
    if (!config_.isValid()) {
        throw std::invalid_argument("FederatedDistillationCoordinator: invalid DistillationConfig");
    }
}

FederatedDistillationCoordinator::~FederatedDistillationCoordinator() noexcept = default;

// ─────────────────────────────────────────────────────────────────────────────
// IFederatedDistillationCoordinator — submitSoftLabels
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::submitSoftLabels(const std::string &teacher_id, std::vector<SoftLabel> labels) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (labels.empty()) {
        throw std::invalid_argument("submitSoftLabels: labels must not be empty");
    }
    if (teacher_id.empty()) {
        throw std::invalid_argument("submitSoftLabels: teacher_id must not be empty");
    }

    // ── Consistency Level: CAUSAL (Round-based ordering) ─────────────────────
    // Detect if we're advancing to next round: either last round was broadcast
    // (last_round_ has value) or this is the first submission (current_round_ == 0).
    // This ensures round N+1 causally depends on round N broadcast.
    const bool will_advance_round = (last_round_.has_value() || current_round_ == 0);
    const uint64_t projected_round = will_advance_round ? (current_round_ + 1) : current_round_;

    // ── Consistency Level: STRONG (Privacy budget check) ────────────────────
    // Privacy budget enforcement: check projected round before accepting submit.
    // This blocks if max_rounds would be exceeded (fail-closed policy).
    if (config_.max_rounds > 0 && projected_round > config_.max_rounds) {
        throw std::runtime_error("FederatedDistillationCoordinator: DP privacy budget exhausted");
    }

    pending_teacher_id_ = teacher_id;
    pending_labels_     = std::move(labels);
    has_pending_        = true;

    // Advance round counter on first submit if we had just broadcast.
    // This implements causal ordering: broadcast at N → submit at N+1.
    if (will_advance_round) {
        ++current_round_;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// broadcastToStudents
// ─────────────────────────────────────────────────────────────────────────────

DistillationRound FederatedDistillationCoordinator::broadcastToStudents() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!has_pending_) {
        throw std::runtime_error("broadcastToStudents: no soft labels submitted for this round");
    }

    // ── Consistency Level: STRONG (Privacy budget guard) ────────────────────
    // Privacy budget verification blocks broadcast if exhausted.
    // This is a strong consistency guard: operation fails immediately if budget
    // exceeded. No eventual consistency fallback for privacy guarantees.
    if (!verifyPrivacyBudget()) {
        throw std::runtime_error("FederatedDistillationCoordinator: DP privacy budget exhausted");
    }

    // ── Policy gate ───────────────────────────────────────────────────────────
    if (policy_gate_ && !policy_gate_(current_round_, pending_teacher_id_)) {
        ++policy_block_count_;
        throw std::runtime_error("Policy gate rejected distillation broadcast");
    }

    // ── Apply DP noise ───────────────────────────────────────────────────────
    auto labels     = pending_labels_;
    bool dp_applied = false;
    if (config_.require_dp) {
        applyDPNoise(labels);
        dp_applied = true;
    }

    // ── Build round ───────────────────────────────────────────────────────────
    DistillationRound round;
    round.round         = current_round_;
    round.teacher_id    = pending_teacher_id_;
    round.labels        = std::move(labels);
    round.epsilon_spent = config_.dp_epsilon;
    round.label_count   = round.labels.size();
    round.dp_applied    = dp_applied;

    total_epsilon_spent_ += config_.dp_epsilon;
    ++broadcast_count_;

    // ── Dispatch to students ──────────────────────────────────────────────────
    for (const auto &[sid, cb] : students_) {
        cb(round);
    }

    // ── Audit ─────────────────────────────────────────────────────────────────
    if (audit_cb_) {
        audit_cb_(nlohmann::json{{"event", "distillation_broadcast"},
                                 {"round", round.round},
                                 {"teacher_id", round.teacher_id},
                                 {"label_count", round.label_count},
                                 {"epsilon_spent", round.epsilon_spent},
                                 {"total_epsilon", total_epsilon_spent_},
                                 {"dp_applied", round.dp_applied},
                                 {"student_count", students_.size()}});
    }

    last_round_  = round;
    has_pending_ = false;

    return round;
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

uint64_t FederatedDistillationCoordinator::currentRound() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_round_;
}

size_t FederatedDistillationCoordinator::submittedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return has_pending_ ? 1u : 0u;
}

std::optional<DistillationRound> FederatedDistillationCoordinator::lastRound() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_round_;
}

nlohmann::json FederatedDistillationCoordinator::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {{"current_round", current_round_},
            {"broadcast_count", broadcast_count_},
            {"rollback_count", rollback_count_},
            {"total_epsilon", total_epsilon_spent_},
            {"budget_remaining", privacyBudgetRemaining()},
            {"student_count", students_.size()},
            {"has_pending", has_pending_},
            {"config",
             {{"dp_epsilon", config_.dp_epsilon},
              {"dp_delta", config_.dp_delta},
              {"temperature", config_.temperature},
              {"alpha", config_.alpha},
              {"max_rounds", config_.max_rounds},
              {"require_dp", config_.require_dp}}}};
}

// ─────────────────────────────────────────────────────────────────────────────
// Student registration
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::registerStudent(const std::string &student_id,
                                                       std::function<void(const DistillationRound &)> cb) {
    if (student_id.empty()) {
        throw std::invalid_argument("registerStudent: student_id must not be empty");
    }
    if (!cb) {
        throw std::invalid_argument("registerStudent: callback must not be null");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    students_.emplace_back(student_id, std::move(cb));
}

// ─────────────────────────────────────────────────────────────────────────────
// DI setters
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::setPolicyGate(PolicyGate gate) {
    std::lock_guard<std::mutex> lock(mutex_);
    policy_gate_ = std::move(gate);
}

void FederatedDistillationCoordinator::setAuditCallback(std::function<void(const nlohmann::json &)> cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_cb_ = std::move(cb);
}

void FederatedDistillationCoordinator::setRollbackTrigger(std::function<void(uint64_t, double)> cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    rollback_trigger_ = std::move(cb);
}

void FederatedDistillationCoordinator::setNoiseGeneratorFn(NoiseGeneratorFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    noise_generator_fn_ = std::move(fn);
}

// ─────────────────────────────────────────────────────────────────────────────
// Utility reporting
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::reportStudentUtility(const std::string & /*student_id*/, double utility) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (utility < config_.min_utility_threshold && rollback_trigger_) {
        ++rollback_count_;
        rollback_trigger_(current_round_, utility);
    }

    // Track utility range
    if (!any_utility_reported_) {
        min_utility_reported_ = utility;
        max_utility_reported_ = utility;
        any_utility_reported_ = true;
    } else {
        if (utility < min_utility_reported_)
            min_utility_reported_ = utility;
        if (utility > max_utility_reported_)
            max_utility_reported_ = utility;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Budget observability
// ─────────────────────────────────────────────────────────────────────────────

double FederatedDistillationCoordinator::privacyBudgetRemaining() const {
    if (config_.max_rounds == 0) {
        return std::numeric_limits<double>::max();
    }
    return static_cast<double>(config_.max_rounds) * config_.dp_epsilon - total_epsilon_spent_;
}

bool FederatedDistillationCoordinator::verifyPrivacyBudget() const {
    if (config_.max_rounds == 0) {
        return true;
    }
    return current_round_ <= config_.max_rounds;
}

// ─────────────────────────────────────────────────────────────────────────────
// Reset (for tests / admin)
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_round_        = 0;
    total_epsilon_spent_  = 0.0;
    broadcast_count_      = 0;
    rollback_count_       = 0;
    policy_block_count_   = 0;
    min_utility_reported_ = 1.0;
    max_utility_reported_ = 1.0;
    any_utility_reported_ = false;
    has_pending_          = false;
    pending_labels_.clear();
    pending_teacher_id_.clear();
    last_round_.reset();
    students_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// DP noise injection
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::applyDPNoise(std::vector<SoftLabel> &labels) const {
    const double sigma = gaussianSigma(config_.dp_epsilon, config_.dp_delta, config_.dp_sensitivity);

    if (noise_generator_fn_) {
        // Delegate to the injected noise generator (e.g. GPU-backed cuRAND).
        noise_generator_fn_(labels, sigma);
        return;
    }

    // PERMANENT FALLBACK NOTE (FederatedDistillation CPU Gaussian noise):
    // Purpose: Uses std::random_device seeded Gaussian noise to satisfy DP.
    //   In production with CUDA-capable hardware, inject a GPU-based Gaussian
    //   noise kernel via setNoiseGeneratorFn() for batch efficiency.
    // Activation: Active when no NoiseGeneratorFn is injected via
    //   setNoiseGeneratorFn(); the CPU path is the production fallback.
    // Production Delta: CPU-only; GPU version would operate on tensors directly.
    // Note: inject via setNoiseGeneratorFn() to override with GPU path.
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::normal_distribution<double> noise_dist(0.0, sigma);

    for (auto &label : labels) {
        for (auto &p : label.probabilities) {
            p = clamp(p + noise_dist(rng), 0.0, 1.0);
        }
        normalise(label.probabilities);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// generateModelCard
// ─────────────────────────────────────────────────────────────────────────────

DistillationModelCard FederatedDistillationCoordinator::generateModelCard(const std::string &coordinator_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    DistillationModelCard card;
    card.coordinator_id       = coordinator_id;
    card.teacher_id           = last_round_.has_value() ? last_round_->teacher_id : pending_teacher_id_;
    card.rounds_completed     = broadcast_count_;
    card.total_epsilon        = total_epsilon_spent_;
    card.dp_epsilon_per_round = config_.dp_epsilon;
    card.dp_delta             = config_.dp_delta;
    card.dp_applied           = config_.require_dp;
    card.min_utility_reported = min_utility_reported_;
    card.max_utility_reported = max_utility_reported_;
    card.rollback_count       = rollback_count_;
    card.policy_blocks        = policy_block_count_;
    card.registered_students  = students_.size();
    return card;
}

} // namespace distributed_knowledge
} // namespace themis
