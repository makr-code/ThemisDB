/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rpc_plugin_interface.h                             ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:54:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     416                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/plugin_interface.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <atomic>
#include <cstdint>

/**
 * @file rpc_plugin_interface.h
 * @brief RPC Plugin Interface for ThemisDB
 * 
 * This header defines the interface for RPC (Remote Procedure Call) plugins.
 * RPC plugins enable ThemisDB to support various RPC protocols like gRPC,
 * Apache Thrift, JSON-RPC, and custom binary protocols.
 * 
 * See docs/plugins/RPC_PLUGIN_ARCHITECTURE.md for detailed documentation.
 */

namespace themis {
namespace plugins {
namespace rpc {

/**
 * @brief RPC Protocol Types
 */
enum class RPCProtocol {
    GRPC,           ///< Google gRPC
    THRIFT,         ///< Apache Thrift
    JSON_RPC,       ///< JSON-RPC 2.0
    MSGPACK_RPC,    ///< MessagePack-RPC
    WIRE_PROTOCOL,  ///< ThemisDB Wire Protocol v1
    CUSTOM          ///< Custom implementations
};

/**
 * @brief Convert RPCProtocol to string
 */
inline const char* rpcProtocolToString(RPCProtocol protocol) {
    switch (protocol) {
        case RPCProtocol::GRPC: return "gRPC";
        case RPCProtocol::THRIFT: return "Thrift";
        case RPCProtocol::JSON_RPC: return "JSON-RPC";
        case RPCProtocol::MSGPACK_RPC: return "MessagePack-RPC";
        case RPCProtocol::WIRE_PROTOCOL: return "Wire Protocol";
        case RPCProtocol::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/**
 * @brief RPC Server Configuration
 */
struct RPCServerConfig {
    std::string host = "0.0.0.0";        ///< Bind address
    uint16_t port = 0;                    ///< Listen port (0 = use default)
    
    // TLS Configuration
    bool tls_enabled = false;             ///< Enable TLS/SSL
    std::string tls_cert_path;            ///< Path to server certificate
    std::string tls_key_path;             ///< Path to private key
    std::string tls_ca_cert_path;         ///< Path to CA certificate (for mutual TLS)
    
    // Authentication
    bool auth_required = true;            ///< Require authentication
    
    // Connection limits
    size_t max_connections = 1000;        ///< Maximum concurrent connections
    size_t thread_pool_size = 8;          ///< Number of worker threads
    
    // Timeouts
    uint32_t connection_timeout_ms = 30000;  ///< Connection timeout in milliseconds
    uint32_t request_timeout_ms = 60000;     ///< Request timeout in milliseconds
    
    // Default namespace
    std::string namespace_default = "default";
    
    // Rate limiting
    bool rate_limit_enabled = false;
    uint32_t rate_limit_requests_per_sec = 1000;
    uint32_t rate_limit_burst_size = 100;
    
    // Compression
    bool compression_enabled = false;
    
    // Additional configuration (protocol-specific)
    std::map<std::string, std::string> extra_config;
};

/**
 * @brief RPC Server Statistics
 */
struct RPCServerStats {
    uint64_t total_requests = 0;          ///< Total number of requests processed
    uint64_t successful_requests = 0;     ///< Number of successful requests
    uint64_t failed_requests = 0;         ///< Number of failed requests
    uint64_t active_connections = 0;      ///< Current active connections
    
    // Latency statistics (in milliseconds)
    double avg_latency_ms = 0.0;          ///< Average latency
    double p50_latency_ms = 0.0;          ///< 50th percentile (median)
    double p95_latency_ms = 0.0;          ///< 95th percentile
    double p99_latency_ms = 0.0;          ///< 99th percentile
    
    // Data transfer
    uint64_t bytes_sent = 0;              ///< Total bytes sent
    uint64_t bytes_received = 0;          ///< Total bytes received
    
    // Errors
    uint64_t auth_failures = 0;           ///< Authentication failures
    uint64_t timeout_errors = 0;          ///< Timeout errors
    uint64_t connection_errors = 0;       ///< Connection errors
    
    // Uptime
    uint64_t uptime_seconds = 0;          ///< Server uptime in seconds
};

/**
 * @brief RPC Server Interface
 * 
 * All RPC server implementations must implement this interface.
 * The RPC server is responsible for listening for incoming connections
 * and dispatching requests to the appropriate service handlers.
 */
class IRPCServer {
public:
    virtual ~IRPCServer() = default;
    
    /**
     * @brief Get RPC protocol type
     * @return Protocol type
     */
    virtual RPCProtocol getProtocol() const = 0;
    
    /**
     * @brief Initialize RPC server with configuration
     * @param config Server configuration
     * @return true if initialization was successful
     */
    virtual bool initialize(const RPCServerConfig& config) = 0;
    
    /**
     * @brief Start RPC server (non-blocking)
     * 
     * The server starts listening for connections in a background thread.
     * Returns immediately after starting.
     * 
     * @return true if server started successfully
     */
    virtual bool start() = 0;
    
    /**
     * @brief Stop RPC server gracefully
     * 
     * Stops accepting new connections and waits for existing
     * requests to complete before shutting down.
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if server is running
     * @return true if server is running
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief Get server statistics
     * @return Current server statistics
     */
    virtual RPCServerStats getStats() const = 0;
    
    /**
     * @brief Register a service implementation
     * 
     * For gRPC: Pass grpc::Service* instance
     * For Thrift: Pass shared_ptr to Thrift processor
     * For JSON-RPC: Pass method handler map
     * 
     * @param service_impl Pointer to service implementation (protocol-specific)
     */
    virtual void registerService(void* service_impl) = 0;
    
    /**
     * @brief Get server address (host:port)
     * @return Server address as string
     */
    virtual std::string getAddress() const = 0;
    
    /**
     * @brief Reset statistics
     */
    virtual void resetStats() = 0;
};

/**
 * @brief RPC Plugin Interface
 * 
 * Extends IThemisPlugin for RPC-specific functionality.
 * RPC plugins create and manage RPC servers.
 */
class IRPCPlugin : public IThemisPlugin {
public:
    /**
     * @brief Create RPC server instance
     * @return Unique pointer to RPC server
     */
    virtual std::unique_ptr<IRPCServer> createServer() = 0;
    
    /**
     * @brief Get RPC protocol supported by this plugin
     * @return RPC protocol type
     */
    virtual RPCProtocol getProtocol() const = 0;
    
    /**
     * @brief Get default port for this RPC protocol
     * @return Default port number
     */
    virtual uint16_t getDefaultPort() const = 0;
    
    /**
     * @brief Get protocol description
     * @return Human-readable protocol description
     */
    virtual const char* getProtocolDescription() const = 0;
};

/**
 * @brief RPC Method Handler
 * 
 * Generic interface for RPC method handlers.
 * Used by JSON-RPC and other flexible RPC protocols.
 */
class IRPCMethodHandler {
public:
    virtual ~IRPCMethodHandler() = default;
    
    /**
     * @brief Handle RPC method call
     * @param method Method name
     * @param params Method parameters (serialized)
     * @param response Response buffer (to be filled)
     * @return true if method was handled successfully
     */
    virtual bool handleMethod(
        const std::string& method,
        const std::vector<uint8_t>& params,
        std::vector<uint8_t>& response
    ) = 0;
};

/**
 * @brief RPC Service Registry
 * 
 * Global registry for RPC services.
 * Allows registration of service implementations that can be
 * used by multiple RPC plugins.
 */
class RPCServiceRegistry {
public:
    /**
     * @brief Register a service implementation
     * @param name Service name
     * @param impl Service implementation pointer
     */
    static void registerService(const std::string& name, void* impl);
    
    /**
     * @brief Get registered service
     * @param name Service name
     * @return Service implementation pointer or nullptr
     */
    static void* getService(const std::string& name);
    
    /**
     * @brief Unregister a service
     * @param name Service name
     */
    static void unregisterService(const std::string& name);
    
    /**
     * @brief Get singleton instance
     */
    static RPCServiceRegistry& instance();
    
private:
    std::map<std::string, void*> services_;
};

/**
 * @brief Helper macro for defining RPC plugin entry points
 * 
 * Usage:
 * ```cpp
 * class MyRPCPlugin : public IRPCPlugin { ... };
 * THEMIS_RPC_PLUGIN_IMPL(MyRPCPlugin)
 * ```
 */
#define THEMIS_RPC_PLUGIN_IMPL(PluginClass) \
    extern "C" { \
        THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() { \
            return new PluginClass(); \
        } \
        THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) { \
            delete plugin; \
        } \
    }

/**
 * @brief RPC Request Context
 * 
 * Contains metadata about the current RPC request.
 * Used for authentication, authorization, and logging.
 */
struct RPCRequestContext {
    std::string client_address;           ///< Client IP address
    std::string username;                 ///< Authenticated username
    std::string namespace_name;           ///< Namespace
    std::map<std::string, std::string> metadata;  ///< Request metadata
    uint64_t request_id = 0;              ///< Unique request ID
    uint64_t timestamp_ms = 0;            ///< Request timestamp
};

/**
 * @brief RPC Error Codes
 * 
 * Standard error codes for RPC operations.
 * Based on ThemisDB error codes and extended for RPC.
 */
enum class RPCErrorCode {
    OK = 0,
    
    // Authentication errors (1000-1099)
    AUTHENTICATION_FAILED = 1000,
    AUTHORIZATION_FAILED = 1001,
    INVALID_TOKEN = 1002,
    TOKEN_EXPIRED = 1003,
    
    // Entity errors (2000-2099)
    ENTITY_NOT_FOUND = 2000,
    ENTITY_ALREADY_EXISTS = 2001,
    VERSION_CONFLICT = 2002,
    
    // Query errors (3000-3099)
    INVALID_QUERY = 3000,
    QUERY_TIMEOUT = 3001,
    INVALID_PARAMETERS = 3002,
    
    // Transaction errors (4000-4099)
    TRANSACTION_CONFLICT = 4000,
    TRANSACTION_ABORTED = 4001,
    
    // Server errors (5000-5099)
    INTERNAL_ERROR = 5000,
    RESOURCE_EXHAUSTED = 5001,
    SERVICE_UNAVAILABLE = 5002,
    NOT_IMPLEMENTED = 5003,
    
    // RPC protocol errors (6000-6099)
    INVALID_REQUEST = 6000,
    METHOD_NOT_FOUND = 6001,
    PARSE_ERROR = 6002,
    SERIALIZATION_ERROR = 6003,
    
    // Rate limiting (7000-7099)
    RATE_LIMIT_EXCEEDED = 7000,
    CONNECTION_LIMIT_EXCEEDED = 7001
};

/**
 * @brief Convert RPC error code to string
 */
inline const char* rpcErrorCodeToString(RPCErrorCode code) {
    switch (code) {
        case RPCErrorCode::OK: return "OK";
        case RPCErrorCode::AUTHENTICATION_FAILED: return "Authentication failed";
        case RPCErrorCode::AUTHORIZATION_FAILED: return "Authorization failed";
        case RPCErrorCode::ENTITY_NOT_FOUND: return "Entity not found";
        case RPCErrorCode::ENTITY_ALREADY_EXISTS: return "Entity already exists";
        case RPCErrorCode::INVALID_QUERY: return "Invalid query";
        case RPCErrorCode::INTERNAL_ERROR: return "Internal error";
        case RPCErrorCode::SERVICE_UNAVAILABLE: return "Service unavailable";
        case RPCErrorCode::INVALID_REQUEST: return "Invalid request";
        case RPCErrorCode::METHOD_NOT_FOUND: return "Method not found";
        case RPCErrorCode::RATE_LIMIT_EXCEEDED: return "Rate limit exceeded";
        default: return "Unknown error";
    }
}

} // namespace rpc
} // namespace plugins
} // namespace themis
