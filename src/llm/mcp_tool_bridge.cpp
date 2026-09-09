/**
 * @file mcp_tool_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#if defined(THEMIS_ENABLE_MCP) && defined(THEMIS_ENABLE_LLM)

#include "llm/ai_orchestrator.h"
#include "server/mcp_server.h"
#include <spdlog/spdlog.h>

namespace themis::llm {

// ============================================================================
// McpToolBridge helpers
// ============================================================================

namespace {

/// Build a JSON-RPC 2.0 "tools/call" request envelope for the MCP server.
json buildMcpToolCallRequest(const std::string& tool_name, const json& args) {
    return {
        {"jsonrpc", "2.0"},
        {"id",      1},
        {"method",  "tools/call"},
        {"params",  {{"name", tool_name}, {"arguments", args}}}
    };
}

/// Extract the result payload from an MCP JSON-RPC response.
json extractMcpResult(const json& mcp_response) {
    if (mcp_response.contains("error")) {
        return {{"error", mcp_response["error"]}};
    }
    if (mcp_response.contains("result")) {
        return mcp_response["result"];
    }
    return mcp_response;
}

} // namespace

// ============================================================================
// McpToolBridge::bridgeTools
// ============================================================================

void McpToolBridge::bridgeTools(themis::server::McpServer& mcp,
                                 ToolRegistry&               registry,
                                 const std::string&          prefix) {
    // LIFETIME CONTRACT: The lambdas registered in `registry` capture `mcp` by
    // reference.  The caller MUST ensure that `mcp` outlives every ToolRegistry
    // that holds bridged handlers.  In practice, both McpServer and the
    // AIOrchestrator ToolRegistry are long-lived server objects that are
    // constructed together and destroyed together, so this is safe.
    // Retrieve the tool list from the MCP server by issuing a tools/list call.
    json list_req = {
        {"jsonrpc", "2.0"},
        {"id",      0},
        {"method",  "tools/list"},
        {"params",  json::object()}
    };

    json list_resp = mcp.handleRequest(list_req);

    json tools_array = {};
    if (list_resp.contains("result") && list_resp["result"].contains("tools")) {
        tools_array = list_resp["result"]["tools"];
    } else {
        spdlog::warn("[McpToolBridge] tools/list returned unexpected structure – no tools bridged");
        return;
    }

    int bridged = 0;
    for (const auto& t : tools_array) {
        const std::string name = t.value("name", "");
        if (name.empty()) {
          continue;
        }

        const std::string alias       = prefix + name;
        const std::string description = t.value("description", "");
        const json        schema      = t.value("inputSchema", json::object());

        ToolSpec spec;
        spec.name        = alias;
        spec.description = description;
        spec.args_schema = schema;
        spec.timeout_ms  = 10000;  // generous default for MCP round-trips

        // Capture by value to avoid dangling references in the lambda
        const std::string captured_name = name;
        registry.registerTool(spec,
            [&mcp, captured_name](const json& args, const ModeSpec&) -> json {
                const json req  = buildMcpToolCallRequest(captured_name, args);
                const json resp = mcp.handleRequest(req);
                return extractMcpResult(resp);
            });

        spdlog::debug("[McpToolBridge] Bridged MCP tool '{}' as '{}'", name, alias);
        ++bridged;
    }

    spdlog::info("[McpToolBridge] Bridged {} MCP tool(s) into ToolRegistry (prefix='{}')",
                 bridged, prefix);
}

// ============================================================================
// McpToolBridge::bridgeTool
// ============================================================================

void McpToolBridge::bridgeTool(themis::server::McpServer& mcp,
                                const std::string&         tool_name,
                                ToolRegistry&               registry,
                                const std::string&          alias) {
    // LIFETIME CONTRACT: see bridgeTools() – mcp must outlive the registry entry.
    const std::string effective_alias = alias.empty() ? tool_name : alias;

    ToolSpec spec;
    spec.name        = effective_alias;
    spec.description = "MCP tool '" + tool_name + "' (bridged from McpServer)";
    spec.timeout_ms  = 10000;

    const std::string captured_name = tool_name;
    registry.registerTool(spec,
        [&mcp, captured_name](const json& args, const ModeSpec&) -> json {
            const json req  = buildMcpToolCallRequest(captured_name, args);
            const json resp = mcp.handleRequest(req);
            return extractMcpResult(resp);
        });

    spdlog::debug("[McpToolBridge] Bridged single MCP tool '{}' as '{}'",
                  tool_name, effective_alias);
}

} // namespace themis::llm

#endif // THEMIS_ENABLE_MCP && THEMIS_ENABLE_LLM
