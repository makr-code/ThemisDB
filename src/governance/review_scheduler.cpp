/**
 * @file review_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/review_scheduler.h"

#include <chrono>
#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== ReviewRequest Implementation ==========

nlohmann::json ReviewRequest::toJson() const {
    nlohmann::json j;
    j["review_id"]    = review_id;
    j["rule_id"]      = rule_id;
    j["requester"]    = requester;
    j["requested_at"] = requested_at;
    j["due_date"]     = due_date;
    j["status"]       = status;
    j["reviewer"]     = reviewer;
    j["reviewed_at"]  = reviewed_at;
    j["comments"]     = comments;
    return j;
}

ReviewRequest ReviewRequest::fromJson(const nlohmann::json &j) {
    ReviewRequest r;
    if (j.contains("review_id")) {
        r.review_id = j["review_id"].get<std::string>();
    }
    if (j.contains("rule_id")) {
        r.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("requester")) {
        r.requester = j["requester"].get<std::string>();
    }
    if (j.contains("requested_at")) {
        r.requested_at = j["requested_at"].get<std::int64_t>();
    }
    if (j.contains("due_date")) {
        r.due_date = j["due_date"].get<std::int64_t>();
    }
    if (j.contains("status")) {
        r.status = j["status"].get<std::string>();
    }
    if (j.contains("reviewer")) {
        r.reviewer = j["reviewer"].get<std::string>();
    }
    if (j.contains("reviewed_at")) {
        r.reviewed_at = j["reviewed_at"].get<std::int64_t>();
    }
    if (j.contains("comments")) {
        r.comments = j["comments"].get<std::string>();
    }
    return r;
}

// ========== ReviewSchedule Implementation ==========

nlohmann::json ReviewSchedule::toJson() const {
    nlohmann::json j;
    j["rule_id"]            = rule_id;
    j["review_period_days"] = review_period_days;
    j["last_review_date"]   = last_review_date;
    j["next_review_date"]   = next_review_date;
    j["auto_schedule"]      = auto_schedule;
    return j;
}

ReviewSchedule ReviewSchedule::fromJson(const nlohmann::json &j) {
    ReviewSchedule s;
    if (j.contains("rule_id")) {
        s.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("review_period_days")) {
        s.review_period_days = j["review_period_days"].get<int>();
    }
    if (j.contains("last_review_date")) {
        s.last_review_date = j["last_review_date"].get<std::int64_t>();
    }
    if (j.contains("next_review_date")) {
        s.next_review_date = j["next_review_date"].get<std::int64_t>();
    }
    if (j.contains("auto_schedule")) {
        s.auto_schedule = j["auto_schedule"].get<bool>();
    }
    return s;
}

// ========== ReviewScheduler Implementation ==========

ReviewScheduler::ReviewScheduler(std::shared_ptr<PolicyManager> policy_manager)
    : policy_manager_(std::move(policy_manager)) {
    if (!policy_manager_) {
        THEMIS_WARN("ReviewScheduler created with null PolicyManager");
    }
}

void ReviewScheduler::configureReviewSchedule(const std::string &rule_id, int period_days) {
    ReviewSchedule schedule;
    schedule.rule_id            = rule_id;
    schedule.review_period_days = period_days;
    schedule.last_review_date   = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    schedule.next_review_date   = calculateNextReviewDate(rule_id);

    schedules_[rule_id] = schedule;

    THEMIS_INFO("Configured review schedule for rule {} with {} day period", rule_id, period_days);
}

std::string ReviewScheduler::createReviewRequest(const std::string &rule_id, const std::string &requester,
                                                 int due_days) {
    ReviewRequest request;
    request.review_id    = generateReviewId();
    request.rule_id      = rule_id;
    request.requester    = requester;
    request.requested_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    request.due_date     = request.requested_at + (due_days * 86400); // days to seconds
    request.status       = "pending";

    reviews_[request.review_id] = request;

    THEMIS_INFO("Created review request {} for rule {}", request.review_id, rule_id);

    return request.review_id;
}

void ReviewScheduler::approveReview(const std::string &review_id, const std::string &reviewer,
                                    const std::string &comments) {
    auto it = reviews_.find(review_id);
    if (it == reviews_.end()) {
        THEMIS_ERROR("Review request not found: {}", review_id);
        return;
    }

    it->second.status      = "approved";
    it->second.reviewer    = reviewer;
    it->second.reviewed_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    it->second.comments    = comments;

    // Update schedule if exists
    auto sched_it = schedules_.find(it->second.rule_id);
    if (sched_it != schedules_.end()) {
        sched_it->second.last_review_date = it->second.reviewed_at;
        sched_it->second.next_review_date = calculateNextReviewDate(it->second.rule_id);
    }

    THEMIS_INFO("Approved review {} for rule {} by {}", review_id, it->second.rule_id, reviewer);
}

void ReviewScheduler::rejectReview(const std::string &review_id, const std::string &reviewer,
                                   const std::string &comments) {
    auto it = reviews_.find(review_id);
    if (it == reviews_.end()) {
        THEMIS_ERROR("Review request not found: {}", review_id);
        return;
    }

    it->second.status      = "rejected";
    it->second.reviewer    = reviewer;
    it->second.reviewed_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    it->second.comments    = comments;

    THEMIS_INFO("Rejected review {} for rule {} by {}: {}", review_id, it->second.rule_id, reviewer, comments);
}

std::vector<ReviewRequest> ReviewScheduler::getPendingReviews() const {
    std::vector<ReviewRequest> pending;

    for (const auto &[id, review] : reviews_) {
        if (review.status == "pending") {
            pending.push_back(review);
        }
    }

    return pending;
}

std::vector<ReviewRequest> ReviewScheduler::getOverdueReviews() const {
    std::vector<ReviewRequest> overdue;

    std::int64_t now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;

    for (const auto &[id, review] : reviews_) {
        if (review.status == "pending" && review.due_date < now) {
            overdue.push_back(review);
        }
    }

    return overdue;
}

std::vector<ReviewRequest> ReviewScheduler::getReviewHistory(const std::string &rule_id) const {
    std::vector<ReviewRequest> history;

    for (const auto &[id, review] : reviews_) {
        if (review.rule_id == rule_id) {
            history.push_back(review);
        }
    }

    return history;
}

std::vector<std::string> ReviewScheduler::checkReviewsDue() const {
    std::vector<std::string> due_rules;

    std::int64_t now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;

    for (const auto &[rule_id, schedule] : schedules_) {
        if (schedule.auto_schedule && schedule.next_review_date <= now) {
            due_rules.push_back(rule_id);
        }
    }

    return due_rules;
}

nlohmann::json ReviewScheduler::getExpirationInfo(const std::string &rule_id) const {
    nlohmann::json info;
    info["rule_id"] = rule_id;

    auto sched_it = schedules_.find(rule_id);
    if (sched_it != schedules_.end()) {
        info["has_schedule"]       = true;
        info["review_period_days"] = sched_it->second.review_period_days;
        info["last_review_date"]   = sched_it->second.last_review_date;
        info["next_review_date"]   = sched_it->second.next_review_date;

        std::int64_t now               = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
        std::int64_t days_until_review = (sched_it->second.next_review_date - now) / 86400;
        info["days_until_review"]      = days_until_review;
        info["is_overdue"]             = (days_until_review < 0);
    } else {
        info["has_schedule"] = false;
    }

    return info;
}

nlohmann::json ReviewScheduler::exportReviews() const {
    nlohmann::json j;

    // Export schedules
    nlohmann::json schedules_array = nlohmann::json::array();
    for (const auto &[id, schedule] : schedules_) {
        schedules_array.push_back(schedule.toJson());
    }
    j["schedules"] = schedules_array;

    // Export reviews
    nlohmann::json reviews_array = nlohmann::json::array();
    for (const auto &[id, review] : reviews_) {
        reviews_array.push_back(review.toJson());
    }
    j["reviews"] = reviews_array;

    return j;
}

bool ReviewScheduler::importReviews(const nlohmann::json &j) {
    try {
        // Import schedules
        if (j.contains("schedules") && j["schedules"].is_array()) {
            for (const auto &sched_json : j["schedules"]) {
                auto schedule                = ReviewSchedule::fromJson(sched_json);
                schedules_[schedule.rule_id] = schedule;
            }
        }

        // Import reviews
        if (j.contains("reviews") && j["reviews"].is_array()) {
            for (const auto &review_json : j["reviews"]) {
                auto review                = ReviewRequest::fromJson(review_json);
                reviews_[review.review_id] = review;
            }
        }

        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Error importing reviews: {}", e.what());
        return false;
    }
}

std::string ReviewScheduler::generateReviewId() const {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss << "review_" << now;
    return oss.str();
}

std::int64_t ReviewScheduler::calculateNextReviewDate(const std::string &rule_id) const {
    auto sched_it = schedules_.find(rule_id);
    if (sched_it == schedules_.end()) {
        // Default: 90 days from now
        return std::chrono::system_clock::now().time_since_epoch().count() / 1000000000 + (90 * 86400);
    }

    std::int64_t base_date = sched_it->second.last_review_date;
    if (base_date == 0) {
        base_date = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    }

    return base_date + (sched_it->second.review_period_days * 86400);
}

} // namespace governance
} // namespace themis
