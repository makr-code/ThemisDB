#include "server/review_scheduling_api_handler.h"
#include "utils/logger.h"

namespace themis {
namespace server {

ReviewSchedulingApiHandler::ReviewSchedulingApiHandler(
    std::shared_ptr<themis::governance::ReviewScheduler> scheduler,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : scheduler_(std::move(scheduler))
    , auth_(std::move(auth))
{
    if (!scheduler_) {
        THEMIS_WARN("ReviewSchedulingApiHandler created with null ReviewScheduler");
    }
}

http::response<http::string_body> ReviewSchedulingApiHandler::handleListPendingReviews(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        
        auto pending = scheduler_->getPendingReviews();
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& review : pending) {
            json_array.push_back(review.toJson());
        }
        
        nlohmann::json response = {
            {"reviews", json_array},
            {"count", pending.size()}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error listing pending reviews: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ReviewSchedulingApiHandler::handleCreateReview(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string requester = body.value("requester", "system");
        int due_days = body.value("due_days", 7);
        
        std::string review_id = scheduler_->createReviewRequest(
            rule_id,
            requester,
            due_days
        );
        
        nlohmann::json response = {
            {"success", true},
            {"review_id", review_id},
            {"rule_id", rule_id},
            {"message", "Review request created successfully"}
        };
        
        return makeResponse(http::status::created, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error creating review for rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ReviewSchedulingApiHandler::handleApproveReview(
    const http::request<http::string_body>& req,
    const std::string& review_id
) {
    try {
        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string reviewer = body.value("reviewer", "unknown");
        std::string comments = body.value("comments", "");
        
        scheduler_->approveReview(review_id, reviewer, comments);
        
        nlohmann::json response = {
            {"success", true},
            {"review_id", review_id},
            {"message", "Review approved successfully"}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error approving review {}: {}", review_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ReviewSchedulingApiHandler::handleRejectReview(
    const http::request<http::string_body>& req,
    const std::string& review_id
) {
    try {
        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string reviewer = body.value("reviewer", "unknown");
        std::string comments = body.value("comments", "No reason provided");
        
        scheduler_->rejectReview(review_id, reviewer, comments);
        
        nlohmann::json response = {
            {"success", true},
            {"review_id", review_id},
            {"message", "Review rejected"}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error rejecting review {}: {}", review_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ReviewSchedulingApiHandler::handleGetExpiration(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        
        auto expiration_info = scheduler_->getExpirationInfo(rule_id);
        
        return makeResponse(http::status::ok, expiration_info.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting expiration info for rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

bool ReviewSchedulingApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) const {
    if (!auth_) {
        return true;
    }
    
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        return false;
    }
    
    return true; // Simplified for now
}

http::response<http::string_body> ReviewSchedulingApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) const {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ReviewSchedulingApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) const {
    nlohmann::json error = {
        {"error", message},
        {"status", static_cast<int>(status)}
    };
    return makeResponse(status, error.dump(2), req);
}

} // namespace server
} // namespace themis
