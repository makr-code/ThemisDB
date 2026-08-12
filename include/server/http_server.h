/**
 * @file http_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Windows compatibility
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <optional>

#include "content/content_manager.h"
#include "content/content_processor.h"
#include "content/mime_detector.h"
#include "cache/semantic_cache.h"
#ifdef THEMIS_ENABLE_SSE
#include "server/sse_connection_manager.h"
#endif
#ifdef THEMIS_ENABLE_WEBSOCKET
#include "server/websocket_session.h"
#endif
#include "server/audit_api_handler.h"
#include "server/export_api_handler.h"
#include "server/admin_api_handler.h"
#include "server/shard_repair_api_handler.h"
#include "server/sharding_metrics_handler.h"
#include "sharding/shard_repair_engine.h"
#include "sharding/prometheus_metrics.h"
#include "server/vector_api_handler.h"
#include "server/rope_api_handler.h"
#include "server/spatial_api_handler.h"
#include "server/monitoring_api_handler.h"
#include "server/query_api_handler.h"
#include "server/continuous_query_api_handler.h"
#include "server/policy_api_handler.h"
#include "server/prompt_api_handler.h"
#include "server/graph_api_handler.h"
#include "server/index_api_handler.h"
#include "server/entity_api_handler.h"
#include "server/bpmn_api_handler.h"
#include "server/content_api_handler.h"
#include "server/changefeed_api_handler.h"
#include "cdc/consumer_group.h"
#include "server/saga_api_handler.h"
#include "server/geo_topology_api_handler.h"
#include "server/replication_topology_api_handler.h"
#include "server/cache_api_handler.h"
#include "server/cache_admin_api_handler.h"
#include "server/pii_api_handler.h"
#include "server/retention_api_handler.h"
#include "server/keys_api_handler.h"
#include "server/api_key_mgmt_handler.h"
#include "server/session_api_handler.h"
#include "server/saml_auth_provider.h"
#include "server/timeseries_api_handler.h"
#include "server/pki_api_handler.h"
#include "server/classification_api_handler.h"
#include "server/reports_api_handler.h"
#include "server/update_api_handler.h"
#include "server/ethics_api_handler.h"
#if THEMIS_ENABLE_LLM
#include "server/feedback_api_handler.h"
#else
namespace themis { namespace server { class FeedbackAPIHandler; } }
#endif
#include "server/error_api_handler.h"
#include "server/schema_api_handler.h"
#include "server/graphql_api_handler.h"
#include "server/grpc_web_proxy_handler.h"
#include "server/serverless_function_api_handler.h"

// Forward declaration for AI Safety Layer HILG approval endpoints (ASL-6).
// Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
namespace themis { namespace server { class McpServer; } }
namespace themis::performance::phase3 { class BaoOptimizer; }
namespace themis::performance { class WorkloadAdaptiveOptimizer; }
namespace themis::prompt_engineering { class FeedbackCollector; }
namespace themis::rag::learning { class ContinuousLearningOrchestrator; }
namespace themis::observability { class IProvenanceStore; }
#include "server/udf_api_handler.h"
#include "server/task_scheduler_api_handler.h"
#include "server/async_job_api_handler.h"
#include "server/maintenance_api_handler.h"
#include "metadata/statistics_collector.h"
#include "metadata/schema_constraints.h"
#include "metadata/schema_version_manager.h"
#include "metadata/index_recommender.h"
#include "metadata/schema_audit_log.h"
#include "metadata/schema_consistency_checker.h"
#include "metadata/column_lineage.h"
#include "server/transaction_api_handler.h"
#include "server/distributed_txn_api_handler.h"
#include "server/wal_api_handler.h"
#include "server/health_error_service.h"
#include "server/rate_limiter.h"
#include "server/rate_limiting_middleware.h"
#include "server/auth_middleware.h"
#include "server/request_validation_middleware.h"
#include "api/tracing_middleware.h"
#include "server/policy_engine.h"
#include "server/opa_adapter.h"
#include "server/ranger_adapter.h"
#include "server/cdn_cache_middleware.h"
#include "utils/pii_pseudonymizer.h"
#include "utils/update_checker.h"
#include "security/encryption.h"
#include "utils/input_validator.h"
#include "storage/security_signature_manager.h"
#include "content/content_fs.h"
#include "transaction/snapshot_manager.h"

namespace themis {
// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class GraphIndexManager;
class VectorIndexManager;
class TransactionManager;
class ProcessGraphManager;
class LLMInteractionStore;
class Changefeed;
class TSStore;
class ContinuousAggregateManager;
class AdaptiveIndexManager;
class PITRManager;
class TaskScheduler;
namespace query { class QueryEngine; }
using QueryEngine = query::QueryEngine;
class MVCCStore;

namespace query {
class ContinuousQueryEngine;
}

namespace prompt_engineering {
class PromptManager;
}

namespace transaction {
class BranchManager;
class MergeEngine;
}

namespace analytics {
class DiffEngine;
}

namespace server {
class DiffApiHandler;
class PITRApiHandler;
class BranchApiHandler;
class MergeApiHandler;
class SnapshotApiHandler;  // Moved here to match namespace
class MvccApiHandler;
}

namespace sharding {
class WALApplier;
class WALManager;
class ReplicationCoordinator;
class MultiPrimaryCoordinator;
class HealthMonitor;
class CollectionRedundancyManager;
class ConsistentHashRing;
class ShardRepairEngine;
class ShardTopology;
class ShardingManager;
}

namespace modules {
class ModuleLoader;
}

namespace index {
class SpatialIndexManager;
}

namespace server {

// Forward declare SSE manager so member can exist without header
class SseConnectionManager;
#ifdef THEMIS_ENABLE_HTTP3
class Http3Handler;
class Http3Session;
#endif

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// HTTP request handler function type
using RequestHandler = std::function<http::response<http::string_body>(
    const http::request<http::string_body>&)>;

/**
 * @brief Async HTTP/REST API Server for THEMIS
 * 
 * Features:
 * - Thread pool for handling requests
 * - RESTful endpoints for CRUD, Query, Graph, Vector operations
 * - JSON request/response format
 * - Connection pooling and session management
 */
class HttpServer {
public:
    /**
     * @brief Server configuration
     */
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8080;
        size_t num_threads = std::thread::hardware_concurrency();
        size_t max_request_size_mb = 10;
        size_t max_header_size_bytes = 8192; // 8 KB default max header size
        uint32_t request_timeout_ms = 30000; // 30 seconds default
        uint32_t graceful_shutdown_timeout_ms = 30000; // 30 second drain timeout
        size_t max_connections = 0; // 0 = unlimited; enforce max concurrent TCP connections
        // Feature flags
        bool feature_semantic_cache = false;
        bool feature_llm_store = false;
        bool feature_llm_query_enhancement = false; // Enterprise: Include LLM context in queries
        bool feature_cdc = false;
        bool feature_timeseries = false;
    bool feature_pii_manager = false; // PII mappings persistence (RocksDB CF + API handler)
    bool feature_update_checker = false; // GitHub update checker subsystem
        // SSE/CDC streaming config
        uint32_t sse_max_events_per_second = 0; // 0 = unlimited; server-side rate limit per connection
        // API rate limits
        uint32_t audit_rate_limit_per_minute = 100; // 0 = unlimited
        
        // TLS/SSL Configuration
        bool enable_tls = false; // Enable HTTPS (TLS)
        std::string tls_cert_path; // Server certificate path (PEM format)
        std::string tls_key_path; // Private key path (PEM format)
        std::string tls_ca_cert_path; // CA certificate for mTLS client verification (optional)
        bool tls_require_client_cert = false; // Enforce mutual TLS (mTLS)
        std::string tls_min_version = "TLSv1.3"; // Minimum TLS version (TLSv1.2 or TLSv1.3)
        std::string tls_cipher_list; // OpenSSL cipher list (empty = secure defaults)
        
        // HTTP Protocol Configuration
        bool enable_http2 = false; // Enable HTTP/2 protocol (requires TLS with ALPN)
        bool enable_http3 = false; // Enable HTTP/3 (QUIC) protocol
        bool enable_websocket = false; // Enable WebSocket protocol
        uint16_t http3_port = 0; // HTTP/3 UDP port (default: same as main port)
        uint32_t http2_max_concurrent_streams = 100; // Max concurrent streams per HTTP/2 connection
        uint32_t http2_initial_window_size = 65535; // HTTP/2 flow control window size
        uint32_t http3_max_idle_timeout_ms = 30000; // HTTP/3 connection idle timeout
        uint32_t websocket_max_message_size = 1048576; // WebSocket max message size (1MB default)
        uint32_t websocket_ping_interval_ms = 30000; // WebSocket ping interval (30s default)
        uint32_t websocket_cdc_poll_interval_ms = 500; // WebSocket CDC polling interval (500ms default)
        
        // Health/Error Service Configuration
        bool health_error_service_enabled = true; // Enable separate health/error service
        std::string health_error_service_bind_address = "127.0.0.1"; // Bind to localhost by default
        uint16_t health_error_service_port = 9090; // Default health/error service port
        
        Config() = default;
        Config(std::string h, uint16_t p, size_t threads = 0) 
            : host(std::move(h)), port(p) {
            if (threads > 0) num_threads = threads;
        }
    };

    /**
     * @brief Construct HTTP server with database access
     */
    HttpServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<sharding::WALApplier> wal_applier = nullptr,
        std::shared_ptr<sharding::WALManager> wal_manager = nullptr,
        std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator = nullptr
    );

    HttpServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<sharding::WALApplier> wal_applier,
        std::shared_ptr<sharding::WALManager> wal_manager,
        std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator,
        std::shared_ptr<sharding::MultiPrimaryCoordinator> multi_primary_coordinator = nullptr,
        std::shared_ptr<sharding::HealthMonitor> health_monitor = nullptr,
        std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager = nullptr,
        std::shared_ptr<sharding::ConsistentHashRing> hash_ring = nullptr,
        std::shared_ptr<sharding::ShardTopology> shard_topology = nullptr
    );

    /** @brief Destructor performs graceful server shutdown. */
    ~HttpServer();

    /**
     * @brief Start the HTTP server (listens and spins worker threads)
     */
    void start();

    /**
     * @brief Stop the HTTP server and join worker threads
     */
    void stop();

    /**
     * @brief Wait for server to finish (blocking)
     */
    void wait();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running_; }

    /**
     * @brief Hot-reload TLS certificate and private key without downtime (SIGHUP)
     *
     * Reloads certificate/key files from the paths stored in Config. New TLS
     * sessions will use the fresh certificate; existing sessions are unaffected.
     * Thread-safe: protected by ssl_ctx_mutex_.
     *
     * @return true if reload succeeded, false if TLS is not enabled or reload failed
     */
    bool reloadTls();

    // Test helper: expose content manager metrics (nullable)
    const themis::content::ContentManager::Metrics* contentMetrics() const {
        return content_manager_ ? &content_manager_->getMetrics() : nullptr;
    }

    /**
     * @brief Inject a ConcernsContext for lifecycle management and health probes.
     *
     * When set before calling start(), the server will:
     *  - forward the context to MonitoringApiHandler so that /health/live and
     *    /health/ready report per-concern health;
     *  - call concerns->shutdown() during stop() after all other teardown is done.
     *
     * This method is idempotent: calling it multiple times replaces the previous
     * context.  It is safe to call only from the thread that owns the server
     * (before start()).
     *
     * @param concerns Shared ownership of the ConcernsContext to use.
     */
    void setConcerns(std::shared_ptr<core::concerns::ConcernsContext> concerns) {
        concerns_ = std::move(concerns);
        // Forward to MonitoringApiHandler if it has already been constructed
        // (i.e. setConcerns() is called after the constructor ran).
        if (monitoring_api_) {
            monitoring_api_->setConcerns(concerns_);
        }
    }

    /// @return the current ConcernsContext (may be nullptr).
    std::shared_ptr<core::concerns::ConcernsContext> getConcerns() const {
        return concerns_;
    }

    /// @return the shared AuditLogger instance (may be nullptr if audit init failed).
    std::shared_ptr<themis::utils::AuditLogger> getAuditLogger() const {
        return audit_logger_;
    }

    /// @return the RequestValidationMiddleware for external schema registration (never nullptr after start()).
    RequestValidationMiddleware* getRequestValidator() {
        return request_validator_.get();
    }
    const RequestValidationMiddleware* getRequestValidator() const {
        return request_validator_.get();
    }

    /**
     * @brief Enable and configure the SAML 2.0 Service Provider.
     *
     * SAML SP endpoints (/api/v1/auth/saml/login, /acs, /slo, /metadata) are
     * only active after this method is called.  Without it every SAML endpoint
     * returns HTTP 503.
     *
     * Call this after constructing the server and before calling start().
     * It is NOT thread-safe with concurrent request handlers — do not call it
     * while the server is running.  Replacing an already-initialized SAML
     * provider at runtime is not supported.
     *
     * @param config  Full SamlAuthProvider::Config including SAMLConfig (IdP
     *                certificate, entity IDs, ACS URL), optional SLO URL, and
     *                an optional custom token factory.
     * @throws std::invalid_argument if required config fields are empty.
     * @throws std::runtime_error   if the IdP certificate cannot be parsed.
     */
    void enableSaml(const SamlAuthProvider::Config& config) {
        saml_provider_ = std::make_unique<SamlAuthProvider>(config);
    }

    /**
     * @brief Return whether SAML SP has been enabled (i.e. enableSaml() was called).
     */
    bool isSamlEnabled() const { return saml_provider_ != nullptr; }

#ifdef THEMIS_ENABLE_WEBSOCKET
    /**
     * @brief Get WebSocket manager for broadcasting
     */
    std::shared_ptr<WebSocketManager> getWebSocketManager() { return websocket_manager_; }
    std::shared_ptr<const WebSocketManager> getWebSocketManager() const { return websocket_manager_; }
#endif

    // Friend classes for protocol handlers
    friend class Http2Session;
    friend class WebSocketSession;
#ifdef THEMIS_ENABLE_HTTP3
    friend class Http3Session;
#endif

    /**
     * @brief Inject the live ShardingManager into the HTTP server.
     *
     * Must be called before start() to activate /v1/admin/shards endpoints.
     * The pointer must remain valid for the lifetime of the HttpServer.
     *
     * @param mgr Pointer to the live ShardingManager (typically the singleton).
     */
    void setShardingManager(sharding::ShardingManager* mgr) {
        sharding_manager_ = mgr;
    }

    /**
     * @brief Inject shard repair engine and wire metrics handler integration.
     * @param engine Shared shard repair engine instance.
     */
    void setShardRepairEngine(std::shared_ptr<sharding::ShardRepairEngine> engine) {
        shard_repair_engine_ = std::move(engine);
        // Lazily construct the repair REST API handler the first time a real
        // engine is injected so that auth_ is guaranteed to be set by then.
        if (shard_repair_engine_ && !shard_repair_api_) {
            shard_repair_api_ = std::make_unique<themis::server::ShardRepairApiHandler>(
                shard_repair_engine_, auth_);
        }
        // Forward engine update to an already-existing handler (e.g. re-injection).
        if (shard_repair_api_) {
            shard_repair_api_->setRepairEngine(shard_repair_engine_);
        }
        // Build (or update) the ShardingMetricsHandler so that anti-entropy
        // repair metrics are exposed on GET /metrics.  We create a fresh
        // PrometheusMetrics instance scoped to the repair engine; no SLO
        // monitor is attached by default (can be added later via a separate
        // setter if needed).
        if (shard_repair_engine_) {
            sharding::PrometheusMetrics::Config pmc;
            pmc.http_port = 0;   // standalone HTTP scrape port disabled;
            pmc.http_path = "/metrics"; // metrics are served via HttpServer
            auto repair_prom = std::make_shared<sharding::PrometheusMetrics>(pmc);
            shard_repair_engine_->setPrometheusMetrics(repair_prom);

            sharding_metrics_handler_ = std::make_shared<ShardingMetricsHandler>(
                std::move(repair_prom));
            sharding_metrics_handler_->setRepairEngine(shard_repair_engine_);

            if (monitoring_api_) {
                monitoring_api_->setShardingMetrics(sharding_metrics_handler_);
            }
        }
    }

    /// @return the injected ShardingManager (may be nullptr before injection).
    sharding::ShardingManager* getShardingManager() const {
        return sharding_manager_;
    }

    /**
     * @brief Inject the live ModuleLoader for /v1/admin/modules/{name} endpoints.
     *
     * Must be called before start() to activate module management endpoints.
     * The pointer must remain valid for the lifetime of the HttpServer.
     *
     * @param loader Pointer to the live ModuleLoader instance (or nullptr to disable).
     */
    void setModuleLoader(modules::ModuleLoader* loader) {
        module_loader_ = loader;
    }

    /// @return the injected ModuleLoader (may be nullptr before injection).
    modules::ModuleLoader* getModuleLoader() const {
        return module_loader_;
    }

    /**
     * @brief Wire a ContinuousQueryEngine and activate the CQL REST endpoints.
     *
     * Registers the engine with the internal ContinuousQueryApiHandler so that
     * the following endpoints become active:
     *   POST   /v1/queries/continuous
     *   DELETE /v1/queries/continuous/:name
     *   GET    /v1/queries/continuous
     *   GET    /v1/queries/continuous/:name/results   (SSE)
     *
     * Must be called after construction and before start().
     * Calling it multiple times replaces the previous engine.
     *
     * @param engine  Shared engine instance; passing nullptr disables the endpoints.
     */
    void setContinuousQueryEngine(
        std::shared_ptr<themis::query::ContinuousQueryEngine> engine);

    /**
     * @brief Attach an MCP server instance to enable the AI Safety Layer
     *        HILG approval endpoints (`/v1/ai/\*`).
     *
     * The pointer must remain valid for the lifetime of the HttpServer.
     * Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
     */
    void setMcpServer(std::shared_ptr<themis::server::McpServer> mcp_server);


    /**
     * @brief Registered endpoint information (dynamically assembled from config)
     */
    struct RegisteredEndpoint {
        std::string method;    // GET, POST, PUT, DELETE, PATCH, etc.
        std::string path;      // e.g. "/query", "/entities/:id"
        std::string description;
    };

    /**
     * @brief Get list of all registered API endpoints (dynamic)
     *
     * Returns dynamically constructed endpoint list based on:
     *  - Always-available core endpoints (/health, /query, /entities, etc.)
     *  - Config-enabled feature endpoints (LLM, CDC, TimeSeries, etc.)
     *  - SAML endpoints (if enableSaml() was called)
     *
     * Useful for:
     *  - Startup logs (no more hardcoding endpoint lists)
     *  - API documentation generation
     *  - Health checks / capability discovery
     *
     * @return vector of RegisteredEndpoint structs, sorted by path
     */
    std::vector<RegisteredEndpoint> getRegisteredEndpoints() const;

private:
    // Session class for handling individual connections
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, HttpServer* server, bool connection_slot_reserved = false);
        ~Session();
        void start();

    private:
        void doRead();
        void onRead(beast::error_code ec, std::size_t bytes_transferred);
        void processRequest();
        void doWrite();
        void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);
        void armReadTimer();
        void cancelReadTimer();

        tcp::socket socket_;
        HttpServer* server_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        http::response<http::string_body> response_;
        net::steady_timer read_timer_; ///< I/O timeout timer: armed before async_read and async_write
    };

    // SSL Session class for handling TLS connections
    class SslSession : public std::enable_shared_from_this<SslSession> {
    public:
        SslSession(tcp::socket socket, boost::asio::ssl::context& ssl_ctx, HttpServer* server, bool connection_slot_reserved = false);
        ~SslSession();
        void start();

    private:
        void doHandshake();
        void onHandshake(beast::error_code ec);
        void doRead();
        void onRead(beast::error_code ec, std::size_t bytes_transferred);
        void processRequest();
        void doWrite();
        void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);
        void doShutdown();
        void armReadTimer();
        void cancelReadTimer();

        beast::ssl_stream<tcp::socket> stream_;
        HttpServer* server_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        http::response<http::string_body> response_;
        net::steady_timer read_timer_; ///< I/O timeout timer: armed before async_read and async_write
    };

    // Request routing
    void setupRoutes();
    
    /**
     * @brief Route an HTTP request to the appropriate handler and return a response.
     * 
     * This is the main request dispatcher that implements the core routing logic:
     * 1. Parse method and path from request
     * 2. Check rate limits (per-client and global)
     * 3. Apply request validation middleware (JSON schema checks for body-carrying methods)
     * 4. Enforce routing-layer authorization before handler dispatch
     * 5. Dispatch to registered handler or return 404/405
     * 6. Optionally apply response transformation middleware
     * 7. Return HTTP response
     * 
     * ### Authorization Enforcement
     * All privileged routes must pass auth checks before reaching their handler.
     * Special-case routes (early-routing blocks, admin paths, metrics/reporting paths)
     * are explicitly gated with authorization checks.
     * 
     * ### Failure Responses
     * - 400 Bad Request: Malformed request body or header
     * - 401 Unauthorized: Authentication failed
     * - 403 Forbidden: Authenticated but insufficient scope
     * - 404 Not Found: No route registered for (method, path)
     * - 405 Method Not Allowed: Route exists but method not supported
     * - 429 Too Many Requests: Rate limit exceeded
     * - 500 Internal Server Error: Handler exception or internal error
     * - 503 Service Unavailable: Server overloaded or shutting down
     * 
     * @param req HTTP request with method, target, headers, and body
     * 
     * @return HTTP response with appropriate status code and body:
     *         - Status 2xx: Successfully routed and handler succeeded
     *         - Status 4xx: Client error (bad request, auth failure, rate limited, not found)
     *         - Status 5xx: Server error (handler exception, internal failure)
     * 
     * @note Thread-safe; multiple threads may call concurrently
     * @note Most expected failures are converted to HTTP error responses; std/json exceptions
     *       are handled at guarded call sites, while non-standard exceptions are not guaranteed
     * @note All authorization decisions are audit-logged (without logging sensitive request data)
     * @note Request body size is limited by max_request_size_mb in Config
     * 
     * @see AuthMiddleware::authorize() for authorization logic
     * @see RequestValidationMiddleware::validate() for request validation
     * @see TokenBucketRateLimiter::tryAcquire() for rate limiting
     */
    http::response<http::string_body> routeRequest(const http::request<http::string_body>& req);


    // Endpoint handlers
    // Note: Health, Version, Stats, Capabilities, and MetricsJson handlers have been
    // moved to MonitoringApiHandler
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);  // Old content-specific metrics (deprecated)
    http::response<http::string_body> handleConfig(const http::request<http::string_body>& req);
    // Entity handlers moved to EntityApiHandler (entity_api_)
    // Query handlers moved to QueryApiHandler
    http::response<http::string_body> handleGraphTraverse(const http::request<http::string_body>& req);
    http::response<http::string_body> handleGraphEdgeCreate(const http::request<http::string_body>& req);
    http::response<http::string_body> handleGraphEdgeDelete(const http::request<http::string_body>& req);
    
    // Vector operations - delegated to VectorApiHandler (vector_api_)
    // Declarations removed - handled by vector_api_
    
    http::response<http::string_body> handleCreateIndex(const http::request<http::string_body>& req);
    http::response<http::string_body> handleDropIndex(const http::request<http::string_body>& req);
    http::response<http::string_body> handleIndexStats(const http::request<http::string_body>& req);
    http::response<http::string_body> handleIndexRebuild(const http::request<http::string_body>& req);
    http::response<http::string_body> handleIndexReindex(const http::request<http::string_body>& req);
    
    // Admin handlers moved to AdminApiHandler (admin_api_)
    // Previously: handleAdminBackup, handleAdminRestore

    // Content API endpoints
    http::response<http::string_body> handleContentImport(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentSearch(const http::request<http::string_body>& req);
    http::response<http::string_body> handleGetContent(const http::request<http::string_body>& req);
    http::response<http::string_body> handleGetContentBlob(const http::request<http::string_body>& req);
    http::response<http::string_body> handleGetContentChunks(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentAssemble(const http::request<http::string_body>& req);
    http::response<http::string_body> handleChunkNavigation(const http::request<http::string_body>& req);
    
    // Virtual Filesystem API
    http::response<http::string_body> handleFilesystemGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleFilesystemPut(const http::request<http::string_body>& req);
    http::response<http::string_body> handleFilesystemDelete(const http::request<http::string_body>& req);
    http::response<http::string_body> handleFilesystemList(const http::request<http::string_body>& req);
    http::response<http::string_body> handleFilesystemMkdir(const http::request<http::string_body>& req);
    
    http::response<http::string_body> handleHybridSearch(const http::request<http::string_body>& req);
    http::response<http::string_body> handleFusionSearch(const http::request<http::string_body>& req);
    http::response<http::string_body> handleFulltextSearch(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentFilterSchemaGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentFilterSchemaPut(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentConfigGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentConfigPut(const http::request<http::string_body>& req);
    http::response<http::string_body> handleEdgeWeightConfigGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleEdgeWeightConfigPut(const http::request<http::string_body>& req);
    http::response<http::string_body> handleEncryptionSchemaGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleEncryptionSchemaPut(const http::request<http::string_body>& req);
    // Capabilities (Core/Enterprise) endpoint

    // Sprint A beta endpoints (feature-flagged)
    http::response<http::string_body> handleLlmInteractionPost(const http::request<http::string_body>& req);
    http::response<http::string_body> handleLlmInteractionList(const http::request<http::string_body>& req);
    http::response<http::string_body> handleLlmInteractionGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleLlmInteractionUpdateMetadata(const http::request<http::string_body>& req);

    // Sprint B: Time-Series endpoints
    // Note: Time-Series methods have been extracted to TimeSeriesApiHandler
    // See: include/server/timeseries_api_handler.h
    
    // Sprint C: Adaptive Indexing endpoints
    http::response<http::string_body> handleIndexSuggestions(const http::request<http::string_body>& req);
    http::response<http::string_body> handleIndexPatterns(const http::request<http::string_body>& req);
    http::response<http::string_body> handleIndexRecordPattern(const http::request<http::string_body>& req);
    http::response<http::string_body> handleIndexClearPatterns(const http::request<http::string_body>& req);
    
    // Audit API endpoints
    http::response<http::string_body> handleAuditQuery(const http::request<http::string_body>& req);
    http::response<http::string_body> handleAuditExportCsv(const http::request<http::string_body>& req);
    
    // Security Signatures API endpoints
    http::response<http::string_body> handleSecuritySignaturesList(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSecuritySignatureGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSecuritySignaturePost(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSecuritySignatureDelete(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSecurityVerify(const http::request<http::string_body>& req);
    
    // Content Policy Validation endpoint
    http::response<http::string_body> handleContentValidate(const http::request<http::string_body>& req);

    // ContentFS API (binary content over HTTP)
    http::response<http::string_body> handleContentFsGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentFsPut(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentFsHead(const http::request<http::string_body>& req);
    http::response<http::string_body> handleContentFsDelete(const http::request<http::string_body>& req);
    
    // SAGA API endpoints
    http::response<http::string_body> handleSagaListBatches(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSagaBatchDetail(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSagaVerifyBatch(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSagaFlush(const http::request<http::string_body>& req);

    // PII API endpoints
    http::response<http::string_body> handlePiiListMappings(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePiiCreateMapping(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePiiGetByUuid(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePiiExportCsv(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePiiDeleteByUuid(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePiiRevealByUuid(const http::request<http::string_body>& req);

    // Retention API endpoints
    http::response<http::string_body> handleRetentionListPolicies(const http::request<http::string_body>& req);
    http::response<http::string_body> handleRetentionCreatePolicy(const http::request<http::string_body>& req);
    http::response<http::string_body> handleRetentionDeletePolicy(const http::request<http::string_body>& req);
    http::response<http::string_body> handleRetentionGetHistory(const http::request<http::string_body>& req);
    http::response<http::string_body> handleRetentionGetPolicyStats(const http::request<http::string_body>& req);

    // Keys API endpoints (Skeleton)
    http::response<http::string_body> handleKeysListKeys(const http::request<http::string_body>& req);
    http::response<http::string_body> handleKeysRotateKey(const http::request<http::string_body>& req);

    // API Key Management endpoints
    http::response<http::string_body> handleApiKeyCreate(const http::request<http::string_body>& req);
    http::response<http::string_body> handleApiKeyList(const http::request<http::string_body>& req);
    http::response<http::string_body> handleApiKeyGet(const http::request<http::string_body>& req);
    http::response<http::string_body> handleApiKeyUpdate(const http::request<http::string_body>& req);
    http::response<http::string_body> handleApiKeyDelete(const http::request<http::string_body>& req);

    // Session Management endpoints
    http::response<http::string_body> handleSessionCreate(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSessionList(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSessionRevokeById(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSessionRevokeOthers(const http::request<http::string_body>& req);

    // SAML 2.0 SP endpoints
    http::response<http::string_body> handleSamlLogin(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSamlAcs(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSamlSlo(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSamlMetadata(const http::request<http::string_body>& req);

    // PKI endpoints (sign/verify)
    http::response<http::string_body> handlePkiSign(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiVerify(const http::request<http::string_body>& req);
    
    // PKI HSM, TSA, eIDAS endpoints
    http::response<http::string_body> handlePkiHsmSign(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiHsmKeys(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiTimestamp(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiTimestampVerify(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiEidasSign(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiEidasVerify(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiCertificates(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiCertificate(const http::request<http::string_body>& req);
    http::response<http::string_body> handlePkiStatus(const http::request<http::string_body>& req);

    // Classification API endpoints (Skeleton)
    http::response<http::string_body> handleClassificationListRules(const http::request<http::string_body>& req);
    http::response<http::string_body> handleClassificationTest(const http::request<http::string_body>& req);

    // Reports API endpoints (Skeleton)
    http::response<http::string_body> handleReportsCompliance(const http::request<http::string_body>& req);

    // Error API endpoints
    http::response<http::string_body> handleErrorApiList(const http::request<http::string_body>& req);
    http::response<http::string_body> handleErrorApiGetByCode(const http::request<http::string_body>& req);
    http::response<http::string_body> handleErrorApiCategories(const http::request<http::string_body>& req);
    http::response<http::string_body> handleErrorApiSearch(const http::request<http::string_body>& req);

    // Schema API endpoints
    http::response<http::string_body> handleSchemaGetFull(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaGetTables(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaGetTable(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaPut(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaPatch(const http::request<http::string_body>& req);

    // Metadata extended endpoints
    http::response<http::string_body> handleMetadataInformationSchema(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataGetStats(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataCollectStats(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataGetConstraints(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataIndexRecommendations(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataAuditLog(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataSchemaImport(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataBatchValidate(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataGetColumnLineage(const http::request<http::string_body>& req);
    http::response<http::string_body> handleMetadataRecordLineageDerivation(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaVersionHistory(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaCreateVersion(const http::request<http::string_body>& req);
    http::response<http::string_body> handleSchemaDiff(const http::request<http::string_body>& req);

    // Utility methods
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );

    // Apply governance-related headers (X-Themis-*) to all responses
    void applyGovernanceHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& res
    );

    // Authorization helper: returns optional error response if unauthorized
    std::optional<http::response<http::string_body>> requireScope(
        const http::request<http::string_body>& req,
        std::string_view scope
    );

    // Combined scope + policy authorization; resource_path is e.g. request target path
    std::optional<http::response<http::string_body>> requireAccess(
        const http::request<http::string_body>& req,
        std::string_view required_scope,
        std::string_view action,
        std::string_view resource_path
    );

    // Extract authentication context (user_id and groups) from JWT token
    // Returns empty user_id and groups if auth is disabled or token is invalid
    struct AuthContext {
        std::string user_id;
        std::vector<std::string> groups;
    };
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;

    std::string extractPathParam(const std::string& path, const std::string& prefix);

    // Lazy initialization for PIIPseudonymizer
    void ensurePIIPseudonymizer();

    // Rate limiting helper for Audit endpoints
    std::optional<http::response<http::string_body>> enforceAuditRateLimit(
        const http::request<http::string_body>& req,
        std::string_view route_key);

    void recordContinuousLearningQueryTelemetry(
        const http::request<http::string_body>& req,
        const http::response<http::string_body>& res,
        std::chrono::steady_clock::time_point request_start,
        bool is_aql);

    // Accept new connections
    void doAccept();
    void onAccept(beast::error_code ec, tcp::socket socket);

    Config config_;
    
    // Database components
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<ProcessGraphManager> process_graph_;
    std::shared_ptr<TransactionManager> tx_manager_;
    
    // Spatial Index Manager (geo MVP)
    std::shared_ptr<index::SpatialIndexManager> spatial_index_;
    // Security Signatures Manager (content integrity & verification)
    std::shared_ptr<storage::SecuritySignatureManager> security_sig_mgr_;
    // MIME Detector for content ingestion classification
    std::shared_ptr<content::MimeDetector> mime_detector_;

    // Content Manager
    std::shared_ptr<themis::content::ContentManager> content_manager_;
    // Built-in processors
    std::unique_ptr<themis::content::TextProcessor> text_processor_;
    
    // Semantic Cache (Sprint A)
    std::shared_ptr<SemanticCache> semantic_cache_;
    rocksdb::ColumnFamilyHandle* cache_cf_handle_ = nullptr;
    
    // LLM Interaction Store (Sprint A)
    std::shared_ptr<LLMInteractionStore> llm_store_;
    rocksdb::ColumnFamilyHandle* llm_cf_handle_ = nullptr;
    // Prompt Manager for managing prompt templates (in-memory or RocksDB-backed)
    std::shared_ptr<themis::prompt_engineering::PromptManager> prompt_manager_;
    rocksdb::ColumnFamilyHandle* prompt_cf_handle_ = nullptr;
    std::shared_ptr<themis::performance::phase3::BaoOptimizer> bao_optimizer_;
    std::shared_ptr<themis::performance::WorkloadAdaptiveOptimizer> workload_optimizer_;
    std::shared_ptr<themis::prompt_engineering::FeedbackCollector> live_feedback_collector_;
    std::shared_ptr<themis::rag::learning::ContinuousLearningOrchestrator>
        continuous_learning_orchestrator_;
    
    // Changefeed (Sprint A CDC)
    std::shared_ptr<Changefeed> changefeed_; // shared_ptr for SSE manager
    rocksdb::ColumnFamilyHandle* cdc_cf_handle_ = nullptr;
    // Consumer group manager for /v2/cdc/stream group-protocol sessions
    std::unique_ptr<cdc::ConsumerGroupManager> consumer_group_manager_;
    
    // Snapshot Manager (Named Snapshots feature)
    std::unique_ptr<transaction::SnapshotManager> snapshot_manager_;
    std::unique_ptr<server::SnapshotApiHandler> snapshot_api_handler_;
    
    // MVCC API Handler (per-record versioning + HLC)
    std::unique_ptr<server::MvccApiHandler> mvcc_api_handler_;
    std::shared_ptr<themis::MVCCStore>      mvcc_store_; // shared with MvccCleanupHandler
    
    // Diff Engine and API Handler (Phase 2 MVCC features)
    std::unique_ptr<analytics::DiffEngine> diff_engine_;
    std::unique_ptr<DiffApiHandler> diff_api_handler_;
    
    // PITR Manager and API Handler (Phase 3 MVCC features)
    std::unique_ptr<PITRManager> pitr_manager_;
    std::unique_ptr<server::PITRApiHandler> pitr_api_handler_;
    
    // Branch Manager and API Handler (Phase 4 MVCC features - Optional)
    std::unique_ptr<transaction::BranchManager> branch_manager_;
    std::unique_ptr<BranchApiHandler> branch_api_handler_;
    
    // Merge Engine and API Handler (Phase 5 MVCC features - 3-Way Merge)
    std::unique_ptr<transaction::MergeEngine> merge_engine_;
    std::unique_ptr<MergeApiHandler> merge_api_handler_;
    
    // SSE Connection Manager for Changefeed streaming
#ifdef THEMIS_ENABLE_SSE
    std::unique_ptr<SseConnectionManager> sse_manager_;
#endif
    
#ifdef THEMIS_ENABLE_WEBSOCKET
    // WebSocket Connection Manager
    std::shared_ptr<WebSocketManager> websocket_manager_;
#endif
    
    // Time-Series Store (Sprint B)
    std::shared_ptr<TSStore> timeseries_;
    rocksdb::ColumnFamilyHandle* ts_cf_handle_ = nullptr;
    std::shared_ptr<ContinuousAggregateManager> ts_agg_manager_;
    // Governance Policy Engine
    std::unique_ptr<themis::PolicyEngine> policy_engine_;
    std::unique_ptr<themis::OpaAdapter> opa_adapter_;
    std::unique_ptr<themis::server::RangerClient> ranger_client_;
    
    // Audit Logger
    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;
    // Field encryption for PII mappings
    std::shared_ptr<themis::FieldEncryption> field_encryption_;
    // Key provider for hierarchical key management (DEK, Group-DEKs, Field-Keys)
    std::shared_ptr<themis::KeyProvider> key_provider_;
    // PII Pseudonymizer (for reveal/erase operations)
    std::shared_ptr<themis::utils::PIIPseudonymizer> pii_pseudonymizer_;
    std::mutex pii_init_mutex_; // For lazy initialization thread-safety
    
    // SAGA Logger
    std::shared_ptr<themis::utils::SAGALogger> saga_logger_;
    
    // Audit API Handler
    std::unique_ptr<themis::server::AuditApiHandler> audit_api_;

    // Export API Handler (JSONL LLM export — EXP-001)
    std::unique_ptr<themis::server::ExportApiHandler> export_api_;

    // Admin API Handler
    std::unique_ptr<themis::server::AdminApiHandler> admin_api_;
    
    // Vector API Handler
    std::unique_ptr<themis::server::VectorApiHandler> vector_api_;
    
    // RoPE API Handler
    std::unique_ptr<themis::server::RopeApiHandler> rope_api_;
    
    // Spatial API Handler
    std::unique_ptr<themis::server::SpatialApiHandler> spatial_api_;

    // Geo Topology API Handler
    std::unique_ptr<themis::server::GeoTopologyApiHandler> geo_topology_api_;

    // Replication Topology API Handler (web UI visualizer)
    std::unique_ptr<themis::server::ReplicationTopologyApiHandler> replication_topology_api_;
    
    // Monitoring API Handler
    std::unique_ptr<themis::server::MonitoringApiHandler> monitoring_api_;
    std::unique_ptr<themis::server::ShardRepairApiHandler> shard_repair_api_;
    std::shared_ptr<themis::server::ShardingMetricsHandler> sharding_metrics_handler_;
    // Shared Alertmanager instance – created during monitoring init, reused for
    // TaskScheduler SLA-breach alerts and Cache SLO monitor.
    std::shared_ptr<observability::DefaultAlertmanager> alertmanager_;
    // Shared persistent provenance store used by observability export endpoints.
    std::shared_ptr<observability::IProvenanceStore> provenance_store_;
    // Cross-cutting concerns (lifecycle hooks + health probes); optional.
    std::shared_ptr<core::concerns::ConcernsContext> concerns_;
    // Query API Handler
    std::unique_ptr<themis::server::QueryApiHandler> query_api_;
    // Continuous Query API Handler (CQL Phase 8 REST/SSE endpoints)
    std::unique_ptr<themis::server::ContinuousQueryApiHandler> continuous_query_api_;
    std::shared_ptr<themis::query::ContinuousQueryEngine> continuous_query_engine_;
    // MCP server reference for AI Safety Layer HILG endpoints (ASL-6)
    std::shared_ptr<themis::server::McpServer> mcp_server_;
    // Policy API Handler
    std::unique_ptr<themis::server::PolicyApiHandler> policy_api_;
    // Prompt API Handler
    std::unique_ptr<themis::server::PromptApiHandler> prompt_api_;
    // Graph API Handler
    std::unique_ptr<themis::server::GraphApiHandler> graph_api_;
    // Index API Handler
    std::unique_ptr<themis::server::IndexApiHandler> index_api_;
    // Entity API Handler
    std::unique_ptr<themis::server::EntityApiHandler> entity_api_;
    
    // BPMN API Handler
    std::unique_ptr<themis::server::BpmnApiHandler> bpmn_api_;
    
    // Content API Handler
    std::unique_ptr<themis::server::ContentApiHandler> content_api_;
    
    // Changefeed API Handler
    std::unique_ptr<themis::server::ChangefeedApiHandler> changefeed_api_;
    
    // SAGA API Handler
    std::unique_ptr<themis::server::SAGAApiHandler> saga_api_;

    // Cache API Handler
    std::unique_ptr<themis::server::CacheApiHandler> cache_api_;

    // Cache Admin API Handler (Phase 3: Admin API for cache operations)
    std::shared_ptr<AdaptiveQueryCache> adaptive_query_cache_;
    std::unique_ptr<themis::server::CacheAdminApiHandler> cache_admin_api_;
    
    // TimeSeries API Handler
    std::unique_ptr<themis::server::TimeSeriesApiHandler> timeseries_api_;

    // PII API Handler
    std::unique_ptr<themis::server::PIIApiHandler> pii_api_;
    rocksdb::ColumnFamilyHandle* pii_cf_handle_ = nullptr;
    
    // Retention API Handler
    std::unique_ptr<themis::server::RetentionApiHandler> retention_api_;
    
    // Keys API Handler (Skeleton)
    std::unique_ptr<themis::server::KeysApiHandler> keys_api_;
    // API Key Management Handler
    std::unique_ptr<themis::server::ApiKeyMgmtHandler> api_key_mgmt_;
    // Session Management Handler
    std::shared_ptr<themis::auth::SessionManager> session_manager_;
    std::unique_ptr<themis::server::SessionApiHandler> session_api_;
    // SAML 2.0 SP Handler
    std::unique_ptr<themis::server::SamlAuthProvider> saml_provider_;
    // PKI API Handler
    std::unique_ptr<themis::server::PkiApiHandler> pki_api_;
    
    // Classification API Handler (Skeleton)
    std::unique_ptr<themis::server::ClassificationApiHandler> classification_api_;
    
    // Reports API Handler (Skeleton)
    std::unique_ptr<themis::server::ReportsApiHandler> reports_api_;
    
    // Transaction API Handler
    std::unique_ptr<themis::server::TransactionApiHandler> transaction_api_;
    
    // Distributed (cross-shard) Transaction API Handler
    std::unique_ptr<themis::server::DistributedTxnApiHandler> distributed_txn_api_;
    
    // WAL API Handler
    std::unique_ptr<themis::server::WALApiHandler> wal_api_;
    
    // Update API Handler
    std::unique_ptr<themis::server::UpdateApiHandler> update_api_;
    std::shared_ptr<themis::utils::UpdateChecker> update_checker_;
    
    // Feedback API Handler
    std::unique_ptr<themis::server::FeedbackAPIHandler> feedback_api_handler_;
    
    // Error API Handler
    std::unique_ptr<themis::server::ErrorApiHandler> error_api_handler_;
    
    // Ethics AI API Handler (ethical decision-making and evaluation)
    std::unique_ptr<QueryEngine>                         ethics_query_engine_;
    std::unique_ptr<themis::server::EthicsApiHandler>   ethics_api_;
    
    // Health/Error Service (separate port)
    std::unique_ptr<themis::server::HealthErrorService> health_error_service_;
    
    // Schema API Handler
    std::unique_ptr<themis::server::SchemaApiHandler> schema_api_handler_;
    std::unique_ptr<SchemaManager> schema_manager_;

    // GraphQL API Handler
    // IMPORTANT: graphql_query_engine_ must be declared BEFORE graphql_api_handler_
    // so that it is destroyed AFTER the handler (C++ destroys in reverse declaration
    // order).  The handler holds a raw pointer to the engine; if the engine were
    // destroyed first the handler's destructor could dereference freed memory.
    std::unique_ptr<QueryEngine> graphql_query_engine_; ///< AQL engine for GraphQL resolvers
    std::unique_ptr<themis::server::GraphQLApiHandler> graphql_api_handler_;

    // gRPC-Web proxy – translates browser gRPC-Web requests to native gRPC
    std::unique_ptr<themis::server::GrpcWebProxyHandler> grpc_web_proxy_;

    // Serverless function hosting – in-process user function registry + executor
    std::unique_ptr<themis::server::ServerlessFunctionApiHandler> serverless_fn_handler_;

    // UDF registration API – AQL-callable user-defined functions
    std::unique_ptr<themis::server::UdfApiHandler> udf_api_handler_;

    // Task Scheduler API – manage and monitor scheduled tasks
    std::unique_ptr<QueryEngine> task_scheduler_engine_;   // QueryEngine owned by the scheduler subsystem
    std::unique_ptr<themis::TaskScheduler> task_scheduler_;
    std::unique_ptr<themis::server::TaskSchedulerApiHandler> task_scheduler_api_;

    // Database Maintenance Orchestrator – central coordinator for all maintenance
    std::unique_ptr<themis::maintenance::DatabaseMaintenanceOrchestrator> maintenance_orchestrator_;
    std::unique_ptr<themis::server::MaintenanceApiHandler> maintenance_api_;

    // Async job API – long-running AQL query submission and polling
    std::unique_ptr<themis::server::AsyncJobApiHandler> async_job_api_;

    // Metadata sub-components owned alongside SchemaApiHandler
    std::unique_ptr<StatisticsCollector>      stats_collector_;
    std::unique_ptr<SchemaConstraints>        schema_constraints_;
    std::unique_ptr<SchemaVersionManager>     schema_version_mgr_;
    std::unique_ptr<themis::metadata::IndexRecommender>       index_recommender_;
    std::unique_ptr<SchemaAuditLog>           schema_audit_log_;
    std::unique_ptr<SchemaConsistencyChecker> schema_consistency_checker_;
    std::unique_ptr<themis::metadata::ColumnLineageTracker> column_lineage_tracker_;
    
    // Adaptive Index Manager (Sprint C)
    std::shared_ptr<AdaptiveIndexManager> adaptive_index_;

    // WAL replication components (optional)
    std::shared_ptr<sharding::WALApplier> wal_applier_;
    std::shared_ptr<sharding::WALManager> wal_manager_;
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator_;
    std::shared_ptr<sharding::MultiPrimaryCoordinator> multi_primary_coordinator_;
    std::shared_ptr<sharding::HealthMonitor> health_monitor_;
    /**
     * @brief Construct HTTP server with extended sharding coordination dependencies.
     */
    std::string wal_shared_secret_;
    std::string wal_hmac_secret_;

    // Live ShardingManager (injected via setShardingManager before start())
    sharding::ShardingManager* sharding_manager_{nullptr};
    std::shared_ptr<sharding::ShardRepairEngine> shard_repair_engine_;

    // Live ModuleLoader (injected via setModuleLoader before start())
    modules::ModuleLoader* module_loader_{nullptr};

    // RAID redundancy components (optional)
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager_;
    std::shared_ptr<sharding::ConsistentHashRing> hash_ring_;
    std::shared_ptr<sharding::ShardTopology> shard_topology_;

    // Authorization middleware
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    // Rate Limiter for DoS protection
    std::unique_ptr<RateLimiter> rate_limiter_;

    // Rate limiting middleware with per-client token bucket (per-endpoint configurable)
    std::unique_ptr<RateLimitingMiddleware> rate_limiting_middleware_;

    // Request correlation ID middleware: extracts/generates X-Correlation-ID and
    // propagates it through all log lines for the duration of each request.
    std::unique_ptr<themis::api::TracingMiddleware> tracing_middleware_;

    // Request body validation (JSON Schema per endpoint)
    std::unique_ptr<RequestValidationMiddleware> request_validator_;

    // CDN / edge-cache cache-control header middleware
    CdnCacheMiddleware cdn_cache_middleware_;

    // Input validation & sanitization
    std::unique_ptr<themis::utils::InputValidator> validator_;

    // Networking
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::unique_ptr<boost::asio::ssl::context> ssl_ctx_; // SSL context for TLS connections
    mutable std::mutex ssl_ctx_mutex_; // Protects ssl_ctx_ during hot-reload

#ifdef THEMIS_ENABLE_HTTP3
    std::shared_ptr<Http3Handler> http3_handler_; // HTTP/3 QUIC handler (UDP)
#endif
    
    // Thread pool
    std::vector<std::thread> threads_;
    std::atomic<bool> running_{false};
    
    // Metrics
    std::atomic<uint64_t> request_count_{0};
    std::atomic<uint64_t> error_count_{0};
    std::atomic<uint64_t> active_requests_{0}; // In-flight request counter for graceful shutdown
    std::atomic<uint64_t> active_connections_{0}; // Open TCP connections
    std::chrono::steady_clock::time_point start_time_;

    // Audit rate limiting state
    struct RateState { uint64_t window_start_ms{0}; uint32_t count{0}; };
    std::mutex audit_rate_mutex_;
    std::unordered_map<std::string, RateState> audit_rate_buckets_;
    uint32_t audit_rate_limit_per_minute_{100};

    // Data race prevention mutexes for shared member variables
    std::mutex api_handlers_mutex_;           // Protects all API handlers: monitoring_api_, cache_api_, cache_admin_api_, ethics_api_, graph_api_, vector_api_, prompt_api_
    std::mutex storage_mutex_;                // Protects storage_ access
    std::mutex registry_mutex_;               // Protects registry_ access
    std::mutex continuous_learning_orchestrator_mutex_;  // Protects continuous_learning_orchestrator_
    std::mutex vector_index_mutex_;           // Protects vector_index_
    std::mutex policy_engine_mutex_;          // Protects policy_engine_
    std::mutex voice_assistant_mutex_;        // Protects voice_assistant_
    std::mutex inference_engine_mutex_;       // Protects inference_engine_
    std::mutex graph_index_mutex_;            // Protects graph_index_
    std::mutex rate_limiting_middleware_mutex_;  // Protects rate_limiting_middleware_
    std::mutex tracing_middleware_mutex_;     // Protects tracing_middleware_
    std::mutex request_validator_mutex_;      // Protects request_validator_ access
    std::mutex max_body_bytes_mutex_;         // Protects max_body_bytes_ access

    // Hot-reloadable config shadows — written via POST /config (on a worker thread),
    // read concurrently by other worker threads.  Atomic to prevent data races.
    std::atomic<uint32_t> request_timeout_ms_live_{30000};
    std::atomic<bool>     feature_semantic_cache_live_{false};
    std::atomic<bool>     feature_llm_store_live_{false};
    std::atomic<bool>     feature_cdc_live_{false};
    std::atomic<bool>     feature_timeseries_live_{false};
    
    // Latency histogram buckets (in microseconds): 100us, 500us, 1ms, 5ms, 10ms, 50ms, 100ms, 500ms, 1s, 5s, 10s+
    std::atomic<uint64_t> latency_bucket_100us_{0};
    std::atomic<uint64_t> latency_bucket_500us_{0};
    std::atomic<uint64_t> latency_bucket_1ms_{0};
    std::atomic<uint64_t> latency_bucket_5ms_{0};
    std::atomic<uint64_t> latency_bucket_10ms_{0};
    std::atomic<uint64_t> latency_bucket_50ms_{0};
    std::atomic<uint64_t> latency_bucket_100ms_{0};
    std::atomic<uint64_t> latency_bucket_500ms_{0};
    std::atomic<uint64_t> latency_bucket_1s_{0};
    std::atomic<uint64_t> latency_bucket_5s_{0};
    std::atomic<uint64_t> latency_bucket_inf_{0};
    std::atomic<uint64_t> latency_sum_us_{0}; // Total latency in microseconds
    
    // Helper to record latency
    void recordLatency(std::chrono::microseconds duration);
    
    // Extract client IP from request
    std::string extractClientIP(const http::request<http::string_body>& req) const;
    
    // Check rate limit and return error response if exceeded
    std::optional<http::response<http::string_body>> checkRateLimit(
        const http::request<http::string_body>& req
    );

    // Preflight response for CORS (OPTIONS)
    http::response<http::string_body> makePreflightResponse(
        const http::request<http::string_body>& req
    );

    // CORS configuration (loaded from environment at startup)
    bool cors_allow_all_{false};
    bool cors_allow_credentials_{false};
    std::vector<std::string> cors_allowed_origins_{}; // exact match list
    std::string cors_allowed_methods_{"GET,POST,PUT,DELETE,OPTIONS"};
    std::string cors_allowed_headers_{"Authorization,Content-Type,X-Requested-With"};

    // Input validation: hard limit for request body
    size_t max_body_bytes_{10 * 1024 * 1024}; // 10 MB default

    // Page Fetch (Cursor) Histogram in Millisekunden: 1,5,10,25,50,100,250,500,1000,5000,+Inf
    std::atomic<uint64_t> page_bucket_1ms_{0};
    std::atomic<uint64_t> page_bucket_5ms_{0};
    std::atomic<uint64_t> page_bucket_10ms_{0};
    std::atomic<uint64_t> page_bucket_25ms_{0};
    std::atomic<uint64_t> page_bucket_50ms_{0};
    std::atomic<uint64_t> page_bucket_100ms_{0};
    std::atomic<uint64_t> page_bucket_250ms_{0};
    std::atomic<uint64_t> page_bucket_500ms_{0};
    std::atomic<uint64_t> page_bucket_1000ms_{0};
    std::atomic<uint64_t> page_bucket_5000ms_{0};
    std::atomic<uint64_t> page_bucket_inf_{0};
    std::atomic<uint64_t> page_sum_ms_{0};
    std::atomic<uint64_t> page_count_{0};
    
    void recordPageFetch(std::chrono::milliseconds duration_ms);

    // ContentFS instance
    std::unique_ptr<themis::ContentFS> content_fs_;
};

} // namespace server
} // namespace themis
