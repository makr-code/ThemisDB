/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_engineering_api_handler.h                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     145                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_engineering_api_handler.h
 * @brief API handler for advanced prompt engineering operations
 * 
 * Handles optimization, A/B testing, feedback, version control, and statistics
 * for the prompt engineering system (Phases 3-6).
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

/**
 * @brief Handler for Prompt Engineering Advanced Operations
 * 
 * Endpoints:
 * - POST /api/v1/prompt_engineering/optimize - Trigger manual optimization
 * - GET /api/v1/prompt_engineering/ab_tests - List active A/B tests
 * - GET /api/v1/prompt_engineering/ab_tests/:id - Get A/B test details
 * - POST /api/v1/prompt_engineering/feedback - Submit feedback
 * - GET /api/v1/prompt_engineering/stats - Get system statistics
 * - GET /api/v1/prompt_engineering/history/:id - Get optimization history
 * - GET /api/v1/prompt_engineering/versions/:id - Get version history
 * - POST /api/v1/prompt_engineering/rollback - Rollback to previous version
 */
class PromptEngineeringApiHandler {
public:
    /**
     * @brief Construct handler with all prompt engineering components
     */
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
    http::response<http::string_body> handleOptimize(
        const http::request<http::string_body>& req);
    
    // A/B testing endpoints
    http::response<http::string_body> handleListABTests(
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleGetABTest(
        const http::request<http::string_body>& req);
    
    // Feedback endpoints
    http::response<http::string_body> handleSubmitFeedback(
        const http::request<http::string_body>& req);
    
    // Statistics endpoints
    http::response<http::string_body> handleGetStats(
        const http::request<http::string_body>& req);
    
    // History endpoints
    http::response<http::string_body> handleGetHistory(
        const http::request<http::string_body>& req);
    
    // Version control endpoints
    http::response<http::string_body> handleGetVersions(
        const http::request<http::string_body>& req);
    
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
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    
    http::response<http::string_body> makeErrorResponse(
        http::status status, 
        const std::string& message, 
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> makeResponse(
        http::status status, 
        const std::string& body, 
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
