/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mcp_server.cpp                                     ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:35:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   71.0/100                                       ║
    • Total Lines:     2368                                           ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e2cf1a07ca  2026-02-22  feat: MCP ↔ AIOrchestrator bidirectional integration (MCP... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifdef THEMIS_ENABLE_MCP

#include "server/mcp_server.h"
#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "metadata/schema_manager.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "query/aql_runner.h"
#include "index/graph_index.h"
#include "llm/embedded_llm.h"
#include "prompt_engineering/prompt_manager.h"
#include "utils/error_registry.h"
#include "utils/string_utils.h"
#include "config/config_path_resolver.h"
#include "version.h"
#ifdef THEMIS_ENABLE_LLM
#include "llm/ai_orchestrator.h"
#endif
#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <fmt/format.h>
#include <regex>

#if defined(_WIN32)
#include <windows.h>
#include <conio.h>  // For _kbhit() if needed
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/select.h>
#endif

namespace themis {
namespace server {

// ============================================================================
// McpServer Implementation
// ============================================================================

McpServer::McpServer(asio::io_context& io_context)
    : McpServer(io_context, Config{}) {
}

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
    
    // Initialize SchemaManager with the database
    if (db && db->isOpen()) {
        // Create SecondaryIndexManager for index metadata
        index_mgr_ = std::make_shared<SecondaryIndexManager>(*db);
        
        // Create SchemaManager with database and index manager
        schema_mgr_ = std::make_unique<SchemaManager>(*db, index_mgr_.get());
        
        // Create GraphIndexManager for graph queries
        auto graph_mgr = std::make_shared<GraphIndexManager>(*db);
        
        // Create QueryEngine for AQL execution
        // Note: VectorIndexManager and SpatialIndexManager are optional
        query_engine_ = std::make_unique<QueryEngine>(*db, *index_mgr_, *graph_mgr);
        
        // Initialize PromptManager and load system prompts
        prompt_mgr_ = std::make_unique<PromptManager>();
        
        // Try to load prompts from YAML file
        std::string prompt_file = themis::config::ConfigPathResolver::mapLegacyToNew("config/llm_system_prompts.yaml");
        // Fall back to legacy path if new one doesn't exist
        if (!std::filesystem::exists(prompt_file)) {
            prompt_file = "config/llm_system_prompts.yaml";
        }
        size_t loaded = prompt_mgr_->loadFromYAML(prompt_file);
        if (loaded > 0) {
            spdlog::info("MCP Server loaded {} system prompts from {}", loaded, prompt_file);
        } else {
            spdlog::warn("MCP Server could not load system prompts from {}", prompt_file);
        }
        
        spdlog::info("MCP Server attached to RocksDB database with SchemaManager, QueryEngine, and PromptManager initialized");
    } else {
        spdlog::info("MCP Server attached to RocksDB database (not open yet)");
    }
}

// ============================================================================
// AI Orchestrator Integration
// ============================================================================

#ifdef THEMIS_ENABLE_LLM
void McpServer::attachOrchestrator(std::shared_ptr<themis::llm::AIOrchestrator> orchestrator) {
    orchestrator_ = std::move(orchestrator);
    if (!orchestrator_) {
        spdlog::warn("MCP Server: attachOrchestrator called with null orchestrator");
        return;
    }

    // Register llm_orchestrate: run a named pipeline mode via the orchestrator
    registerTool("llm_orchestrate",
        "Execute a named LLM pipeline mode (ask, edit, rag, agentic, ethics, …) "
        "using the ThemisDB AI Orchestrator. Supports YAML-configured retrieval, "
        "tool use, budgets and observability.",
        {
            {"type", "object"},
            {"properties", {
                {"query",   {{"type", "string"}, {"description", "The user query or instruction"}}},
                {"mode",    {{"type", "string"}, {"description",
                              "Mode id (ask, edit, rag, agentic, ethics, multi_agent, …). "
                              "Omit to use the pack's default mode."}}},
                {"request_id", {{"type", "string"}, {"description", "Optional request id for tracing"}}},
                {"max_tokens", {{"type", "integer"}, {"description", "Override max tokens for this request"}}},
                {"temperature", {{"type", "number"}, {"description", "Override temperature for this request"}}}
            }},
            {"required", {"query"}}
        },
        [this](const json& args) { return toolLLMOrchestrate(args); });

    // Register llm_list_modes: enumerate available orchestration modes
    registerTool("llm_list_modes",
        "List all LLM orchestration modes available in the loaded ModePack, "
        "including their descriptions, retrieval settings, tool permissions and budgets.",
        {
            {"type", "object"},
            {"properties", {}}
        },
        [this](const json& args) { return toolLLMListModes(args); });

    const auto& pack = orchestrator_->modePack();
    spdlog::info("MCP Server: AIOrchestrator attached (pack='{}' v{}, {} mode(s), default='{}')",
                 pack.name, pack.version, pack.modes.size(), pack.default_mode);
}
#endif

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
        errors::logError(ErrorCode::ERR_MCP_TRANSPORT_FAILED, e.what());
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
    registerTool("query", "Execute AQL, Cypher, or SQL query on ThemisDB",
        {
            {"type", "object"},
            {"properties", {
                {"query", {{"type", "string"}, {"description", "Query string"}}},
                {"language", {{"type", "string"}, {"enum", {"aql", "cypher", "sql", "auto"}}, {"default", "aql"}, {"description", "Query language (aql=full support, others pending)"}}}
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
                {"table", {{"type", "string"}, {"description", "Table/collection name"}}},
                {"column", {{"type", "string"}, {"description", "Column/property name"}}},
                {"type", {{"type", "string"}, {"enum", {"regular", "range", "sparse", "geo", "fulltext", "ttl"}}, {"default", "regular"}, {"description", "Index type"}}},
                {"unique", {{"type", "boolean"}, {"default", false}, {"description", "Unique constraint (for regular/sparse indexes)"}}},
                {"ttl_seconds", {{"type", "integer"}, {"description", "TTL in seconds (for ttl index type)"}}},
                {"fulltext_config", {{"type", "object"}, {"description", "Fulltext configuration (for fulltext index type)"}}}
            }},
            {"required", {"table", "column"}}
        },
        [this](const json& args) { return toolCreateIndex(args); });

    registerTool("drop_index", "Drop a database index",
        {
            {"type", "object"},
            {"properties", {
                {"table", {{"type", "string"}, {"description", "Table/collection name"}}},
                {"column", {{"type", "string"}, {"description", "Column/property name"}}},
                {"type", {{"type", "string"}, {"enum", {"regular", "range", "sparse", "geo", "fulltext", "ttl"}}, {"default", "regular"}, {"description", "Index type"}}}
            }},
            {"required", {"table", "column"}}
        },
        [this](const json& args) { return toolDropIndex(args); });

    registerTool("list_indexes", "List all database indexes",
        {
            {"type", "object"},
            {"properties", {}}
        },
        [this](const json& args) { return toolListIndexes(args); });

    // ========================================================================
    // LLM Tools (NEW)
    // ========================================================================
    
    #ifdef THEMIS_ENABLE_LLM
    registerTool("llm_complete", "Generate text completion using LLM",
        {
            {"type", "object"},
            {"properties", {
                {"prompt", {{"type", "string"}, {"description", "Text prompt for generation"}}},
                {"max_tokens", {{"type", "integer"}, {"description", "Maximum tokens to generate"}, {"default", 512}}},
                {"temperature", {{"type", "number"}, {"description", "Sampling temperature (0.0-2.0)"}, {"default", 0.7}}}
            }},
            {"required", {"prompt"}}
        },
        [this](const json& args) { return toolLLMComplete(args); });

    registerTool("llm_embed", "Generate embeddings for text",
        {
            {"type", "object"},
            {"properties", {
                {"text", {{"type", "string"}, {"description", "Text to embed"}}}
            }},
            {"required", {"text"}}
        },
        [this](const json& args) { return toolLLMEmbed(args); });

    registerTool("llm_chat", "Multi-turn chat completion",
        {
            {"type", "object"},
            {"properties", {
                {"messages", {
                    {"type", "array"},
                    {"items", {
                        {"type", "object"},
                        {"properties", {
                            {"role", {{"type", "string"}, {"enum", {"system", "user", "assistant"}}}},
                            {"content", {{"type", "string"}}}
                        }},
                        {"required", {"role", "content"}}
                    }}
                }}
            }},
            {"required", {"messages"}}
        },
        [this](const json& args) { return toolLLMChat(args); });

    registerTool("database_query_with_llm", "Execute query and analyze with LLM",
        {
            {"type", "object"},
            {"properties", {
                {"query", {{"type", "string"}, {"description", "Database query"}}},
                {"analysis_prompt", {{"type", "string"}, {"description", "How to analyze results"}}}
            }},
            {"required", {"query", "analysis_prompt"}}
        },
        [this](const json& args) { return toolDatabaseQueryWithLLM(args); });
    #endif

    // ========================================================================
    // Error Introspection Tools (NEW)
    // ========================================================================
    
    registerTool("get_error_info", "Get detailed information about an error code or message",
        {
            {"type", "object"},
            {"properties", {
                {"query", {
                    {"type", "string"},
                    {"description", "Error code (e.g., '2000') or search query"}
                }}
            }},
            {"required", {"query"}}
        },
        [this](const json& args) { return toolGetErrorInfo(args); });

    registerTool("search_errors", "Search for errors by category, keyword, or description",
        {
            {"type", "object"},
            {"properties", {
                {"query", {
                    {"type", "string"},
                    {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
                }},
                {"category", {
                    {"type", "string"},
                    {"description", "Filter by category (optional)"}
                }}
            }},
            {"required", {"query"}}
        },
        [this](const json& args) { return toolSearchErrors(args); });

    registerTool("introspect_database", "Ask questions about the database capabilities, errors, and features",
        {
            {"type", "object"},
            {"properties", {
                {"question", {
                    {"type", "string"},
                    {"description", "Natural language question about the database"}
                }}
            }},
            {"required", {"question"}}
        },
        [this](const json& args) { return toolIntrospectDatabase(args); });
}

json McpServer::toolQuery(const json& args) {
    std::string query = args["query"];
    std::string language = args.value("language", "aql");
    
    spdlog::info("MCP Tool 'query' called: {} ({})", query, language);
    
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
            {"query", query},
            {"language", language}
        };
    }

    try {
        // Detect query language if set to "auto"
        if (language == "auto") {
            // Simple heuristic: if query contains "FOR", "FILTER", "RETURN" -> AQL
            // if query contains "SELECT", "FROM" -> SQL
            // if query contains "MATCH", "WHERE" -> Cypher
            std::string upper_query = query;
            std::transform(upper_query.begin(), upper_query.end(), upper_query.begin(), ::toupper);
            
            if (upper_query.find("FOR ") != std::string::npos && 
                upper_query.find("RETURN") != std::string::npos) {
                language = "aql";
            } else if (upper_query.find("SELECT") != std::string::npos) {
                language = "sql";
            } else if (upper_query.find("MATCH") != std::string::npos) {
                language = "cypher";
            } else {
                language = "aql"; // default
            }
        }
        
        // Execute AQL queries using the query engine
        if (language == "aql") {
            if (!query_engine_) {
                return {
                    {"status", "error"},
                    {"message", "Query engine not initialized"},
                    {"query", query},
                    {"language", language}
                };
            }
            
            // Execute AQL query
            auto result = executeAql(query, *query_engine_);
            
            if (!result) {
                return {
                    {"status", "error"},
                    {"message", fmt::format("AQL execution failed: {}", result.error().message())},
                    {"query", query},
                    {"language", language}
                };
            }
            
            // Return successful result
            return {
                {"status", "success"},
                {"message", "AQL query executed successfully"},
                {"query", query},
                {"language", language},
                {"results", *result}
            };
        }
        // SQL and Cypher not yet implemented
        else if (language == "sql" || language == "cypher") {
            return {
                {"status", "error"},
                {"message", fmt::format("{} query language not yet implemented", language)},
                {"query", query},
                {"language", language},
                {"note", "Use 'aql' language for full query support"}
            };
        }
        // Unknown language
        else {
            return {
                {"status", "error"},
                {"message", fmt::format("Unsupported query language: {}", language)},
                {"query", query},
                {"language", language},
                {"supported_languages", json::array({"aql", "sql", "cypher"})}
            };
        }
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", fmt::format("Query execution failed: {}", e.what())},
            {"query", query},
            {"language", language}
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
    
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"}
        };
    }
    
    if (!index_mgr_) {
        return {
            {"status", "error"},
            {"message", "Index manager not initialized"}
        };
    }
    
    // Extract parameters
    std::string table = args.value("table", "");
    std::string column = args.value("column", "");
    std::string index_type = args.value("type", "regular");
    bool unique = args.value("unique", false);
    
    if (table.empty()) {
        return {
            {"status", "error"},
            {"message", "Missing required parameter: table"}
        };
    }
    
    if (column.empty()) {
        return {
            {"status", "error"},
            {"message", "Missing required parameter: column"}
        };
    }
    
    try {
        SecondaryIndexManager::Status status;
        
        // Convert index_type string to enum and create appropriate index
        if (index_type == "regular" || index_type == "secondary") {
            status = index_mgr_->createIndex(table, column, unique);
        } else if (index_type == "range") {
            status = index_mgr_->createRangeIndex(table, column);
        } else if (index_type == "sparse") {
            status = index_mgr_->createSparseIndex(table, column, unique);
        } else if (index_type == "geo" || index_type == "geospatial") {
            status = index_mgr_->createGeoIndex(table, column);
        } else if (index_type == "fulltext") {
            // Get optional fulltext configuration from args
            SecondaryIndexManager::FulltextConfig config;
            if (args.contains("fulltext_config")) {
                auto ft_config = args["fulltext_config"];
                config.stemming_enabled = ft_config.value("stemming", false);
                config.language = ft_config.value("language", "none");
                config.stopwords_enabled = ft_config.value("stopwords", false);
                config.normalize_umlauts = ft_config.value("normalize_umlauts", false);
            }
            status = index_mgr_->createFulltextIndex(table, column, config);
        } else if (index_type == "ttl") {
            int64_t ttl_seconds = args.value("ttl_seconds", 86400); // default 1 day
            status = index_mgr_->createTTLIndex(table, column, ttl_seconds);
        } else {
            return {
                {"status", "error"},
                {"message", fmt::format("Unsupported index type: {}. Supported types: regular, range, sparse, geo, fulltext, ttl", index_type)}
            };
        }
        
        if (status.ok) {
            return {
                {"status", "success"},
                {"message", fmt::format("Index created successfully on {}.{}", table, column)},
                {"table", table},
                {"column", column},
                {"index_type", index_type},
                {"unique", unique}
            };
        } else {
            return {
                {"status", "error"},
                {"message", status.message},
                {"table", table},
                {"column", column},
                {"index_type", index_type}
            };
        }
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", fmt::format("Index creation failed: {}", e.what())},
            {"table", table},
            {"column", column},
            {"index_type", index_type}
        };
    }
}

json McpServer::toolDropIndex(const json& args) {
    spdlog::info("MCP Tool 'drop_index' called");
    
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"}
        };
    }
    
    if (!index_mgr_) {
        return {
            {"status", "error"},
            {"message", "Index manager not initialized"}
        };
    }
    
    // Extract parameters
    std::string table = args.value("table", "");
    std::string column = args.value("column", "");
    std::string index_type = args.value("type", "regular");
    
    if (table.empty() || column.empty()) {
        return {
            {"status", "error"},
            {"message", "Missing required parameters: table and column"}
        };
    }
    
    try {
        SecondaryIndexManager::Status status;
        
        // Drop the appropriate index type
        if (index_type == "regular" || index_type == "secondary") {
            status = index_mgr_->dropIndex(table, column);
        } else if (index_type == "range") {
            status = index_mgr_->dropRangeIndex(table, column);
        } else if (index_type == "sparse") {
            status = index_mgr_->dropSparseIndex(table, column);
        } else if (index_type == "geo" || index_type == "geospatial") {
            status = index_mgr_->dropGeoIndex(table, column);
        } else if (index_type == "fulltext") {
            status = index_mgr_->dropFulltextIndex(table, column);
        } else if (index_type == "ttl") {
            status = index_mgr_->dropTTLIndex(table, column);
        } else {
            return {
                {"status", "error"},
                {"message", fmt::format("Unsupported index type: {}", index_type)}
            };
        }
        
        if (status.ok) {
            return {
                {"status", "success"},
                {"message", fmt::format("Index dropped successfully from {}.{}", table, column)},
                {"table", table},
                {"column", column},
                {"index_type", index_type}
            };
        } else {
            return {
                {"status", "error"},
                {"message", status.message},
                {"table", table},
                {"column", column}
            };
        }
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", fmt::format("Index drop failed: {}", e.what())},
            {"table", table},
            {"column", column}
        };
    }
}

json McpServer::toolListIndexes(const json& args) {
    spdlog::info("MCP Tool 'list_indexes' called");
    
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
            {"indexes", json::array()}
        };
    }
    
    if (!index_mgr_) {
        return {
            {"status", "error"},
            {"message", "Index manager not initialized"},
            {"indexes", json::array()}
        };
    }
    
    try {
        // Get all tables from schema manager
        json indexes = json::array();
        
        if (schema_mgr_) {
            auto tables = schema_mgr_->getAllTables();
            
            for (const auto& table : tables) {
                // Get index stats for each table
                auto stats = index_mgr_->getAllIndexStats(table.name);
                
                for (const auto& stat : stats) {
                    json index_info = {
                        {"table", stat.table},
                        {"column", stat.column},
                        {"type", stat.type},
                        {"unique", stat.unique},
                        {"estimated_size_bytes", stat.estimated_size_bytes},
                        {"entry_count", stat.entry_count},
                        {"additional_info", stat.additional_info}
                    };
                    indexes.push_back(index_info);
                }
            }
        }
        
        return {
            {"status", "success"},
            {"indexes", indexes},
            {"total_count", indexes.size()}
        };
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", fmt::format("Failed to list indexes: {}", e.what())},
            {"indexes", json::array()}
        };
    }
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

    if (!schema_mgr_) {
        return {
            {"status", "error"},
            {"message", "SchemaManager not initialized"},
            {"integration_level", "minimal"},
            {"nodes", json::array()},
            {"edges", json::array()},
            {"properties", json::object()}
        };
    }

    // Full integration: return real schema data from SchemaManager
    try {
        auto schema_json = schema_mgr_->toJSON();
        
        // Add integration level indicator
        schema_json["integration_level"] = "full";
        
        return schema_json;
    } catch (const std::exception& e) {
        spdlog::error("Error retrieving schema: {}", e.what());
        return {
            {"status", "error"},
            {"message", std::string("Failed to retrieve schema: ") + e.what()},
            {"integration_level", "full"},
            {"nodes", json::array()},
            {"edges", json::array()},
            {"properties", json::object()}
        };
    }
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

    // Full integration: return real statistics from SchemaManager
    if (schema_mgr_) {
        try {
            auto metadata = schema_mgr_->getDatabaseMetadata();
            return {
                {"status", "success"},
                {"database_connected", db_->isOpen()},
                {"integration_level", "full"},
                {"version", metadata.version},
                {"table_count", metadata.table_count},
                {"total_rows", metadata.total_rows},
                {"capabilities", metadata.capabilities}
            };
        } catch (const std::exception& e) {
            spdlog::error("Error retrieving stats: {}", e.what());
            return {
                {"status", "error"},
                {"message", std::string("Failed to retrieve stats: ") + e.what()},
                {"database_connected", db_->isOpen()},
                {"integration_level", "full"}
            };
        }
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
// LLM Tool Handlers (NEW)
// ============================================================================

#ifdef THEMIS_ENABLE_LLM
json McpServer::toolLLMComplete(const json& args) {
    spdlog::info("MCP Tool 'llm_complete' called");
    
    try {
        std::string prompt = args.at("prompt");
        int max_tokens = args.value("max_tokens", 512);
        float temperature = args.value("temperature", 0.7f);
        
        // Use EmbeddedLLM for generation
        std::string result = THEMIS_LLM_GENERATE(prompt);
        
        return {
            {"status", "success"},
            {"text", result},
            {"prompt_length", prompt.length()},
            {"model", "default"}
        };
        
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("LLM completion failed: ") + e.what()}
        };
    }
}

json McpServer::toolLLMEmbed(const json& args) {
    spdlog::info("MCP Tool 'llm_embed' called");
    
    try {
        std::string text = args.at("text");
        
        // Use EmbeddedLLM for embeddings
        auto embedding = THEMIS_LLM_EMBED(text);
        
        return {
            {"status", "success"},
            {"embedding", embedding},
            {"dimensions", embedding.size()},
            {"text_length", text.length()}
        };
        
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("LLM embedding failed: ") + e.what()}
        };
    }
}

json McpServer::toolLLMChat(const json& args) {
    spdlog::info("MCP Tool 'llm_chat' called");
    
    try {
        auto messages_json = args.at("messages");
        
        // Convert JSON messages to ChatMessage objects
        std::vector<llm::ChatMessage> messages;
        for (const auto& msg : messages_json) {
            messages.push_back({
                msg.at("role").get<std::string>(),
                msg.at("content").get<std::string>()
            });
        }
        
        // Use EmbeddedLLM for chat
        std::string response = THEMIS_LLM_CHAT(messages);
        
        return {
            {"status", "success"},
            {"response", response},
            {"message_count", messages.size()}
        };
        
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("LLM chat failed: ") + e.what()}
        };
    }
}

json McpServer::toolDatabaseQueryWithLLM(const json& args) {
    spdlog::info("MCP Tool 'database_query_with_llm' called");
    
    try {
        std::string query = args.at("query");
        std::string analysis_prompt = args.at("analysis_prompt");
        
        // Execute query (simplified - would use actual query engine)
        json query_results = toolQuery({{"query", query}, {"language", "cypher"}});
        
        // Analyze results with LLM
        std::string llm_prompt = analysis_prompt + "\n\nQuery Results:\n" + query_results.dump(2);
        std::string analysis = THEMIS_LLM_GENERATE(llm_prompt);
        
        return {
            {"status", "success"},
            {"query", query},
            {"results", query_results},
            {"analysis", analysis}
        };
        
    } catch (const std::exception& e) {
        return {
            {"status", "error"},
            {"message", std::string("Query with LLM failed: ") + e.what()}
        };
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AI Orchestrator Tool Handlers
// ─────────────────────────────────────────────────────────────────────────────

json McpServer::toolLLMOrchestrate(const json& args) {
    spdlog::info("MCP Tool 'llm_orchestrate' called");

    if (!orchestrator_) {
        return {{"status", "error"}, {"message", "AIOrchestrator not attached. Call attachOrchestrator() first."}};
    }

    try {
        themis::llm::OrchestratorContext ctx;
        ctx.query      = args.at("query").get<std::string>();
        ctx.mode_id    = args.value("mode", "");
        ctx.request_id = args.value("request_id", "");

        if (args.contains("max_tokens")) {
            ctx.max_tokens = args["max_tokens"].get<int>();
        }
        if (args.contains("temperature")) {
            ctx.temperature = args["temperature"].get<float>();
        }

        const auto result = orchestrator_->run(ctx);

        json out;
        out["status"]          = result.success ? "success" : "error";
        out["text"]            = result.text;
        out["mode_id"]         = result.metadata.mode_id;
        out["model_id"]        = result.metadata.model_id;
        out["tokens_prompt"]   = result.metadata.tokens_prompt;
        out["tokens_generated"]= result.metadata.tokens_generated;
        out["retrieved_docs"]  = result.metadata.retrieved_docs;
        out["latency_ms"]      = result.metadata.latency.total_ms;
        if (!result.metadata.tool_calls_made.empty()) {
            out["tool_calls_made"] = result.metadata.tool_calls_made;
        }
        if (!result.success) {
            out["error"] = result.error;
        }
        return out;

    } catch (const std::exception& e) {
        return {{"status", "error"}, {"message", std::string("llm_orchestrate failed: ") + e.what()}};
    }
}

json McpServer::toolLLMListModes(const json& /*args*/) {
    spdlog::info("MCP Tool 'llm_list_modes' called");

    if (!orchestrator_) {
        return {{"status", "error"}, {"message", "AIOrchestrator not attached."}};
    }

    const auto& pack = orchestrator_->modePack();

    json modes_arr = json::array();
    for (const auto& m : pack.modes) {
        json entry;
        entry["id"]          = m.id;
        entry["description"] = m.description;
        entry["model"]       = m.model_id;
        entry["retrieval_enabled"] = m.retrieval.enabled;
        entry["tools_allowed"]     = m.tools_allowed;
        entry["max_tokens"]        = m.budgets.max_tokens;
        entry["temperature"]       = m.budgets.temperature;
        modes_arr.push_back(std::move(entry));
    }

    return {
        {"status",       "success"},
        {"pack_name",    pack.name},
        {"pack_version", pack.version},
        {"default_mode", pack.default_mode},
        {"modes",        std::move(modes_arr)}
    };
}

#endif // THEMIS_ENABLE_LLM

// ============================================================================
// Error Introspection Tool Handlers (NEW)
// ============================================================================

json McpServer::toolGetErrorInfo(const json& args) {
    spdlog::info("MCP Tool 'get_error_info' called");
    
    std::string query = args.value("query", "");
    
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // Try to parse as error code
    try {
        int code = std::stoi(query);
        auto metadata = registry.getError(static_cast<errors::ErrorCode>(code));
        
        return {
            {"status", "success"},
            {"error", metadata.toJSON()}
        };
    } catch (...) {
        // Search by query
        auto results = registry.searchErrors(query);
        
        if (results.empty()) {
            return {
                {"status", "not_found"},
                {"message", "No errors found matching query"}
            };
        }
        
        json errors_json = json::array();
        for (const auto& error : results) {
            errors_json.push_back(error.toJSON());
        }
        
        return {
            {"status", "success"},
            {"errors", errors_json},
            {"count", results.size()}
        };
    }
}

json McpServer::toolSearchErrors(const json& args) {
    spdlog::info("MCP Tool 'search_errors' called");
    
    std::string query = args.value("query", "");
    std::string category = args.value("category", "");
    
    auto& registry = errors::ErrorRegistry::getInstance();
    std::vector<errors::ErrorMetadata> results;
    
    if (!category.empty()) {
        results = registry.getErrorsByCategory(category);
    } else {
        results = registry.searchErrors(query);
    }
    
    json errors_json = json::array();
    for (const auto& error : results) {
        errors_json.push_back(error.toJSON());
    }
    
    return {
        {"status", "success"},
        {"errors", errors_json},
        {"count", results.size()}
    };
}

json McpServer::toolIntrospectDatabase(const json& args) {
    std::string question = args.value("question", "");
    
    spdlog::info("MCP Tool 'introspect_database' called: {}", question);
    
    if (question.empty()) {
        return {{"status", "error"}, {"message", "No question provided"}};
    }
    
    // If PromptManager or SchemaManager not available, fall back to basic response
    if (!prompt_mgr_ || !schema_mgr_) {
        spdlog::warn("PromptManager or SchemaManager not initialized, using fallback response");
        return {
            {"status", "success"},
            {"question", question},
            {"answer", "Schema introspection not available. Database may not be fully initialized."}
        };
    }
    
    // Build context from SchemaManager
    auto context = themis::prompt_engineering::PromptManager::buildContextFromSchema(
        schema_mgr_.get(),
        themis::version::getEditionString(),
        themis::version::getVersionString()
    );
    
    std::string answer;
    std::string prompt_id;
    
    // Determine question type and select appropriate prompt
    // Error-related questions (preserve existing functionality)
    if (utils::containsCaseInsensitive(question, "fehler") ||
        utils::containsCaseInsensitive(question, "error") ||
        utils::containsCaseInsensitive(question, "problem")) {
        // Keep existing error handling
        answer = generateErrorAnswer(question);
        return {
            {"status", "success"},
            {"question", question},
            {"answer", answer}
        };
    }
    // "What can you do?" / "Was kannst du?"
    else if (utils::containsCaseInsensitive(question, "what can") ||
             utils::containsCaseInsensitive(question, "was kannst") ||
             utils::containsCaseInsensitive(question, "capabilities") ||
             utils::containsCaseInsensitive(question, "features")) {
        prompt_id = "what_can_you_do_prompt";
    }
    // "How is data structured?" / "Wie sind die Daten aufgebaut?"
    else if (utils::containsCaseInsensitive(question, "data structure") ||
             utils::containsCaseInsensitive(question, "aufgebaut") ||
             utils::containsCaseInsensitive(question, "tables") ||
             utils::containsCaseInsensitive(question, "schema") ||
             utils::containsCaseInsensitive(question, "how is") ||
             utils::containsCaseInsensitive(question, "wie sind")) {
        prompt_id = "data_structure_prompt";
    }
    // "What is your purpose?" / "Was ist deine Aufgabe?"
    else if (utils::containsCaseInsensitive(question, "purpose") ||
             utils::containsCaseInsensitive(question, "aufgabe") ||
             utils::containsCaseInsensitive(question, "why") ||
             utils::containsCaseInsensitive(question, "warum")) {
        prompt_id = "purpose_prompt";
    }
    // Specific table inquiry
    else if (utils::containsCaseInsensitive(question, "table ") ||
             utils::containsCaseInsensitive(question, "about ")) {
        prompt_id = "table_inquiry_prompt";
        
        // Try to extract table name from question
        // This is a simple implementation - could be enhanced
        auto tables = schema_mgr_->getAllTables();
        for (const auto& table : tables) {
            if (utils::containsCaseInsensitive(question, table.name)) {
                auto table_json = table.toJSON();
                context["table_details"] = table_json.dump(2);
                break;
            }
        }
    }
    // Schema introspection
    else if (utils::containsCaseInsensitive(question, "introspect") ||
             utils::containsCaseInsensitive(question, "analyze") ||
             utils::containsCaseInsensitive(question, "describe")) {
        prompt_id = "schema_introspection_prompt";
    }
    // Default: self-awareness
    else {
        prompt_id = "self_awareness_prompt";
    }
    
    // Get prompt with context injection
    auto prompt_opt = prompt_mgr_->getPromptWithContext(prompt_id, context);
    
    if (prompt_opt.has_value()) {
        answer = *prompt_opt;
        spdlog::debug("Used prompt '{}' for introspection query", prompt_id);
    } else {
        // Fallback if prompt not found
        prompt_id = "unknown_query_prompt";
        prompt_opt = prompt_mgr_->getPromptWithContext(prompt_id, context);
        
        if (prompt_opt.has_value()) {
            answer = *prompt_opt;
        } else {
            answer = "I don't have enough information to answer this question. "
                    "Please try asking about my capabilities, data structure, or purpose.";
        }
    }
    
    return {
        {"status", "success"},
        {"question", question},
        {"answer", answer},
        {"prompt_used", prompt_id}
    };
}

std::string McpServer::generateErrorAnswer(const std::string& question) {
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // "Welche Fehler können auftreten?" / "What errors can occur?"
    if (utils::containsCaseInsensitive(question, "welche fehler") ||
        utils::containsCaseInsensitive(question, "what errors") ||
        utils::containsCaseInsensitive(question, "which errors")) {
        
        auto categories = registry.getAllCategories();
        std::string answer = "I can help with the following error categories:\n\n";
        
        for (const auto& category : categories) {
            auto errors = registry.getErrorsByCategory(category);
            answer += fmt::format("**{}** ({} error types)\n", category, errors.size());
        }
        
        answer += "\nAsk me about specific errors, e.g., 'What does error 2000 mean?'";
        return answer;
    }
    
    // "Was bedeutet Fehler X?" / "What does error X mean?"
    if (utils::containsCaseInsensitive(question, "bedeutet") ||
        utils::containsCaseInsensitive(question, "mean") ||
        utils::containsCaseInsensitive(question, "what is")) {
        
        // Extract error code
        std::regex code_regex(R"(\b\d{4}\b)");
        std::smatch match;
        
        if (std::regex_search(question, match, code_regex)) {
            int code = std::stoi(match.str());
            auto metadata = registry.getError(static_cast<errors::ErrorCode>(code));
            
            if (static_cast<int>(metadata.code) != static_cast<int>(errors::ErrorCode::ERR_UNKNOWN) || 
                code == static_cast<int>(errors::ErrorCode::ERR_UNKNOWN)) {
                
                // Manual join for documentation links (fmt::join may not be available in all versions)
                std::string docs_str;
                for (size_t i = 0; i < metadata.related_docs.size(); ++i) {
                    if (i > 0) docs_str += ", ";
                    docs_str += metadata.related_docs[i];
                }
                
                return fmt::format(
                    "**Error {}: {}**\n\n"
                    "**Category:** {}\n"
                    "**Severity:** {}\n\n"
                    "**Cause:**\n{}\n\n"
                    "**Solution:**\n{}\n\n"
                    "**Documentation:** {}",
                    code,
                    metadata.message_template,
                    metadata.category,
                    metadata.severity,
                    metadata.cause,
                    metadata.solution,
                    docs_str
                );
            }
        }
    }
    
    // "How do I fix...?" / "Wie behebe ich...?"
    if (utils::containsCaseInsensitive(question, "fix") ||
        utils::containsCaseInsensitive(question, "solve") ||
        utils::containsCaseInsensitive(question, "behebe")) {
        
        // Search by keywords in question
        auto results = registry.searchErrors(question);
        if (!results.empty()) {
            std::string answer = fmt::format("I found {} relevant error(s) that might help:\n\n", 
                                            results.size());
            
            for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
                const auto& error = results[i];
                answer += fmt::format(
                    "**[{}] {}**\n{}\n\n**Solution:**\n{}\n\n",
                    static_cast<int>(error.code),
                    error.message_template,
                    error.cause,
                    error.solution
                );
            }
            
            return answer;
        }
    }
    
    // Fallback: Search by keywords
    auto results = registry.searchErrors(question);
    if (!results.empty()) {
        std::string answer = fmt::format("I found {} relevant error(s):\n\n", 
                                        results.size());
        
        for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
            const auto& error = results[i];
            answer += fmt::format(
                "**[{}] {}**\n{}\n\n",
                static_cast<int>(error.code),
                error.message_template,
                error.cause
            );
        }
        
        return answer;
    }
    
    return "I couldn't find matching error information. "
           "Try asking with a specific error code or keyword like 'gpu', 'model', 'storage', etc.";
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
    // Full integration: return real schema data from SchemaManager
    if (!schema_mgr_) {
        return {
            {"status", "error"},
            {"message", "SchemaManager not initialized"},
            {"nodes", json::array()},
            {"edges", json::array()}
        };
    }
    
    try {
        return schema_mgr_->toJSON();
    } catch (const std::exception& e) {
        spdlog::error("Error retrieving schema resource: {}", e.what());
        return {
            {"status", "error"},
            {"message", std::string("Failed to retrieve schema: ") + e.what()},
            {"nodes", json::array()},
            {"edges", json::array()}
        };
    }
}

json McpServer::resourceStats(const std::string& uri) {
    // Provide real statistics if SchemaManager is available
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "disconnected"},
            {"database_open", false},
            {"message", "Database not attached or not open"}
        };
    }
    
    if (schema_mgr_) {
        try {
            auto metadata = schema_mgr_->getDatabaseMetadata();
            return {
                {"status", "connected"},
                {"database_open", true},
                {"version", metadata.version},
                {"table_count", metadata.table_count},
                {"total_rows", metadata.total_rows},
                {"capabilities", metadata.capabilities},
                {"last_refresh", metadata.toJSON()["last_refresh"]}
            };
        } catch (const std::exception& e) {
            spdlog::error("Error retrieving stats: {}", e.what());
            return {
                {"status", "connected"},
                {"database_open", true},
                {"message", std::string("Error retrieving stats: ") + e.what()}
            };
        }
    }
    
    return {
        {"status", "connected"},
        {"database_open", true},
        {"message", "Database statistics available in full integration"},
        {"note", "Minimal integration provides basic connectivity status only"}
    };
}

json McpServer::resourceMetadata(const std::string& uri) {
    // Determine integration level based on SchemaManager availability
    std::string integration_level = schema_mgr_ ? "full" : "minimal";
    
    json supported_ops = {"put_entity", "get_entity", "delete_entity", "create_index"};
    json pending_ops = json::array();
    
    if (schema_mgr_) {
        supported_ops.push_back("get_schema");
        supported_ops.push_back("schema_discovery");
        supported_ops.push_back("full_query");
    } else {
        pending_ops.push_back("full_query");
        pending_ops.push_back("schema_discovery");
    }
    
    return {
        {"version", config_.server_version},
        {"name", config_.server_name},
        {"integration_level", integration_level},
        {"supported_operations", supported_ops},
        {"pending_operations", pending_ops},
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
    
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    // Start async stdin reading
    readStdin();
#else
    spdlog::warn("MCP stdio transport: Unsupported platform, stdin reading not implemented");
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
#if defined(_WIN32)
    // Windows implementation using PeekNamedPipe for non-blocking stdin
    asio::post(io_context_, [this]() {
        HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
        if (h_stdin == INVALID_HANDLE_VALUE) {
            errors::logError(ErrorCode::ERR_MCP_STDIO_INIT_FAILED, "stdin handle");
            return;
        }

        while (is_running_) {
            // Check if data is available
            DWORD bytes_available = 0;
            BOOL peek_result = PeekNamedPipe(h_stdin, NULL, 0, NULL, &bytes_available, NULL);
            
            if (!peek_result) {
                // PeekNamedPipe fails on console handles, use alternative approach
                // Use _kbhit() for console or just do blocking read with timeout via thread
                DWORD bytes_read = 0;
                char buffer[4096];
                
                // Set read to timeout by using asynchronous pattern
                DWORD mode = 0;
                GetConsoleMode(h_stdin, &mode);
                
                if (ReadFile(h_stdin, buffer, 1, &bytes_read, NULL) && bytes_read > 0) {
                    // Read rest of line
                    std::string line;
                    line += buffer[0];
                    
                    while (std::cin.peek() != '\n' && std::cin.peek() != EOF) {
                        char ch;
                        if (std::cin.get(ch)) {
                            line += ch;
                        } else {
                            break;
                        }
                    }
                    
                    // Consume the newline
                    if (std::cin.peek() == '\n') {
                        std::cin.get();
                    }
                    
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
                } else if (!is_running_) {
                    break;
                }
            } else if (bytes_available > 0) {
                // Data available, read it
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
            } else {
                // No data, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
#elif defined(__unix__) || defined(__APPLE__)
    // POSIX implementation using select()
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
#else
    spdlog::warn("MCP stdio transport not supported on this platform");
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
        errors::logError(ErrorCode::ERR_MCP_TRANSPORT_FAILED, e.what());
        
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
