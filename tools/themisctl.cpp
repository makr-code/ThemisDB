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
 *   query <aql>                  Execute an AQL query
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
 *   index recommend [table]      Show automatic index recommendations
 *   rag query [--collection C] [--top-k N] [--lora ID] <question>
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
#include <cstdlib>
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
            std::string port_value;
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
            std::string timeout_value;
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
    std::string body;
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
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Get(path.c_str(), makeHeaders());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpPost(const std::string& path, const std::string& body,
                         const std::string& ctype = "application/json") {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Post(path.c_str(), makeHeaders(), body, ctype.c_str());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpPut(const std::string& path, const std::string& body,
                        const std::string& ctype = "application/json") {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Put(path.c_str(), makeHeaders(), body, ctype.c_str());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
}

static Response httpDelete(const std::string& path) {
    httplib::Client cli(g_ctx.host, g_ctx.port);
    cli.set_connection_timeout(g_ctx.timeout);
    cli.set_read_timeout(g_ctx.timeout);
    auto res = cli.Delete(path.c_str(), makeHeaders());
    if (!res) return {-1, "Connection error: " + httplib::to_string(res.error())};
    return {res->status, res->body};
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
        if (r.status == -1)                      return col(Color::Red,    "unreachable");
        if (r.status >= 200 && r.status < 300)   return col(Color::Green,  "healthy");
        if (r.status == 503)                      return col(Color::Yellow, "unavailable");
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
        if (j.contains("version")) kv("version", j["version"].get<std::string>());
        if (j.contains("build"))   kv("build",   j["build"].get<std::string>());
        if (j.contains("commit"))  kv("commit",  j["commit"].get<std::string>());
    } catch (...) {
        std::cout << r.body << "\n";
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
    std::string aql;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) aql += ' ';
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
                if (value == "true")       patch[outer][inner] = true;
                else if (value == "false") patch[outer][inner] = false;
                else {
                    try { patch[outer][inner] = std::stold(value); }
                    catch (...) { patch[outer][inner] = value; }
                }
            } else {
                // Top-level key
                if (value == "true")       patch[key] = true;
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
                if (value == "true")       proposed[outer][inner] = true;
                else if (value == "false") proposed[outer][inner] = false;
                else {
                    try { proposed[outer][inner] = std::stold(value); }
                    catch (...) { proposed[outer][inner] = value; }
                }
            } else {
                if (value == "true")       proposed[key] = true;
                else if (value == "false") proposed[key] = false;
                else {
                    try { proposed[key] = std::stold(value); }
                    catch (...) { proposed[key] = value; }
                }
            }
        }

        // Fetch current config for diff base.
        Response current_r = httpGet("/config");
        json current_cfg;
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
                    json old_val;
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
        json req;
        if (args.size() > 1) req["name"] = args[1];
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
            if (j.contains("status")) kv("status",   j["status"].get<std::string>(),  Color::Green);
            if (j.contains("uptime")) kv("uptime",   j["uptime"].get<std::string>());
            if (j.contains("nodes"))  kv("nodes",    std::to_string(j["nodes"].get<int>()));
            if (j.contains("version")) kv("version", j["version"].get<std::string>());
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
    std::string table_name;
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
                if (!recs.is_array() || recs.empty()) continue;
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
// rag query [--collection <name>] [--top-k <n>] [--lora <id>] <nl-question>
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
//   "lora_adapter": "..."}.  Response fields: "text", "query",
//   "documents_retrieved", "tokens_generated", "inference_time_ms", "cache_hit".

static int cmdRag(const std::vector<std::string>& args) {
    // Usage: rag query [--collection C] [--top-k N] [--lora ID] <question...>
    if (args.empty() || args[0] != "query") {
        std::cerr << "Usage: themisctl rag query [--collection <name>] "
                     "[--top-k <n>] [--lora <adapter-id>] <nl-question>\n";
        return 2;
    }

    // Parse optional flags after "query".
    std::string collection;
    int         top_k = 5;
    std::string lora_id;
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
        } else {
            // Remaining tokens form the natural-language question.
            for (; i < args.size(); ++i) question_parts.push_back(args[i]);
        }
    }

    if (question_parts.empty()) {
        std::cerr << "[" << fail() << "] No question provided.\n"
                  << "Usage: themisctl rag query [--collection <name>] "
                     "[--top-k <n>] [--lora <adapter-id>] <nl-question>\n";
        return 2;
    }

    // Join question tokens into a single string.
    std::string question;
    for (size_t i = 0; i < question_parts.size(); ++i) {
        if (i > 0) question += ' ';
        question += question_parts[i];
    }

    // Build request body.
    json req_body;
    req_body["query"] = question;
    if (!collection.empty()) req_body["collection"] = collection;
    req_body["top_k"] = top_k;
    if (!lora_id.empty()) req_body["lora_adapter"] = lora_id;

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
    std::string current;
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
    if (!current.empty()) tokens.push_back(current);
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
        std::string line;

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
        if (start == std::string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t");
        if (end != std::string::npos) line = line.substr(0, end + 1);
        if (line.empty() || line[0] == '#') continue;

        // Built-in REPL commands
        if (line == "exit" || line == "quit") break;

        // Tokenise
        std::vector<std::string> tokens;
        std::string tok_err;
        if (!tokenizeLine(line, tokens, tok_err)) {
            std::cerr << "[" << fail() << "] " << tok_err << "\n";
            continue;
        }
        if (tokens.empty()) continue;

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
    std::cout
        << col(Color::Bold, "themisctl") << " — ThemisDB unified management CLI\n\n"
        << "Usage: " << prog << " [global-options] <command> [command-options]\n\n"
        << col(Color::Bold, "Global options") << ":\n"
        << "  --host <h>      ThemisDB host       (default: localhost / $THEMIS_HOST)\n"
        << "  --port <p>      ThemisDB port       (default: 8765   / $THEMIS_PORT)\n"
        << "  --token <jwt>   Bearer auth token   ($THEMIS_TOKEN)\n"
        << "  --timeout <s>   Request timeout     (default: 30 s)\n"
        << "  --json          Print raw JSON responses\n"
        << "  --no-color      Disable ANSI color output\n"
        << "  --help, -h, /?  Print this help\n\n"
        << col(Color::Bold, "Commands") << ":\n"
        << "  " << col(Color::Cyan, "health")
            << "                       Check server liveness and readiness\n"
        << "  " << col(Color::Cyan, "version")
            << "                      Print server version information\n"
        << "  " << col(Color::Cyan, "query") << " <aql>"
            << "                  Execute an AQL query\n"
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
        << "  " << col(Color::Cyan, "index") << " recommend [table]"
            << "        Show automatic index recommendations\n"
        << "  " << col(Color::Cyan, "rag") << " query [--collection <name>] [--top-k <n>] [--lora <id>] <question>"
            << "\n                              AgenticRAG natural-language query\n"
        << "  " << col(Color::Cyan, "repl")
            << "                        Start interactive REPL (with history)\n\n"
        << col(Color::Bold, "Examples") << ":\n"
        << "  " << prog << " health\n"
        << "  " << prog << " --host db.internal --port 9000 version\n"
        << "  " << prog << " query 'FOR d IN users FILTER d.active == true RETURN d'\n"
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
        << "  " << prog << " index recommend\n"
        << "  " << prog << " index recommend users\n"
        << "  " << prog << " rag query 'Welche Unterlagen fehlen für den Bauantrag?'\n"
        << "  " << prog << " rag query --collection procs --top-k 10 What is the next step?\n"
        << "  " << prog << " repl\n";
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
        {"query",    cmdQuery},
        {"get",      cmdGet},
        {"put",      cmdPut},
        {"delete",   cmdDelete},
        {"schema",   cmdSchema},
        {"config",   cmdConfig},
        {"branch",   cmdBranch},
        {"snapshot", cmdSnapshot},
        {"admin",    cmdAdmin},
        {"index",    cmdIndex},
        {"rag",      cmdRag},
        {"repl",     cmdRepl},
    };

    if (cmd == "help" || is_help_flag(cmd)) {
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
    if (const char* env = std::getenv("THEMIS_HOST"))  g_ctx.host  = env;
    if (const char* env = std::getenv("THEMIS_PORT"))  g_ctx.port  = std::atoi(env);
    if (const char* env = std::getenv("THEMIS_TOKEN")) g_ctx.token = env;

    std::vector<std::string> all_args(argv + 1, argv + argc);

    ThemisCtlGlobalOptions parsed_options;
    parsed_options.host = g_ctx.host;
    parsed_options.port = g_ctx.port;
    parsed_options.token = g_ctx.token;
    parsed_options.timeout = g_ctx.timeout;

    std::string parse_error;
    if (!parse_global_options(all_args, parsed_options, parse_error)) {
        std::cerr << parse_error << "\n";
        printHelp(argv[0]);
        return 2;
    }

    if (parsed_options.show_help) {
        printHelp(argv[0]);
        return 0;
    }

    g_use_color = !parsed_options.no_color;
    g_ctx.raw_json = parsed_options.raw_json;
    g_ctx.host = std::move(parsed_options.host);
    g_ctx.port = parsed_options.port;
    g_ctx.token = std::move(parsed_options.token);
    g_ctx.timeout = parsed_options.timeout;
    all_args = std::move(parsed_options.remaining_args);

    if (all_args.empty()) {
        printHelp(argv[0]);
        return 2;
    }

    const std::string command = all_args[0];
    const std::vector<std::string> cmd_args(all_args.begin() + 1, all_args.end());

    return dispatchCommand(command, cmd_args);
}
#endif  // THEMISCTL_TEST_BUILD
