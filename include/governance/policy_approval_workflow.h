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

/**
 * @brief Approval workflow states
 * 
 * Defines the lifecycle of policy changes through approval process:
 * - DRAFT: Initial state, no review yet
 * - REVIEW: Under review, awaiting approval
 * - APPROVED: Approved but not yet activated
 * - ACTIVE: Approved and activated for enforcement
 * - DEPRECATED: Approved change rolled back
 */
enum class ApprovalState {
    DRAFT      = 0,  ///< Policy drafted, pending review
    REVIEW     = 1,  ///< Policy under review
    APPROVED   = 2,  ///< Policy approved but not active
    ACTIVE     = 3,  ///< Policy approved and active
    DEPRECATED = 4,  ///< Policy deprecated (approval rolled back)
};

/**
 * @brief Approval action types
 */
enum class ApprovalAction {
    SUBMIT_FOR_REVIEW  = 0,  ///< Move from DRAFT to REVIEW
    APPROVE            = 1,  ///< Move from REVIEW to APPROVED
    ACTIVATE           = 2,  ///< Move from APPROVED to ACTIVE
    REJECT             = 3,  ///< Move from REVIEW back to DRAFT
    ROLLBACK           = 4,  ///< Move from ACTIVE to DEPRECATED
    EMERGENCY_OVERRIDE = 5,  ///< Force ACTIVE without full approval (audit trail)
};

/**
 * @brief Approval record for a single action
 */
struct ApprovalRecord {
    std::string rule_id;                      ///< Rule being approved
    std::string approver;                     ///< User who approved
    int64_t timestamp = 0;                    ///< When approval occurred
    ApprovalAction action;                    ///< What action was taken
    std::string comment;                      ///< Approver's comment
    std::string old_state;                    ///< State before action
    std::string new_state;                    ///< State after action
    bool is_emergency_override = false;       ///< Whether this was emergency override
    
    nlohmann::json toJson() const;
    static ApprovalRecord fromJson(const nlohmann::json& j);
};

/**
 * @brief Complete approval status for a policy rule
 */
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
    
    nlohmann::json toJson() const;
    static ApprovalStatus fromJson(const nlohmann::json& j);
};

/**
 * @brief Manages policy approval workflow and change governance
 * 
 * Enforces approval state machine ensuring:
 * - All policy changes go through review process
 * - Multiple approval required for sensitive policies
 * - Complete audit trail of all approvals
 * - Emergency override capability with logging
 */
class PolicyApprovalWorkflow {
public:
    PolicyApprovalWorkflow();
    
    /// Create new approval request for a policy change
    /// @param rule_id Rule identifier
    /// @param current_version Version being reviewed
    /// @param submitted_by User submitting for review
    /// @param required_approvers Number of approvals needed
    /// @return Approval status in DRAFT state
    ApprovalStatus initiateReview(
        const std::string& rule_id,
        const std::string& current_version,
        const std::string& submitted_by,
        int required_approvers = 1
    );
    
    /// Submit policy for review
    /// @param rule_id Rule identifier
    /// @param reviewer Assigning reviewer
    /// @return True if successful
    bool submitForReview(
        const std::string& rule_id,
        const std::string& reviewer
    );
    
    /// Approve a policy change
    /// @param rule_id Rule identifier
    /// @param approver User approving the change
    /// @param comment Optional approval comment
    /// @return True if successful
    bool approveChange(
        const std::string& rule_id,
        const std::string& approver,
        const std::string& comment = ""
    );
    
    /// Reject a policy change, move back to DRAFT
    /// @param rule_id Rule identifier
    /// @param reviewer User rejecting
    /// @param reason Rejection reason
    /// @return True if successful
    bool rejectChange(
        const std::string& rule_id,
        const std::string& reviewer,
        const std::string& reason
    );
    
    /// Activate an approved policy
    /// @param rule_id Rule identifier
    /// @param activator User activating the policy
    /// @return True if successful
    bool activatePolicy(
        const std::string& rule_id,
        const std::string& activator
    );
    
    /// Rollback an active policy to deprecated state
    /// @param rule_id Rule identifier
    /// @param operator_user User performing rollback
    /// @param reason Rollback reason
    /// @return True if successful
    bool rollbackApproval(
        const std::string& rule_id,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /// Emergency override to activate policy without full approval
    /// @param rule_id Rule identifier
    /// @param override_by User authorizing override
    /// @param reason Reason for emergency override
    /// @param required_approvers Update required approvers count
    /// @return True if successful
    bool emergencyOverride(
        const std::string& rule_id,
        const std::string& override_by,
        const std::string& reason,
        int required_approvers = 1
    );
    
    /// Get approval status for a rule
    /// @param rule_id Rule identifier
    /// @return Approval status if found
    std::optional<ApprovalStatus> getApprovalStatus(const std::string& rule_id) const;
    
    /// Check if rule can be transitioned to target state
    /// @param rule_id Rule identifier
    /// @param target_state Desired target state
    /// @return True if transition is allowed
    bool canTransitionTo(
        const std::string& rule_id,
        ApprovalState target_state
    ) const;
    
    /// Get all rules in a specific approval state
    /// @param state State to query
    /// @return Vector of rule IDs in that state
    std::vector<std::string> getRulesInState(ApprovalState state) const;
    
    /// Get pending approvals for a specific approver
    /// @param approver Approver identifier
    /// @return Vector of rule IDs pending approval
    std::vector<std::string> getPendingApprovalsFor(const std::string& approver) const;
    
    /// Query approval history
    /// @param rule_id Optional rule ID filter
    /// @param approver Optional approver filter
    /// @param start_time Optional start time filter
    /// @param end_time Optional end time filter
    /// @return Filtered approval records
    std::vector<ApprovalRecord> queryApprovalHistory(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& approver = std::nullopt,
        const std::optional<int64_t>& start_time = std::nullopt,
        const std::optional<int64_t>& end_time = std::nullopt
    ) const;
    
    /// Export approval workflow as JSON
    nlohmann::json exportWorkflow() const;
    
    /// Import approval workflow from JSON
    bool importWorkflow(const nlohmann::json& j);
    
    /// Save approval workflow to file
    bool saveToFile(const std::string& path) const;
    
    /// Load approval workflow from file
    bool loadFromFile(const std::string& path);
    
    /// Clear all approval records (for testing/cleanup)
    void clear();
    
private:
    mutable std::mutex mutex_;
    
    // Map: rule_id -> ApprovalStatus
    std::unordered_map<std::string, ApprovalStatus> approvals_;
    
    // Audit trail of all approval actions
    std::vector<ApprovalRecord> audit_trail_;
    
    /// Validate state transition
    bool isValidTransition(ApprovalState from, ApprovalState to) const;
    
    /// Record approval action in audit trail
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
