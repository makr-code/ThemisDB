/**
 * @file mcp_server.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=2, Mock=1, Sim=1, Debt=0, C=1, H=27, M=36, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_MCP

#include "server/mcp_server.h"
#include <stdexcept>
#include "server/http_server.h"
#include "storage/rocksdb_wrapper.h"
#include "metadata/schema_manager.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "query/aql_runner.h"
#include "query/aql_safety_validator.h"
#include "query/cypher_parser.h"
#include "index/graph_index.h"
#include "llm/embedded_llm.h"
#include "prompt_engineering/prompt_manager.h"
#include "security/ai_operation_guard.h"
#include "utils/error_registry.h"
#include "utils/string_utils.h"
#include "config/config_path_resolver.h"
#include "security/ai_snapshot_cleanup.h"
#include "utils/audit_logger.h"
#include "version.h"
#ifdef THEMIS_ENABLE_LLM
#include "llm/ai_orchestrator.h"
#endif
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <fmt/format.h>
#include <regex>
#include <cctype>

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

    // AI Safety Layer — Schichten 1 & 2: Initialise Destructive Operation Guard.
    // Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
    // Roadmap: src/security/ROADMAP.md § Phase 2 (ASL-4)
    themis::security::AiOperationGuard::Config guard_cfg;
    guard_cfg.enabled                = true;
    guard_cfg.approval_threshold     = themis::security::OperationClass::DESTRUCTIVE;
    guard_cfg.approval_timeout_s     = 60;
    guard_cfg.auto_snapshot          = true;
    guard_cfg.snapshot_dir           = themis::security::themisDefaultSnapshotDir();
    guard_cfg.environment            = "development";  // Override via attachConfig()
    guard_cfg.block_critical_in_prod = true;
    operation_guard_ = std::make_unique<themis::security::AiOperationGuard>(std::move(guard_cfg));
    spdlog::info("MCP AI Safety Guard initialised (DOG+HILG, threshold=DESTRUCTIVE)");

    // ASL-9: Load security.yaml to override guard config with deployment settings.
    // Docs: src/security/ROADMAP.md § Phase 3 (ASL-9)
    try {
        std::string yaml_path;
        if (auto resolved = themis::config::ConfigPathResolver::tryResolve("config/security.yaml")) {
            yaml_path = *resolved;
        } else {
            yaml_path = "config/security.yaml";
        }

        const YAML::Node root = YAML::LoadFile(yaml_path);

        themis::security::AiOperationGuard::Config new_cfg = operation_guard_->config();

        if (const auto& env = root["environment"]) {
            if (env["name"]) {
                new_cfg.environment = env["name"].as<std::string>(new_cfg.environment);
            }
            if (const auto& restrictions = env["ai_agent_restrictions"]) {
                if (restrictions["block_destructive"]) {
                    new_cfg.block_critical_in_prod = restrictions["block_destructive"].as<bool>(
                        new_cfg.block_critical_in_prod);
                }
                if (restrictions["denied_collections"]) {
                    new_cfg.denied_collections.clear();
                    for (const auto& c : restrictions["denied_collections"]) {
                        new_cfg.denied_collections.push_back(c.as<std::string>());
                    }
                }
                if (restrictions["allowed_collections"]) {
                    new_cfg.allowed_collections.clear();
                    for (const auto& c : restrictions["allowed_collections"]) {
                        new_cfg.allowed_collections.push_back(c.as<std::string>());
                    }
                }
                if (restrictions["require_role_for_critical"]) {
                    new_cfg.critical_ops_role = restrictions["require_role_for_critical"].as<std::string>(
                        new_cfg.critical_ops_role);
                }
            }
        }

        if (const auto& ai_safety = root["ai_safety"]) {
            if (const auto& snapshot = ai_safety["snapshot"]) {
                if (snapshot["dir"]) {
                    new_cfg.snapshot_dir = snapshot["dir"].as<std::string>(new_cfg.snapshot_dir);
                }
                if (snapshot["retention_days"]) {
                    snapshot_retention_days_ = snapshot["retention_days"].as<int>(snapshot_retention_days_);
                }
                if (snapshot["max_total_size_gb"]) {
                    snapshot_max_total_gb_ = snapshot["max_total_size_gb"].as<int>(snapshot_max_total_gb_);
                }
            }
        }

        operation_guard_ = std::make_unique<themis::security::AiOperationGuard>(std::move(new_cfg));
        spdlog::info("MCP AI Safety Guard: config loaded from '{}' (env='{}', snapshot_dir='{}')",
                     yaml_path, operation_guard_->config().environment,
                     operation_guard_->config().snapshot_dir);
    } catch (const std::exception& ex) {
        spdlog::warn("MCP AI Safety Guard: could not load security.yaml ({}), using defaults", ex.what());
    }

    // ASL-7: Load the 'agentic' mode safety: block from the Mode-YAML spec.
    // Overrides guard config fields (enabled, approval_threshold, approval_timeout_s,
    // auto_snapshot, snapshot_dir, dry_run_preview) with values from the agentic mode.
    // Falls back silently to the defaults already set by the security.yaml (ASL-9) load above.
    // Docs:    src/security/ROADMAP.md § Phase 2 (ASL-7)
    // Config:  config/ai_ml/llm/modes/default.yaml → modes[id=agentic].safety
    try {
        std::string mode_yaml_path;
        // HIGH-GAP FIX: unnecessary_copy — use string_view for const static path
        constexpr std::string_view kModeYamlKey = "config/ai_ml/llm/modes/default.yaml";
        if (auto resolved = themis::config::ConfigPathResolver::tryResolve(kModeYamlKey)) {
            mode_yaml_path = *resolved;
        } else {
            mode_yaml_path = std::string(kModeYamlKey);
        }

        const YAML::Node mode_root = YAML::LoadFile(mode_yaml_path);
        const auto& modes_node = mode_root["modes"];
        if (modes_node && modes_node.IsSequence()) {
            themis::security::AiOperationGuard::Config mode_cfg = operation_guard_->config();
            bool applied = false;
            for (const auto& mode : modes_node) {
                const auto& id_node = mode["id"];
                if (!id_node || id_node.as<std::string>() != "agentic") continue;
                const auto& safety = mode["safety"];
                if (!safety || !safety.IsMap()) break;

                if (safety["enabled"]) {
                    mode_cfg.enabled = safety["enabled"].as<bool>(mode_cfg.enabled);
                }
                if (safety["approval_timeout_s"]) {
                    mode_cfg.approval_timeout_s =
                        safety["approval_timeout_s"].as<int>(mode_cfg.approval_timeout_s);
                }
                if (safety["auto_snapshot"]) {
                    mode_cfg.auto_snapshot =
                        safety["auto_snapshot"].as<bool>(mode_cfg.auto_snapshot);
                }
                if (safety["snapshot_dir"]) {
                    mode_cfg.snapshot_dir =
                        safety["snapshot_dir"].as<std::string>(mode_cfg.snapshot_dir);
                }
                if (safety["dry_run_preview"]) {
                    mode_cfg.dry_run_preview =
                        safety["dry_run_preview"].as<bool>(mode_cfg.dry_run_preview);
                }
                // require_approval_for: [DESTRUCTIVE, CRITICAL]
                // → approval_threshold = lowest class in the list
                if (safety["require_approval_for"] &&
                    safety["require_approval_for"].IsSequence()) {
                    themis::security::OperationClass min_threshold =
                        themis::security::OperationClass::CRITICAL;
                    for (const auto& cls : safety["require_approval_for"]) {
                        // HIGH-GAP FIX: unnecessary_copy — avoid std::string copy, use string_view
                        const std::string_view cls_name = cls.as<std::string>();
                        if (cls_name == "WRITE_SAFE") {
                            min_threshold = themis::security::OperationClass::WRITE_SAFE;
                            break;
                        }
                        if (cls_name == "DESTRUCTIVE") {
                            // DESTRUCTIVE < CRITICAL, keep searching for lower
                            min_threshold = themis::security::OperationClass::DESTRUCTIVE;
                        }
                    }
                    mode_cfg.approval_threshold = min_threshold;
                }

                applied = true;
                break;
            }
            if (applied) {
                operation_guard_ =
                    std::make_unique<themis::security::AiOperationGuard>(std::move(mode_cfg));
                spdlog::info(
                    "MCP ASL-7: agentic mode safety config applied from '{}' "
                    "(threshold={}, timeout={}s, auto_snapshot={}, dry_run_preview={})",
                    mode_yaml_path,
                    themis::security::operationClassName(operation_guard_->config().approval_threshold),
                    operation_guard_->config().approval_timeout_s,
                    operation_guard_->config().auto_snapshot,
                    operation_guard_->config().dry_run_preview);
            } else {
                spdlog::debug(
                    "MCP ASL-7: no 'agentic' mode with safety: block in '{}', "
                    "guard config unchanged", mode_yaml_path);
            }
        }
    } catch (const std::exception& ex) {
        spdlog::warn("MCP ASL-7: could not load mode YAML ({}), guard config unchanged", ex.what());
    }
}

McpServer::~McpServer() {
    stop();
}

void McpServer::start() {
    bool expected = false;
    if (!is_running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
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
        // Defensive null guard: make_shared throws on OOM, so stdio_transport_
        // is always non-null here; the guard makes the invariant explicit for
        // static analyzers and future refactors.
        if (stdio_transport_) {
            stdio_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
            stdio_transport_->start();
            spdlog::info("MCP stdio transport started");
        } else {
            spdlog::error("MCP: stdio transport allocation failed — stdio disabled");
        }
    }

    if (config_.enable_sse) {
        sse_transport_ = std::make_shared<SseTransport>(io_context_, config_.sse_keepalive_ms);
        if (sse_transport_) {
            sse_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
            sse_transport_->start();
            spdlog::info("MCP SSE transport started");
        } else {
            spdlog::error("MCP: SSE transport allocation failed — SSE disabled");
        }
    }

    if (config_.enable_websocket) {
        ws_transport_ = std::make_shared<WebSocketTransport>(io_context_, config_.websocket_ping_interval_ms);
        if (ws_transport_) {
            ws_transport_->setMessageHandler([this](const json& req) { return handleRequest(req); });
            ws_transport_->start();
            spdlog::info("MCP WebSocket transport started");
        } else {
            spdlog::error("MCP: WebSocket transport allocation failed — WebSocket disabled");
        }
    }

    spdlog::info("MCP Server started successfully");
}

void McpServer::stop() {
    if (!is_running_.exchange(false, std::memory_order_acq_rel)) {
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
// ASL-12: AI Session Audit Trail — setAuditLogger + logAiEvent
// Docs: docs/de/security/ai_safety/AI_SAFETY_AUDIT_TRAIL.md
// Roadmap: src/security/ROADMAP.md § Phase 4 (ASL-12)
// ============================================================================

void McpServer::setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger) {
    audit_logger_ = std::move(logger);
}

void McpServer::logAiEvent(
    themis::utils::SecurityEventType type,
    const std::string&               tool_name,
    const std::string&               ai_session_id,
    const nlohmann::json&            details
) {
    if (!audit_logger_) [[unlikely]] {
        return;
    }
    auto& audit_logger = *audit_logger_;
    audit_logger.logSecurityEvent(
        type,
        ai_session_id,
        "mcp://" + tool_name,
        details
    );
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
    auto& orchestrator_ref = *orchestrator_;

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

    const auto& pack = orchestrator_ref.modePack();
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
    initialized_.store(true, std::memory_order_release);
    
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
    
    // Guard against an empty/null std::function stored for this tool.
    // Calling an empty std::function throws std::bad_function_call; we
    // catch that below, but making the guard explicit surfaces registration
    // bugs as a distinct, unambiguous error code.
    if (!it->second.handler) {
        return createError(-32601, "Tool handler not available: " + name);
    }
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

    // Guard against an empty/null std::function stored for this resource.
    if (!it->second.handler) {
        return createError(-32601, "Resource handler not available: " + uri);
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
    
    // Guard against an empty/null std::function stored for this prompt.
    if (!it->second.handler) {
        return createError(-32601, "Prompt handler not available: " + name);
    }
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
                {"key", {{"type", "string"}}},
                {"dry_run", {{"type", "boolean"}, {"default", false},
                             {"description", "AI Safety: preview the delete without executing it (Phase 1 guard)"}}}
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
                {"type", {{"type", "string"}, {"enum", {"regular", "range", "sparse", "geo", "fulltext", "ttl"}}, {"default", "regular"}, {"description", "Index type"}}},
                {"dry_run", {{"type", "boolean"}, {"default", false},
                             {"description", "AI Safety: preview the drop without executing it (Phase 1 guard)"}}}
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

    // ASL-11: AI snapshot cleanup tool
    registerTool("ai_cleanup_snapshots",
        "Delete expired AI pre-operation snapshots (retention policy enforcement)",
        json::object(),
        [this](const json& args) { return toolAiCleanupSnapshots(args); });

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
    // AI Safety Layer (Schicht 3): enforce_read_only flag carried in args
    // when this tool is called from the `agentic` mode with aql_execute.
    // The flag is set by the MCP tool spec (enforce_read_only: true in YAML).
    const bool enforce_read_only = args.value("enforce_read_only", false);
    
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

            // AI Safety Layer — Schicht 3: AQL Read-Only Enforcer (ASL-3)
            // Activated when enforce_read_only=true is carried in the tool args
            // (set by the `aql_execute` MCP tool spec in the agentic LLM mode).
            // Docs: docs/de/security/ai_safety/AI_SAFETY_AQL_VALIDATOR.md
            if (enforce_read_only) {
                themis::query::AqlSafetyValidator aql_validator;
                auto violation = aql_validator.validate(query);
                if (violation.has_value()) {
                    spdlog::warn("AI Safety Layer (ASL-3): AQL read-only violation "
                                 "blocked: keyword='{}' pos={} query='{}'",
                                 violation->keyword, violation->position, query);
                    return {
                        {"status", "error"},
                        {"error_code", "AQL_READ_ONLY_VIOLATION"},
                        {"message", violation->message},
                        {"query", query},
                        {"language", language},
                        {"blocked_by", "AqlSafetyValidator"},
                        {"violation_keyword", violation->keyword},
                        {"violation_position", violation->position}
                    };
                }
            }

            // AI Safety Layer — Schichten 1 & 2: DOG + HILG (ASL-4/5)
            // For AQL write/delete operations: require approval before execution.
            if (const auto guard_resp = checkOperationGuard(
                    "query", args,
                    /*ai_session_id=*/"", /*caller_role=*/"")) {
                return *guard_resp;
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
        // SQL and Cypher: transpile to AQL, then execute through the AQL engine
        else if (language == "sql" || language == "cypher") {
            if (!query_engine_) {
                return {
                    {"status", "error"},
                    {"message", "Query engine not initialized"},
                    {"query", query},
                    {"language", language}
                };
            }

            // Transpile SQL/Cypher → AQL
            std::string aql_query;
            if (language == "sql") {
                themis::query::SQLParser sql_parser;
                auto parse_result = sql_parser.parse(query);
                if (!parse_result) {
                    return {
                        {"status", "error"},
                        {"message", fmt::format("SQL parse error: {}", parse_result.error().message())},
                        {"query", query},
                        {"language", language}
                    };
                }
                themis::query::SQLToAQLTranspiler transpiler;
                auto transpile_result = transpiler.transpile(*parse_result);
                if (!transpile_result) {
                    return {
                        {"status", "error"},
                        {"message", fmt::format("SQL→AQL transpilation failed: {}", transpile_result.error().message())},
                        {"query", query},
                        {"language", language}
                    };
                }
                aql_query = *transpile_result;
            } else {  // cypher
                themis::query::CypherParser cypher_parser;
                auto parse_result = cypher_parser.parse(query);
                if (!parse_result) {
                    return {
                        {"status", "error"},
                        {"message", fmt::format("Cypher parse error: {}", parse_result.error().message())},
                        {"query", query},
                        {"language", language}
                    };
                }
                themis::query::CypherToAQLTranspiler transpiler;
                auto transpile_result = transpiler.transpile(*parse_result);
                if (!transpile_result) {
                    return {
                        {"status", "error"},
                        {"message", fmt::format("Cypher→AQL transpilation failed: {}", transpile_result.error().message())},
                        {"query", query},
                        {"language", language}
                    };
                }
                aql_query = *transpile_result;
            }

            // Execute the transpiled AQL query
            auto result = executeAql(aql_query, *query_engine_);
            if (!result) {
                return {
                    {"status", "error"},
                    {"message", fmt::format("{} execution failed (via AQL transpilation): {}",
                                           language == "sql" ? "SQL" : "Cypher",
                                           result.error().message())},
                    {"query", query},
                    {"transpiled_aql", aql_query},
                    {"language", language}
                };
            }

            return {
                {"status", "success"},
                {"message", fmt::format("{} query executed successfully (via AQL transpilation)",
                                        language == "sql" ? "SQL" : "Cypher")},
                {"query", query},
                {"transpiled_aql", aql_query},
                {"language", language},
                {"results", *result}
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
    
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
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
    
    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
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
    // AI Safety Layer (Phase 1): dry_run flag — preview the operation without
    // executing it. Phase 2 will replace this with the full HILG Approval-Flow.
    const bool dry_run = args.value("dry_run", false);

    spdlog::info("MCP Tool 'delete_entity' called: key={} dry_run={}", key, dry_run);

    if (!db_ || !db_->isOpen()) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
            {"key", key}
        };
    }

    // AI Safety Layer — Schichten 1 & 2: DOG + HILG (ASL-4/5)
    // Guard intercept for DESTRUCTIVE operations.
    if (const auto guard_resp = checkOperationGuard(
            "delete_entity", args,
            /*ai_session_id=*/"", /*caller_role=*/"")) {
        return *guard_resp;
    }

    // Dry-run mode: return a preview without touching the database.
    if (dry_run) {
        spdlog::info("AI Safety dry_run: delete_entity key={} — preview only", key);
        return {
            {"status", "dry_run"},
            {"message", "Dry-run preview: entity would be deleted (not executed)"},
            {"key", key},
            {"dry_run", true},
            {"classification", "DESTRUCTIVE"},
            {"note", "Submit with dry_run=false after human approval to execute."}
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
    auto& index_mgr = *index_mgr_;
    
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
            status = index_mgr.createIndex(table, column, unique);
        } else if (index_type == "range") {
            status = index_mgr.createRangeIndex(table, column);
        } else if (index_type == "sparse") {
            status = index_mgr.createSparseIndex(table, column, unique);
        } else if (index_type == "geo" || index_type == "geospatial") {
            status = index_mgr.createGeoIndex(table, column);
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
            status = index_mgr.createFulltextIndex(table, column, config);
        } else if (index_type == "ttl") {
            int64_t ttl_seconds = args.value("ttl_seconds", 86400); // default 1 day
            status = index_mgr.createTTLIndex(table, column, ttl_seconds);
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
    auto& index_mgr = *index_mgr_;
    
    // Extract parameters
    std::string table = args.value("table", "");
    std::string column = args.value("column", "");
    std::string index_type = args.value("type", "regular");
    // AI Safety Layer (Phase 1): dry_run flag — preview the drop without
    // executing it. Phase 2 will replace this with the full HILG Approval-Flow.
    const bool dry_run = args.value("dry_run", false);
    
    if (table.empty() || column.empty()) {
        return {
            {"status", "error"},
            {"message", "Missing required parameters: table and column"}
        };
    }

    // AI Safety Layer — Schichten 1 & 2: DOG + HILG (ASL-4/5)
    if (const auto guard_resp = checkOperationGuard(
            "drop_index", args,
            /*ai_session_id=*/"", /*caller_role=*/"")) {
        return *guard_resp;
    }

    // Dry-run mode: return a preview without dropping the index.
    if (dry_run) {
        spdlog::info("AI Safety dry_run: drop_index {}.{} ({}) — preview only",
                     table, column, index_type);
        return {
            {"status", "dry_run"},
            {"message", fmt::format("Dry-run preview: index {}.{} ({}) would be dropped (not executed)",
                                    table, column, index_type)},
            {"table", table},
            {"column", column},
            {"index_type", index_type},
            {"dry_run", true},
            {"classification", "DESTRUCTIVE"},
            {"note", "Submit with dry_run=false after human approval to execute."}
        };
    }

    try {
        SecondaryIndexManager::Status status;
        
        // Drop the appropriate index type
        if (index_type == "regular" || index_type == "secondary") {
            status = index_mgr.dropIndex(table, column);
        } else if (index_type == "range") {
            status = index_mgr.dropRangeIndex(table, column);
        } else if (index_type == "sparse") {
            status = index_mgr.dropSparseIndex(table, column);
        } else if (index_type == "geo" || index_type == "geospatial") {
            status = index_mgr.dropGeoIndex(table, column);
        } else if (index_type == "fulltext") {
            status = index_mgr.dropFulltextIndex(table, column);
        } else if (index_type == "ttl") {
            status = index_mgr.dropTTLIndex(table, column);
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
    auto& index_mgr = *index_mgr_;
    
    try {
        // Get all tables from schema manager
        json indexes = json::array();
        
        if (schema_mgr_) {
            auto& schema_mgr = *schema_mgr_;
            auto tables = schema_mgr.getAllTables();
            
            for (const auto& table : tables) {
                // Get index stats for each table
                auto stats = index_mgr.getAllIndexStats(table.name);
                
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

    const bool database_connected = db_ && db_->isOpen();
    if (!database_connected) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
            {"database_connected", false},
            {"nodes", json::array()},
            {"edges", json::array()},
            {"properties", json::object()}
        };
    }

    if (!schema_mgr_) {
        return {
            {"status", "error"},
            {"message", "SchemaManager not initialized"},
            {"database_connected", true},
            {"integration_level", "minimal"},
            {"nodes", json::array()},
            {"edges", json::array()},
            {"properties", json::object()}
        };
    }
    auto& schema_mgr = *schema_mgr_;

    // Full integration: return real schema data from SchemaManager
    try {
        auto schema_json = schema_mgr.toJSON();
        
        // Add integration level indicator
        schema_json["integration_level"] = "full";
        schema_json["database_connected"] = database_connected;
        
        return schema_json;
    } catch (const std::exception& e) {
        spdlog::error("Error retrieving schema: {}", e.what());
        return {
            {"status", "error"},
            {"message", std::string("Failed to retrieve schema: ") + e.what()},
            {"database_connected", database_connected},
            {"integration_level", "full"},
            {"nodes", json::array()},
            {"edges", json::array()},
            {"properties", json::object()}
        };
    }
}

json McpServer::toolGetStats(const json& args) {
    spdlog::info("MCP Tool 'get_stats' called");

    const bool database_connected = db_ && db_->isOpen();
    if (!database_connected) {
        return {
            {"status", "error"},
            {"message", "Database not attached or not open"},
            {"database_connected", false},
            {"node_count", 0},
            {"edge_count", 0},
            {"storage_size_bytes", 0}
        };
    }

    // Full integration: return real statistics from SchemaManager
    if (schema_mgr_) {
        auto& schema_mgr = *schema_mgr_;
        try {
            auto metadata = schema_mgr.getDatabaseMetadata();
            return {
                {"status", "success"},
                {"database_connected", database_connected},
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
                {"database_connected", database_connected},
                {"integration_level", "full"}
            };
        }
    }

    // Minimal integration: return connection status
    return {
        {"status", "success"},
        {"database_connected", database_connected},
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
    auto& orchestrator = *orchestrator_;

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

        const auto result = orchestrator.run(ctx);

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

    auto& orchestrator = *orchestrator_;
    const auto& pack = orchestrator.modePack();

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
        THEMIS_DEBUG("mcp_server: unhandled exception caught");
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
    auto& schema_mgr = *schema_mgr_;
    auto context = themis::prompt_engineering::PromptManager::buildContextFromSchema(
        &schema_mgr,
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
        auto tables = schema_mgr.getAllTables();
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
    auto& schema_mgr = *schema_mgr_;
    
    try {
        return schema_mgr.toJSON();
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
        auto& schema_mgr = *schema_mgr_;
        try {
            auto metadata = schema_mgr.getDatabaseMetadata();
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
// AI Safety Layer — HILG: Approval Pipeline (ASL-4..6)
// Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
// Roadmap: src/security/ROADMAP.md § Phase 2
// ============================================================================

std::optional<json> McpServer::checkOperationGuard(
    const std::string& tool_name,
    const json&        args,
    const std::string& ai_session_id,
    const std::string& caller_role
) {
    if (!operation_guard_) {
        return std::nullopt;  // Guard not initialised — pass through
    }
    auto& operation_guard = *operation_guard_;

    const auto decision = operation_guard.evaluate(
        tool_name, args, ai_session_id, caller_role);

    // Hard block: return immediately without storing in queue
    if (!decision.block_reason.empty()) {
        spdlog::warn("AI Safety DOG: HARD BLOCK tool='{}' reason='{}'",
                     tool_name, decision.block_reason);
        logAiEvent(themis::utils::SecurityEventType::AI_OPERATION_DENIED,
                   tool_name, ai_session_id,
                   {{"reason", decision.block_reason}, {"op_class", themis::security::operationClassName(decision.op_class)}});
        return operation_guard.buildBlockedResponse(decision);
    }

    // READ_ONLY / WRITE_SAFE: no interception needed
    if (!decision.requires_approval) {
        logAiEvent(themis::utils::SecurityEventType::AI_TOOL_CALL,
                   tool_name, ai_session_id,
                   {{"op_class", themis::security::operationClassName(decision.op_class)}});
        return std::nullopt;
    }

    // DESTRUCTIVE / CRITICAL: enter HILG queue
    spdlog::warn("AI Safety HILG: requires_approval tool='{}' class='{}' op_id='{}'",
                 tool_name,
                 themis::security::operationClassName(decision.op_class),
                 decision.operation_id);

    // Build the approval response before locking
    const json approval_resp = operation_guard.buildRequiresApprovalResponse(decision);

    {
        std::lock_guard<std::mutex> lock(pending_approvals_mutex_);

        // Purge stale entries to keep the map bounded
        purgeExpiredApprovals();

        PendingApproval pa;
        pa.operation_id      = decision.operation_id;
        pa.ai_session_id     = ai_session_id;
        pa.tool_name         = tool_name;
        pa.operation_args    = args;
        pa.classification    =
            themis::security::operationClassName(decision.op_class);
        pa.approval_response = approval_resp;
        pa.created_at        = std::chrono::system_clock::now();
        pa.expires_at        = pa.created_at +
            std::chrono::seconds(operation_guard.config().approval_timeout_s);
        pa.is_executed       = false;

        pending_approvals_.emplace(decision.operation_id, std::move(pa));
    }

    logAiEvent(themis::utils::SecurityEventType::AI_APPROVAL_REQUIRED,
               tool_name, ai_session_id,
               {{"operation_id", decision.operation_id},
                {"op_class", themis::security::operationClassName(decision.op_class)}});

    return approval_resp;
}

json McpServer::handleAiApprove(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(pending_approvals_mutex_);
    purgeExpiredApprovals();

    auto it = pending_approvals_.find(operation_id);
    if (it == pending_approvals_.end()) {
        return {
            {"status",  "error"},
            {"error_code", "OPERATION_NOT_FOUND"},
            {"message", fmt::format(
                "Operation '{}' not found. It may have expired or already been executed.",
                operation_id)}
        };
    }

    if (it->second.is_executed) {
        return {
            {"status",  "error"},
            {"error_code", "ALREADY_EXECUTED"},
            {"message", fmt::format("Operation '{}' was already executed.", operation_id)}
        };
    }

    // Retrieve cached operation details
    const std::string tool   = it->second.tool_name;
    const json        op_args = it->second.operation_args;
    it->second.is_executed = true;

    spdlog::info("AI Safety HILG: APPROVED tool='{}' op_id='{}'", tool, operation_id);

    // ASL-8: Pre-operation snapshot for DESTRUCTIVE/CRITICAL operations.
    // Docs: src/security/ROADMAP.md § Phase 3 (ASL-8)
    std::string pre_snapshot_path;
    const bool needs_snapshot = db_ && operation_guard_ &&
        (it->second.classification == "DESTRUCTIVE" || it->second.classification == "CRITICAL");
    if (needs_snapshot) {
        pre_snapshot_path = operation_guard_->config().snapshot_dir +
                            "/" + operation_id + "_pre_op";
        try {
            if (db_->createCheckpoint(pre_snapshot_path)) {
                it->second.pre_snapshot_path = pre_snapshot_path;
                spdlog::info("AI Safety ASL-8: pre-op snapshot created at '{}' for op_id='{}'",
                             pre_snapshot_path, operation_id);
                logAiEvent(themis::utils::SecurityEventType::AI_SNAPSHOT_CREATED,
                           tool, it->second.ai_session_id,
                           {{"operation_id", operation_id}, {"snapshot_path", pre_snapshot_path}});
            } else {
                pre_snapshot_path.clear();
                spdlog::warn("AI Safety ASL-8: createCheckpoint returned false for op_id='{}'",
                             operation_id);
            }
        } catch (const std::exception& ex) {
            pre_snapshot_path.clear();
            spdlog::error("AI Safety ASL-8: snapshot failed for op_id='{}': {}", operation_id, ex.what());
        }
    }

    // Execute the originally queued operation by re-dispatching through the
    // normal tool handlers (with enforce_read_only=false, dry_run=false).
    json mutable_args = op_args;
    mutable_args["dry_run"]         = false;
    mutable_args["enforce_read_only"] = false;

    json exec_result;
    try {
        const auto tool_it = tools_.find(tool);
        if (tool_it == tools_.end()) {
            exec_result = {{"status","error"},{"message","Tool not found after approval"}};
        } else if (!tool_it->second.handler) {
            // Guard: handler stored as empty std::function — indicates a registration bug.
            exec_result = {{"status","error"},{"message","Tool handler not available after approval: " + tool}};
        } else {
            exec_result = tool_it->second.handler(mutable_args);
        }
    } catch (const std::exception& e) {
        exec_result = {{"status","error"},{"message", std::string("Execution failed: ") + e.what()}};
    }

    // Remove from pending map
    const std::string approved_session = it->second.ai_session_id;
    pending_approvals_.erase(it);

    logAiEvent(themis::utils::SecurityEventType::AI_OPERATION_EXECUTED,
               tool, approved_session,
               {{"operation_id", operation_id}, {"pre_snapshot", pre_snapshot_path}});

    return {
        {"status",                  "executed"},
        {"operation_id",            operation_id},
        {"approved_by",             "(http-approval-endpoint)"},
        {"pre_operation_snapshot",  pre_snapshot_path},
        {"result",                  exec_result}
    };
}

json McpServer::handleAiDeny(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(pending_approvals_mutex_);

    auto it = pending_approvals_.find(operation_id);
    if (it == pending_approvals_.end()) {
        return {
            {"status",     "error"},
            {"error_code", "OPERATION_NOT_FOUND"},
            {"message",    fmt::format("Operation '{}' not found.", operation_id)}
        };
    }

    spdlog::info("AI Safety HILG: DENIED tool='{}' op_id='{}'",
                 it->second.tool_name, operation_id);
    const std::string denied_tool    = it->second.tool_name;
    const std::string denied_session = it->second.ai_session_id;
    pending_approvals_.erase(it);

    logAiEvent(themis::utils::SecurityEventType::AI_OPERATION_DENIED,
               denied_tool, denied_session,
               {{"operation_id", operation_id}});

    return {
        {"status",       "denied"},
        {"operation_id", operation_id},
        {"message",      "Operation was denied by operator."}
    };
}

json McpServer::handleAiPendingApprovals() {
    std::lock_guard<std::mutex> lock(pending_approvals_mutex_);
    purgeExpiredApprovals();

    json list = json::array();
    for (const auto& [op_id, pa] : pending_approvals_) {
        if (!pa.is_executed) {
            list.push_back({
                {"operation_id",  pa.operation_id},
                {"tool_name",     pa.tool_name},
                {"classification",pa.classification},
                {"ai_session_id", pa.ai_session_id},
                {"created_at",    pa.approval_response.value("expires_at", "")},
                {"expires_at",    pa.approval_response.value("expires_at", "")},
                {"approve_url",   fmt::format("/v1/ai/approve/{}", pa.operation_id)},
                {"deny_url",      fmt::format("/v1/ai/deny/{}", pa.operation_id)}
            });
        }
    }

    return {
        {"status",  "success"},
        {"count",   list.size()},
        {"pending", list}
    };
}

void McpServer::purgeExpiredApprovals() {
    // Caller holds pending_approvals_mutex_.
    const auto now = std::chrono::system_clock::now();
    for (auto it = pending_approvals_.begin(); it != pending_approvals_.end(); ) {
        if (now >= it->second.expires_at) {
            spdlog::debug("AI Safety HILG: expired op_id='{}' tool='{}'",
                          it->second.operation_id, it->second.tool_name);
            logAiEvent(themis::utils::SecurityEventType::AI_OPERATION_EXPIRED,
                       it->second.tool_name, it->second.ai_session_id,
                       {{"operation_id", it->second.operation_id}});
            it = pending_approvals_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// ASL-10: Rollback endpoint
// Docs: src/security/ROADMAP.md § Phase 3 (ASL-10)
// ============================================================================

json McpServer::handleAiRollback(const std::string& snapshot_id) {
    auto hasWindowsDrivePrefix = [](const std::string& value) {
        return value.size() >= 2 &&
               std::isalpha(static_cast<unsigned char>(value[0])) &&
               value[1] == ':';
    };
    auto isSafeSnapshotId = [](const std::string& value) {
        constexpr size_t kMaxSnapshotIdLength = 128;
        if (value.empty() || value.size() > kMaxSnapshotIdLength) {
            return false;
        }

        for (const unsigned char ch : value) {
            if (std::iscntrl(ch)) {
                return false;
            }
            const bool is_alnum = std::isalnum(ch) != 0;
            const bool is_allowed_symbol = (ch == '_') || (ch == '-') || (ch == '.');
            if (!is_alnum && !is_allowed_symbol) {
                return false;
            }
        }
        return true;
    };
    auto isPathWithinBase = [](const std::filesystem::path& child,
                               const std::filesystem::path& base) {
        const std::filesystem::path rel = child.lexically_relative(base);
        if (rel.empty() || rel.is_absolute()) {
            return false;
        }
        for (const auto& part : rel) {
            if (part == "..") {
                return false;
            }
        }
        return true;
    };

    if (!db_ || !db_->isOpen()) {
        spdlog::warn("AI Safety ASL-10: rollback requested but database not attached/open");
        return {
            {"status",  "error"},
            {"error_code", "NO_DATABASE"},
            {"message", "Database not attached or not open"}
        };
    }

    // Security: reject path traversal and absolute paths.
    // Use lexically_normal() to normalise the path and verify it stays
    // within the allowed snapshot directory after resolution.
    if (!isSafeSnapshotId(snapshot_id) ||
        snapshot_id.find("..") != std::string::npos ||
        snapshot_id.find('%') != std::string::npos ||
        snapshot_id.find('/') != std::string::npos ||
        snapshot_id.find('\\') != std::string::npos ||
        hasWindowsDrivePrefix(snapshot_id) ||
        std::filesystem::path(snapshot_id).is_absolute()) {
        spdlog::warn("AI Safety ASL-10: rejected invalid snapshot_id='{}'", snapshot_id);
        return {
            {"status",  "error"},
            {"error_code", "INVALID_SNAPSHOT_ID"},
            {"message", "Invalid snapshot ID"}
        };
    }

    const std::string snap_base = operation_guard_
        ? operation_guard_->config().snapshot_dir
        : themis::security::themisDefaultSnapshotDir();
    const std::filesystem::path base_normal =
        std::filesystem::path(snap_base).lexically_normal();
    const std::filesystem::path snapshot_path =
        (base_normal / snapshot_id).lexically_normal();

    std::error_code ec;
    const std::filesystem::path base_abs = std::filesystem::absolute(base_normal, ec).lexically_normal();
    if (ec) {
        spdlog::warn("AI Safety ASL-10: failed to resolve snapshot base path '{}'", snap_base);
        return {
            {"status",  "error"},
            {"error_code", "INVALID_SNAPSHOT_ID"},
            {"message", "Invalid snapshot ID"}
        };
    }
    ec.clear();
    const std::filesystem::path snapshot_abs =
        std::filesystem::absolute(snapshot_path, ec).lexically_normal();
    if (ec || !isPathWithinBase(snapshot_abs, base_abs)) {
        spdlog::warn("AI Safety ASL-10: path escape attempt, snapshot_id='{}'", snapshot_id);
        return {
            {"status",  "error"},
            {"error_code", "INVALID_SNAPSHOT_ID"},
            {"message", "Invalid snapshot ID"}
        };
    }

    // Verify the resolved path stays within the snapshot directory.
    const std::string snap_str = snapshot_abs.string();

    spdlog::info("AI Safety ASL-10: restoring checkpoint snapshot_id='{}' path='{}'",
                 snapshot_id, snap_str);

    try {
        if (db_->restoreFromCheckpoint(snap_str)) {
            spdlog::info("AI Safety ASL-10: restore succeeded for snapshot_id='{}'", snapshot_id);
            logAiEvent(themis::utils::SecurityEventType::AI_ROLLBACK_EXECUTED,
                       "ai_rollback", "",
                       {{"snapshot_id", snapshot_id}, {"snapshot_path", snap_str}});
            return {
                {"status",      "success"},
                {"snapshot_id", snapshot_id},
                {"message",     "Database restored from checkpoint"}
            };
        }
        spdlog::error("AI Safety ASL-10: restoreFromCheckpoint returned false for snapshot_id='{}'",
                      snapshot_id);
        return {
            {"status",  "error"},
            {"error_code", "RESTORE_FAILED"},
            {"message", "Checkpoint restore failed"}
        };
    } catch (const std::exception& ex) {
        spdlog::error("AI Safety ASL-10: restore threw exception for snapshot_id='{}': {}",
                      snapshot_id, ex.what());
        return {
            {"status",  "error"},
            {"error_code", "RESTORE_EXCEPTION"},
            {"message", std::string("Checkpoint restore failed: ") + ex.what()}
        };
    }
}

// ============================================================================
// ASL-11: Snapshot cleanup tool
// Docs: src/security/ROADMAP.md § Phase 3 (ASL-11)
// ============================================================================

json McpServer::toolAiCleanupSnapshots(const json& /*args*/) {
    const std::string snap_dir = operation_guard_
        ? operation_guard_->config().snapshot_dir
        : themis::security::themisDefaultSnapshotDir();

    const int safe_retention_days = (snapshot_retention_days_ > 0) ? snapshot_retention_days_ : 7;
    const int safe_max_gb         = (snapshot_max_total_gb_ > 0) ? snapshot_max_total_gb_ : 100;

    themis::security::AiSnapshotCleanupJob job({
        snap_dir,
        safe_retention_days,
        static_cast<std::uint64_t>(safe_max_gb)
    });

    try {
        const int deleted = job.runCleanup();
        spdlog::info("AI Safety ASL-11: snapshot cleanup complete, deleted={}", deleted);
        logAiEvent(themis::utils::SecurityEventType::AI_CLEANUP_EXECUTED,
                   "ai_cleanup_snapshots", "",
                   {{"deleted_count", deleted}, {"snapshot_dir", snap_dir}});
        return {{"status", "success"}, {"deleted_count", deleted}};
    } catch (const std::exception& ex) {
        spdlog::error("AI Safety ASL-11: snapshot cleanup failed: {}", ex.what());
        return {
            {"status",  "error"},
            {"error_code", "CLEANUP_FAILED"},
            {"message", std::string("Snapshot cleanup failed: ") + ex.what()}
        };
    }
}

// ============================================================================
// StdioTransport Implementation
// ============================================================================

static std::mutex& stdioReadFnMutex() { static std::mutex m; return m; }
static StdioTransport::StdioReadFn& stdioReadFnStorage() {
    static StdioTransport::StdioReadFn fn;
    return fn;
}
void StdioTransport::setStdioReadFn(StdioReadFn fn) {
    std::lock_guard<std::mutex> lk(stdioReadFnMutex());
    stdioReadFnStorage() = std::move(fn);
}

StdioTransport::StdioTransport(asio::io_context& io_context, int buffer_size)
    : io_context_(io_context), buffer_size_(buffer_size), read_buffer_(buffer_size) {
}

StdioTransport::~StdioTransport() {
    stop();
}

void StdioTransport::start() {
    bool expected = false;
    if (!is_running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    spdlog::info("MCP stdio transport started");
    
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    // Start async stdin reading
    readStdin();
#else
    // PERMANENT HARDWARE FALLBACK NOTE (MCP StdioTransport — exotic platform):
    // Purpose: Allow McpServer StdioTransport to be compiled and linked on
    //   platforms other than Windows, Unix, and macOS (e.g., embedded or
    //   exotic toolchain targets).  On those platforms, async stdin reading
    //   via platform threads is not implemented; the transport runs but
    //   silently ignores all stdin input — unless a StdioReadFn is injected
    //   via StdioTransport::setStdioReadFn().
    // Activation: Compiled when none of _WIN32, __unix__, __APPLE__ are defined.
    // Production Delta: Without an injected StdioReadFn, MCP clients connected
    //   via stdio receive no responses (transport deaf).
    // Note: Inject a platform-specific reader via setStdioReadFn() at startup,
    //   or add the target platform guard to the `#if` condition above.
    {
        StdioReadFn fn;
        {
            std::lock_guard<std::mutex> lk(stdioReadFnMutex());
            fn = stdioReadFnStorage();
        }
        if (fn) {
            try { fn(); } catch (...) {}
        } else {
            spdlog::warn("MCP stdio transport: Unsupported platform, stdin reading not implemented");
        }
    }
#endif
}

void StdioTransport::stop() {
    if (!is_running_.exchange(false, std::memory_order_acq_rel)) return;
    spdlog::info("MCP stdio transport stopped");
}

void StdioTransport::send(const json& message) {
    if (!is_running_.load(std::memory_order_acquire)) return;
    writeStdout(message.dump() + "\n");
}

void StdioTransport::readStdin() {
#if defined(_WIN32)
    // Windows implementation using PeekNamedPipe for non-blocking stdin
    std::weak_ptr<StdioTransport> weak_self = weak_from_this();
    asio::post(io_context_, [weak_self]() {
        auto self = weak_self.lock();
        if (!self) return;

        HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
        if (h_stdin == INVALID_HANDLE_VALUE) {
            errors::logError(ErrorCode::ERR_MCP_STDIO_INIT_FAILED, "stdin handle");
            return;
        }

        while (self->is_running_.load(std::memory_order_acquire)) {
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
                    
                    self->partial_message_ += line;
                    
                    // Try to parse as JSON
                    try {
                        json request = json::parse(self->partial_message_);
                        
                        // Call message handler
                        if (self->message_handler_) {
                            json response = self->message_handler_(request);
                            self->send(response);
                        }
                        
                        // Clear partial message
                        self->partial_message_.clear();
                    } catch (const json::parse_error&) {
                        // Incomplete JSON, wait for more input
                    }
                } else if (!self->is_running_.load(std::memory_order_acquire)) {
                    break;
                }
            } else if (bytes_available > 0) {
                // Data available, read it
                std::string line;
                if (std::getline(std::cin, line)) {
                    self->partial_message_ += line;
                    
                    // Try to parse as JSON
                    try {
                        json request = json::parse(self->partial_message_);
                        
                        // Call message handler
                        if (self->message_handler_) {
                            json response = self->message_handler_(request);
                            self->send(response);
                        }
                        
                        // Clear partial message
                        self->partial_message_.clear();
                    } catch (const json::parse_error&) {
                        // Incomplete JSON, wait for more input
                    }
                } else {
                    // EOF on stdin
                    self->is_running_.store(false, std::memory_order_release);
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
    std::weak_ptr<StdioTransport> weak_self = weak_from_this();
    asio::post(io_context_, [weak_self]() {
        auto self = weak_self.lock();
        if (!self) return;

        while (self->is_running_.load(std::memory_order_acquire)) {
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
                    self->partial_message_ += line;
                    
                    // Try to parse as JSON
                    try {
                        json request = json::parse(self->partial_message_);
                        
                        // Call message handler
                        if (self->message_handler_) {
                            json response = self->message_handler_(request);
                            self->send(response);
                        }
                        
                        // Clear partial message
                        self->partial_message_.clear();
                    } catch (const json::parse_error&) {
                        // Incomplete JSON, wait for more input
                    }
                } else {
                    // EOF on stdin
                    self->is_running_.store(false, std::memory_order_release);
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
    bool expected = false;
    if (!is_running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    spdlog::info("MCP SSE transport started with {}ms keepalive interval", keepalive_ms_);
    
    // Start keepalive timer
    scheduleKeepalive();
}

void SseTransport::stop() {
    if (!is_running_.exchange(false, std::memory_order_acq_rel)) return;
    keepalive_timer_.cancel();
    
    // Clear all clients
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
    }
    
    spdlog::info("MCP SSE transport stopped");
}

void SseTransport::send(const json& message) {
    if (!is_running_.load(std::memory_order_acquire)) return;
    
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
    if (!is_running_.load(std::memory_order_acquire)) return;
    
    // Send SSE comment as keepalive
    std::string keepalive = ": keepalive\n\n";
    
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& [client_id, buffer] : clients_) {
        buffer += keepalive;
    }
    
    spdlog::trace("MCP SSE keepalive sent to {} clients", clients_.size());
}

void SseTransport::scheduleKeepalive() {
    if (!is_running_.load(std::memory_order_acquire)) return;
    
    keepalive_timer_.expires_after(std::chrono::milliseconds(keepalive_ms_));
    std::weak_ptr<SseTransport> weak_self = weak_from_this();
    keepalive_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
        auto self = weak_self.lock();
        if (!self) return;

        if (!ec && self->is_running_.load(std::memory_order_acquire)) {
            self->sendKeepalive();
            self->scheduleKeepalive();
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
    bool expected = false;
    if (!is_running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    spdlog::info("MCP WebSocket transport started with {}ms ping interval", ping_interval_ms_);
    
    // Start ping timer
    schedulePing();
}

void WebSocketTransport::stop() {
    if (!is_running_.exchange(false, std::memory_order_acq_rel)) return;
    ping_timer_.cancel();
    
    // Clear all sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.clear();
    }
    
    spdlog::info("MCP WebSocket transport stopped");
}

void WebSocketTransport::send(const json& message) {
    if (!is_running_.load(std::memory_order_acquire)) return;
    
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
    if (!is_running_.load(std::memory_order_acquire)) return;
    
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
    if (!is_running_.load(std::memory_order_acquire)) return;
    
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
    if (!is_running_.load(std::memory_order_acquire)) return;
    
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
    if (!is_running_.load(std::memory_order_acquire)) return;
    
    ping_timer_.expires_after(std::chrono::milliseconds(ping_interval_ms_));
    std::weak_ptr<WebSocketTransport> weak_self = weak_from_this();
    ping_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
        auto self = weak_self.lock();
        if (!self) return;

        if (!ec && self->is_running_.load(std::memory_order_acquire)) {
            self->sendPing();
            self->schedulePing();
        }
    });
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MCP

