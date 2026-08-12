/**
 * @file raid_data_pusher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * RAID Data Push Utility
 * 
 * Schreibt Test-Daten in RAID Shards und sammelt Metriken
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

/** @brief Raid data pusher. */
class RAIDDataPusher {
public:
    struct ShardConfig {
        std::string host;
        int rest_port;
        int metrics_port;
        std::string raid_type;  // "RAID0", "RAID1", "RAID5"
    };
    
    struct PushResult {
        std::string shard_name;
        int total_pushed;
        int total_failed;
        std::vector<std::string> errors;
        std::chrono::milliseconds duration;
    };
    
    struct MetricsSnapshot {
        std::string timestamp;
        std::string shard_name;
        json raw_metrics;
        int documents_count;
        double disk_usage_mb;
    };

private:
    std::vector<ShardConfig> shards_;
    std::vector<MetricsSnapshot> metrics_before_;
    std::vector<MetricsSnapshot> metrics_after_;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    RAIDDataPusher() {
        initializeShards();
    }
    
    void initializeShards() {
        shards_ = {
            {"themis-raid0-shard1", 8080, 9090, "RAID0"},
            {"themis-raid0-shard2", 8080, 9091, "RAID0"},
            {"themis-raid0-shard3", 8080, 9092, "RAID0"},
            {"themis-raid1-primary", 8080, 9093, "RAID1"},
            {"themis-raid1-mirror", 8080, 9094, "RAID1"},
            {"themis-raid5-shard1", 8080, 9095, "RAID5"},
            {"themis-raid5-shard2", 8080, 9096, "RAID5"},
            {"themis-raid5-shard3", 8080, 9097, "RAID5"},
        };
    }
    
    bool waitForShardsHealthy(int timeout_seconds = 120) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            int healthy_count = 0;
            
            for (const auto& shard : shards_) {
                std::string url = "http://" + shard.host + ":" + std::to_string(shard.rest_port) + "/health";
                
                CURL* curl = curl_easy_init();
                if (curl) {
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                    
                    CURLcode res = curl_perform(curl);
                    if (res == CURLE_OK) {
                        long response_code;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                        if (response_code == 200) {
                            healthy_count++;
                        }
                    }
                    curl_easy_cleanup(curl);
                }
            }
            
            if (healthy_count == (int)shards_.size()) {
                return true;
            }
            
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start
            );
            
            if (elapsed.count() >= timeout_seconds) {
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    PushResult pushTestData(const std::string& collection_name, int num_records) {
        auto start = std::chrono::steady_clock::now();
        
        PushResult result;
        result.total_pushed = 0;
        result.total_failed = 0;
        
        // Generate and push test records (round-robin distribution)
        for (int i = 0; i < num_records; ++i) {
            const auto& shard = shards_[i % shards_.size()];
            
            json record = {
                {"id", "test_" + std::to_string(i)},
                {"domain", std::vector<std::string>{"legal", "medical", "finance"}[i % 3]},
                {"content", "Test record " + std::to_string(i)},
                {"timestamp", getCurrentTimestamp()}
            };
            
            std::string url = "http://" + shard.host + ":" + 
                            std::to_string(shard.rest_port) + 
                            "/api/v1/collections/" + collection_name + "/documents";
            
            CURL* curl = curl_easy_init();
            if (curl) {
                std::string response;
                std::string payload = record.dump();
                
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                
                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                
                CURLcode res = curl_easy_perform(curl);
                
                long response_code = 0;
                if (res == CURLE_OK) {
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                    if (response_code >= 200 && response_code < 300) {
                        result.total_pushed++;
                    } else {
                        result.total_failed++;
                        result.errors.push_back(
                            "Shard: " + shard.host + ", Status: " + std::to_string(response_code)
                        );
                    }
                } else {
                    result.total_failed++;
                    result.errors.push_back("curl error: " + std::string(curl_easy_strerror(res)));
                }
                
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
            }
        }
        
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        
        return result;
    }
    
    MetricsSnapshot getShardMetrics(const ShardConfig& shard) {
        MetricsSnapshot snapshot;
        snapshot.shard_name = shard.host;
        snapshot.timestamp = getCurrentTimestamp();
        
        std::string url = "http://" + shard.host + ":" + 
                        std::to_string(shard.metrics_port) + "/metrics";
        
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string response;
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res == CURLE_OK) {
                try {
                    snapshot.raw_metrics = json::parse(response);
                } catch (...) {
                    // If not JSON, treat as Prometheus format
                    snapshot.raw_metrics = json::object();
                    snapshot.raw_metrics["raw_text"] = response;
                }
            }
            
            curl_easy_cleanup(curl);
        }
        
        return snapshot;
    }
    
    void collectMetricsBaseline() {
        metrics_before_.clear();
        for (const auto& shard : shards_) {
            metrics_before_.push_back(getShardMetrics(shard));
        }
    }
    
    void collectMetricsAfter() {
        metrics_after_.clear();
        for (const auto& shard : shards_) {
            metrics_after_.push_back(getShardMetrics(shard));
        }
    }
    
    json generateReport() const {
        json report = {
            {"timestamp", getCurrentTimestamp()},
            {"total_shards", shards_.size()},
            {"metrics_baseline", json::array()},
            {"metrics_after", json::array()}
        };
        
        for (const auto& m : metrics_before_) {
            report["metrics_baseline"].push_back({
                {"shard", m.shard_name},
                {"timestamp", m.timestamp}
            });
        }
        
        for (const auto& m : metrics_after_) {
            report["metrics_after"].push_back({
                {"shard", m.shard_name},
                {"timestamp", m.timestamp}
            });
        }
        
        return report;
    }
    
private:
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};
