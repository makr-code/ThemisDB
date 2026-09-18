/**
 * @file http_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
// Execution module — SLA-aware query scheduler and work-stealing thread pool.
// Compiled unconditionally (headers are header-only types); initialized in
// HttpServer constructor when THEMIS_EXECUTION_MODULE is ON.
#include "execution/query_scheduler.h"
#include "execution/thread_pool_manager.h"
// New production-consumer route handler headers.
// Each is gated by the corresponding feature/plugin flag at instantiation time.
#include "server/ai_plugin_api_handler.h"
#include "server/scraper_plugin_api_handler.h"
#ifdef THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED
#include "server/encrypted_storage_api_handler.h"
#endif
#ifdef THEMIS_CHAOS_ADMIN
#  include "server/chaos_admin_api_handler.h"
#endif

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

class HttpServer {
public:
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
            if (threads > 0) {
              num_threads = threads;
            }
        }
    };

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

    ~HttpServer();

    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Stop.
     */
    void stop();

    /**
     * @brief Wait.
     */
    void wait();

    bool isRunning() const { return running_; }

    /**
     * @brief Reload Tls.
     * @return True when the operation succeeds.
     */
    bool reloadTls();

    // Test helper: expose content manager metrics (nullable)
    const themis::content::ContentManager::Metrics* contentMetrics() const {
        return content_manager_ ? &content_manager_->getMetrics() : nullptr;
    }

    /**
     * @brief Set Concerns.
     * @param[in] concerns Input parameter.
     * @details Calls: std::move().
     */
    void setConcerns(std::shared_ptr<core::concerns::ConcernsContext> concerns) {
        concerns_ = std::move(concerns);
        // Forward to MonitoringApiHandler if it has already been constructed
        // (i.e. setConcerns() is called after the constructor ran).
        if (monitoring_api_) {
            monitoring_api_->setConcerns(concerns_);
        }
    }

    std::shared_ptr<core::concerns::ConcernsContext> getConcerns() const {
        return concerns_;
    }

    std::shared_ptr<themis::utils::AuditLogger> getAuditLogger() const {
        return audit_logger_;
    }

    /**
     * @brief Get Request Validator.
     * @return Pointer to the result.
     * @details Calls: get().
     */
    RequestValidationMiddleware* getRequestValidator() {
        return request_validator_.get();
    }
    const RequestValidationMiddleware* getRequestValidator() const {
        return request_validator_.get();
    }

    /**
     * @brief Enable Saml.
     * @param[in] config Input parameter.
     * @details Implements enableSaml without additional internal calls.
     */
    void enableSaml(const SamlAuthProvider::Config& config) {
        saml_provider_ = std::make_unique<SamlAuthProvider>(config);
    }

    bool isSamlEnabled() const { return saml_provider_ != nullptr; }

#ifdef THEMIS_ENABLE_WEBSOCKET
    /**
     * @brief Get Web Socket Manager.
     * @return Return value.
     * @details Implements getWebSocketManager without additional internal calls.
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
     * @brief Set Sharding Manager.
     * @param[in,out] mgr Input/output parameter.
     * @details Implements setShardingManager without additional internal calls.
     */
    void setShardingManager(sharding::ShardingManager* mgr) {
        sharding_manager_ = mgr;
    }

    /**
     * @brief Set Shard Repair Engine.
     * @param[in] engine Input parameter.
     * @details Calls: std::move(), setRepairEngine(), setPrometheusMetrics(), setShardingMetrics().
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

    sharding::ShardingManager* getShardingManager() const {
        return sharding_manager_;
    }

    /**
     * @brief Set Module Loader.
     * @param[in,out] loader Input/output parameter.
     * @details Implements setModuleLoader without additional internal calls.
     */
    void setModuleLoader(modules::ModuleLoader* loader) {
        module_loader_ = loader;
    }

    modules::ModuleLoader* getModuleLoader() const {
        return module_loader_;
    }

    /**
     * @brief Set Continuous Query Engine.
     * @param[in] engine Input parameter.
     */
    void setContinuousQueryEngine(
        std::shared_ptr<themis::query::ContinuousQueryEngine> engine);

    /**
     * @brief Set Mcp Server.
     * @param[in] mcp_server Input parameter.
     */
    void setMcpServer(std::shared_ptr<themis::server::McpServer> mcp_server);


    struct RegisteredEndpoint {
        std::string method;    // GET, POST, PUT, DELETE, PATCH, etc.
        std::string path;      // e.g. "/query", "/entities/:id"
        std::string description;
    };

    /**
     * @brief Get Registered Endpoints.
     * @return Return value.
     */
    std::vector<RegisteredEndpoint> getRegisteredEndpoints() const;

private:
    // Session class for handling individual connections
    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, HttpServer* server, bool connection_slot_reserved = false);
        ~Session();
        /**
         * @brief Start.
         */
        void start();

    private:
        /**
         * @brief Do Read.
         */
        void doRead();
        /**
         * @brief On Read.
         * @param[in] ec Input parameter.
         * @param[in] bytes_transferred Input parameter.
         */
        void onRead(beast::error_code ec, std::size_t bytes_transferred);
        /**
         * @brief Process Request.
         */
        void processRequest();
        /**
         * @brief Do Write.
         */
        void doWrite();
        /**
         * @brief On Write.
         * @param[in] close Input parameter.
         * @param[in] ec Input parameter.
         * @param[in] bytes_transferred Input parameter.
         */
        void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);
        /**
         * @brief Arm Read Timer.
         */
        void armReadTimer();
        /**
         * @brief Cancel Read Timer.
         */
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
        /**
         * @brief Start.
         */
        void start();

    private:
        /**
         * @brief Do Handshake.
         */
        void doHandshake();
        /**
         * @brief On Handshake.
         * @param[in] ec Input parameter.
         */
        void onHandshake(beast::error_code ec);
        /**
         * @brief Do Read.
         */
        void doRead();
        /**
         * @brief On Read.
         * @param[in] ec Input parameter.
         * @param[in] bytes_transferred Input parameter.
         */
        void onRead(beast::error_code ec, std::size_t bytes_transferred);
        /**
         * @brief Process Request.
         */
        void processRequest();
        /**
         * @brief Do Write.
         */
        void doWrite();
        /**
         * @brief On Write.
         * @param[in] close Input parameter.
         * @param[in] ec Input parameter.
         * @param[in] bytes_transferred Input parameter.
         */
        void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);
        /**
         * @brief Do Shutdown.
         */
        void doShutdown();
        /**
         * @brief Arm Read Timer.
         */
        void armReadTimer();
        /**
         * @brief Cancel Read Timer.
         */
        void cancelReadTimer();

        beast::ssl_stream<tcp::socket> stream_;
        HttpServer* server_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> request_;
        http::response<http::string_body> response_;
        net::steady_timer read_timer_; ///< I/O timeout timer: armed before async_read and async_write
    };

    // Request routing
    /**
     * @brief Setup Routes.
     */
    void setupRoutes();
    
    /**
     * @brief Route Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> routeRequest(const http::request<http::string_body>& req);


    /**
     * @brief Handle Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);  // Old content-specific metrics (deprecated)
    /**
     * @brief Handle Config.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleConfig(const http::request<http::string_body>& req);
    /**
     * @brief Handle Graph Traverse.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGraphTraverse(const http::request<http::string_body>& req);
    /**
     * @brief Handle Graph Edge Create.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGraphEdgeCreate(const http::request<http::string_body>& req);
    /**
     * @brief Handle Graph Edge Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGraphEdgeDelete(const http::request<http::string_body>& req);
    
    
    /**
     * @brief Handle Create Index.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateIndex(const http::request<http::string_body>& req);
    /**
     * @brief Handle Drop Index.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDropIndex(const http::request<http::string_body>& req);
    /**
     * @brief Handle Index Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexStats(const http::request<http::string_body>& req);
    /**
     * @brief Handle Index Rebuild.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexRebuild(const http::request<http::string_body>& req);
    /**
     * @brief Handle Index Reindex.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexReindex(const http::request<http::string_body>& req);
    
    // Admin handlers moved to AdminApiHandler (admin_api_)
    // Previously: handleAdminBackup, handleAdminRestore

    // Content API endpoints
    /**
     * @brief Handle Content Import.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentImport(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentSearch(const http::request<http::string_body>& req);
    /**
     * @brief Handle Get Content.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetContent(const http::request<http::string_body>& req);
    /**
     * @brief Handle Get Content Blob.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetContentBlob(const http::request<http::string_body>& req);
    /**
     * @brief Handle Get Content Chunks.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetContentChunks(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Assemble.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentAssemble(const http::request<http::string_body>& req);
    /**
     * @brief Handle Chunk Navigation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleChunkNavigation(const http::request<http::string_body>& req);
    
    // Virtual Filesystem API
    /**
     * @brief Handle Filesystem Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFilesystemGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Filesystem Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFilesystemPut(const http::request<http::string_body>& req);
    /**
     * @brief Handle Filesystem Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFilesystemDelete(const http::request<http::string_body>& req);
    /**
     * @brief Handle Filesystem List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFilesystemList(const http::request<http::string_body>& req);
    /**
     * @brief Handle Filesystem Mkdir.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFilesystemMkdir(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Hybrid Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHybridSearch(const http::request<http::string_body>& req);
    /**
     * @brief Handle Fusion Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFusionSearch(const http::request<http::string_body>& req);
    /**
     * @brief Handle Fulltext Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleFulltextSearch(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Filter Schema Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFilterSchemaGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Filter Schema Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFilterSchemaPut(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentConfigGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentConfigPut(const http::request<http::string_body>& req);
    /**
     * @brief Handle Edge Weight Config Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEdgeWeightConfigGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Edge Weight Config Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEdgeWeightConfigPut(const http::request<http::string_body>& req);
    /**
     * @brief Handle Encryption Schema Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEncryptionSchemaGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Encryption Schema Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEncryptionSchemaPut(const http::request<http::string_body>& req);
    /**
     * @brief Capabilities (Core/Enterprise) endpoint
     * @param[in] req Input parameter.
     * @return Return value.
     */

    http::response<http::string_body> handleLlmInteractionPost(const http::request<http::string_body>& req);
    /**
     * @brief Handle Llm Interaction List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLlmInteractionList(const http::request<http::string_body>& req);
    /**
     * @brief Handle Llm Interaction Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLlmInteractionGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Llm Interaction Update Metadata.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleLlmInteractionUpdateMetadata(const http::request<http::string_body>& req);

    /**
     * @brief Sprint B: Time-Series endpoints Note: Time-Series methods have been extracted to TimeSeriesApiHandler See: include/server/timeseries_api_handler.
     * @param[in] req Input parameter.
     * @return Return value.
     * @details h
     */
    
    http::response<http::string_body> handleIndexSuggestions(const http::request<http::string_body>& req);
    /**
     * @brief Handle Index Patterns.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexPatterns(const http::request<http::string_body>& req);
    /**
     * @brief Handle Index Record Pattern.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexRecordPattern(const http::request<http::string_body>& req);
    /**
     * @brief Handle Index Clear Patterns.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIndexClearPatterns(const http::request<http::string_body>& req);
    
    // Audit API endpoints
    /**
     * @brief Handle Audit Query.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuditQuery(const http::request<http::string_body>& req);
    /**
     * @brief Handle Audit Export Csv.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAuditExportCsv(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Security Signatures List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSecuritySignaturesList(const http::request<http::string_body>& req);
    /**
     * @brief Handle Security Signature Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSecuritySignatureGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Security Signature Post.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSecuritySignaturePost(const http::request<http::string_body>& req);
    /**
     * @brief Handle Security Signature Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSecuritySignatureDelete(const http::request<http::string_body>& req);
    /**
     * @brief Handle Security Verify.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSecurityVerify(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Content Validate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentValidate(const http::request<http::string_body>& req);

    /**
     * @brief Handle Content Fs Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFsGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Fs Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFsPut(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Fs Head.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFsHead(const http::request<http::string_body>& req);
    /**
     * @brief Handle Content Fs Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleContentFsDelete(const http::request<http::string_body>& req);
    
    // SAGA API endpoints
    /**
     * @brief Handle Saga List Batches.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSagaListBatches(const http::request<http::string_body>& req);
    /**
     * @brief Handle Saga Batch Detail.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSagaBatchDetail(const http::request<http::string_body>& req);
    /**
     * @brief Handle Saga Verify Batch.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSagaVerifyBatch(const http::request<http::string_body>& req);
    /**
     * @brief Handle Saga Flush.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSagaFlush(const http::request<http::string_body>& req);

    // PII API endpoints
    /**
     * @brief Handle Pii List Mappings.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiListMappings(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pii Create Mapping.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiCreateMapping(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pii Get By Uuid.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiGetByUuid(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pii Export Csv.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiExportCsv(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pii Delete By Uuid.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiDeleteByUuid(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pii Reveal By Uuid.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePiiRevealByUuid(const http::request<http::string_body>& req);

    // Retention API endpoints
    /**
     * @brief Handle Retention List Policies.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionListPolicies(const http::request<http::string_body>& req);
    /**
     * @brief Handle Retention Create Policy.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionCreatePolicy(const http::request<http::string_body>& req);
    /**
     * @brief Handle Retention Delete Policy.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionDeletePolicy(const http::request<http::string_body>& req);
    /**
     * @brief Handle Retention Get History.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionGetHistory(const http::request<http::string_body>& req);
    /**
     * @brief Handle Retention Get Policy Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRetentionGetPolicyStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle Keys List Keys.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleKeysListKeys(const http::request<http::string_body>& req);
    /**
     * @brief Handle Keys Rotate Key.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleKeysRotateKey(const http::request<http::string_body>& req);

    /**
     * @brief Handle Api Key Create.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleApiKeyCreate(const http::request<http::string_body>& req);
    /**
     * @brief Handle Api Key List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleApiKeyList(const http::request<http::string_body>& req);
    /**
     * @brief Handle Api Key Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleApiKeyGet(const http::request<http::string_body>& req);
    /**
     * @brief Handle Api Key Update.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleApiKeyUpdate(const http::request<http::string_body>& req);
    /**
     * @brief Handle Api Key Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleApiKeyDelete(const http::request<http::string_body>& req);

    // Session Management endpoints
    /**
     * @brief Handle Session Create.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSessionCreate(const http::request<http::string_body>& req);
    /**
     * @brief Handle Session List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSessionList(const http::request<http::string_body>& req);
    /**
     * @brief Handle Session Revoke By Id.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSessionRevokeById(const http::request<http::string_body>& req);
    /**
     * @brief Handle Session Revoke Others.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSessionRevokeOthers(const http::request<http::string_body>& req);

    /**
     * @brief Handle Saml Login.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSamlLogin(const http::request<http::string_body>& req);
    /**
     * @brief Handle Saml Acs.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSamlAcs(const http::request<http::string_body>& req);
    /**
     * @brief Handle Saml Slo.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSamlSlo(const http::request<http::string_body>& req);
    /**
     * @brief Handle Saml Metadata.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSamlMetadata(const http::request<http::string_body>& req);

    /**
     * @brief Handle Pki Sign.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiSign(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Verify.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiVerify(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Pki Hsm Sign.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiHsmSign(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Hsm Keys.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiHsmKeys(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Timestamp.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiTimestamp(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Timestamp Verify.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiTimestampVerify(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Eidas Sign.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiEidasSign(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Eidas Verify.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiEidasVerify(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Certificates.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiCertificates(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Certificate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiCertificate(const http::request<http::string_body>& req);
    /**
     * @brief Handle Pki Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePkiStatus(const http::request<http::string_body>& req);

    /**
     * @brief Handle Classification List Rules.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleClassificationListRules(const http::request<http::string_body>& req);
    /**
     * @brief Handle Classification Test.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleClassificationTest(const http::request<http::string_body>& req);

    /**
     * @brief Handle Reports Compliance.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReportsCompliance(const http::request<http::string_body>& req);

    // Error API endpoints
    /**
     * @brief Handle Error Api List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleErrorApiList(const http::request<http::string_body>& req);
    /**
     * @brief Handle Error Api Get By Code.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleErrorApiGetByCode(const http::request<http::string_body>& req);
    /**
     * @brief Handle Error Api Categories.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleErrorApiCategories(const http::request<http::string_body>& req);
    /**
     * @brief Handle Error Api Search.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleErrorApiSearch(const http::request<http::string_body>& req);

    // Schema API endpoints
    /**
     * @brief Handle Schema Get Full.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaGetFull(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Get Tables.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaGetTables(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Get Table.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaGetTable(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Put.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaPut(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Patch.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaPatch(const http::request<http::string_body>& req);

    // Metadata extended endpoints
    /**
     * @brief Handle Metadata Information Schema.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataInformationSchema(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Get Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataGetStats(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Collect Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataCollectStats(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Get Constraints.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataGetConstraints(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Index Recommendations.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataIndexRecommendations(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Audit Log.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataAuditLog(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Schema Import.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataSchemaImport(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Batch Validate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataBatchValidate(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Get Column Lineage.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataGetColumnLineage(const http::request<http::string_body>& req);
    /**
     * @brief Handle Metadata Record Lineage Derivation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetadataRecordLineageDerivation(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Version History.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaVersionHistory(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Create Version.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaCreateVersion(const http::request<http::string_body>& req);
    /**
     * @brief Handle Schema Diff.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleSchemaDiff(const http::request<http::string_body>& req);

    // Utility methods
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );

    /**
     * @brief Apply Governance Headers.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void applyGovernanceHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& res
    );

    /**
     * @brief Require Scope.
     * @param[in] req Input parameter.
     * @param[in] scope Input parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> requireScope(
        const http::request<http::string_body>& req,
        std::string_view scope
    );

    /**
     * @brief Require Access.
     * @param[in] req Input parameter.
     * @param[in] required_scope Input parameter.
     * @param[in] action Input parameter.
     * @param[in] resource_path Path to the resource.
     * @return Return value.
     */
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
    /**
     * @brief Extract Auth Context.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    AuthContext extractAuthContext(const http::request<http::string_body>& req) const;

    /**
     * @brief Extract Path Param.
     * @param[in] path Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractPathParam(const std::string& path, const std::string& prefix);

    /**
     * @brief Ensure PIIPseudonymizer.
     */
    void ensurePIIPseudonymizer();

    /**
     * @brief Enforce Audit Rate Limit.
     * @param[in] req Input parameter.
     * @param[in] route_key Input parameter.
     * @return Return value.
     */
    std::optional<http::response<http::string_body>> enforceAuditRateLimit(
        const http::request<http::string_body>& req,
        std::string_view route_key);

    /**
     * @brief Record Continuous Learning Query Telemetry.
     * @param[in] req Input parameter.
     * @param[in] res Input parameter.
     * @param[in] request_start Input parameter.
     * @param[in] is_aql Input parameter.
     */
    void recordContinuousLearningQueryTelemetry(
        const http::request<http::string_body>& req,
        const http::response<http::string_body>& res,
        std::chrono::steady_clock::time_point request_start,
        bool is_aql);

    // Accept new connections
    /**
     * @brief Do Accept.
     */
    void doAccept();
    /**
     * @brief On Accept.
     * @param[in] ec Input parameter.
     * @param[in] socket Input parameter.
     */
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

    // Execution module – SLA-aware query dispatcher and work-stealing thread pool.
    // Active when THEMIS_EXECUTION_MODULE is ON (see cmake/CMakeLists.txt).
    // QueryScheduler provides EDF-based backpressure; WorkStealingThreadPool
    // serves as the backing executor for async query work items.
    std::unique_ptr<themis::execution::QueryScheduler>            query_scheduler_;
    std::unique_ptr<themis::resource::WorkStealingThreadPool>     execution_thread_pool_;

    // Production-consumer route handlers for previously test-only modules.
    // All are lazily initialised in the HttpServer constructor and accessed in handleRequest().
    std::unique_ptr<themis::server::AiPluginApiHandler>            ai_plugin_api_;
#ifdef THEMIS_PLUGIN_SCRAPER
    std::unique_ptr<themis::server::ScraperPluginApiHandler>       scraper_plugin_api_;
#endif
#ifdef THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED
    std::unique_ptr<themis::server::EncryptedStorageApiHandler>    encrypted_storage_api_;
#endif
#ifdef THEMIS_CHAOS_ADMIN
    std::unique_ptr<themis::server::ChaosAdminApiHandler>          chaos_admin_api_;
    std::shared_ptr<themis::chaos::ChaosScheduler>                 chaos_scheduler_;
#endif

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
    
    /**
     * @brief Record Latency.
     * @param[in] duration Input parameter.
     */
    void recordLatency(std::chrono::microseconds duration);
    
    /**
     * @brief Extract Client IP.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    std::string extractClientIP(const http::request<http::string_body>& req) const;
    
    /**
     * @brief Check whether a user exceeds the current rate limit.
     * @param[in] req Input parameter.
     * @return True when the user remains within the configured limit.
     */
    std::optional<http::response<http::string_body>> checkRateLimit(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Make Preflight Response.
     * @param[in] req Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Record Page Fetch.
     * @param[in] duration_ms Input parameter.
     */
    void recordPageFetch(std::chrono::milliseconds duration_ms);

    // ContentFS instance
    std::unique_ptr<themis::ContentFS> content_fs_;
};

} // namespace server
} // namespace themis
