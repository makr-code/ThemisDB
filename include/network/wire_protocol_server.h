/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_server.h                             ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     331                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol Server Header
// Secure, isolated TCP server for native binary protocol

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <deque>
#include <atomic>
#include <mutex>
#include <unordered_map>

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

namespace network {

namespace net = boost::asio;
using tcp = net::ip::tcp;

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
        std::shared_ptr<ContinuousAggregateManager> agg_manager = nullptr
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

private:
    class Session;  // Forward declaration

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

    // Rate limiting state (per-IP)
    struct RateLimitState {
        uint64_t window_start_ms = 0;
        uint32_t request_count_second = 0;
        uint32_t request_count_minute = 0;
    };
    mutable std::mutex rate_limit_mutex_;
    std::unordered_map<std::string, RateLimitState> rate_limits_;

    // Server state
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> session_id_counter_{0};

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

private:
    // Async operations
    void asyncReadHeader();
    void asyncReadPayload(uint32_t payload_size);
    void asyncReadChecksum();
    void asyncWriteResponse(const std::vector<uint8_t>& data);
    void doWrite();  // Internal write loop

    // Message handlers (OpCode dispatch)
    void handleMessage();
    void handleHello();
    void handleAuthRequest();
    void handleGet();
    void handlePut();
    void handleDelete();
    void handleQuery();
    void handleVectorSearch();
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
    std::string username_;
    std::string client_ip_;

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
