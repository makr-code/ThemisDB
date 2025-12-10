/**
 * ThemisDB TrueTime-Inspired Clock Implementation
 */

#include "sharding/truetime_clock.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#include <cmath>
#include <algorithm>

#ifdef __linux__
#include <sys/time.h>
#include <sys/timex.h>
#endif

namespace themis::sharding {

// TrueTimeStamp implementation

int TrueTimeStamp::compare(const TrueTimeStamp& other) const {
    // If definitely before
    if (definitelyBefore(other)) {
        return -1;
    }
    // If definitely after
    if (definitelyAfter(other)) {
        return 1;
    }
    // Uncertain/concurrent - use logical counter as tiebreaker
    if (logical < other.logical) {
        return -1;
    }
    if (logical > other.logical) {
        return 1;
    }
    // Same logical counter - use node_id for deterministic ordering
    if (node_id < other.node_id) {
        return -1;
    }
    if (node_id > other.node_id) {
        return 1;
    }
    return 0;
}

std::string TrueTimeStamp::toJson() const {
    nlohmann::json j;
    j["earliest_us"] = earliest_us;
    j["latest_us"] = latest_us;
    j["logical"] = logical;
    j["node_id"] = node_id;
    return j.dump();
}

std::optional<TrueTimeStamp> TrueTimeStamp::fromJson(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        TrueTimeStamp ts;
        ts.earliest_us = j["earliest_us"].get<uint64_t>();
        ts.latest_us = j["latest_us"].get<uint64_t>();
        ts.logical = j["logical"].get<uint64_t>();
        ts.node_id = j["node_id"].get<std::string>();
        return ts;
    } catch (...) {
        return std::nullopt;
    }
}

bool TrueTimeStamp::operator<(const TrueTimeStamp& other) const {
    return compare(other) < 0;
}

bool TrueTimeStamp::operator>(const TrueTimeStamp& other) const {
    return compare(other) > 0;
}

bool TrueTimeStamp::operator==(const TrueTimeStamp& other) const {
    return earliest_us == other.earliest_us &&
           latest_us == other.latest_us &&
           logical == other.logical &&
           node_id == other.node_id;
}

// TrueTimeClock implementation

TrueTimeClock::TrueTimeClock(const TrueTimeConfig& config)
    : config_(config)
{
    current_uncertainty_us_ = config_.base_uncertainty_us;
}

TrueTimeClock::~TrueTimeClock() {
    stop();
}

bool TrueTimeClock::start() {
    if (running_.exchange(true)) {
        return false;  // Already running
    }
    
    // Initial sync
    performSync();
    
    // Start background sync thread
    sync_thread_stop_ = false;
    sync_thread_ = std::make_unique<std::thread>([this]() { syncLoop(); });
    
    return true;
}

void TrueTimeClock::stop() {
    if (!running_.exchange(false)) {
        return;  // Not running
    }
    
    // Stop sync thread
    sync_thread_stop_ = true;
    if (sync_thread_ && sync_thread_->joinable()) {
        sync_thread_->join();
    }
    sync_thread_.reset();
}

TrueTimeStamp TrueTimeClock::now() {
    std::lock_guard<std::mutex> lock(clock_mutex_);
    
    // Get current physical time
    uint64_t physical_us = getPhysicalTimeUs();
    
    // Apply clock offset from synchronization
    physical_us += clock_offset_us_.load();
    
    // Increment logical counter if same physical time
    if (physical_us == last_physical_us_.load()) {
        logical_counter_++;
    } else {
        last_physical_us_ = physical_us;
        logical_counter_ = 0;
    }
    
    // Calculate uncertainty
    uint64_t uncertainty = calculateUncertainty();
    current_uncertainty_us_ = uncertainty;
    
    // Create timestamp with uncertainty interval
    TrueTimeStamp ts;
    ts.earliest_us = physical_us - uncertainty;
    ts.latest_us = physical_us + uncertainty;
    ts.logical = logical_counter_.load();
    ts.node_id = config_.node_id;
    
    return ts;
}

TrueTimeStamp TrueTimeClock::after(uint64_t physical_us) {
    auto ts = now();
    
    // Ensure earliest is after requested time
    if (ts.earliest_us < physical_us) {
        uint64_t diff = physical_us - ts.earliest_us;
        ts.earliest_us = physical_us;
        ts.latest_us += diff;
        ts.logical++;
    }
    
    return ts;
}

bool TrueTimeClock::waitUntilPast(const TrueTimeStamp& ts) {
    if (!config_.enable_commit_wait) {
        return true;  // Commit-wait disabled
    }
    
    auto now_ts = now();
    
    // If timestamp is definitely in the past, no need to wait
    if (ts.definitelyBefore(now_ts)) {
        return true;
    }
    
    // Calculate how long to wait
    uint64_t wait_us = 0;
    if (ts.latest_us > now_ts.earliest_us) {
        wait_us = ts.latest_us - now_ts.earliest_us;
        // Add safety margin
        wait_us += ts.uncertainty() * config_.commit_wait_multiplier;
    }
    
    // Cap wait time at max uncertainty
    wait_us = std::min(wait_us, config_.max_uncertainty_us);
    
    // Perform the wait
    if (wait_us > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(wait_us));
    }
    
    // Verify timestamp is now in the past
    auto after_wait = now();
    return ts.definitelyBefore(after_wait);
}

TrueTimeStamp TrueTimeClock::receive(const TrueTimeStamp& received) {
    std::lock_guard<std::mutex> lock(clock_mutex_);
    
    // Get current time
    uint64_t physical_us = getPhysicalTimeUs() + clock_offset_us_.load();
    
    // Update based on received timestamp (HLC semantics)
    uint64_t new_physical = std::max(physical_us, received.midpoint());
    
    // Update logical counter
    if (new_physical == last_physical_us_.load()) {
        logical_counter_ = std::max(logical_counter_.load(), received.logical) + 1;
    } else {
        last_physical_us_ = new_physical;
        if (new_physical == received.midpoint()) {
            logical_counter_ = received.logical + 1;
        } else {
            logical_counter_ = 0;
        }
    }
    
    // Track maximum observed skew
    uint64_t skew = std::abs(static_cast<int64_t>(received.midpoint() - physical_us));
    uint64_t current_max = max_observed_skew_us_.load();
    if (skew > current_max) {
        max_observed_skew_us_ = skew;
    }
    
    // Create new timestamp
    uint64_t uncertainty = calculateUncertainty();
    TrueTimeStamp ts;
    ts.earliest_us = new_physical - uncertainty;
    ts.latest_us = new_physical + uncertainty;
    ts.logical = logical_counter_.load();
    ts.node_id = config_.node_id;
    
    return ts;
}

bool TrueTimeClock::syncNow() {
    return performSync();
}

uint64_t TrueTimeClock::getCurrentUncertainty() const {
    return current_uncertainty_us_.load();
}

ClockSyncStats TrueTimeClock::getStats() const {
    ClockSyncStats stats;
    stats.sync_count = sync_count_.load();
    stats.sync_failures = sync_failures_.load();
    stats.last_sync_us = last_sync_us_.load();
    stats.clock_offset_us = clock_offset_us_.load();
    stats.current_uncertainty_us = current_uncertainty_us_.load();
    stats.max_observed_skew_us = max_observed_skew_us_.load();
    stats.drift_rate_ppm = drift_rate_ppm_.load();
    
    switch (config_.source) {
        case ClockSource::NTP:
            stats.sync_source = "NTP";
            break;
        case ClockSource::PTP:
            stats.sync_source = "PTP";
            break;
        case ClockSource::GPS:
            stats.sync_source = "GPS";
            break;
        case ClockSource::ATOMIC:
            stats.sync_source = "ATOMIC";
            break;
        default:
            stats.sync_source = "SYSTEM_CLOCK";
    }
    
    return stats;
}

std::string TrueTimeClock::exportPrometheusMetrics() const {
    auto stats = getStats();
    std::ostringstream oss;
    
    oss << "# HELP themis_truetime_sync_count Total clock synchronizations\n";
    oss << "# TYPE themis_truetime_sync_count counter\n";
    oss << "themis_truetime_sync_count{node=\"" << config_.node_id << "\"} " 
        << stats.sync_count << "\n";
    
    oss << "# HELP themis_truetime_sync_failures Failed synchronizations\n";
    oss << "# TYPE themis_truetime_sync_failures counter\n";
    oss << "themis_truetime_sync_failures{node=\"" << config_.node_id << "\"} " 
        << stats.sync_failures << "\n";
    
    oss << "# HELP themis_truetime_clock_offset_us Clock offset in microseconds\n";
    oss << "# TYPE themis_truetime_clock_offset_us gauge\n";
    oss << "themis_truetime_clock_offset_us{node=\"" << config_.node_id << "\"} " 
        << stats.clock_offset_us << "\n";
    
    oss << "# HELP themis_truetime_uncertainty_us Current uncertainty interval in microseconds\n";
    oss << "# TYPE themis_truetime_uncertainty_us gauge\n";
    oss << "themis_truetime_uncertainty_us{node=\"" << config_.node_id << "\"} " 
        << stats.current_uncertainty_us << "\n";
    
    oss << "# HELP themis_truetime_max_skew_us Maximum observed clock skew in microseconds\n";
    oss << "# TYPE themis_truetime_max_skew_us gauge\n";
    oss << "themis_truetime_max_skew_us{node=\"" << config_.node_id << "\"} " 
        << stats.max_observed_skew_us << "\n";
    
    oss << "# HELP themis_truetime_drift_rate_ppm Clock drift rate in parts per million\n";
    oss << "# TYPE themis_truetime_drift_rate_ppm gauge\n";
    oss << "themis_truetime_drift_rate_ppm{node=\"" << config_.node_id << "\"} " 
        << stats.drift_rate_ppm << "\n";
    
    return oss.str();
}

// Private methods

void TrueTimeClock::syncLoop() {
    while (!sync_thread_stop_.load()) {
        // Perform synchronization
        performSync();
        
        // Sleep until next sync interval
        for (uint32_t i = 0; i < config_.sync_interval_ms / 100; ++i) {
            if (sync_thread_stop_.load()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool TrueTimeClock::performSync() {
    uint64_t sync_start_us = getPhysicalTimeUs();
    bool success = false;
    
    // Try to sync based on configured source
    switch (config_.source) {
        case ClockSource::NTP:
            success = syncWithNTP();
            break;
        case ClockSource::PTP:
            success = syncWithPTP();
            break;
        case ClockSource::GPS:
            success = syncWithGPS();
            break;
        case ClockSource::ATOMIC:
        case ClockSource::SYSTEM_CLOCK:
        default:
            // No external sync needed
            success = true;
            clock_offset_us_ = 0;
            break;
    }
    
    uint64_t sync_end_us = getPhysicalTimeUs();
    last_sync_us_ = sync_end_us;
    
    if (success) {
        sync_count_++;
        
        // Update drift rate if we have a previous sync
        if (sync_count_.load() > 1) {
            uint64_t elapsed_us = sync_end_us - last_sync_us_.load();
            updateDriftRate(clock_offset_us_.load(), elapsed_us);
        }
    } else {
        sync_failures_++;
    }
    
    return success;
}

bool TrueTimeClock::syncWithNTP() {
    // Simplified NTP sync simulation
    // In production, this would use actual NTP protocol (e.g., libntpclient)
    
    // For now, assume local clock is reasonably accurate
    // In production:
    // 1. Send NTP request to configured server
    // 2. Measure round-trip time
    // 3. Calculate offset and uncertainty from RTT
    // 4. Update clock_offset_us_ and current_uncertainty_us_
    
    // Placeholder implementation
    clock_offset_us_ = 0;
    
    // Uncertainty based on network RTT (assume ~1ms RTT)
    uint64_t network_uncertainty = 500;  // ±500µs for 1ms RTT
    uint64_t total_uncertainty = config_.base_uncertainty_us + network_uncertainty;
    current_uncertainty_us_ = std::min(total_uncertainty, config_.max_uncertainty_us);
    
    return true;
}

bool TrueTimeClock::syncWithPTP() {
    // Simplified PTP sync simulation
    // In production, this would use IEEE 1588 PTP protocol
    
#ifdef __linux__
    // Could use Linux PTP subsystem
    // For now, similar to NTP but with better precision
    clock_offset_us_ = 0;
    
    // PTP can achieve sub-microsecond accuracy on local networks
    uint64_t ptp_uncertainty = 50;  // ±50µs
    uint64_t total_uncertainty = config_.base_uncertainty_us + ptp_uncertainty;
    current_uncertainty_us_ = std::min(total_uncertainty, config_.max_uncertainty_us);
    
    return true;
#else
    // PTP not available on this platform
    return syncWithNTP();
#endif
}

bool TrueTimeClock::syncWithGPS() {
    // GPS time source (if hardware available)
    // This would require GPS receiver hardware and driver
    
    // Fallback to NTP for now
    return syncWithNTP();
}

uint64_t TrueTimeClock::getPhysicalTimeUs() const {
    auto now = std::chrono::system_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count();
    return static_cast<uint64_t>(us);
}

uint64_t TrueTimeClock::calculateUncertainty() const {
    // Base uncertainty
    uint64_t uncertainty = current_uncertainty_us_.load();
    
    // Add uncertainty from clock drift since last sync
    uint64_t time_since_sync_us = getPhysicalTimeUs() - last_sync_us_.load();
    if (time_since_sync_us > 0) {
        // Apply drift rate (parts per million)
        double drift_ppm = drift_rate_ppm_.load();
        uint64_t drift_uncertainty = static_cast<uint64_t>(
            time_since_sync_us * drift_ppm / 1000000.0
        );
        uncertainty += drift_uncertainty;
    }
    
    // Cap at maximum
    return std::min(uncertainty, config_.max_uncertainty_us);
}

void TrueTimeClock::updateDriftRate(int64_t offset_us, uint64_t elapsed_us) {
    if (elapsed_us == 0) {
        return;
    }
    
    // Calculate drift rate in parts per million
    double drift_ppm = (std::abs(offset_us) * 1000000.0) / elapsed_us;
    
    // Exponential moving average
    double current = drift_rate_ppm_.load();
    double alpha = 0.2;  // Smoothing factor
    double new_drift = alpha * drift_ppm + (1.0 - alpha) * current;
    
    drift_rate_ppm_ = new_drift;
}

// CommitWaitHelper implementation

bool CommitWaitHelper::waitForCommit(
    TrueTimeClock& clock,
    const TrueTimeStamp& commit_ts,
    uint32_t timeout_ms
) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        // Check if timestamp is in the past
        if (clock.waitUntilPast(commit_ts)) {
            return true;
        }
        
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= std::chrono::milliseconds(timeout_ms)) {
            return false;
        }
        
        // Small sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

uint64_t CommitWaitHelper::calculateWaitDuration(
    const TrueTimeStamp& commit_ts,
    const TrueTimeStamp& now_ts
) {
    // Wait until commit timestamp is definitely in the past
    if (commit_ts.definitelyBefore(now_ts)) {
        return 0;
    }
    
    // Wait for the difference plus uncertainty
    uint64_t wait_us = 0;
    if (commit_ts.latest_us > now_ts.earliest_us) {
        wait_us = commit_ts.latest_us - now_ts.earliest_us;
        wait_us += commit_ts.uncertainty();
    }
    
    return wait_us;
}

} // namespace themis::sharding
