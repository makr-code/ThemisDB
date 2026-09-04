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
        if (!op.summary.empty()) {
          operation["summary"]     = op.summary;
        }
        if (!op.description.empty()) {
          operation["description"] = op.description;
        }
        if (!op.operationId.empty()) {
          operation["operationId"] = op.operationId;
        }
        if (!op.tags.empty()) {
            json tags_arr = json::array();
            for (const auto& t : op.tags) {
              tags_arr.push_back(t);
            }
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
        if (t == "monitoring") {
          return "Health, metrics and observability endpoints";
        }
        if (t == "observability") {
          return "Operator observability REST API – alerts, silences, health";
        }
        if (t == "entities") {
          return "Entity CRUD operations";
        }
        if (t == "query") {
          return "Query execution endpoints";
        }
        if (t == "license") {
          return "License management and status";
        }
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

// ── Phase 3 Schema-Governance: Drift Detection ────────────────────────────────

#include <sstream>
#include <functional>
#include <unordered_map>

namespace themis {
namespace server {

namespace {

/// Build a canonical string key for a route entry: "METHOD /path".
inline std::string routeKey(const RouteEntry& e) {
    return e.method + " " + e.path;
}

/// Produce a deterministic hash of a route entry's operation metadata.
/// Uses a simple FNV-1a hash over the serialised fields to avoid heavy
/// JSON dependency in the hot registration path.
inline std::size_t hashRouteOperation(const RouteEntry& e) {
    // Include path, method, operationId, summary, deprecated flag.
    std::string canonical = e.path + "|" + e.method
        + "|" + e.operation.operationId
        + "|" + e.operation.summary
        + "|" + (e.operation.deprecated ? "1" : "0");
    return std::hash<std::string>{}(canonical);
}

/// Parse a snapshot string (produced by captureSpecSnapshot) back into a
/// map of routeKey → operation hash.
std::unordered_map<std::string, std::size_t>
parseSnapshot(const std::string& snapshot) {
    std::unordered_map<std::string, std::size_t> result = {};

    if (snapshot.empty()) { return result; }

    std::istringstream ss(snapshot);
    std::string line = {};
    while (std::getline(ss, line)) {
        auto sep = line.rfind('\t');
        if (sep == std::string::npos) { continue; }
        std::string key  = line.substr(0, sep);
        std::string hstr = line.substr(sep + 1);
        try {
            result.emplace(std::move(key), static_cast<std::size_t>(std::stoull(hstr)));
        } catch (...) {
            // Malformed line: skip silently (caller can treat missing key as drift)
        }
    }
    return result;
}

} // anonymous namespace

std::string RouteRegistry::captureSpecSnapshot() const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    // Build a line-per-route text: "METHOD /path\t<hash>\n"
    // Sorted for determinism regardless of insertion order.
    std::vector<std::string> lines = {};

    lines.reserve(entries_.size());
    for (const auto& e : entries_) {
        lines.push_back(routeKey(e) + "\t"
                        + std::to_string(hashRouteOperation(e)));
    }
    std::sort(lines.begin(), lines.end());
    std::string result = {};
    result.reserve(lines.size() * 64);
    for (const auto& l : lines) { result += l + "\n"; }
    return result;
}

RouteRegistry::DriftReport
RouteRegistry::detectDrift(const std::string& baseline_snapshot) const {
    // Parse baseline into key → hash map
    auto baseline = parseSnapshot(baseline_snapshot);

    // Capture current state (under lock)
    std::unordered_map<std::string, std::size_t> current;
    {
        std::shared_lock<std::shared_mutex> lk(mutex_);
        current.reserve(entries_.size());
        for (const auto& e : entries_) {
            current.emplace(routeKey(e), hashRouteOperation(e));
        }
    }

    DriftReport report;
    // Added: in current but not in baseline
    for (const auto& [key, hash] : current) {
        if (baseline.find(key) == baseline.end()) {
            report.added.push_back(key);
        }
    }
    // Removed / Changed: in baseline but not in current, or hash changed
    for (const auto& [key, base_hash] : baseline) {
        auto it = current.find(key);
        if (it == current.end()) {
            report.removed.push_back(key);
        } else if (it->second != base_hash) {
            report.changed.push_back(key);
        }
    }

    // Sort each list for deterministic ordering
    std::sort(report.added.begin(), report.added.end());
    std::sort(report.removed.begin(), report.removed.end());
    std::sort(report.changed.begin(), report.changed.end());
    return report;
}

} // namespace server
} // namespace themis
