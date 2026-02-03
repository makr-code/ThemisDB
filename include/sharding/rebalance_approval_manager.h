#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>

namespace themis {
namespace sharding {

// Forward declarations
class RebalanceOperation;

/**
 * Approval mode for rebalance operations
 */
enum class ApprovalMode {
    AUTO_APPROVE,      // System can execute without human approval
    MANUAL_APPROVE,    // Require human approval before execution
    DRY_RUN_ONLY       // Only allow simulation
};

/**
 * Approval decision
 */
enum class ApprovalDecision {
    APPROVED,
    REJECTED
};

/**
 * Approval record for audit trail
 */
struct ApprovalRecord {
    std::string operation_id;
    std::string approver_id;
    ApprovalDecision decision;
    std::string reason;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * RebalanceApprovalManager
 * 
 * Manages approval workflow for rebalance operations.
 * Supports:
 * - Automatic approval for low-risk operations
 * - Manual approval workflow with audit trail
 * - Dry-run mode for testing
 */
class RebalanceApprovalManager {
public:
    using ApprovalNotificationCallback = std::function<void(const std::string& operation_id)>;
    
    explicit RebalanceApprovalManager(ApprovalMode mode = ApprovalMode::MANUAL_APPROVE);
    
    /**
     * Set approval mode
     */
    void setApprovalMode(ApprovalMode mode);
    
    /**
     * Get current approval mode
     */
    ApprovalMode getApprovalMode() const;
    
    /**
     * Request approval for operation
     * @param operation_id Operation identifier
     * @param estimated_risk Risk level (0.0 - 1.0)
     * @param description Operation description
     * @return true if auto-approved, false if pending manual approval
     */
    bool requestApproval(
        const std::string& operation_id,
        double estimated_risk,
        const std::string& description
    );
    
    /**
     * Approve operation
     * @param operation_id Operation identifier
     * @param approver_id Approver identifier
     * @param reason Approval reason
     * @return true if approved successfully
     */
    bool approve(
        const std::string& operation_id,
        const std::string& approver_id,
        const std::string& reason
    );
    
    /**
     * Reject operation
     * @param operation_id Operation identifier
     * @param rejector_id Rejector identifier
     * @param reason Rejection reason
     * @return true if rejected successfully
     */
    bool reject(
        const std::string& operation_id,
        const std::string& rejector_id,
        const std::string& reason
    );
    
    /**
     * Check if operation is approved
     * @param operation_id Operation identifier
     * @return true if approved
     */
    bool isApproved(const std::string& operation_id) const;
    
    /**
     * Get approval history for operation
     * @param operation_id Operation identifier
     * @return Vector of approval records
     */
    std::vector<ApprovalRecord> getApprovalHistory(const std::string& operation_id) const;
    
    /**
     * Get all pending approvals
     * @return Vector of operation IDs pending approval
     */
    std::vector<std::string> getPendingApprovals() const;
    
    /**
     * Set callback for approval notifications
     */
    void setApprovalNotificationCallback(ApprovalNotificationCallback callback);
    
private:
    ApprovalMode mode_;
    std::map<std::string, std::vector<ApprovalRecord>> approval_records_;
    std::map<std::string, bool> approval_status_;  // operation_id -> approved
    mutable std::mutex mutex_;
    ApprovalNotificationCallback notification_callback_;
    
    // Auto-approval threshold (risk level)
    double auto_approval_threshold_{0.3};  // Auto-approve if risk < 30%
};

} // namespace sharding
} // namespace themis
