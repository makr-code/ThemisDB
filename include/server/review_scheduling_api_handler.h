/**
 * @file review_scheduling_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "governance/review_scheduler.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

/**
 * @brief Handler for Policy Review Scheduling API
 * 
 * This handler manages policy review endpoints:
 * - GET /policies/reviews/pending - List pending reviews
 * - POST /policies/reviews/:ruleId - Create review request
 * - POST /policies/reviews/:reviewId/approve - Approve review
 * - POST /policies/reviews/:reviewId/reject - Reject review
 * - GET /policies/rules/:id/expiration - Get expiration info
 */
class ReviewSchedulingApiHandler {
public:
    ReviewSchedulingApiHandler(
        std::shared_ptr<themis::governance::ReviewScheduler> scheduler,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    http::response<http::string_body> handleListPendingReviews(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleCreateReview(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    http::response<http::string_body> handleApproveReview(
        const http::request<http::string_body>& req,
        const std::string& review_id
    );
    
    http::response<http::string_body> handleRejectReview(
        const http::request<http::string_body>& req,
        const std::string& review_id
    );
    
    http::response<http::string_body> handleGetExpiration(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
private:
    std::shared_ptr<themis::governance::ReviewScheduler> scheduler_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role) const;
    
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    ) const;
    
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;
};

} // namespace server
} // namespace themis
