// ThemisDB Wire Protocol Server Implementation
// Binary protocol for high-performance native client communication

#include "network/wire_protocol_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"

#include <iostream>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>  // For ntohl/htonl

namespace themis::network {

// =============================================================================
// WireProtocolServer Implementation
// =============================================================================

WireProtocolServer::WireProtocolServer(
    const Config& config,
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<TransactionManager> tx_manager,
    std::shared_ptr<TSStore> ts_store,
    std::shared_ptr<ContinuousAggregateManager> agg_manager)
    : config_(config)
    , storage_(storage)
    , secondary_index_(secondary_index)
    , graph_index_(graph_index)
    , vector_index_(vector_index)
    , tx_manager_(tx_manager)
    , ts_store_(ts_store)
    , agg_manager_(agg_manager)
{
    io_context_ = std::make_unique<net::io_context>();
    worker_pool_ = std::make_unique<net::thread_pool>(config_.num_worker_threads);
    acceptor_ = std::make_unique<tcp::acceptor>(*io_context_);
}

WireProtocolServer::~WireProtocolServer() {
    stop();
}

void WireProtocolServer::start() {
    running_.store(true, std::memory_order_release);

    tcp::endpoint endpoint(tcp::v4(), config_.port);
    acceptor_->open(endpoint.protocol());
    acceptor_->set_option(tcp::acceptor::reuse_address(true));
    acceptor_->bind(endpoint);
    acceptor_->listen();

    // Start I/O threads
    for (size_t i = 0; i < config_.num_io_threads; ++i) {
        io_threads_.emplace_back([this]() {
            try {
                io_context_->run();
            } catch (const std::exception& e) {
                std::cerr << "[WireProtocol] IO thread error: " << e.what() << std::endl;
            }
        });
    }

    doAccept();
}

void WireProtocolServer::stop() {
    running_.store(false, std::memory_order_release);
    
    if (acceptor_ && acceptor_->is_open()) {
        acceptor_->close();
    }

    if (io_context_) {
        io_context_->stop();
    }

    for (auto& t : io_threads_) {
        if (t.joinable()) t.join();
    }

    if (worker_pool_) {
        worker_pool_->wait();
    }
}

void WireProtocolServer::wait() {
    for (auto& t : io_threads_) {
        if (t.joinable()) t.join();
    }
}

size_t WireProtocolServer::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return active_sessions_.size();
}

WireProtocolServer::Stats WireProtocolServer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

bool WireProtocolServer::checkConnectionLimit(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_per_ip_.find(remote_ip);
    if (it != connections_per_ip_.end() && it->second >= config_.max_connections_per_ip) {
        return false;
    }
    return true;
}

bool WireProtocolServer::checkRateLimit(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    auto& state = rate_limits_[remote_ip];
    if (state.window_start_ms == 0) {
        state.window_start_ms = now_ms;
    }

    // Reset if window expired
    if (now_ms - state.window_start_ms >= 1000) {
        state.window_start_ms = now_ms;
        state.request_count_second = 0;
        state.request_count_minute = 0;
    }

    // Check rate limits
    if (state.request_count_second >= config_.max_requests_per_second ||
        state.request_count_minute >= config_.max_requests_per_minute) {
        return false;
    }

    state.request_count_second++;
    state.request_count_minute++;
    return true;
}

void WireProtocolServer::registerConnection(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_per_ip_[remote_ip]++;
}

void WireProtocolServer::unregisterConnection(const std::string& remote_ip) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_per_ip_.find(remote_ip);
    if (it != connections_per_ip_.end() && it->second > 0) {
        it->second--;
    }
}

void WireProtocolServer::doAccept() {
    if (!running_.load(std::memory_order_acquire)) return;

    auto session = std::make_shared<Session>(
        session_id_counter_.fetch_add(1, std::memory_order_acq_rel),
        tcp::socket(*io_context_),
        this);

    acceptor_->async_accept(
        const_cast<tcp::socket&>(session->socket_),
        [this, session](const boost::system::error_code& ec) {
            handleAccept(session, ec);
        });
}

void WireProtocolServer::handleAccept(std::shared_ptr<Session> session, const boost::system::error_code& error) {
    if (!error) {
        // Get remote IP from accepted socket
        std::string remote_ip = "unknown";
        try {
            remote_ip = session->socket_.remote_endpoint().address().to_string();
        } catch (...) {
            // Fall back to unknown if we can't get the endpoint
        }

        if (!checkConnectionLimit(remote_ip)) {
            return;
        }

        if (!checkRateLimit(remote_ip)) {
            return;
        }

        registerConnection(remote_ip);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            active_sessions_[remote_ip] = session;
        }

        session->start();
    }

    doAccept();
}

// =============================================================================
// Session Implementation
// =============================================================================

WireProtocolServer::Session::Session(uint64_t session_id, tcp::socket socket, WireProtocolServer* server)
    : session_id_(session_id)
    , socket_(std::move(socket))
    , server_(server)
    , client_ip_("unknown")  // Will be set after accept in start()
{
}

WireProtocolServer::Session::~Session() {
    close();
}

void WireProtocolServer::Session::start() {
    try {
        // Now that socket is accepted, we can get the remote endpoint
        client_ip_ = socket_.remote_endpoint().address().to_string();
    } catch (const std::exception& e) {
        client_ip_ = "unknown";
    }
    
    startTimeout(std::chrono::seconds(server_->config_.request_timeout_sec));
    asyncReadHeader();
}

void WireProtocolServer::Session::close() {
    try {
        if (socket_.is_open()) {
            socket_.close();
        }
    } catch (...) {
    }

    server_->unregisterConnection(client_ip_);

    {
        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        server_->active_sessions_.erase(client_ip_);
    }
}

std::string WireProtocolServer::Session::getRemoteIP() const {
    try {
        return socket_.remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}

void WireProtocolServer::Session::asyncReadHeader() {
    auto self = shared_from_this();
    net::async_read(
        socket_,
        net::buffer(header_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                handleMessage();
                asyncReadHeader();
            } else {
                handleError("asyncReadHeader", ec);
            }
        });
}

void WireProtocolServer::Session::asyncReadPayload(uint32_t payload_size) {
    if (payload_size == 0) {
        asyncReadChecksum();
        return;
    }

    payload_buffer_.resize(payload_size);
    auto self = shared_from_this();
    
    net::async_read(
        socket_,
        net::buffer(payload_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                asyncReadChecksum();
            } else {
                handleError("asyncReadPayload", ec);
            }
        });
}

void WireProtocolServer::Session::asyncReadChecksum() {
    auto self = shared_from_this();
    net::async_read(
        socket_,
        net::buffer(&checksum_buffer_, sizeof(checksum_buffer_)),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                // Process message
            } else {
                handleError("asyncReadChecksum", ec);
            }
        });
}

void WireProtocolServer::Session::handleMessage() {
    requests_processed_.fetch_add(1, std::memory_order_relaxed);
    bytes_received_.fetch_add(header_buffer_.size(), std::memory_order_relaxed);
    
    // Parse header: Magic (4) + Version (1) + OpCode (1) + Flags (2) + PayloadSize (4)
    // Extract OpCode from header_buffer_[5] (0-indexed: bytes 0-3 are magic, 4 is version, 5 is opcode)
    uint8_t opcode = header_buffer_[5];
    
    // Extract payload size (bytes 6-9, big-endian)
    uint32_t payload_size = 0;
    std::memcpy(&payload_size, &header_buffer_[6], sizeof(uint32_t));
    // Convert from network byte order (big-endian) to host byte order
    payload_size = ntohl(payload_size);
    
    // Validate payload size
    if (payload_size > server_->config_.max_frame_size_mb * 1024 * 1024) {
        sendError(0x0001, "Payload size exceeds maximum allowed");
        return;
    }
    
    // Dispatch based on OpCode
    switch (opcode) {
        case 0x01: // HELLO
            handleHello();
            break;
        case 0x03: // AUTH_REQUEST
            handleAuthRequest();
            break;
        case 0x10: // GET
            handleGet();
            break;
        case 0x11: // PUT
            handlePut();
            break;
        case 0x12: // DELETE
            handleDelete();
            break;
        case 0x20: // QUERY_AQL
            handleQuery();
            break;
        case 0x40: // VECTOR_SEARCH
            handleVectorSearch();
            break;
        case 0x50: // GEO_QUERY
            handleGeoQuery();
            break;
        case 0x51: // TIMESERIES_QUERY
            handleTimeseriesQuery();
            break;
        case 0xFE: // PING
            handlePing();
            break;
        case 0xFF: // CLOSE
            handleClose();
            break;
        default:
            sendError(0x0002, "Unknown OpCode: 0x" + std::to_string(opcode));
            break;
    }
}

void WireProtocolServer::Session::sendError(uint32_t error_code, const std::string& message) {
    // Implementation placeholder
}

void WireProtocolServer::Session::handleError(const std::string& context, const boost::system::error_code& ec) {
    if (ec != net::error::operation_aborted) {
        // Log error
    }
    close();
}

void WireProtocolServer::Session::startTimeout(std::chrono::seconds timeout) {
    timeout_timer_ = std::make_unique<net::steady_timer>(*server_->io_context_, timeout);
    auto self = shared_from_this();
    timeout_timer_->async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            close();
        }
    });
}

void WireProtocolServer::Session::cancelTimeout() {
    if (timeout_timer_) {
        timeout_timer_->cancel();
    }
}

void WireProtocolServer::Session::asyncWriteResponse(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    write_queue_.push_back(data);
}

// =============================================================================
// Message Handler Implementations
// =============================================================================

void WireProtocolServer::Session::handleHello() {
    // TODO: Implement HELLO handshake
    sendError(0x0003, "HELLO not yet implemented");
}

void WireProtocolServer::Session::handleAuthRequest() {
    // TODO: Implement authentication
    sendError(0x0003, "AUTH not yet implemented");
}

void WireProtocolServer::Session::handleGet() {
    // TODO: Implement GET operation
    sendError(0x0003, "GET not yet implemented");
}

void WireProtocolServer::Session::handlePut() {
    // TODO: Implement PUT operation
    sendError(0x0003, "PUT not yet implemented");
}

void WireProtocolServer::Session::handleDelete() {
    // TODO: Implement DELETE operation
    sendError(0x0003, "DELETE not yet implemented");
}

void WireProtocolServer::Session::handleQuery() {
    // TODO: Implement AQL query execution
    sendError(0x0003, "QUERY not yet implemented");
}

void WireProtocolServer::Session::handleVectorSearch() {
    // TODO: Implement vector search
    sendError(0x0003, "VECTOR_SEARCH not yet implemented");
}

void WireProtocolServer::Session::handleGeoQuery() {
    // TODO: Implement geospatial query
    sendError(0x0003, "GEO_QUERY not yet implemented");
}

void WireProtocolServer::Session::handleTimeseriesQuery() {
    // Check if TSStore is available
    if (!server_->ts_store_) {
        sendError(0x0004, "Time-series storage not configured");
        return;
    }
    
    // NOTE: This is a minimal implementation that demonstrates the wire protocol integration.
    // In production, this would parse the protobuf payload from payload_buffer_.
    // For now, we provide a stub that shows the integration pattern.
    
    try {
        // Parse payload (in a full implementation, this would deserialize TimeSeriesQueryRequest protobuf)
        // For now, we use hardcoded values to demonstrate the flow
        
        // Example: Query the last hour of data for a test metric
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto one_hour_ago_ms = now_ms - (60 * 60 * 1000);
        
        TSStore::QueryOptions query_opts;
        query_opts.metric = "test_metric";  // Would come from protobuf request
        query_opts.from_timestamp_ms = one_hour_ago_ms;
        query_opts.to_timestamp_ms = now_ms;
        query_opts.limit = 1000;
        
        // Execute query using TSStore
        auto result = server_->ts_store_->query(query_opts);
        
        if (!result) {
            sendError(0x0005, "Time-series query failed: " + std::string(result.error().what()));
            return;
        }
        
        // If continuous aggregates are requested, use AggregateManager
        // This demonstrates integration with both TSStore and ContinuousAggregateManager
        bool use_aggregates = false;  // Would come from protobuf request
        if (use_aggregates && server_->agg_manager_) {
            // Example: Use pre-computed aggregates for better performance
            auto agg_result = server_->ts_store_->aggregate(query_opts);
            if (!agg_result) {
                sendError(0x0006, "Time-series aggregation failed: " + std::string(agg_result.error().what()));
                return;
            }
            
            // In a full implementation, we would serialize agg_result to TimeSeriesQueryResponse protobuf
            // and send it back via asyncWriteResponse
        }
        
        // In a full implementation:
        // 1. Serialize result to TimeSeriesQueryResponse protobuf
        // 2. Build wire frame with OpCode::QUERY_RESULT (0x21)
        // 3. Calculate checksum
        // 4. Send via asyncWriteResponse
        
        // For now, send a placeholder success response
        std::vector<uint8_t> response;
        // Wire frame header: Magic (4) + Version (1) + OpCode (1) + Flags (2) + PayloadSize (4) = 12 bytes
        uint32_t magic = 0x544D4442;  // "TMDB"
        response.resize(12);
        std::memcpy(&response[0], &magic, 4);
        response[4] = 0x01;  // Version 1
        response[5] = 0x21;  // OpCode: QUERY_RESULT
        response[6] = 0x00;  // Flags (low byte)
        response[7] = 0x00;  // Flags (high byte)
        uint32_t payload_size = 0;  // Empty payload for now
        std::memcpy(&response[8], &payload_size, 4);
        
        asyncWriteResponse(response);
        
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("Time-series query exception: ") + e.what());
    }
}

void WireProtocolServer::Session::handlePing() {
    // TODO: Implement PING/PONG
    sendError(0x0003, "PING not yet implemented");
}

void WireProtocolServer::Session::handleClose() {
    // Handle graceful connection close
    close();
}

} // namespace themis::network

