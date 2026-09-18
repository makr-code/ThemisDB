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
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ReviewRequest fromJson(const nlohmann::json& j);
};

struct ReviewSchedule {
    std::string rule_id;
    int review_period_days = 90;  // Default: 90 days
    std::int64_t last_review_date = 0;
    std::int64_t next_review_date = 0;
    bool auto_schedule = true;
    
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
    static ReviewSchedule fromJson(const nlohmann::json& j);
};

class ReviewScheduler {
public:
    ReviewScheduler(std::shared_ptr<PolicyManager> policy_manager);
    
    /**
     * @brief Configure Review Schedule.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] period_days Input parameter.
     */
    void configureReviewSchedule(const std::string& rule_id, int period_days);
    
    std::string createReviewRequest(
        const std::string& rule_id,
        const std::string& requester,
        int due_days = 7
    );
    
    void approveReview(
        const std::string& review_id,
        const std::string& reviewer,
        const std::string& comments = ""
    );
    
    /**
     * @brief Reject Review.
     * @param[in] review_id Identifier of the review.
     * @param[in] reviewer Input parameter.
     * @param[in] comments Input parameter.
     */
    void rejectReview(
        const std::string& review_id,
        const std::string& reviewer,
        const std::string& comments
    );
    
    /**
     * @brief Get Pending Reviews.
     * @return Return value.
     */
    std::vector<ReviewRequest> getPendingReviews() const;
    
    /**
     * @brief Get Overdue Reviews.
     * @return Return value.
     */
    std::vector<ReviewRequest> getOverdueReviews() const;
    
    /**
     * @brief Get Review History.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<ReviewRequest> getReviewHistory(const std::string& rule_id) const;
    
    /**
     * @brief Check Reviews Due.
     * @return Return value.
     */
    std::vector<std::string> checkReviewsDue() const;
    
    /**
     * @brief Get Expiration Info.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    nlohmann::json getExpirationInfo(const std::string& rule_id) const;
    
    /**
     * @brief Export Reviews.
     * @return Return value.
     */
    nlohmann::json exportReviews() const;
    
    /**
     * @brief Import Reviews.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importReviews(const nlohmann::json& j);
    
private:
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unordered_map<std::string, ReviewSchedule> schedules_;
    std::unordered_map<std::string, ReviewRequest> reviews_;
    
    /**
     * @brief Generate Review Id.
     * @return Return value.
     */
    std::string generateReviewId() const;
    
    /**
     * @brief Calculate Next Review Date.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::int64_t calculateNextReviewDate(const std::string& rule_id) const;
};

} // namespace governance
} // namespace themis
