/**
 * @file truetime.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/truetime.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <thread>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>

// Platform-specific includes for socket operations
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>
#endif

namespace themis::sharding {

/**
 * @brief Construct TrueTime clock and optionally start sync thread.
 * @param config TrueTime configuration.
 */
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

/** @brief Destructor stops background synchronization thread. */
TrueTime::~TrueTime() {
    stopSyncThread();
}

/**
 * @brief Return current corrected time interval with uncertainty bounds.
 * @return TrueTime interval [earliest, latest].
 */
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

/**
 * @brief Wait until timestamp is definitely before TT.now().earliest.
 * @param timestamp Target timestamp.
 */
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

/** @brief Alias for now() emphasizing uncertainty semantics. */
TTInterval TrueTime::now_with_uncertainty() const {
    // Identical to now(); provided as an explicit named method for the
    // Percolator commit-wait pattern where callers need the [earliest, latest]
    // interval to compute: wait until TT.now().earliest > commit_ts + epsilon.
    return now();
}

/** @brief Return current uncertainty epsilon. */
std::chrono::nanoseconds TrueTime::getUncertainty() const {
    return std::chrono::nanoseconds(calculateUncertainty());
}

/** @brief Return current drift estimate. */
std::chrono::nanoseconds TrueTime::getDrift() const {
    return std::chrono::nanoseconds(drift_ns_.load(std::memory_order_relaxed));
}

/** @brief Trigger immediate synchronization attempt. */
bool TrueTime::syncNow() {
    return performSync();
}

/** @brief Return JSON stats snapshot for diagnostics/monitoring. */
std::string TrueTime::getStats() const {
    std::ostringstream oss = {};
    oss << "{"
        << "\"uncertainty_us\": " << (uncertainty_ns_.load() / 1000) << ", "
        << "\"drift_us\": " << (drift_ns_.load() / 1000) << ", "
        << "\"last_sync_ns\": " << last_sync_ns_.load() << ", "
        << "\"ntp_servers\": " <<static_cast<int>(config_.ntp_servers.size())
        << "}";
    return oss.str();
}

/** @brief Start periodic synchronization thread if not already running. */
void TrueTime::startSyncThread() {
    if (sync_thread_running_.exchange(true)) {
        return; // Already running
    }
    
    sync_thread_ = std::thread(&TrueTime::syncThreadFunc, this);
}

/** @brief Stop periodic synchronization thread if running. */
void TrueTime::stopSyncThread() {
    if (!sync_thread_running_.exchange(false)) {
        return; // Not running
    }
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(sync_thread_)) {
        THEMIS_WARN("[TrueTime] sync thread did not finish within shutdown deadline; detaching.");
    }
}

/** @brief Read raw system clock in nanoseconds since epoch. */
std::chrono::nanoseconds TrueTime::getSystemTime() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}

/**
 * @brief Synchronize against configured NTP servers and update drift/uncertainty.
 * @return true if at least one server produced a valid offset.
 */
bool TrueTime::performSync() {
    if (config_.ntp_servers.empty()) {
        // No NTP servers configured, use local time with increased uncertainty
        uncertainty_ns_.store(config_.base_uncertainty_us * 1000 * 10);
        return true;
    }
    
    std::vector<int64_t> offsets = {};

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

/**
 * @brief Query one NTP server and compute local clock offset.
 * @param server NTP server hostname/address.
 * @param offset Output offset in nanoseconds.
 * @return true on successful query and offset calculation.
 */
bool TrueTime::queryNTPServer(const std::string& server, int64_t& offset) {
    // Implement SNTP (Simple Network Time Protocol) client - RFC 4330
    // This is a simplified version suitable for time synchronization
    
#ifdef _WIN32
    using SocketHandle = SOCKET;
    static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
    using SocketHandle = int;
    static constexpr SocketHandle kInvalidSocket = -1;
#endif

    // RAII wrapper for socket to ensure cleanup
    class SocketGuard {
        SocketHandle fd_;
    public:
        explicit SocketGuard(SocketHandle fd) : fd_(fd) {}
        ~SocketGuard() {
            if (fd_ != kInvalidSocket) {
#ifdef _WIN32
                closesocket(fd_);
#else
                close(fd_);
#endif
            }
        }
        SocketHandle get() const { return fd_; }
        // Prevent copying
        SocketGuard(const SocketGuard&) = delete;
        SocketGuard& operator=(const SocketGuard&) = delete;
    };
    
    try {
        // NTP packet structure (48 bytes)
        struct NTPPacket {
            uint8_t li_vn_mode;      // Leap Indicator (2 bits) + Version (3 bits) + Mode (3 bits)
            uint8_t stratum;         // Stratum level of the local clock
            uint8_t poll;            // Maximum interval between successive messages
            uint8_t precision;       // Precision of the local clock
            uint32_t rootDelay;      // Total round trip delay to reference clock
            uint32_t rootDispersion; // Total dispersion to reference clock
            uint32_t refId;          // Reference clock identifier
            uint32_t refTm_s;        // Reference timestamp (seconds)
            uint32_t refTm_f;        // Reference timestamp (fraction)
            uint32_t origTm_s;       // Originate timestamp (seconds)
            uint32_t origTm_f;       // Originate timestamp (fraction)
            uint32_t rxTm_s;         // Receive timestamp (seconds)
            uint32_t rxTm_f;         // Receive timestamp (fraction)
            uint32_t txTm_s;         // Transmit timestamp (seconds)
            uint32_t txTm_f;         // Transmit timestamp (fraction)
        };
        
        // Create socket with RAII cleanup
        SocketHandle sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockfd == kInvalidSocket) {
            return false;
        }
        SocketGuard socketGuard(sockfd);
        
        // Set socket timeout (5 seconds) - platform-specific
#ifdef _WIN32
        DWORD timeout = 5000; // milliseconds on Windows
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            return false;
        }
#else
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            return false;
        }
#endif
        
        // Resolve server address using thread-safe getaddrinfo
        struct addrinfo hints, *result = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_DGRAM;  // UDP
        hints.ai_protocol = IPPROTO_UDP;
        
        // Use NTP port (123)
        if (getaddrinfo(server.c_str(), "123", &hints, &result) != 0 || !result) {
            return false;
        }
        
        // Copy the resolved address
        struct sockaddr_in serv_addr;
        memcpy(&serv_addr, result->ai_addr, result->ai_addrlen);
        
        // Free the result
        freeaddrinfo(result);
        
        // Prepare NTP request packet
        NTPPacket packet;
        memset(&packet, 0, sizeof(packet));
        
        // Set NTP version 4 and mode 3 (client) - RFC 4330
        packet.li_vn_mode = 0x23; // LI=0, VN=4 (SNTPv4), Mode=3 (client)
        
        // Record T1 (client transmit time)
        auto t1 = std::chrono::system_clock::now();
        auto t1_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            t1.time_since_epoch()
        ).count();
        
        // Send request
        if (sendto(sockfd, reinterpret_cast<const char*>(&packet), static_cast<int>(sizeof(packet)), 0,
                   reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
            return false;
        }
        
        // Receive response
#ifdef _WIN32
        int len = sizeof(serv_addr);
        int n = recvfrom(sockfd, reinterpret_cast<char*>(&packet), static_cast<int>(sizeof(packet)), 0,
                         reinterpret_cast<struct sockaddr*>(&serv_addr), &len);
#else
        socklen_t len = sizeof(serv_addr);
        ssize_t n = recvfrom(sockfd, &packet, sizeof(packet), 0,
                             reinterpret_cast<struct sockaddr*>(&serv_addr), &len);
#endif
        
        // Record T4 (client receive time)
        auto t4 = std::chrono::system_clock::now();
        auto t4_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            t4.time_since_epoch()
        ).count();
        
        // Socket will be automatically closed by SocketGuard destructor
        
#ifdef _WIN32
        if (n < static_cast<int>(sizeof(packet))) {
            return false;
        }
#else
        if (n < static_cast<ssize_t>(sizeof(packet))) {
            return false;
        }
#endif
        
        // Extract T2 (server receive time) and T3 (server transmit time)
        // NTP timestamps are in seconds since 1900-01-01, with 32-bit fraction
        // Convert to nanoseconds since Unix epoch (1970-01-01)
        
        // NTP_EPOCH_OFFSET = Number of seconds between NTP epoch (1900-01-01) and Unix epoch (1970-01-01)
        // Calculated as: 70 years * 365.25 days/year * 24 hours/day * 3600 seconds/hour = 2,208,988,800 seconds
        // This accounts for 17 leap years between 1900 and 1970
        const uint64_t NTP_EPOCH_OFFSET = 2208988800;
        
        uint64_t t2_s = ntohl(packet.rxTm_s);
        uint64_t t2_f = ntohl(packet.rxTm_f);
        uint64_t t3_s = ntohl(packet.txTm_s);
        uint64_t t3_f = ntohl(packet.txTm_f);
        
        // Validate NTP timestamps to prevent integer overflow
        if (t2_s < NTP_EPOCH_OFFSET || t3_s < NTP_EPOCH_OFFSET) {
            // Invalid NTP response - timestamps before NTP epoch
            return false;
        }
        
        // Convert NTP timestamps to nanoseconds since Unix epoch
        int64_t t2_ns = static_cast<int64_t>(
            ((t2_s - NTP_EPOCH_OFFSET) * 1000000000) +
            ((t2_f * 1000000000) >> 32)
        );
        
        int64_t t3_ns = static_cast<int64_t>(
            ((t3_s - NTP_EPOCH_OFFSET) * 1000000000) +
            ((t3_f * 1000000000) >> 32)
        );
        
        // Calculate offset using the standard NTP formula:
        // offset = ((T2 - T1) + (T3 - T4)) / 2
        offset = ((t2_ns - t1_ns) + (t3_ns - t4_ns)) / 2;
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/** @brief Compute time uncertainty growth since last successful sync. */
uint64_t TrueTime::calculateUncertainty() const {
    uint64_t base_uncertainty = uncertainty_ns_.load(std::memory_order_relaxed);
    
    // Calculate time since last sync
    uint64_t now_ns = static_cast<uint64_t>(getSystemTime().count());
    uint64_t last_sync = last_sync_ns_.load(std::memory_order_relaxed);
    uint64_t time_since_sync_ns = now_ns > last_sync ? now_ns - last_sync : 0;
    
    // Uncertainty grows with time since last sync
    // Assume 1us of drift per second (1us per 1,000,000us = 1us per 1e9 ns)
    uint64_t drift_uncertainty = time_since_sync_ns / 1000000; // Convert ns to drift in ns
    
    uint64_t total_uncertainty = base_uncertainty + drift_uncertainty;
    
    // Cap at max drift
    const auto max_drift_ns = static_cast<uint64_t>(config_.max_drift_us) * static_cast<uint64_t>(1000);
    return std::min<uint64_t>(total_uncertainty, max_drift_ns);
}

/** @brief Background synchronization loop running at configured interval. */
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


