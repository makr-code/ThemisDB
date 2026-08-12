/**
 * @file feedback_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "llm/lora_framework/lora_feedback_storage.h"

namespace themis {
namespace prompt_engineering {
class FeedbackCollector;
}
namespace rag::learning {
class ContinuousLearningOrchestrator;
}
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

/**
 * @brief Feedback API Handler for LoRA Adapter Feedback
 * 
 * Implements RESTful endpoints for feedback operations:
 * - POST   /api/feedback           - Create new feedback
 * - GET    /api/feedback           - List/filter feedback
 * - GET    /api/feedback/{id}      - Get specific feedback
 * - PUT    /api/feedback/{id}      - Update feedback
 * - DELETE /api/feedback/{id}      - Delete feedback
 * - GET    /api/feedback/adapter/{adapter_id} - Get feedback for adapter
 * - GET    /api/feedback/stats     - Get feedback statistics
 */
class FeedbackAPIHandler {
public:
    /**
     * @brief Construct FeedbackAPIHandler
     * @param storage_service Feedback storage service
     */
    explicit FeedbackAPIHandler(
        std::shared_ptr<llm::lora::FeedbackStorageService> storage_service
    );
    
    ~FeedbackAPIHandler() = default;
    
    /**
     * @brief Handle POST /api/feedback - Create feedback
     * @param req HTTP request
     * @return HTTP response
     */
    http::response<http::string_body> handleCreateFeedback(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle GET /api/feedback - List feedback
     * @param req HTTP request
     * @return HTTP response
     */
    http::response<http::string_body> handleListFeedback(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle GET /api/feedback/{id} - Get feedback
     * @param req HTTP request
     * @param id Feedback ID
     * @return HTTP response
     */
    http::response<http::string_body> handleGetFeedback(
        const http::request<http::string_body>& req,
        const std::string& id
    );
    
    /**
     * @brief Handle PUT /api/feedback/{id} - Update feedback
     * @param req HTTP request
     * @param id Feedback ID
     * @return HTTP response
     */
    http::response<http::string_body> handleUpdateFeedback(
        const http::request<http::string_body>& req,
        const std::string& id
    );
    
    /**
     * @brief Handle DELETE /api/feedback/{id} - Delete feedback
     * @param req HTTP request
     * @param id Feedback ID
     * @return HTTP response
     */
    http::response<http::string_body> handleDeleteFeedback(
        const http::request<http::string_body>& req,
        const std::string& id
    );
    
    /**
     * @brief Handle GET /api/feedback/adapter/{adapter_id} - Get adapter feedback
     * @param req HTTP request
     * @param adapter_id Adapter ID
     * @return HTTP response
     */
    http::response<http::string_body> handleGetAdapterFeedback(
        const http::request<http::string_body>& req,
        const std::string& adapter_id
    );
    
    /**
     * @brief Handle GET /api/feedback/stats - Get feedback statistics
     * @param req HTTP request
     * @return HTTP response
     */
    http::response<http::string_body> handleGetStatistics(
        const http::request<http::string_body>& req
    );

    void setLiveFeedbackCollector(
        std::shared_ptr<themis::prompt_engineering::FeedbackCollector> feedback_collector) {
        feedback_collector_ = std::move(feedback_collector);
    }

    void setLearningOrchestrator(
        std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator> orchestrator) {
        learning_orchestrator_ = std::move(orchestrator);
    }

private:
    std::shared_ptr<llm::lora::FeedbackStorageService> storage_service_;
    std::shared_ptr<themis::prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator> learning_orchestrator_;
    
    // Helper methods
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> makeJsonResponse(
        http::status status,
        const json& body,
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& error,
        const http::request<http::string_body>& req
    );
    
    llm::lora::FeedbackFilter parseFilterFromQuery(const std::string& query) const;
};

} // namespace server
} // namespace themis
