/**
 * @file feedback_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/feedback_api_handler.h"
#include "prompt_engineering/feedback_collector.h"
#include "rag/continuous_learning_orchestrator.h"
#include "utils/logger.h"
#include "utils/input_validator.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include "utils/tracing.h"

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxFeedbackIdentifierLength = 256;
constexpr size_t kMaxFeedbackFilterValueLength = 256;
constexpr size_t kMaxFeedbackQueryLimit = 1000;

bool isValidFeedbackIdentifier(const std::string& value, const bool allow_empty = false) {
    if (value.empty()) {
        return allow_empty;
    }

    themis::utils::InputValidator validator;
    return validator.validateStringLength(value, kMaxFeedbackIdentifierLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

bool isValidFeedbackFilterValue(const std::string& value) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(value, kMaxFeedbackFilterValueLength) &&
           validator.validateHeaderValue(value) &&
           validator.validatePathSegment(value);
}

themis::prompt_engineering::FeedbackType toCollectorFeedbackType(
    const llm::lora::Feedback& feedback) {
    if (feedback.training_category == "negative" || feedback.rating <= 2) {
        return themis::prompt_engineering::FeedbackType::USER_NEGATIVE;
    }
    return themis::prompt_engineering::FeedbackType::USER_POSITIVE;
}

double toCollectorSeverity(const llm::lora::Feedback& feedback) {
    if (feedback.rating <= 0) {
        return 0.5;
    }
    const double normalized = std::clamp(static_cast<double>(feedback.rating), 1.0, 5.0) / 5.0;
    return 1.0 - normalized;
}

std::string toCollectorPromptId(const llm::lora::Feedback& feedback) {
    if (!feedback.adapter_id.empty()) {
        return feedback.adapter_id;
    }
    if (feedback.model_response_id.has_value() && !feedback.model_response_id->empty()) {
        return *feedback.model_response_id;
    }
    return "llm-feedback";
}

} // namespace

// ═══════════════════════════════════════════════════════════
// FeedbackAPIHandler Implementation
// ═══════════════════════════════════════════════════════════

FeedbackAPIHandler::FeedbackAPIHandler(
    std::shared_ptr<llm::lora::FeedbackStorageService> storage_service
)
    : storage_service_(storage_service)
{
    if (!storage_service_) {
        throw std::runtime_error("FeedbackAPIHandler: storage_service is required");
    }
    spdlog::info("FeedbackAPIHandler initialized");
}

http::response<http::string_body> FeedbackAPIHandler::handleCreateFeedback(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleCreateFeedback");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        // Parse request body
        auto body_json = json::parse(req.body());
        
        // Create feedback from JSON
        auto feedback = llm::lora::Feedback::fromJSON(body_json);

        if (!isValidFeedbackIdentifier(feedback.adapter_id, true) ||
            !isValidFeedbackIdentifier(feedback.user_id, true) ||
            (feedback.model_response_id.has_value() &&
             !isValidFeedbackIdentifier(*feedback.model_response_id))) {
            return makeErrorResponse(
                http::status::bad_request,
                "Feedback contains invalid identifier fields",
                req
            );
        }
        
        // Store feedback
        auto stored = storage_service.createFeedback(feedback);
        
        if (!stored) {
            return makeErrorResponse(
                http::status::bad_request,
                "Feedback validation failed",
                req
            );
        }

        if (feedback_collector_) {
            try {
                json metadata = {
                    {"adapter_id", feedback.adapter_id},
                    {"user_id", feedback.user_id},
                    {"rating", feedback.rating},
                    {"training_category", feedback.training_category},
                    {"flagged_for_training", feedback.flagged_for_training},
                    {"is_cached_response", feedback.is_cached_response},
                    {"training_weight", feedback.training_weight}
                };
                if (feedback.model_response_id.has_value()) {
                    metadata["model_response_id"] = *feedback.model_response_id;
                }
                if (feedback.cache_key.has_value()) {
                    metadata["cache_key"] = *feedback.cache_key;
                }

                feedback_collector_->recordFeedback(
                    toCollectorPromptId(feedback),
                    feedback.prompt,
                    feedback.response,
                    toCollectorFeedbackType(feedback),
                    feedback.feedback_text,
                    toCollectorSeverity(feedback),
                    metadata
                );

                if (learning_orchestrator_) {
                    learning_orchestrator_->triggerLoop4AdapterImprovement();
                }
            } catch (const std::exception& e) {
                spdlog::warn("Failed to mirror feedback into live learning collector: {}", e.what());
            }
        }
         
        // Return created feedback
        return makeJsonResponse(
            http::status::created,
            stored->toJSON(),
            req
        );
        
    } catch (const json::parse_error& e) {
        return makeErrorResponse(
            http::status::bad_request,
            std::string("Invalid JSON: ") + e.what(),
            req
        );
    } catch (const std::exception& e) {
        spdlog::error("Error creating feedback: {}", e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

http::response<http::string_body> FeedbackAPIHandler::handleListFeedback(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleListFeedback");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        // Parse query parameters
        std::string target(req.target());
        size_t query_pos = target.find('?');
        std::string query = (query_pos != std::string::npos) 
            ? target.substr(query_pos + 1) 
            : "";
        
        auto filter = parseFilterFromQuery(query);
        
        // Get feedback list
        auto feedback_list = storage_service.listFeedback(filter);
        
        // Build response
        json response;
        response["count"] = feedback_list.size();
        response["feedback"] = json::array();
        
        for (const auto& fb : feedback_list) {
            response["feedback"].push_back(fb.toJSON());
        }
        
        return makeJsonResponse(http::status::ok, response, req);
    
    } catch (const std::invalid_argument& e) {
        return makeErrorResponse(
            http::status::bad_request,
            e.what(),
            req
        );
    } catch (const std::out_of_range& e) {
        return makeErrorResponse(
            http::status::bad_request,
            e.what(),
            req
        );
        
    } catch (const std::exception& e) {
        spdlog::error("Error listing feedback: {}", e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

http::response<http::string_body> FeedbackAPIHandler::handleGetFeedback(
    const http::request<http::string_body>& req,
    const std::string& id
) {
    auto span = Tracer::startSpan("handleGetFeedback");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        if (!isValidFeedbackIdentifier(id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid feedback id",
                req
            );
        }

        auto feedback = storage_service.getFeedback(id);
        
        if (!feedback) {
            return makeErrorResponse(
                http::status::not_found,
                "Feedback not found",
                req
            );
        }
        
        return makeJsonResponse(http::status::ok, feedback->toJSON(), req);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting feedback {}: {}", id, e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

http::response<http::string_body> FeedbackAPIHandler::handleUpdateFeedback(
    const http::request<http::string_body>& req,
    const std::string& id
) {
    auto span = Tracer::startSpan("handleUpdateFeedback");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        if (!isValidFeedbackIdentifier(id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid feedback id",
                req
            );
        }

        // Parse request body
        auto body_json = json::parse(req.body());
        
        // Create feedback from JSON
        auto feedback = llm::lora::Feedback::fromJSON(body_json);

        if (!isValidFeedbackIdentifier(feedback.adapter_id, true) ||
            !isValidFeedbackIdentifier(feedback.user_id, true) ||
            (feedback.model_response_id.has_value() &&
             !isValidFeedbackIdentifier(*feedback.model_response_id))) {
            return makeErrorResponse(
                http::status::bad_request,
                "Feedback contains invalid identifier fields",
                req
            );
        }
        
        // Update feedback
        bool success = storage_service.updateFeedback(id, feedback);
        
        if (!success) {
            return makeErrorResponse(
                http::status::not_found,
                "Feedback not found",
                req
            );
        }
        
        // Get updated feedback
        auto updated = storage_service.getFeedback(id);
        return makeJsonResponse(http::status::ok, updated->toJSON(), req);
        
    } catch (const json::parse_error& e) {
        return makeErrorResponse(
            http::status::bad_request,
            std::string("Invalid JSON: ") + e.what(),
            req
        );
    } catch (const std::exception& e) {
        spdlog::error("Error updating feedback {}: {}", id, e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

http::response<http::string_body> FeedbackAPIHandler::handleDeleteFeedback(
    const http::request<http::string_body>& req,
    const std::string& id
) {
    auto span = Tracer::startSpan("handleDeleteFeedback");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        if (!isValidFeedbackIdentifier(id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid feedback id",
                req
            );
        }

        bool success = storage_service.deleteFeedback(id);
        
        if (!success) {
            return makeErrorResponse(
                http::status::not_found,
                "Feedback not found",
                req
            );
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Feedback deleted successfully";
        
        return makeJsonResponse(http::status::ok, response, req);
        
    } catch (const std::exception& e) {
        spdlog::error("Error deleting feedback {}: {}", id, e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

http::response<http::string_body> FeedbackAPIHandler::handleGetAdapterFeedback(
    const http::request<http::string_body>& req,
    const std::string& adapter_id
) {
    auto span = Tracer::startSpan("handleGetAdapterFeedback");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        if (!isValidFeedbackIdentifier(adapter_id)) {
            return makeErrorResponse(
                http::status::bad_request,
                "Invalid adapter id",
                req
            );
        }

        // Parse limit from query if provided
        std::string target(req.target());
        size_t query_pos = target.find('?');
        size_t limit = 100;
        
        if (query_pos != std::string::npos) {
            std::string query = target.substr(query_pos + 1);
            // Simple parsing for limit parameter
            size_t limit_pos = query.find("limit=");
            if (limit_pos != std::string::npos) {
                limit = std::stoul(query.substr(limit_pos + 6));
                if (limit == 0 || limit > kMaxFeedbackQueryLimit) {
                    return makeErrorResponse(
                        http::status::bad_request,
                        "Invalid limit",
                        req
                    );
                }
            }
        }
        
        auto feedback_list = storage_service.getFeedbackForAdapter(adapter_id, limit);
        
        json response;
        response["adapter_id"] = adapter_id;
        response["count"] = feedback_list.size();
        response["feedback"] = json::array();
        
        for (const auto& fb : feedback_list) {
            response["feedback"].push_back(fb.toJSON());
        }
        
        return makeJsonResponse(http::status::ok, response, req);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting adapter feedback {}: {}", adapter_id, e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

http::response<http::string_body> FeedbackAPIHandler::handleGetStatistics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetStatistics");
    try {
        if (!storage_service_) {
            return makeErrorResponse(
                http::status::internal_server_error,
                "Feedback storage service unavailable",
                req
            );
        }
        auto& storage_service = *storage_service_;

        // Parse adapter_id from query if provided
        std::string target(req.target());
        size_t query_pos = target.find('?');
        std::optional<std::string> adapter_id;
        
        if (query_pos != std::string::npos) {
            std::string query = target.substr(query_pos + 1);
            size_t adapter_pos = query.find("adapter_id=");
            if (adapter_pos != std::string::npos) {
                adapter_id = query.substr(adapter_pos + 11);
                // Remove any trailing query params
                size_t end_pos = adapter_id->find('&');
                if (end_pos != std::string::npos) {
                    *adapter_id = adapter_id->substr(0, end_pos);
                }
                if (!isValidFeedbackIdentifier(*adapter_id, true)) {
                    return makeErrorResponse(
                        http::status::bad_request,
                        "Invalid adapter_id",
                        req
                    );
                }
            }
        }
        
        auto stats = storage_service.getStatistics(adapter_id);
        
        return makeJsonResponse(http::status::ok, stats, req);
        
    } catch (const std::exception& e) {
        spdlog::error("Error getting statistics: {}", e.what());
        return makeErrorResponse(
            http::status::internal_server_error,
            "Internal server error",
            req
        );
    }
}

// ═══════════════════════════════════════════════════════════
// Private Helper Methods
// ═══════════════════════════════════════════════════════════

http::response<http::string_body> FeedbackAPIHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("makeResponse");
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> FeedbackAPIHandler::makeJsonResponse(
    http::status status,
    const json& body,
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> FeedbackAPIHandler::makeErrorResponse(
    http::status status,
    const std::string& error,
    const http::request<http::string_body>& req
) {
    json error_json;
    error_json["error"] = error;
    error_json["status"] = static_cast<int>(status);
    return makeJsonResponse(status, error_json, req);
}

llm::lora::FeedbackFilter FeedbackAPIHandler::parseFilterFromQuery(const std::string& query) const {
    llm::lora::FeedbackFilter filter;
    
    if (query.empty()) {
        return filter;
    }
    
    // Simple query parameter parsing
    // Format: key1=value1&key2=value2
    
    auto parse_param = [&query](const std::string& key) -> std::optional<std::string> {
        size_t pos = query.find(key + "=");
        if (pos == std::string::npos) {
          return std::nullopt;
        }
        
        size_t start = pos + key.length() + 1;
        size_t end = query.find('&', start);
        
        if (end == std::string::npos) {
            return query.substr(start);
        } else {
            return query.substr(start, end - start);
        }
    };
    
    if (auto val = parse_param("adapter_id")) {
        if (!isValidFeedbackFilterValue(*val)) {
            throw std::invalid_argument("Invalid adapter_id filter");
        }
        filter.adapter_id = *val;
    }
    if (auto val = parse_param("user_id")) {
        if (!isValidFeedbackFilterValue(*val)) {
            throw std::invalid_argument("Invalid user_id filter");
        }
        filter.user_id = *val;
    }
    if (auto val = parse_param("min_rating")) {
        auto rating = std::stoi(*val);
        if (rating < 0 || rating > 5) {
            throw std::invalid_argument("Invalid min_rating filter");
        }
        filter.min_rating = rating;
    }
    if (auto val = parse_param("flagged_for_training")) {
        if (*val == "true" || *val == "1") {
            filter.flagged_for_training = true;
        } else if (*val == "false" || *val == "0") {
            filter.flagged_for_training = false;
        } else {
            throw std::invalid_argument("Invalid flagged_for_training filter");
        }
    }
    if (auto val = parse_param("training_category")) {
        if (!isValidFeedbackFilterValue(*val)) {
            throw std::invalid_argument("Invalid training_category filter");
        }
        filter.training_category = *val;
    }
    if (auto val = parse_param("limit")) {
        auto limit = std::stoul(*val);
        if (limit == 0 || limit > kMaxFeedbackQueryLimit) {
            throw std::invalid_argument("Invalid limit filter");
        }
        filter.limit = limit;
    }
    if (auto val = parse_param("offset")) {
        filter.offset = std::stoul(*val);
    }
    
    return filter;
}

} // namespace server
} // namespace themis
