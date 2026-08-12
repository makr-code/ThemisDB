/**
 * @file openapi_route_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/openapi_route_registry.h"

#include <algorithm>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

RouteRegistry& RouteRegistry::instance() {
    static RouteRegistry inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void RouteRegistry::registerRoute(RouteEntry entry) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    // Replace existing entry with same path+method (last-registration-wins).
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [&entry](const RouteEntry& e) {
            return e.path == entry.path && e.method == entry.method;
        });
    if (it != entries_.end()) {
        *it = std::move(entry);
    } else {
        entries_.push_back(std::move(entry));
    }
}

std::vector<RouteEntry> RouteRegistry::entries() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return entries_;
}

void RouteRegistry::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    entries_.clear();
}

// ---------------------------------------------------------------------------
// OpenAPI 3.1.0 spec builder
// ---------------------------------------------------------------------------

json RouteRegistry::buildOpenApiSpec(const std::string& api_version) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    // ---- paths object: merge all registered entries ----
    json paths = json::object();
    for (const auto& entry : entries_) {
        const RouteOperation& op = entry.operation;

        // Build parameters array
        json params = json::array();
        for (const auto& p : op.parameters) {
            json param = {
                {"name",        p.name},
                {"in",          p.in},
                {"required",    p.required},
                {"description", p.description},
                {"schema",      p.schema.is_null() ? json{{"type","string"}} : p.schema}
            };
            params.push_back(std::move(param));
        }

        json operation = json::object();
        if (!op.summary.empty())     operation["summary"]     = op.summary;
        if (!op.description.empty()) operation["description"] = op.description;
        if (!op.operationId.empty()) operation["operationId"] = op.operationId;
        if (!op.tags.empty()) {
            json tags_arr = json::array();
            for (const auto& t : op.tags) tags_arr.push_back(t);
            operation["tags"] = std::move(tags_arr);
        }
        if (!params.empty()) {
            operation["parameters"] = std::move(params);
        }
        if (!op.requestBody.empty()) {
            operation["requestBody"] = op.requestBody;
        }
        if (!op.responses.empty()) {
            operation["responses"] = op.responses;
        }
        if (op.deprecated) {
            operation["deprecated"] = true;
        }

        paths[entry.path][entry.method] = std::move(operation);
    }

    // ---- reusable components ----
    json components = {
        {"schemas", {
            {"Error", {
                {"type", "object"},
                {"properties", {
                    {"error",       {{"type","boolean"}}},
                    {"message",     {{"type","string"}}},
                    {"status_code", {{"type","integer"}}}
                }}
            }},
            {"HealthStatus", {
                {"type", "object"},
                {"properties", {
                    {"status",          {{"type","string"},
                                         {"enum", json::array({"healthy","degraded","unhealthy"})}}},
                    {"uptime_seconds",  {{"type","integer"}}},
                    {"request_count",   {{"type","integer"}}},
                    {"error_count",     {{"type","integer"}}}
                }}
            }},
            {"ReadinessStatus", {
                {"type", "object"},
                {"properties", {
                    {"status", {{"type","string"},
                                {"enum", json::array({"ready","not_ready"})}}},
                    {"checks", {
                        {"type","object"},
                        {"properties", {
                            {"server_running",    {{"type","boolean"}}},
                            {"storage_available", {{"type","boolean"}}},
                            {"active_connections",{{"type","integer"}}},
                            {"active_requests",   {{"type","integer"}}},
                            {"memory_rss_bytes",  {{"type","integer"}}}
                        }}
                    }}
                }}
            }}
        }},
        {"headers", {
            {"API-Version", {
                {"description", "The API version used to process this request (e.g. v1.4.1). "
                                "Clients may request a specific version via Accept-Version."},
                {"schema", {{"type","string"},{"example","v1.4.1"}}}
            }},
            {"Deprecation", {
                {"description", "Present when the accessed endpoint is deprecated. "
                                "Format: 'true; deprecated-version=\"v1.0.0\"; removal-version=\"v2.0.0\"'"},
                {"schema", {{"type","string"},
                             {"example","true; deprecated-version=\"v1.0.0\"; removal-version=\"v2.0.0\""}}}
            }},
            {"Sunset", {
                {"description", "RFC 8594 Sunset header. The HTTP-date at which the deprecated "
                                "endpoint will be removed (e.g. 'Wed, 24 Jan 2028 06:00:00 GMT')."},
                {"schema", {{"type","string"},{"example","Wed, 24 Jan 2028 06:00:00 GMT"}}}
            }},
            {"Link", {
                {"description", "Link to the migration guide for the deprecated endpoint. "
                                "Format: '<url>; rel=\"deprecation\"'"},
                {"schema", {{"type","string"},
                             {"example","<https://docs.themisdb.com/migration/v1-to-v2>; rel=\"deprecation\""}}}
            }}
        }},
        {"parameters", {
            {"AcceptVersion", {
                {"name", "Accept-Version"},
                {"in", "header"},
                {"required", false},
                {"description", "Request a specific API version (e.g. v1.4.0, v1.4, v1, latest). "
                                "If omitted, the current stable version is used."},
                {"schema", {{"type","string"},{"example","v1.4.0"}}}
            }}
        }},
        {"securitySchemes", {
            {"BearerAuth", {
                {"type", "http"},
                {"scheme", "bearer"},
                {"bearerFormat", "JWT"}
            }}
        }}
    };

    // ---- top-level tags: collect unique tags from all operations ----
    std::vector<std::string> seen_tags;
    json tags_arr = json::array();
    auto add_tag = [&](const std::string& name, const std::string& description) {
        if (std::find(seen_tags.begin(), seen_tags.end(), name) == seen_tags.end()) {
            seen_tags.push_back(name);
            tags_arr.push_back(json{{"name", name}, {"description", description}});
        }
    };

    // Well-known tag descriptions
    auto tag_description = [](const std::string& t) -> std::string {
        if (t == "monitoring")    return "Health, metrics and observability endpoints";
        if (t == "observability") return "Operator observability REST API – alerts, silences, health";
        if (t == "entities")      return "Entity CRUD operations";
        if (t == "query")         return "Query execution endpoints";
        if (t == "license")       return "License management and status";
        return "";
    };

    for (const auto& entry : entries_) {
        for (const auto& t : entry.operation.tags) {
            add_tag(t, tag_description(t));
        }
    }

    // ---- assemble final document ----
    json spec = {
        {"openapi", "3.1.0"},
        {"info", {
            {"title",       "ThemisDB REST API"},
            {"description", "Production-ready HTTP API for the ThemisDB distributed database engine"},
            {"version",     api_version},
            {"contact", {
                {"name", "ThemisDB"},
                {"url",  "https://github.com/makr-code/ThemisDB"}
            }},
            {"license", {
                {"name", "See LICENSE in repository"},
                {"url",  "https://github.com/makr-code/ThemisDB/blob/main/LICENSE"}
            }}
        }},
        {"servers", json::array({
            json{{"url", "/"}, {"description", "This server"}}
        })},
        {"paths",      std::move(paths)},
        {"components", std::move(components)},
        {"security",   json::array({ json{{"BearerAuth", json::array()}} })},
        {"tags",       std::move(tags_arr)}
    };

    return spec;
}

} // namespace server
} // namespace themis
