// Copyright 2026 ThemisDB — Licensed under MIT License

/**
 * @file federated_distillation_coordinator.cpp
 * @brief FederatedDistillationCoordinator — teacher→student soft-label
 *        transfer with Gaussian (ε, δ)-Differential Privacy.
 *
 * DP noise formula (Gaussian mechanism, same as Ebene B):
 *   σ = sensitivity * sqrt(2 * ln(1.25 / δ)) / ε
 *
 * References:
 *   Hinton et al. (2015). Distilling the Knowledge in a Neural Network.
 *   Jeong et al. (2018). Communication-Efficient On-Device ML: Federated
 *     Distillation and Augmentation under Non-IID Private Data.
 *   Dwork & Roth (2014). Algorithmic Foundations of Differential Privacy.
 */

#include "distributed_knowledge/federated_distillation_coordinator.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <stdexcept>

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// SoftLabelBatch serialisation
// ─────────────────────────────────────────────────────────────────────────────

nlohmann::json SoftLabelBatch::toJson() const
{
    nlohmann::json j;
    j["shard_id"]    = shard_id;
    j["round"]       = round;
    j["sample_count"] = sample_count;
    nlohmann::json jl = nlohmann::json::array();
    for (const auto& row : labels) { jl.push_back(row); }
    j["labels"] = std::move(jl);
    return j;
}

SoftLabelBatch SoftLabelBatch::fromJson(const nlohmann::json& j)
{
    SoftLabelBatch b;
    b.shard_id    = j.value("shard_id", "");
    b.round       = j.value<uint64_t>("round", 0);
    b.sample_count = j.value<size_t>("sample_count", 0);
    if (j.contains("labels") && j["labels"].is_array()) {
        for (const auto& row : j["labels"]) {
            b.labels.push_back(row.get<std::vector<double>>());
        }
    }
    return b;
}

// ─────────────────────────────────────────────────────────────────────────────
// AggregatedLabelBatch serialisation
// ─────────────────────────────────────────────────────────────────────────────

nlohmann::json AggregatedLabelBatch::toJson() const
{
    nlohmann::json j;
    j["round"]         = round;
    j["teachers"]      = teachers;
    j["epsilon_spent"] = epsilon_spent;
    j["version"]       = version;
    nlohmann::json jl = nlohmann::json::array();
    for (const auto& row : labels) { jl.push_back(row); }
    j["labels"] = std::move(jl);
    return j;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

FederatedDistillationCoordinator::FederatedDistillationCoordinator(
    DistillationConfig config)
    : config_(std::move(config))
{
    if (!config_.isValid()) {
        throw std::invalid_argument(
            "FederatedDistillationCoordinator: invalid DistillationConfig");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// submitLabels
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::submitLabels(const SoftLabelBatch& batch)
{
    if (batch.shard_id.empty()) {
        throw std::invalid_argument(
            "FederatedDistillationCoordinator::submitLabels: empty shard_id");
    }
    if (batch.labels.empty()) {
        throw std::invalid_argument(
            "FederatedDistillationCoordinator::submitLabels: empty labels");
    }
    const size_t label_size = batch.labels.front().size();
    if (label_size == 0) {
        throw std::invalid_argument(
            "FederatedDistillationCoordinator::submitLabels: inner label vector is empty");
    }
    for (const auto& row : batch.labels) {
        if (row.size() != label_size) {
            throw std::invalid_argument(
                "FederatedDistillationCoordinator::submitLabels: inconsistent label sizes");
        }
    }

    std::lock_guard<std::mutex> lk(mutex_);

    if (!verifyPrivacyBudget()) {
        throw std::runtime_error(
            "FederatedDistillationCoordinator: privacy budget exhausted");
    }

    // Idempotent: ignore duplicate from same shard in this round
    if (pending_labels_.count(batch.shard_id)) { return; }
    pending_labels_[batch.shard_id] = batch;

    if (pending_labels_.size() >= config_.min_teachers) {
        try {
            auto agg = doAggregation();
            last_batch_ = agg;
            if (student_callback_) { student_callback_(agg); }
        } catch (...) {
            // Leave pending_labels_ intact for manual retry
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// triggerAggregation
// ─────────────────────────────────────────────────────────────────────────────

AggregatedLabelBatch FederatedDistillationCoordinator::triggerAggregation()
{
    std::lock_guard<std::mutex> lk(mutex_);

    if (pending_labels_.size() < config_.min_teachers) {
        throw std::runtime_error(
            "FederatedDistillationCoordinator: not enough teacher labels submitted ("
            + std::to_string(pending_labels_.size())
            + "/" + std::to_string(config_.min_teachers) + ")");
    }

    auto agg = doAggregation();
    last_batch_ = agg;
    if (student_callback_) { student_callback_(agg); }
    return agg;
}

// ─────────────────────────────────────────────────────────────────────────────
// doAggregation (internal, caller holds mutex_)
// ─────────────────────────────────────────────────────────────────────────────

AggregatedLabelBatch FederatedDistillationCoordinator::doAggregation()
{
    // ── Policy gate ─────────────────────────────────────────────────────────
    if (policy_gate_) {
        if (!policy_gate_(current_round_, pending_labels_.size())) {
            const std::string reason =
                "policy gate rejected round " + std::to_string(current_round_);
            if (rollback_trigger_) { rollback_trigger_(current_round_, reason); }
            pending_labels_.clear();
            ++current_round_;
            throw std::runtime_error(
                "FederatedDistillationCoordinator: " + reason);
        }
    }

    // ── Validate that all batches have the same shape ────────────────────────
    const size_t n_queries    = pending_labels_.begin()->second.labels.size();
    const size_t n_classes    = pending_labels_.begin()->second.labels.front().size();
    for (const auto& [sid, b] : pending_labels_) {
        if (b.labels.size() != n_queries || b.labels.front().size() != n_classes) {
            throw std::runtime_error(
                "FederatedDistillationCoordinator: shape mismatch in shard '" + sid + "'");
        }
    }

    // ── Aggregate: weighted arithmetic mean (FedAvg over soft labels) ────────
    std::vector<std::vector<double>> agg(n_queries,
                                         std::vector<double>(n_classes, 0.0));
    double total_weight = 0.0;
    for (const auto& [sid, b] : pending_labels_) {
        const double w = static_cast<double>(b.sample_count > 0 ? b.sample_count : 1);
        total_weight += w;
        for (size_t q = 0; q < n_queries; ++q) {
            for (size_t c = 0; c < n_classes; ++c) {
                agg[q][c] += b.labels[q][c] * w;
            }
        }
    }
    if (total_weight > 0.0) {
        for (auto& row : agg) {
            for (auto& v : row) { v /= total_weight; }
        }
    }

    // ── Apply Gaussian DP noise ──────────────────────────────────────────────
    agg = applyGaussianNoise(std::move(agg));

    // ── Audit record ─────────────────────────────────────────────────────────
    if (audit_callback_) {
        nlohmann::json rec;
        rec["decision_type"] = "DISTILLATION_ROUND";
        rec["round"]         = current_round_;
        rec["teachers"]      = pending_labels_.size();
        rec["epsilon_spent"] = config_.dp_epsilon;
        audit_callback_(rec);
    }

    // ── Build result ─────────────────────────────────────────────────────────
    AggregatedLabelBatch result;
    result.round         = current_round_;
    result.teachers      = pending_labels_.size();
    result.labels        = std::move(agg);
    result.epsilon_spent = config_.dp_epsilon;
    result.version       = nextBatchVersion();

    total_epsilon_spent_ += config_.dp_epsilon;
    ++completed_rounds_;
    ++current_round_;
    pending_labels_.clear();

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// applyGaussianNoise
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::vector<double>>
FederatedDistillationCoordinator::applyGaussianNoise(
    std::vector<std::vector<double>> labels) const
{
    // σ = sensitivity * sqrt(2 * ln(1.25 / δ)) / ε
    const double sigma =
        config_.sensitivity *
        std::sqrt(2.0 * std::log(1.25 / config_.dp_delta)) /
        config_.dp_epsilon;

    if (sigma <= 0.0) { return labels; }

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::normal_distribution<double> noise(0.0, sigma);

    for (auto& row : labels) {
        for (auto& v : row) { v += noise(gen); }
    }
    return labels;
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

uint64_t FederatedDistillationCoordinator::currentRound() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return current_round_;
}

size_t FederatedDistillationCoordinator::submittedCount() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return pending_labels_.size();
}

size_t FederatedDistillationCoordinator::completedRounds() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return completed_rounds_;
}

double FederatedDistillationCoordinator::privacyBudgetRemaining() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (config_.max_rounds == 0) {
        return std::numeric_limits<double>::max();
    }
    return static_cast<double>(config_.max_rounds) * config_.dp_epsilon
           - total_epsilon_spent_;
}

bool FederatedDistillationCoordinator::verifyPrivacyBudget() const
{
    if (config_.max_rounds == 0) { return true; }
    return current_round_ <= config_.max_rounds;
}

std::optional<AggregatedLabelBatch>
FederatedDistillationCoordinator::lastBatch() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return last_batch_;
}

// ─────────────────────────────────────────────────────────────────────────────
// DI-setters
// ─────────────────────────────────────────────────────────────────────────────

void FederatedDistillationCoordinator::setStudentCallback(
    std::function<void(const AggregatedLabelBatch&)> cb)
{
    std::lock_guard<std::mutex> lk(mutex_);
    student_callback_ = std::move(cb);
}

void FederatedDistillationCoordinator::setPolicyGate(
    std::function<bool(uint64_t, size_t)> gate)
{
    std::lock_guard<std::mutex> lk(mutex_);
    policy_gate_ = std::move(gate);
}

void FederatedDistillationCoordinator::setAuditCallback(
    std::function<void(const nlohmann::json&)> cb)
{
    std::lock_guard<std::mutex> lk(mutex_);
    audit_callback_ = std::move(cb);
}

void FederatedDistillationCoordinator::setRollbackTrigger(
    std::function<void(uint64_t, const std::string&)> trigger)
{
    std::lock_guard<std::mutex> lk(mutex_);
    rollback_trigger_ = std::move(trigger);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string FederatedDistillationCoordinator::nextBatchVersion() const
{
    return "distill-v" + std::to_string(completed_rounds_ + 1);
}

} // namespace themis::distributed_knowledge
