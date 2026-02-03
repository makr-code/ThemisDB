#include "sharding/rebalance_approval_manager.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {
namespace sharding {

RebalanceApprovalManager::RebalanceApprovalManager(ApprovalMode mode)
    : mode_(mode) {
    THEMIS_INFO("RebalanceApprovalManager initialized with mode={}",
                mode_ == ApprovalMode::AUTO_APPROVE ? "AUTO" :
                mode_ == ApprovalMode::MANUAL_APPROVE ? "MANUAL" : "DRY_RUN");
}

void RebalanceApprovalManager::setApprovalMode(ApprovalMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    mode_ = mode;
    THEMIS_INFO("Approval mode changed to {}",
                mode_ == ApprovalMode::AUTO_APPROVE ? "AUTO" :
                mode_ == ApprovalMode::MANUAL_APPROVE ? "MANUAL" : "DRY_RUN");
}

ApprovalMode RebalanceApprovalManager::getApprovalMode() const {
    return mode_;
}

bool RebalanceApprovalManager::requestApproval(
    const std::string& operation_id,
    double estimated_risk,
    const std::string& description) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    THEMIS_INFO("Approval requested for operation {}: risk={:.2f}, desc={}",
                operation_id, estimated_risk, description);
    
    // Handle based on mode
    switch (mode_) {
        case ApprovalMode::DRY_RUN_ONLY:
            THEMIS_INFO("DRY_RUN mode: Operation {} would be queued for approval", operation_id);
            approval_status_[operation_id] = false;
            return false;
            
        case ApprovalMode::AUTO_APPROVE:
            // Auto-approve if risk is below threshold
            if (estimated_risk < auto_approval_threshold_) {
                ApprovalRecord record;
                record.operation_id = operation_id;
                record.approver_id = "system";
                record.decision = ApprovalDecision::APPROVED;
                record.reason = "Auto-approved (low risk)";
                record.timestamp = std::chrono::system_clock::now();
                
                approval_records_[operation_id].push_back(record);
                approval_status_[operation_id] = true;
                
                THEMIS_INFO("Operation {} auto-approved (risk={:.2f} < threshold={:.2f})",
                           operation_id, estimated_risk, auto_approval_threshold_);
                
                // Trigger notification
                if (notification_callback_) {
                    notification_callback_(operation_id);
                }
                
                return true;
            } else {
                // Queue for manual approval even in auto mode if risk is high
                THEMIS_WARN("Operation {} requires manual approval (risk={:.2f} >= threshold={:.2f})",
                           operation_id, estimated_risk, auto_approval_threshold_);
                approval_status_[operation_id] = false;
                return false;
            }
            
        case ApprovalMode::MANUAL_APPROVE:
        default:
            // Queue for manual approval
            approval_status_[operation_id] = false;
            THEMIS_INFO("Operation {} queued for manual approval", operation_id);
            return false;
    }
}

bool RebalanceApprovalManager::approve(
    const std::string& operation_id,
    const std::string& approver_id,
    const std::string& reason) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if operation exists
    if (approval_status_.find(operation_id) == approval_status_.end()) {
        THEMIS_WARN("Operation not found for approval: {}", operation_id);
        return false;
    }
    
    // Check if already approved
    if (approval_status_[operation_id]) {
        THEMIS_WARN("Operation {} already approved", operation_id);
        return true;
    }
    
    // Create approval record
    ApprovalRecord record;
    record.operation_id = operation_id;
    record.approver_id = approver_id;
    record.decision = ApprovalDecision::APPROVED;
    record.reason = reason;
    record.timestamp = std::chrono::system_clock::now();
    
    approval_records_[operation_id].push_back(record);
    approval_status_[operation_id] = true;
    
    THEMIS_INFO("Operation {} approved by {} (reason: {})",
                operation_id, approver_id, reason);
    
    // Trigger notification
    if (notification_callback_) {
        notification_callback_(operation_id);
    }
    
    return true;
}

bool RebalanceApprovalManager::reject(
    const std::string& operation_id,
    const std::string& rejector_id,
    const std::string& reason) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if operation exists
    if (approval_status_.find(operation_id) == approval_status_.end()) {
        THEMIS_WARN("Operation not found for rejection: {}", operation_id);
        return false;
    }
    
    // Create rejection record
    ApprovalRecord record;
    record.operation_id = operation_id;
    record.approver_id = rejector_id;
    record.decision = ApprovalDecision::REJECTED;
    record.reason = reason;
    record.timestamp = std::chrono::system_clock::now();
    
    approval_records_[operation_id].push_back(record);
    approval_status_[operation_id] = false;
    
    THEMIS_INFO("Operation {} rejected by {} (reason: {})",
                operation_id, rejector_id, reason);
    
    return true;
}

bool RebalanceApprovalManager::isApproved(const std::string& operation_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approval_status_.find(operation_id);
    if (it != approval_status_.end()) {
        return it->second;
    }
    
    return false;
}

std::vector<ApprovalRecord> RebalanceApprovalManager::getApprovalHistory(
    const std::string& operation_id) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approval_records_.find(operation_id);
    if (it != approval_records_.end()) {
        return it->second;
    }
    
    return {};
}

std::vector<std::string> RebalanceApprovalManager::getPendingApprovals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> pending;
    
    for (const auto& [op_id, approved] : approval_status_) {
        if (!approved) {
            pending.push_back(op_id);
        }
    }
    
    return pending;
}

void RebalanceApprovalManager::setApprovalNotificationCallback(ApprovalNotificationCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    notification_callback_ = callback;
}

} // namespace sharding
} // namespace themis
