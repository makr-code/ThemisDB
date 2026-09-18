/**
 * @file policy_approval_workflow.h
 * @brief Policy change approval workflow with state machine enforcement.
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Implements change approval state machine for governance policies:
 * DRAFT → REVIEW → APPROVED → ACTIVE
 * 
 * Supports:
 * - Explicit approval before activation
 * - Rollback of approvals (ACTIVE → DEPRECATED)
 * - Approver identity and timestamp tracking
 * - Emergency override with audit trail
 * - Multi-policy approval coordination
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <mutex>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// Forward declarations
struct PolicyRule;

enum class ApprovalState {
    DRAFT      = 0,  ///< Policy drafted, pending review
    REVIEW     = 1,  ///< Policy under review
    APPROVED   = 2,  ///< Policy approved but not active
    ACTIVE     = 3,  ///< Policy approved and active
    DEPRECATED = 4,  ///< Policy deprecated (approval rolled back)
};

enum class ApprovalAction {
    SUBMIT_FOR_REVIEW  = 0,  ///< Move from DRAFT to REVIEW
    APPROVE            = 1,  ///< Move from REVIEW to APPROVED
    ACTIVATE           = 2,  ///< Move from APPROVED to ACTIVE
    REJECT             = 3,  ///< Move from REVIEW back to DRAFT
    ROLLBACK           = 4,  ///< Move from ACTIVE to DEPRECATED
    EMERGENCY_OVERRIDE = 5,  ///< Force ACTIVE without full approval (audit trail)
};

struct ApprovalRecord {
    std::string rule_id;                      ///< Rule being approved
    std::string approver;                     ///< User who approved
    int64_t timestamp = 0;                    ///< When approval occurred
    ApprovalAction action;                    ///< What action was taken
    std::string comment;                      ///< Approver's comment
    std::string old_state;                    ///< State before action
    std::string new_state;                    ///< State after action
    bool is_emergency_override = false;       ///< Whether this was emergency override
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ApprovalRecord fromJson(const nlohmann::json& j);
};

struct ApprovalStatus {
    std::string rule_id;                      ///< Rule identifier
    ApprovalState current_state;              ///< Current approval state
    std::string current_version;              ///< Version being approved
    std::vector<ApprovalRecord> history;      ///< Full approval history
    
    std::string submitted_by;                 ///< User who submitted for review
    int64_t submitted_at = 0;                 ///< When submitted
    
    std::string approved_by;                  ///< User who approved
    int64_t approved_at = 0;                  ///< When approved
    
    std::string activated_by;                 ///< User who activated
    int64_t activated_at = 0;                 ///< When activated
    
    int required_approvers = 1;               ///< Number of required approvers
    std::vector<std::string> approvers;       ///< List of assigned approvers
    std::vector<std::string> approved_by_list;  ///< Users who have approved
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ApprovalStatus fromJson(const nlohmann::json& j);
};

class PolicyApprovalWorkflow {
public:
    PolicyApprovalWorkflow();
    
    ApprovalStatus initiateReview(
        const std::string& rule_id,
        const std::string& current_version,
        const std::string& submitted_by,
        int required_approvers = 1
    );
    
    /**
     * @brief Submit For Review.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] reviewer Input parameter.
     * @return True when the operation succeeds.
     */
    bool submitForReview(
        const std::string& rule_id,
        const std::string& reviewer
    );
    
    bool approveChange(
        const std::string& rule_id,
        const std::string& approver,
        const std::string& comment = ""
    );
    
    /**
     * @brief Reject Change.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] reviewer Input parameter.
     * @param[in] reason Input parameter.
     * @return True when the operation succeeds.
     */
    bool rejectChange(
        const std::string& rule_id,
        const std::string& reviewer,
        const std::string& reason
    );
    
    /**
     * @brief Activate Policy.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] activator Input parameter.
     * @return True when the operation succeeds.
     */
    bool activatePolicy(
        const std::string& rule_id,
        const std::string& activator
    );
    
    /**
     * @brief Rollback Approval.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] operator_user Input parameter.
     * @param[in] reason Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackApproval(
        const std::string& rule_id,
        const std::string& operator_user,
        const std::string& reason
    );
    
    bool emergencyOverride(
        const std::string& rule_id,
        const std::string& override_by,
        const std::string& reason,
        int required_approvers = 1
    );
    
    /**
     * @brief Get Approval Status.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::optional<ApprovalStatus> getApprovalStatus(const std::string& rule_id) const;
    
    /**
     * @brief Can Transition To.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_state Input parameter.
     * @return True when the operation succeeds.
     */
    bool canTransitionTo(
        const std::string& rule_id,
        ApprovalState target_state
    ) const;
    
    /**
     * @brief Get Rules In State.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getRulesInState(ApprovalState state) const;
    
    /**
     * @brief Get Pending Approvals For.
     * @param[in] approver Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getPendingApprovalsFor(const std::string& approver) const;
    
    std::vector<ApprovalRecord> queryApprovalHistory(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& approver = std::nullopt,
        const std::optional<int64_t>& start_time = std::nullopt,
        const std::optional<int64_t>& end_time = std::nullopt
    ) const;
    
    /**
     * @brief Export Workflow.
     * @return Return value.
     */
    nlohmann::json exportWorkflow() const;
    
    /**
     * @brief Import Workflow.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importWorkflow(const nlohmann::json& j);
    
    /**
     * @brief Save To File.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveToFile(const std::string& path) const;
    
    /**
     * @brief Load From File.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromFile(const std::string& path);
    
    /**
     * @brief Clear.
     */
    void clear();
    
private:
    mutable std::mutex mutex_;
    
    // Map: rule_id -> ApprovalStatus
    std::unordered_map<std::string, ApprovalStatus> approvals_;
    
    // Audit trail of all approval actions
    std::vector<ApprovalRecord> audit_trail_;
    
    /**
     * @brief Is Valid Transition.
     * @param[in] from Input parameter.
     * @param[in] to Input parameter.
     * @return True when the operation succeeds.
     */
    bool isValidTransition(ApprovalState from, ApprovalState to) const;
    
    void recordApprovalAction(
        const std::string& rule_id,
        ApprovalAction action,
        const std::string& actor,
        ApprovalState old_state,
        ApprovalState new_state,
        const std::string& comment = ""
    );
};

} // namespace governance
} // namespace themis
