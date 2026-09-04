/**
 * @file review_scheduling_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/review_scheduling_api_handler.h"
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"
#include "utils/tracing.h"

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
        THEMIS_WARN([[maybe_unused]] "ReviewSchedulingApiHandler created with null ReviewScheduler");
    }
}

http::response<http::string_body> ReviewSchedulingApiHandler::handleListPendingReviews(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleListPendingReviews");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        auto& scheduler = *scheduler_;
        auto pending = scheduler.getPendingReviews();
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& review : pending) {
            json_array.push_back(review.toJson());
        }
        
        nlohmann::json response = {
            {"reviews", json_array},
            {"count",static_cast<int>(pending.size())}
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
    auto span = Tracer::startSpan("handleCreateReview");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        auto& scheduler = *scheduler_;
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string requester = body.value("requester", "system");
        int due_days = body.value("due_days", 7);
        
        std::string review_id = scheduler.createReviewRequest(
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
    auto span = Tracer::startSpan("handleApproveReview");
    try {
        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        auto& scheduler = *scheduler_;
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string reviewer = body.value("reviewer", "unknown");
        std::string comments = body.value("comments", "");
        
        scheduler.approveReview(review_id, reviewer, comments);
        
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
    auto span = Tracer::startSpan([[maybe_unused]] "handleRejectReview");
    try {
        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        auto& scheduler = *scheduler_;
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string reviewer = body.value("reviewer", "unknown");
        std::string comments = body.value("comments", "No reason provided");
        
        scheduler.rejectReview(review_id, reviewer, comments);
        
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
    auto span = Tracer::startSpan("handleGetExpiration");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!scheduler_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ReviewScheduler not initialized", req);
        }
        auto& scheduler = *scheduler_;
        auto expiration_info = scheduler.getExpirationInfo(rule_id);
        
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
    // Backward compatibility: If no auth configured or disabled, allow access but log a warning
    // Production deployments should always enable authentication
    if (!auth_ || !auth_->isEnabled()) {
        THEMIS_WARN("AuthMiddleware not configured or disabled - allowing unauthenticated access to review scheduling endpoint (dev/test mode only)");
        return true;
    }
    auto& auth = *auth_;
    
    // Extract authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        THEMIS_WARN("Missing Authorization header for review scheduling endpoint");
        return false;
    }
    
    // Extract Bearer token
    const auto auth_value = std::string(auth_header.data(),static_cast<int>(auth_header.size()));
    auto token = AuthMiddleware::extractBearerToken(auth_value);
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for review scheduling endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    std::string required_scope = auth_scope_mapper::mapPolicyRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth.authorize(*token, required_scope);
    if (!auth_result.authorized) {
        THEMIS_WARN("Authorization failed for review scheduling endpoint - user: {}, required scope: {}, reason: {}",
            auth_result.user_id.empty() ? "unknown" : auth_result.user_id,
            required_scope,
            auth_result.reason.empty() ? "insufficient_scope" : auth_result.reason);
        return false;
    }
    
    THEMIS_DEBUG("Authorization successful for review scheduling endpoint - user: {}, scope: {}",
        auth_result.user_id, required_scope);
    return true;
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
