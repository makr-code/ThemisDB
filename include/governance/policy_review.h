/**
 * @file policy_review.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// PolicyReview represents a policy review request
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
    
    nlohmann::json toJson() const;
    static PolicyReview fromJson(const nlohmann::json& j);
};

/// ReviewScheduler manages scheduled policy reviews
class ReviewScheduler {
public:
    struct ReviewSchedule {
        std::string rule_id;
        int review_period_days = 90;               // Review every N days
        int64_t last_review_date = 0;              // Last review timestamp
        int64_t next_review_date = 0;              // Next scheduled review
        bool auto_review_enabled = true;           // Whether automatic scheduling is enabled
        
        nlohmann::json toJson() const;
        static ReviewSchedule fromJson(const nlohmann::json& j);
    };
    
    /// Set review schedule for a rule
    void setSchedule(const std::string& rule_id, int review_period_days);
    
    /// Get review schedule for a rule
    std::optional<ReviewSchedule> getSchedule(const std::string& rule_id) const;
    
    /// Remove review schedule for a rule
    void removeSchedule(const std::string& rule_id);
    
    /// Get all review schedules
    std::vector<ReviewSchedule> getAllSchedules() const;
    
    /// Get rules due for review
    std::vector<std::string> getRulesDueForReview(int64_t current_time = 0) const;
    
    /// Get overdue reviews
    std::vector<std::string> getOverdueReviews(int64_t current_time = 0) const;
    
    /// Mark rule as reviewed
    void markAsReviewed(const std::string& rule_id, int64_t review_time = 0);
    
    /// Export schedules as JSON
    nlohmann::json exportSchedules() const;
    
    /// Import schedules from JSON
    bool importSchedules(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ReviewSchedule> schedules_;
};

/// ReviewWorkflow manages the review approval process
class ReviewWorkflow {
public:
    /// Create a review request
    std::string createReview(
        const std::string& rule_id,
        const std::string& reviewer,
        const std::string& requester,
        int days_to_complete = 7
    );
    
    /// Get a review by ID
    std::optional<PolicyReview> getReview(const std::string& review_id) const;
    
    /// List all reviews
    std::vector<PolicyReview> listReviews() const;
    
    /// List reviews by status
    std::vector<PolicyReview> listReviewsByStatus(const std::string& status) const;
    
    /// List reviews by reviewer
    std::vector<PolicyReview> listReviewsByReviewer(const std::string& reviewer) const;
    
    /// List pending reviews
    std::vector<PolicyReview> listPendingReviews() const;
    
    /// Approve a review
    bool approveReview(const std::string& review_id, const std::string& notes);
    
    /// Reject a review
    bool rejectReview(const std::string& review_id, const std::string& reason, const std::string& notes);
    
    /// Get review history for a rule
    std::vector<PolicyReview> getReviewHistory(const std::string& rule_id) const;
    
    /// Get overdue reviews
    std::vector<PolicyReview> getOverdueReviews(int64_t current_time = 0) const;
    
    /// Cancel a review
    bool cancelReview(const std::string& review_id);
    
    /// Export reviews as JSON
    nlohmann::json exportReviews() const;
    
    /// Import reviews from JSON
    bool importReviews(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PolicyReview> reviews_;
    
    std::string generateReviewId() const;
};

/// PolicyExpiration manages automatic rule expiration
class PolicyExpiration {
public:
    struct ExpirationConfig {
        std::string rule_id;
        int64_t expiration_date = 0;               // When rule expires
        int grace_period_days = 7;                 // Days before automatic disable
        bool auto_disable_enabled = true;          // Whether to auto-disable on expiration
        std::vector<int> warning_days = {30, 14, 7}; // Days before expiration to warn
        
        nlohmann::json toJson() const;
        static ExpirationConfig fromJson(const nlohmann::json& j);
    };
    
    struct ExpirationWarning {
        std::string rule_id = {};
        int64_t expiration_date;
        int days_until_expiration;
        std::string severity;                      // info, warning, critical
        
        nlohmann::json toJson() const;
    };
    
    /// Set expiration for a rule
    void setExpiration(const std::string& rule_id, int64_t expiration_date, int grace_period_days = 7);
    
    /// Get expiration config for a rule
    std::optional<ExpirationConfig> getExpiration(const std::string& rule_id) const;
    
    /// Remove expiration for a rule
    void removeExpiration(const std::string& rule_id);
    
    /// Get all expiration configs
    std::vector<ExpirationConfig> getAllExpirations() const;
    
    /// Get expired rules
    std::vector<std::string> getExpiredRules(int64_t current_time = 0) const;
    
    /// Get rules expiring soon
    std::vector<ExpirationWarning> getRulesExpiringSoon(int64_t current_time = 0) const;
    
    /// Process expirations (disable expired rules)
    std::vector<std::string> processExpirations(PolicyManager& policy_mgr, int64_t current_time = 0);
    
    /// Extend expiration for a rule
    void extendExpiration(const std::string& rule_id, int additional_days);
    
    /// Export expirations as JSON
    nlohmann::json exportExpirations() const;
    
    /// Import expirations from JSON
    bool importExpirations(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ExpirationConfig> expirations_;
};

/// NotificationManager handles notifications for reviews and expirations
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
        
        nlohmann::json toJson() const;
        static NotificationConfig fromJson(const nlohmann::json& j);
    };
    
    /// Configure notification settings
    void configure(const NotificationConfig& config);
    
    /// Get current configuration
    NotificationConfig getConfig() const;
    
    /// Send notification for upcoming review
    bool notifyReviewDue(const std::string& recipient, const PolicyReview& review);
    
    /// Send notification for overdue review
    bool notifyReviewOverdue(const std::string& recipient, const PolicyReview& review, int days_overdue);
    
    /// Send notification for expiration warning
    bool notifyExpirationWarning(const std::string& recipient, const PolicyExpiration::ExpirationWarning& warning);
    
    /// Send notification for rule expired
    bool notifyRuleExpired(const std::string& recipient, const std::string& rule_id);
    
    /// Create a notification (queued for sending)
    std::string createNotification(
        const std::string& type,
        const std::string& recipient,
        const std::string& subject,
        const std::string& message
    );
    
    /// Get pending notifications
    std::vector<Notification> getPendingNotifications() const;
    
    /// Mark notification as sent
    void markAsSent(const std::string& notification_id);
    
    /// Get notification history
    std::vector<Notification> getNotificationHistory(int64_t since = 0) const;
    
private:
    mutable std::mutex mutex_;
    NotificationConfig config_;
    std::unordered_map<std::string, Notification> notifications_;
    
    std::string generateNotificationId() const;
    bool sendEmail(const Notification& notification);
    bool sendWebhook(const Notification& notification);
};

} // namespace governance
} // namespace themis

