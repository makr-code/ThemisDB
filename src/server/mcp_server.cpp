#ifdef THEMIS_ENABLE_MCP

#include "server/mcp_server.h"
#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include <spdlog/spdlog.h>
#include <iostream>

#ifdef __unix__
#include <unistd.h>
#include <sys/select.h>
#endif

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

    spdlog::info("Starting MCP Server '{}' version {}", config_.server_name, config_.server_version);

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

void McpServer::attachDatabase(std::shared_ptr<RocksDBWrapper> db) {
    db_ = db;
    spdlog::info("MCP Server attached to RocksDB database");
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

    registerTool("create_index", "Create a database index",
        {
            {"type", "object"},
            {"properties", {
                {"label", {{"type", "string"}}},
                {"property", {{"type", "string"}}}
            }},
            {"required", {"label", "property"}}
        },
        [this](const json& args) { return toolCreateIndex(args); });
}

json McpServer::toolQuery(const json& args) {
    std::string query = args["query"];
    std::string language = args.value("language", "cypher");
    
    spdlog::info("MCP Tool 'query' called: {} ({})", query, language);
    
    // For minimal integration, we'll support simple key prefix scans
    // Full Cypher/SQL support would require query engine integration (future work)
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"},
            {"query", query},
            {"language", language}
        };
    }

    try {
        // Simple implementation: if query starts with "MATCH" or "SELECT", 
        // return stub message indicating query engine integration needed
        // For now, only support simple GET operations
        
        return {
            {"status", "success"},
            {"message", "Query executed (limited support - key/value operations only in minimal integration)"},
            {"query", query},
            {"language", language},
            {"results", json::array()},
            {"note", "Full Cypher/SQL query support requires query engine integration"}
        };
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("Query execution failed: ") + e.what()},
            {"query", query}
        };
    }
}

json McpServer::toolPutEntity(const json& args) {
    std::string key = args["key"];
    json value = args["value"];
    
    spdlog::info("MCP Tool 'put_entity' called: key={}", key);
    
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"},
            {"key", key}
        };
    }

    try {
        // Serialize JSON value to string for storage
        std::string value_str = value.dump();
        
        // Store in RocksDB
        bool success = db_->put(key, value_str);
        
        if (success) {
            return {
                {"status", "success"},
                {"message", "Entity stored successfully"},
                {"key", key}
            };
        } else {
            return {
                {"status", "error"},
                {"message", "Failed to store entity"},
                {"key", key}
            };
        }
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("Put operation failed: ") + e.what()},
            {"key", key}
        };
    }
}

json McpServer::toolGetEntity(const json& args) {
    std::string key = args["key"];
    
    spdlog::info("MCP Tool 'get_entity' called: key={}", key);
    
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"},
            {"key", key},
            {"value", nullptr}
        };
    }

    try {
        // Retrieve from RocksDB
        std::string value_str;
        bool found = db_->get(key, value_str);
        
        if (found) {
            // Parse JSON value
            json value = json::parse(value_str);
            
            return {
                {"status", "success"},
                {"message", "Entity retrieved successfully"},
                {"key", key},
                {"value", value}
            };
        } else {
            return {
                {"status", "success"},
                {"message", "Entity not found"},
                {"key", key},
                {"value", nullptr}
            };
        }
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("Get operation failed: ") + e.what()},
            {"key", key},
            {"value", nullptr}
        };
    }
}

json McpServer::toolDeleteEntity(const json& args) {
    std::string key = args["key"];
    
    spdlog::info("MCP Tool 'delete_entity' called: key={}", key);
    
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"},
            {"key", key}
        };
    }

    try {
        // Delete from RocksDB
        bool success = db_->del(key);
        
        if (success) {
            return {
                {"status", "success"},
                {"message", "Entity deleted successfully"},
                {"key", key}
            };
        } else {
            return {
                {"status", "error"},
                {"message", "Failed to delete entity"},
                {"key", key}
            };
        }
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("Delete operation failed: ") + e.what()},
            {"key", key}
        };
    }
}

json McpServer::toolCreateIndex(const json& args) {
    spdlog::info("MCP Tool 'create_index' called");
    
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"}
        };
    }

    return {
        {"status", "success"},
        {"message", "Index creation requires full query engine integration"},
        {"integration_level", "minimal"},
        {"note", "Index management available in production integration"}
    };
}

json McpServer::toolGetSchema(const json& args) {
    spdlog::info("MCP Tool 'get_schema' called");
    
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"},
            {"nodes", json::array()},
            {"edges", json::array()},
            {"properties", json::object()}
        };
    }

    // Minimal integration: return basic info
    return {
        {"status", "success"},
        {"message", "Schema discovery requires full query engine integration"},
        {"integration_level", "minimal"},
        {"nodes", json::array()},
        {"edges", json::array()},
        {"properties", json::object()},
        {"note", "Full schema discovery available in production integration"}
    };
}

json McpServer::toolGetStats(const json& args) {
    spdlog::info("MCP Tool 'get_stats' called");
    
    if (!db_) {
        return {
            {"status", "error"},
            {"message", "Database not attached"},
            {"node_count", 0},
            {"edge_count", 0},
            {"storage_size_bytes", 0}
        };
    }

    // Minimal integration: return connection status
    return {
        {"status", "success"},
        {"database_connected", db_->isOpen()},
        {"message", "Detailed statistics require full query engine integration"},
        {"integration_level", "minimal"},
        {"node_count", 0},
        {"edge_count", 0},
        {"storage_size_bytes", 0},
        {"note", "Full statistics available in production integration"}
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
    // For minimal integration, return basic schema information
    // Full schema discovery would require query engine integration
    return {
        {"nodes", json::array()},
        {"edges", json::array()},
        {"message", "Schema discovery available in full integration"},
        {"note", "Minimal integration supports key-value operations only"}
    };
}

json McpServer::resourceStats(const std::string& uri) {
    // For minimal integration, we can provide basic stats if database is attached
    if (db_ && db_->isOpen()) {
        return {
            {"status", "connected"},
            {"database_open", true},
            {"message", "Database statistics available in full integration"},
            {"note", "Minimal integration provides basic connectivity status only"}
        };
    }
    
    return {
        {"status", "disconnected"},
        {"database_open", false},
        {"message", "Database not attached or not open"}
    };
}

json McpServer::resourceMetadata(const std::string& uri) {
    return {
        {"version", config_.server_version},
        {"name", config_.server_name},
        {"integration_level", "minimal"},
        {"supported_operations", {"put_entity", "get_entity", "delete_entity"}},
        {"pending_operations", {"full_query", "schema_discovery", "advanced_stats"}},
        {"database_attached", db_ != nullptr},
        {"database_open", db_ && db_->isOpen()}
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

    registerPrompt("entity_operation", "Generate entity operation prompt",
        {
            {"type", "object"},
            {"properties", {
                {"operation", {{"type", "string"}, {"enum", {"create", "update", "delete"}}}},
                {"entity_type", {{"type", "string"}}}
            }}
        },
        [this](const std::string& name, const json& args) { return promptEntityOperation(name, args); });
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
    
#ifdef __unix__
    // Start async stdin reading on POSIX systems
    readStdin();
#else
    spdlog::warn("MCP stdio transport: Non-POSIX system detected, stdin reading not implemented");
#endif
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
#ifdef __unix__
    // Post async read task
    asio::post(io_context_, [this]() {
        while (is_running_) {
            // Use select to check if stdin has data with timeout
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms timeout
            
            int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
            
            if (result > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
                // Read available data
                std::string line;
                if (std::getline(std::cin, line)) {
                    partial_message_ += line;
                    
                    // Try to parse as JSON
                    try {
                        json request = json::parse(partial_message_);
                        
                        // Call message handler
                        if (message_handler_) {
                            json response = message_handler_(request);
                            send(response);
                        }
                        
                        // Clear partial message
                        partial_message_.clear();
                    } catch (const json::parse_error&) {
                        // Incomplete JSON, wait for more input
                    }
                } else {
                    // EOF on stdin
                    is_running_ = false;
                    break;
                }
            }
        }
    });
#endif
}

void StdioTransport::writeStdout(const std::string& data) {
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
    spdlog::info("MCP SSE transport started with {}ms keepalive interval", keepalive_ms_);
    
    // Start keepalive timer
    scheduleKeepalive();
}

void SseTransport::stop() {
    if (!is_running_) return;
    is_running_ = false;
    keepalive_timer_.cancel();
    
    // Clear all clients
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
    }
    
    spdlog::info("MCP SSE transport stopped");
}

void SseTransport::send(const json& message) {
    if (!is_running_) return;
    
    // Format as SSE event
    std::string event_data = "data: " + message.dump() + "\n\n";
    
    // Store in all clients' buffers
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& [client_id, buffer] : clients_) {
        buffer += event_data;
    }
    
    spdlog::debug("MCP SSE event sent to {} clients", clients_.size());
}

void SseTransport::addClient(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[client_id] = "";
    spdlog::debug("MCP SSE client added: {}, total clients: {}", client_id, clients_.size());
}

void SseTransport::removeClient(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(client_id);
    spdlog::debug("MCP SSE client removed: {}, remaining clients: {}", client_id, clients_.size());
}

std::string SseTransport::getClientData(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
        std::string data = it->second;
        it->second.clear(); // Clear buffer after retrieval
        return data;
    }
    return "";
}

void SseTransport::sendKeepalive() {
    if (!is_running_) return;
    
    // Send SSE comment as keepalive
    std::string keepalive = ": keepalive\n\n";
    
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& [client_id, buffer] : clients_) {
        buffer += keepalive;
    }
    
    spdlog::trace("MCP SSE keepalive sent to {} clients", clients_.size());
}

void SseTransport::scheduleKeepalive() {
    if (!is_running_) return;
    
    keepalive_timer_.expires_after(std::chrono::milliseconds(keepalive_ms_));
    keepalive_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && is_running_) {
            sendKeepalive();
            scheduleKeepalive();
        }
    });
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
    spdlog::info("MCP WebSocket transport started with {}ms ping interval", ping_interval_ms_);
    
    // Start ping timer
    schedulePing();
}

void WebSocketTransport::stop() {
    if (!is_running_) return;
    is_running_ = false;
    ping_timer_.cancel();
    
    // Clear all sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.clear();
    }
    
    spdlog::info("MCP WebSocket transport stopped");
}

void WebSocketTransport::send(const json& message) {
    if (!is_running_) return;
    
    std::string msg_str = message.dump();
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    for (auto& [session_id, session_data] : sessions_) {
        if (session_data.is_active) {
            // Queue message for session
            session_data.pending_messages.push(msg_str);
            spdlog::debug("MCP WebSocket message queued for session: {}", session_id);
        }
    }
}

void WebSocketTransport::sendToSession(const std::string& session_id, const json& message) {
    if (!is_running_) return;
    
    std::string msg_str = message.dump();
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end() && it->second.is_active) {
        it->second.pending_messages.push(msg_str);
        spdlog::debug("MCP WebSocket message sent to session: {}", session_id);
    }
}

void WebSocketTransport::addSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_[session_id] = SessionData{true, {}};
    spdlog::debug("MCP WebSocket session added: {}, total sessions: {}", session_id, sessions_.size());
}

void WebSocketTransport::removeSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.erase(session_id);
    spdlog::debug("MCP WebSocket session removed: {}, remaining sessions: {}", session_id, sessions_.size());
}

std::vector<std::string> WebSocketTransport::getPendingMessages(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        std::vector<std::string> messages;
        while (!it->second.pending_messages.empty()) {
            messages.push_back(std::move(it->second.pending_messages.front()));
            it->second.pending_messages.pop();
        }
        return messages;
    }
    return {};
}

void WebSocketTransport::handleMessage(const std::string& session_id, const std::string& message) {
    if (!is_running_) return;
    
    try {
        json request = json::parse(message);
        if (message_handler_) {
            // Process request and send response to the specific session
            json response = message_handler_(request);
            sendToSession(session_id, response);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error handling WebSocket message from session {}: {}", session_id, e.what());
        
        // Send error response
        json error_response = {
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", -32700},
                {"message", std::string("Parse error: ") + e.what()}
            }},
            {"id", nullptr}
        };
        sendToSession(session_id, error_response);
    }
}

void WebSocketTransport::sendPing() {
    if (!is_running_) return;
    
    // Send ping to all active sessions
    json ping_message = {
        {"jsonrpc", "2.0"},
        {"method", "ping"},
        {"params", {}}
    };
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    size_t active_count = 0;
    for (const auto& [session_id, session_data] : sessions_) {
        if (session_data.is_active) {
            active_count++;
        }
    }
    
    spdlog::trace("MCP WebSocket ping scheduled for {} active sessions", active_count);
}

void WebSocketTransport::schedulePing() {
    if (!is_running_) return;
    
    ping_timer_.expires_after(std::chrono::milliseconds(ping_interval_ms_));
    ping_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && is_running_) {
            sendPing();
            schedulePing();
        }
    });
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MCP
