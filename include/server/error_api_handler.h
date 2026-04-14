/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            error_api_handler.h                                ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:42:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     68                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>

namespace themis {
namespace server {

// Forward declarations to avoid circular dependencies
struct Request {
    std::string method;
    std::string path;
    nlohmann::json params;
    nlohmann::json query;
    nlohmann::json body;
};

struct Response {
    int status_code = 200;
    nlohmann::json body;
    std::string content_type = "application/json";
    
    void setJSON(const nlohmann::json& j) {
        body = j;
        content_type = "application/json";
    }
};

// Simple route handler type
using RouteHandler = std::function<void(const Request&, Response&)>;

class ErrorApiHandler {
public:
    ErrorApiHandler() = default;
    
    // Handler methods
    void handleGetErrors(const Request& req, Response& res);
    void handleGetError(const Request& req, Response& res);
    void handleGetCategories(const Request& req, Response& res);
    void handleSearchErrors(const Request& req, Response& res);
};

} // namespace server
} // namespace themis
