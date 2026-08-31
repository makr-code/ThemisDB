/**
 * @file mcp_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_MCP

#include <chrono>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

// Forward-declare AIOrchestrator for the MCP ↔ Orchestrator integration bridge.
// Declared at global scope before the server namespace block to avoid the
// namespace being re-opened inside an already-open namespace.
#ifdef THEMIS_ENABLE_LLM
namespace themis::llm { class AIOrchestrator; }
#endif

// Forward-declare AiOperationGuard for the AI Safety Layer (Schichten 1 & 2).
// AI Safety Layer docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
namespace themis::security {
class AiOperationGuard;
struct GuardDecision;
}

// Forward-declare AuditLogger for the AI Session Audit Trail (ASL-12).
// Docs: docs/de/security/ai_safety/AI_SAFETY_AUDIT_TRAIL.md
// NOTE: Do NOT include audit_logger.h here; mcp_server.cpp includes it.
namespace themis::utils {
class AuditLogger;
enum class SecurityEventType : int;
}

namespace themis {
namespace server {

using json = nlohmann::json;
namespace asio = boost::asio;

// Forward declarations
class McpTransport;
class HttpServer;
class RocksDBWrapper;
class SecondaryIndexManager;
class SchemaManager;
class QueryEngine;

namespace prompt_engineering {
class PromptManager;
}

/**
 * @class McpServer
 * @brief MCP (Model Context Protocol) Server Implementation.
 *
 * Provides LLM integration for ThemisDB through the Model Context Protocol.
 * Supports multiple transports: stdio, SSE (Server-Sent Events), and WebSocket.
 *
 * Architecture:
 * - Tools: Database operations exposed as callable LLM tools
 * - Resources: Read-only context (schema, stats, metadata)
 * - Prompts: Query templates for common operations
 *
 * @see MCP_PROTOCOL_SUPPORT.md (path relative to project root: docs/apis/MCP_PROTOCOL_SUPPORT.md)
 */
class McpServer : public std::enable_shared_from_this<McpServer> {
public:
    /**
     * @brief Tool handler function type
     * @param args Tool arguments as JSON object
     * @return Tool result as JSON object
     */
    using ToolHandler = std::function<json(const json& args)>;

    /**
     * @brief Resource handler function type
     * @param uri Resource URI
     * @return Resource content as JSON object
     */
    using ResourceHandler = std::function<json(const std::string& uri)>;

    /**
     * @brief Prompt handler function type
     * @param name Prompt name
     * @param args Prompt arguments
     * @return Prompt messages as JSON array
     */
    using PromptHandler = std::function<json(const std::string& name, const json& args)>;

    /**
     * @brief MCP Server configuration
     */
    struct Config {
        std::string server_name = "ThemisDB";
        std::string server_version = "1.0.0";
        bool enable_stdio = true;    // stdio transport for Claude Desktop
        bool enable_sse = true;       // SSE transport for HTTP clients
        bool enable_websocket = true; // WebSocket transport for bidirectional
        int stdio_buffer_size = 4096;
        int sse_keepalive_ms = 30000;
        int websocket_ping_interval_ms = 30000;
    };

    /**
     * @brief Construct an MCP server with default configuration.
     * @param io_context Asio I/O context for async operations.
     */
    explicit McpServer(asio::io_context& io_context);
    /**
     * @brief Construct an MCP server with explicit configuration.
     * @param io_context Asio I/O context for async operations.
     * @param config     Server configuration controlling transports and buffers.
     */
    explicit McpServer(asio::io_context& io_context, const Config& config);
    /** @brief Destroy the server and release all transport resources. */
    ~McpServer();

    /** @brief Start all enabled transports and begin accepting requests. */
    void start();
    /** @brief Stop all transports and release associated resources. */
    void stop();
    /** @brief Return true if the server is currently running. */
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

    /**
     * @brief Register a tool that can be invoked by an LLM client.
     * @param name         Unique tool name exposed via MCP tools/list.
     * @param description  Human-readable description of the tool's purpose.
     * @param input_schema JSON Schema object describing accepted arguments.
     * @param handler      Callable invoked when the tool is called.
     */
    void registerTool(const std::string& name, const std::string& description,
                      const json& input_schema, ToolHandler handler);
    /**
     * @brief Unregister a previously registered tool by name.
     * @param name Tool name to remove.
     */
    void unregisterTool(const std::string& name);

    /**
     * @brief Register a read-only resource accessible to LLM clients.
     * @param uri         Resource URI used to address the resource.
     * @param description Human-readable description of the resource.
     * @param mime_type   MIME type of the returned content.
     * @param handler     Callable that returns the resource content for a given URI.
     */
    void registerResource(const std::string& uri, const std::string& description,
                          const std::string& mime_type, ResourceHandler handler);
    /**
     * @brief Unregister a previously registered resource by URI.
     * @param uri Resource URI to remove.
     */
    void unregisterResource(const std::string& uri);

    /**
     * @brief Register a prompt template for LLM clients.
     * @param name             Unique prompt name exposed via MCP prompts/list.
     * @param description      Human-readable description of the prompt.
     * @param arguments_schema JSON Schema describing the prompt's arguments.
     * @param handler          Callable that produces prompt messages for the given name and args.
     */
    void registerPrompt(const std::string& name, const std::string& description,
                        const json& arguments_schema, PromptHandler handler);
    /**
     * @brief Unregister a previously registered prompt by name.
     * @param name Prompt name to remove.
     */
    void unregisterPrompt(const std::string& name);

    /**
     * @brief Attach an HTTP server for SSE and WebSocket transports.
     * @param http_server Shared pointer to the HTTP server instance.
     */
    void attachHttpServer(std::shared_ptr<HttpServer> http_server);
    /**
     * @brief Attach the primary database backend for default tool handlers.
     * @param db Shared pointer to the RocksDB wrapper.
     */
    void attachDatabase(std::shared_ptr<RocksDBWrapper> db);

    /**
     * @brief Attach an AuditLogger for AI Session Audit Trail (ASL-12).
     *
     * When attached, all AI Safety Layer events (tool calls, approvals, denials,
     * rollbacks, etc.) are recorded via the audit logger.
     *
     * Docs: docs/de/security/ai_safety/AI_SAFETY_AUDIT_TRAIL.md
     *
     * @param logger Shared pointer to a fully initialised AuditLogger.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /**
     * @brief Attach an AIOrchestrator to expose mode-based LLM pipelines as MCP tools.
     *
     * When attached, two new MCP tools are registered:
     *  - **llm_orchestrate**: executes an AIOrchestrator pipeline for a given mode.
     *  - **llm_list_modes**: returns all available modes from the loaded ModePack.
     *
     * This is the primary integration point between the Model Context Protocol
     * transport layer and the YAML-configurable LLM orchestration layer.
     *
     * Only available when both THEMIS_ENABLE_MCP and THEMIS_ENABLE_LLM are set.
     *
     * @param orchestrator Shared pointer to a fully initialised AIOrchestrator.
     */
    #ifdef THEMIS_ENABLE_LLM
    void attachOrchestrator(std::shared_ptr<themis::llm::AIOrchestrator> orchestrator);
    #endif
    /** @brief Get the stdio transport instance (may be null if stdio is disabled). */
    std::shared_ptr<McpTransport> getStdioTransport() const { return stdio_transport_; }
    /** @brief Get the SSE transport instance (may be null if SSE is disabled). */
    std::shared_ptr<McpTransport> getSseTransport() const { return sse_transport_; }
    /** @brief Get the WebSocket transport instance (may be null if WebSocket is disabled). */
    std::shared_ptr<McpTransport> getWebSocketTransport() const { return ws_transport_; }

    /**
     * @brief Dispatch an incoming MCP JSON-RPC request to the appropriate handler.
     * @param request JSON-RPC request object.
     * @return JSON-RPC response object.
     */
    json handleRequest(const json& request);

private:
    // Request handlers
    json handleInitialize(const json& params);
    json handleToolsList(const json& params);
    json handleToolsCall(const json& params);
    json handleResourcesList(const json& params);
    json handleResourcesRead(const json& params);
    json handlePromptsList(const json& params);
    json handlePromptsGet(const json& params);

    // Default tool handlers
    void registerDefaultTools();
    json toolQuery(const json& args);
    json toolPutEntity(const json& args);
    json toolGetEntity(const json& args);
    json toolDeleteEntity(const json& args);
    json toolCreateIndex(const json& args);
    json toolDropIndex(const json& args);
    json toolListIndexes(const json& args);
    json toolGetSchema(const json& args);
    json toolGetStats(const json& args);

    // Error introspection tool handlers (NEW)
    json toolGetErrorInfo(const json& args);
    json toolSearchErrors(const json& args);
    json toolIntrospectDatabase(const json& args);
    std::string generateErrorAnswer(const std::string& question);

    // LLM Tool handlers (NEW)
    #ifdef THEMIS_ENABLE_LLM
    json toolLLMComplete(const json& args);
    json toolLLMEmbed(const json& args);
    json toolLLMChat(const json& args);
    json toolDatabaseQueryWithLLM(const json& args);

    // AI Orchestrator tools – mode-based LLM pipelines (ask / edit / rag / agentic / ethics …)
    json toolLLMOrchestrate(const json& args);
    json toolLLMListModes(const json& args);
    #endif

    // ── Group 1: Knowledge Graph tools (Q4 2026) ──────────────────────────
    json toolKgNeighbours(const json& args);
    json toolKgShortestPath(const json& args);
    json toolKgNodeProperties(const json& args);

    // ── Group 2: Vector / Hybrid / RAG tools (Q4 2026) ────────────────────
    json toolSemanticSearch(const json& args);
    json toolHybridSearch(const json& args);
    json toolRagRetrieve(const json& args);
    json toolVectorIndexList(const json& args);

    // ── Group 7: Schema extensions (Q4 2026) ──────────────────────────────
    json toolSchemaDiff(const json& args);
    json toolSchemaValidate(const json& args);
    json toolExplainQuery(const json& args);

    // Default resource handlers
    void registerDefaultResources();
    json resourceSchema(const std::string& uri);
    json resourceStats(const std::string& uri);
    json resourceMetadata(const std::string& uri);
    json resourceExamples(const std::string& uri);

    // Default prompt handlers
    void registerDefaultPrompts();
    json promptSimpleQuery(const std::string& name, const json& args);
    json promptComplexQuery(const std::string& name, const json& args);
    json promptEntityOperation(const std::string& name, const json& args);

    // Error handling
    json createError(int code, const std::string& message);
    json createSuccessResponse(const json& result);

private:
    asio::io_context& io_context_;
    Config config_;
    std::atomic<bool> is_running_{false};

    // Tool registry
    struct ToolInfo {
        std::string description;
        json input_schema;
        ToolHandler handler;
    };
    std::unordered_map<std::string, ToolInfo> tools_;

    // Resource registry
    struct ResourceInfo {
        std::string description;
        std::string mime_type;
        ResourceHandler handler;
    };
    std::unordered_map<std::string, ResourceInfo> resources_;

    // Prompt registry
    struct PromptInfo {
        std::string description;
        json arguments_schema;
        PromptHandler handler;
    };
    std::unordered_map<std::string, PromptInfo> prompts_;

    // Transports
    std::shared_ptr<McpTransport> stdio_transport_;
    std::shared_ptr<McpTransport> sse_transport_;
    std::shared_ptr<McpTransport> ws_transport_;

    // HTTP server reference (for SSE and WebSocket)
    std::weak_ptr<HttpServer> http_server_;

    // Database reference
    std::shared_ptr<RocksDBWrapper> db_;
    
    // Schema management
    std::shared_ptr<SecondaryIndexManager> index_mgr_;
    std::unique_ptr<SchemaManager> schema_mgr_;
    
    // Query engine for AQL execution
    std::unique_ptr<QueryEngine> query_engine_;
    
    // Prompt management for natural language queries
    std::unique_ptr<themis::prompt_engineering::PromptManager> prompt_mgr_;

    // Session state
    std::atomic<bool> initialized_{false};
    std::string client_info_;

    // AI Orchestrator reference (optional – set via attachOrchestrator())
    #ifdef THEMIS_ENABLE_LLM
    std::shared_ptr<themis::llm::AIOrchestrator> orchestrator_;
    #endif

    // ── AI Safety Layer — Schichten 1 & 2: DOG + HILG (ASL-4..6) ──────────
    // Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
    // Roadmap: src/security/ROADMAP.md § Phase 2

    /**
     * @brief In-memory record for one pending HILG approval.
     *
     * Stored in `pending_approvals_` from the moment an AI-initiated
     * DESTRUCTIVE/CRITICAL operation is classified until it either expires,
     * is approved, or is denied.
     */
    struct PendingApproval {
        std::string operation_id;       ///< UUID (matches GuardDecision::operation_id)
        std::string ai_session_id;      ///< AI session that triggered the operation
        std::string tool_name;          ///< MCP tool name
        json        operation_args;     ///< Original, unmodified args
        std::string classification;     ///< "DESTRUCTIVE" or "CRITICAL"
        json        approval_response;  ///< Pre-built requires_approval JSON
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point expires_at;
        bool        is_executed = false;
        std::string pre_snapshot_path;  ///< ASL-8: path of pre-op snapshot (empty if not taken)
    };

    /// Map: operation_id → PendingApproval entry.
    std::unordered_map<std::string, PendingApproval> pending_approvals_;
    mutable std::mutex pending_approvals_mutex_;

    /// AI Safety Layer guard (DOG).  Constructed once in the constructor.
    std::unique_ptr<themis::security::AiOperationGuard> operation_guard_;

    /// AI Session Audit Logger (ASL-12).  Optional — null if not attached.
    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;

    // ── HILG handler methods ───────────────────────────────────────────────

    /// Dispatch a write tool through the DOG + HILG pipeline.
    /// Returns a "requires_approval" or "blocked" JSON when the guard fires,
    /// std::nullopt when the operation may proceed immediately.
    std::optional<json> checkOperationGuard(
        const std::string& tool_name,
        const json&        args,
        const std::string& ai_session_id = "",
        const std::string& caller_role   = ""
    );

    /// Handle POST /v1/ai/approve/{operation_id}
    json handleAiApprove(const std::string& operation_id);

    /// Handle POST /v1/ai/deny/{operation_id}
    json handleAiDeny(const std::string& operation_id);

    /// Handle GET /v1/ai/pending-approvals
    json handleAiPendingApprovals();

    /// Handle POST /v1/ai/rollback/{snapshot_id}  (ASL-10)
    json handleAiRollback(const std::string& snapshot_id);

    /// Cleanup expired AI pre-operation snapshots (ASL-11)
    json toolAiCleanupSnapshots(const json& args);

    /// Remove expired entries from pending_approvals_.  Called on demand.
    void purgeExpiredApprovals();

    /// Log an AI Safety Layer audit event (ASL-12).
    /// No-op when audit_logger_ is null.
    void logAiEvent(
        themis::utils::SecurityEventType type,
        const std::string&               tool_name,
        const std::string&               ai_session_id,
        const nlohmann::json&            details = {}
    );

private:
    int snapshot_retention_days_ = 7;   ///< ASL-9/11: from security.yaml
    int snapshot_max_total_gb_   = 100; ///< ASL-9/11: from security.yaml
};

/**
 * @brief Abstract base class for MCP transports
 */
class McpTransport {
public:
    virtual ~McpTransport() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send(const json& message) = 0;
    
    void setMessageHandler(std::function<json(const json&)> handler) {
        message_handler_ = std::move(handler);
    }

protected:
    std::function<json(const json&)> message_handler_;
};

/**
 * @brief stdio transport for Claude Desktop integration
 */
class StdioTransport : public McpTransport, public std::enable_shared_from_this<StdioTransport> {
public:
    explicit StdioTransport(asio::io_context& io_context, int buffer_size = 4096);
    ~StdioTransport() override;

    void start() override;
    void stop() override;
    void send(const json& message) override;
    [[nodiscard]] bool isRunning() const noexcept { return is_running_.load(std::memory_order_acquire); }

    // Bridge callback for exotic/embedded platforms that lack _WIN32, __unix__,
    // and __APPLE__ (STUB #65). When set, the injected function is called from
    // start() instead of the warn-only stub path, allowing platform-specific
    // async stdin reading to be wired in without changing preprocessor guards.
    // Passing nullptr reverts to the default warn-only behaviour.
    using StdioReadFn = std::function<void()>;
    static void setStdioReadFn(StdioReadFn fn);

private:
    void readStdin();
    void writeStdout(const std::string& data);

private:
    asio::io_context& io_context_;
    int buffer_size_;
    std::vector<char> read_buffer_;
    std::string partial_message_;
    std::atomic<bool> is_running_{false};
};

/**
 * @brief SSE transport for HTTP-based clients
 */
class SseTransport : public McpTransport, public std::enable_shared_from_this<SseTransport> {
public:
    explicit SseTransport(asio::io_context& io_context, int keepalive_ms = 30000);
    ~SseTransport() override;

    void start() override;
    void stop() override;
    void send(const json& message) override;

    void addClient(const std::string& client_id);
    void removeClient(const std::string& client_id);
    std::string getClientData(const std::string& client_id);

private:
    void sendKeepalive();
    void scheduleKeepalive();

private:
    asio::io_context& io_context_;
    int keepalive_ms_;
    std::unordered_map<std::string, std::string> clients_; // client_id -> pending_data
    std::mutex clients_mutex_;
    asio::steady_timer keepalive_timer_;
    std::atomic<bool> is_running_{false};
};

/**
 * @brief WebSocket transport for bidirectional communication
 */
class WebSocketTransport : public McpTransport, public std::enable_shared_from_this<WebSocketTransport> {
public:
    explicit WebSocketTransport(asio::io_context& io_context, int ping_interval_ms = 30000);
    ~WebSocketTransport() override;

    void start() override;
    void stop() override;
    void send(const json& message) override;
    
    void sendToSession(const std::string& session_id, const json& message);

    void addSession(const std::string& session_id);
    void removeSession(const std::string& session_id);
    void handleMessage(const std::string& session_id, const std::string& message);
    std::vector<std::string> getPendingMessages(const std::string& session_id);

private:
    void sendPing();
    void schedulePing();
    
    struct SessionData {
        bool is_active;
        std::queue<std::string> pending_messages;
    };

private:
    asio::io_context& io_context_;
    int ping_interval_ms_;
    std::unordered_map<std::string, SessionData> sessions_; // session_id -> session_data
    std::mutex sessions_mutex_;
    asio::steady_timer ping_timer_;
    std::atomic<bool> is_running_{false};
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MCP
