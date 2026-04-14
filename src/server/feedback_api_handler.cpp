/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_api_handler.cpp                           ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:05:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     408                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 22506a3f62  2026-03-20  fix: remove unused boost/url.hpp include from feedback_ap... ║
    • a2a0e15fab  2026-03-11  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/feedback_api_handler.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include "utils/tracing.h"

namespace themis {
namespace server {

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
        // Parse request body
        auto body_json = json::parse(req.body());
        
        // Create feedback from JSON
        auto feedback = llm::lora::Feedback::fromJSON(body_json);
        
        // Store feedback
        auto stored = storage_service_->createFeedback(feedback);
        
        if (!stored) {
            return makeErrorResponse(
                http::status::bad_request,
                "Feedback validation failed",
                req
            );
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
        // Parse query parameters
        std::string target(req.target());
        size_t query_pos = target.find('?');
        std::string query = (query_pos != std::string::npos) 
            ? target.substr(query_pos + 1) 
            : "";
        
        auto filter = parseFilterFromQuery(query);
        
        // Get feedback list
        auto feedback_list = storage_service_->listFeedback(filter);
        
        // Build response
        json response;
        response["count"] = feedback_list.size();
        response["feedback"] = json::array();
        
        for (const auto& fb : feedback_list) {
            response["feedback"].push_back(fb.toJSON());
        }
        
        return makeJsonResponse(http::status::ok, response, req);
        
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
        auto feedback = storage_service_->getFeedback(id);
        
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
        // Parse request body
        auto body_json = json::parse(req.body());
        
        // Create feedback from JSON
        auto feedback = llm::lora::Feedback::fromJSON(body_json);
        
        // Update feedback
        bool success = storage_service_->updateFeedback(id, feedback);
        
        if (!success) {
            return makeErrorResponse(
                http::status::not_found,
                "Feedback not found",
                req
            );
        }
        
        // Get updated feedback
        auto updated = storage_service_->getFeedback(id);
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
        bool success = storage_service_->deleteFeedback(id);
        
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
            }
        }
        
        auto feedback_list = storage_service_->getFeedbackForAdapter(adapter_id, limit);
        
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
            }
        }
        
        auto stats = storage_service_->getStatistics(adapter_id);
        
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
        if (pos == std::string::npos) return std::nullopt;
        
        size_t start = pos + key.length() + 1;
        size_t end = query.find('&', start);
        
        if (end == std::string::npos) {
            return query.substr(start);
        } else {
            return query.substr(start, end - start);
        }
    };
    
    if (auto val = parse_param("adapter_id")) {
        filter.adapter_id = *val;
    }
    if (auto val = parse_param("user_id")) {
        filter.user_id = *val;
    }
    if (auto val = parse_param("min_rating")) {
        filter.min_rating = std::stoi(*val);
    }
    if (auto val = parse_param("flagged_for_training")) {
        filter.flagged_for_training = (*val == "true" || *val == "1");
    }
    if (auto val = parse_param("training_category")) {
        filter.training_category = *val;
    }
    if (auto val = parse_param("limit")) {
        filter.limit = std::stoul(*val);
    }
    if (auto val = parse_param("offset")) {
        filter.offset = std::stoul(*val);
    }
    
    return filter;
}

} // namespace server
} // namespace themis
