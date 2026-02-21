/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_server.cpp                           ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   75.0/100                                       ║
    • Total Lines:     1208                                           ║
    • Open Issues:     TODOs: 10, Stubs: 1                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol Server Implementation
// Binary protocol for high-performance native client communication

#include "network/wire_protocol_server.h"
#include "network/wire_protocol_helpers.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "index/process_graph.h"
#include "transaction/transaction_manager.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "security/transport_security_checker.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>  // For snprintf
#ifdef _WIN32
    #include <winsock2.h>  // For ntohl/htonl on Windows
#else
    #include <arpa/inet.h>  // For ntohl/htonl on Unix
#endif
#include <map>  // For multi-bucket aggregation
#include <algorithm>  // For std::min/max

using json = nlohmann::json;

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
    std::shared_ptr<ProcessGraphManager> process_graph,
    std::shared_ptr<TSStore> ts_store,
    std::shared_ptr<ContinuousAggregateManager> agg_manager)
    : config_(config)
    , storage_(storage)
    , secondary_index_(secondary_index)
    , graph_index_(graph_index)
    , vector_index_(vector_index)
    , tx_manager_(tx_manager)
    , process_graph_(process_graph)
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

bool WireProtocolServer::validateTransportSecurity(int argc, const char* const argv[]) const {
    return themis::security::TransportSecurityChecker::validateProductionSafety(
        config_.enable_tls,
        "Wire Protocol",
        argc,
        argv
    );
}

void WireProtocolServer::start() {
    // Enforce transport security validation as a startup gate
    // We do not have argc/argv here, so we pass an empty argument list
    if (!validateTransportSecurity(0, nullptr)) {
        std::cerr << "[WireProtocol] Transport security validation failed. Server will not start."
                  << std::endl;
        return;
    }
    
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
                // Parse header to get payload size, then read payload
                if (header_buffer_.size() >= 12) {
                    // Extract flags (bytes 6-7, big-endian)
                    uint16_t flags = 0;
                    std::memcpy(&flags, &header_buffer_[6], sizeof(uint16_t));
                    flags = ntohs(flags);
                    current_flags_ = flags;
                    
                    // Extract payload size (bytes 8-11, big-endian)
                    // Wire format: Magic(4) + Version(1) + OpCode(1) + Flags(2) + PayloadSize(4)
                    uint32_t payload_size = 0;
                    std::memcpy(&payload_size, &header_buffer_[8], sizeof(uint32_t));
                    payload_size = ntohl(payload_size);
                    
                    // Validate payload size
                    if (payload_size > server_->config_.max_frame_size_mb * 1024 * 1024) {
                        sendError(0x0001, "Payload size exceeds maximum allowed");
                        asyncReadHeader();  // Continue reading
                        return;
                    }
                    
                    // Read payload if present
                    asyncReadPayload(payload_size);
                } else {
                    sendError(0x0008, "Invalid header size");
                    asyncReadHeader();
                }
            } else {
                handleError("asyncReadHeader", ec);
            }
        });
}

void WireProtocolServer::Session::asyncReadPayload(uint32_t payload_size) {
    if (payload_size == 0) {
        // No payload, check if we need to read checksum
        payload_buffer_.clear();
        
        // Check if SKIP_CHECKSUM flag is set (bit 2)
        const uint16_t SKIP_CHECKSUM_FLAG = 0x0004;
        if (!(current_flags_ & SKIP_CHECKSUM_FLAG)) {
            // Checksum expected, read it
            asyncReadChecksum();
        } else {
            // No checksum, dispatch immediately
            handleMessage();
            asyncReadHeader();
        }
        return;
    }

    payload_buffer_.resize(payload_size);
    auto self = shared_from_this();
    
    net::async_read(
        socket_,
        net::buffer(payload_buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes*/) {
            if (!ec) {
                // Payload read successfully, check if we need to read checksum
                const uint16_t SKIP_CHECKSUM_FLAG = 0x0004;
                if (!(current_flags_ & SKIP_CHECKSUM_FLAG)) {
                    // Checksum expected, read it
                    asyncReadChecksum();
                } else {
                    // No checksum, dispatch immediately
                    handleMessage();
                    asyncReadHeader();
                }
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
                // Checksum read successfully
                // TODO: Verify checksum against header + payload
                // For now, we just consume it and continue
                
                // Dispatch message
                handleMessage();
                asyncReadHeader();  // Continue reading next message
            } else {
                handleError("asyncReadChecksum", ec);
            }
        });
}

void WireProtocolServer::Session::handleMessage() {
    requests_processed_.fetch_add(1, std::memory_order_relaxed);
    bytes_received_.fetch_add(header_buffer_.size() + payload_buffer_.size(), std::memory_order_relaxed);
    
    // Validate header size (must be at least 12 bytes)
    if (header_buffer_.size() < 12) {
        sendError(0x0008, "Invalid header size");
        return;
    }
    
    // Parse header: Magic (4) + Version (1) + OpCode (1) + Flags (2) + PayloadSize (4)
    // Extract OpCode from header_buffer_[5] (0-indexed: bytes 0-3 are magic, 4 is version, 5 is opcode)
    uint8_t opcode = header_buffer_[5];
    
    // Dispatch based on OpCode
    // Note: payload_buffer_ is already populated by asyncReadPayload
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
        case 0x60: // BPMN_START_PROCESS
            handleBpmnStartProcess();
            break;
        case 0x61: // BPMN_TASK_COMPLETE
            handleBpmnTaskComplete();
            break;
        case 0x62: // BPMN_QUERY_INSTANCE
            handleBpmnQueryInstance();
            break;
        case 0xFE: // PING
            handlePing();
            break;
        case 0xFF: // CLOSE
            handleClose();
            break;
        default: {
            // Format opcode as hexadecimal
            char hex_opcode[8];
            std::snprintf(hex_opcode, sizeof(hex_opcode), "0x%02X", opcode);
            sendError(0x0002, std::string("Unknown OpCode: ") + hex_opcode);
            break;
        }
    }
}

void WireProtocolServer::Session::sendError(uint32_t error_code, const std::string& message) {
    // Build error response with header
    json error_json;
    error_json["error_code"] = error_code;
    error_json["error_message"] = message;
    
    std::string error_str = error_json.dump();
    std::vector<uint8_t> error_data(error_str.begin(), error_str.end());
    
    asyncWriteResponse(error_data);
}

void WireProtocolServer::Session::handlePing() {
    // Simple ping response
    json response;
    response["pong"] = true;
    response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::string response_str = response.dump();
    std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
    asyncWriteResponse(response_data);
}

void WireProtocolServer::Session::handleClose() {
    close();
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
    
    // Start writing if not already in progress
    if (!write_in_progress_) {
        write_in_progress_ = true;
        doWrite();
    }
}

void WireProtocolServer::Session::doWrite() {
    // Must be called with write_mutex_ already locked OR from async callback
    if (write_queue_.empty()) {
        write_in_progress_ = false;
        return;
    }
    
    auto self = shared_from_this();
    auto& front = write_queue_.front();
    
    net::async_write(
        socket_,
        net::buffer(front),
        [this, self](const boost::system::error_code& ec, std::size_t bytes) {
            if (!ec) {
                bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);
                
                std::lock_guard<std::mutex> lock(write_mutex_);
                write_queue_.pop_front();
                
                if (!write_queue_.empty()) {
                    doWrite();  // Continue with next message
                } else {
                    write_in_progress_ = false;
                }
            } else {
                std::lock_guard<std::mutex> lock(write_mutex_);
                write_in_progress_ = false;
                handleError("asyncWrite", ec);
            }
        });
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
    
    try {
        // Parse TimeSeriesQueryRequest from payload
        TimeSeriesQueryRequest request;
        if (!TimeSeriesQueryRequest::parse(payload_buffer_, request)) {
            sendError(0x0009, "Failed to parse TimeSeriesQueryRequest");
            return;
        }
        
        // Validate request
        if (request.collection.empty()) {
            sendError(0x000A, "Collection (metric) name is required");
            return;
        }
        
        if (request.start_time_ns >= request.end_time_ns) {
            sendError(0x000B, "start_time_ns must be less than end_time_ns");
            return;
        }
        
        // Start timing
        auto query_start = std::chrono::high_resolution_clock::now();
        
        // Convert timestamps from nanoseconds to milliseconds (TSStore internal format)
        int64_t start_ms = static_cast<int64_t>(request.start_time_ns / 1000000);
        int64_t end_ms = static_cast<int64_t>(request.end_time_ns / 1000000);
        
        // Build TSStore query options
        TSStore::QueryOptions query_opts;
        query_opts.metric = request.collection;
        query_opts.from_timestamp_ms = start_ms;
        query_opts.to_timestamp_ms = end_ms;
        query_opts.limit = 10000;  // Production limit
        
        TimeSeriesQueryResponse response;
        
        // Check if aggregation is requested
        // Note: aggregation enum has AVG=0, so we can't use aggregation!=0 to detect requests
        // Use bucket_size_ns > 0 OR explicit aggregation field presence as indicator
        // For production: if request has aggregation field set (even if AVG=0), treat as aggregation request
        bool needs_aggregation = (request.bucket_size_ns > 0);
        
        if (needs_aggregation) {
            // If bucket_size is specified, create time buckets with raw data
            if (request.bucket_size_ns > 0) {
                // Query raw data points to enable per-bucket aggregation
                auto result = server_->ts_store_->query(query_opts);
                
                if (!result) {
                    std::string error_msg = "Time-series query failed";
                    try {
                        error_msg = std::string("Time-series query failed: ") + result.error().message();
                    } catch (...) {
                        // Fallback if error() access fails
                    }
                    sendError(0x0005, error_msg);
                    return;
                }
                
                const auto& data_points = result.value();
                
                // Bucket the data by time windows
                uint64_t bucket_size_ms = request.bucket_size_ns / 1000000;
                if (bucket_size_ms == 0) bucket_size_ms = 1;  // Minimum 1ms buckets
                
                // Calculate number of buckets
                int64_t time_range_ms = end_ms - start_ms;
                size_t num_buckets = static_cast<size_t>((time_range_ms + bucket_size_ms - 1) / bucket_size_ms);
                
                // Limit buckets to reasonable number
                if (num_buckets > 10000) {
                    num_buckets = 10000;
                    bucket_size_ms = time_range_ms / num_buckets;
                }
                
                // Create buckets and aggregate data points into them
                std::map<int64_t, std::vector<double>> bucket_data;
                
                for (const auto& point : data_points) {
                    // Determine which bucket this point belongs to
                    int64_t bucket_index = (point.timestamp_ms - start_ms) / bucket_size_ms;
                    int64_t bucket_start_ms = start_ms + (bucket_index * bucket_size_ms);
                    bucket_data[bucket_start_ms].push_back(point.value);
                }
                
                // Create response buckets with aggregated values
                for (const auto& [bucket_start_ms, values] : bucket_data) {
                    if (values.empty()) continue;
                    
                    TimeSeriesBucket bucket;
                    bucket.timestamp_ns = static_cast<uint64_t>(bucket_start_ms) * 1000000;
                    bucket.count = values.size();
                    
                    // Calculate aggregation
                    double min_val = values[0];
                    double max_val = values[0];
                    double sum_val = 0.0;
                    
                    for (double val : values) {
                        min_val = std::min(min_val, val);
                        max_val = std::max(max_val, val);
                        sum_val += val;
                    }
                    
                    bucket.min = min_val;
                    bucket.max = max_val;
                    
                    // Set value based on aggregation type
                    switch (request.aggregation) {
                        case 0:  // AVG
                            bucket.value = sum_val / values.size();
                            break;
                        case 1:  // SUM
                            bucket.value = sum_val;
                            break;
                        case 2:  // MIN
                            bucket.value = min_val;
                            break;
                        case 3:  // MAX
                            bucket.value = max_val;
                            break;
                        case 4:  // COUNT
                            bucket.value = static_cast<double>(values.size());
                            break;
                        default:
                            bucket.value = sum_val / values.size();
                    }
                    
                    response.buckets.push_back(bucket);
                }
                
                response.stats.total_data_points = data_points.size();
                response.stats.buckets_returned = response.buckets.size();
                response.stats.data_density = data_points.empty() ? 0.0 : 
                    static_cast<double>(data_points.size()) / response.buckets.size();
            } else {
                // No bucketing specified, return single aggregated result
                auto agg_result = server_->ts_store_->aggregate(query_opts);
                
                if (!agg_result) {
                    std::string error_msg = "Time-series aggregation failed";
                    try {
                        error_msg = std::string("Time-series aggregation failed: ") + agg_result.error().message();
                    } catch (...) {
                        // Fallback if error() access fails
                    }
                    sendError(0x0005, error_msg);
                    return;
                }
                
                const auto& agg_data = agg_result.value();
                
                // Create single bucket with aggregated values
                TimeSeriesBucket bucket;
                bucket.timestamp_ns = request.start_time_ns;
                bucket.count = agg_data.count;
                bucket.min = agg_data.min;
                bucket.max = agg_data.max;
                
                // Set value based on aggregation type
                switch (request.aggregation) {
                    case 0:  // AVG
                        bucket.value = agg_data.avg;
                        break;
                    case 1:  // SUM
                        bucket.value = agg_data.sum;
                        break;
                    case 2:  // MIN
                        bucket.value = agg_data.min;
                        break;
                    case 3:  // MAX
                        bucket.value = agg_data.max;
                        break;
                    case 4:  // COUNT
                        bucket.value = static_cast<double>(agg_data.count);
                        break;
                    default:
                        bucket.value = agg_data.avg;
                }
                
                response.buckets.push_back(bucket);
                response.stats.total_data_points = agg_data.count;
                response.stats.buckets_returned = 1;
                response.stats.data_density = static_cast<double>(agg_data.count);
            }
        } else {
            // Use raw query path
            auto result = server_->ts_store_->query(query_opts);
            
            if (!result) {
                std::string error_msg = "Time-series query failed";
                try {
                    error_msg = std::string("Time-series query failed: ") + result.error().message();
                } catch (...) {
                    // Fallback if error() access fails
                }
                sendError(0x0005, error_msg);
                return;
            }
            
            const auto& data_points = result.value();
            
            // Convert data points to buckets (one bucket per data point)
            for (const auto& point : data_points) {
                TimeSeriesBucket bucket;
                bucket.timestamp_ns = static_cast<uint64_t>(point.timestamp_ms) * 1000000;  // ms to ns
                bucket.value = point.value;
                bucket.count = 1;
                bucket.min = point.value;
                bucket.max = point.value;
                
                response.buckets.push_back(bucket);
            }
            
            response.stats.total_data_points = data_points.size();
            response.stats.buckets_returned = data_points.size();
            response.stats.data_density = data_points.empty() ? 0.0 : 1.0;
        }
        
        // Calculate query time
        auto query_end = std::chrono::high_resolution_clock::now();
        auto query_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(query_end - query_start).count();
        response.query_time_us = static_cast<uint64_t>(query_duration_us);
        
        // Serialize response to protobuf wire format
        auto response_payload = response.serialize();
        
        // Build wire frame response
        std::vector<uint8_t> wire_response;
        
        // Wire frame header: Magic (4) + Version (1) + OpCode (1) + Flags (2) + PayloadSize (4) = 12 bytes
        uint32_t magic = htonl(0x544D4442);  // "TMDB" in network byte order
        wire_response.resize(12);
        std::memcpy(&wire_response[0], &magic, 4);
        wire_response[4] = 0x01;  // Version 1
        wire_response[5] = 0x21;  // OpCode: QUERY_RESULT
        wire_response[6] = 0x00;  // Flags (low byte)
        wire_response[7] = 0x00;  // Flags (high byte)
        
        uint32_t payload_size = static_cast<uint32_t>(response_payload.size());
        uint32_t payload_size_net = htonl(payload_size);  // Convert to network byte order
        std::memcpy(&wire_response[8], &payload_size_net, 4);
        
        // Append payload
        wire_response.insert(wire_response.end(), response_payload.begin(), response_payload.end());
        
        // Send response
        asyncWriteResponse(wire_response);
        
    } catch (const std::exception& e) {
        sendError(0x0007, std::string("Time-series query exception: ") + e.what());
    }
}

// =============================================================================
// BPMN Process Handlers
// =============================================================================

namespace {
    // Helper to parse JSON from payload buffer
    json parsePayloadJson(const std::vector<uint8_t>& payload_buffer) {
        std::string payload_str(payload_buffer.begin(), payload_buffer.end());
        return json::parse(payload_str);
    }
}

void WireProtocolServer::Session::handleBpmnStartProcess() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    if (!server_->process_graph_) {
        sendError(503, "Process engine not available");
        return;
    }

    try {
        // Parse JSON payload
        // Expected format: { "process_definition_key": "...", "variables": {...}, "business_key": "..." }
        json request = parsePayloadJson(payload_buffer_);

        std::string process_key = request.value("process_definition_key", "");
        json variables = request.value("variables", json::object());
        std::string business_key = request.value("business_key", "");

        if (process_key.empty()) {
            sendError(400, "Missing process_definition_key");
            return;
        }

        // Start process instance
        auto [status, instance_id] = server_->process_graph_->startProcess(process_key, variables);

        if (!status.ok) {
            sendError(500, "Failed to start process: " + status.message);
            return;
        }

        // Get instance state to return active tasks
        auto [get_status, instance] = server_->process_graph_->getProcessInstance(instance_id);
        
        // Build response
        json response;
        response["process_instance_id"] = instance_id;
        
        // Map process state to ProcessStatus enum
        std::string state_str = "RUNNING";
        int status_code = 0; // RUNNING
        if (get_status.ok) {
            switch (instance.state) {
                case ProcessInstance::State::RUNNING:
                    state_str = "RUNNING";
                    status_code = 0;
                    break;
                case ProcessInstance::State::COMPLETED:
                    state_str = "COMPLETED";
                    status_code = 1;
                    break;
                case ProcessInstance::State::FAILED:
                    state_str = "FAILED";
                    status_code = 2;
                    break;
                case ProcessInstance::State::SUSPENDED:
                    state_str = "SUSPENDED";
                    status_code = 3;
                    break;
                case ProcessInstance::State::TERMINATED:
                    state_str = "TERMINATED";
                    status_code = 4;
                    break;
                default:
                    state_str = "RUNNING";
                    status_code = 0;
            }

            // Extract active task IDs from tokens
            json active_tasks = json::array();
            for (const auto& token : instance.tokens) {
                if (token.state == ProcessToken::State::READY || 
                    token.state == ProcessToken::State::ACTIVE) {
                    active_tasks.push_back(instance_id + ":" + token.current_node);
                }
            }
            response["active_task_ids"] = active_tasks;
        } else {
            response["active_task_ids"] = json::array();
        }
        
        response["status"] = status_code;
        response["status_string"] = state_str;

        // Serialize response
        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(500, std::string("Internal error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleBpmnTaskComplete() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    if (!server_->process_graph_) {
        sendError(503, "Process engine not available");
        return;
    }

    try {
        // Parse JSON payload
        // Expected format: { "task_id": "...", "variables": {...}, "assignee": "..." }
        json request = parsePayloadJson(payload_buffer_);

        std::string task_id = request.value("task_id", "");
        json variables = request.value("variables", json::object());
        std::string assignee = request.value("assignee", username_);

        if (task_id.empty()) {
            sendError(400, "Missing task_id");
            return;
        }

        // Task ID format: typically "token_id" or we need to parse instance_id and node_id
        // For simplicity, we'll assume task_id format is "instance_id:node_id" or just token_id
        // Let's extract instance_id from the task if it contains ':'
        
        std::string instance_id;
        std::string node_id;
        
        size_t colon_pos = task_id.find(':');
        if (colon_pos != std::string::npos) {
            instance_id = task_id.substr(0, colon_pos);
            node_id = task_id.substr(colon_pos + 1);
        } else {
            // If no colon, treat as token_id and we need to find the instance
            // For now, return error - client should provide instance:node format
            sendError(400, "Invalid task_id format. Expected 'instance_id:node_id'");
            return;
        }

        // Complete the task
        auto status = server_->process_graph_->completeTask(instance_id, node_id, variables);

        // Build response
        json response;
        response["success"] = status.ok;
        
        if (!status.ok) {
            response["error"] = status.message;
            response["next_task_id"] = "";
        } else {
            response["error"] = "";
            
            // Try to find next active task
            auto [get_status, instance] = server_->process_graph_->getProcessInstance(instance_id);
            if (get_status.ok && !instance.tokens.empty()) {
                // Find first active token
                for (const auto& token : instance.tokens) {
                    if (token.state == ProcessToken::State::READY || 
                        token.state == ProcessToken::State::ACTIVE) {
                        response["next_task_id"] = instance_id + ":" + token.current_node;
                        break;
                    }
                }
                if (!response.contains("next_task_id")) {
                    response["next_task_id"] = "";
                }
            } else {
                response["next_task_id"] = "";
            }
        }

        // Serialize response
        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(500, std::string("Internal error: ") + e.what());
    }
}

void WireProtocolServer::Session::handleBpmnQueryInstance() {
    if (!authenticated_.load()) {
        sendError(401, "Authentication required");
        return;
    }

    if (!server_->process_graph_) {
        sendError(503, "Process engine not available");
        return;
    }

    try {
        // Parse JSON payload
        // Expected format: { "process_instance_id": "...", "include_variables": true/false, "include_history": true/false }
        json request = parsePayloadJson(payload_buffer_);

        std::string instance_id = request.value("process_instance_id", "");
        bool include_variables = request.value("include_variables", true);
        bool include_history = request.value("include_history", false);

        if (instance_id.empty()) {
            sendError(400, "Missing process_instance_id");
            return;
        }

        // Get process instance
        auto [status, instance] = server_->process_graph_->getProcessInstance(instance_id);

        if (!status.ok) {
            sendError(404, "Process instance not found: " + status.message);
            return;
        }

        // Build response
        json response;

        // Map status
        int status_code = 0;
        switch (instance.state) {
            case ProcessInstance::State::RUNNING:
                status_code = 0;
                break;
            case ProcessInstance::State::COMPLETED:
                status_code = 1;
                break;
            case ProcessInstance::State::FAILED:
                status_code = 2;
                break;
            case ProcessInstance::State::SUSPENDED:
                status_code = 3;
                break;
            case ProcessInstance::State::TERMINATED:
                status_code = 4;
                break;
            default:
                status_code = 0;
        }
        response["status"] = status_code;

        // Active tasks
        json active_tasks = json::array();
        for (const auto& token : instance.tokens) {
            if (token.state == ProcessToken::State::READY || 
                token.state == ProcessToken::State::ACTIVE) {
                json task;
                task["task_id"] = instance_id + ":" + token.current_node;
                task["task_name"] = token.current_node;
                task["task_type"] = "userTask"; // Default, could be enhanced
                task["assignee"] = ""; // Not stored in token currently
                task["created_at_ns"] = token.created_at_ms * 1000000; // Convert ms to ns
                active_tasks.push_back(task);
            }
        }
        response["active_tasks"] = active_tasks;

        // Variables
        if (include_variables) {
            response["variables"] = instance.variables;
        } else {
            response["variables"] = json::object();
        }

        // History (simplified - just list visited nodes)
        if (include_history) {
            json history = json::array();
            for (const auto& token : instance.tokens) {
                for (const auto& node : token.visited_nodes) {
                    json event;
                    event["event_type"] = "node_visited";
                    // TODO: ProcessGraphManager doesn't store individual visit timestamps per node,
                    // only token creation time. Future enhancement: add visit timestamp tracking.
                    event["timestamp_ns"] = token.created_at_ms * 1000000;
                    event["data"] = json::object();
                    event["data"]["node_id"] = node;
                    history.push_back(event);
                }
            }
            response["history"] = history;
        } else {
            response["history"] = json::array();
        }

        // Timestamps
        response["start_time_ns"] = instance.started_at_ms * 1000000; // Convert ms to ns
        if (instance.completed_at_ms.has_value()) {
            response["end_time_ns"] = instance.completed_at_ms.value() * 1000000;
        } else {
            response["end_time_ns"] = 0;
        }

        // Serialize response
        std::string response_str = response.dump();
        std::vector<uint8_t> response_data(response_str.begin(), response_str.end());
        asyncWriteResponse(response_data);

    } catch (const json::exception& e) {
        sendError(400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendError(500, std::string("Internal error: ") + e.what());
    }
}

} // namespace themis::network

