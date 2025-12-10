// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/truetime.h"
#include <thread>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace themis::sharding {

TrueTime::TrueTime(const Config& config)
    : config_(config)
    , uncertainty_ns_(config.base_uncertainty_us * 1000)
    , drift_ns_(0)
    , last_sync_ns_(0)
{
    // Initialize last sync time
    last_sync_ns_.store(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    
    // Start sync thread if NTP servers configured
    if (!config_.ntp_servers.empty()) {
        startSyncThread();
    }
}

TrueTime::~TrueTime() {
    stopSyncThread();
}

TTInterval TrueTime::now() const {
    auto system_time = getSystemTime();
    
    // Apply drift correction
    int64_t drift = drift_ns_.load(std::memory_order_relaxed);
    auto corrected_time = system_time + std::chrono::nanoseconds(drift);
    
    // Calculate current uncertainty
    uint64_t epsilon = calculateUncertainty();
    
    return TTInterval(
        corrected_time - std::chrono::nanoseconds(epsilon),
        corrected_time + std::chrono::nanoseconds(epsilon)
    );
}

void TrueTime::waitUntil(std::chrono::nanoseconds timestamp) {
    // Wait until timestamp is definitely in the past
    // i.e., timestamp < now().earliest
    
    while (true) {
        auto current = now();
        
        // If timestamp is definitely before current time, we're done
        if (timestamp < current.earliest) {
            break;
        }
        
        // Calculate how long to wait
        auto wait_duration = timestamp - current.earliest;
        
        // Add the uncertainty to ensure we wait long enough
        wait_duration += current.uncertainty();
        
        // Sleep for the required duration
        if (wait_duration.count() > 0) {
            if (config_.enable_wait_optimization && wait_duration > std::chrono::milliseconds(1)) {
                // For longer waits, sleep in chunks to allow interruption
                auto chunk = std::min(
                    wait_duration, 
                    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(100))
                );
                std::this_thread::sleep_for(chunk);
            } else {
                std::this_thread::sleep_for(wait_duration);
            }
        } else {
            // Very short wait, just yield
            std::this_thread::yield();
        }
    }
}

std::chrono::nanoseconds TrueTime::getUncertainty() const {
    return std::chrono::nanoseconds(calculateUncertainty());
}

std::chrono::nanoseconds TrueTime::getDrift() const {
    return std::chrono::nanoseconds(drift_ns_.load(std::memory_order_relaxed));
}

bool TrueTime::syncNow() {
    return performSync();
}

std::string TrueTime::getStats() const {
    std::ostringstream oss;
    oss << "{"
        << "\"uncertainty_us\": " << (uncertainty_ns_.load() / 1000) << ", "
        << "\"drift_us\": " << (drift_ns_.load() / 1000) << ", "
        << "\"last_sync_ns\": " << last_sync_ns_.load() << ", "
        << "\"ntp_servers\": " << config_.ntp_servers.size()
        << "}";
    return oss.str();
}

void TrueTime::startSyncThread() {
    if (sync_thread_running_.exchange(true)) {
        return; // Already running
    }
    
    sync_thread_ = std::thread(&TrueTime::syncThreadFunc, this);
}

void TrueTime::stopSyncThread() {
    if (!sync_thread_running_.exchange(false)) {
        return; // Not running
    }
    
    if (sync_thread_.joinable()) {
        sync_thread_.join();
    }
}

std::chrono::nanoseconds TrueTime::getSystemTime() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}

bool TrueTime::performSync() {
    if (config_.ntp_servers.empty()) {
        // No NTP servers configured, use local time with increased uncertainty
        uncertainty_ns_.store(config_.base_uncertainty_us * 1000 * 10);
        return true;
    }
    
    std::vector<int64_t> offsets;
    offsets.reserve(config_.ntp_servers.size());
    
    // Query all NTP servers
    for (const auto& server : config_.ntp_servers) {
        int64_t offset = 0;
        if (queryNTPServer(server, offset)) {
            offsets.push_back(offset);
        }
    }
    
    if (offsets.empty()) {
        // Failed to sync with any server, increase uncertainty
        uint64_t current_uncertainty = uncertainty_ns_.load();
        uncertainty_ns_.store(std::min(
            current_uncertainty * 2,
            config_.max_drift_us * 1000
        ));
        return false;
    }
    
    // Use median offset to reduce impact of outliers
    std::sort(offsets.begin(), offsets.end());
    int64_t median_offset = offsets[offsets.size() / 2];
    
    // Update drift estimate
    drift_ns_.store(median_offset, std::memory_order_relaxed);
    
    // Calculate uncertainty from offset spread
    int64_t min_offset = offsets.front();
    int64_t max_offset = offsets.back();
    uint64_t spread = std::abs(max_offset - min_offset);
    
    // Uncertainty is base uncertainty plus half the spread
    uint64_t new_uncertainty = config_.base_uncertainty_us * 1000 + spread / 2;
    uncertainty_ns_.store(new_uncertainty, std::memory_order_relaxed);
    
    // Update last sync time
    last_sync_ns_.store(getSystemTime().count(), std::memory_order_relaxed);
    
    return true;
}

bool TrueTime::queryNTPServer(const std::string& server, int64_t& offset) {
    // TODO: Implement actual NTP protocol
    // For now, this is a placeholder that simulates NTP query
    // In production, this would use SNTP/NTP protocol (RFC 5905)
    
    // Simulate network delay and server processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Simulate small random offset (for testing)
    // In production, this would calculate actual offset from NTP server
    offset = 0; // Assume local time is accurate for now
    
    return true; // Simulate success
}

uint64_t TrueTime::calculateUncertainty() const {
    uint64_t base_uncertainty = uncertainty_ns_.load(std::memory_order_relaxed);
    
    // Calculate time since last sync
    auto now_ns = getSystemTime().count();
    uint64_t last_sync = last_sync_ns_.load(std::memory_order_relaxed);
    uint64_t time_since_sync_ns = now_ns > last_sync ? now_ns - last_sync : 0;
    
    // Uncertainty grows with time since last sync
    // Assume 1us of drift per second
    uint64_t drift_uncertainty = time_since_sync_ns / 1000000; // Convert ns to us, then to drift
    
    uint64_t total_uncertainty = base_uncertainty + drift_uncertainty;
    
    // Cap at max drift
    return std::min(total_uncertainty, config_.max_drift_us * 1000);
}

void TrueTime::syncThreadFunc() {
    while (sync_thread_running_.load()) {
        // Perform sync
        performSync();
        
        // Sleep until next sync interval
        for (uint64_t i = 0; i < config_.sync_interval_s && sync_thread_running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace themis::sharding
