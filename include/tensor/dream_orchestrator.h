#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

/**
 * @brief Canonical run states for the deterministic Dream-Mode lifecycle.
 *
 * The machine-dreaming lifecycle is intentionally constrained to a small matrix:
 * R0 -> R1 -> R2 -> R3 -> R0. Any other transition is rejected fail-closed.
 */
enum class DreamRunState : std::uint8_t {
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
};

/**
 * @brief Explicit audit reason groups for failed or aborted Dream-Mode runs.
 */
enum class DreamRunAuditReasonClass : std::uint8_t {
    none = 0,
    invalid_mode,
    invalid_transition,
    missing_policy_version,
    missing_dataset_ref,
    missing_seed_set,
    generic_failure,
    abort,
    gate_rejected,
};

/**
 * @brief Shared status contract for Dream-Mode validation gates.
 */
enum class DreamGateStatus : std::uint8_t {
    pass = 0, ///< Required evidence is complete and satisfies the gate.
    hold,     ///< Evidence is missing or requires review.
    fail,     ///< Evidence violates the gate.
};

/**
 * @brief Evidence returned by evaluation, ethics, and security checks.
 *
 * Missing evidence is held rather than treated as a successful check.
 */
struct DreamValidationEvidence {
    bool isolation_check_completed = false; ///< Whether the firewall check ran.
    bool truth_path_firewall_passed = false; ///< False means a containment failure.
    bool label_integrity_check_completed = false; ///< Whether labels were checked.
    bool provenance_labels_valid = false; ///< Both synthetic provenance tags are intact.
    std::optional<double> utility_delta_recall_at_10; ///< Candidate delta vs baseline.
    std::optional<double> utility_delta_ndcg_at_10; ///< Candidate delta vs baseline.
    std::optional<double> online_hallucination_delta; ///< Positive values are regressions.
    std::optional<double> online_error_delta; ///< Positive values are regressions.
    bool ethics_check_completed = false; ///< Whether the ethics boundary check ran.
    bool ethics_veto = false; ///< A veto fails the ethics gate.
    std::string ethics_reason; ///< Veto reason; a default is emitted if empty.
    bool security_check_completed = false; ///< Whether leakage checks completed.
    bool security_veto = false; ///< Unresolved veto holds promotion.
    bool security_leakage_detected = false; ///< Confirmed leakage fails isolation.
    std::string security_reason; ///< Veto or leakage reason; a default is emitted if empty.
};

/**
 * @brief One normalized Dream-Mode gate outcome.
 */
struct DreamGateOutcome {
    DreamGateStatus status = DreamGateStatus::hold; ///< Normalized shared status.
    std::string reason; ///< Human-readable pass, hold, or failure reason.
};

/**
 * @brief Validation report combining all five BRAIN-DREAM gates.
 *
 * Security leakage is represented as a separate safety outcome and also blocks
 * the isolation gate; the five-gate `gates` object remains schema-compatible.
 */
struct DreamGateReport {
    std::string run_id; ///< Identifier of the persisted Dream run.
    DreamGateOutcome isolation; ///< Truth-path isolation outcome.
    DreamGateOutcome label_integrity; ///< Synthetic provenance outcome.
    DreamGateOutcome utility; ///< Retrieval utility outcome.
    DreamGateOutcome nonregression; ///< Online hallucination outcome.
    DreamGateOutcome ethics_boundary; ///< Ethics veto outcome.
    DreamGateOutcome security_leakage; ///< Security check outcome.
    bool ethics_veto_triggered = false; ///< Whether the ethics checker vetoed.
    bool security_veto_triggered = false; ///< Whether security left a veto unresolved.
    bool security_leakage_detected = false; ///< Whether leakage was confirmed.
    std::optional<double> utility_delta_recall_at_10; ///< Recall delta or unavailable.
    std::optional<double> utility_delta_ndcg_at_10; ///< nDCG delta or unavailable.
    std::optional<double> online_hallucination_delta; ///< Hallucination delta or unavailable.
    std::optional<double> online_error_delta; ///< Error-rate delta or unavailable.
    DreamGateStatus status = DreamGateStatus::hold; ///< Aggregate of all gate outcomes.
    std::string recommendation = "research_only"; ///< no_go, research_only, or candidate.
    std::string rationale; ///< First blocking reason or pass rationale.

    /**
     * @brief Serializes the normalized report, including reasons and metrics.
     * @return JSON report whose `run_id` identifies the validated run; unavailable
     *         metrics are null and all five gate statuses are present.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief Returns true only when every required validation and security gate passes.
     * @return True when a promotion-candidate recommendation is permitted.
     */
    [[nodiscard]] bool permitsPromotionCandidate() const noexcept;
};

/**
 * @brief Persisted descriptor for a single Dream-Mode run.
 */
struct DreamRunDescriptor {
    std::string run_id;
    std::string created_at_utc;
    std::string updated_at_utc;
    std::string mode = "dream_research";
    DreamRunState state = DreamRunState::R0;
    std::string policy_version;
    std::string dataset_ref;
    std::vector<std::int64_t> seed_set;
    bool synthetic = true;
    bool dream_mode = true;

    /**
     * @brief Serializes the descriptor into a JSON document for persistence/audit.
     */
    [[nodiscard]] nlohmann::json toJson() const;
};

/**
 * @brief Audit record for a lifecycle event or denied transition.
 */
struct DreamRunAuditEvent {
    std::string run_id;
    std::string timestamp_utc;
    DreamRunState state = DreamRunState::R0;
    DreamRunAuditReasonClass reason_class = DreamRunAuditReasonClass::none;
    std::string reason;
    std::string phase;

    [[nodiscard]] nlohmann::json toJson() const;
};

/**
 * @brief Orchestrates Dream-Mode lifecycle creation, state transitions, and audit logging.
 *
 * The implementation enforces a single central transition matrix so helper layers
 * cannot silently bypass the R0..R3 contract.
 */
class DreamOrchestrator {
public:
    DreamOrchestrator();

    /**
     * @brief Returns true when the requested transition is part of the canonical matrix.
     */
    [[nodiscard]] static bool isAllowedTransition(DreamRunState from_state,
                                                 DreamRunState to_state) noexcept;

    /**
     * @brief Converts a run state to an explicit and stable string.
     */
    [[nodiscard]] static std::string toString(DreamRunState state) noexcept;

    /**
     * @brief Converts reason class to a stable audit label.
     */
    [[nodiscard]] static std::string reasonClassToString(
        DreamRunAuditReasonClass reason_class) noexcept;

    /**
     * @brief Converts the shared gate status to its stable schema spelling.
     * @return `pass`, `hold`, or `fail`; unknown enum values fail closed as `fail`.
     */
    [[nodiscard]] static std::string gateStatusToString(DreamGateStatus status) noexcept;

    /**
     * @brief Creates a new Dream run and fails closed when the mode is not `dream_research`.
     *
     * @param mode Runtime mode. Must be exactly `dream_research`.
     * @param policy_version Policy version string attached to the descriptor.
     * @param dataset_ref Dataset snapshot reference.
     * @param seed_set Seed set used for deterministic replay.
     * @param error Optional text reason returned on rejection.
     * @param reason_class Optional explicit failure class returned on rejection.
     * @return Persisted descriptor on success; std::nullopt on rejection.
     */
    [[nodiscard]] std::optional<DreamRunDescriptor> StartDreamRun(
        const std::string& mode,
        const std::string& policy_version,
        const std::string& dataset_ref,
        const std::vector<std::int64_t>& seed_set,
        std::string* error = nullptr,
        DreamRunAuditReasonClass* reason_class = nullptr);

    /**
     * @brief Backwards-compatible alias for StartDreamRun(); matches issue wording.
     */
    [[nodiscard]] std::optional<DreamRunDescriptor> startRun(
        const std::string& mode,
        const std::string& policy_version,
        const std::string& dataset_ref,
        const std::vector<std::int64_t>& seed_set,
        std::string* error = nullptr,
        DreamRunAuditReasonClass* reason_class = nullptr) {
        return StartDreamRun(mode, policy_version, dataset_ref, seed_set, error, reason_class);
    }

    /**
     * @brief Applies a lifecycle transition only when its required gates pass.
     * @param run_id Existing Dream run identifier.
     * @param next_state Requested lifecycle state.
     * @param detail Optional error or rejection explanation.
     * @param reason_class Optional failure classification.
     * @return True when the state changes; false for invalid transitions, missing
     *         reports, or non-passing required gates.
     * @note R0->R1, R1->R2, and R2->R3 require all five gates and the security
     *       outcome to pass. Reaching R3 records lifecycle state only; it does
     *       not authorize or execute promotion transfer.
     */
    [[nodiscard]] bool transitionRun(const std::string& run_id,
                                    DreamRunState next_state,
                                    std::string* detail = nullptr,
                                    DreamRunAuditReasonClass* reason_class = nullptr);

    /**
     * @brief Records a run abort in the audit log and resets to the safe baseline state.
     */
    [[nodiscard]] bool abortRun(const std::string& run_id,
                                const std::string& reason);

    /**
     * @brief Records an explicit failure reason and preserves the audit trail.
     */
    [[nodiscard]] bool failRun(const std::string& run_id,
                               const std::string& reason,
                               DreamRunAuditReasonClass reason_class =
                                   DreamRunAuditReasonClass::generic_failure);

    /**
     * @brief Aggregates module-produced evidence and stores its report by run ID.
     * @param run_id Existing Dream run receiving the validation result.
     * @param evidence Completed evaluation, ethics, and security evidence; absent
     *        measurements remain on hold.
     * @return Stored report, or std::nullopt when run_id is unknown.
     * @note Thread-safe. The report is retained in this orchestrator's in-memory
     *       store for the lifetime of the instance.
     */
    [[nodiscard]] std::optional<DreamGateReport> validateRun(
        const std::string& run_id,
        const DreamValidationEvidence& evidence);

    /**
     * @brief Retrieves the most recently stored validation report for a run.
     * @param run_id Existing Dream run identifier.
     * @return Stored report, or std::nullopt when no report exists.
     * @note Thread-safe; reports do not survive orchestrator destruction.
     */
    [[nodiscard]] std::optional<DreamGateReport> getGateReportByRunId(
        const std::string& run_id) const;

    /**
     * @brief Returns a persisted descriptor keyed by run_id.
     */
    [[nodiscard]] std::optional<DreamRunDescriptor> getRunById(
        const std::string& run_id) const;

    /**
     * @brief Returns the full audit trail for start, abort, and failure events.
     */
    [[nodiscard]] std::vector<DreamRunAuditEvent> auditLog() const;

    /**
     * @brief Serializes a run descriptor into a canonical JSON packet.
     */
    [[nodiscard]] std::string serializeRunDescriptor(
        const DreamRunDescriptor& descriptor) const;

private:
    void recordAuditLocked(const std::string& run_id,
                           DreamRunState state,
                           const std::string& phase,
                           const std::string& reason,
                           DreamRunAuditReasonClass reason_class);

    void updateDescriptorTimestampLocked(std::string& run_id);

    std::map<std::string, DreamRunDescriptor> runs_;
    std::map<std::string, DreamGateReport> gate_reports_;
    std::vector<DreamRunAuditEvent> audit_events_;
    mutable std::mutex mutex_;
    std::uint64_t next_run_number_ = 0;
};

} // namespace tensor
} // namespace themis
