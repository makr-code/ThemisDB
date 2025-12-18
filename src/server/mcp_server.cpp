#ifdef THEMIS_ENABLE_MCP

#include "server/mcp_server.h"
#include "server/http_server.h"
#include <spdlog/spdlog.h>
#include <iostream>

namespace themis {
namespace server {

// ============================================================================
// McpServer Implementation
// ============================================================================

McpServer::McpServer(asio::io_context& io_context, const Config& config)
    : io_context_(io_context), config_(config) {
    spdlog::info("MCP Server initializing with transports: stdio={}, sse={}, websocket={}",
                 config_.enable_stdio, config_.enable_sse, config_.enable_websocket);
}

McpServer::~McpServer() {
    stop();
}

void McpServer::start() {
    if (is_running_) {
        spdlog::warn("MCP Server already running");
        return;
    }

    spdlog::info("Starting MCP Server '{}'  version {}", config_.server_name, config_.server_version);

    // Register default tools, resources, and prompts
    registerDefaultTools();
    registerDefaultResources();
    registerDefaultPrompts();

    // Initialize transports
    if (config_.enable_stdio) {
        stdio_transport_ = std::make_shared<StdioTransport>(io_context_, config_.stdio_buffer_size);
        stdio_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
        stdio_transport_->start();
        spdlog::info("MCP stdio transport started");
    }

    if (config_.enable_sse) {
        sse_transport_ = std::make_shared<SseTransport>(io_context_, config_.sse_keepalive_ms);
        sse_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
        sse_transport_->start();
        spdlog::info("MCP SSE transport started");
    }

    if (config_.enable_websocket) {
        ws_transport_ = std::make_shared<WebSocketTransport>(io_context_, config_.websocket_ping_interval_ms);
        ws_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
        ws_transport_->start();
        spdlog::info("MCP WebSocket transport started");
    }

    is_running_ = true;
    spdlog::info("MCP Server started successfully");
}

void McpServer::stop() {
    if (!is_running_) {
        return;
    }

    spdlog::info("Stopping MCP Server");

    if (stdio_transport_) {
        stdio_transport_->stop();
    }
    if (sse_transport_) {
        sse_transport_->stop();
    }
    if (ws_transport_) {
        ws_transport_->stop();
    }

    is_running_ = false;
    spdlog::info("MCP Server stopped");
}

void McpServer::attachHttpServer(std::shared_ptr<HttpServer> http_server) {
    http_server_ = http_server;
    spdlog::info("MCP Server attached to HTTP server for SSE/WebSocket endpoints");
}

// ============================================================================
// Tool Registration
// ============================================================================

void McpServer::registerTool(const std::string& name, const std::string& description,
                              const json& input_schema, ToolHandler handler) {
    tools_[name] = {description, input_schema, std::move(handler)};
    spdlog::debug("Registered MCP tool: {}", name);
}

void McpServer::unregisterTool(const std::string& name) {
    tools_.erase(name);
    spdlog::debug("Unregistered MCP tool: {}", name);
}

// ============================================================================
// Resource Registration
// ============================================================================

void McpServer::registerResource(const std::string& uri, const std::string& description,
                                  const std::string& mime_type, ResourceHandler handler) {
    resources_[uri] = {description, mime_type, std::move(handler)};
    spdlog::debug("Registered MCP resource: {}", uri);
}

void McpServer::unregisterResource(const std::string& uri) {
    resources_.erase(uri);
    spdlog::debug("Unregistered MCP resource: {}", uri);
}

// ============================================================================
// Prompt Registration
// ============================================================================

void McpServer::registerPrompt(const std::string& name, const std::string& description,
                                const json& arguments_schema, PromptHandler handler) {
    prompts_[name] = {description, arguments_schema, std::move(handler)};
    spdlog::debug("Registered MCP prompt: {}", name);
}

void McpServer::unregisterPrompt(const std::string& name) {
    prompts_.erase(name);
    spdlog::debug("Unregistered MCP prompt: {}", name);
}

// ============================================================================
// Request Handling
// ============================================================================

json McpServer::handleRequest(const json& request) {
    try {
        // Validate JSON-RPC 2.0 request
        if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
            return createError(-32600, "Invalid Request: missing or invalid 'jsonrpc' field");
        }

        if (!request.contains("method")) {
            return createError(-32600, "Invalid Request: missing 'method' field");
        }

        std::string method = request["method"];
        json params = request.contains("params") ? request["params"] : json::object();

        // Route to appropriate handler
        if (method == "initialize") {
            return handleInitialize(params);
        } else if (method == "tools/list") {
            return handleToolsList(params);
        } else if (method == "tools/call") {
            return handleToolsCall(params);
        } else if (method == "resources/list") {
            return handleResourcesList(params);
        } else if (method == "resources/read") {
            return handleResourcesRead(params);
        } else if (method == "prompts/list") {
            return handlePromptsList(params);
        } else if (method == "prompts/get") {
            return handlePromptsGet(params);
        } else {
            return createError(-32601, "Method not found: " + method);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error handling MCP request: {}", e.what());
        return createError(-32603, std::string("Internal error: ") + e.what());
    }
}

json McpServer::handleInitialize(const json& params) {
    initialized_ = true;
    
    if (params.contains("clientInfo")) {
        client_info_ = params["clientInfo"].dump();
    }

    json capabilities = {
        {"tools", {{"listChanged", false}}},
        {"resources", {{"subscribe", false}, {"listChanged", false}}},
        {"prompts", {{"listChanged", false}}}
    };

    json result = {
        {"protocolVersion", "2024-11-05"},
        {"capabilities", capabilities},
        {"serverInfo", {
            {"name", config_.server_name},
            {"version", config_.server_version}
        }}
    };

    spdlog::info("MCP initialized for client: {}", client_info_);
    return createSuccessResponse(result);
}

json McpServer::handleToolsList(const json& params) {
    json tools_list = json::array();
    
    for (const auto& [name, info] : tools_) {
        tools_list.push_back({
            {"name", name},
            {"description", info.description},
            {"inputSchema", info.input_schema}
        });
    }

    return createSuccessResponse({{"tools", tools_list}});
}

json McpServer::handleToolsCall(const json& params) {
    if (!params.contains("name")) {
        return createError(-32602, "Invalid params: missing 'name'");
    }

    std::string name = params["name"];
    auto it = tools_.find(name);
    
    if (it == tools_.end()) {
        return createError(-32602, "Tool not found: " + name);
    }

    json args = params.contains("arguments") ? params["arguments"] : json::object();
    
    try {
        json result = it->second.handler(args);
        return createSuccessResponse({{"content", {{{"type", "text"}, {"text", result.dump()}}}}});
    } catch (const std::exception& e) {
        return createError(-32000, std::string("Tool execution failed: ") + e.what());
    }
}

json McpServer::handleResourcesList(const json& params) {
    json resources_list = json::array();
    
    for (const auto& [uri, info] : resources_) {
        resources_list.push_back({
            {"uri", uri},
            {"name", uri},
            {"description", info.description},
            {"mimeType", info.mime_type}
        });
    }

    return createSuccessResponse({{"resources", resources_list}});
}

json McpServer::handleResourcesRead(const json& params) {
    if (!params.contains("uri")) {
        return createError(-32602, "Invalid params: missing 'uri'");
    }

    std::string uri = params["uri"];
    auto it = resources_.find(uri);
    
    if (it == resources_.end()) {
        return createError(-32602, "Resource not found: " + uri);
    }

    try {
        json content = it->second.handler(uri);
        return createSuccessResponse({
            {"contents", {{
                {"uri", uri},
                {"mimeType", it->second.mime_type},
                {"text", content.dump()}
            }}}
        });
    } catch (const std::exception& e) {
        return createError(-32000, std::string("Resource read failed: ") + e.what());
    }
}

json McpServer::handlePromptsList(const json& params) {
    json prompts_list = json::array();
    
    for (const auto& [name, info] : prompts_) {
        prompts_list.push_back({
            {"name", name},
            {"description", info.description},
            {"arguments", info.arguments_schema}
        });
    }

    return createSuccessResponse({{"prompts", prompts_list}});
}

json McpServer::handlePromptsGet(const json& params) {
    if (!params.contains("name")) {
        return createError(-32602, "Invalid params: missing 'name'");
    }

    std::string name = params["name"];
    auto it = prompts_.find(name);
    
    if (it == prompts_.end()) {
        return createError(-32602, "Prompt not found: " + name);
    }

    json args = params.contains("arguments") ? params["arguments"] : json::object();
    
    try {
        json messages = it->second.handler(name, args);
        return createSuccessResponse({{"messages", messages}});
    } catch (const std::exception& e) {
        return createError(-32000, std::string("Prompt execution failed: ") + e.what());
    }
}

// ============================================================================
// Default Tool Handlers (Stubs)
// ============================================================================

void McpServer::registerDefaultTools() {
    // Query tool
    registerTool("query", "Execute Cypher or SQL query on ThemisDB",
        {
            {"type", "object"},
            {"properties", {
                {"query", {{"type", "string"}, {"description", "Query string"}}},
                {"language", {{"type", "string"}, {"enum", {"cypher", "sql"}}, {"default", "cypher"}}}
            }},
            {"required", {"query"}}
        },
        [this](const json& args) { return toolQuery(args); });

    // Entity operations
    registerTool("put_entity", "Create or update an entity",
        {
            {"type", "object"},
            {"properties", {
                {"key", {{"type", "string"}}},
                {"value", {{"type", "object"}}}
            }},
            {"required", {"key", "value"}}
        },
        [this](const json& args) { return toolPutEntity(args); });

    registerTool("get_entity", "Retrieve an entity by key",
        {
            {"type", "object"},
            {"properties", {
                {"key", {{"type", "string"}}}
            }},
            {"required", {"key"}}
        },
        [this](const json& args) { return toolGetEntity(args); });

    registerTool("delete_entity", "Delete an entity by key",
        {
            {"type", "object"},
            {"properties", {
                {"key", {{"type", "string"}}}
            }},
            {"required", {"key"}}
        },
        [this](const json& args) { return toolDeleteEntity(args); });

    // Schema and stats
    registerTool("get_schema", "Get database schema information",
        {{"type", "object"}, {"properties", {}}},
        [this](const json& args) { return toolGetSchema(args); });

    registerTool("get_stats", "Get database statistics",
        {{"type", "object"}, {"properties", {}}},
        [this](const json& args) { return toolGetStats(args); });
}

json McpServer::toolQuery(const json& args) {
    // Stub implementation - to be integrated with actual query engine
    std::string query = args["query"];
    std::string language = args.value("language", "cypher");
    
    spdlog::info("MCP Tool 'query' called: {} ({})", query, language);
    
    return {
        {"status", "success"},
        {"message", "Query execution stub - integration pending"},
        {"query", query},
        {"language", language},
        {"results", json::array()}
    };
}

json McpServer::toolPutEntity(const json& args) {
    std::string key = args["key"];
    json value = args["value"];
    
    spdlog::info("MCP Tool 'put_entity' called: key={}", key);
    
    return {
        {"status", "success"},
        {"message", "Entity operation stub - integration pending"},
        {"key", key}
    };
}

json McpServer::toolGetEntity(const json& args) {
    std::string key = args["key"];
    
    spdlog::info("MCP Tool 'get_entity' called: key={}", key);
    
    return {
        {"status", "success"},
        {"message", "Entity retrieval stub - integration pending"},
        {"key", key},
        {"value", nullptr}
    };
}

json McpServer::toolDeleteEntity(const json& args) {
    std::string key = args["key"];
    
    spdlog::info("MCP Tool 'delete_entity' called: key={}", key);
    
    return {
        {"status", "success"},
        {"message", "Entity deletion stub - integration pending"},
        {"key", key}
    };
}

json McpServer::toolCreateIndex(const json& args) {
    spdlog::info("MCP Tool 'create_index' called");
    
    return {
        {"status", "success"},
        {"message", "Index creation stub - integration pending"}
    };
}

json McpServer::toolGetSchema(const json& args) {
    spdlog::info("MCP Tool 'get_schema' called");
    
    return {
        {"nodes", json::array()},
        {"edges", json::array()},
        {"properties", json::object()}
    };
}

json McpServer::toolGetStats(const json& args) {
    spdlog::info("MCP Tool 'get_stats' called");
    
    return {
        {"node_count", 0},
        {"edge_count", 0},
        {"storage_size_bytes", 0}
    };
}

// ============================================================================
// Default Resource Handlers (Stubs)
// ============================================================================

void McpServer::registerDefaultResources() {
    registerResource("schema://database", "Database schema information",
        "application/json",
        [this](const std::string& uri) { return resourceSchema(uri); });

    registerResource("stats://database", "Database statistics",
        "application/json",
        [this](const std::string& uri) { return resourceStats(uri); });

    registerResource("metadata://database", "Database metadata",
        "application/json",
        [this](const std::string& uri) { return resourceMetadata(uri); });

    registerResource("examples://queries", "Example query patterns",
        "application/json",
        [this](const std::string& uri) { return resourceExamples(uri); });
}

json McpServer::resourceSchema(const std::string& uri) {
    return {
        {"nodes", json::array()},
        {"edges", json::array()},
        {"message", "Schema resource stub - integration pending"}
    };
}

json McpServer::resourceStats(const std::string& uri) {
    return {
        {"node_count", 0},
        {"edge_count", 0},
        {"message", "Stats resource stub - integration pending"}
    };
}

json McpServer::resourceMetadata(const std::string& uri) {
    return {
        {"version", config_.server_version},
        {"name", config_.server_name},
        {"message", "Metadata resource stub - integration pending"}
    };
}

json McpServer::resourceExamples(const std::string& uri) {
    return {
        {"examples", {
            {"simple_match", "MATCH (n:User) RETURN n LIMIT 10"},
            {"with_filter", "MATCH (n:User) WHERE n.age > 25 RETURN n"}
        }}
    };
}

// ============================================================================
// Default Prompt Handlers (Stubs)
// ============================================================================

void McpServer::registerDefaultPrompts() {
    registerPrompt("simple_query", "Generate a simple Cypher query",
        {
            {"type", "object"},
            {"properties", {
                {"node_type", {{"type", "string"}}},
                {"limit", {{"type", "integer"}, {"default", 10}}}
            }}
        },
        [this](const std::string& name, const json& args) { return promptSimpleQuery(name, args); });

    registerPrompt("complex_query", "Generate a complex Cypher query with filters",
        {
            {"type", "object"},
            {"properties", {
                {"node_type", {{"type", "string"}}},
                {"filters", {{"type", "array"}}}
            }}
        },
        [this](const std::string& name, const json& args) { return promptComplexQuery(name, args); });
}

json McpServer::promptSimpleQuery(const std::string& name, const json& args) {
    std::string node_type = args.value("node_type", "User");
    int limit = args.value("limit", 10);
    
    return json::array({
        {
            {"role", "user"},
            {"content", {
                {"type", "text"},
                {"text", "Generate a simple Cypher query for " + node_type + " nodes (limit: " + std::to_string(limit) + ")"}
            }}
        }
    });
}

json McpServer::promptComplexQuery(const std::string& name, const json& args) {
    return json::array({
        {
            {"role", "user"},
            {"content", {
                {"type", "text"},
                {"text", "Generate a complex Cypher query with filters"}
            }}
        }
    });
}

json McpServer::promptEntityOperation(const std::string& name, const json& args) {
    return json::array({
        {
            {"role", "user"},
            {"content", {
                {"type", "text"},
                {"text", "Perform an entity operation"}
            }}
        }
    });
}

// ============================================================================
// Utility Methods
// ============================================================================

json McpServer::createError(int code, const std::string& message) {
    return {
        {"jsonrpc", "2.0"},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };
}

json McpServer::createSuccessResponse(const json& result) {
    return {
        {"jsonrpc", "2.0"},
        {"result", result}
    };
}

// ============================================================================
// StdioTransport Implementation
// ============================================================================

StdioTransport::StdioTransport(asio::io_context& io_context, int buffer_size)
    : io_context_(io_context), buffer_size_(buffer_size), read_buffer_(buffer_size) {
}

StdioTransport::~StdioTransport() {
    stop();
}

void StdioTransport::start() {
    if (is_running_) return;
    is_running_ = true;
    spdlog::info("MCP stdio transport started");
    // Note: Actual stdin reading would be implemented with asio::posix::stream_descriptor
    // This is a stub for the base implementation
}

void StdioTransport::stop() {
    if (!is_running_) return;
    is_running_ = false;
    spdlog::info("MCP stdio transport stopped");
}

void StdioTransport::send(const json& message) {
    if (!is_running_) return;
    writeStdout(message.dump() + "\n");
}

void StdioTransport::readStdin() {
    // Stub: Would read from stdin and call message_handler_
}

void StdioTransport::writeStdout(const std::string& data) {
    // Stub: Would write to stdout
    std::cout << data << std::flush;
}

// ============================================================================
// SseTransport Implementation
// ============================================================================

SseTransport::SseTransport(asio::io_context& io_context, int keepalive_ms)
    : io_context_(io_context), keepalive_ms_(keepalive_ms),
      keepalive_timer_(io_context) {
}

SseTransport::~SseTransport() {
    stop();
}

void SseTransport::start() {
    if (is_running_) return;
    is_running_ = true;
    spdlog::info("MCP SSE transport started");
    // Stub: Would start keepalive timer
}

void SseTransport::stop() {
    if (!is_running_) return;
    is_running_ = false;
    keepalive_timer_.cancel();
    spdlog::info("MCP SSE transport stopped");
}

void SseTransport::send(const json& message) {
    if (!is_running_) return;
    // Stub: Would send SSE event to all connected clients
    std::string event_data = "data: " + message.dump() + "\n\n";
    // Store in clients_ map for actual HTTP response
}

void SseTransport::addClient(const std::string& client_id) {
    clients_[client_id] = "";
    spdlog::debug("MCP SSE client added: {}", client_id);
}

void SseTransport::removeClient(const std::string& client_id) {
    clients_.erase(client_id);
    spdlog::debug("MCP SSE client removed: {}", client_id);
}

void SseTransport::sendKeepalive() {
    // Stub: Would send keepalive to all clients
}

// ============================================================================
// WebSocketTransport Implementation
// ============================================================================

WebSocketTransport::WebSocketTransport(asio::io_context& io_context, int ping_interval_ms)
    : io_context_(io_context), ping_interval_ms_(ping_interval_ms),
      ping_timer_(io_context) {
}

WebSocketTransport::~WebSocketTransport() {
    stop();
}

void WebSocketTransport::start() {
    if (is_running_) return;
    is_running_ = true;
    spdlog::info("MCP WebSocket transport started");
    // Stub: Would start ping timer
}

void WebSocketTransport::stop() {
    if (!is_running_) return;
    is_running_ = false;
    ping_timer_.cancel();
    spdlog::info("MCP WebSocket transport stopped");
}

void WebSocketTransport::send(const json& message) {
    if (!is_running_) return;
    // Stub: Would send to all connected WebSocket sessions
    std::string msg_str = message.dump();
    for (const auto& [session_id, is_active] : sessions_) {
        if (is_active) {
            // Would send via WebSocket session
            spdlog::debug("MCP WebSocket sending to session: {}", session_id);
        }
    }
}

void WebSocketTransport::addSession(const std::string& session_id) {
    sessions_[session_id] = true;
    spdlog::debug("MCP WebSocket session added: {}", session_id);
}

void WebSocketTransport::removeSession(const std::string& session_id) {
    sessions_.erase(session_id);
    spdlog::debug("MCP WebSocket session removed: {}", session_id);
}

void WebSocketTransport::handleMessage(const std::string& session_id, const std::string& message) {
    if (!is_running_) return;
    
    try {
        json request = json::parse(message);
        if (message_handler_) {
            json response = message_handler_(request);
            send(response);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error handling WebSocket message: {}", e.what());
    }
}

void WebSocketTransport::sendPing() {
    // Stub: Would send ping to all sessions
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MCP
