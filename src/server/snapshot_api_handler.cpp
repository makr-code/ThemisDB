#include "server/snapshot_api_handler.h"
#include "transaction/snapshot_manager.h"
#include "utils/logger.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

namespace http = boost::beast::http;

SnapshotApiHandler::SnapshotApiHandler(SnapshotManager& snapshot_mgr)
    : snapshot_mgr_(snapshot_mgr) {
    THEMIS_INFO("SnapshotApiHandler initialized");
}

void SnapshotApiHandler::handleCreateTag(const http::request<http::string_body>& req, 
                                          http::response<http::string_body>& res) {
    try {
        // Parse request body
        auto json = nlohmann::json::parse(req.body());
        
        // Extract parameters
        if (!json.contains("tag_name")) {
            sendErrorResponse(res, 400, "Missing required field: tag_name");
            return;
        }

        std::string tag_name = json["tag_name"].get<std::string>();
        std::string description = json.value("description", "");
        std::string created_by = json.value("created_by", "system");

        // Create snapshot
        auto status = snapshot_mgr_.createTag(tag_name, description, created_by);
        
        if (!status.ok) {
            sendErrorResponse(res, 400, status.message);
            return;
        }

        // Get the created snapshot
        auto snapshot = snapshot_mgr_.getTag(tag_name);
        if (!snapshot) {
            sendErrorResponse(res, 500, "Failed to retrieve created snapshot");
            return;
        }

        // Return success response
        sendJsonResponse(res, 201, snapshot->toJson());
        
        THEMIS_INFO("API: Created snapshot tag '{}' by {}", tag_name, created_by);
        
    } catch (const nlohmann::json::exception& e) {
        sendErrorResponse(res, 400, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal error: " + std::string(e.what()));
    }
}

void SnapshotApiHandler::handleListTags(const http::request<http::string_body>& req, 
                                         http::response<http::string_body>& res) {
    try {
        auto snapshots = snapshot_mgr_.listTags();
        
        nlohmann::json response;
        response["tags"] = nlohmann::json::array();
        
        for (const auto& snapshot : snapshots) {
            response["tags"].push_back(snapshot.toJson());
        }
        
        response["total"] = snapshots.size();
        
        sendJsonResponse(res, 200, response);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal error: " + std::string(e.what()));
    }
}

void SnapshotApiHandler::handleGetTag(const http::request<http::string_body>& req, 
                                       http::response<http::string_body>& res) {
    try {
        // Extract tag name from URL path
        std::string path = std::string(req.target());
        std::string prefix = "/api/v1/snapshots/tags/";
        
        if (path.find(prefix) != 0) {
            sendErrorResponse(res, 400, "Invalid tag path");
            return;
        }
        
        std::string tag_name = path.substr(prefix.length());
        // Remove query string if present
        auto qpos = tag_name.find('?');
        if (qpos != std::string::npos) {
            tag_name = tag_name.substr(0, qpos);
        }
        
        auto snapshot = snapshot_mgr_.getTag(tag_name);
        
        if (!snapshot) {
            sendErrorResponse(res, 404, "Tag '" + tag_name + "' not found");
            return;
        }
        
        sendJsonResponse(res, 200, snapshot->toJson());
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal error: " + std::string(e.what()));
    }
}

void SnapshotApiHandler::handleDeleteTag(const http::request<http::string_body>& req, 
                                          http::response<http::string_body>& res) {
    try {
        // Extract tag name from URL path
        std::string path = std::string(req.target());
        std::string prefix = "/api/v1/snapshots/tags/";
        
        if (path.find(prefix) != 0) {
            sendErrorResponse(res, 400, "Invalid tag path");
            return;
        }
        
        std::string tag_name = path.substr(prefix.length());
        // Remove query string if present
        auto qpos = tag_name.find('?');
        if (qpos != std::string::npos) {
            tag_name = tag_name.substr(0, qpos);
        }
        
        auto status = snapshot_mgr_.deleteTag(tag_name);
        
        if (!status.ok) {
            if (status.message.find("does not exist") != std::string::npos) {
                sendErrorResponse(res, 404, status.message);
            } else {
                sendErrorResponse(res, 500, status.message);
            }
            return;
        }
        
        nlohmann::json response;
        response["message"] = "Tag '" + tag_name + "' deleted successfully";
        sendJsonResponse(res, 200, response);
        
        THEMIS_INFO("API: Deleted snapshot tag '{}'", tag_name);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal error: " + std::string(e.what()));
    }
}

void SnapshotApiHandler::handleGetStats(const http::request<http::string_body>& req, 
                                         http::response<http::string_body>& res) {
    try {
        auto stats = snapshot_mgr_.getStats();
        
        nlohmann::json response;
        response["total_tags"] = stats.total_tags;
        response["oldest_sequence"] = stats.oldest_sequence;
        response["newest_sequence"] = stats.newest_sequence;
        response["oldest_timestamp_ms"] = stats.oldest_timestamp_ms;
        response["newest_timestamp_ms"] = stats.newest_timestamp_ms;
        
        sendJsonResponse(res, 200, response);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, "Internal error: " + std::string(e.what()));
    }
}

// Helper methods
void SnapshotApiHandler::sendJsonResponse(http::response<http::string_body>& res, int status, const nlohmann::json& json) {
    res.result(static_cast<http::status>(status));
    res.set(http::field::content_type, "application/json");
    res.body() = json.dump(2);
    res.prepare_payload();
}

void SnapshotApiHandler::sendErrorResponse(http::response<http::string_body>& res, int status, const std::string& error) {
    nlohmann::json json;
    json["error"] = error;
    sendJsonResponse(res, status, json);
}

} // namespace themis
