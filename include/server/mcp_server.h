/**
 * @file mcp_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class McpServer : public std::enable_shared_from_this<McpServer> {
public:
    using ToolHandler = std::function<json(const json& args)>;

    using ResourceHandler = std::function<json(const std::string& uri)>;

    using PromptHandler = std::function<json(const std::string& name, const json& args)>;

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
     * @brief Mcp Server.
     * @param[in,out] io_context Input/output parameter.
     * @return Return value.
     */
    explicit McpServer(asio::io_context& io_context);
    /**
     * @brief Mcp Server.
     * @param[in,out] io_context Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit McpServer(asio::io_context& io_context, const Config& config);
    ~McpServer();

    /**
     * @brief Start.
     */
    void start();
    /**
     * @brief Stop.
     */
    void stop();
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

    /**
     * @brief Register Tool.
     * @param[in] name Input parameter.
     * @param[in] description Input parameter.
     * @param[in] input_schema Input parameter.
     * @param[in] handler Input parameter.
     */
    void registerTool(const std::string& name, const std::string& description,
                      const json& input_schema, ToolHandler handler);
    /**
     * @brief Unregister Tool.
     * @param[in] name Input parameter.
     */
    void unregisterTool(const std::string& name);

    /**
     * @brief Register Resource.
     * @param[in] uri Input parameter.
     * @param[in] description Input parameter.
     * @param[in] mime_type Input parameter.
     * @param[in] handler Input parameter.
     */
    void registerResource(const std::string& uri, const std::string& description,
                          const std::string& mime_type, ResourceHandler handler);
    /**
     * @brief Unregister Resource.
     * @param[in] uri Input parameter.
     */
    void unregisterResource(const std::string& uri);

    /**
     * @brief Register Prompt.
     * @param[in] name Input parameter.
     * @param[in] description Input parameter.
     * @param[in] arguments_schema Input parameter.
     * @param[in] handler Input parameter.
     */
    void registerPrompt(const std::string& name, const std::string& description,
                        const json& arguments_schema, PromptHandler handler);
    /**
     * @brief Unregister Prompt.
     * @param[in] name Input parameter.
     */
    void unregisterPrompt(const std::string& name);

    /**
     * @brief Attach Http Server.
     * @param[in] http_server Input parameter.
     */
    void attachHttpServer(std::shared_ptr<HttpServer> http_server);
    /**
     * @brief Attach Database.
     * @param[in] db Input parameter.
     */
    void attachDatabase(std::shared_ptr<RocksDBWrapper> db);

    /**
     * @brief Set Audit Logger.
     * @param[in] logger Input parameter.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    #ifdef THEMIS_ENABLE_LLM
    /**
     * @brief Attach Orchestrator.
     * @param[in] orchestrator Input parameter.
     */
    void attachOrchestrator(std::shared_ptr<themis::llm::AIOrchestrator> orchestrator);
    #endif
    std::shared_ptr<McpTransport> getStdioTransport() const { return stdio_transport_; }
    std::shared_ptr<McpTransport> getSseTransport() const { return sse_transport_; }
    std::shared_ptr<McpTransport> getWebSocketTransport() const { return ws_transport_; }

    /**
     * @brief Handle Request.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    json handleRequest(const json& request);

private:
    // Request handlers
    /**
     * @brief Handle Initialize.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleInitialize(const json& params);
    /**
     * @brief Handle Tools List.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleToolsList(const json& params);
    /**
     * @brief Handle Tools Call.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleToolsCall(const json& params);
    /**
     * @brief Handle Resources List.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleResourcesList(const json& params);
    /**
     * @brief Handle Resources Read.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handleResourcesRead(const json& params);
    /**
     * @brief Handle Prompts List.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handlePromptsList(const json& params);
    /**
     * @brief Handle Prompts Get.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    json handlePromptsGet(const json& params);

    // Default tool handlers
    /**
     * @brief Register Default Tools.
     */
    void registerDefaultTools();
    /**
     * @brief Tool Query.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolQuery(const json& args);
    /**
     * @brief Tool Put Entity.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolPutEntity(const json& args);
    /**
     * @brief Tool Get Entity.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolGetEntity(const json& args);
    /**
     * @brief Tool Delete Entity.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolDeleteEntity(const json& args);
    /**
     * @brief Tool Create Index.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolCreateIndex(const json& args);
    /**
     * @brief Tool Drop Index.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolDropIndex(const json& args);
    /**
     * @brief Tool List Indexes.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolListIndexes(const json& args);
    /**
     * @brief Tool Get Schema.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolGetSchema(const json& args);
    /**
     * @brief Tool Get Stats.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolGetStats(const json& args);

    /**
     * @brief Tool Get Error Info.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolGetErrorInfo(const json& args);
    /**
     * @brief Tool Search Errors.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolSearchErrors(const json& args);
    /**
     * @brief Tool Introspect Database.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolIntrospectDatabase(const json& args);
    /**
     * @brief Generate Error Answer.
     * @param[in] question Input parameter.
     * @return Return value.
     */
    std::string generateErrorAnswer(const std::string& question);

    // LLM Tool handlers (NEW)
    #ifdef THEMIS_ENABLE_LLM
    /**
     * @brief Tool LLMComplete.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolLLMComplete(const json& args);
    /**
     * @brief Tool LLMEmbed.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolLLMEmbed(const json& args);
    /**
     * @brief Tool LLMChat.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolLLMChat(const json& args);
    /**
     * @brief Tool Database Query With LLM.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolDatabaseQueryWithLLM(const json& args);

    /**
     * @brief Tool LLMOrchestrate.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolLLMOrchestrate(const json& args);
    /**
     * @brief Tool LLMList Modes.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolLLMListModes(const json& args);
    #endif

    /**
     * @brief Tool Kg Neighbours.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolKgNeighbours(const json& args);
    /**
     * @brief Tool Kg Shortest Path.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolKgShortestPath(const json& args);
    /**
     * @brief Tool Kg Node Properties.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolKgNodeProperties(const json& args);

    /**
     * @brief Tool Semantic Search.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolSemanticSearch(const json& args);
    /**
     * @brief Tool Hybrid Search.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolHybridSearch(const json& args);
    /**
     * @brief Tool Rag Retrieve.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolRagRetrieve(const json& args);
    /**
     * @brief Tool Vector Index List.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolVectorIndexList(const json& args);

    /**
     * @brief Tool Schema Diff.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolSchemaDiff(const json& args);
    /**
     * @brief Tool Schema Validate.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolSchemaValidate(const json& args);
    /**
     * @brief Tool Explain Query.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolExplainQuery(const json& args);

    // Default resource handlers
    /**
     * @brief Register Default Resources.
     */
    void registerDefaultResources();
    /**
     * @brief Resource Schema.
     * @param[in] uri Input parameter.
     * @return Return value.
     */
    json resourceSchema(const std::string& uri);
    /**
     * @brief Resource Stats.
     * @param[in] uri Input parameter.
     * @return Return value.
     */
    json resourceStats(const std::string& uri);
    /**
     * @brief Resource Metadata.
     * @param[in] uri Input parameter.
     * @return Return value.
     */
    json resourceMetadata(const std::string& uri);
    /**
     * @brief Resource Examples.
     * @param[in] uri Input parameter.
     * @return Return value.
     */
    json resourceExamples(const std::string& uri);

    // Default prompt handlers
    /**
     * @brief Register Default Prompts.
     */
    void registerDefaultPrompts();
    /**
     * @brief Prompt Simple Query.
     * @param[in] name Input parameter.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json promptSimpleQuery(const std::string& name, const json& args);
    /**
     * @brief Prompt Complex Query.
     * @param[in] name Input parameter.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json promptComplexQuery(const std::string& name, const json& args);
    /**
     * @brief Prompt Entity Operation.
     * @param[in] name Input parameter.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json promptEntityOperation(const std::string& name, const json& args);

    // Error handling
    /**
     * @brief Create Error.
     * @param[in] code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    json createError(int code, const std::string& message);
    /**
     * @brief Create Success Response.
     * @param[in] result Input parameter.
     * @return Return value.
     */
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

    std::unordered_map<std::string, PendingApproval> pending_approvals_;
    mutable std::mutex pending_approvals_mutex_;

    std::unique_ptr<themis::security::AiOperationGuard> operation_guard_;

    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;

    // ── HILG handler methods ───────────────────────────────────────────────

    std::optional<json> checkOperationGuard(
        const std::string& tool_name,
        const json&        args,
        const std::string& ai_session_id = "",
        const std::string& caller_role   = ""
    );

    /**
     * @brief Handle Ai Approve.
     * @param[in] operation_id Identifier of the operation.
     * @return Return value.
     */
    json handleAiApprove(const std::string& operation_id);

    /**
     * @brief Handle Ai Deny.
     * @param[in] operation_id Identifier of the operation.
     * @return Return value.
     */
    json handleAiDeny(const std::string& operation_id);

    /**
     * @brief Handle Ai Pending Approvals.
     * @return Return value.
     */
    json handleAiPendingApprovals();

    /**
     * @brief Handle Ai Rollback.
     * @param[in] snapshot_id Identifier of the snapshot.
     * @return Return value.
     */
    json handleAiRollback(const std::string& snapshot_id);

    /**
     * @brief Tool Ai Cleanup Snapshots.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    json toolAiCleanupSnapshots(const json& args);

    /**
     * @brief Purge Expired Approvals.
     */
    void purgeExpiredApprovals();

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

class McpTransport {
public:
    /**
     * @brief Mcp Transport.
     * @return Return value.
     */
    virtual ~McpTransport() = default;

    /**
     * @brief Start.
     */
    virtual void start() = 0;
    /**
     * @brief Stop.
     */
    virtual void stop() = 0;
    /**
     * @brief Send.
     * @param[in] message Input parameter.
     */
    virtual void send(const json& message) = 0;
    
    void setMessageHandler(std::function<json(const json&)> handler) {
        message_handler_ = std::move(handler);
    }

protected:
    std::function<json(const json&)> message_handler_;
};

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
    /**
     * @brief Set Stdio Read Fn.
     * @param[in] fn Input parameter.
     */
    static void setStdioReadFn(StdioReadFn fn);

private:
    /**
     * @brief Read Stdin.
     */
    void readStdin();
    /**
     * @brief Write Stdout.
     * @param[in] data Input parameter.
     */
    void writeStdout(const std::string& data);

private:
    asio::io_context& io_context_;
    int buffer_size_;
    std::vector<char> read_buffer_;
    std::string partial_message_;
    std::atomic<bool> is_running_{false};
};

class SseTransport : public McpTransport, public std::enable_shared_from_this<SseTransport> {
public:
    explicit SseTransport(asio::io_context& io_context, int keepalive_ms = 30000);
    ~SseTransport() override;

    void start() override;
    void stop() override;
    void send(const json& message) override;

    /**
     * @brief Add Client.
     * @param[in] client_id Identifier of the client.
     */
    void addClient(const std::string& client_id);
    /**
     * @brief Remove Client.
     * @param[in] client_id Identifier of the client.
     */
    void removeClient(const std::string& client_id);
    /**
     * @brief Get Client Data.
     * @param[in] client_id Identifier of the client.
     * @return Return value.
     */
    std::string getClientData(const std::string& client_id);

private:
    /**
     * @brief Send Keepalive.
     */
    void sendKeepalive();
    /**
     * @brief Schedule Keepalive.
     */
    void scheduleKeepalive();

private:
    asio::io_context& io_context_;
    int keepalive_ms_;
    std::unordered_map<std::string, std::string> clients_; // client_id -> pending_data
    std::mutex clients_mutex_;
    asio::steady_timer keepalive_timer_;
    std::atomic<bool> is_running_{false};
};

class WebSocketTransport : public McpTransport, public std::enable_shared_from_this<WebSocketTransport> {
public:
    explicit WebSocketTransport(asio::io_context& io_context, int ping_interval_ms = 30000);
    ~WebSocketTransport() override;

    void start() override;
    void stop() override;
    void send(const json& message) override;
    
    /**
     * @brief Send To Session.
     * @param[in] session_id Identifier of the session.
     * @param[in] message Input parameter.
     */
    void sendToSession(const std::string& session_id, const json& message);

    /**
     * @brief Add Session.
     * @param[in] session_id Identifier of the session.
     */
    void addSession(const std::string& session_id);
    /**
     * @brief Remove Session.
     * @param[in] session_id Identifier of the session.
     */
    void removeSession(const std::string& session_id);
    /**
     * @brief Handle Message.
     * @param[in] session_id Identifier of the session.
     * @param[in] message Input parameter.
     */
    void handleMessage(const std::string& session_id, const std::string& message);
    /**
     * @brief Get Pending Messages.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    std::vector<std::string> getPendingMessages(const std::string& session_id);

private:
    /**
     * @brief Send Ping.
     */
    void sendPing();
    /**
     * @brief Schedule Ping.
     */
    void schedulePing();
    
    struct SessionData {
        bool is_active = 0;
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
