/**
 * @file health_check.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/health_check.h"
#include <stdexcept>
#include "sharding/mtls_client.h"
#include "utils/thread_join_utils.h"
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace themis {
namespace sharding {

/** @brief Construct health-check system with immutable runtime config. */
HealthCheckSystem::HealthCheckSystem(const Config& config)
    : config_(config) {
}

/** @brief Destructor stops periodic checks and joins worker if needed. */
HealthCheckSystem::~HealthCheckSystem() {
    stopPeriodicChecks();
}

/** @brief Perform complete shard health assessment (cert/storage/network). */
ShardHealthInfo HealthCheckSystem::checkShardHealth(const std::string& shard_id, 
                                                     const std::string& endpoint,
                                                     const std::string& cert_path) {
    ShardHealthInfo info;
    info.shard_id = shard_id;
    info.status = HealthStatus::HEALTHY;

    // Check certificate validity
    info.cert_valid = checkCertificateValidity(cert_path, info.cert_expiry_seconds);
    if (!info.cert_valid) {
        info.errors.push_back("Certificate invalid");
        info.status = HealthStatus::CRITICAL;
    } else if (info.cert_expiry_seconds < config_.cert_expiry_warning_days * 86400) {
        info.warnings.push_back("Certificate expires in " + std::to_string(info.cert_expiry_seconds / 86400) + " days");
        if (info.status == HealthStatus::HEALTHY) {
            info.status = HealthStatus::DEGRADED;
        }
    }

    // Check storage capacity
    info.storage_ok = checkStorageCapacity(endpoint, info.storage_usage_percent);
    if (info.storage_usage_percent >= config_.storage_critical_percent) {
        info.errors.push_back("Storage critical: " + std::to_string(info.storage_usage_percent) + "%");
        info.status = HealthStatus::CRITICAL;
    } else if (info.storage_usage_percent >= config_.storage_warning_percent) {
        info.warnings.push_back("Storage high: " + std::to_string(info.storage_usage_percent) + "%");
        if (info.status == HealthStatus::HEALTHY) {
            info.status = HealthStatus::DEGRADED;
        }
    }

    // Check network connectivity
    info.network_ok = checkNetworkConnectivity(endpoint, info.response_time_ms);
    if (!info.network_ok) {
        info.errors.push_back("Network unreachable");
        info.status = HealthStatus::CRITICAL;
    } else if (info.response_time_ms >= config_.response_time_unhealthy_ms) {
        info.errors.push_back("Response time too high: " + std::to_string(info.response_time_ms) + "ms");
        info.status = HealthStatus::UNHEALTHY;
    } else if (info.response_time_ms >= config_.response_time_degraded_ms) {
        info.warnings.push_back("Response time degraded: " + std::to_string(info.response_time_ms) + "ms");
        if (info.status == HealthStatus::HEALTHY) {
            info.status = HealthStatus::DEGRADED;
        }
    }

    return info;
}

/** @brief Execute health checks over all shards and aggregate cluster status. */
ClusterHealthInfo HealthCheckSystem::checkClusterHealth(const std::map<std::string, std::string>& shard_endpoints) {
    ClusterHealthInfo cluster_info;
    cluster_info.total_shards = static_cast<int>(shard_endpoints.size());
    cluster_info.healthy_shards = 0;
    cluster_info.degraded_shards = 0;
    cluster_info.unhealthy_shards = 0;
    cluster_info.critical_shards = 0;

    // Check each shard
    for (const auto& [shard_id, endpoint] : shard_endpoints) {
        std::string cert_path = "/etc/themis/pki/" + shard_id + ".crt";  // Default path
        auto shard_health = checkShardHealth(shard_id, endpoint, cert_path);
        cluster_info.shard_health.push_back(shard_health);

        switch (shard_health.status) {
            case HealthStatus::HEALTHY:
                cluster_info.healthy_shards++;
                break;
            case HealthStatus::DEGRADED:
                cluster_info.degraded_shards++;
                break;
            case HealthStatus::UNHEALTHY:
                cluster_info.unhealthy_shards++;
                break;
            case HealthStatus::CRITICAL:
                cluster_info.critical_shards++;
                break;
        }
    }

    // Check quorum
    cluster_info.has_quorum = hasQuorum(cluster_info.healthy_shards, cluster_info.total_shards);
    if (!cluster_info.has_quorum) {
        cluster_info.cluster_warnings.push_back("No quorum - less than 50% shards healthy");
    }

    // Aggregate cluster status
    cluster_info.cluster_status = aggregateHealth(cluster_info.shard_health);

    return cluster_info;
}

/** @brief Register callback for periodic cluster-health updates. */
void HealthCheckSystem::registerCallback([[maybe_unused]] HealthCheckCallback callback) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    callback_ = callback;
}

/** @brief Start periodic cluster-health monitoring loop. */
void HealthCheckSystem::startPeriodicChecks(const std::map<std::string, std::string>& shard_endpoints) {
    std::thread stale_thread;
    {
        std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
        if (running_.load()) {
            return;
        }
        running_.store(true);

        if (periodic_thread_.joinable()) {
            if (periodic_thread_.get_id() == std::this_thread::get_id()) {
                running_.store(false);
                return;
            }
            stale_thread = std::move(periodic_thread_);
        }
    }

    if (stale_thread.joinable()) {
        const bool joined = themis::utils::joinThreadWithin(stale_thread);
        if (!joined) {
            spdlog::warn("HealthCheckSystem stale periodic thread join timed out");
        }
    }

    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    if (!running_.load()) {
        return;
    }
    periodic_thread_ = std::thread([this, shard_endpoints]() {
        while (running_.load()) {
            auto health = checkClusterHealth(shard_endpoints);

            HealthCheckCallback callback_copy;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                current_health_ = health;
                callback_copy = callback_;
            }

            if ([[maybe_unused]] callback_copy) {
                callback_copy([[maybe_unused]] health);
            }

            std::unique_lock<std::mutex> lock(cv_mutex_);
            cv_.wait_for(lock,
                         std::chrono::milliseconds(config_.check_interval_ms),
                         [this] { return !running_.load(); });
        }
    });
}

/** @brief Stop periodic monitoring loop and join worker thread safely. */
void HealthCheckSystem::stopPeriodicChecks() {
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
        running_.store(false);
        cv_.notify_all();

        if (!periodic_thread_.joinable()) {
            return;
        }
        if (periodic_thread_.get_id() == std::this_thread::get_id()) {
            return;
        }
        thread_to_join = std::move(periodic_thread_);
    }

    const bool joined = themis::utils::joinThreadWithin(thread_to_join);
    if (!joined) {
        spdlog::warn("HealthCheckSystem periodic thread join timed out");
    }
}

/** @brief Return latest cached cluster-health snapshot. */
ClusterHealthInfo HealthCheckSystem::getCurrentHealth() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_health_;
}

/** @brief Validate X509 certificate and return remaining lifetime in seconds. */
bool HealthCheckSystem::checkCertificateValidity(const std::string& cert_path, int64_t& seconds_until_expiry) {
    FILE* fp = fopen(cert_path.c_str(), "r");
    if (!fp) {
        seconds_until_expiry = 0;
        return false;
    }

    X509* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);

    if (!cert) {
        seconds_until_expiry = 0;
        return false;
    }

    // Get certificate expiration time
    const ASN1_TIME* not_after = X509_get0_notAfter(cert);
    if (!not_after) {
        X509_free(cert);
        seconds_until_expiry = 0;
        return false;
    }
    
    // Use OpenSSL's ASN1_TIME_diff for safe and portable time comparison.
    int day_diff = 0;
    int sec_diff = 0;
    
    // ASN1_TIME_diff calculates (to - from) and stores days and seconds separately.
    if (ASN1_TIME_diff(&day_diff, &sec_diff, nullptr, not_after) != 1) {
        X509_free(cert);
        seconds_until_expiry = 0;
        return false;
    } else {
        // ASN1_TIME_diff succeeded - convert days and seconds to total seconds
        seconds_until_expiry = static_cast<int64_t>(day_diff) * 86400 + sec_diff;
    }
    
    X509_free(cert);
    
    // Certificate is valid if it hasn't expired yet
    return seconds_until_expiry > 0;
}

/** @brief Probe shard storage metrics endpoint and derive usage percentage. */
bool HealthCheckSystem::checkStorageCapacity(const std::string& endpoint, double& usage_percent) {
    // Make HTTP request to shard to get storage metrics
    // Uses the /metrics or /health endpoint on the shard
    
    try {
        // Create HTTP client for health check
        sharding::MTLSClient::Config client_config;
        client_config.ca_cert_path = config_.ca_cert_path;
        client_config.verify_peer = !config_.ca_cert_path.empty();
        client_config.connect_timeout_ms = 5000;
        client_config.request_timeout_ms = 10000;
        client_config.max_retries = 2;
        
        sharding::MTLSClient client(client_config);
        
        // Request storage metrics from shard
        auto response = client.get(endpoint, "/api/v1/metrics/storage");
        
        if (!response.success) {
            // Try fallback health endpoint
            response = client.get(endpoint, "/health");
            
            if (!response.success) {
                usage_percent = 0.0;
                return false;
            }
        }
        
        // Parse response to extract storage usage
        if (response.body.contains("storage_usage_percent")) {
            usage_percent = response.body["storage_usage_percent"].get<double>();
        } else if (response.body.contains("storage")) {
            auto& storage = response.body["storage"];
            if (storage.contains("used_bytes") && storage.contains("total_bytes")) {
                uint64_t used = storage["used_bytes"].get<uint64_t>();
                uint64_t total = storage["total_bytes"].get<uint64_t>();
                usage_percent = (total > 0) ? (static_cast<double>(used) / total * 100.0) : 0.0;
            } else if (storage.contains("usage_percent")) {
                usage_percent = storage["usage_percent"].get<double>();
            } else {
                usage_percent = 50.0;  // Default if no storage info
            }
        } else {
            usage_percent = 50.0;  // Default if no storage info in response
        }
        
        return true;
        
    } catch (...) {
        usage_percent = 0.0;
        return false;
    }
}

/** @brief Probe shard health endpoint and measure network response latency. */
bool HealthCheckSystem::checkNetworkConnectivity(const std::string& endpoint, double& response_time_ms) {
    // Measure actual network latency to the shard endpoint
    
    try {
        // Create HTTP client for health check
        sharding::MTLSClient::Config client_config;
        client_config.ca_cert_path = config_.ca_cert_path;
        client_config.verify_peer = !config_.ca_cert_path.empty();
        client_config.connect_timeout_ms = 5000;
        client_config.request_timeout_ms = 10000;
        client_config.max_retries = 1;  // Single attempt for latency measurement
        
        sharding::MTLSClient client(client_config);
        
        // Measure request latency
        auto start = std::chrono::steady_clock::now();
        
        // Ping the health endpoint
        auto response = client.get(endpoint, "/health");
        
        auto end = std::chrono::steady_clock::now();
        response_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (!response.success) {
            // Connection failed but we measured the time
            return false;
        }
        
        return true;
        
    } catch (...) {
        response_time_ms = 0.0;
        return false;
    }
}

/** @brief Reduce per-shard health statuses to one cluster severity level. */
HealthStatus HealthCheckSystem::aggregateHealth(const std::vector<ShardHealthInfo>& shard_health) {
    int critical_count = 0;
    int unhealthy_count = 0;
    int degraded_count = 0;

    for (const auto& shard : shard_health) {
        switch (shard.status) {
            case HealthStatus::CRITICAL:
                critical_count++;
                break;
            case HealthStatus::UNHEALTHY:
                unhealthy_count++;
                break;
            case HealthStatus::DEGRADED:
                degraded_count++;
                break;
            default:
                break;
        }
    }

    // Aggregate logic
    if (critical_count > 0) {
        return HealthStatus::CRITICAL;
    } else if (unhealthy_count > 0) {
        return HealthStatus::UNHEALTHY;
    } else if (degraded_count > 0) {
        return HealthStatus::DEGRADED;
    }
    
    return HealthStatus::HEALTHY;
}

/** @brief Return true when healthy shards form strict majority quorum. */
bool HealthCheckSystem::hasQuorum(int healthy_shards, int total_shards) {
    if (total_shards == 0) {
      return false;
    }
    return healthy_shards > total_shards / 2;
}

} // namespace sharding
} // namespace themis

