/**
 * @file wire_protocol_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: wire_protocol_server.h | Version: 0.0.47 | Last Modified: 2026-05-29 14:12:47
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 554
 * Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3631 feat(network): implement Wi... (2026-03-12) | #2545 [network] WebSocket upgrade... (2026-03-11) | #1136 Complete production-ready T... (2026-03-11) | #1137 Implement BPMN wire protoco... (2026-03-11) | #1157 Enforce TLS for Wire Protoc... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// ThemisDB Wire Protocol Server Header
// Secure, isolated TCP server for native binary protocol

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#ifdef THEMIS_ENABLE_WEBSOCKET
#  include <boost/beast/core.hpp>
#  include <boost/beast/http.hpp>
#  include <boost/beast/websocket.hpp>
#endif
#include "network/qos_manager.h"
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <deque>
#include <atomic>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class GraphIndexManager;
class VectorIndexManager;
class TransactionManager;
class ProcessGraphManager;
class TSStore;
class ContinuousAggregateManager;
class QueryEngine;
namespace index { class SpatialIndexManager; }

namespace network {

namespace net = boost::asio;
using tcp = net::ip::tcp;

#ifdef THEMIS_ENABLE_WEBSOCKET
namespace beast     = boost::beast;
namespace http_ws   = beast::http;
namespace websocket = beast::websocket;
// Forward declaration of the WebSocket session (defined in wire_protocol_websocket.h)
class WireProtocolWebSocketSession;
#endif

// ---------------------------------------------------------------------------
// GEO_QUERY injection bridge (stub #284 replacement)
// ---------------------------------------------------------------------------

/**
 * @brief Geospatial query injection bridge for the JSON wire protocol.
 *
 * When set via setNetworkGeoQueryFn(), GEO_QUERY "near" messages can be
 * dispatched to this function when no SpatialIndexManager is configured.
 * This closes the previous startup/runtime mismatch where the server accepted
 * a geo bridge during bootstrap but still returned GEO_NOT_INTEGRATED.
 *
 * The function receives the collection name, centre coordinates (WGS84
 * decimal degrees), search radius in metres, and result limit.  It must
 * return a nlohmann::json array of matching document objects.
 *
 * Thread-safety: the stored function pointer is protected by an internal
 * mutex; callers may register/clear the bridge at any time.
 *
 * @param collection  Target collection name.
 * @param lat         Latitude of the search centre (WGS84, decimal degrees).
 * @param lon         Longitude of the search centre (WGS84, decimal degrees).
 * @param radius_m    Search radius in metres (>0).
 * @param limit       Maximum number of results (<=0 means no limit).
 * @return            JSON array of matching document objects.
 */
using GeoQueryFn = std::function<nlohmann::json(
    const std::string& collection, double lat, double lon,
    double radius_m, int limit)>;

/**
 * @brief Register the geospatial query injection bridge.
 *
 * Thread-safe.  Replaces any previously registered function.  Pass a
 * null/empty function to clear the bridge and restore the
 * GEO_NOT_INTEGRATED fallback behaviour.
 *
 * @param fn  Callable to handle GEO_QUERY messages, or nullptr to clear.
 */
void setNetworkGeoQueryFn(GeoQueryFn fn);

/**
 * @brief Wire Protocol Server - Binary TCP Protocol
 * 
 * Security Features:
 * - Separate IO thread pool (isoliert vom HTTP Server)
 * - Connection limits & rate limiting
 * - Authentication required (SCRAM-SHA-256)
 * - TLS/mTLS support
 * - Request size limits
 * - Timeout protection
 * 
 * Performance Features:
 * - Dedicated thread pool (nicht shared mit HTTP)
 * - Lock-free connection management
 * - Zero-copy where possible
 * - Connection pooling
 */
class WireProtocolServer {
public:
    struct Config {
        std::string host = "0.0.0.0";
        uint16_t port = 8766;
        size_t num_io_threads = 4;  // Dedicated I/O threads
        size_t num_worker_threads = std::thread::hardware_concurrency();
        
        // Security limits
        uint32_t max_connections = 1000;
        uint32_t max_connections_per_ip = 10;
        uint32_t max_frame_size_mb = 64;
        uint32_t connection_timeout_sec = 300;  // 5 minutes
        uint32_t auth_timeout_sec = 10;
        uint32_t request_timeout_sec = 30;
        
        // Rate limiting (per IP)
        uint32_t max_requests_per_second = 1000;
        uint32_t max_requests_per_minute = 10000;
        
        // TLS Configuration
        bool enable_tls = false;
        std::string tls_cert_path;
        std::string tls_key_path;
        std::string tls_ca_cert_path;
        bool tls_require_client_cert = false;  // mTLS
        
        // Authentication
        bool require_auth = true;
        std::string auth_mechanism = "SCRAM-SHA-256";
        // Optional pre-shared token for simple token-based authentication.
        // When non-empty and require_auth=true the AUTH_REQUEST payload must
        // contain {"token":"<value>"} matching this string exactly.
        // When empty any non-empty token is accepted (development mode only).
        std::string auth_token;

        // WebSocket upgrade on wire protocol port (requires THEMIS_ENABLE_WEBSOCKET)
        // When true, incoming HTTP Upgrade: websocket requests on this port are
        // accepted and served by WireProtocolWebSocketSession instead of being
        // rejected as invalid binary frames.
        bool enable_websocket_upgrade = false;

        // IPv6 support
        // When true the server binds to an IPv6 socket.  If host is the default
        // "0.0.0.0" it is automatically promoted to "::" (IPv6 any-address).
        // Explicit IPv6 addresses in host (e.g. "::1" or "fe80::1") are always
        // honoured regardless of this flag.
        bool enable_ipv6 = false;

        // Dual-stack mode (IPV6_V6ONLY=0).
        // When enable_ipv6 is true and this flag is true, a single IPv6 socket
        // also accepts IPv4-mapped connections, eliminating the need for two
        // listener sockets.  Defaults to true; set to false to accept only pure
        // IPv6 connections.
        bool ipv6_dual_stack = true;

        // TCP listen backlog — controls the OS-level pending-connection queue
        // size passed to listen(2).  Under heavy load, connections beyond this
        // limit are silently dropped by the kernel before they ever reach the
        // application.  A value of 128 is the Linux default; increase for
        // high-concurrency deployments (the effective maximum is bounded by
        // /proc/sys/net/core/somaxconn on Linux).
        int tcp_backlog = 128;

        Config() = default;
    };

    WireProtocolServer(
        const Config& config,
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<TransactionManager> tx_manager,
        std::shared_ptr<ProcessGraphManager> process_graph = nullptr,
        std::shared_ptr<TSStore> ts_store = nullptr,
        std::shared_ptr<ContinuousAggregateManager> agg_manager = nullptr,
        std::shared_ptr<QueryEngine> query_engine = nullptr
    );

    ~WireProtocolServer();

    /**
     * @brief Validate transport security configuration for production
     * 
     * Checks if TLS is properly configured when running in production mode.
     * 
     * @param argc: Command-line argument count
     * @param argv: Command-line arguments
     * @return true if configuration is safe, false if should exit
     */
    bool validateTransportSecurity(int argc, const char* const argv[]) const;

    /**
     * @brief Start server in dedicated thread pool
     * 
     * Creates separate IO context and worker threads,
     * isolated from HTTP server to prevent interference.
     * Enforces transport security validation before starting.
    * Enforces fail-closed runtime dependency checks: QueryEngine must be
    * present and at least one geospatial backend (SpatialIndexManager or
    * GeoQueryFn bridge) must be configured.
     */
    void start();

    /**
     * @brief Graceful shutdown
     * 
     * Closes all connections, waits for in-flight requests,
     * shuts down thread pool.
     */
    void stop();

    /**
     * @brief Block until server stops
     */
    void wait();

    /**
     * @brief Check if running
     */
    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    /**
     * @brief Get active connection count
     */
    size_t getActiveConnections() const;

    /**
     * @brief Get server statistics
     */
    struct Stats {
        uint64_t total_connections = 0;
        uint64_t active_connections = 0;
        uint64_t rejected_connections = 0;  // Rate limit/max conn
        uint64_t total_requests = 0;
        uint64_t total_errors = 0;
        uint64_t auth_failures = 0;
        uint64_t bytes_received = 0;
        uint64_t bytes_sent = 0;
    };
    Stats getStats() const;

    // -------------------------------------------------------------------------
    // Per-tenant bandwidth quota management
    // -------------------------------------------------------------------------

    /**
     * @brief Register or update an aggregate bandwidth quota for a tenant.
     *
     * All connections that have been assigned to this tenant via
     * Session::setTenant() share a single token bucket enforcing the
     * aggregate limit.  Call this during server configuration or whenever
     * the tenant's quota changes.
     *
     * @param tenant_id   Unique tenant identifier.
     * @param rate_bps    Sustained aggregate bandwidth in bits per second
     *                    (0 = unlimited).
     * @param burst_bytes Maximum burst in bytes (0 = auto: 1 s of sustained rate).
     */
    void registerTenantQuota(const std::string& tenant_id,
                              uint64_t rate_bps,
                              uint64_t burst_bytes = 0);

    /**
     * @brief Update an existing tenant quota at runtime.
     *
     * Alias for registerTenantQuota; creates the entry when absent.
     */
    void setTenantQuota(const std::string& tenant_id,
                        uint64_t rate_bps,
                        uint64_t burst_bytes = 0);

    /**
     * @brief Remove the bandwidth quota for a tenant.
     *
     * Existing connections assigned to this tenant continue to work but
     * without the aggregate quota constraint.
     *
     * @param tenant_id Tenant identifier.
     */
    void unregisterTenantQuota(const std::string& tenant_id);

    /**
     * @brief Retrieve bandwidth statistics for a specific tenant.
     * @param tenant_id Tenant identifier.
     * @return Stats snapshot; default-constructed if tenant not found.
     */
    QoSManager::TenantQuotaStats getTenantBandwidthStats(
        const std::string& tenant_id) const;

    /**
     * @brief Retrieve bandwidth statistics for all registered tenants.
     */
    std::vector<QoSManager::TenantQuotaStats> getAllTenantBandwidthStats() const;

    /**
     * @brief Inject a SpatialIndexManager for GEO_QUERY dispatch.
     *
     * When set, GEO_QUERY commands on this wire-protocol port are dispatched to
     * the provided spatial index instead of returning GEO_NOT_INTEGRATED.  The
     * manager is accessed from multiple session threads and must be thread-safe.
     *
     * @param idx  Shared spatial index manager; nullptr disables geo dispatch
     *             and restores the NOT_INTEGRATED fallback.
     */
    void setSpatialIndexManager(std::shared_ptr<index::SpatialIndexManager> idx);

    // Cursor entry used by paginated query responses.
    struct CursorEntry {
        nlohmann::json results;    // Full result set (JSON array)
        size_t         offset = 0; // Next item index to return
        int64_t        ttl_ms = 0; // Expiry (epoch ms); 0 = never
    };

private:
    class Session;  // Forward declaration

#ifdef THEMIS_ENABLE_WEBSOCKET
    friend class WireProtocolWebSocketSession;
#endif

    // Accept new connections
    void doAccept();
    void handleAccept(std::shared_ptr<Session> session, const boost::system::error_code& error);

    // Security checks
    bool checkConnectionLimit(const std::string& remote_ip);
    bool checkRateLimit(const std::string& remote_ip);
    void registerConnection(const std::string& remote_ip);
    void unregisterConnection(const std::string& remote_ip);

    // Configuration
    Config config_;

    // Database access (shared, thread-safe)
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<ProcessGraphManager> process_graph_;
    std::shared_ptr<TSStore> ts_store_;
    std::shared_ptr<ContinuousAggregateManager> agg_manager_;
    std::shared_ptr<QueryEngine> query_engine_;
    std::shared_ptr<index::SpatialIndexManager> spatial_index_; ///< Optional geo-query back-end (stub #284).

    // Geospatial query injection bridge (stub #284)
    mutable std::mutex geo_query_fn_mutex_;
    GeoQueryFn         geo_query_fn_;

    // Cursor registry: stores live AQL query results for batch pagination.
    // cursor_id -> {results as JSON array, current offset, TTL timestamp}
    mutable std::mutex cursors_mutex_;
    std::unordered_map<std::string, CursorEntry> cursors_;

    // Networking (SEPARATE from HTTP server!)
    std::unique_ptr<net::io_context> io_context_;  // Dedicated IO context
    std::unique_ptr<tcp::acceptor> acceptor_;
    
    // Thread pools (ISOLATED)
    std::vector<std::thread> io_threads_;     // Accept + network I/O
    std::unique_ptr<boost::asio::thread_pool> worker_pool_;  // Request processing

    // Connection tracking (per-IP)
    mutable std::mutex connections_mutex_;
    std::unordered_map<std::string, uint32_t> connections_per_ip_;
    std::unordered_map<std::string, std::shared_ptr<Session>> active_sessions_;

#ifdef THEMIS_ENABLE_WEBSOCKET
    // Active WebSocket sessions on the wire protocol port (keyed by session_id)
    std::unordered_map<uint64_t, std::shared_ptr<WireProtocolWebSocketSession>>
        active_ws_sessions_;
#endif

    // Rate limiting state (per-IP)
    struct RateLimitState {
        uint64_t window_start_ms        = 0;  // start of current 1-second window
        uint64_t minute_window_start_ms = 0;  // start of current 60-second window
        uint32_t request_count_second   = 0;
        uint32_t request_count_minute   = 0;
    };
    mutable std::mutex rate_limit_mutex_;
    std::unordered_map<std::string, RateLimitState> rate_limits_;

    // Per-tenant bandwidth quota enforcement
    QoSManager qos_manager_;

    // Server state
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> session_id_counter_{0};

    // Backpressure / overload tracking
    // Counts accepted (live) connections; incremented in registerConnection(),
    // decremented in unregisterConnection().  Used for the global max_connections
    // check without holding connections_mutex_.
    std::atomic<uint32_t> active_connection_count_{0};
    // True while the server is in an overloaded state (active connections >=
    // max_connections).  Used to gate "entering overload" / "recovering" log
    // messages to one transition per state change instead of one per request.
    std::atomic<bool> overloaded_{false};

    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
};

/**
 * @brief Wire Protocol Session - Handle individual client connection
 * 
 * Security:
 * - Timeout on all operations
 * - Frame size validation
 * - Authentication state machine
 * - Request queue limits
 */
class WireProtocolServer::Session : public std::enable_shared_from_this<Session> {
public:
    friend class WireProtocolServer;

    Session(
        uint64_t session_id,
        tcp::socket socket,
        WireProtocolServer* server
    );

    ~Session();

    void start();
    void close();
    
    std::string getRemoteIP() const;
    uint64_t getSessionID() const { return session_id_; }
    bool isAuthenticated() const { return authenticated_.load(); }

    /**
     * @brief Assign this session to a tenant for bandwidth quota enforcement.
     *
     * Called after successful authentication once the tenant identity is known.
     * Registers the connection with the server's QoSManager so that the
     * per-tenant token bucket is enforced on subsequent sends.
     *
     * @param tenant_id Unique tenant identifier.
     */
    void setTenant(const std::string& tenant_id);

private:
    // Async operations
    void asyncReadHeader();
    void asyncReadPayload(uint32_t payload_size);
    void asyncReadChecksum();
    void asyncWriteResponse(const std::vector<uint8_t>& data);
    void doWrite();  // Internal write loop

    // Dispatch a heavy handler function to the server's worker_pool_.
    // Copies payload_buffer_ and header_buffer_ for the worker lambda so the
    // I/O thread can immediately begin reading the next frame.
    // Falls back to direct invocation when worker_pool_ is not configured.
    void dispatchToWorkerPool(std::function<void()> handler);

#ifdef THEMIS_ENABLE_WEBSOCKET
    // Protocol detection: reads first 4 bytes and decides binary vs WebSocket
    void asyncDetectProtocol();
    // Continues binary header read after 4 bytes have been peeked
    void asyncReadRemainingHeader();
    // Reads the remaining HTTP request lines and performs WebSocket upgrade
    void asyncUpgradeToWebSocket(const std::array<uint8_t, 4>& first_bytes);
#endif

    // Message handlers (OpCode dispatch)
    void handleMessage();
    void handleHello();
    void handleAuthRequest();   // 0x03 (backward-compat alias) and 0x04 AUTH_RESPONSE
    void handleGet();
    void handlePut();
    void handleDelete();
    void handleBatchGet();
    void handleBatchPut();
    void handleQuery();
    void handleCursorNext();
    void handleCursorClose();
    void handleTransactionBegin();
    void handleTransactionCommit();
    void handleTransactionAbort();
    void handleVectorSearch();
    void handleGraphTraverse();
    void handleGeoQuery();
    void handleTimeseriesQuery();
    void handleBpmnStartProcess();
    void handleBpmnTaskComplete();
    void handleBpmnQueryInstance();
    void handlePing();
    void handleClose();

    // Error handling
    void sendError(uint32_t error_code, const std::string& message);
    void handleError(const std::string& context, const boost::system::error_code& ec);

    // Timeout management
    void startTimeout(std::chrono::seconds timeout);
    void cancelTimeout();

    // Session data
    uint64_t session_id_;
    tcp::socket socket_;
    WireProtocolServer* server_;
    
    // Authentication state
    std::atomic<bool> authenticated_{false};
    std::atomic<bool> closed_{false};
    std::string username_;
    std::string client_ip_;

    // Tenant assigned to this session (set after authentication)
    std::string tenant_id_;

    // Read buffers
    std::array<uint8_t, 12> header_buffer_;  // Wire frame header
    std::vector<uint8_t> payload_buffer_;
    uint32_t checksum_buffer_;
    uint16_t current_flags_ = 0;  // Current message flags

    // Write queue (prevent write-write race)
    std::mutex write_mutex_;
    std::deque<std::vector<uint8_t>> write_queue_;
    bool write_in_progress_ = false;

    // Timeout timer
    std::unique_ptr<net::steady_timer> timeout_timer_;

    // Statistics
    std::atomic<uint64_t> requests_processed_{0};
    std::atomic<uint64_t> bytes_received_{0};
    std::atomic<uint64_t> bytes_sent_{0};
};

} // namespace network
} // namespace themis
