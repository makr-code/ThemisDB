/**
 * @file policy_review.h
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
#include <optional>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

struct PolicyReview {
    std::string review_id;                         // Unique review identifier
    std::string rule_id;                           // Rule being reviewed
    std::string status;                            // pending, approved, rejected, expired
    std::string reviewer;                          // User assigned to review
    std::string requester;                         // User who requested review
    int64_t created_at = 0;                        // When review was created
    int64_t due_date = 0;                          // Review deadline
    int64_t completed_at = 0;                      // When review was completed
    std::string review_notes;                      // Reviewer's notes
    std::string rejection_reason;                  // Reason if rejected
    
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
    static PolicyReview fromJson(const nlohmann::json& j);
};

class ReviewScheduler {
public:
    struct ReviewSchedule {
        std::string rule_id;
        int review_period_days = 90;               // Review every N days
        int64_t last_review_date = 0;              // Last review timestamp
        int64_t next_review_date = 0;              // Next scheduled review
        bool auto_review_enabled = true;           // Whether automatic scheduling is enabled
        
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
    
    /**
     * @brief Set Schedule.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] review_period_days Input parameter.
     */
    void setSchedule(const std::string& rule_id, int review_period_days);
    
    /**
     * @brief Get Schedule.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::optional<ReviewSchedule> getSchedule(const std::string& rule_id) const;
    
    /**
     * @brief Remove Schedule.
     * @param[in] rule_id Identifier of the rule.
     */
    void removeSchedule(const std::string& rule_id);
    
    /**
     * @brief Get All Schedules.
     * @return Return value.
     */
    std::vector<ReviewSchedule> getAllSchedules() const;
    
    std::vector<std::string> getRulesDueForReview(int64_t current_time = 0) const;
    
    std::vector<std::string> getOverdueReviews(int64_t current_time = 0) const;
    
    void markAsReviewed(const std::string& rule_id, int64_t review_time = 0);
    
    /**
     * @brief Export Schedules.
     * @return Return value.
     */
    nlohmann::json exportSchedules() const;
    
    /**
     * @brief Import Schedules.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importSchedules(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ReviewSchedule> schedules_;
};

class ReviewWorkflow {
public:
    std::string createReview(
        const std::string& rule_id,
        const std::string& reviewer,
        const std::string& requester,
        int days_to_complete = 7
    );
    
    /**
     * @brief Get Review.
     * @param[in] review_id Identifier of the review.
     * @return Return value.
     */
    std::optional<PolicyReview> getReview(const std::string& review_id) const;
    
    /**
     * @brief List Reviews.
     * @return Return value.
     */
    std::vector<PolicyReview> listReviews() const;
    
    /**
     * @brief List Reviews By Status.
     * @param[in] status Input parameter.
     * @return Return value.
     */
    std::vector<PolicyReview> listReviewsByStatus(const std::string& status) const;
    
    /**
     * @brief List Reviews By Reviewer.
     * @param[in] reviewer Input parameter.
     * @return Return value.
     */
    std::vector<PolicyReview> listReviewsByReviewer(const std::string& reviewer) const;
    
    /**
     * @brief List Pending Reviews.
     * @return Return value.
     */
    std::vector<PolicyReview> listPendingReviews() const;
    
    /**
     * @brief Approve Review.
     * @param[in] review_id Identifier of the review.
     * @param[in] notes Input parameter.
     * @return True when the operation succeeds.
     */
    bool approveReview(const std::string& review_id, const std::string& notes);
    
    /**
     * @brief Reject Review.
     * @param[in] review_id Identifier of the review.
     * @param[in] reason Input parameter.
     * @param[in] notes Input parameter.
     * @return True when the operation succeeds.
     */
    bool rejectReview(const std::string& review_id, const std::string& reason, const std::string& notes);
    
    /**
     * @brief Get Review History.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<PolicyReview> getReviewHistory(const std::string& rule_id) const;
    
    std::vector<PolicyReview> getOverdueReviews(int64_t current_time = 0) const;
    
    /**
     * @brief Cancel Review.
     * @param[in] review_id Identifier of the review.
     * @return True when the operation succeeds.
     */
    bool cancelReview(const std::string& review_id);
    
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
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PolicyReview> reviews_;
    
    /**
     * @brief Generate Review Id.
     * @return Return value.
     */
    std::string generateReviewId() const;
};

class PolicyExpiration {
public:
    struct ExpirationConfig {
        std::string rule_id;
        int64_t expiration_date = 0;               // When rule expires
        int grace_period_days = 7;                 // Days before automatic disable
        bool auto_disable_enabled = true;          // Whether to auto-disable on expiration
        std::vector<int> warning_days = {30, 14, 7}; // Days before expiration to warn
        
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
        static ExpirationConfig fromJson(const nlohmann::json& j);
    };
    
    struct ExpirationWarning {
        std::string rule_id = {};
        int64_t expiration_date;
        int days_until_expiration;
        std::string severity;                      // info, warning, critical
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    void setExpiration(const std::string& rule_id, int64_t expiration_date, int grace_period_days = 7);
    
    /**
     * @brief Get Expiration.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::optional<ExpirationConfig> getExpiration(const std::string& rule_id) const;
    
    /**
     * @brief Remove Expiration.
     * @param[in] rule_id Identifier of the rule.
     */
    void removeExpiration(const std::string& rule_id);
    
    /**
     * @brief Get All Expirations.
     * @return Return value.
     */
    std::vector<ExpirationConfig> getAllExpirations() const;
    
    std::vector<std::string> getExpiredRules(int64_t current_time = 0) const;
    
    std::vector<ExpirationWarning> getRulesExpiringSoon(int64_t current_time = 0) const;
    
    std::vector<std::string> processExpirations(PolicyManager& policy_mgr, int64_t current_time = 0);
    
    /**
     * @brief Extend Expiration.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] additional_days Input parameter.
     */
    void extendExpiration(const std::string& rule_id, int additional_days);
    
    /**
     * @brief Export Expirations.
     * @return Return value.
     */
    nlohmann::json exportExpirations() const;
    
    /**
     * @brief Import Expirations.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importExpirations(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ExpirationConfig> expirations_;
};

class NotificationManager {
public:
    struct Notification {
        std::string notification_id;
        std::string notification_type;             // review_due, review_overdue, expiration_warning, etc.
        std::string recipient;                     // Email or webhook URL
        std::string subject;
        std::string message;
        int64_t created_at = 0;
        bool sent = false;
        int64_t sent_at = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    struct NotificationConfig {
        bool email_enabled = false;
        std::string smtp_server;
        int smtp_port = 587;
        std::string smtp_username;
        std::string smtp_password;
        std::string from_email;
        
        bool webhook_enabled = false;
        std::string webhook_url;
        std::string webhook_secret;
        
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
        static NotificationConfig fromJson(const nlohmann::json& j);
    };
    
    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     */
    void configure(const NotificationConfig& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    NotificationConfig getConfig() const;
    
    /**
     * @brief Notify Review Due.
     * @param[in] recipient Input parameter.
     * @param[in] review Input parameter.
     * @return True when the operation succeeds.
     */
    bool notifyReviewDue(const std::string& recipient, const PolicyReview& review);
    
    /**
     * @brief Notify Review Overdue.
     * @param[in] recipient Input parameter.
     * @param[in] review Input parameter.
     * @param[in] days_overdue Input parameter.
     * @return True when the operation succeeds.
     */
    bool notifyReviewOverdue(const std::string& recipient, const PolicyReview& review, int days_overdue);
    
    /**
     * @brief Notify Expiration Warning.
     * @param[in] recipient Input parameter.
     * @param[in] warning Input parameter.
     * @return True when the operation succeeds.
     */
    bool notifyExpirationWarning(const std::string& recipient, const PolicyExpiration::ExpirationWarning& warning);
    
    /**
     * @brief Notify Rule Expired.
     * @param[in] recipient Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool notifyRuleExpired(const std::string& recipient, const std::string& rule_id);
    
    /**
     * @brief Create Notification.
     * @param[in] type Input parameter.
     * @param[in] recipient Input parameter.
     * @param[in] subject Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    std::string createNotification(
        const std::string& type,
        const std::string& recipient,
        const std::string& subject,
        const std::string& message
    );
    
    /**
     * @brief Get Pending Notifications.
     * @return Return value.
     */
    std::vector<Notification> getPendingNotifications() const;
    
    /**
     * @brief Mark As Sent.
     * @param[in] notification_id Identifier of the notification.
     */
    void markAsSent(const std::string& notification_id);
    
    std::vector<Notification> getNotificationHistory(int64_t since = 0) const;
    
private:
    mutable std::mutex mutex_;
    NotificationConfig config_;
    std::unordered_map<std::string, Notification> notifications_;
    
    /**
     * @brief Generate Notification Id.
     * @return Return value.
     */
    std::string generateNotificationId() const;
    /**
     * @brief Send Email.
     * @param[in] notification Input parameter.
     * @return True when the operation succeeds.
     */
    bool sendEmail(const Notification& notification);
    /**
     * @brief Send Webhook.
     * @param[in] notification Input parameter.
     * @return True when the operation succeeds.
     */
    bool sendWebhook(const Notification& notification);
};

} // namespace governance
} // namespace themis

