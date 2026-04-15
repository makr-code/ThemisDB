/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            health_check.cpp                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:44:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     403                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/health_check.h"
#include "sharding/mtls_client.h"
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

HealthCheckSystem::HealthCheckSystem(const Config& config)
    : config_(config) {
}

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

void HealthCheckSystem::registerCallback(HealthCheckCallback callback) {
    callback_ = callback;
}

void HealthCheckSystem::startPeriodicChecks(const std::map<std::string, std::string>& shard_endpoints) {
    running_ = true;
    
    std::thread([this, shard_endpoints]() {
        while (running_) {
            auto health = checkClusterHealth(shard_endpoints);
            current_health_ = health;
            
            if (callback_) {
                callback_(health);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.check_interval_ms));
        }
    }).detach();
}

void HealthCheckSystem::stopPeriodicChecks() {
    running_ = false;
}

ClusterHealthInfo HealthCheckSystem::getCurrentHealth() const {
    return current_health_;
}

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
    
    // Use OpenSSL's ASN1_TIME_diff for safe and portable time comparison
    // This is preferred over manual parsing as it handles all ASN1 time formats
    int day_diff = 0;
    int sec_diff = 0;
    
    // ASN1_TIME_diff calculates (to - from) and stores days and seconds separately
    // We pass nullptr for 'from' which defaults to current time
    if (ASN1_TIME_diff(&day_diff, &sec_diff, nullptr, not_after) != 1) {
        // Fallback: try manual parsing if ASN1_TIME_diff fails
        // This handles edge cases on older OpenSSL versions
        
        struct tm tm_expiry;
        memset(&tm_expiry, 0, sizeof(tm_expiry));
        
        // Parse the ASN1_TIME string with input validation
        const unsigned char* data = not_after->data;
        int len = not_after->length;
        
        // Validate minimum length and digit characters
        auto isDigit = [](unsigned char c) { return c >= '0' && c <= '9'; };
        
        if (not_after->type == V_ASN1_UTCTIME && len >= 12) {
            // YYMMDDHHMMSSZ format - validate first 12 chars are digits
            bool valid = true;
            for (int i = 0; i < 12 && valid; i++) {
                valid = isDigit(data[i]);
            }
            
            if (valid) {
                int year = (data[0] - '0') * 10 + (data[1] - '0');
                tm_expiry.tm_year = (year >= 50) ? year : (100 + year);  // 1950-2049
                tm_expiry.tm_mon = (data[2] - '0') * 10 + (data[3] - '0') - 1;
                tm_expiry.tm_mday = (data[4] - '0') * 10 + (data[5] - '0');
                tm_expiry.tm_hour = (data[6] - '0') * 10 + (data[7] - '0');
                tm_expiry.tm_min = (data[8] - '0') * 10 + (data[9] - '0');
                tm_expiry.tm_sec = (data[10] - '0') * 10 + (data[11] - '0');
            } else {
                X509_free(cert);
                seconds_until_expiry = 30 * 86400;  // Default: assume 30 days
                return true;
            }
        } else if (not_after->type == V_ASN1_GENERALIZEDTIME && len >= 14) {
            // YYYYMMDDHHMMSSZ format - validate first 14 chars are digits
            bool valid = true;
            for (int i = 0; i < 14 && valid; i++) {
                valid = isDigit(data[i]);
            }
            
            if (valid) {
                tm_expiry.tm_year = (data[0] - '0') * 1000 + (data[1] - '0') * 100 + 
                                   (data[2] - '0') * 10 + (data[3] - '0') - 1900;
                tm_expiry.tm_mon = (data[4] - '0') * 10 + (data[5] - '0') - 1;
                tm_expiry.tm_mday = (data[6] - '0') * 10 + (data[7] - '0');
                tm_expiry.tm_hour = (data[8] - '0') * 10 + (data[9] - '0');
                tm_expiry.tm_min = (data[10] - '0') * 10 + (data[11] - '0');
                tm_expiry.tm_sec = (data[12] - '0') * 10 + (data[13] - '0');
            } else {
                X509_free(cert);
                seconds_until_expiry = 30 * 86400;  // Default: assume 30 days
                return true;
            }
        } else {
            X509_free(cert);
            seconds_until_expiry = 30 * 86400;  // Default: assume 30 days if parsing fails
            return true;
        }
        
        // Convert to time_t (using mktime and adjusting for UTC)
        // Note: mktime interprets as local time, so we adjust
        tm_expiry.tm_isdst = 0;
        time_t expiry_local = mktime(&tm_expiry);
        
        // Get timezone offset to convert local to UTC
        struct tm* utc_tm = gmtime(&expiry_local);
        time_t expiry_utc = mktime(utc_tm);
        time_t tz_offset = expiry_local - expiry_utc;
        
        time_t expiry_time = expiry_local + tz_offset;
        time_t now = time(nullptr);
        
        seconds_until_expiry = static_cast<int64_t>(expiry_time - now);
    } else {
        // ASN1_TIME_diff succeeded - convert days and seconds to total seconds
        seconds_until_expiry = static_cast<int64_t>(day_diff) * 86400 + sec_diff;
    }
    
    X509_free(cert);
    
    // Certificate is valid if it hasn't expired yet
    return seconds_until_expiry > 0;
}

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
        
    } catch (const std::exception& e) {
        usage_percent = 0.0;
        return false;
    }
}

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
        
    } catch (const std::exception& e) {
        response_time_ms = 0.0;
        return false;
    }
}

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

bool HealthCheckSystem::hasQuorum(int healthy_shards, int total_shards) {
    if (total_shards == 0) return false;
    return healthy_shards > total_shards / 2;
}

} // namespace sharding
} // namespace themis
