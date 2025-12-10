/**
 * ThemisDB Shard Network Latency Monitor Implementation
 */

#include "sharding/shard_latency_monitor.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <cmath>

namespace themis::sharding {

// PingMessage implementation

std::string PingMessage::toJson() const {
    nlohmann::json j;
    j["sender_id"] = sender_id;
    j["target_id"] = target_id;
    j["sequence_number"] = sequence_number;
    j["sender_timestamp"] = sender_timestamp.toJson();
    j["is_response"] = is_response;
    
    if (is_response) {
        j["receiver_timestamp"] = receiver_timestamp.toJson();
        j["response_timestamp"] = response_timestamp.toJson();
    }
    
    return j.dump();
}

std::optional<PingMessage> PingMessage::fromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        
        PingMessage msg;
        msg.sender_id = j["sender_id"].get<std::string>();
        msg.target_id = j["target_id"].get<std::string>();
        msg.sequence_number = j["sequence_number"].get<uint64_t>();
        
        auto ts_opt = TrueTimeStamp::fromJson(j["sender_timestamp"].get<std::string>());
        if (!ts_opt) return std::nullopt;
        msg.sender_timestamp = *ts_opt;
        
        msg.is_response = j["is_response"].get<bool>();
        
        if (msg.is_response) {
            auto recv_ts = TrueTimeStamp::fromJson(j["receiver_timestamp"].get<std::string>());
            auto resp_ts = TrueTimeStamp::fromJson(j["response_timestamp"].get<std::string>());
            if (!recv_ts || !resp_ts) return std::nullopt;
            
            msg.receiver_timestamp = *recv_ts;
            msg.response_timestamp = *resp_ts;
        }
        
        return msg;
    } catch (...) {
        return std::nullopt;
    }
}

// ShardLatencyMonitor implementation

ShardLatencyMonitor::ShardLatencyMonitor(
    const LatencyMonitorConfig& config,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<TrueTimeClock> truetime_clock
)
    : config_(config)
    , topology_(topology)
    , truetime_clock_(truetime_clock)
{
}

ShardLatencyMonitor::~ShardLatencyMonitor() {
    stop();
}

bool ShardLatencyMonitor::start() {
    if (running_.exchange(true)) {
        return false;  // Already running
    }
    
    // Start monitoring thread
    monitor_stop_ = false;
    monitor_thread_ = std::make_unique<std::thread>([this]() { monitorLoop(); });
    
    return true;
}

void ShardLatencyMonitor::stop() {
    if (!running_.exchange(false)) {
        return;  // Not running
    }
    
    // Stop monitoring thread
    monitor_stop_ = true;
    if (monitor_thread_ && monitor_thread_->joinable()) {
        monitor_thread_->join();
    }
    monitor_thread_.reset();
}

std::optional<ShardLatencyMeasurement> ShardLatencyMonitor::pingShard(const std::string& shard_id) {
    // Create ping request
    PingMessage request;
    request.sender_id = config_.local_shard_id;
    request.target_id = shard_id;
    request.sequence_number = next_sequence_++;
    request.is_response = false;
    
    if (truetime_clock_) {
        request.sender_timestamp = truetime_clock_->now();
    }
    
    // Record pending ping
    auto start = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_pings_[request.sequence_number] = PendingPing{
            request.sequence_number,
            shard_id,
            start,
            request.sender_timestamp
        };
    }
    
    // Send ping request
    bool sent = sendPingRequest(shard_id, request);
    if (!sent) {
        // Cleanup pending ping
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_pings_.erase(request.sequence_number);
        
        ShardLatencyMeasurement measurement;
        measurement.shard_id = shard_id;
        measurement.success = false;
        measurement.error_msg = "Failed to send ping request";
        return measurement;
    }
    
    // Wait for response (with timeout)
    auto timeout = std::chrono::milliseconds(config_.ping_timeout_ms);
    auto deadline = start + timeout;
    
    while (std::chrono::steady_clock::now() < deadline) {
        // Check if we got a response
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            if (pending_pings_.find(request.sequence_number) == pending_pings_.end()) {
                // Response received, find it in history
                std::lock_guard<std::mutex> hist_lock(history_mutex_);
                auto& history = shard_history_[shard_id];
                std::lock_guard<std::mutex> shard_lock(history.mutex);
                
                if (!history.measurements.empty()) {
                    return history.measurements.back();
                }
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Timeout
    std::lock_guard<std::mutex> lock(pending_mutex_);
    pending_pings_.erase(request.sequence_number);
    
    ShardLatencyMeasurement measurement;
    measurement.shard_id = shard_id;
    measurement.success = false;
    measurement.error_msg = "Ping timeout";
    
    recordMeasurement(shard_id, measurement);
    return measurement;
}

PingMessage ShardLatencyMonitor::handlePingRequest(const PingMessage& request) {
    // Create response
    PingMessage response;
    response.sender_id = config_.local_shard_id;
    response.target_id = request.sender_id;
    response.sequence_number = request.sequence_number;
    response.is_response = true;
    
    // Copy sender's timestamp
    response.sender_timestamp = request.sender_timestamp;
    
    // Add our timestamps
    if (truetime_clock_) {
        response.receiver_timestamp = truetime_clock_->now();
        
        // Update our clock based on sender's timestamp
        truetime_clock_->receive(request.sender_timestamp);
        
        response.response_timestamp = truetime_clock_->now();
    }
    
    return response;
}

void ShardLatencyMonitor::handlePingResponse(const PingMessage& response) {
    // Find pending ping
    std::optional<PendingPing> pending;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        auto it = pending_pings_.find(response.sequence_number);
        if (it == pending_pings_.end()) {
            return;  // Unknown or timed out
        }
        pending = it->second;
        pending_pings_.erase(it);
    }
    
    auto now = std::chrono::steady_clock::now();
    auto rtt_duration = now - pending->sent_at;
    uint64_t rtt_us = std::chrono::duration_cast<std::chrono::microseconds>(rtt_duration).count();
    
    // Create measurement
    ShardLatencyMeasurement measurement;
    measurement.shard_id = pending->target_shard;
    measurement.rtt_us = rtt_us;
    measurement.one_way_us = rtt_us / 2;
    measurement.measured_at = std::chrono::system_clock::now();
    measurement.success = true;
    
    // Record measurement
    recordMeasurement(pending->target_shard, measurement);
    
    // Update TrueTime clock if available
    if (truetime_clock_ && config_.adjust_truetime_uncertainty) {
        truetime_clock_->receive(response.response_timestamp);
    }
}

std::optional<ShardLatencyStats> ShardLatencyMonitor::getStats(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    auto it = shard_history_.find(shard_id);
    if (it == shard_history_.end()) {
        return std::nullopt;
    }
    
    return calculateStats(shard_id);
}

std::map<std::string, ShardLatencyStats> ShardLatencyMonitor::getAllStats() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    std::map<std::string, ShardLatencyStats> result;
    
    for (const auto& [shard_id, _] : shard_history_) {
        result[shard_id] = calculateStats(shard_id);
    }
    
    return result;
}

uint64_t ShardLatencyMonitor::getAverageNetworkLatency() const {
    auto all_stats = getAllStats();
    if (all_stats.empty()) {
        return 0;
    }
    
    uint64_t total = 0;
    uint64_t count = 0;
    
    for (const auto& [_, stats] : all_stats) {
        if (stats.is_reachable && stats.avg_one_way_us > 0) {
            total += stats.avg_one_way_us;
            count++;
        }
    }
    
    return count > 0 ? total / count : 0;
}

uint64_t ShardLatencyMonitor::getMaxNetworkLatency() const {
    auto all_stats = getAllStats();
    uint64_t max_latency = 0;
    
    for (const auto& [_, stats] : all_stats) {
        if (stats.is_reachable && stats.p95_one_way_us > max_latency) {
            max_latency = stats.p95_one_way_us;
        }
    }
    
    return max_latency;
}

std::optional<int64_t> ShardLatencyMonitor::estimateClockOffset(const std::string& shard_id) const {
    auto stats_opt = getStats(shard_id);
    if (!stats_opt) {
        return std::nullopt;
    }
    
    return stats_opt->estimated_clock_offset_us;
}

std::string ShardLatencyMonitor::exportPrometheusMetrics() const {
    auto all_stats = getAllStats();
    std::ostringstream oss;
    
    oss << "# HELP themis_shard_latency_rtt_us Round-trip time to shard in microseconds\n";
    oss << "# TYPE themis_shard_latency_rtt_us gauge\n";
    
    oss << "# HELP themis_shard_latency_one_way_us One-way latency to shard in microseconds\n";
    oss << "# TYPE themis_shard_latency_one_way_us gauge\n";
    
    oss << "# HELP themis_shard_ping_success_rate Success rate of pings to shard\n";
    oss << "# TYPE themis_shard_ping_success_rate gauge\n";
    
    oss << "# HELP themis_shard_reachable Whether shard is reachable (1=yes, 0=no)\n";
    oss << "# TYPE themis_shard_reachable gauge\n";
    
    for (const auto& [shard_id, stats] : all_stats) {
        // RTT metrics
        oss << "themis_shard_latency_rtt_us{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\",quantile=\"avg\"} " 
            << stats.avg_rtt_us << "\n";
        oss << "themis_shard_latency_rtt_us{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\",quantile=\"p95\"} " 
            << stats.p95_rtt_us << "\n";
        oss << "themis_shard_latency_rtt_us{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\",quantile=\"p99\"} " 
            << stats.p99_rtt_us << "\n";
        
        // One-way latency
        oss << "themis_shard_latency_one_way_us{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\",quantile=\"avg\"} " 
            << stats.avg_one_way_us << "\n";
        oss << "themis_shard_latency_one_way_us{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\",quantile=\"p95\"} " 
            << stats.p95_one_way_us << "\n";
        
        // Success rate
        oss << "themis_shard_ping_success_rate{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\"} " 
            << stats.success_rate << "\n";
        
        // Reachability
        oss << "themis_shard_reachable{local=\"" << config_.local_shard_id 
            << "\",remote=\"" << shard_id << "\"} " 
            << (stats.is_reachable ? 1 : 0) << "\n";
    }
    
    return oss.str();
}

// Private methods

void ShardLatencyMonitor::monitorLoop() {
    while (!monitor_stop_.load()) {
        // Ping all shards
        pingAllShards();
        
        // Update TrueTime uncertainty based on network measurements
        if (config_.adjust_truetime_uncertainty && truetime_clock_) {
            updateTrueTimeUncertainty();
        }
        
        // Sleep until next ping interval
        for (uint32_t i = 0; i < config_.ping_interval_ms / 100; ++i) {
            if (monitor_stop_.load()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void ShardLatencyMonitor::pingAllShards() {
    // Get all shards from topology
    auto shards = topology_->getShards();
    
    for (const auto& shard_info : shards) {
        // Skip self
        if (shard_info.shard_id == config_.local_shard_id) {
            continue;
        }
        
        // Skip unhealthy shards
        if (!shard_info.is_healthy) {
            continue;
        }
        
        // Ping asynchronously (fire and forget)
        std::thread([this, shard_id = shard_info.shard_id]() {
            pingShard(shard_id);
        }).detach();
    }
}

bool ShardLatencyMonitor::sendPingRequest(const std::string& shard_id, PingMessage& request) {
    // Get shard endpoint from topology
    auto shard_info_opt = topology_->getShard(shard_id);
    if (!shard_info_opt) {
        return false;
    }
    
    // In a real implementation, this would use HTTP/gRPC to send the ping
    // For now, this is a simplified stub
    // TODO: Integrate with RemoteExecutor or HTTP client
    
    // Simulate network send (placeholder)
    // In production, this would be:
    // auto response = http_client->post(shard_info->primary_endpoint + config_.ping_endpoint_path, request.toJson());
    
    return true;  // Assume success for now
}

void ShardLatencyMonitor::recordMeasurement(
    const std::string& shard_id,
    const ShardLatencyMeasurement& measurement
) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    auto& history = shard_history_[shard_id];
    std::lock_guard<std::mutex> shard_lock(history.mutex);
    
    // Add to history
    history.measurements.push_back(measurement);
    
    // Keep only recent measurements
    while (history.measurements.size() > config_.history_size) {
        history.measurements.pop_front();
    }
}

ShardLatencyStats ShardLatencyMonitor::calculateStats(const std::string& shard_id) const {
    ShardLatencyStats stats;
    stats.shard_id = shard_id;
    
    auto it = shard_history_.find(shard_id);
    if (it == shard_history_.end()) {
        return stats;
    }
    
    const auto& history = it->second;
    std::lock_guard<std::mutex> lock(history.mutex);
    
    if (history.measurements.empty()) {
        return stats;
    }
    
    // Collect RTT values
    std::vector<uint64_t> rtt_values;
    std::vector<uint64_t> one_way_values;
    
    for (const auto& m : history.measurements) {
        stats.total_pings++;
        if (m.success) {
            stats.successful_pings++;
            rtt_values.push_back(m.rtt_us);
            one_way_values.push_back(m.one_way_us);
        } else {
            stats.failed_pings++;
        }
    }
    
    if (rtt_values.empty()) {
        stats.is_reachable = false;
        return stats;
    }
    
    stats.is_reachable = true;
    stats.success_rate = static_cast<double>(stats.successful_pings) / stats.total_pings;
    stats.last_ping_at = history.measurements.back().measured_at;
    
    // Sort for percentile calculation
    std::sort(rtt_values.begin(), rtt_values.end());
    std::sort(one_way_values.begin(), one_way_values.end());
    
    // Calculate statistics
    stats.min_rtt_us = rtt_values.front();
    stats.max_rtt_us = rtt_values.back();
    stats.avg_rtt_us = std::accumulate(rtt_values.begin(), rtt_values.end(), 0ULL) / rtt_values.size();
    stats.median_rtt_us = calculatePercentile(rtt_values, 0.50);
    
    if (config_.enable_percentiles) {
        stats.p95_rtt_us = calculatePercentile(rtt_values, 0.95);
        stats.p99_rtt_us = calculatePercentile(rtt_values, 0.99);
    }
    
    stats.avg_one_way_us = std::accumulate(one_way_values.begin(), one_way_values.end(), 0ULL) / one_way_values.size();
    stats.p95_one_way_us = calculatePercentile(one_way_values, 0.95);
    
    // Estimate clock offset (simplified - would use Cristian's algorithm in production)
    stats.estimated_clock_offset_us = 0;
    stats.offset_uncertainty_us = stats.avg_one_way_us;
    
    return stats;
}

void ShardLatencyMonitor::updateTrueTimeUncertainty() {
    if (!truetime_clock_) {
        return;
    }
    
    // Get maximum network latency
    uint64_t max_network_latency = getMaxNetworkLatency();
    if (max_network_latency == 0) {
        return;
    }
    
    // Add network uncertainty to TrueTime base uncertainty
    // This is a simplified approach - in production, would adjust TrueTimeConfig
    // For now, this is informational only
}

uint64_t ShardLatencyMonitor::calculatePercentile(
    const std::vector<uint64_t>& values,
    double percentile
) const {
    if (values.empty()) {
        return 0;
    }
    
    size_t index = static_cast<size_t>(std::ceil(values.size() * percentile)) - 1;
    if (index >= values.size()) {
        index = values.size() - 1;
    }
    
    return values[index];
}

// LatencyAwareTrueTime implementation

LatencyAwareTrueTime::LatencyAwareTrueTime(
    std::shared_ptr<TrueTimeClock> truetime_clock,
    std::shared_ptr<ShardLatencyMonitor> latency_monitor
)
    : truetime_clock_(truetime_clock)
    , latency_monitor_(latency_monitor)
{
}

TrueTimeStamp LatencyAwareTrueTime::now() {
    auto ts = truetime_clock_->now();
    
    // Add network latency to uncertainty
    uint64_t max_network = latency_monitor_->getMaxNetworkLatency();
    if (max_network > 0) {
        ts.latest_us += max_network;
    }
    
    return ts;
}

TrueTimeStamp LatencyAwareTrueTime::nowForShard(const std::string& target_shard_id) {
    auto ts = truetime_clock_->now();
    
    // Add specific shard network latency to uncertainty
    auto stats_opt = latency_monitor_->getStats(target_shard_id);
    if (stats_opt && stats_opt->is_reachable) {
        uint64_t network_uncertainty = stats_opt->p95_one_way_us;
        ts.latest_us += network_uncertainty;
    }
    
    return ts;
}

bool LatencyAwareTrueTime::waitUntilPast(
    const TrueTimeStamp& ts,
    const std::string& target_shard_id
) {
    // Get adjusted timestamp with network latency
    TrueTimeStamp adjusted_ts = ts;
    
    if (!target_shard_id.empty()) {
        auto stats_opt = latency_monitor_->getStats(target_shard_id);
        if (stats_opt && stats_opt->is_reachable) {
            // Add network latency to wait time
            adjusted_ts.latest_us += stats_opt->p95_one_way_us;
        }
    }
    
    return truetime_clock_->waitUntilPast(adjusted_ts);
}

} // namespace themis::sharding
