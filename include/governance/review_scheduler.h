/**
 * @file review_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// Review request for a policy rule
struct ReviewRequest {
    std::string review_id;
    std::string rule_id;
    std::string requester;
    std::int64_t requested_at;
    std::int64_t due_date;
    std::string status;  // "pending", "approved", "rejected"
    std::string reviewer;
    std::int64_t reviewed_at = 0;
    std::string comments;
    
    /**
     * @brief TBD: Describe toJson.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief TBD: Describe fromJson.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ReviewRequest fromJson(const nlohmann::json& j);
};

/// Review schedule configuration for a rule
struct ReviewSchedule {
    std::string rule_id;
    int review_period_days = 90;  // Default: 90 days
    std::int64_t last_review_date = 0;
    std::int64_t next_review_date = 0;
    bool auto_schedule = true;
    
    /**
     * @brief TBD: Describe toJson.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief TBD: Describe fromJson.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ReviewSchedule fromJson(const nlohmann::json& j);
};

/// Policy review scheduler
class ReviewScheduler {
public:
    ReviewScheduler(std::shared_ptr<PolicyManager> policy_manager);
    
    /**
     * @brief Configure review schedule for a rule @param rule_id Rule identifier @param period_days Review period in days
     * @param[in] rule_id Input parameter.
     * @param[in] period_days Input parameter.
     */
    void configureReviewSchedule(const std::string& rule_id, int period_days);
    
    /// Create a review request for a rule
    /// @param rule_id Rule to review
    /// @param requester User requesting review
    /// @param due_days Days until review is due
    /// @return Review request ID
    std::string createReviewRequest(
        const std::string& rule_id,
        const std::string& requester,
        int due_days = 7
    );
    
    /// Approve a review
    /// @param review_id Review request ID
    /// @param reviewer User approving the review
    /// @param comments Review comments
    void approveReview(
        const std::string& review_id,
        const std::string& reviewer,
        const std::string& comments = ""
    );
    
    /**
     * @brief Reject a review @param review_id Review request ID @param reviewer User rejecting the review @param comments Rejection reason
     * @param[in] review_id Input parameter.
     * @param[in] reviewer Input parameter.
     * @param[in] comments Input parameter.
     */
    void rejectReview(
        const std::string& review_id,
        const std::string& reviewer,
        const std::string& comments
    );
    
    /**
     * @brief Get pending reviews @return List of pending review requests
     * @return Return value.
     */
    std::vector<ReviewRequest> getPendingReviews() const;
    
    /**
     * @brief Get overdue reviews @return List of overdue review requests
     * @return Return value.
     */
    std::vector<ReviewRequest> getOverdueReviews() const;
    
    /**
     * @brief Get review history for a rule @param rule_id Rule identifier @return List of review requests for the rule
     * @param[in] rule_id Input parameter.
     * @return Return value.
     */
    std::vector<ReviewRequest> getReviewHistory(const std::string& rule_id) const;
    
    /**
     * @brief Check for rules needing review @return List of rule IDs that need review
     * @return Return value.
     */
    std::vector<std::string> checkReviewsDue() const;
    
    /**
     * @brief Get expiration info for a rule @param rule_id Rule identifier @return JSON with expiration details
     * @param[in] rule_id Input parameter.
     * @return Return value.
     */
    nlohmann::json getExpirationInfo(const std::string& rule_id) const;
    
    /// Export review data
    nlohmann::json exportReviews() const;
    
    /// Import review data
    bool importReviews(const nlohmann::json& j);
    
private:
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unordered_map<std::string, ReviewSchedule> schedules_;
    std::unordered_map<std::string, ReviewRequest> reviews_;
    
    /// Helper: Generate unique review ID
    std::string generateReviewId() const;
    
    /// Helper: Calculate next review date
    std::int64_t calculateNextReviewDate(const std::string& rule_id) const;
};

} // namespace governance
} // namespace themis
