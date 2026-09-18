/**
 * @file feedback_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class FeedbackAPIHandler {
public:
    /**
     * @brief Feedback APIHandler.
     * @param[in] storage_service Input parameter.
     * @return Return value.
     */
    explicit FeedbackAPIHandler(
        std::shared_ptr<llm::lora::FeedbackStorageService> storage_service
    );
    
    ~FeedbackAPIHandler() = default;
    
    /**
     * @brief Handle Create Feedback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateFeedback(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle List Feedback.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListFeedback(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Get Feedback.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetFeedback(
        const http::request<http::string_body>& req,
        const std::string& id
    );
    
    /**
     * @brief Handle Update Feedback.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateFeedback(
        const http::request<http::string_body>& req,
        const std::string& id
    );
    
    /**
     * @brief Handle Delete Feedback.
     * @param[in] req Input parameter.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteFeedback(
        const http::request<http::string_body>& req,
        const std::string& id
    );
    
    /**
     * @brief Handle Get Adapter Feedback.
     * @param[in] req Input parameter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetAdapterFeedback(
        const http::request<http::string_body>& req,
        const std::string& adapter_id
    );
    
    /**
     * @brief Handle Get Statistics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStatistics(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Set Live Feedback Collector.
     * @param[in] feedback_collector Input parameter.
     * @details Calls: std::move().
     */
    void setLiveFeedbackCollector(
        std::shared_ptr<themis::prompt_engineering::FeedbackCollector> feedback_collector) {
        feedback_collector_ = std::move(feedback_collector);
    }

    /**
     * @brief Set Learning Orchestrator.
     * @param[in] orchestrator Input parameter.
     * @details Calls: std::move().
     */
    void setLearningOrchestrator(
        std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator> orchestrator) {
        learning_orchestrator_ = std::move(orchestrator);
    }

private:
    std::shared_ptr<llm::lora::FeedbackStorageService> storage_service_;
    std::shared_ptr<themis::prompt_engineering::FeedbackCollector> feedback_collector_;
    std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator> learning_orchestrator_;
    
    // Helper methods
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
    );
    
    /**
     * @brief Make Json Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeJsonResponse(
        http::status status,
        const json& body,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] error Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& error,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Parse Filter From Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    llm::lora::FeedbackFilter parseFilterFromQuery(const std::string& query) const;
};

} // namespace server
} // namespace themis
