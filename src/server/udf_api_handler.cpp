/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            udf_api_handler.cpp                                ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:52:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     221                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2a0e15fab  2026-03-11  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 1200426fcd  2026-02-26  feat(query): implement UDF registration API (Issue #2433) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/udf_api_handler.h"
#include "query/functions/udf_registry.h"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;
using namespace themis::query::functions;

// ─────────────────────────────────────────────────────────────────────────────
// HTTP helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
UdfApiHandler::makeJsonResponse(
    http::status status,
    const json& body,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
UdfApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req) const
{
    return makeJsonResponse(status, {{"error", message}}, req);
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/v1/query/udfs
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
UdfApiHandler::handleRegister(const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleRegister");
    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::parse_error&) {
        return makeErrorResponse(http::status::bad_request,
                                 "Request body is not valid JSON", req);
    }

    // Required: name
    if (!body.contains("name") || !body["name"].is_string() ||
        body["name"].get<std::string>().empty()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Field 'name' is required and must be a non-empty string", req);
    }

    // Required: body expression
    if (!body.contains("body") || !body["body"].is_object()) {
        return makeErrorResponse(http::status::bad_request,
                                 "Field 'body' is required and must be a JSON object "
                                 "(expression DSL)", req);
    }

    UdfDefinition def;
    def.name = body["name"].get<std::string>();

    if (body.contains("description") && body["description"].is_string()) {
        def.description = body["description"].get<std::string>();
    }

    // Parse arguments array
    if (body.contains("arguments") && body["arguments"].is_array()) {
        for (const auto& a : body["arguments"]) {
            if (!a.is_object()) {
                return makeErrorResponse(http::status::bad_request,
                                         "Each element of 'arguments' must be a JSON object", req);
            }
            ArgSpec spec;
            if (a.contains("name") && a["name"].is_string()) {
                spec.name = a["name"].get<std::string>();
            }
            if (a.contains("type") && a["type"].is_string()) {
                try {
                    spec.type = UdfDefinition::parseArgType(a["type"].get<std::string>());
                } catch (const std::runtime_error& e) {
                    return makeErrorResponse(http::status::bad_request, e.what(), req);
                }
            }
            if (a.contains("required") && a["required"].is_boolean()) {
                spec.required = a["required"].get<bool>();
            }
            if (a.contains("description") && a["description"].is_string()) {
                spec.description = a["description"].get<std::string>();
            }
            def.arguments.push_back(std::move(spec));
        }
    }

    if (body.contains("return_type") && body["return_type"].is_string()) {
        try {
            def.return_type = UdfDefinition::parseArgType(body["return_type"].get<std::string>());
        } catch (const std::runtime_error& e) {
            return makeErrorResponse(http::status::bad_request, e.what(), req);
        }
    }

    if (body.contains("is_deterministic") && body["is_deterministic"].is_boolean()) {
        def.is_deterministic = body["is_deterministic"].get<bool>();
    }

    def.body = body["body"];

    try {
        UdfRegistry::instance().registerUdf(std::move(def));
    } catch (const std::runtime_error& e) {
        return makeErrorResponse(http::status::bad_request, e.what(), req);
    }

    // Return the stored definition
    try {
        auto stored = UdfRegistry::instance().getUdf(
            body["name"].get<std::string>());
        return makeJsonResponse(http::status::created, stored.toJson(), req);
    } catch (const std::runtime_error& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/query/udfs
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
UdfApiHandler::handleList(const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleList");
    auto udfs = UdfRegistry::instance().listUdfs();
    json arr = json::array();
    for (const auto& d : udfs) {
        arr.push_back(d.toJson());
    }
    return makeJsonResponse(http::status::ok,
                            {{"udfs", arr}, {"count", arr.size()}},
                            req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/query/udfs/{name}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
UdfApiHandler::handleGet(
    const http::request<http::string_body>& req,
    const std::string& name)
{
    auto span = Tracer::startSpan("handleGet");
    try {
        auto def = UdfRegistry::instance().getUdf(name);
        return makeJsonResponse(http::status::ok, def.toJson(), req);
    } catch (const std::runtime_error&) {
        return makeErrorResponse(http::status::not_found,
                                 "UDF not found: " + name, req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DELETE /api/v1/query/udfs/{name}
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body>
UdfApiHandler::handleDelete(
    const http::request<http::string_body>& req,
    const std::string& name)
{
    auto span = Tracer::startSpan("handleDelete");
    try {
        UdfRegistry::instance().unregisterUdf(name);
    } catch (const std::runtime_error&) {
        return makeErrorResponse(http::status::not_found,
                                 "UDF not found: " + name, req);
    }

    http::response<http::string_body> res{http::status::no_content, req.version()};
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis
