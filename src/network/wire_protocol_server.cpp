// ThemisDB Wire Protocol Server Implementation
// Binary protocol for high-performance native client communication

#include "network/wire_protocol_server.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "index/process_graph.h"
#include "transaction/transaction_manager.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <cstring>

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
    std::shared_ptr<ProcessGraphManager> process_graph)
    : config_(config)
    , storage_(storage)
    , secondary_index_(secondary_index)
    , graph_index_(graph_index)
    , vector_index_(vector_index)
    , tx_manager_(tx_manager)
    , process_graph_(process_graph)
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
// BPMN Process Handlers
// =============================================================================

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
        std::string payload_str(payload_buffer_.begin(), payload_buffer_.end());
        json request = json::parse(payload_str);

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
                    active_tasks.push_back(token.token_id);
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
        std::string payload_str(payload_buffer_.begin(), payload_buffer_.end());
        json request = json::parse(payload_str);

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
        std::string payload_str(payload_buffer_.begin(), payload_buffer_.end());
        json request = json::parse(payload_str);

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

