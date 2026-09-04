/**
 * @file shard_topology.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/shard_topology.h"
#include "sharding/mtls_client.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstring>
#include <unordered_set>

namespace themis::sharding {

// Base64 encoding lookup table
static const char B64_ENCODE_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 decoding lookup table
static const int B64_DECODE_TABLE[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

// Helper function: Base64 encode
static std::string base64Encode(const std::string& input) {
    std::string output = {};
    output.reserve(((input.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= input.size()) {
        uint32_t n = (static_cast<uint8_t>(input[i]) << 16) | 
                    (static_cast<uint8_t>(input[i + 1]) << 8) | 
                    static_cast<uint8_t>(input[i + 2]);
        output.push_back(B64_ENCODE_TABLE[(n >> 18) & 63]);
        output.push_back(B64_ENCODE_TABLE[(n >> 12) & 63]);
        output.push_back(B64_ENCODE_TABLE[(n >> 6) & 63]);
        output.push_back(B64_ENCODE_TABLE[n & 63]);
        i += 3;
    }
    if (i + 1 == input.size()) {
        uint32_t n = static_cast<uint8_t>(input[i]) << 16;
        output.push_back(B64_ENCODE_TABLE[(n >> 18) & 63]);
        output.push_back(B64_ENCODE_TABLE[(n >> 12) & 63]);
        output.push_back('=');
        output.push_back('=');
    } else if (i + 2 == input.size()) {
        uint32_t n = (static_cast<uint8_t>(input[i]) << 16) | 
                    (static_cast<uint8_t>(input[i + 1]) << 8);
        output.push_back(B64_ENCODE_TABLE[(n >> 18) & 63]);
        output.push_back(B64_ENCODE_TABLE[(n >> 12) & 63]);
        output.push_back(B64_ENCODE_TABLE[(n >> 6) & 63]);
        output.push_back('=');
    }
    return output;
}

// Helper function: Base64 decode
static std::string base64Decode(const std::string& input) {
    std::string output = {};
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (c == '=') {
          break;
        }
        int d = B64_DECODE_TABLE[c];
        if (d == -1) {
          continue;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

ShardTopology::ShardTopology()
    : ShardTopology(Config{"", "", 0, false}) {}

/**
 * @brief Construct shard topology manager with optional metadata bootstrap.
 * @param config Topology configuration.
 */
ShardTopology::ShardTopology(const Config& config) 
    : config_(config) {
    // If metadata endpoint is configured, load initial topology
    if (!config_.metadata_endpoint.empty()) {
        try {
            loadFromMetadataStore();
        } catch (const std::exception& e) {
            std::cerr << "ShardTopology: Failed to load from metadata store: " 
                      << e.what() << std::endl;
            // Continue with in-memory mode
        }
    }
}

/** @brief Add or replace shard metadata entry by shard_id. */
void ShardTopology::addShard(const ShardInfo& shard) {
    std::lock_guard<std::mutex> lock(mutex_);
    shards_[shard.shard_id] = shard;
}

/** @brief Remove shard metadata entry by shard_id. */
void ShardTopology::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    shards_.erase(shard_id);
}

/** @brief Fetch shard metadata entry by id. */
std::optional<ShardInfo> ShardTopology::getShard(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shards_.find(shard_id);
    if (it == shards_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

/** @brief Return snapshot of all shard metadata entries. */
std::vector<ShardInfo> ShardTopology::getAllShards() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ShardInfo> result = {};

    result.reserve(shards_.size());
    
    for (const auto& [id, info] : shards_) {
        result.push_back(info);
    }
    
    return result;
}

/** @brief Return snapshot of healthy shard metadata entries. */
std::vector<ShardInfo> ShardTopology::getHealthyShards() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ShardInfo> result;
    
    for (const auto& [id, info] : shards_) {
        if (info.is_healthy) {
            result.push_back(info);
        }
    }
    
    return result;
}

/** @brief Update health bit for one shard if it exists. */
void ShardTopology::updateHealth(const std::string& shard_id, bool is_healthy) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shards_.find(shard_id);
    if (it != shards_.end()) {
        it->second.is_healthy = is_healthy;
    }
}

/** @brief Refresh topology from configured metadata store backend. */
void ShardTopology::refresh() {
    // Load latest topology from metadata store
    loadFromMetadataStore();
}

/** @brief Persist current topology state to metadata store backend. */
void ShardTopology::save() {
    // Save current topology to metadata store
    saveToMetadataStore();
}

/**
 * @brief Load topology entries from metadata store.
 *
 * Uses configured HTTP-compatible etcd/Consul API endpoints and rebuilds
 * in-memory shard map from remote key/value state.
 */
void ShardTopology::loadFromMetadataStore() {
    // Load topology from etcd/Consul metadata store
    // Uses HTTP API for etcd v3 gateway or Consul HTTP API
    //
    // etcd key structure:
    //   /themis/{cluster_name}/shards/{shard_id}/info   - JSON with shard info
    //   /themis/{cluster_name}/topology/version         - Topology version number
    //
    // Consul key structure:
    //   themis/{cluster_name}/shards/{shard_id}         - JSON with shard info
    
    if (config_.metadata_endpoint.empty()) {
        // No metadata store configured, use in-memory only
        return;
    }
    
    // Create HTTP client for metadata store access
    MTLSClient::Config client_config;
    client_config.verify_peer = false;  // etcd/Consul may not use mTLS
    client_config.connect_timeout_ms = 5000;
    client_config.request_timeout_ms = 10000;
    client_config.max_retries = 3;
    
    MTLSClient client(client_config);
    
    // Build the key prefix for shard list
    std::string prefix = "/v3/kv/range";  // etcd v3 HTTP API
    std::string key_prefix = "/themis/" + config_.cluster_name + "/shards/";
    
    // Create range end key for prefix scan
    // Safe overflow handling: append '\x00' and increment, or use '\xff' suffix
    std::string range_end = key_prefix;
    if (!range_end.empty()) {
        unsigned char last_char = static_cast<unsigned char>(range_end.back());
        if (last_char < 255) {
            range_end.back() = static_cast<char>(last_char + 1);
        } else {
            // Handle edge case: append a character that's lexically greater
            range_end.push_back('\x00');
        }
    }
    
    // Request body for etcd range query
    nlohmann::json request_body = {
        {"key", base64Encode(key_prefix)},
        {"range_end", base64Encode(range_end)}
    };
    
    try {
        auto response = client.post(config_.metadata_endpoint, prefix, request_body);
        
        if (!response.success) {
            std::cerr << "ShardTopology: Failed to query metadata store: " 
                      << response.error << std::endl;
            return;
        }
        
        // Parse response
        if (!response.body.contains("kvs") || !response.body["kvs"].is_array()) {
            // No shards found in metadata store
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Clear existing shards and load from metadata store
        shards_.clear();
        
        for (const auto& kv : response.body["kvs"]) {
            if (!kv.contains("value")) {
              continue;
            }
            
            // Decode base64 value using shared helper function
            std::string encoded_value = kv["value"].get<std::string>();
            std::string decoded_value = base64Decode(encoded_value);
            
            try {
                auto shard_json = nlohmann::json::parse(decoded_value);
                
                ShardInfo shard;
                shard.shard_id = shard_json.value("shard_id", "");
                shard.primary_endpoint = shard_json.value("primary_endpoint", "");
                shard.datacenter = shard_json.value("datacenter", "");
                shard.rack = shard_json.value("rack", "");
                shard.token_start = shard_json.value("token_start", 0ULL);
                shard.token_end = shard_json.value("token_end", 0ULL);
                shard.is_healthy = shard_json.value("is_healthy", true);
                shard.certificate_serial = shard_json.value("certificate_serial", "");
                
                if (shard_json.contains("replica_endpoints") && 
                    shard_json["replica_endpoints"].is_array()) {
                    for (const auto& ep : shard_json["replica_endpoints"]) {
                        shard.replica_endpoints.push_back(ep.get<std::string>());
                    }
                }
                
                if (shard_json.contains("capabilities") && 
                    shard_json["capabilities"].is_array()) {
                    for (const auto& cap : shard_json["capabilities"]) {
                        shard.capabilities.push_back(cap.get<std::string>());
                    }
                }
                
                if (!shard.shard_id.empty()) {
                    shards_[shard.shard_id] = shard;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "ShardTopology: Failed to parse shard info: " 
                          << e.what() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "ShardTopology: Exception loading from metadata store: " 
                  << e.what() << std::endl;
    }
}

/** @brief Save current in-memory shard map to metadata store. */
void ShardTopology::saveToMetadataStore() {
    // Save topology to etcd/Consul metadata store
    
    if (config_.metadata_endpoint.empty()) {
        // No metadata store configured, use in-memory only
        return;
    }
    
    // Create HTTP client for metadata store access
    MTLSClient::Config client_config;
    client_config.verify_peer = false;  // etcd/Consul may not use mTLS
    client_config.connect_timeout_ms = 5000;
    client_config.request_timeout_ms = 10000;
    client_config.max_retries = 3;
    
    MTLSClient client(client_config);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Save each shard to metadata store
    for (const auto& [shard_id, shard] : shards_) {
        // Build etcd key
        std::string key = "/themis/" + config_.cluster_name + "/shards/" + shard_id;
        
        // Build shard info JSON
        nlohmann::json shard_json = {
            {"shard_id", shard.shard_id},
            {"primary_endpoint", shard.primary_endpoint},
            {"replica_endpoints", shard.replica_endpoints},
            {"datacenter", shard.datacenter},
            {"rack", shard.rack},
            {"token_start", shard.token_start},
            {"token_end", shard.token_end},
            {"is_healthy", shard.is_healthy},
            {"certificate_serial", shard.certificate_serial},
            {"capabilities", shard.capabilities}
        };
        
        std::string value = shard_json.dump();
        
        // Request body for etcd put using shared helper function
        nlohmann::json request_body = {
            {"key", base64Encode(key)},
            {"value", base64Encode(value)}
        };
        
        try {
            auto response = client.post(config_.metadata_endpoint, "/v3/kv/put", request_body);
            
            if (!response.success) {
                std::cerr << "ShardTopology: Failed to save shard " << shard_id 
                          << " to metadata store: " << response.error << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "ShardTopology: Exception saving shard " << shard_id 
                      << ": " << e.what() << std::endl;
        }
    }
}

/** @brief Update Raft role/term/leader fields for one shard entry. */
void ShardTopology::updateRaftStatus(const std::string& shard_id,
                                    const std::string& role,
                                    uint64_t term,
                                    uint64_t commit_index,
                                    const std::string& leader_id,
                                    bool has_quorum) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shards_.find(shard_id);
    if (it != shards_.end()) {
        it->second.raft_role = role;
        it->second.raft_term = term;
        it->second.raft_commit_index = commit_index;
        it->second.raft_leader_id = leader_id;
        it->second.raft_has_quorum = has_quorum;
    }
}

/** @brief Return ids of all shards currently marked as Raft leaders. */
std::vector<std::string> ShardTopology::getRaftLeaders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> leaders = {};

    for (const auto& [id, info] : shards_) {
        if (info.isRaftLeader()) {
            leaders.push_back(id);
        }
    }
    
    return leaders;
}

/** @brief Return all shards assigned to given region string. */
std::vector<ShardInfo> ShardTopology::getShardsInRegion(const std::string& region) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ShardInfo> result = {};

    for (const auto& [id, info] : shards_) {
        if (info.region == region) {
            result.push_back(info);
        }
    }
    return result;
}

/** @brief Return healthy shards assigned to given region string. */
std::vector<ShardInfo> ShardTopology::getHealthyShardsInRegion(const std::string& region) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ShardInfo> result = {};

    for (const auto& [id, info] : shards_) {
        if (info.region == region && info.is_healthy) {
            result.push_back(info);
        }
    }
    return result;
}

/** @brief Return sorted list of distinct non-empty region names. */
std::vector<std::string> ShardTopology::getRegions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_set<std::string> seen = {};

    for (const auto& [id, info] : shards_) {
        if (!info.region.empty()) {
            seen.insert(info.region);
        }
    }
    std::vector<std::string> regions(seen.begin(), seen.end());
    std::sort(regions.begin(), regions.end());
    return regions;
}

/** @brief Return whether region has at least required healthy shard count. */
bool ShardTopology::regionHasQuorum(const std::string& region, uint32_t required) const {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t healthy = 0;
    for (const auto& [id, info] : shards_) {
        if (info.region == region && info.is_healthy) {
            ++healthy;
        }
    }
    return healthy >= required;
}

} // namespace themis::sharding
