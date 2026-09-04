/**
 * @file diff_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/diff_api_handler.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "utils/tracing.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_HTTP_SERVER

namespace themis {
namespace server {

DiffApiHandler::DiffApiHandler(analytics::DiffEngine& diff_engine)
    : diff_engine_(diff_engine) {
}

void DiffApiHandler::registerRoutes([[maybe_unused]] httplib::Server& server) {
    // GET /api/v1/diff - Compute diff
    server.Get("/api/v1/diff", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetDiff(req, res);
    });

    // GET /api/v1/diff/cache/stats - Get cache statistics
    server.Get("/api/v1/diff/cache/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetCacheStats(req, res);
    });

    // DELETE /api/v1/diff/cache - Clear cache
    server.Delete("/api/v1/diff/cache", [this](const httplib::Request& req, httplib::Response& res) {
        handleClearCache(req, res);
    });

    spdlog::info("Diff API routes registered");
}

void DiffApiHandler::handleGetDiff(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleGetDiff");
        // Parse query parameters
        auto options = parseOptions(req);
        
        // Check for required parameters: from and to
        if (!req.has_param("from") || !req.has_param("to")) {
            sendError(res, 400, "Missing required parameters: 'from' and 'to'");
            return;
        }
        
        std::string from_str = req.get_param_value("from");
        std::string to_str = req.get_param_value("to");
        
        analytics::DiffEngine::DiffResult result;
        
        // Check if tag-based diff
        if (req.has_param("from_tag") && req.has_param("to_tag")) {
            std::string from_tag = req.get_param_value("from_tag");
            std::string to_tag = req.get_param_value("to_tag");
            
            spdlog::info("Computing diff by tags: from_tag='{}', to_tag='{}'", from_tag, to_tag);
            result = diff_engine_.computeDiffByTag(from_tag, to_tag, options);
        }
        // Check if timestamp-based diff (ISO 8601 format or milliseconds)
        else if (!isSequenceNumber(from_str) || !isSequenceNumber(to_str)) {
            int64_t from_ts = parseTimestamp(from_str);
            int64_t to_ts = parseTimestamp(to_str);
            
            spdlog::info("Computing diff by timestamp: from={}, to={}", from_ts, to_ts);
            result = diff_engine_.computeDiffByTimestamp(from_ts, to_ts, options);
        }
        // Sequence-based diff
        else {
            uint64_t from_seq = std::stoull(from_str);
            uint64_t to_seq = std::stoull(to_str);
            
            spdlog::info("Computing diff by sequence: from={}, to={}", from_seq, to_seq);
            result = diff_engine_.computeDiff(from_seq, to_seq, options);
        }
        
        // Convert result to JSON and send
        sendJson(res, result.toJson());
        
    } catch (const std::invalid_argument& e) {
        sendError(res, 400, fmt::format("Invalid parameter: {}", e.what()));
    } catch (const std::runtime_error& e) {
        sendError(res, 500, fmt::format("Runtime error: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void DiffApiHandler::handleGetCacheStats(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleGetCacheStats");
        auto stats = diff_engine_.getCacheStats();
        sendJson(res, stats);
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Failed to get cache stats: {}", e.what()));
    }
}

void DiffApiHandler::handleClearCache(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleClearCache");
        diff_engine_.clearCache();
        json response;
        response["status"] = "success";
        response["message"] = "Cache cleared successfully";
        sendJson(res, response);
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Failed to clear cache: {}", e.what()));
    }
}

analytics::DiffEngine::DiffOptions DiffApiHandler::parseOptions([[maybe_unused]] const httplib::Request& req) const {
    analytics::DiffEngine::DiffOptions options;
    
    // Parse table filter
    if (req.has_param("table")) {
        options.table_filter = req.get_param_value("table");
    }
    
    // Parse key prefix filter
    if (req.has_param("key_prefix")) {
        options.key_prefix = req.get_param_value("key_prefix");
    }
    
    // Parse include_values flag
    if (req.has_param("include_values")) {
        std::string value = req.get_param_value("include_values");
        options.include_values = (value == "true" || value == "1");
    }
    
    // Parse limit
    if (req.has_param("limit")) {
        try {
            size_t limit = std::stoull(req.get_param_value("limit"));
            options.limit = limit;
        } catch (...) {
            THEMIS_WARN([[maybe_unused]] "diff_api_handler: unhandled exception caught");
            throw std::invalid_argument("Invalid limit parameter");
        }
    }
    
    // Parse offset
    if (req.has_param("offset")) {
        try {
            size_t offset = std::stoull(req.get_param_value("offset"));
            options.offset = offset;
        } catch (...) {
            THEMIS_WARN([[maybe_unused]] "diff_api_handler: unhandled exception caught");
            throw std::invalid_argument("Invalid offset parameter");
        }
    }
    
    // Parse enable_caching flag
    if (req.has_param("enable_caching")) {
        std::string value = req.get_param_value("enable_caching");
        options.enable_caching = (value == "true" || value == "1");
    }
    
    return options;
}

int64_t DiffApiHandler::parseTimestamp([[maybe_unused]] const std::string& str) const {
    // Try to parse as milliseconds first
    try {
        return std::stoll(str);
    } catch (...) {
        spdlog::debug("DiffApiHandler::parseTimestamp: '{}' is not a numeric millisecond timestamp, trying ISO 8601", str);
    }
    
    // Parse ISO 8601 format: YYYY-MM-DDTHH:MM:SS or YYYY-MM-DD
    std::tm tm = {};
    std::istringstream ss(str);
    
    // Try full timestamp format
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) {
        // Try date-only format
        ss.clear();
        ss.str(str);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            throw std::invalid_argument(
                fmt::format("Invalid timestamp format: '{}'. "
                           "Expected milliseconds or ISO 8601 format (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS)",
                           str)
            );
        }
    }
    
    // Convert to milliseconds since epoch
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return ms.count();
}

bool DiffApiHandler::isSequenceNumber([[maybe_unused]] const std::string& str) const {
    if (str.empty()) {
      return false;
    }
    
    // Check if all characters are digits
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    
    return true;
}

void DiffApiHandler::sendError(httplib::Response& res, int status_code, const std::string& message) const {
    json error;
    error["error"] = message;
    error["status"] = status_code;
    
    res.status = status_code;
    res.set_content(error.dump(2), "application/json");
    
    spdlog::warn("Diff API error ({}): {}", status_code, message);
}

void DiffApiHandler::sendJson(httplib::Response& res, const json& data, int status_code) const {
    res.status = status_code;
    res.set_content(data.dump(2), "application/json");
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP_SERVER
