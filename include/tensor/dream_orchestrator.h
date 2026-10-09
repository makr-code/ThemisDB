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
     * @brief Applies a lifecycle transition within the central R0..R3 matrix.
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
    std::vector<DreamRunAuditEvent> audit_events_;
    mutable std::mutex mutex_;
    std::uint64_t next_run_number_ = 0;
};

} // namespace tensor
} // namespace themis
