/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisctl.cpp                                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:58:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1052                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 30763c38a6  2026-04-13  feat(metadata): complete Automatic Indexing Recommendatio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file themisctl.cpp
 * @brief Unified ThemisDB command-line management tool.
 *
 * Connects to a running ThemisDB server over HTTP and exposes the most
 * frequently-used operations as convenient sub-commands.
 *
 * Global options (must come before <command>):
 *   --host <h>     ThemisDB host (default: localhost / $THEMIS_HOST)
 *   --port <p>     ThemisDB port (default: 8765   / $THEMIS_PORT)
 *   --token <jwt>  Bearer auth token   ($THEMIS_TOKEN)
 *   --timeout <s>  Request timeout in seconds (default: 30)
 *   --json         Print raw JSON responses
 *   --no-color     Disable ANSI colour
 *   --help, -h     Print this help
 *
 * Commands:
 *   health                       Show server liveness/readiness status
 *   version                      Print server version string
 *   capabilities                 Show server capability map
 *   query <aql>                  Execute an AQL query
 *   batch-insert --collection C  Import JSONL payload via /entities/batch
 *   api <METHOD> <PATH>          Generic HTTP passthrough to any endpoint
 *   get    <id>                  Get an entity by key
 *   put    <id> <json-body>      Create or update an entity
 *   delete <id>                  Delete an entity
 *   schema [table]               Show schema (optionally for one table)
 *   config get                   Print current server configuration
 *   config set <key=value> ...   Hot-reload one or more config keys
 *   config validate [key=value ...]  Dry-run + diff proposed config changes
 *   branch list                  List branches
 *   branch create <name>         Create a branch
 *   branch switch <name>         Switch the active branch
 *   branch delete <name>         Delete a branch
 *   snapshot list                List snapshot tags
 *   snapshot create [tag]        Create a snapshot tag
 *   admin stats                  Show observability health / node stats
 *   admin cache                  Show cache health and statistics
 *   provenance-export [options]  Export observability provenance records (--query-id, --start-ts, --end-ts, --limit, --format, --output)
 *   self-report                  Bundle self-disclosure (content + health)
 *   index recommend [table]      Show automatic index recommendations
 *   chat [options] <prompt>      Chat mode via LLM or RAG
 *   agent [options] <task>       Agent mode with planning response
 *   rag query [--collection C] [--top-k N] [--lora ID]
 *             [--rag-mode text|iterative|map_reduce]
 *             [--response-budget-tokens N] [--max-tokens N] <question>
 *                                AgenticRAG natural-language query
 *   repl                         Start interactive REPL (with history)
 *
 * Exit codes:
 *   0  Success
 *   1  Server-side error (HTTP 4xx / 5xx)
 *   2  Usage / argument error
 *   3  Connection / transport error
 */

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "utils/cli_parser_utils.h"

// ── httplib (cpp-httplib, header-only) ────────────────────────────────────────
#include <httplib.h>

// ── nlohmann/json (header-only) ───────────────────────────────────────────────
#include <nlohmann/json.hpp>

// ── GNU Readline (optional — disabled in test builds) ─────────────────────────
#ifndef THEMISCTL_TEST_BUILD
#  ifdef THEMISCTL_ENABLE_READLINE
#    include <readline/readline.h>
#    include <readline/history.h>
#    define THEMISCTL_HAS_READLINE 1
#  else
#    define THEMISCTL_HAS_READLINE 0
#  endif
#else
#  define THEMISCTL_HAS_READLINE 0
#endif

using json = nlohmann::json;

// ============================================================================
// ANSI colour helpers
// ============================================================================

namespace Color {
    static const std::string Reset  = "\033[0m";
    static const std::string Bold   = "\033[1m";
    static const std::string Green  = "\033[32m";
    static const std::string Yellow = "\033[33m";
    static const std::string Red    = "\033[31m";
    static const std::string Cyan   = "\033[36m";
    static const std::string Blue   = "\033[34m";
    static const std::string Dim    = "\033[2m";
}

static bool g_use_color = true;
static bool g_help_routes_json = false;

static std::string col(const std::string& c, const std::string& s) {
    return g_use_color ? c + s + Color::Reset : s;
}
static std::string ok()   { return col(Color::Green,  "OK");   }
static std::string fail() { return col(Color::Red,    "FAIL"); }
static std::string warn() { return col(Color::Yellow, "WARN"); }

// ============================================================================
// Global connection context
// ============================================================================

struct Ctx {
    std::string host    = "localhost";
    int         port    = 8765;
    std::string token;
    int         timeout = 30;  // seconds
    bool        raw_json = false;
};

static Ctx g_ctx;

// ============================================================================
// Argument parsing helpers
// ============================================================================

namespace {

struct ThemisCtlGlobalOptions {
    std::string host;
    int port = 8765;
    std::string token;
    int timeout = 30;
    bool raw_json = false;
    bool routes_json = false;
    bool no_color = false;
    bool show_help = false;
    std::vector<std::string> remaining_args;
};

using themis::cli::is_help_flag;
using themis::cli::consume_next_argument;

bool parse_global_options(const std::vector<std::string>& args,
                          ThemisCtlGlobalOptions& options,
                          std::string& error_message) {
    options.remaining_args.clear();

    for (size_t index = 0; index < args.size(); ++index) {
        const auto& arg = args[index];

        if (arg == "--no-color") {
            options.no_color = true;
            continue;
        }
        if (arg == "--json") {
            options.raw_json = true;
            continue;
        }
        if (arg == "--routes-json") {
            options.routes_json = true;
            continue;
        }
        if (is_help_flag(arg)) {
            options.show_help = true;
            continue;
        }
        if (arg == "--host") {
            if (!consume_next_argument(args, index, arg, options.host, error_message)) {
                return false;
            }
            continue;
        }
        if (arg == "--port") {
            std::string port_value = {};
            if (!consume_next_argument(args, index, arg, port_value, error_message)) {
                return false;
            }
            try {
                options.port = std::stoi(port_value);
            } catch (const std::exception&) {
                error_message = "Invalid numeric value for option --port: " + port_value;
                return false;
            }
            continue;
        }
        if (arg == "--token") {
            if (!consume_next_argument(args, index, arg, options.token, error_message)) {
                return false;
            }
            continue;
        }
        if (arg == "--timeout") {
            std::string timeout_value = {};
            if (!consume_next_argument(args, index, arg, timeout_value, error_message)) {
                return false;
            }
            try {
                options.timeout = std::stoi(timeout_value);
            } catch (const std::exception&) {
                error_message = "Invalid numeric value for option --timeout: " + timeout_value;
                return false;
            }
            continue;
        }

        options.remaining_args.assign(args.begin() + static_cast<std::ptrdiff_t>(index), args.end());
        return true;
    }

    return true;
}

} // namespace

// ============================================================================
// HTTP client helpers
// ============================================================================

struct Response {
    int         status  = -1;
    std::string body = {};
    bool        ok() const { return status >= 200 && status < 300; }
};

static httplib::Headers makeHeaders() {
    httplib::Headers hdrs;
    hdrs.emplace("Accept", "application/json");
    if (!g_ctx.token.empty()) {
        hdrs.emplace("Authorization", "Bearer " + g_ctx.token);
    }
    return hdrs;
}

static Response httpGet(const std::string& path) {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_follow_location(true);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Get(path.c_str(), makeHeaders());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpPost(const std::string& path, const std::string& body,
                         const std::string& ctype = "application/json") {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_follow_location(true);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Post(path.c_str(), makeHeaders(), body, ctype.c_str());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpPut(const std::string& path, const std::string& body,
                        const std::string& ctype = "application/json") {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_follow_location(true);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Put(path.c_str(), makeHeaders(), body, ctype.c_str());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpDelete(const std::string& path) {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_follow_location(true);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Delete(path.c_str(), makeHeaders());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpRequest(const std::string& method,
                            const std::string& path,
                            const std::string& body = "",
                            const std::string& ctype = "application/json") {
    std::string upper = method;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_follow_location(true);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);

    if (upper == "GET") {
        auto res = cli.Get(path.c_str(), makeHeaders());
        if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
        return {res->status, res->body};
    }
    if (upper == "POST") {
        auto res = cli.Post(path.c_str(), makeHeaders(), body, ctype.c_str());
        if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
        return {res->status, res->body};
    }
    if (upper == "PUT") {
        auto res = cli.Put(path.c_str(), makeHeaders(), body, ctype.c_str());
        if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
        return {res->status, res->body};
    }
    if (upper == "DELETE") {
        auto res = cli.Delete(path.c_str(), makeHeaders());
        if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
        return {res->status, res->body};
    }
    if (upper == "PATCH") {
        auto res = cli.Patch(path.c_str(), makeHeaders(), body, ctype.c_str());
        if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
        return {res->status, res->body};
    }
    if (upper == "HEAD") {
        auto res = cli.Head(path.c_str(), makeHeaders());
        if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
        return {res->status, ""};
    }

    return {-1, "Unsupported HTTP method: " + method};
}

// ============================================================================
// Output helpers
// ============================================================================

static void printJson(const std::string& body) {
    try {
        std::cout << json::parse(body).dump(2) << "\n";
    } catch (...) {
        std::cout << body << "\n";
    }
}

/// Print a key-value line with optional colour key.
static void kv(const std::string& key, const std::string& value,
               const std::string& key_color = Color::Cyan) {
    std::cout << col(key_color, key) << ": " << value << "\n";
}

/// Print the result of an HTTP call, returning the appropriate exit code.
static int handleResponse(const Response& r, const std::string& success_msg = "") {
    if (r.status == -1) {
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        if (!r.body.empty()) {
            try {
                std::cerr << json::parse(r.body).dump(2) << "\n";
            } catch (...) {
                std::cerr << r.body << "\n";
            }
        }
        return 1;
    }
    if (g_ctx.raw_json) {
        printJson(r.body);
    } else if (!success_msg.empty()) {
        std::cout << "[" << ok() << "] " << success_msg << "\n";
    }
    return 0;
}

static Response httpGetWithApplicationRedirect(const std::string& path) {
    Response r = httpGet(path);
    if (r.status != 301) {
        return r;
    }

    try {
        auto moved = json::parse(r.body);
        const auto location = moved.value("location", std::string{});
        if (!location.empty()) {
            return httpGet(location);
        }
    } catch (...) {
        // Fall through with original 301 response.
    }
    return r;
}

static bool tryCollectOpenApiRoutes(std::vector<std::pair<std::string, std::string>>& routes,
                                    std::string& warning) {
    routes.clear();
    Response spec = httpGetWithApplicationRedirect("/api/openapi.json");
    if (spec.status == -1) {
        warning = spec.body;
        return false;
    }
    if (!spec.ok()) {
        warning = "HTTP " + std::to_string(spec.status);
        return false;
    }

    try {
        auto openapi = json::parse(spec.body);
        auto paths_it = openapi.find("paths");
        if (paths_it == openapi.end() || !paths_it->is_object()) {
            warning = "OpenAPI response has no 'paths' object";
            return false;
        }

        for (const auto& [path, methods] : paths_it->items()) {
            if (!methods.is_object()) {
                continue;
            }
            for (const auto& [method, _] : methods.items()) {
                std::string m = method;
                std::transform(m.begin(), m.end(), m.begin(),
                               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                routes.emplace_back(m, path);
            }
        }

        std::sort(routes.begin(), routes.end(), [](const auto& a, const auto& b) {
            return (a.second == b.second) ? (a.first < b.first) : (a.second < b.second);
        });
        if (routes.empty()) {
            warning = "OpenAPI routes list is empty";
            return false;
        }
        return true;
    } catch (...) {
        warning = "OpenAPI could not be parsed";
        return false;
    }
}

static std::vector<std::pair<std::string, std::string>>
generateRoutesFromCapabilities(const json& caps) {
    std::vector<std::pair<std::string, std::string>> routes;

    // Core routes expected in almost all builds.
    routes.emplace_back("GET", "/health/live");
    routes.emplace_back("GET", "/health/ready");
    routes.emplace_back("GET", "/version");
    routes.emplace_back("POST", "/api/aql");
    routes.emplace_back("GET", "/entities/{key}");
    routes.emplace_back("PUT", "/entities/{key}");
    routes.emplace_back("DELETE", "/entities/{key}");

    if (caps.contains("schema_awareness") && caps["schema_awareness"].is_object() &&
        caps["schema_awareness"].value("enabled", false)) {
        routes.emplace_back("GET", "/api/v1/schema");
    }

    if (caps.contains("geo") && caps["geo"].is_object() && caps["geo"].value("enabled", false)) {
        routes.emplace_back("POST", "/spatial/index/create");
        routes.emplace_back("GET", "/spatial/metrics");
    }

    if (caps.contains("vector") && caps["vector"].is_object()) {
        routes.emplace_back("POST", "/vector/search");
        routes.emplace_back("POST", "/vector/batch_insert");
    }

    std::sort(routes.begin(), routes.end(), [](const auto& a, const auto& b) {
        return (a.second == b.second) ? (a.first < b.first) : (a.second < b.second);
    });
    routes.erase(std::unique(routes.begin(), routes.end()), routes.end());
    return routes;
}

// ============================================================================
// Commands
// ============================================================================

// ── health ───────────────────────────────────────────────────────────────────

static int cmdHealth(const std::vector<std::string>& /*args*/) {
    // Check liveness
    Response live = httpGet("/health/live");
    Response ready = httpGet("/health/ready");

    if (g_ctx.raw_json) {
        json out;
        out["live"]  = {{"status", live.status},  {"body", live.body}};
        out["ready"] = {{"status", ready.status}, {"body", ready.body}};
        std::cout << out.dump(2) << "\n";
        return (live.ok() && ready.ok()) ? 0 : 1;
    }

    auto statusLine = [](const Response& r) -> std::string {
        if (r.status == -1) {
          return col(Color::Red,    "unreachable");
        }
        if (r.status >= 200 && r.status < 300) {
          return col(Color::Green,  "healthy");
        }
        if (r.status == 503) {
          return col(Color::Yellow, "unavailable");
        }
        return col(Color::Red, "HTTP " + std::to_string(r.status));
    };

    kv("liveness",  statusLine(live));
    kv("readiness", statusLine(ready));

    if (live.status == -1) {
        std::cerr << "[" << fail() << "] Cannot reach " << g_ctx.host
                  << ":" << g_ctx.port << " — " << live.body << "\n";
        return 3;
    }
    return (live.ok() && ready.ok()) ? 0 : 1;
}

// ── version ──────────────────────────────────────────────────────────────────

static int cmdVersion(const std::vector<std::string>& /*args*/) {
    Response r = httpGet("/version");
    if (r.status == -1) {
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        return 1;
    }
    if (g_ctx.raw_json) {
        printJson(r.body);
        return 0;
    }
    try {
        json j = json::parse(r.body);
        if (j.contains("version")) {
          kv("version", j["version"].get<std::string>());
        }
        if (j.contains("build")) {
          kv("build",   j["build"].get<std::string>());
        }
        if (j.contains("commit")) {
          kv("commit",  j["commit"].get<std::string>());
        }
    } catch (...) {
        std::cout << r.body << "\n";
    }
    return 0;
}

// ── capabilities ──────────────────────────────────────────────────────────────

static int cmdCapabilities(const std::vector<std::string>& args) {
    bool show_openapi = false;
    for (const auto& arg : args) {
        if (arg == "--openapi" || arg == "--routes") {
            show_openapi = true;
            continue;
        }
        std::cerr << "Usage: themisctl capabilities [--openapi]\n";
        return 2;
    }

    Response caps = httpGetWithApplicationRedirect("/api/capabilities");
    if (caps.status == -1) {
        std::cerr << "[" << fail() << "] " << caps.body << "\n";
        return 3;
    }
    if (caps.status == 404) {
        if (!g_ctx.raw_json) {
            std::cout << "[" << warn() << "] /api/capabilities not available on this server build\n";
            Response v = httpGet("/version");
            if (v.ok()) {
                try {
                    auto j = json::parse(v.body);
                    if (j.contains("version")) {
                        kv("version", j["version"].get<std::string>());
                    }
                } catch (...) {
                    std::cout << v.body << "\n";
                }
            }
            std::cout << "Use 'themisctl api <METHOD> <PATH>' for full generic endpoint access.\n";
        }
        if (!show_openapi) {
            return 0;
        }
    }
    if (!caps.ok() && caps.status != 404) {
        std::cerr << "[" << fail() << "] HTTP " << caps.status << "\n";
        try { std::cerr << json::parse(caps.body).dump(2) << "\n"; }
        catch (...) { std::cerr << caps.body << "\n"; }
        return 1;
    }

    if (caps.ok() && g_ctx.raw_json && !show_openapi) {
        printJson(caps.body);
        return 0;
    }

    if (caps.ok()) {
        try {
        auto c = json::parse(caps.body);
        std::cout << col(Color::Bold, "Server capabilities") << "\n";
        if (c.contains("edition") && c["edition"].is_object()) {
            kv("edition", c["edition"].value("name", "unknown"));
        }
        if (c.contains("build") && c["build"].is_object()) {
            kv("build", c["build"].value("type", "unknown"));
        }
        if (c.contains("geo") && c["geo"].is_object()) {
            kv("geo.enabled", c["geo"].value("enabled", false) ? "true" : "false");
        }
        if (c.contains("vector") && c["vector"].is_object()) {
            kv("vector.gpu_compiled", c["vector"].value("gpu_compiled", false) ? "true" : "false");
        }
        if (!g_ctx.raw_json) {
            std::cout << c.dump(2) << "\n";
        }
        } catch (...) {
            std::cout << caps.body << "\n";
        }
    }

    if (!show_openapi) {
        return 0;
    }

    std::vector<std::pair<std::string, std::string>> routes;
    std::string openapi_warning = {};
    if (tryCollectOpenApiRoutes(routes, openapi_warning)) {
        std::cout << "\n" << col(Color::Bold, "OpenAPI routes")
                  << " (" << routes.size() << ")\n";
        for (const auto& [method, path] : routes) {
            std::cout << "  " << col(Color::Cyan, method) << " " << path << "\n";
        }
        return 0;
    }

    if (!g_ctx.raw_json) {
        std::cout << "\n[" << warn() << "] OpenAPI route discovery unavailable: "
                  << openapi_warning << "\n";
    }

    if (caps.ok()) {
        try {
            auto c = json::parse(caps.body);
            auto cap_routes = generateRoutesFromCapabilities(c);
            if (!cap_routes.empty() && !g_ctx.raw_json) {
                std::cout << col(Color::Bold, "Capability profile routes")
                          << " (" << cap_routes.size() << ")\n";
                for (const auto& [method, path] : cap_routes) {
                    std::cout << "  " << col(Color::Cyan, method) << " " << path << "\n";
                }
            }
        } catch (...) {
            // Keep silent: capability route list is best-effort.
        }
    }

    return 0;
}

// ── query ─────────────────────────────────────────────────────────────────────

static int cmdQuery(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: themisctl query <aql-statement>\n";
        return 2;
    }
    // Join all remaining args as the query (allows unquoted single-word queries)
    std::string aql = {};
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
          aql += ' ';
        }
        aql += args[i];
    }
    json req;
    req["query"] = aql;
    Response r = httpPost("/api/aql", req.dump());
    if (r.status == -1) {
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
        catch (...) { std::cerr << r.body << "\n"; }
        return 1;
    }
    printJson(r.body);
    return 0;
}

// ── api ───────────────────────────────────────────────────────────────────────

static int cmdApi(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: themisctl api <METHOD> <PATH> [--body <data>] [--body-file <file>] [--stdin] [--content-type <type>]\n";
        return 2;
    }

    const std::string method = args[0];
    const std::string path = args[1];

    std::string body = {};
    std::string content_type = "application/json";

    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--body") {
            if (i + 1 >= args.size()) {
                std::cerr << "--body requires a value\n";
                return 2;
            }
            body = args[++i];
            continue;
        }
        if (args[i] == "--body-file") {
            if (i + 1 >= args.size()) {
                std::cerr << "--body-file requires a file path\n";
                return 2;
            }
            std::ifstream in(args[++i], std::ios::binary);
            if (!in) {
                std::cerr << "[" << fail() << "] Cannot open file: " << args[i] << "\n";
                return 2;
            }
            std::ostringstream oss = {};
            oss << in.rdbuf();
            body = oss.str();
            continue;
        }
        if (args[i] == "--stdin") {
            std::ostringstream oss = {};
            oss << std::cin.rdbuf();
            body = oss.str();
            continue;
        }
        if (args[i] == "--content-type") {
            if (i + 1 >= args.size()) {
                std::cerr << "--content-type requires a value\n";
                return 2;
            }
            content_type = args[++i];
            continue;
        }

        std::cerr << "Unknown option for api: " << args[i] << "\n";
        return 2;
    }

    Response r = httpRequest(method, path, body, content_type);
    if (r.status == -1) {
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }

    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        if (!r.body.empty()) {
            try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
            catch (...) { std::cerr << r.body << "\n"; }
        }
        return 1;
    }

    if (!r.body.empty()) {
        printJson(r.body);
    } else if (!g_ctx.raw_json) {
        std::cout << "[" << ok() << "] HTTP " << r.status << "\n";
    }

    return 0;
}

// ── batch-insert ───────────────────────────────────────────────────────────────

static int cmdBatchInsert(const std::vector<std::string>& args) {
    std::string collection = {};
    size_t batch_size = 500;
    bool edges_mode = false;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--collection") {
            if (i + 1 >= args.size()) {
                std::cerr << "Usage: themisctl batch-insert --collection <name> [--batch-size <n>] [--edges] < data.jsonl\n";
                return 2;
            }
            collection = args[++i];
            continue;
        }
        if (args[i] == "--batch-size") {
            if (i + 1 >= args.size()) {
                std::cerr << "Usage: themisctl batch-insert --collection <name> [--batch-size <n>] [--edges] < data.jsonl\n";
                return 2;
            }
            try {
                batch_size = static_cast<size_t>(std::stoul(args[++i]));
            } catch (...) {
                std::cerr << "[" << fail() << "] --batch-size requires a positive integer\n";
                return 2;
            }
            if (batch_size == 0) {
                std::cerr << "[" << fail() << "] --batch-size must be > 0\n";
                return 2;
            }
            continue;
        }
        if (args[i] == "--edges") {
            edges_mode = true;
            continue;
        }

        std::cerr << "Unknown option for batch-insert: " << args[i] << "\n";
        return 2;
    }

    if (collection.empty()) {
        std::cerr << "Usage: themisctl batch-insert --collection <name> [--batch-size <n>] [--edges] < data.jsonl\n";
        return 2;
    }

    auto flush_ops = [](json& operations, size_t& succeeded, size_t& failed) -> int {
        if (operations.empty()) {
            return 0;
        }

        json req;
        req["operations"] = operations;
        Response r = httpPost("/entities/batch", req.dump());
        if (r.status == -1) {
            std::cerr << "[" << fail() << "] " << r.body << "\n";
            return 3;
        }
        if (!r.ok()) {
            bool fallback_to_put = false;
            if (r.status == 400) {
                try {
                    auto err = json::parse(r.body);
                    const std::string msg = err.value("message", "");
                    if (msg.find("missing required field: key") != std::string::npos) {
                        fallback_to_put = true;
                    }
                } catch (...) {
                    // Keep default false.
                }
            }

            if (fallback_to_put) {
                for (const auto& op : operations) {
                    if (!op.contains("key") || !op["key"].is_string()) {
                        ++failed;
                        continue;
                    }
                    const std::string key = op["key"].get<std::string>();
                    json put_body;
                    put_body["key"] = key;
                    if (op.contains("blob")) {
                        put_body["blob"] = op["blob"];
                    }
                    Response put_res = httpPut("/entities/" + key, put_body.dump());
                    if (put_res.ok()) {
                        ++succeeded;
                    } else {
                        ++failed;
                    }
                }
                operations.clear();
                return 0;
            }

            std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
            try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
            catch (...) { std::cerr << r.body << "\n"; }
            return 1;
        }

        try {
            auto body = json::parse(r.body);
            succeeded += static_cast<size_t>(body.value("succeeded", 0));
            failed += static_cast<size_t>(body.value("failed", 0));
        } catch (...) {
            succeeded += operations.size();
        }

        operations.clear();
        return 0;
    };

    json operations = json::array();
    std::string line = {};
    size_t line_no = 0;
    size_t row_no = 0;
    size_t succeeded = 0;
    size_t failed = 0;

    while (std::getline(std::cin, line)) {
        ++line_no;
        if (line.empty()) {
            continue;
        }

        json doc;
        try {
            doc = json::parse(line);
            if (!doc.is_object()) {
                std::cerr << "[" << warn() << "] line " << line_no << ": not a JSON object, skipped\n";
                ++failed;
                continue;
            }
        } catch (const std::exception& e) {
            std::cerr << "[" << warn() << "] line " << line_no << ": invalid JSON (" << e.what() << "), skipped\n";
            ++failed;
            continue;
        }

        ++row_no;
        std::string key = {};
        if (doc.contains("_key") && doc["_key"].is_string() && !doc["_key"].get<std::string>().empty()) {
            const auto k = doc["_key"].get<std::string>();
            key = (k.find(':') == std::string::npos) ? (collection + ":" + k) : k;
        } else {
            std::ostringstream oss = {};
            oss << collection << ":" << (edges_mode ? "edge_" : "row_")
                << std::setw(6) << std::setfill('0') << row_no;
            key = oss.str();
        }

        json op;
        op["op"] = "put";
        op["key"] = key;
        op["blob"] = line;
        operations.push_back(std::move(op));

        if (operations.size() >= batch_size) {
            const int rc = flush_ops(operations, succeeded, failed);
            if (rc != 0) {
                return rc;
            }
        }
    }

    const int rc = flush_ops(operations, succeeded, failed);
    if (rc != 0) {
        return rc;
    }

    if (!g_ctx.raw_json) {
        std::cout << "[" << ok() << "] batch-insert complete\n";
        kv("collection", collection);
        kv("succeeded", std::to_string(succeeded));
        kv("failed", std::to_string(failed));
    } else {
        json out;
        out["collection"] = collection;
        out["succeeded"] = succeeded;
        out["failed"] = failed;
        std::cout << out.dump(2) << "\n";
    }

    return (failed == 0) ? 0 : 1;
}

// ── get ──────────────────────────────────────────────────────────────────────

static int cmdGet(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: themisctl get <entity-id>\n";
        return 2;
    }
    const std::string& id = args[0];
    Response r = httpGet("/entities/" + id);
    if (r.status == 404) {
        std::cerr << "[" << warn() << "] Entity not found: " << id << "\n";
        return 1;
    }
    return handleResponse(r);
}

// ── put ──────────────────────────────────────────────────────────────────────

static int cmdPut(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: themisctl put <entity-id> <json-body>\n";
        return 2;
    }
    const std::string& id   = args[0];
    const std::string& body = args[1];

    // Validate JSON body
    try {
        auto validated_json = json::parse(body);
        (void)validated_json;
    }
    catch (const json::exception& e) {
        std::cerr << "[" << fail() << "] Invalid JSON body: " << e.what() << "\n";
        return 2;
    }

    Response r = httpPut("/entities/" + id, body);
    return handleResponse(r, "Entity '" + id + "' stored.");
}

// ── delete ───────────────────────────────────────────────────────────────────

static int cmdDelete(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: themisctl delete <entity-id>\n";
        return 2;
    }
    const std::string& id = args[0];
    Response r = httpDelete("/entities/" + id);
    if (r.status == 404) {
        std::cerr << "[" << warn() << "] Entity not found: " << id << "\n";
        return 1;
    }
    return handleResponse(r, "Entity '" + id + "' deleted.");
}

// ── schema ───────────────────────────────────────────────────────────────────

static int cmdSchema(const std::vector<std::string>& args) {
    std::string path = "/api/v1/schema";
    if (!args.empty() && args[0] != "--json") {
        path += "/tables/" + args[0];
    }
    Response r = httpGet(path);
    if (r.status == 404) {
        std::cerr << "[" << warn() << "] Table not found.\n";
        return 1;
    }
    return handleResponse(r);
}

// ── config ───────────────────────────────────────────────────────────────────
//
// config get                  — GET /config, pretty-print response
// config set key=value ...    — POST /config with a patch JSON object
//
// Supported dotted keys (mirroring the server handleConfig hot-reload):
//   logging.level              string  (trace|debug|info|warn|error)
//   logging.format             string  (text|json)
//   request_timeout_ms         number  (1000-300000)
//   features.semantic_cache    bool    (true|false)
//   features.llm_store         bool    (true|false)
//   features.cdc               bool    (true|false)
//   features.timeseries        bool    (true|false)
//   cdc_retention_hours        number  (1-8760)

static int cmdConfig(const std::vector<std::string>& args) {
    const std::string sub = args.empty() ? "get" : args[0];

    // ── config get ─────────────────────────────────────────────────────────
    if (sub == "get") {
        Response r = httpGet("/config");
        if (r.status == -1) {
            std::cerr << "[" << fail() << "] " << r.body << "\n";
            return 3;
        }
        if (!r.ok()) {
            std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
            return 1;
        }
        if (g_ctx.raw_json) {
            printJson(r.body);
            return 0;
        }
        try {
            json j = json::parse(r.body);
            std::cout << j.dump(2) << "\n";
        } catch (...) {
            std::cout << r.body << "\n";
        }
        return 0;
    }

    // ── config set key=value ... ───────────────────────────────────────────
    if (sub == "set") {
        const std::vector<std::string> pairs(args.begin() + 1, args.end());
        if (pairs.empty()) {
            std::cerr << "Usage: themisctl config set <key=value> [...]\n"
                      << "Keys: logging.level, logging.format, request_timeout_ms,\n"
                      << "      features.semantic_cache, features.llm_store,\n"
                      << "      features.cdc, features.timeseries,\n"
                      << "      cdc_retention_hours\n";
            return 2;
        }

        // Build a nested patch JSON from the flat key=value pairs
        json patch = json::object();

        for (const auto& pair : pairs) {
            auto eq = pair.find('=');
            if (eq == std::string::npos) {
                std::cerr << "[" << fail() << "] Invalid key=value pair: " << pair
                          << " (missing '=')\n";
                return 2;
            }
            std::string key   = pair.substr(0, eq);
            std::string value = pair.substr(eq + 1);

            // Dotted key → nested JSON (one level only for current schema)
            auto dot = key.find('.');
            if (dot != std::string::npos) {
                std::string outer = key.substr(0, dot);
                std::string inner = key.substr(dot + 1);
                if (!patch.contains(outer) || !patch[outer].is_object()) {
                    patch[outer] = json::object();
                }
                // Coerce booleans and numbers
                if (value == "true") {
                  patch[outer][inner] = true;
                }
                else if (value == "false") patch[outer][inner] = false;
                else {
                    try { patch[outer][inner] = std::stold(value); }
                    catch (...) { patch[outer][inner] = value; }
                }
            } else {
                // Top-level key
                if (value == "true") {
                  patch[key] = true;
                }
                else if (value == "false") patch[key] = false;
                else {
                    try { patch[key] = std::stold(value); }
                    catch (...) { patch[key] = value; }
                }
            }
        }

        Response r = httpPost("/config", patch.dump());
        if (r.status == -1) {
            std::cerr << "[" << fail() << "] " << r.body << "\n";
            return 3;
        }
        if (!r.ok()) {
            std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
            try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
            catch (...) { std::cerr << r.body << "\n"; }
            return 1;
        }
        if (g_ctx.raw_json) {
            printJson(r.body);
        } else {
            std::cout << "[" << ok() << "] Config updated.\n";
            try {
                json j = json::parse(r.body);
                std::cout << j.dump(2) << "\n";
            } catch (...) { /* no body */ }
        }
        return 0;
    }

    // ── config validate [key=value ...] ────────────────────────────────────
    // Dry-run: send proposed changes to POST /config/validate and show a diff
    // between the current config and the validated result.
    //
    // CVL-01..06: schema validation, diff display, dry-run semantics (no
    // server-side mutation), error reporting, raw-JSON mode, empty args mode.
    if (sub == "validate") {
        // Build proposed patch from optional key=value args (same logic as set).
        json proposed = json::object();
        const std::vector<std::string> pairs(args.begin() + 1, args.end());
        for (const auto& pair : pairs) {
            auto eq = pair.find('=');
            if (eq == std::string::npos) {
                std::cerr << "[" << fail() << "] Invalid key=value pair: " << pair
                          << " (missing '=')\n";
                return 2;
            }
            std::string key   = pair.substr(0, eq);
            std::string value = pair.substr(eq + 1);
            auto dot = key.find('.');
            if (dot != std::string::npos) {
                std::string outer = key.substr(0, dot);
                std::string inner = key.substr(dot + 1);
                if (!proposed.contains(outer) || !proposed[outer].is_object()) {
                    proposed[outer] = json::object();
                }
                if (value == "true") {
                  proposed[outer][inner] = true;
                }
                else if (value == "false") proposed[outer][inner] = false;
                else {
                    try { proposed[outer][inner] = std::stold(value); }
                    catch (...) { proposed[outer][inner] = value; }
                }
            } else {
                if (value == "true") {
                  proposed[key] = true;
                }
                else if (value == "false") proposed[key] = false;
                else {
                    try { proposed[key] = std::stold(value); }
                    catch (...) { proposed[key] = value; }
                }
            }
        }

        // Fetch current config for diff base.
        Response current_r = httpGet("/config");
        json current_cfg = {};
        if (current_r.status != -1 && current_r.ok()) {
            try { current_cfg = json::parse(current_r.body); }
            catch (...) { current_cfg = json::object(); }
        }

        // Send validate request (dry-run — server MUST NOT apply changes).
        Response r = httpPost("/config/validate", proposed.dump());
        if (r.status == -1) {
            std::cerr << "[" << fail() << "] " << r.body << "\n";
            return 3;
        }
        if (!r.ok()) {
            if (g_ctx.raw_json) {
                printJson(r.body);
                return 1;
            }
            std::cerr << "[" << fail() << "] Validation failed (HTTP " << r.status << ")\n";
            try {
                json err = json::parse(r.body);
                std::cerr << err.dump(2) << "\n";
            } catch (...) { std::cerr << r.body << "\n"; }
            return 1;
        }

        json validated;
        try { validated = json::parse(r.body); }
        catch (...) { validated = proposed; }

        if (g_ctx.raw_json) {
            printJson(r.body);
            return 0;
        }

        // Print human-readable diff: current → validated.
        std::cout << "[" << ok() << "] Config validation passed (dry-run — no changes applied).\n";
        if (proposed.empty()) {
            std::cout << "  (no changes proposed — server config is valid as-is)\n";
            return 0;
        }

        std::cout << "\n" << col(Color::Bold, "Diff (current → validated):") << "\n";
        for (const auto& [key, val] : proposed.items()) {
            if (val.is_object()) {
                for (const auto& [inner_key, inner_val] : val.items()) {
                    std::string full_key = key + "." + inner_key;
                    json old_val = {};
                    if (current_cfg.contains(key) && current_cfg[key].is_object() &&
                        current_cfg[key].contains(inner_key)) {
                        old_val = current_cfg[key][inner_key];
                    }
                    json new_val = inner_val;
                    if (validated.contains(key) && validated[key].is_object() &&
                        validated[key].contains(inner_key)) {
                        new_val = validated[key][inner_key];
                    }
                    if (old_val != new_val) {
                        std::cout << "  " << col(Color::Cyan, full_key) << ": "
                                  << col(Color::Red, old_val.dump()) << " → "
                                  << col(Color::Bold, new_val.dump()) << "\n";
                    } else {
                        std::cout << "  " << col(Color::Dim, full_key) << ": "
                                  << old_val.dump() << " (unchanged)\n";
                    }
                }
            } else {
                json old_val = current_cfg.contains(key) ? current_cfg[key] : json{};
                json new_val = validated.contains(key) ? validated[key] : val;
                if (old_val != new_val) {
                    std::cout << "  " << col(Color::Cyan, key) << ": "
                              << col(Color::Red, old_val.dump()) << " → "
                              << col(Color::Bold, new_val.dump()) << "\n";
                } else {
                    std::cout << "  " << col(Color::Dim, key) << ": "
                              << old_val.dump() << " (unchanged)\n";
                }
            }
        }
        return 0;
    }

    std::cerr << "Unknown config sub-command: " << sub
              << "\n  Valid sub-commands: get, set, validate\n";
    return 2;
}

// ── branch ───────────────────────────────────────────────────────────────────

static int cmdBranch(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "list") {
        Response r = httpGet("/api/v1/branches");
        if (!r.ok() && r.status != -1) {
            std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
            return 1;
        }
        if (r.status == -1) { std::cerr << "[" << fail() << "] " << r.body << "\n"; return 3; }
        if (g_ctx.raw_json) { printJson(r.body); return 0; }
        try {
            json j = json::parse(r.body);
            auto arr = j.is_array() ? j : (j.contains("branches") ? j["branches"] : json::array());
            if (arr.empty()) {
                std::cout << col(Color::Dim, "(no branches)") << "\n";
            } else {
                for (const auto& b : arr) {
                    std::string name = b.is_string() ? b.get<std::string>()
                                                     : b.value("name", "?");
                    std::cout << "  " << col(Color::Cyan, name) << "\n";
                }
            }
        } catch (...) { std::cout << r.body << "\n"; }
        return 0;
    }

    const std::string sub = args[0];
    const std::vector<std::string> sub_args(args.begin() + 1, args.end());

    if (sub == "create") {
        if (sub_args.empty()) { std::cerr << "Usage: themisctl branch create <name>\n"; return 2; }
        json req; req["name"] = sub_args[0];
        Response r = httpPost("/api/v1/branches", req.dump());
        return handleResponse(r, "Branch '" + sub_args[0] + "' created.");
    }
    if (sub == "switch") {
        if (sub_args.empty()) { std::cerr << "Usage: themisctl branch switch <name>\n"; return 2; }
        Response r = httpPost("/api/v1/branches/" + sub_args[0] + "/switch", "{}");
        return handleResponse(r, "Switched to branch '" + sub_args[0] + "'.");
    }
    if (sub == "delete") {
        if (sub_args.empty()) { std::cerr << "Usage: themisctl branch delete <name>\n"; return 2; }
        Response r = httpDelete("/api/v1/branches/" + sub_args[0]);
        if (r.status == 404) {
            std::cerr << "[" << warn() << "] Branch not found: " << sub_args[0] << "\n";
            return 1;
        }
        return handleResponse(r, "Branch '" + sub_args[0] + "' deleted.");
    }

    std::cerr << "Unknown branch sub-command: " << sub
              << "\n  Valid sub-commands: list, create, switch, delete\n";
    return 2;
}

// ── snapshot ─────────────────────────────────────────────────────────────────

static int cmdSnapshot(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "list") {
        Response r = httpGet("/api/v1/snapshots/tags");
        if (r.status == -1) { std::cerr << "[" << fail() << "] " << r.body << "\n"; return 3; }
        if (!r.ok()) { std::cerr << "[" << fail() << "] HTTP " << r.status << "\n"; return 1; }
        if (g_ctx.raw_json) { printJson(r.body); return 0; }
        try {
            json j = json::parse(r.body);
            auto arr = j.is_array() ? j : (j.contains("tags") ? j["tags"] : json::array());
            if (arr.empty()) {
                std::cout << col(Color::Dim, "(no snapshot tags)") << "\n";
            } else {
                for (const auto& t : arr) {
                    std::string tag = t.is_string() ? t.get<std::string>()
                                                    : t.value("name", "?");
                    std::cout << "  " << col(Color::Cyan, tag) << "\n";
                }
            }
        } catch (...) { std::cout << r.body << "\n"; }
        return 0;
    }

    if (args[0] == "create") {
        json req = {};
        if (args.size() > 1) {
          req["name"] = args[1];
        }
        Response r = httpPost("/api/v1/snapshots/tags", req.dump());
        std::string label = (args.size() > 1) ? ("'" + args[1] + "'") : "unnamed";
        return handleResponse(r, "Snapshot tag " + label + " created.");
    }

    std::cerr << "Unknown snapshot sub-command: " << args[0]
              << "\n  Valid sub-commands: list, create\n";
    return 2;
}

// ── admin ─────────────────────────────────────────────────────────────────────

static int cmdAdmin(const std::vector<std::string>& args) {
    const std::string sub = args.empty() ? "stats" : args[0];

    if (sub == "stats") {
        Response r = httpGet("/api/v1/observability/health");
        if (r.status == -1) { std::cerr << "[" << fail() << "] " << r.body << "\n"; return 3; }
        if (!r.ok()) { std::cerr << "[" << fail() << "] HTTP " << r.status << "\n"; return 1; }
        if (g_ctx.raw_json) { printJson(r.body); return 0; }
        try {
            json j = json::parse(r.body);
            if (j.contains("status")) {
              kv("status",   j["status"].get<std::string>(),  Color::Green);
            }
            if (j.contains("uptime")) {
              kv("uptime",   j["uptime"].get<std::string>());
            }
            if (j.contains("nodes")) {
              kv("nodes",    std::to_string(j["nodes"].get<int>()));
            }
            if (j.contains("version")) {
              kv("version", j["version"].get<std::string>());
            }
            // Dump anything else as formatted JSON
            if (!j.contains("status") && !j.contains("uptime")) {
                std::cout << j.dump(2) << "\n";
            }
        } catch (...) { std::cout << r.body << "\n"; }
        return 0;
    }

    if (sub == "cache") {
        Response r = httpGet("/v1/admin/cache/health");
        if (r.status == -1) { std::cerr << "[" << fail() << "] " << r.body << "\n"; return 3; }
        if (!r.ok()) {
            // Fallback to cache stats endpoint
            r = httpGet("/v1/admin/cache/stats");
            if (!r.ok() && r.status != -1) {
                std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
                return 1;
            }
        }
        if (r.status == -1) { std::cerr << "[" << fail() << "] " << r.body << "\n"; return 3; }
        if (g_ctx.raw_json) { printJson(r.body); return 0; }
        try {
            json j = json::parse(r.body);
            std::cout << j.dump(2) << "\n";
        } catch (...) { std::cout << r.body << "\n"; }
        return 0;
    }

    std::cerr << "Unknown admin sub-command: " << sub
              << "\n  Valid sub-commands: stats, cache\n";
    return 2;
}

// ── provenance-export ─────────────────────────────────────────────────────────
//
// Export provenance records from the observability store.
//
// Usage:
//   themisctl provenance-export [--query-id <id>] [--start-ts <ms>] [--end-ts <ms>]
//                               [--limit <n>] [--format json|csv] [--output <file>]

static int cmdProvenanceExport(const std::vector<std::string>& args) {
    std::string query_id = {};
    int64_t start_ts_ms = -1;
    int64_t end_ts_ms = -1;
    int limit = 1000;
    std::string format = "json";
    std::string output_file = {};

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--query-id") {
            if (i + 1 >= args.size()) {
                std::cerr << "[" << fail() << "] --query-id requires an argument\n";
                return 2;
            }
            query_id = args[++i];
        } else if (arg == "--start-ts") {
            if (i + 1 >= args.size()) {
                std::cerr << "[" << fail() << "] --start-ts requires a numeric argument (milliseconds)\n";
                return 2;
            }
            try {
                start_ts_ms = std::stoll(args[++i]);
            } catch (...) {
                std::cerr << "[" << fail() << "] --start-ts: invalid number: " << args[i] << "\n";
                return 2;
            }
        } else if (arg == "--end-ts") {
            if (i + 1 >= args.size()) {
                std::cerr << "[" << fail() << "] --end-ts requires a numeric argument (milliseconds)\n";
                return 2;
            }
            try {
                end_ts_ms = std::stoll(args[++i]);
            } catch (...) {
                std::cerr << "[" << fail() << "] --end-ts: invalid number: " << args[i] << "\n";
                return 2;
            }
        } else if (arg == "--limit") {
            if (i + 1 >= args.size()) {
                std::cerr << "[" << fail() << "] --limit requires a numeric argument\n";
                return 2;
            }
            try {
                limit = std::stoi(args[++i]);
            } catch (...) {
                std::cerr << "[" << fail() << "] --limit: invalid number: " << args[i] << "\n";
                return 2;
            }
            if (limit <= 0) {
                std::cerr << "[" << fail() << "] --limit must be > 0\n";
                return 2;
            }
        } else if (arg == "--format") {
            if (i + 1 >= args.size()) {
                std::cerr << "[" << fail() << "] --format requires an argument (json or csv)\n";
                return 2;
            }
            format = args[++i];
            if (format != "json" && format != "csv") {
                std::cerr << "[" << fail() << "] --format must be 'json' or 'csv'\n";
                return 2;
            }
        } else if (arg == "--output") {
            if (i + 1 >= args.size()) {
                std::cerr << "[" << fail() << "] --output requires a file path\n";
                return 2;
            }
            output_file = args[++i];
        } else {
            std::cerr << "[" << fail() << "] unknown option: " << arg << "\n";
            return 2;
        }
    }

    // Build query string
    std::ostringstream qs = {};
    qs << "/api/v1/observability/provenance?limit=" << limit;
    if (!query_id.empty()) {
        qs << "&query_id=" << query_id;
    }
    if (start_ts_ms >= 0) {
        qs << "&start_ts_ms=" << start_ts_ms;
    }
    if (end_ts_ms >= 0) {
        qs << "&end_ts_ms=" << end_ts_ms;
    }

    Response r = httpGet(qs.str());
    if (r.status == -1) {
        std::cerr << "[" << fail() << "] Connection error: " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        if (!r.body.empty()) {
            try {
                std::cerr << json::parse(r.body).dump(2) << "\n";
            } catch (...) {
                std::cerr << r.body << "\n";
            }
        }
        return 1;
    }

    // Parse response
    json response_data;
    try {
        response_data = json::parse(r.body);
    } catch (const std::exception& e) {
        std::cerr << "[" << fail() << "] Failed to parse response JSON: " << e.what() << "\n";
        return 1;
    }

    // Extract records array
    json records = json::array();
    if (response_data.is_object() && response_data.contains("records")) {
        records = response_data["records"];
    } else if (response_data.is_array()) {
        records = response_data;
    }

    if (!records.is_array()) {
        std::cerr << "[" << fail() << "] Response does not contain a records array\n";
        return 1;
    }

    // Format and output
    std::ostringstream output_stream = {};
    if (format == "csv") {
        // CSV header
        output_stream << "query_id,operation,timestamp_ms,details\n";

        for (const auto& rec : records) {
            if (!rec.is_object()) {
              continue;
            }

            std::string query_id_str = rec.value("query_id", "");
            std::string operation = rec.value("operation", "");
            int64_t ts_ms = rec.value("timestamp_ms", 0);

            // Escape quotes in details for CSV
            std::string details_str = {};
            if (rec.contains("details")) {
                try {
                    details_str = rec["details"].dump();
                } catch (...) {
                    details_str = "{}";
                }
            }
            // Escape quotes
            for (char c : details_str) {
                if (c == '"') {
                    output_stream << "\"";
                }
                output_stream << c;
            }

            // CSV row
            output_stream << query_id_str << ","
                          << operation << ","
                          << ts_ms << ","
                          << "\"" << details_str << "\"\n";
        }
    } else {
        // JSON output (default)
        json output_obj;
        output_obj["count"] = records.size();
        output_obj["records"] = records;
        output_stream << output_obj.dump(2) << "\n";
    }

    // Write to file or stdout
    if (!output_file.empty()) {
        std::ofstream out(output_file);
        if (!out) {
            std::cerr << "[" << fail() << "] Cannot open output file: " << output_file << "\n";
            return 1;
        }
        out << output_stream.str();
        out.close();
        if (!g_ctx.raw_json) {
            std::cout << "[" << ok() << "] Provenance records exported to: " << output_file << "\n";
            kv("format", format);
            kv("count", std::to_string(records.size()));
        }
    } else {
        std::cout << output_stream.str();
    }

    return 0;
}

// ── self-report ──────────────────────────────────────────────────────────────

static int cmdSelfReport(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cerr << "Usage: themisctl self-report\n";
        return 2;
    }

    struct Probe {
        std::string name;
        std::string path;
        bool required = false;
        bool use_redirect = false;
    };

    const std::vector<Probe> probes = {
        {"health_live", "/health/live", true, false},
        {"health_ready", "/health/ready", true, false},
        {"version", "/version", true, false},
        {"capabilities", "/api/capabilities", false, true},
        {"schema", "/api/v1/schema", false, false},
        {"observability", "/api/v1/observability/health", false, false},
        {"cache", "/v1/admin/cache/health", false, false},
        {"llm_health", "/api/v1/llm/health", false, false},
    };

    json out;
    out["host"] = g_ctx.host;
    out["port"] = g_ctx.port;
    out["sections"] = json::object();

    int required_failures = 0;
    int optional_failures = 0;
    int connection_failures = 0;

    for (const auto& p : probes) {
        Response r = p.use_redirect ? httpGetWithApplicationRedirect(p.path) : httpGet(p.path);

        json section;
        section["path"] = p.path;
        section["required"] = p.required;
        section["http_status"] = r.status;
        section["ok"] = r.ok();

        if (r.status == -1) {
            section["error"] = r.body;
            ++connection_failures;
            if (p.required) {
              ++required_failures;
            }
            else ++optional_failures;
        } else {
            try {
                section["body"] = json::parse(r.body);
            } catch (...) {
                section["raw_body"] = r.body;
            }

            if (!r.ok()) {
                if (p.required) {
                  ++required_failures;
                }
                else ++optional_failures;
            }
        }

        out["sections"][p.name] = section;
    }

    // Cache fallback probe for builds exposing stats but not health endpoint.
    if (out["sections"]["cache"]["http_status"].is_number_integer() &&
        out["sections"]["cache"]["http_status"].get<int>() != 200) {
        Response cache_stats = httpGet("/v1/admin/cache/stats");
        json fallback;
        fallback["path"] = "/v1/admin/cache/stats";
        fallback["http_status"] = cache_stats.status;
        fallback["ok"] = cache_stats.ok();
        if (cache_stats.status == -1) {
            fallback["error"] = cache_stats.body;
        } else {
            try { fallback["body"] = json::parse(cache_stats.body); }
            catch (...) { fallback["raw_body"] = cache_stats.body; }
        }
        out["sections"]["cache_fallback"] = fallback;
    }

    out["summary"] = {
        {"required_failures", required_failures},
        {"optional_failures", optional_failures},
        {"connection_failures", connection_failures},
        {"overall_ok", required_failures == 0}
    };

    if (g_ctx.raw_json) {
        std::cout << out.dump(2) << "\n";
    } else {
        std::cout << col(Color::Bold, "ThemisDB Self Report") << "\n";
        kv("target", g_ctx.host + ":" + std::to_string(g_ctx.port));
        kv("required_failures", std::to_string(required_failures),
           required_failures == 0 ? Color::Green : Color::Red);
        kv("optional_failures", std::to_string(optional_failures),
           optional_failures == 0 ? Color::Green : Color::Yellow);

        auto print_probe_line = [&](const std::string& name, const json& section) {
            const int status = section.value("http_status", -1);
            const bool ok_state = section.value("ok", false);
            const std::string label = ok_state ? ok() : (status == -1 ? fail() : warn());
            std::cout << "  [" << label << "] " << name
                      << " (" << section.value("path", "?")
                      << ") status=" << status << "\n";
        };

        for (const auto& [name, section] : out["sections"].items()) {
            print_probe_line(name, section);
        }

        if (out["sections"].contains("schema")) {
            const auto& schema_section = out["sections"]["schema"];
            if (schema_section.value("ok", false) && schema_section.contains("body")) {
                const auto& body = schema_section["body"];
                if (body.is_object()) {
                    kv("schema_keys", std::to_string(body.size()));
                } else if (body.is_array()) {
                    kv("schema_items", std::to_string(body.size()));
                }
            }
        }
    }

    if (connection_failures > 0) {
      return 3;
    }
    return (required_failures == 0) ? 0 : 1;
}

// ── index ─────────────────────────────────────────────────────────────────────
//
// index recommend [table]   — GET /api/v1/metadata/index_recommendations[/:table]
//
// Without a table argument lists ADD/DROP recommendations for all tracked tables.
// With a table argument restricts output to that table.

static int cmdIndex(const std::vector<std::string>& args) {
    const std::string sub = args.empty() ? "" : args[0];

    if (!sub.empty() && sub != "recommend") {
        std::cerr << "Unknown index sub-command: " << sub
                  << "\n  Valid sub-command: recommend [table]\n";
        return 2;
    }

    // Determine the optional table name (first non-sub argument).
    std::string table_name = {};
    if (sub == "recommend" && args.size() > 1) {
        table_name = args[1];
    }

    std::string path = "/api/v1/metadata/index_recommendations";
    if (!table_name.empty()) {
        path += "/" + table_name;
    }

    Response r = httpGet(path);
    if (r.status == -1) { std::cerr << "[" << fail() << "] " << r.body << "\n"; return 3; }
    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        if (!r.body.empty()) {
            try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
            catch (...) { std::cerr << r.body << "\n"; }
        }
        return 1;
    }

    if (g_ctx.raw_json) { printJson(r.body); return 0; }

    try {
        json j = json::parse(r.body);

        auto print_rec = [](const json& rec) {
            std::string action     = rec.value("action", "?");
            std::string col_name   = rec.value("column_name", "?");
            std::string idx_type   = rec.value("index_type", "?");
            double      score      = rec.value("benefit_score", 0.0);
            std::string rationale  = rec.value("rationale", "");

            std::string action_col = (action == "ADD") ? Color::Green : Color::Yellow;
            std::cout << "  " << col(action_col, action) << "  "
                      << col(Color::Cyan, col_name)
                      << "  (" << idx_type << ")"
                      << "  score=" << std::to_string(static_cast<int>(score + 0.5)) << "\n";
            if (!rationale.empty()) {
                std::cout << "       " << col(Color::Dim, rationale) << "\n";
            }
        };

        if (!table_name.empty()) {
            // Single-table response: {"status":…, "table_name":…, "recommendations":[…]}
            auto recs = j.value("recommendations", json::array());
            if (recs.empty()) {
                std::cout << col(Color::Dim, "(no recommendations for " + table_name + ")") << "\n";
            } else {
                std::cout << col(Color::Bold, table_name) << ":\n";
                for (const auto& rec : recs) { print_rec(rec); }
            }
        } else {
            // All-tables response: {"status":…, "recommendations":{"tbl":[…], …}}
            auto all = j.value("recommendations", json::object());
            bool any = false;
            for (const auto& [tbl, recs] : all.items()) {
                if (!recs.is_array() || recs.empty()) {
                  continue;
                }
                any = true;
                std::cout << col(Color::Bold, tbl) << ":\n";
                for (const auto& rec : recs) { print_rec(rec); }
            }
            if (!any) {
                std::cout << col(Color::Dim, "(no recommendations available)") << "\n";
            }
        }
    } catch (...) {
        std::cout << r.body << "\n";
    }

    return 0;
}

// ── rag ───────────────────────────────────────────────────────────────────────
//
// rag query [--collection <name>] [--top-k <n>] [--lora <id>]
//           [--rag-mode <text|iterative|map_reduce>] [--response-budget-tokens <n>]
//           [--max-tokens <n>] <nl-question>
//
// Sends a natural-language question to the server's AgenticRAG endpoint
// (POST /api/v1/llm/rag) and displays the generated answer together with
// retrieval metadata.
//
// TRQ-01..06: argument validation, successful query, raw-JSON mode,
//   collection/top-k flags, missing question error, connection error.
//
// Note: The endpoint mirrors the server-side LLMApiHandler::handleRAG()
//   request format: {"query": "...", "collection": "...", "top_k": N,
//   "lora_adapter": "...", "rag_mode": "...", "response_budget_tokens": N,
//   "max_tokens": N}. Response fields: "text", "query",
//   "documents_retrieved", "tokens_generated", "inference_time_ms", "cache_hit".

struct LlmRequestOptions {
    bool use_rag = false;
    bool interactive = false;
    std::string collection;
    int top_k = 5;
    std::string lora_id;
    std::string model_id = {};
    int max_tokens = 256;
    double temperature = 0.7;
};

static Response invokeLlmEndpoint(const std::string& user_text,
                                  const LlmRequestOptions& options,
                                  std::string& endpoint_used) {
    json req = {};
    if (options.use_rag) {
        req["query"] = user_text;
        req["top_k"] = options.top_k;
        if (!options.collection.empty()) {
          req["collection"] = options.collection;
        }
        if (!options.lora_id.empty()) {
          req["lora_adapter"] = options.lora_id;
        }
        if (!options.model_id.empty()) {
          req["model"] = options.model_id;
        }
        req["max_tokens"] = options.max_tokens;
        req["temperature"] = options.temperature;
        endpoint_used = "/api/v1/llm/rag";
        return httpPost(endpoint_used, req.dump());
    }

    req["prompt"] = user_text;
    if (!options.model_id.empty()) {
      req["model"] = options.model_id;
    }
    if (!options.lora_id.empty()) {
      req["lora_adapter"] = options.lora_id;
    }
    req["max_tokens"] = options.max_tokens;
    req["temperature"] = options.temperature;
    endpoint_used = "/api/v1/llm/inference";
    return httpPost(endpoint_used, req.dump());
}

static int printLlmResult(const Response& r,
                          const std::string& endpoint_used,
                          const std::string& mode,
                          const std::string& input_text) {
    if (r.status == -1) {
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = false;
            out["error"] = {
                {"type", "connection"},
                {"message", r.body}
            };
            std::cout << out.dump(2) << "\n";
            return 3;
        }
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = false;
            out["http_status"] = r.status;
            try { out["error"] = json::parse(r.body); }
            catch (...) { out["error"] = json{{"message", r.body}}; }
            std::cout << out.dump(2) << "\n";
            return 1;
        }
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
        catch (...) { std::cerr << r.body << "\n"; }
        return 1;
    }

    try {
        json body = json::parse(r.body);
        const std::string text = body.value("text", std::string{});
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = true;
            out["response"] = body;
            std::cout << out.dump(2) << "\n";
            return 0;
        }

        std::cout << col(Color::Bold, "assistant") << ":\n";
        if (!text.empty()) {
            std::cout << text << "\n";
        } else {
            std::cout << body.dump(2) << "\n";
        }
        return 0;
    } catch (...) {
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = true;
            out["raw_response"] = r.body;
            std::cout << out.dump(2) << "\n";
        } else {
            std::cout << r.body << "\n";
        }
        return 0;
    }
}

static bool parseLlmCommonOptions(const std::vector<std::string>& args,
                                  size_t start_index,
                                  LlmRequestOptions& opts,
                                  std::vector<std::string>& text_parts,
                                  const std::string& usage) {
    for (size_t i = start_index; i < args.size(); ++i) {
        if (args[i] == "--rag") {
            opts.use_rag = true;
            continue;
        }
        if (args[i] == "--interactive") {
            opts.interactive = true;
            continue;
        }
        if (args[i] == "--collection" && i + 1 < args.size()) {
            opts.collection = args[++i];
            continue;
        }
        if (args[i] == "--top-k" && i + 1 < args.size()) {
            try { opts.top_k = std::stoi(args[++i]); }
            catch (...) { std::cerr << "[" << fail() << "] --top-k requires integer\n"; return false; }
            continue;
        }
        if (args[i] == "--lora" && i + 1 < args.size()) {
            opts.lora_id = args[++i];
            continue;
        }
        if (args[i] == "--model" && i + 1 < args.size()) {
            opts.model_id = args[++i];
            continue;
        }
        if (args[i] == "--max-tokens" && i + 1 < args.size()) {
            try { opts.max_tokens = std::stoi(args[++i]); }
            catch (...) { std::cerr << "[" << fail() << "] --max-tokens requires integer\n"; return false; }
            continue;
        }
        if (args[i] == "--temperature" && i + 1 < args.size()) {
            try { opts.temperature = std::stod(args[++i]); }
            catch (...) { std::cerr << "[" << fail() << "] --temperature requires number\n"; return false; }
            continue;
        }

        // Remaining args are prompt/task text.
        for (; i < args.size(); ++i) {
          text_parts.push_back(args[i]);
        }
        break;
    }

    if (!opts.interactive && text_parts.empty()) {
        std::cerr << usage << "\n";
        return false;
    }
    return true;
}

static std::string joinParts(const std::vector<std::string>& parts) {
    std::string out = {};
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
          out += ' ';
        }
        out += parts[i];
    }
    return out;
}

struct DocsHelpOptions {
    std::string mode = "lora";  // rag | llm | lora
    std::string user_id = "themisctl";
    std::string lora_id;
    std::string model_id = {};
    int max_tokens = 256;
    double temperature = 0.2;
};

static bool parseDocsHelpOptions(const std::vector<std::string>& args,
                                 DocsHelpOptions& opts,
                                 std::vector<std::string>& question_parts,
                                 const std::string& usage) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--mode" && i + 1 < args.size()) {
            opts.mode = args[++i];
            std::transform(opts.mode.begin(), opts.mode.end(), opts.mode.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            continue;
        }
        if (args[i] == "--user-id" && i + 1 < args.size()) {
            opts.user_id = args[++i];
            continue;
        }
        if (args[i] == "--lora" && i + 1 < args.size()) {
            opts.lora_id = args[++i];
            continue;
        }
        if (args[i] == "--model" && i + 1 < args.size()) {
            opts.model_id = args[++i];
            continue;
        }
        if (args[i] == "--max-tokens" && i + 1 < args.size()) {
            try { opts.max_tokens = std::stoi(args[++i]); }
            catch (...) { std::cerr << "[" << fail() << "] --max-tokens requires integer\n"; return false; }
            continue;
        }
        if (args[i] == "--temperature" && i + 1 < args.size()) {
            try { opts.temperature = std::stod(args[++i]); }
            catch (...) { std::cerr << "[" << fail() << "] --temperature requires number\n"; return false; }
            continue;
        }
        if (args[i] == "--help" || args[i] == "-h" || args[i] == "/?") {
            std::cerr << usage << "\n";
            return false;
        }

        for (; i < args.size(); ++i) {
            question_parts.push_back(args[i]);
        }
        break;
    }

    if (opts.mode != "rag" && opts.mode != "llm" && opts.mode != "lora") {
        std::cerr << "[" << fail() << "] --mode must be one of: rag, llm, lora\n";
        return false;
    }

    if (question_parts.empty()) {
        std::cerr << usage << "\n";
        return false;
    }
    return true;
}

static Response invokeDocsHelpEndpoint(const std::string& question,
                                       const DocsHelpOptions& opts,
                                       std::string& endpoint_used) {
    endpoint_used = "/api/v1/llm/docs/query";
    json req = {};

    if (opts.mode == "rag") {
        req["query"] = question;
    } else if (opts.mode == "llm") {
        req["query"] = std::string("[LLM mode] ") + question;
    } else {
        req["query"] = std::string("[LoRA mode] ") + question;
        if (!opts.lora_id.empty()) {
            req["lora_adapter"] = opts.lora_id;
        }
    }

    if (!opts.model_id.empty()) {
        req["model"] = opts.model_id;
    }
    req["user_id"] = opts.user_id;
    req["max_tokens"] = opts.max_tokens;
    req["temperature"] = opts.temperature;
    return httpPost(endpoint_used, req.dump());
}

static int printDocsHelpResult(const Response& r,
                               const std::string& endpoint_used,
                               const std::string& mode,
                               const std::string& input_text) {
    if (r.status == -1) {
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = false;
            out["error"] = { {"type", "connection"}, {"message", r.body} };
            std::cout << out.dump(2) << "\n";
            return 3;
        }
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = false;
            out["http_status"] = r.status;
            try { out["error"] = json::parse(r.body); }
            catch (...) { out["error"] = json{{"message", r.body}}; }
            std::cout << out.dump(2) << "\n";
            return 1;
        }
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
        catch (...) { std::cerr << r.body << "\n"; }
        return 1;
    }

    try {
        auto body = json::parse(r.body);
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = true;
            out["response"] = body;
            std::cout << out.dump(2) << "\n";
            return 0;
        }

        std::cout << col(Color::Bold, "themis-help") << " (" << mode << "):\n";
        if (body.contains("answer") && body["answer"].is_string()) {
            std::cout << body["answer"].get<std::string>() << "\n";
            return 0;
        }
        if (body.contains("result") && body["result"].is_array() && !body["result"].empty()) {
            const auto& first = body["result"][0];
            if (first.is_string()) {
                std::cout << first.get<std::string>() << "\n";
            } else {
                std::cout << body.dump(2) << "\n";
            }
            return 0;
        }
        std::cout << body.dump(2) << "\n";
        return 0;
    } catch (...) {
        if (g_ctx.raw_json) {
            json out;
            out["mode"] = mode;
            out["endpoint"] = endpoint_used;
            out["input"] = input_text;
            out["ok"] = true;
            out["raw_response"] = r.body;
            std::cout << out.dump(2) << "\n";
        } else {
            std::cout << r.body << "\n";
        }
        return 0;
    }
}

static int cmdDocsHelp(const std::vector<std::string>& args) {
    const std::string usage =
        "Usage: themisctl help [--mode rag|llm|lora] [--user-id <id>] [--lora <id>] "
        "[--model <id>] [--max-tokens <n>] [--temperature <t>] <question>\n"
        "  rag  = docs.db-backed RAG help\n"
        "  llm  = docs.db-backed LLM-style help\n"
        "  lora = docs.db-backed LoRA-style help";

    DocsHelpOptions opts;
    std::vector<std::string> question_parts = {};

    if (!parseDocsHelpOptions(args, opts, question_parts, usage)) {
        return 2;
    }

    const std::string question = joinParts(question_parts);
    std::string endpoint = {};
    Response r = invokeDocsHelpEndpoint(question, opts, endpoint);
    return printDocsHelpResult(r, endpoint, opts.mode, question);
}

// ── chat ──────────────────────────────────────────────────────────────────────

static int cmdChat(const std::vector<std::string>& args) {
    const std::string usage =
        "Usage: themisctl chat [--rag] [--collection <name>] [--top-k <n>] [--lora <id>] "
        "[--model <id>] [--max-tokens <n>] [--temperature <t>] [--interactive] <prompt>";

    LlmRequestOptions opts;
    std::vector<std::string> prompt_parts = {};

    if (!parseLlmCommonOptions(args, 0, opts, prompt_parts, usage)) {
        return 2;
    }

    if (opts.interactive) {
        std::cout << col(Color::Bold, "ThemisDB Chat Mode") << "\n"
                  << "Type your prompt and press Enter. Type 'exit' or 'quit' to leave.\n\n";
        while (true) {
            std::cout << col(Color::Green, "you") << ": " << std::flush;
            std::string line = {};
            if (!std::getline(std::cin, line)) {
                std::cout << "\n";
                break;
            }
            if (line == "exit" || line == "quit") {
              break;
            }
            if (line.empty()) {
              continue;
            }

            std::string endpoint = {};
            Response r = invokeLlmEndpoint(line, opts, endpoint);
            const int rc = printLlmResult(r, endpoint, "chat", line);
            if (rc != 0) {
              return rc;
            }
            std::cout << "\n";
        }
        return 0;
    }

    const std::string prompt = joinParts(prompt_parts);
    std::string endpoint = {};
    Response r = invokeLlmEndpoint(prompt, opts, endpoint);
    return printLlmResult(r, endpoint, "chat", prompt);
}

// ── agent ─────────────────────────────────────────────────────────────────────

static int cmdAgent(const std::vector<std::string>& args) {
    const std::string usage =
        "Usage: themisctl agent [--rag] [--collection <name>] [--top-k <n>] [--lora <id>] "
        "[--model <id>] [--max-tokens <n>] [--temperature <t>] [--interactive] <task>";

    LlmRequestOptions opts;
    opts.use_rag = true; // Agent mode defaults to RAG.
    std::vector<std::string> task_parts = {};

    if (!parseLlmCommonOptions(args, 0, opts, task_parts, usage)) {
        return 2;
    }

    auto wrapAgentPrompt = [](const std::string& task) {
        return std::string("You are ThemisDB Agent. Solve the task with this structure: ") +
               "1) Short plan, 2) Answer, 3) Risks, 4) Next steps. Task: " + task;
    };

    if (opts.interactive) {
        std::cout << col(Color::Bold, "ThemisDB Agent Mode") << "\n"
                  << "Type your task and press Enter. Type 'exit' or 'quit' to leave.\n\n";
        while (true) {
            std::cout << col(Color::Green, "task") << ": " << std::flush;
            std::string line = {};
            if (!std::getline(std::cin, line)) {
                std::cout << "\n";
                break;
            }
            if (line == "exit" || line == "quit") {
              break;
            }
            if (line.empty()) {
              continue;
            }

            std::string endpoint = {};
            Response r = invokeLlmEndpoint(wrapAgentPrompt(line), opts, endpoint);
            const int rc = printLlmResult(r, endpoint, "agent", line);
            if (rc != 0) {
              return rc;
            }
            std::cout << "\n";
        }
        return 0;
    }

    const std::string task = joinParts(task_parts);
    std::string endpoint = {};
    Response r = invokeLlmEndpoint(wrapAgentPrompt(task), opts, endpoint);
    return printLlmResult(r, endpoint, "agent", task);
}

static int cmdRag(const std::vector<std::string>& args) {
    // Usage: rag query [--collection C] [--top-k N] [--lora ID]
    //                  [--rag-mode MODE] [--response-budget-tokens N]
    //                  [--max-tokens N] <question...>
    if (args.empty() || args[0] != "query") {
        std::cerr << "Usage: themisctl rag query [--collection <name>] "
                     "[--top-k <n>] [--lora <adapter-id>] [--rag-mode <mode>] "
                     "[--response-budget-tokens <n>] [--max-tokens <n>] <nl-question>\n";
        return 2;
    }

    // Parse optional flags after "query".
    std::string collection = {};
    int         top_k = 5;
    std::string lora_id = {};
    std::string rag_mode = "text";
    int         response_budget_tokens = 512;
    int         max_tokens = 256;
    std::vector<std::string> question_parts;

    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--collection" && i + 1 < args.size()) {
            collection = args[++i];
        } else if (args[i] == "--top-k" && i + 1 < args.size()) {
            try { top_k = std::stoi(args[++i]); }
            catch (...) {
                std::cerr << "[" << fail() << "] --top-k requires an integer\n";
                return 2;
            }
        } else if (args[i] == "--lora" && i + 1 < args.size()) {
            lora_id = args[++i];
        } else if (args[i] == "--rag-mode" && i + 1 < args.size()) {
            rag_mode = args[++i];
            std::transform(
                rag_mode.begin(),
                rag_mode.end(),
                rag_mode.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::replace(rag_mode.begin(), rag_mode.end(), '-', '_');
            if (rag_mode == "mapreduce") {
                rag_mode = "map_reduce";
            }
        } else if (args[i] == "--response-budget-tokens" && i + 1 < args.size()) {
            try { response_budget_tokens = std::stoi(args[++i]); }
            catch (...) {
                std::cerr << "[" << fail() << "] --response-budget-tokens requires an integer\n";
                return 2;
            }
        } else if (args[i] == "--max-tokens" && i + 1 < args.size()) {
            try { max_tokens = std::stoi(args[++i]); }
            catch (...) {
                std::cerr << "[" << fail() << "] --max-tokens requires an integer\n";
                return 2;
            }
        } else {
            // Remaining tokens form the natural-language question.
            for (; i < args.size(); ++i) {
              question_parts.push_back(args[i]);
            }
        }
    }

    {
        static constexpr std::array<const char*, 3> kAllowedRagModes = {
            "text", "iterative", "map_reduce"
        };
        const bool rag_mode_ok = std::any_of(
            kAllowedRagModes.begin(),
            kAllowedRagModes.end(),
            [&](const char* allowed) { return rag_mode == allowed; });
        if (!rag_mode_ok) {
            std::cerr << "[" << fail() << "] --rag-mode must be one of: text, iterative, map_reduce\n";
            return 2;
        }
    }

    if (question_parts.empty()) {
        std::cerr << "[" << fail() << "] No question provided.\n"
                  << "Usage: themisctl rag query [--collection <name>] "
                     "[--top-k <n>] [--lora <adapter-id>] [--rag-mode <mode>] "
                     "[--response-budget-tokens <n>] [--max-tokens <n>] <nl-question>\n";
        return 2;
    }

    // Join question tokens into a single string.
    std::string question = {};
    for (size_t i = 0; i < question_parts.size(); ++i) {
        if (i > 0) {
          question += ' ';
        }
        question += question_parts[i];
    }

    // Build request body.
    json req_body;
    req_body["query"] = question;
    if (!collection.empty()) {
      req_body["collection"] = collection;
    }
    req_body["top_k"] = top_k;
    if (!lora_id.empty()) {
      req_body["lora_adapter"] = lora_id;
    }
    req_body["rag_mode"] = rag_mode;
    req_body["response_budget_tokens"] = response_budget_tokens;
    req_body["max_tokens"] = max_tokens;

    Response r = httpPost("/api/v1/llm/rag", req_body.dump());
    if (r.status == -1) {
        std::cerr << "[" << fail() << "] " << r.body << "\n";
        return 3;
    }
    if (!r.ok()) {
        std::cerr << "[" << fail() << "] HTTP " << r.status << "\n";
        try { std::cerr << json::parse(r.body).dump(2) << "\n"; }
        catch (...) { std::cerr << r.body << "\n"; }
        return 1;
    }

    if (g_ctx.raw_json) { printJson(r.body); return 0; }

    try {
        json j = json::parse(r.body);

        std::string answer   = j.value("text", "");
        int docs_retrieved   = j.value("documents_retrieved", 0);
        int tokens_gen       = j.value("tokens_generated", 0);
        int infer_ms         = j.value("inference_time_ms", 0);
        bool cache_hit       = j.value("cache_hit", false);

        std::cout << col(Color::Bold, "Answer:") << "\n"
                  << answer << "\n\n"
                  << col(Color::Dim, "documents_retrieved=") << docs_retrieved
                  << col(Color::Dim, "  tokens_generated=") << tokens_gen
                  << col(Color::Dim, "  inference_time_ms=") << infer_ms
                  << col(Color::Dim, "  cache_hit=") << (cache_hit ? "true" : "false")
                  << "\n";
    } catch (...) {
        std::cout << r.body << "\n";
    }

    return 0;
}

// ============================================================================
// REPL support — shared tokeniser used by both cmdRepl and tests
// ============================================================================

/// Split a shell-style line into tokens:
///   - Words separated by whitespace
///   - Single- and double-quoted strings (no escape sequences)
/// Returns false and sets @p error if there is an unterminated quote.
static bool tokenizeLine(const std::string& line,
                         std::vector<std::string>& tokens,
                         std::string& error) {
    tokens.clear();
    error.clear();
    std::string current = {};
    bool in_single = false;
    bool in_double = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_single) {
            if (c == '\'') { in_single = false; }
            else           { current += c; }
        } else if (in_double) {
            if (c == '"') { in_double = false; }
            else          { current += c; }
        } else {
            if (c == '\'') {
                in_single = true;
            } else if (c == '"') {
                in_double = true;
            } else if (c == ' ' || c == '\t') {
                if (!current.empty()) { tokens.push_back(current); current.clear(); }
            } else {
                current += c;
            }
        }
    }
    if (in_single || in_double) {
        error = "Unterminated quote";
        return false;
    }
    if (!current.empty()) {
      tokens.push_back(current);
    }
    return true;
}

// ── repl ─────────────────────────────────────────────────────────────────────

static int cmdRepl(const std::vector<std::string>& /*args*/);

// Forward-declare dispatch table lookup so cmdRepl can call it
static int dispatchCommand(const std::string& cmd,
                           const std::vector<std::string>& cmd_args);

static int cmdRepl(const std::vector<std::string>& /*args*/) {
#if THEMISCTL_HAS_READLINE
    // Use GNU readline for line editing and persistent history
    const std::string hist_file = (std::getenv("HOME") ? std::string(std::getenv("HOME")) : "/tmp")
                                  + "/.themisctl_history";
    read_history(hist_file.c_str());
    rl_bind_key('\t', rl_complete);
#endif

    const std::string prompt_str =
        col(Color::Green, "themisctl") + col(Color::Dim, "> ");

    std::cout << col(Color::Bold, "ThemisDB Interactive Shell") << "\n"
              << "Connected to " << col(Color::Cyan, g_ctx.host + ":" + std::to_string(g_ctx.port)) << "\n"
              << "Type " << col(Color::Yellow, "help") << " for available commands, "
              << col(Color::Yellow, "exit") << " or " << col(Color::Yellow, "quit") << " to leave.\n\n";

    while (true) {
        std::string line = {};

#if THEMISCTL_HAS_READLINE
        char* rl_line = readline(prompt_str.c_str());
        if (!rl_line) {
            // EOF (Ctrl-D)
            std::cout << "\n";
            break;
        }
        line = rl_line;
        free(rl_line);
        if (!line.empty()) {
            add_history(line.c_str());
            append_history(1, hist_file.c_str());
        }
#else
        std::cout << prompt_str << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }
#endif

        // Strip leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
          continue;
        }
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t");
        if (end != std::string::npos) {
          line = line.substr(0, end + 1);
        }
        if (line.empty() || line[0] == '#') {
          continue;
        }

        // Built-in REPL commands
        if (line == "exit" || line == "quit") {
          break;
        }

        // Tokenise
        std::vector<std::string> tokens;
        std::string tok_err = {};
        if (!tokenizeLine(line, tokens, tok_err)) {
            std::cerr << "[" << fail() << "] " << tok_err << "\n";
            continue;
        }
        if (tokens.empty()) {
          continue;
        }

        const std::string cmd = tokens[0];
        const std::vector<std::string> cmd_args(tokens.begin() + 1, tokens.end());

        // Dispatch
        dispatchCommand(cmd, cmd_args);
    }

#if THEMISCTL_HAS_READLINE
    write_history(hist_file.c_str());
#endif
    return 0;
}

// ============================================================================
// Help
// ============================================================================

static void printHelp(const char* prog) {
    if (g_help_routes_json) {
        json out;
        out["source"] = "fallback";
        out["warnings"] = json::array();
        out["routes"] = json::array();

        std::vector<std::pair<std::string, std::string>> routes;
        std::string openapi_warning = {};
        if (tryCollectOpenApiRoutes(routes, openapi_warning)) {
            out["source"] = "openapi";
        } else {
            if (!openapi_warning.empty()) {
                out["warnings"].push_back(std::string("openapi: ") + openapi_warning);
            }

            Response caps = httpGetWithApplicationRedirect("/api/capabilities");
            if (caps.ok()) {
                try {
                    auto c = json::parse(caps.body);
                    routes = generateRoutesFromCapabilities(c);
                    out["source"] = "capabilities";
                } catch (...) {
                    out["warnings"].push_back("capabilities: parse failed");
                }
            } else {
                if (caps.status == -1) {
                    out["warnings"].push_back(std::string("capabilities: ") + caps.body);
                } else {
                    out["warnings"].push_back(
                        std::string("capabilities: HTTP ") + std::to_string(caps.status));
                }
            }
        }

        if (routes.empty()) {
            routes = generateRoutesFromCapabilities(json::object());
            out["source"] = "fallback";
        }

        for (const auto& [method, path] : routes) {
            out["routes"].push_back({{"method", method}, {"path", path}});
        }

        std::cout << out.dump(2) << "\n";
        return;
    }

    std::cout
        << col(Color::Bold, "themisctl") << " — ThemisDB unified management CLI\n\n"
        << "Usage: " << prog << " [global-options] <command> [command-options]\n\n"
        << col(Color::Bold, "Global options") << ":\n"
        << "  --host <h>      ThemisDB host       (default: localhost / $THEMIS_HOST)\n"
        << "  --port <p>      ThemisDB port       (default: 8765   / $THEMIS_PORT)\n"
        << "  --token <jwt>   Bearer auth token   ($THEMIS_TOKEN)\n"
        << "  --timeout <s>   Request timeout     (default: 30 s)\n"
        << "  --json          Print raw JSON responses\n"
        << "  --routes-json   With --help, print generated endpoints as JSON\n"
        << "  --no-color      Disable ANSI color output\n"
        << "  --help, -h, /?  Print this help\n\n"
        << col(Color::Bold, "Commands") << ":\n"
        << "  " << col(Color::Cyan, "health")
            << "                       Check server liveness and readiness\n"
        << "  " << col(Color::Cyan, "version")
            << "                      Print server version information\n"
        << "  " << col(Color::Cyan, "capabilities")
            << " [--openapi]         Show capabilities and optional route list\n"
        << "  " << col(Color::Cyan, "query") << " <aql>"
            << "                  Execute an AQL query\n"
        << "  " << col(Color::Cyan, "api") << " <METHOD> <PATH> [--body <data>|--body-file <file>|--stdin]"
            << "\n                              Generic HTTP call for any endpoint\n"
        << "  " << col(Color::Cyan, "batch-insert") << " --collection <name> [--batch-size <n>] [--edges]"
            << "\n                              Import JSONL rows via /entities/batch\n"
        << "  " << col(Color::Cyan, "get") << " <id>"
            << "                    Retrieve an entity by key\n"
        << "  " << col(Color::Cyan, "put") << " <id> <json>"
            << "             Create or update an entity\n"
        << "  " << col(Color::Cyan, "delete") << " <id>"
            << "                 Delete an entity\n"
        << "  " << col(Color::Cyan, "schema") << " [table]"
            << "               Show schema (all tables or a specific one)\n"
        << "  " << col(Color::Cyan, "config") << " get|set|validate [key=value ...]"
            << "  Read, hot-reload, or dry-run validate server config\n"
        << "  " << col(Color::Cyan, "branch") << " list|create|switch|delete\n"
        << "  " << col(Color::Cyan, "snapshot") << " list|create [tag]\n"
        << "  " << col(Color::Cyan, "admin") << " stats|cache"
            << "             Show observability/cache statistics\n"
        << "  " << col(Color::Cyan, "self-report")
            << "                  Bundle self-disclosure (content + health)\n"
        << "  " << col(Color::Cyan, "index") << " recommend [table]"
            << "        Show automatic index recommendations\n"
        << "  " << col(Color::Cyan, "help") << " [--mode rag|llm|lora] [--user-id <id>] <question>"
            << "\n                              ThemisDB docs.db-backed Hilfe (RAG/LLM/LoRA)\n"
        << "  " << col(Color::Cyan, "chat") << " [--rag] [--collection <name>] [--top-k <n>] [--lora <id>]"
            << "\n                              Chat via LLM inference or RAG\n"
        << "  " << col(Color::Cyan, "agent") << " [--rag] [--collection <name>] [--top-k <n>] [--lora <id>]"
            << "\n                              Agent mode (planning-style response)\n"
        << "  " << col(Color::Cyan, "rag") << " query [--collection <name>] [--top-k <n>] [--lora <id>] <question>"
            << "\n                              AgenticRAG natural-language query\n"
        << "  " << col(Color::Cyan, "repl")
            << "                        Start interactive REPL (with history)\n\n"
        << col(Color::Bold, "Examples") << ":\n"
        << "  " << prog << " health\n"
        << "  " << prog << " --host db.internal --port 9000 version\n"
        << "  " << prog << " capabilities --openapi\n"
        << "  " << prog << " query 'FOR d IN users FILTER d.active == true RETURN d'\n"
        << "  " << prog << " api GET /api/v1/observability/health\n"
        << "  " << prog << " api POST /entities/batch --body-file ops.json\n"
        << "  " << prog << " batch-insert --collection demo_articles < demo_articles.jsonl\n"
        << "  " << prog << " get user:42\n"
        << "  " << prog << " put user:42 '{\"name\":\"Alice\",\"active\":true}'\n"
        << "  " << prog << " delete user:42\n"
        << "  " << prog << " schema users\n"
        << "  " << prog << " config get\n"
        << "  " << prog << " config set logging.level=debug request_timeout_ms=60000\n"
        << "  " << prog << " config set features.cdc=true\n"
        << "  " << prog << " config validate features.cdc=true logging.level=info\n"
        << "  " << prog << " branch list\n"
        << "  " << prog << " branch create feature-x\n"
        << "  " << prog << " snapshot create v1.2.0\n"
        << "  " << prog << " admin stats\n"
        << "  " << prog << " --json admin cache\n"
        << "  " << prog << " --json self-report\n"
        << "  " << prog << " index recommend\n"
        << "  " << prog << " index recommend users\n"
        << "  " << prog << " help --mode lora How do I configure sharding safely?\n"
        << "  " << prog << " chat --rag --collection demo_articles --lora legal-lora Explain ACID briefly\n"
        << "  " << prog << " --json agent --collection demo_articles --top-k 5 Analyze transaction risk\n"
        << "  " << prog << " rag query 'Welche Unterlagen fehlen für den Bauantrag?'\n"
        << "  " << prog << " rag query --collection procs --top-k 10 What is the next step?\n"
        << "  " << prog << " rag query --collection procs --rag-mode iterative --response-budget-tokens 400 --max-tokens 64 Summarize next actions\n"
        << "  " << prog << " repl\n";

    std::cout << "\n" << col(Color::Bold, "Generated endpoints (live from server)") << ":\n";
    std::vector<std::pair<std::string, std::string>> routes;
    std::string openapi_warning = {};
    if (tryCollectOpenApiRoutes(routes, openapi_warning)) {
        for (const auto& [method, path] : routes) {
            std::cout << "  " << col(Color::Cyan, method) << " " << path << "\n";
        }
        return;
    }

    std::cout << "  [" << warn() << "] OpenAPI-Discovery nicht verfuegbar: "
              << openapi_warning << "\n";

    Response caps = httpGetWithApplicationRedirect("/api/capabilities");
    if (caps.ok()) {
        try {
            auto c = json::parse(caps.body);
            auto cap_routes = generateRoutesFromCapabilities(c);
            if (!cap_routes.empty()) {
                std::cout << "  " << col(Color::Bold, "Capability profile routes") << ":\n";
                for (const auto& [method, path] : cap_routes) {
                    std::cout << "    " << col(Color::Cyan, method) << " " << path << "\n";
                }
                return;
            }
        } catch (...) {
            // Keep fallback warning below.
        }
    }

    if (caps.status == -1) {
        std::cout << "  [" << warn() << "] Server nicht erreichbar: " << caps.body << "\n";
    } else {
        std::cout << "  [" << warn() << "] /api/capabilities nicht verfuegbar (HTTP "
                  << caps.status << ")\n";
    }
    auto fallback_routes = generateRoutesFromCapabilities(json::object());
    if (!fallback_routes.empty()) {
        std::cout << "  " << col(Color::Bold, "Fallback profile routes") << ":\n";
        for (const auto& [method, path] : fallback_routes) {
            std::cout << "    " << col(Color::Cyan, method) << " " << path << "\n";
        }
    }
    std::cout << "  [" << warn() << "] Verwende: themisctl api <METHOD> <PATH> fuer direkte Aufrufe\n";
}

// ============================================================================
// Dispatch helper (used by both main and REPL)
// ============================================================================

static int dispatchCommand(const std::string& cmd,
                           const std::vector<std::string>& cmd_args) {
    using CmdFn = int(*)(const std::vector<std::string>&);
    static const std::unordered_map<std::string, CmdFn> dispatch = {
        {"health",   cmdHealth},
        {"version",  cmdVersion},
        {"capabilities", cmdCapabilities},
        {"query",    cmdQuery},
        {"api",      cmdApi},
        {"batch-insert", cmdBatchInsert},
        {"get",      cmdGet},
        {"put",      cmdPut},
        {"delete",   cmdDelete},
        {"schema",   cmdSchema},
        {"config",   cmdConfig},
        {"branch",   cmdBranch},
        {"snapshot", cmdSnapshot},
        {"admin",    cmdAdmin},
        {"provenance-export", cmdProvenanceExport},
        {"self-report", cmdSelfReport},
        {"index",    cmdIndex},
        {"chat",     cmdChat},
        {"agent",    cmdAgent},
        {"rag",      cmdRag},
        {"repl",     cmdRepl},
    };

    if (cmd == "help") {
        if (cmd_args.empty()) {
            printHelp("themisctl");
            return 0;
        }
        return cmdDocsHelp(cmd_args);
    }
    if (is_help_flag(cmd)) {
        printHelp("themisctl");
        return 0;
    }

    auto it = dispatch.find(cmd);
    if (it == dispatch.end()) {
        std::cerr << "Unknown command: " << cmd << "\n"
                  << "Type 'help' for available commands.\n";
        return 2;
    }
    return it->second(cmd_args);
}

// ============================================================================
// main
// ============================================================================

#ifndef THEMISCTL_TEST_BUILD
int main(int argc, char* argv[]) {
    // ── Read environment defaults ────────────────────────────────────────────
    if (const char* env = std::getenv("THEMIS_HOST")) {
      g_ctx.host  = env;
    }
    if (const char* env = std::getenv("THEMIS_PORT")) {
      g_ctx.port  = std::atoi(env);
    }
    if (const char* env = std::getenv("THEMIS_TOKEN")) {
      g_ctx.token = env;
    }

    std::vector<std::string> all_args(argv + 1, argv + argc);

    ThemisCtlGlobalOptions parsed_options;
    parsed_options.host = g_ctx.host;
    parsed_options.port = g_ctx.port;
    parsed_options.token = g_ctx.token;
    parsed_options.timeout = g_ctx.timeout;

    std::string parse_error = {};
    if (!parse_global_options(all_args, parsed_options, parse_error)) {
        std::cerr << parse_error << "\n";
        printHelp(argv[0]);
        return 2;
    }

    g_use_color = !parsed_options.no_color;
    g_ctx.raw_json = parsed_options.raw_json;
    g_help_routes_json = parsed_options.routes_json;
    g_ctx.host = parsed_options.host;
    g_ctx.port = parsed_options.port;
    g_ctx.token = parsed_options.token;
    g_ctx.timeout = parsed_options.timeout;
    all_args = parsed_options.remaining_args;

    if (parsed_options.show_help) {
        printHelp(argv[0]);
        return 0;
    }

    if (all_args.empty()) {
        printHelp(argv[0]);
        return 2;
    }

    const std::string command = all_args[0];
    const std::vector<std::string> cmd_args(all_args.begin() + 1, all_args.end());

    return dispatchCommand(command, cmd_args);
}
#endif  // THEMISCTL_TEST_BUILD
