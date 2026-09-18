/**
 * @file review_scheduling_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ReviewSchedulingApiHandler {
public:
    ReviewSchedulingApiHandler(
        std::shared_ptr<themis::governance::ReviewScheduler> scheduler,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle List Pending Reviews.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListPendingReviews(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Create Review.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateReview(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle Approve Review.
     * @param[in] req Input parameter.
     * @param[in] review_id Identifier of the review.
     * @return Return value.
     */
    http::response<http::string_body> handleApproveReview(
        const http::request<http::string_body>& req,
        const std::string& review_id
    );
    
    /**
     * @brief Handle Reject Review.
     * @param[in] req Input parameter.
     * @param[in] review_id Identifier of the review.
     * @return Return value.
     */
    http::response<http::string_body> handleRejectReview(
        const http::request<http::string_body>& req,
        const std::string& review_id
    );
    
    /**
     * @brief Handle Get Expiration.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    http::response<http::string_body> handleGetExpiration(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
private:
    std::shared_ptr<themis::governance::ReviewScheduler> scheduler_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    /**
     * @brief Check Auth.
     * @param[in] req Input parameter.
     * @param[in] required_role Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role) const;
    
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    ) const;
    
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;
};

} // namespace server
} // namespace themis
