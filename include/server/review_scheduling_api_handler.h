/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            review_scheduling_api_handler.h                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:58:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     104                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
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
