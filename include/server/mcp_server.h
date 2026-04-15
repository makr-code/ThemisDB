/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mcp_server.h                                       ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     395                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 39e499706c  2026-02-23  fix: code-audit – namespace corruption, wildcard false-po... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef THEMIS_ENABLE_MCP

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

// Forward-declare AIOrchestrator for the MCP ↔ Orchestrator integration bridge.
// Declared at global scope before the server namespace block to avoid the
// namespace being re-opened inside an already-open namespace.
#ifdef THEMIS_ENABLE_LLM
namespace themis::llm { class AIOrchestrator; }
#endif

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
 * @brief MCP (Model Context Protocol) Server Implementation
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

    explicit McpServer(asio::io_context& io_context);
    explicit McpServer(asio::io_context& io_context, const Config& config);
    ~McpServer();

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return is_running_; }

    // Tool registration
    void registerTool(const std::string& name, const std::string& description,
                      const json& input_schema, ToolHandler handler);
    void unregisterTool(const std::string& name);

    // Resource registration
    void registerResource(const std::string& uri, const std::string& description,
                          const std::string& mime_type, ResourceHandler handler);
    void unregisterResource(const std::string& uri);

    // Prompt registration
    void registerPrompt(const std::string& name, const std::string& description,
                        const json& arguments_schema, PromptHandler handler);
    void unregisterPrompt(const std::string& name);

    // Transport management
    void attachHttpServer(std::shared_ptr<HttpServer> http_server);
    void attachDatabase(std::shared_ptr<RocksDBWrapper> db);

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
    std::shared_ptr<McpTransport> getStdioTransport() const { return stdio_transport_; }
    std::shared_ptr<McpTransport> getSseTransport() const { return sse_transport_; }
    std::shared_ptr<McpTransport> getWebSocketTransport() const { return ws_transport_; }

    // Request handling
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
    bool is_running_ = false;

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
    bool initialized_ = false;
    std::string client_info_;

    // AI Orchestrator reference (optional – set via attachOrchestrator())
    #ifdef THEMIS_ENABLE_LLM
    std::shared_ptr<themis::llm::AIOrchestrator> orchestrator_;
    #endif
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
class StdioTransport : public McpTransport {
public:
    explicit StdioTransport(asio::io_context& io_context, int buffer_size = 4096);
    ~StdioTransport() override;

    void start() override;
    void stop() override;
    void send(const json& message) override;

private:
    void readStdin();
    void writeStdout(const std::string& data);

private:
    asio::io_context& io_context_;
    int buffer_size_;
    std::vector<char> read_buffer_;
    std::string partial_message_;
    bool is_running_ = false;
};

/**
 * @brief SSE transport for HTTP-based clients
 */
class SseTransport : public McpTransport {
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
    bool is_running_ = false;
};

/**
 * @brief WebSocket transport for bidirectional communication
 */
class WebSocketTransport : public McpTransport {
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
    bool is_running_ = false;
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MCP
