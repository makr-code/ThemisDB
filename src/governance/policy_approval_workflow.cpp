/**
 * @file policy_approval_workflow.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @date 2026-08-18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Policy approval workflow with state machine enforcement
 */

#include "governance/policy_approval_workflow.h"

#include <algorithm>
#include <chrono>
#include <uuid.h>
#include <fmt/format.h>

namespace themis {
namespace governance {

// ========== ApprovalRecord Implementation ==========

nlohmann::json ApprovalRecord::toJson() const {
    nlohmann::json j;
    j["rule_id"]                   = rule_id;
    j["approver"]                  = approver;
    j["timestamp"]                 = timestamp;
    j["action"]                    = static_cast<int>(action);
    j["comment"]                   = comment;
    j["old_state"]                 = old_state;
    j["new_state"]                 = new_state;
    j["is_emergency_override"]     = is_emergency_override;
    return j;
}

ApprovalRecord ApprovalRecord::fromJson(const nlohmann::json& j) {
    ApprovalRecord r;
    if (j.contains("rule_id")) {
        r.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("approver")) {
        r.approver = j["approver"].get<std::string>();
    }
    if (j.contains("timestamp")) {
        r.timestamp = j["timestamp"].get<int64_t>();
    }
    if (j.contains("action")) {
        r.action = static_cast<ApprovalAction>(j["action"].get<int>());
    }
    if (j.contains("comment")) {
        r.comment = j["comment"].get<std::string>();
    }
    if (j.contains("old_state")) {
        r.old_state = j["old_state"].get<std::string>();
    }
    if (j.contains("new_state")) {
        r.new_state = j["new_state"].get<std::string>();
    }
    if (j.contain[[maybe_unused]] s("is_emergency_overrid[[maybe_unused]] e")) {
        r.is_emergency_override = j["is_emergency_override"].get<bool>();
    }
    return r;
}

// ========== ApprovalStatus Implementation ==========

nlohmann::json ApprovalStatus::toJson() const {
    nlohmann::json j;
    j["rule_id"]              = rule_id;
    j["current_state"]        = static_cast<int>(current_state);
    j["current_version"]      = current_version;
    j["submitted_by"]         = submitted_by;
    j["submitted_at"]         = submitted_at;
    j["approved_by"]          = approved_by;
    j["approved_at"]          = approved_at;
    j["activated_by"]         = activated_by;
    j["activated_at"]         = activated_at;
    j["required_approvers"]   = required_approvers;
    j["approvers"]            = approvers;
    j["approved_by_list"]     = approved_by_list;
    
    j["history"] = nlohmann::json::array();
    for (const auto& record : history) {
        j["history"].push_back(record.toJson());
    }
    return j;
}

ApprovalStatus ApprovalStatus::fromJson(const nlohmann::json& j) {
    ApprovalStatus s;
    if (j.contains("rule_id")) {
        s.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("current_state")) {
        s.current_state = static_cast<ApprovalState>(j["current_state"].get<int>());
    }
    if (j.contains("current_version")) {
        s.current_version = j["current_version"].get<std::string>();
    }
    if (j.contains("submitted_by")) {
        s.submitted_by = j["submitted_by"].get<std::string>();
    }
    if (j.contains("submitted_at")) {
        s.submitted_at = j["submitted_at"].get<int64_t>();
    }
    if (j.contains("approved_by")) {
        s.approved_by = j["approved_by"].get<std::string>();
    }
    if (j.contains("approved_at")) {
        s.approved_at = j["approved_at"].get<int64_t>();
    }
    if (j.contains("activated_by")) {
        s.activated_by = j["activated_by"].get<std::string>();
    }
    if (j.contains("activated_at")) {
        s.activated_at = j["activated_at"].get<int64_t>();
    }
    if (j.contains("required_approvers")) {
        s.required_approvers = j["required_approvers"].get<int>();
    }
    if (j.contains("approvers")) {
        s.approvers = j["approvers"].get<std::vector<std::string>>();
    }
    if (j.contains("approved_by_list")) {
        s.approved_by_list = j["approved_by_list"].get<std::vector<std::string>>();
    }
    if (j.contains("history")) {
        for (const auto& item : j["history"]) {
            s.history.push_back(ApprovalRecord::fromJson(item));
        }
    }
    return s;
}

// ========== PolicyApprovalWorkflow Implementation ==========

PolicyApprovalWorkflow::PolicyApprovalWorkflow() = default;

ApprovalStatus PolicyApprovalWorkflow::initiateReview(
    const std::string& rule_id,
    const std::string& current_version,
    const std::string& submitted_by,
    int required_approvers
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ApprovalStatus status;
    status.rule_id              = rule_id;
    status.current_state        = ApprovalState::DRAFT;
    status.current_version      = current_version;
    status.submitted_by         = submitted_by;
    status.submitted_at         = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    status.required_approvers   = required_approvers;
    
    approvals_[rule_id] = status;
    return status;
}

bool PolicyApprovalWorkflow::submitForReview(
    const std::string& rule_id,
    const std::string& reviewer
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return false;
    }
    
    auto& status = it->second;
    if (!isValidTransition(status.current_state, ApprovalState::REVIEW)) {
        return false;
    }
    
    ApprovalState old_state = status.current_state;
    status.current_state = ApprovalState::REVIEW;
    
    recordApprovalAction(
        rule_id,
        ApprovalAction::SUBMIT_FOR_REVIEW,
        reviewer,
        old_state,
        ApprovalState::REVIEW
    );
    
    return true;
}

bool PolicyApprovalWorkflow::approveChange(
    const std::string& rule_id,
    const std::string& approver,
    const std::string& comment
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return false;
    }
    
    auto& status = it->second;
    if (status.current_state != ApprovalState::REVIEW) {
        return false;
    }
    
    // Track approver
    auto approved_it = std::find(status.approved_by_list.begin(), status.approved_by_list.end(), approver);
    if (approved_it == status.approved_by_list.end()) {
        status.approved_by_list.push_back(approver);
    }
    
    // Check if we have enough approvals
    if (static_cast<int>(status.approved_by_list.size()) >= status.required_approvers) {
        status.current_state = ApprovalState::APPROVED;
        status.approved_by   = approver;
        status.approved_at   = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        recordApprovalAction(
            rule_id,
            ApprovalAction::APPROVE,
            approver,
            ApprovalState::REVIEW,
            ApprovalState::APPROVED,
            comment
        );
    } else {
        recordApprovalAction(
            rule_id,
            ApprovalAction::APPROVE,
            approver,
            ApprovalState::REVIEW,
            ApprovalState::REVIEW,
            fmt::format("Approval received ({}/{} approvers)", 
                       status.approved_by_list.size(), 
                       status.required_approvers) + 
                (comment.empty() ? "" : ": " + comment)
        );
    }
    
    return true;
}

bool PolicyApprovalWorkflow::rejectChange(
    const std::string& rule_id,
    const std::string& reviewer,
    const std::string& reason
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return false;
    }
    
    auto& status = it->second;
    if (status.current_state != ApprovalState::REVIEW) {
        return false;
    }
    
    status.current_state        = ApprovalState::DRAFT;
    status.approved_by_list.clear();
    
    recordApprovalAction(
        rule_id,
        ApprovalAction::REJECT,
        reviewer,
        ApprovalState::REVIEW,
        ApprovalState::DRAFT,
        reason
    );
    
    return true;
}

bool PolicyApprovalWorkflow::activatePolicy(
    const std::string& rule_id,
    const std::string& activator
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return false;
    }
    
    auto& status = it->second;
    if (status.current_state != ApprovalState::APPROVED) {
        return false;
    }
    
    status.current_state = ApprovalState::ACTIVE;
    status.activated_by  = activator;
    status.activated_at  = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    recordApprovalAction(
        rule_id,
        ApprovalAction::ACTIVATE,
        activator,
        ApprovalState::APPROVED,
        ApprovalState::ACTIVE
    );
    
    return true;
}

bool PolicyApprovalWorkflow::rollbackApproval(
    const std::string& rule_id,
    const std::string& operator_user,
    const std::string& reason
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return false;
    }
    
    auto& status = it->second;
    if (status.current_state != ApprovalState::ACTIVE) {
        return false;
    }
    
    status.current_state = ApprovalState::DEPRECATED;
    
    recordApprovalAction(
        rule_id,
        ApprovalAction::ROLLBACK,
        operator_user,
        ApprovalState::ACTIVE,
        ApprovalState::DEPRECATED,
        reason
    );
    
    return true;
}

bool PolicyApprovalWorkflow::emergencyOverride(
    const std::string& rule_id,
    const std::string& override_by,
    const std::string& reason,
    int required_approvers
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        // Create new approval status if not exists
        ApprovalStatus status;
        status.rule_id         = rule_id;
        status.current_state   = ApprovalState::ACTIVE;
        status.activated_by    = override_by;
        status.activated_at    = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        status.required_approvers = required_approvers;
        
        approvals_[rule_id] = status;
    } else {
        auto& status = it->second;
        status.current_state = ApprovalState::ACTIVE;
        status.activated_by  = override_by;
        status.activated_at  = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    recordApprovalAction(
        rule_id,
        ApprovalAction::EMERGENCY_OVERRIDE,
        override_by,
        ApprovalState::DRAFT,
        ApprovalState::ACTIVE,
        fmt::format("EMERGENCY OVERRIDE: {}", reason)
    );
    
    return true;
}

std::optional<ApprovalStatus> PolicyApprovalWorkflow::getApprovalStatus(
    const std::string& rule_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool PolicyApprovalWorkflow::canTransitionTo(
    const std::string& rule_id,
    ApprovalState target_state
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = approvals_.find(rule_id);
    if (it == approvals_.end()) {
        return false;
    }
    
    return isValidTransition(it->second.current_state, target_state);
}

std::vector<std::string> PolicyApprovalWorkflow::getRulesInState(
    ApprovalState state
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> rules;
    for (const auto& [rule_id, status] : approvals_) {
        if (status.current_state == state) {
            rules.push_back(rule_id);
        }
    }
    
    return rules;
}

std::vector<std::string> PolicyApprovalWorkflow::getPendingApprovalsFor(
    const std::string& approver
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> pending;
    for (const auto& [rule_id, status] : approvals_) {
        if (status.current_state == ApprovalState::REVIEW) {
            auto it = std::find(status.approvers.begin(), status.approvers.end(), approver);
            if (it != status.approvers.end()) {
                // Check if this approver hasn't already approved
                auto approved_it = std::find(status.approved_by_list.begin(), 
                                            status.approved_by_list.end(), 
                                            approver);
                if (approved_it == status.approved_by_list.end()) {
                    pending.push_back(rule_id);
                }
            }
        }
    }
    
    return pending;
}

std::vector<ApprovalRecord> PolicyApprovalWorkflow::queryApprovalHistory(
    const std::optional<std::string>& rule_id,
    const std::optional<std::string>& approver,
    const std::optional<int64_t>& start_time,
    const std::optional<int64_t>& end_time
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ApprovalRecord> result;
    
    for (const auto& record : audit_trail_) {
        if (rule_id && record.rule_id != rule_id.value()) {
            continue;
        }
        if (approver && record.approver != approver.value()) {
            continue;
        }
        if (start_time && record.timestamp < start_time.value()) {
            continue;
        }
        if (end_time && record.timestamp > end_time.value()) {
            continue;
        }
        
        result.push_back(record);
    }
    
    return result;
}

nlohmann::json PolicyApprovalWorkflow::exportWorkflow() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json j;
    j["approvals"] = nlohmann::json::object();
    
    for (const auto& [rule_id, status] : approvals_) {
        j["approvals"][rule_id] = status.toJson();
    }
    
    j["audit_trail"] = nlohmann::json::array();
    for (const auto& record : audit_trail_) {
        j["audit_trail"].push_back(record.toJson());
    }
    
    return j;
}

bool PolicyApprovalWorkflow::importWorkflow(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!j.contains("approvals") || !j.contains("audit_trail")) {
        return false;
    }
    
    approvals_.clear();
    audit_trail_.clear();
    
    for (const auto& [rule_id, status_json] : j["approvals"].items()) {
        approvals_[rule_id] = ApprovalStatus::fromJson(status_json);
    }
    
    for (const auto& record_json : j["audit_trail"]) {
        audit_trail_.push_back(ApprovalRecord::fromJson(record_json));
    }
    
    return true;
}

bool PolicyApprovalWorkflow::saveToFile(const std::string& path) const {
    try {
        auto json = exportWorkflow();
        std::ofstream file(path);
        file << json.dump(2);
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool PolicyApprovalWorkflow::loadFromFile(const std::string& path) {
    try {
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        file.close();
        return importWorkflow(json);
    } catch (...) {
        return false;
    }
}

void PolicyApprovalWorkflow::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    approvals_.clear();
    audit_trail_.clear();
}

bool PolicyApprovalWorkflow::isValidTransition(
    ApprovalState from,
    ApprovalState to
) const {
    // Valid transitions:
    // DRAFT -> REVIEW
    // REVIEW -> DRAFT (rejection)
    // REVIEW -> APPROVED (approval)
    // APPROVED -> ACTIVE (activation)
    // ACTIVE -> DEPRECATED (rollback)
    
    if (from == ApprovalState::DRAFT) {
        return to == ApprovalState::REVIEW;
    }
    if (from == ApprovalState::REVIEW) {
        return to == ApprovalState::DRAFT || to == ApprovalState::APPROVED;
    }
    if (from == ApprovalState::APPROVED) {
        return to == ApprovalState::ACTIVE;
    }
    if (from == ApprovalState::ACTIVE) {
        return to == ApprovalState::DEPRECATED;
    }
    
    return false;
}

void PolicyApprovalWorkflow::recordApprovalAction(
    const std::string& rule_id,
    ApprovalAction action,
    const std::string& actor,
    ApprovalState old_state,
    ApprovalState new_state,
    const std::string& comment
) {
    ApprovalRecord record;
    record.rule_id         = rule_id;
    record.approver        = actor;
    record.timestamp       = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    record.action          = action;
    record.comment         = comment;
    record.old_state       = fmt::format("{}", static_cast<int>(old_state));
    record.new_state       = fmt::format("{}", static_cast<int>(new_state));
    record.is_emergency_override = (actio[[maybe_unused]] n == ApprovalActio[[maybe_unused]] n::EMERGENCY_OVERRID[[maybe_unused]] E);
    
    audit_trail_.push_back(record);
}

} // namespace governance
} // namespace themis
