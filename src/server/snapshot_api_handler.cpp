/**
 * @file snapshot_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/snapshot_api_handler.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include "utils/tracing.h"
#include "utils/logger.h"

namespace themis {
namespace server {

SnapshotApiHandler::SnapshotApiHandler(transaction::SnapshotManager& snapshot_manager)
    : snapshot_manager_(snapshot_manager) {
}

void SnapshotApiHandler::registerRoutes([[maybe_unused]] httplib::Server& server) {
    // POST /api/v1/snapshots/tags - Create tag
    server.Post("/api/v1/snapshots/tags", [this](const httplib::Request& req, httplib::Response& res) {
        handleCreateTag(req, res);
    });

    // GET /api/v1/snapshots/tags - List tags
    server.Get("/api/v1/snapshots/tags", [this](const httplib::Request& req, httplib::Response& res) {
        handleListTags(req, res);
    });

    // GET /api/v1/snapshots/tags/:name - Get specific tag
    server.Get(R"(/api/v1/snapshots/tags/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetTag(req, res);
    });

    // DELETE /api/v1/snapshots/tags/:name - Delete tag
    server.Delete(R"(/api/v1/snapshots/tags/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        handleDeleteTag(req, res);
    });

    // GET /api/v1/snapshots/stats - Get statistics
    server.Get("/api/v1/snapshots/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStats(req, res);
    });

    spdlog::info("Snapshot API routes registered");
}

void SnapshotApiHandler::handleCreateTag(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleCreateTag");
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("tag_name") || !body.contains("description")) {
            sendError(res, 400, "Missing required fields: tag_name and description");
            return;
        }
        
        std::string tag_name = body["tag_name"];
        std::string description = body["description"];
        std::string created_by = body.value("created_by", "system");
        
        // Create tag
        auto snapshot = snapshot_manager_.createTag(tag_name, description, created_by);
        
        if (!snapshot.has_value()) {
            sendError(res, 409, fmt::format("Failed to create tag '{}'. Tag may already exist or name is invalid.", tag_name));
            return;
        }
        
        // Return created snapshot
        sendJson(res, snapshot->toJson(), 201);
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void SnapshotApiHandler::handleListTags(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleListTags");
        // Parse query parameters
        size_t limit = 0;
        if (req.has_param("limit")) {
            try {
                limit = std::stoull(req.get_param_value("limit"));
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "snapshot_api_handler: unhandled exception caught");
                sendError(res, 400, "Invalid limit parameter");
                return;
            }
        }
        
        std::string sort_by = req.get_param_value("sort_by");
        if (sort_by.empty()) {
            sort_by = "timestamp";
        }
        
        bool ascending = false;
        if (req.has_param("ascending")) {
            std::string asc_str = req.get_param_value("ascending");
            ascending = (asc_str == "true" || asc_str == "1");
        }
        
        // List tags
        auto snapshots = snapshot_manager_.listTags(limit, sort_by, ascending);
        
        // Convert to JSON array
        json result = json::array();
        for (const auto& snapshot : snapshots) {
            result.push_back(snapshot.toJson());
        }
        
        sendJson(res, result);
        
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void SnapshotApiHandler::handleGetTag(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleGetTag");
        // Extract tag name from URL
        std::string tag_name = req.matches[1];
        
        // Get tag
        auto snapshot = snapshot_manager_.getTag(tag_name);
        
        if (!snapshot.has_value()) {
            sendError(res, 404, fmt::format("Tag '{}' not found", tag_name));
            return;
        }
        
        sendJson(res, snapshot->toJson());
        
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void SnapshotApiHandler::handleDeleteTag(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleDeleteTag");
        // Extract tag name from URL
        std::string tag_name = req.matches[1];
        
        // Delete tag
        bool success = snapshot_manager_.deleteTag(tag_name);
        
        if (!success) {
            sendError(res, 404, fmt::format("Tag '{}' not found", tag_name));
            return;
        }
        
        json response;
        response["status"] = "success";
        response["message"] = fmt::format("Tag '{}' deleted successfully", tag_name);
        sendJson(res, response);
        
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void SnapshotApiHandler::handleGetStats(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleGetStats");
        auto stats = snapshot_manager_.getStats();
        sendJson(res, stats.toJson());
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Failed to get stats: {}", e.what()));
    }
}

void SnapshotApiHandler::sendError(httplib::Response& res, int status_code, const std::string& message) const {
    json error;
    error["error"] = message;
    error["status"] = status_code;
    
    res.status = status_code;
    res.set_content(error.dump(2), "application/json");
    
    spdlog::warn("Snapshot API error ({}): {}", status_code, message);
}

void SnapshotApiHandler::sendJson(httplib::Response& res, const json& data, int status_code) const {
    res.status = status_code;
    res.set_content(data.dump(2), "application/json");
}

} // namespace server
} // namespace themis

