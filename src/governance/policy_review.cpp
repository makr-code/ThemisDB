/**
 * @file policy_review.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=1, H=0, M=23, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_review.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

#include "security/pii_redaction_policy.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// Helper function to get current time in milliseconds
static int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ========== PolicyReview Implementation ==========

nlohmann::json PolicyReview::toJson() const {
    nlohmann::json j;
    j["review_id"]        = review_id;
    j["rule_id"]          = rule_id;
    j["status"]           = status;
    j["reviewer"]         = reviewer;
    j["requester"]        = requester;
    j["created_at"]       = created_at;
    j["due_date"]         = due_date;
    j["completed_at"]     = completed_at;
    j["review_notes"]     = review_notes;
    j["rejection_reason"] = rejection_reason;
    return j;
}

PolicyReview PolicyReview::fromJson(const nlohmann::json &j) {
    PolicyReview review = {};
    if (j.contains("review_id")) {
        review.review_id = j["review_id"].get<std::string>();
    }
    if (j.contains("rule_id")) {
        review.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("status")) {
        review.status = j["status"].get<std::string>();
    }
    if (j.contains("reviewer")) {
        review.reviewer = j["reviewer"].get<std::string>();
    }
    if (j.contains("requester")) {
        review.requester = j["requester"].get<std::string>();
    }
    if (j.contains("created_at")) {
        review.created_at = j["created_at"].get<int64_t>();
    }
    if (j.contains("due_date")) {
        review.due_date = j["due_date"].get<int64_t>();
    }
    if (j.contains("completed_at")) {
        review.completed_at = j["completed_at"].get<int64_t>();
    }
    if (j.contains("review_notes")) {
        review.review_notes = j["review_notes"].get<std::string>();
    }
    if (j.contains("rejection_reason")) {
        review.rejection_reason = j["rejection_reason"].get<std::string>();
    }
    return review;
}

// ========== ReviewScheduler::ReviewSchedule Implementation ==========

nlohmann::json ReviewScheduler::ReviewSchedule::toJson() const {
    nlohmann::json j;
    j["rule_id"]             = rule_id;
    j["review_period_days"]  = review_period_days;
    j["last_review_date"]    = last_review_date;
    j["next_review_date"]    = next_review_date;
    j["auto_review_enabled"] = auto_review_enabled;
    return j;
}

ReviewScheduler::ReviewSchedule ReviewScheduler::ReviewSchedule::fromJson(const nlohmann::json &j) {
    ReviewSchedule schedule = {};
    if (j.contains("rule_id")) {
        schedule.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("review_period_days")) {
        schedule.review_period_days = j["review_period_days"].get<int>();
    }
    if (j.contains("last_review_date")) {
        schedule.last_review_date = j["last_review_date"].get<int64_t>();
    }
    if (j.contains("next_review_date")) {
        schedule.next_review_date = j["next_review_date"].get<int64_t>();
    }
    if (j.contains("auto_review_enabled")) {
        schedule.auto_review_enabled = j["auto_review_enabled"].get<bool>();
    }
    return schedule;
}

// ========== ReviewScheduler Implementation ==========

void ReviewScheduler::setSchedule(const std::string &rule_id, int review_period_days) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = getCurrentTimeMs();

    ReviewSchedule schedule;
    schedule.rule_id             = rule_id;
    schedule.review_period_days  = review_period_days;
    schedule.last_review_date    = now;
    schedule.next_review_date    = now + (static_cast<int64_t>(review_period_days) * 24 * 60 * 60 * 1000);
    schedule.auto_review_enabled = true;

    schedules_[rule_id] = schedule;

    THEMIS_DEBUG("Set review schedule for rule {}: every {} days, next review at {}", rule_id, review_period_days,
                 schedule.next_review_date);
}

std::optional<ReviewScheduler::ReviewSchedule> ReviewScheduler::getSchedule(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = schedules_.find(rule_id);
    if (it != schedules_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ReviewScheduler::removeSchedule(const std::string &rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = schedules_.find(rule_id);
    if (it != schedules_.end()) {
        schedules_.erase(it);
        THEMIS_DEBUG("Removed review schedule for rule {}", rule_id);
    }
}

std::vector<ReviewScheduler::ReviewSchedule> ReviewScheduler::getAllSchedules() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ReviewSchedule> result = {};

    result.reserve(schedules_.size());

    for (const auto &pair : schedules_) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<std::string> ReviewScheduler::getRulesDueForReview(int64_t current_time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_time == 0) {
        current_time = getCurrentTimeMs();
    }

    std::vector<std::string> due_rules;

    for (const auto &pair : schedules_) {
        const auto &schedule = pair.second;
        if (schedule.auto_review_enabled && schedule.next_review_date <= current_time) {
            due_rules.push_back(schedule.rule_id);
        }
    }

    THEMIS_DEBUG("Found {} rules due for review",static_cast<int>(due_rules.size()));
    return due_rules;
}

std::vector<std::string> ReviewScheduler::getOverdueReviews(int64_t current_time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_time == 0) {
        current_time = getCurrentTimeMs();
    }

    std::vector<std::string> overdue_rules;

    for (const auto &pair : schedules_) {
        const auto &schedule = pair.second;
        // Consider overdue if more than 7 days past due date
        int64_t grace_period = 7 * 24 * 60 * 60 * 1000;
        if (schedule.auto_review_enabled && schedule.next_review_date < (current_time - grace_period)) {
            overdue_rules.push_back(schedule.rule_id);
        }
    }

    THEMIS_INFO("Found {} overdue reviews",static_cast<int>(overdue_rules.size()));
    return overdue_rules;
}

void ReviewScheduler::markAsReviewed(const std::string &rule_id, int64_t review_time) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = schedules_.find(rule_id);
    if (it != schedules_.end()) {
        if (review_time == 0) {
            review_time = getCurrentTimeMs();
        }

        it->second.last_review_date = review_time;
        it->second.next_review_date
            = review_time + (static_cast<int64_t>(it->second.review_period_days) * 24 * 60 * 60 * 1000);

        THEMIS_INFO("Marked rule {} as reviewed, next review at {}", rule_id, it->second.next_review_date);
    }
}

nlohmann::json ReviewScheduler::exportSchedules() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;
    nlohmann::json schedules_arr = nlohmann::json::array();

    for (const auto &pair : schedules_) {
        schedules_arr.push_back(pair.second.toJson());
    }

    j["schedules"] = schedules_arr;
    j["count"]     = schedules_.size();

    return j;
}

bool ReviewScheduler::importSchedules(const nlohmann::json &j) {
    try {
        if (!j.contains("schedules") || !j["schedules"].is_array()) {
            THEMIS_ERROR("Invalid schedules JSON: missing or invalid 'schedules' field");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        schedules_.clear();

        for (const auto &schedule_json : j["schedules"]) {
            auto schedule                = ReviewSchedule::fromJson(schedule_json);
            schedules_[schedule.rule_id] = schedule;
        }

        THEMIS_INFO("Imported {} review schedules",static_cast<int>(schedules_.size()));
        return true;

    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to import schedules: {}", e.what());
        return false;
    }
}

// ========== ReviewWorkflow Implementation ==========

std::string ReviewWorkflow::generateReviewId() const {
    // Generate UUID-like ID
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::ostringstream oss = {};
    oss << "review-" << std::hex << dis(gen);
    return oss.str();
}

std::string ReviewWorkflow::createReview(const std::string &rule_id, const std::string &reviewer,
                                         const std::string &requester, int days_to_complete) {
    std::lock_guard<std::mutex> lock(mutex_);

    PolicyReview review;
    review.review_id    = generateReviewId();
    review.rule_id      = rule_id;
    review.status       = "pending";
    review.reviewer     = reviewer;
    review.requester    = requester;
    review.created_at   = getCurrentTimeMs();
    review.due_date     = review.created_at + (static_cast<int64_t>(days_to_complete) * 24 * 60 * 60 * 1000);
    review.completed_at = 0;

    reviews_[review.review_id] = review;

    THEMIS_INFO("Created review {} for rule {} assigned to {}, due at {}", review.review_id, rule_id, reviewer,
                review.due_date);

    return review.review_id;
}

std::optional<PolicyReview> ReviewWorkflow::getReview(const std::string &review_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reviews_.find(review_id);
    if (it != reviews_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PolicyReview> ReviewWorkflow::listReviews() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PolicyReview> result = {};

    result.reserve(reviews_.size());

    for (const auto &pair : reviews_) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<PolicyReview> ReviewWorkflow::listReviewsByStatus(const std::string &status) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PolicyReview> result;

    for (const auto &pair : reviews_) {
        if (pair.second.status == status) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<PolicyReview> ReviewWorkflow::listReviewsByReviewer(const std::string &reviewer) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PolicyReview> result;

    for (const auto &pair : reviews_) {
        if (pair.second.reviewer == reviewer) {
            result.push_back(pair.second);
        }
    }

    return result;
}

std::vector<PolicyReview> ReviewWorkflow::listPendingReviews() const {
    return listReviewsByStatus("pending");
}

bool ReviewWorkflow::approveReview(const std::string &review_id, const std::string &notes) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reviews_.find(review_id);
    if (it == reviews_.end()) {
        THEMIS_ERROR("Review {} not found", review_id);
        return false;
    }

    if (it->second.status != "pending") {
        THEMIS_ERROR("Review {} is not pending (status: {})", review_id, it->second.status);
        return false;
    }

    it->second.status       = "approved";
    it->second.review_notes = notes;
    it->second.completed_at = getCurrentTimeMs();

    THEMIS_INFO("Approved review {} for rule {}", review_id, it->second.rule_id);
    return true;
}

bool ReviewWorkflow::rejectReview(const std::string &review_id, const std::string &reason, const std::string &notes) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reviews_.find(review_id);
    if (it == reviews_.end()) {
        THEMIS_ERROR("Review {} not found", review_id);
        return false;
    }

    if (it->second.status != "pending") {
        THEMIS_ERROR("Review {} is not pending (status: {})", review_id, it->second.status);
        return false;
    }

    it->second.status           = "rejected";
    it->second.rejection_reason = reason;
    it->second.review_notes     = notes;
    it->second.completed_at     = getCurrentTimeMs();

    THEMIS_INFO("Rejected review {} for rule {}: {}", review_id, it->second.rule_id, reason);
    return true;
}

std::vector<PolicyReview> ReviewWorkflow::getReviewHistory(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PolicyReview> result;

    for (const auto &pair : reviews_) {
        if (pair.second.rule_id == rule_id) {
            result.push_back(pair.second);
        }
    }

    // Sort by created_at descending
    std::sort(result.begin(), result.end(),
              [](const PolicyReview &a, const PolicyReview &b) { return a.created_at > b.created_at; });

    return result;
}

std::vector<PolicyReview> ReviewWorkflow::getOverdueReviews(int64_t current_time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_time == 0) {
        current_time = getCurrentTimeMs();
    }

    std::vector<PolicyReview> result;

    for (const auto &pair : reviews_) {
        if (pair.second.status == "pending" && pair.second.due_date < current_time) {
            result.push_back(pair.second);
        }
    }

    THEMIS_INFO("Found {} overdue reviews",static_cast<int>(result.size()));
    return result;
}

bool ReviewWorkflow::cancelReview(const std::string &review_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = reviews_.find(review_id);
    if (it == reviews_.end()) {
        THEMIS_ERROR("Review {} not found", review_id);
        return false;
    }

    if (it->second.status != "pending") {
        THEMIS_ERROR("Cannot cancel review {} with status {}", review_id, it->second.status);
        return false;
    }

    it->second.status       = "cancelled";
    it->second.completed_at = getCurrentTimeMs();

    THEMIS_INFO("Cancelled review {}", review_id);
    return true;
}

nlohmann::json ReviewWorkflow::exportReviews() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;
    nlohmann::json reviews_arr = nlohmann::json::array();

    for (const auto &pair : reviews_) {
        reviews_arr.push_back(pair.second.toJson());
    }

    j["reviews"] = reviews_arr;
    j["count"]   = reviews_.size();

    return j;
}

bool ReviewWorkflow::importReviews(const nlohmann::json &j) {
    try {
        if (!j.contains("reviews") || !j["reviews"].is_array()) {
            THEMIS_ERROR("Invalid reviews JSON: missing or invalid 'reviews' field");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        reviews_.clear();

        for (const auto &review_json : j["reviews"]) {
            auto review                = PolicyReview::fromJson(review_json);
            reviews_[review.review_id] = review;
        }

        THEMIS_INFO("Imported {} reviews",static_cast<int>(reviews_.size()));
        return true;

    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to import reviews: {}", e.what());
        return false;
    }
}

// ========== PolicyExpiration::ExpirationConfig Implementation ==========

nlohmann::json PolicyExpiration::ExpirationConfig::toJson() const {
    nlohmann::json j;
    j["rule_id"]              = rule_id;
    j["expiration_date"]      = expiration_date;
    j["grace_period_days"]    = grace_period_days;
    j["auto_disable_enabled"] = auto_disable_enabled;
    j["warning_days"]         = warning_days;
    return j;
}

PolicyExpiration::ExpirationConfig PolicyExpiration::ExpirationConfig::fromJson(const nlohmann::json &j) {
    ExpirationConfig config = {};
    if (j.contains("rule_id")) {
        config.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("expiration_date")) {
        config.expiration_date = j["expiration_date"].get<int64_t>();
    }
    if (j.contains("grace_period_days")) {
        config.grace_period_days = j["grace_period_days"].get<int>();
    }
    if (j.contains("auto_disable_enabled")) {
        config.auto_disable_enabled = j["auto_disable_enabled"].get<bool>();
    }
    if (j.contains("warning_days")) {
        config.warning_days = j["warning_days"].get<std::vector<int>>();
    }
    return config;
}

// ========== PolicyExpiration::ExpirationWarning Implementation ==========

nlohmann::json PolicyExpiration::ExpirationWarning::toJson() const {
    nlohmann::json j;
    j["rule_id"]               = rule_id;
    j["expiration_date"]       = expiration_date;
    j["days_until_expiration"] = days_until_expiration;
    j["severity"]              = severity;
    return j;
}

// ========== PolicyExpiration Implementation ==========

void PolicyExpiration::setExpiration(const std::string &rule_id, int64_t expiration_date, int grace_period_days) {
    std::lock_guard<std::mutex> lock(mutex_);

    ExpirationConfig config;
    config.rule_id              = rule_id;
    config.expiration_date      = expiration_date;
    config.grace_period_days    = grace_period_days;
    config.auto_disable_enabled = true;
    config.warning_days         = {30, 14, 7};

    expirations_[rule_id] = config;

    THEMIS_DEBUG("Set expiration for rule {} at {}, grace period {} days", rule_id, expiration_date, grace_period_days);
}

std::optional<PolicyExpiration::ExpirationConfig> PolicyExpiration::getExpiration(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = expirations_.find(rule_id);
    if (it != expirations_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void PolicyExpiration::removeExpiration(const std::string &rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = expirations_.find(rule_id);
    if (it != expirations_.end()) {
        expirations_.erase(it);
        THEMIS_DEBUG("Removed expiration for rule {}", rule_id);
    }
}

std::vector<PolicyExpiration::ExpirationConfig> PolicyExpiration::getAllExpirations() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ExpirationConfig> result = {};

    result.reserve(expirations_.size());

    for (const auto &pair : expirations_) {
        result.push_back(pair.second);
    }

    return result;
}

std::vector<std::string> PolicyExpiration::getExpiredRules(int64_t current_time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_time == 0) {
        current_time = getCurrentTimeMs();
    }

    std::vector<std::string> expired_rules;

    for (const auto &pair : expirations_) {
        const auto &config = pair.second;
        if (config.expiration_date <= current_time) {
            expired_rules.push_back(config.rule_id);
        }
    }

    THEMIS_INFO("Found {} expired rules",static_cast<int>(expired_rules.size()));
    return expired_rules;
}

std::vector<PolicyExpiration::ExpirationWarning> PolicyExpiration::getRulesExpiringSoon(int64_t current_time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_time == 0) {
        current_time = getCurrentTimeMs();
    }

    std::vector<ExpirationWarning> warnings;

    for (const auto &pair : expirations_) {
        const auto &config = pair.second;

        if (config.expiration_date <= current_time) {
            continue; // Already expired
        }

        int64_t time_until_expiration = config.expiration_date - current_time;
        int days_until_expiration     = static_cast<int>(time_until_expiration / (24 * 60 * 60 * 1000));

        // Check if we should warn
        for (int warning_days : config.warning_days) {
            if (days_until_expiration <= warning_days) {
                ExpirationWarning warning;
                warning.rule_id               = config.rule_id;
                warning.expiration_date       = config.expiration_date;
                warning.days_until_expiration = days_until_expiration;

                // Determine severity
                if (days_until_expiration <= 7) {
                    warning.severity = "critical";
                } else if (days_until_expiration <= 14) {
                    warning.severity = "warning";
                } else {
                    warning.severity = "info";
                }

                warnings.push_back(warning);
                break; // Only warn once per rule
            }
        }
    }

    THEMIS_DEBUG("Found {} rules expiring soon",static_cast<int>(warnings.size()));
    return warnings;
}

std::vector<std::string> PolicyExpiration::processExpirations(PolicyManager &policy_mgr, int64_t current_time) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_time == 0) {
        current_time = getCurrentTimeMs();
    }

    std::vector<std::string> disabled_rules;

    for (const auto &pair : expirations_) {
        const auto &config = pair.second;

        if (!config.auto_disable_enabled) {
            continue;
        }

        // Check if past expiration + grace period
        int64_t grace_end
            = config.expiration_date + (static_cast<int64_t>(config.grace_period_days) * 24 * 60 * 60 * 1000);

        if (current_time >= grace_end) {
            // Disable the rule
            auto rule = policy_mgr.getRule(config.rule_id);
            if (rule.has_value() && rule->enabled) {
                PolicyRule updated_rule         = rule.value();
                updated_rule.enabled            = false;
                updated_rule.change_description = "Auto-disabled due to expiration";

                policy_mgr.updateRule(config.rule_id, updated_rule, "system", updated_rule.change_description);
                disabled_rules.push_back(config.rule_id);

                THEMIS_INFO("Auto-disabled expired rule {}", config.rule_id);
            }
        }
    }

    THEMIS_INFO("Processed expirations: disabled {} rules",static_cast<int>(disabled_rules.size()));
    return disabled_rules;
}

void PolicyExpiration::extendExpiration(const std::string &rule_id, int additional_days) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = expirations_.find(rule_id);
    if (it != expirations_.end()) {
        int64_t extension = static_cast<int64_t>(additional_days) * 24 * 60 * 60 * 1000;
        it->second.expiration_date += extension;

        THEMIS_INFO("Extended expiration for rule {} by {} days, new expiration: {}", rule_id, additional_days,
                    it->second.expiration_date);
    } else {
        THEMIS_ERROR("Cannot extend expiration for rule {}: not found", rule_id);
    }
}

nlohmann::json PolicyExpiration::exportExpirations() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;
    nlohmann::json expirations_arr = nlohmann::json::array();

    for (const auto &pair : expirations_) {
        expirations_arr.push_back(pair.second.toJson());
    }

    j["expirations"] = expirations_arr;
    j["count"]       = expirations_.size();

    return j;
}

bool PolicyExpiration::importExpirations(const nlohmann::json &j) {
    try {
        if (!j.contains("expirations") || !j["expirations"].is_array()) {
            THEMIS_ERROR("Invalid expirations JSON: missing or invalid 'expirations' field");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        expirations_.clear();

        for (const auto &expiration_json : j["expirations"]) {
            auto config                  = ExpirationConfig::fromJson(expiration_json);
            expirations_[config.rule_id] = config;
        }

        THEMIS_INFO("Imported {} expirations",static_cast<int>(expirations_.size()));
        return true;

    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to import expirations: {}", e.what());
        return false;
    }
}

// ========== NotificationManager::Notification Implementation ==========

nlohmann::json NotificationManager::Notification::toJson() const {
    nlohmann::json j;
    j["notification_id"]   = notification_id;
    j["notification_type"] = notification_type;
    j["recipient"]         = recipient;
    j["subject"]           = subject;
    j["message"]           = message;
    j["created_at"]        = created_at;
    j["sent"]              = sent;
    j["sent_at"]           = sent_at;
    return j;
}

// ========== NotificationManager::NotificationConfig Implementation ==========

nlohmann::json NotificationManager::NotificationConfig::toJson() const {
    nlohmann::json j;
    j["email_enabled"]   = email_enabled;
    j["smtp_server"]     = smtp_server;
    j["smtp_port"]       = smtp_port;
    j["smtp_username"]   = smtp_username;
    j["smtp_password"]   = smtp_password;
    j["from_email"]      = from_email;
    j["webhook_enabled"] = webhook_enabled;
    j["webhook_url"]     = webhook_url;
    j["webhook_secret"]  = webhook_secret;
    return j;
}

NotificationManager::NotificationConfig NotificationManager::NotificationConfig::fromJson(const nlohmann::json &j) {
    NotificationConfig config = {};
    if (j.contains("email_enabled")) {
        config.email_enabled = j["email_enabled"].get<bool>();
    }
    if (j.contains("smtp_server")) {
        config.smtp_server = j["smtp_server"].get<std::string>();
    }
    if (j.contains("smtp_port")) {
        config.smtp_port = j["smtp_port"].get<int>();
    }
    if (j.contains("smtp_username")) {
        config.smtp_username = j["smtp_username"].get<std::string>();
    }
    if (j.contains("smtp_password")) {
        config.smtp_password = j["smtp_password"].get<std::string>();
    }
    if (j.contains("from_email")) {
        config.from_email = j["from_email"].get<std::string>();
    }
    if (j.contains("webhook_enabled")) {
        config.webhook_enabled = j["webhook_enabled"].get<bool>();
    }
    if (j.contains("webhook_url")) {
        config.webhook_url = j["webhook_url"].get<std::string>();
    }
    if (j.contains("webhook_secret")) {
        config.webhook_secret = j["webhook_secret"].get<std::string>();
    }
    return config;
}

// ========== NotificationManager Implementation ==========

std::string NotificationManager::generateNotificationId() const {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::ostringstream oss = {};
    oss << "notif-" << std::hex << dis(gen);
    return oss.str();
}

void NotificationManager::configure(const NotificationConfig &config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;

    THEMIS_INFO("Configured notification manager: email={}, webhook={}", // NOPII: logging boolean flags, not addresses
                config_.email_enabled, config_.webhook_enabled);
}

NotificationManager::NotificationConfig NotificationManager::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

bool NotificationManager::notifyReviewDue(const std::string &recipient, const PolicyReview &review) {
    std::ostringstream subject = {};
    subject << "Policy Review Due: " << review.rule_id;

    std::ostringstream message = {};
    message << "A policy review is due for rule: " << review.rule_id << "\n"
            << "Review ID: " << review.review_id << "\n"
            << "Reviewer: " << review.reviewer << "\n"
            << "Due Date: " << review.due_date << "\n"
            << "Please complete the review at your earliest convenience.";

    createNotification("review_due", recipient, subject.str(), message.str());
    return true;
}

bool NotificationManager::notifyReviewOverdue(const std::string &recipient, const PolicyReview &review,
                                              int days_overdue) {
    std::ostringstream subject = {};
    subject << "OVERDUE: Policy Review for " << review.rule_id;

    std::ostringstream message = {};
    message << "A policy review is OVERDUE for rule: " << review.rule_id << "\n"
            << "Review ID: " << review.review_id << "\n"
            << "Reviewer: " << review.reviewer << "\n"
            << "Due Date: " << review.due_date << "\n"
            << "Days Overdue: " << days_overdue << "\n"
            << "Please complete this review immediately.";

    createNotification("review_overdue", recipient, subject.str(), message.str());
    return true;
}

bool NotificationManager::notifyExpirationWarning(const std::string &recipient,
                                                  const PolicyExpiration::ExpirationWarning &warning) {
    std::ostringstream subject = {};
    subject << "Policy Expiration Warning: " << warning.rule_id;

    std::ostringstream message = {};
    message << "Policy rule " << warning.rule_id << " is expiring soon.\n"
            << "Expiration Date: " << warning.expiration_date << "\n"
            << "Days Until Expiration: " << warning.days_until_expiration << "\n"
            << "Severity: " << warning.severity << "\n"
            << "Please review and extend if necessary.";

    createNotification("expiration_warning", recipient, subject.str(), message.str());
    return true;
}

bool NotificationManager::notifyRuleExpired(const std::string &recipient, const std::string &rule_id) {
    std::ostringstream subject = {};
    subject << "Policy Rule Expired: " << rule_id;

    std::ostringstream message = {};
    message << "Policy rule " << rule_id << " has expired and has been disabled.\n"
            << "Please review the rule and update or remove it as appropriate.";

    createNotification("rule_expired", recipient, subject.str(), message.str());
    return true;
}

std::string NotificationManager::createNotification(const std::string &type, const std::string &recipient,
                                                    const std::string &subject, const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex_);

    Notification notification;
    notification.notification_id   = generateNotificationId();
    notification.notification_type = type;
    notification.recipient         = recipient;
    notification.subject           = subject;
    notification.message           = message;
    notification.created_at        = getCurrentTimeMs();
    notification.sent              = false;
    notification.sent_at           = 0;

    notifications_[notification.notification_id] = notification;

    THEMIS_DEBUG("Created notification {} of type {} for {}", notification.notification_id, type, recipient);

    // Attempt to send immediately
    if (config_.email_enabled || config_.webhook_enabled) {
        sendEmail(notification);
        sendWebhook(notification);
    }

    return notification.notification_id;
}

std::vector<NotificationManager::Notification> NotificationManager::getPendingNotifications() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Notification> result;

    for (const auto &pair : notifications_) {
        if (!pair.second.sent) {
            result.push_back(pair.second);
        }
    }

    return result;
}

void NotificationManager::markAsSent(const std::string &notification_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = notifications_.find(notification_id);
    if (it != notifications_.end()) {
        it->second.sent    = true;
        it->second.sent_at = getCurrentTimeMs();

        THEMIS_DEBUG("Marked notification {} as sent", notification_id);
    }
}

std::vector<NotificationManager::Notification> NotificationManager::getNotificationHistory(int64_t since) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Notification> result;

    for (const auto &pair : notifications_) {
        if (pair.second.created_at >= since) {
            result.push_back(pair.second);
        }
    }

    // Sort by created_at descending
    std::sort(result.begin(), result.end(),
              [](const Notification &a, const Notification &b) { return a.created_at > b.created_at; });

    return result;
}

bool NotificationManager::sendEmail(const Notification &notification) {
    if (!config_.email_enabled) {
        return false;
    }

    // Simulate email sending – recipient is PII (email address), redact before logging.
    auto safe_recipient = themis::security::PIIRedactionPolicy::get().redactForLog(notification.recipient);
    THEMIS_INFO("Simulated email to {}: {} - {}", safe_recipient, notification.subject,
                notification.message); // NOPII: safe_recipient is already redacted above

    return true;
}

bool NotificationManager::sendWebhook(const Notification &notification) {
    if (!config_.webhook_enabled) {
        return false;
    }

    // Simulate webhook sending
    THEMIS_INFO("Simulated webhook to {}: {}", config_.webhook_url, notification.toJson().dump());

    return true;
}

} // namespace governance
} // namespace themis
