/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            review_scheduling_api_handler.h                    ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:37:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     100                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
