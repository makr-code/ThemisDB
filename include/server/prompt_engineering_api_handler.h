/**
 * @file prompt_engineering_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "server/auth_middleware.h"
#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace prompt_engineering {
class PromptManager;
class PromptOptimizer;
class PromptPerformanceTracker;
class SelfImprovementOrchestrator;
class FeedbackCollector;
class PromptVersionControl;
class PromptEngineeringIntegration;
}

namespace server {

class PromptEngineeringApiHandler {
public:
    PromptEngineeringApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<prompt_engineering::PromptManager> manager,
        std::shared_ptr<prompt_engineering::PromptOptimizer> optimizer,
        std::shared_ptr<prompt_engineering::PromptPerformanceTracker> tracker,
        std::shared_ptr<prompt_engineering::SelfImprovementOrchestrator> orchestrator,
        std::shared_ptr<prompt_engineering::FeedbackCollector> feedback_collector,
        std::shared_ptr<prompt_engineering::PromptVersionControl> version_control,
        std::shared_ptr<prompt_engineering::PromptEngineeringIntegration> integration,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    // Optimization endpoints
    /**
     * @brief Handle Optimize.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOptimize(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle List ABTests.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListABTests(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Get ABTest.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetABTest(
        const http::request<http::string_body>& req);
    
    // Feedback endpoints
    /**
     * @brief Handle Submit Feedback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSubmitFeedback(
        const http::request<http::string_body>& req);
    
    // Statistics endpoints
    /**
     * @brief Handle Get Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStats(
        const http::request<http::string_body>& req);
    
    // History endpoints
    /**
     * @brief Handle Get History.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetHistory(
        const http::request<http::string_body>& req);
    
    // Version control endpoints
    /**
     * @brief Handle Get Versions.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetVersions(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Rollback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRollback(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<prompt_engineering::PromptManager> manager_;
    std::shared_ptr<prompt_engineering::PromptOptimizer> optimizer_;
    std::shared_ptr<prompt_engineering::PromptPerformanceTracker> tracker_;
    std::shared_ptr<prompt_engineering::SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<prompt_engineering::PromptVersionControl> version_control_;
    std::shared_ptr<prompt_engineering::PromptEngineeringIntegration> integration_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods
    /**
     * @brief Extract Path Param.
     * @param[in] target Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    
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
        const http::request<http::string_body>& req);
    
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
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
