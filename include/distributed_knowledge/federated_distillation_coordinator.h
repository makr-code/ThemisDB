#pragma once

/**
 * @file federated_distillation_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "distributed_knowledge/distributed_knowledge_api_contract.h"

namespace themis {
namespace distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// SoftLabel — temperature-scaled probability distribution from teacher model
// ─────────────────────────────────────────────────────────────────────────────

struct SoftLabel {
    std::string              query_id;       ///< Opaque identifier (e.g. hash of query)
    std::vector<double>      probabilities;  ///< Temperature-scaled softmax distribution
    double                   temperature;    ///< Distillation temperature T (1.0 = argmax)
    std::string              teacher_id;     ///< Opaque teacher model identifier

    [[nodiscard]] nlohmann::json toJson() const {
        return {{"query_id",      query_id},
                {"probabilities", probabilities},
                {"temperature",   temperature},
                {"teacher_id",    teacher_id}};
    }

    [[nodiscard]] static SoftLabel fromJson(const nlohmann::json& j) {
        SoftLabel s;
        s.query_id     = j.value("query_id", "");
        s.probabilities = j.value("probabilities", std::vector<double>{});
        s.temperature  = j.value("temperature", 1.0);
        s.teacher_id   = j.value("teacher_id", "");
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DistillationRound — coordinator-signed batch of soft labels for one round
// ─────────────────────────────────────────────────────────────────────────────

struct DistillationRound {
    uint64_t                 round = 0;          ///< Monotonic round counter
    std::string              teacher_id;     ///< Teacher model identifier
    std::vector<SoftLabel>   labels;         ///< DP-protected soft labels
    double                   epsilon_spent;  ///< DP budget spent this round
    size_t                   label_count;    ///< Number of soft labels
    bool                     dp_applied;     ///< Whether DP noise was applied

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json js_labels = nlohmann::json::array();
        for (const auto& l : labels) {
          js_labels.push_back(l.toJson());
        }
        return {{"round",         round},
                {"teacher_id",    teacher_id},
                {"labels",        js_labels},
                {"epsilon_spent", epsilon_spent},
                {"label_count",   label_count},
                {"dp_applied",    dp_applied}};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DistillationModelCard — governance snapshot per coordinator lifecycle
// ─────────────────────────────────────────────────────────────────────────────

struct DistillationModelCard {
    // Identity
    std::string coordinator_id;     ///< Coordinator instance identifier (optional)
    std::string teacher_id;         ///< Last teacher model identifier (or empty)

    // Privacy accounting
    uint64_t    rounds_completed;   ///< Total rounds broadcast
    double      total_epsilon;      ///< DP budget spent (sum over all rounds)
    double      dp_epsilon_per_round; ///< Configured ε per round
    double      dp_delta;           ///< Configured δ
    bool        dp_applied;         ///< Whether DP noise was applied in this session

    // Utility
    double      min_utility_reported; ///< Lowest utility reported by any student
    double      max_utility_reported; ///< Highest utility reported (1.0 = no reports)
    size_t      rollback_count;     ///< Number of rollbacks triggered

    // Policy
    size_t      policy_blocks;      ///< Broadcasts blocked by policy gate
    size_t      registered_students; ///< Number of registered student callbacks

    [[nodiscard]] nlohmann::json toJson() const {
        return {{"coordinator_id",      coordinator_id},
                {"teacher_id",         teacher_id},
                {"rounds_completed",   rounds_completed},
                {"total_epsilon",      total_epsilon},
                {"dp_epsilon_per_round", dp_epsilon_per_round},
                {"dp_delta",           dp_delta},
                {"dp_applied",         dp_applied},
                {"min_utility_reported", min_utility_reported},
                {"max_utility_reported", max_utility_reported},
                {"rollback_count",     rollback_count},
                {"policy_blocks",      policy_blocks},
                {"registered_students", registered_students}};
    }
};



struct DistillationConfig {
    // DP parameters (Gaussian mechanism)
    double dp_epsilon   = 0.5;    ///< DP ε per round (lower = more private)
    double dp_delta     = 1e-5;   ///< DP failure probability δ
    double dp_sensitivity = 0.1;  ///< L2 sensitivity of probability vectors

    // Distillation hyper-parameters
    double temperature  = 4.0;    ///< Default distillation temperature
    double alpha        = 0.5;    ///< Blend weight (0=hard labels, 1=soft labels only)

    // Budget control
    size_t max_rounds   = 0;      ///< 0 = unlimited

    // Utility gate — trigger rollback if student utility drops below this
    double min_utility_threshold = 0.90; ///< Fraction of teacher utility required

    // Require DP noise before broadcasting
    bool require_dp     = true;

    [[nodiscard]] bool isValid() const {
        return dp_epsilon > 0.0 && dp_epsilon <= 1.0 &&
               dp_delta   > 0.0 && dp_delta < 0.1 &&
               dp_sensitivity > 0.0 &&
               temperature >= 1.0 &&
               alpha >= 0.0 && alpha <= 1.0 &&
               min_utility_threshold > 0.0 && min_utility_threshold <= 1.0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// IFederatedDistillationCoordinator — public interface
// ─────────────────────────────────────────────────────────────────────────────

class IFederatedDistillationCoordinator {
public:
    /**
     * @brief IFederated Distillation Coordinator.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    virtual ~IFederatedDistillationCoordinator() noexcept = default;

    /**
     * @brief Submit Soft Labels.
     * @param[in] teacher_id Identifier of the teacher.
     * @param[in] labels Input parameter.
     */
    virtual void submitSoftLabels(const std::string& teacher_id,
                                  std::vector<SoftLabel> labels) = 0;

    /**
     * @brief Broadcast To Students.
     * @return Return value.
     */
    virtual DistillationRound broadcastToStudents() = 0;

    [[nodiscard]] virtual uint64_t currentRound() const = 0;

    [[nodiscard]] virtual size_t submittedCount() const = 0;

    [[nodiscard]] virtual std::optional<DistillationRound> lastRound() const = 0;

    [[nodiscard]] virtual nlohmann::json getStats() const = 0;

    virtual void registerStudent(
        const std::string& student_id,
        std::function<void(const DistillationRound&)> cb) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// FederatedDistillationCoordinator — production implementation
// ─────────────────────────────────────────────────────────────────────────────

class FederatedDistillationCoordinator : public IFederatedDistillationCoordinator {
public:
    using PolicyGate =
        std::function<bool(uint64_t round, const std::string& teacher_id)>;

    explicit FederatedDistillationCoordinator(DistillationConfig cfg = {});
    ~FederatedDistillationCoordinator() noexcept override;

    FederatedDistillationCoordinator(const FederatedDistillationCoordinator&)            = delete;
    FederatedDistillationCoordinator& operator=(const FederatedDistillationCoordinator&) = delete;
    FederatedDistillationCoordinator(FederatedDistillationCoordinator&&)                 noexcept = default;
    FederatedDistillationCoordinator& operator=(FederatedDistillationCoordinator&&)      noexcept;

    // ── IFederatedDistillationCoordinator ─────────────────────────────────────

    void submitSoftLabels(const std::string& teacher_id,
                          std::vector<SoftLabel> labels) override;

    DistillationRound broadcastToStudents() override;

    [[nodiscard]] uint64_t                        currentRound()   const override;
    [[nodiscard]] size_t                          submittedCount() const override;
    [[nodiscard]] std::optional<DistillationRound> lastRound()     const override;
    [[nodiscard]] nlohmann::json                  getStats()       const override;

    void registerStudent(const std::string& student_id,
                         std::function<void(const DistillationRound&)> cb) override;

    // ── DI setters ─────────────────────────────────────────────────────────────

    using NoiseGeneratorFn =
        std::function<void(std::vector<SoftLabel>& labels, double sigma)>;

    /**
     * @brief Set Noise Generator Fn.
     * @param[in] fn Input parameter.
     */
    void setNoiseGeneratorFn(NoiseGeneratorFn fn);

    /**
     * @brief Set Policy Gate.
     * @param[in] gate Input parameter.
     */
    void setPolicyGate(PolicyGate gate);

    /**
     * @brief Set Bounded Policy.
     * @param[in] policy Input parameter.
     */
    void setBoundedPolicy(DistillationBoundedPolicy policy);

    void setAuditCallback(std::function<void(const nlohmann::json&)> cb);

    void setRollbackTrigger(std::function<void(uint64_t, double)> cb);

    /**
     * @brief ── Utility reporting ──────────────────────────────────────────────────────
     * @param[in] student_id Identifier of the student.
     * @param[in] utility Input parameter.
     */

    void reportStudentUtility(const std::string& student_id, double utility);

    // ── Budget observability ───────────────────────────────────────────────────

    [[nodiscard]] double privacyBudgetRemaining() const;

    [[nodiscard]] bool verifyPrivacyBudget() const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

    [[nodiscard]] DistillationModelCard generateModelCard(
        const std::string& coordinator_id = "") const;

    [[nodiscard]] const DistillationConfig& config() const { return config_; }

private:
    /**
     * @brief ── Implementation ─────────────────────────────────────────────────────────
     * @param[in,out] labels Input/output parameter.
     */

    void applyDPNoise(std::vector<SoftLabel>& labels) const;

    DistillationConfig config_;

    mutable std::mutex                         mutex_;
    uint64_t                                   current_round_  = 0;
    double                                     total_epsilon_spent_ = 0.0;
    size_t                                     broadcast_count_ = 0;
    size_t                                     rollback_count_  = 0;
    size_t                                     policy_block_count_ = 0;
    double                                     min_utility_reported_ = 1.0;
    double                                     max_utility_reported_ = 1.0;
    bool                                       any_utility_reported_ = false;

    // Pending submission for current round
    std::string                                pending_teacher_id_;
    std::vector<SoftLabel>                     pending_labels_;
    bool                                       has_pending_ = false;

    std::optional<DistillationRound>           last_round_;

    // Registered students
    std::vector<std::pair<std::string,
        std::function<void(const DistillationRound&)>>>   students_;

    // Callbacks
    PolicyGate                                 policy_gate_;
    DistillationBoundedPolicy                  bounded_policy_;
    std::function<void(const nlohmann::json&)> audit_cb_;
    std::function<void(uint64_t, double)>      rollback_trigger_;
    NoiseGeneratorFn                           noise_generator_fn_;
};

} // namespace distributed_knowledge
} // namespace themis
