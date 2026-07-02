/**
 * @file adapter_version_manager.cpp
 * @brief Implementation of adapter version management and rollback
 */

#include "llm/lora_framework/adapter_version_manager.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

namespace themis {
namespace llm {
namespace lora {

namespace fs = std::filesystem;

// ============================================================================
// AdapterVersionInfo Implementation
// ============================================================================

json AdapterVersionInfo::toJSON() const {
    auto time_t = std::chrono::system_clock::to_time_t(created_at);
    return json{
        {"version_id", version_id},
        {"adapter_id", adapter_id},
        {"created_at", time_t},
        {"training_source", training_source},
        {"training_samples", training_samples},
        {"final_loss", final_loss},
        {"validation_accuracy", validation_accuracy},
        {"training_duration_seconds", training_duration.count()},
        {"avg_accuracy", avg_accuracy},
        {"avg_latency_ms", avg_latency_ms},
        {"total_inferences", total_inferences},
        {"errors", errors},
        {"is_active", is_active},
        {"is_stable", is_stable},
        {"deployment_status", deployment_status},
        {"weights_hash", weights_hash},
        {"config_hash", config_hash},
        {"can_rollback_to", can_rollback_to},
        {"rollback_reason_if_not", rollback_reason_if_not}
    };
}

AdapterVersionInfo AdapterVersionInfo::fromJSON(const json& j) {
    AdapterVersionInfo info;
    if (j.contains("version_id")) info.version_id = j["version_id"];
    if (j.contains("adapter_id")) info.adapter_id = j["adapter_id"];
    if (j.contains("training_source")) info.training_source = j["training_source"];
    if (j.contains("training_samples")) info.training_samples = j["training_samples"];
    if (j.contains("final_loss")) info.final_loss = j["final_loss"];
    if (j.contains("validation_accuracy")) info.validation_accuracy = j["validation_accuracy"];
    if (j.contains("training_duration_seconds")) 
        info.training_duration = std::chrono::seconds(j["training_duration_seconds"]);
    if (j.contains("avg_accuracy")) info.avg_accuracy = j["avg_accuracy"];
    if (j.contains("avg_latency_ms")) info.avg_latency_ms = j["avg_latency_ms"];
    if (j.contains("total_inferences")) info.total_inferences = j["total_inferences"];
    if (j.contains("errors")) info.errors = j["errors"];
    if (j.contains("is_active")) info.is_active = j["is_active"];
    if (j.contains("is_stable")) info.is_stable = j["is_stable"];
    if (j.contains("deployment_status")) info.deployment_status = j["deployment_status"];
    if (j.contains("weights_hash")) info.weights_hash = j["weights_hash"];
    if (j.contains("config_hash")) info.config_hash = j["config_hash"];
    if (j.contains("can_rollback_to")) info.can_rollback_to = j["can_rollback_to"];
    if (j.contains("rollback_reason_if_not")) info.rollback_reason_if_not = j["rollback_reason_if_not"];
    
    info.created_at = std::chrono::system_clock::now();
    return info;
}

// ============================================================================
// AdapterVersionSnapshot Implementation
// ============================================================================

json AdapterVersionSnapshot::toJSON() const {
    auto time_t = std::chrono::system_clock::to_time_t(snapshot_time);
    json j;
    j["version_id"] = version_id;
    j["adapter_id"] = adapter_id;
    j["snapshot_time"] = time_t;
    j["snapshot_size_bytes"] = snapshot_size_bytes;
    j["storage_path"] = storage_path;
    j["is_compressed"] = is_compressed;
    j["checksum"] = checksum;
    j["adapter_config"] = adapter_config;
    j["training_config"] = training_config;
    j["dependency_versions"] = dependency_versions;
    return j;
}

AdapterVersionSnapshot AdapterVersionSnapshot::fromJSON(const json& j) {
    AdapterVersionSnapshot snapshot;
    if (j.contains("version_id")) snapshot.version_id = j["version_id"];
    if (j.contains("adapter_id")) snapshot.adapter_id = j["adapter_id"];
    if (j.contains("snapshot_size_bytes")) snapshot.snapshot_size_bytes = j["snapshot_size_bytes"];
    if (j.contains("storage_path")) snapshot.storage_path = j["storage_path"];
    if (j.contains("is_compressed")) snapshot.is_compressed = j["is_compressed"];
    if (j.contains("checksum")) snapshot.checksum = j["checksum"];
    if (j.contains("adapter_config")) snapshot.adapter_config = j["adapter_config"];
    if (j.contains("training_config")) snapshot.training_config = j["training_config"];
    if (j.contains("dependency_versions")) snapshot.dependency_versions = j["dependency_versions"];
    
    snapshot.snapshot_time = std::chrono::system_clock::now();
    return snapshot;
}

// ============================================================================
// VersionComparison Implementation
// ============================================================================

json VersionComparison::toJSON() const {
    return json{
        {"version_a", version_a},
        {"version_b", version_b},
        {"accuracy_delta", accuracy_delta},
        {"latency_delta_ms", latency_delta_ms},
        {"error_rate_delta", error_rate_delta},
        {"is_b_better", is_b_better},
        {"improvement_confidence", improvement_confidence}
    };
}

// ============================================================================
// AdapterVersionManager Implementation
// ============================================================================

AdapterVersionManager::AdapterVersionManager(
    const std::string& adapter_id,
    const Config& config
)
    : adapter_id_(adapter_id)
    , config_(config)
{
    // Create snapshot storage directory if needed
    try {
        fs::create_directories(config_.snapshot_storage_path);
    } catch (const std::exception& e) {
        spdlog::warn("Failed to create snapshot directory: {}", e.what());
    }
    
    spdlog::debug("AdapterVersionManager initialized for adapter '{}'", adapter_id_);
}

bool AdapterVersionManager::createVersion(
    const std::string& version_id,
    const std::string& training_source,
    const json& metrics
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if version already exists
    for (const auto& v : versions_) {
        if (v.version_id == version_id) {
            spdlog::warn("Version '{}' already exists for adapter '{}'", 
                        version_id, adapter_id_);
            return false;
        }
    }
    
    AdapterVersionInfo info;
    info.version_id = version_id;
    info.adapter_id = adapter_id_;
    info.created_at = std::chrono::system_clock::now();
    info.training_source = training_source;
    info.deployment_status = "staging";
    
    // Extract metrics
    if (metrics.contains("training_samples")) 
        info.training_samples = metrics["training_samples"];
    if (metrics.contains("final_loss")) 
        info.final_loss = metrics["final_loss"];
    if (metrics.contains("validation_accuracy")) 
        info.validation_accuracy = metrics["validation_accuracy"];
    if (metrics.contains("training_duration_seconds")) 
        info.training_duration = std::chrono::seconds(metrics["training_duration_seconds"]);
    
    versions_.push_back(info);
    
    spdlog::info("Created version '{}' for adapter '{}' from source '{}'", 
                version_id, adapter_id_, training_source);
    
    return true;
}

std::optional<AdapterVersionSnapshot> AdapterVersionManager::createSnapshot(
    const std::string& version_id,
    const json& adapter_data
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify version exists
    auto version_it = std::find_if(
        versions_.begin(),
        versions_.end(),
        [&version_id](const AdapterVersionInfo& v) { return v.version_id == version_id; }
    );
    
    if (version_it == versions_.end()) {
        spdlog::warn("Version '{}' not found for adapter '{}'", version_id, adapter_id_);
        return std::nullopt;
    }
    
    AdapterVersionSnapshot snapshot;
    snapshot.version_id = version_id;
    snapshot.adapter_id = adapter_id_;
    snapshot.snapshot_time = std::chrono::system_clock::now();
    snapshot.adapter_config = adapter_data;
    snapshot.is_compressed = config_.compress_snapshots;
    
    // Compute checksum
    std::string data_str = adapter_data.dump();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data_str.c_str(), data_str.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    snapshot.checksum = ss.str();
    
    // Compute storage path
    std::string filename = adapter_id_ + "_" + version_id + ".json";
    if (config_.compress_snapshots) {
        filename += ".gz";
    }
    snapshot.storage_path = config_.snapshot_storage_path + "/" + filename;
    snapshot.snapshot_size_bytes = data_str.size();
    
    // Store snapshot
    if (saveSnapshot(version_id, adapter_data)) {
        snapshots_[version_id] = snapshot;
        spdlog::info("Created snapshot for version '{}' ({})", version_id, snapshot.checksum);
        return snapshot;
    }
    
    return std::nullopt;
}

std::optional<AdapterVersionInfo> AdapterVersionManager::getVersionInfo(
    const std::string& version_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& v : versions_) {
        if (v.version_id == version_id) {
            return v;
        }
    }
    return std::nullopt;
}

std::vector<std::string> AdapterVersionManager::getAllVersions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& v : versions_) {
        result.push_back(v.version_id);
    }
    return result;
}

std::string AdapterVersionManager::getActiveVersion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_version_;
}

bool AdapterVersionManager::setActiveVersion(const std::string& version_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify version exists
    auto version_it = std::find_if(
        versions_.begin(),
        versions_.end(),
        [&version_id](const AdapterVersionInfo& v) { return v.version_id == version_id; }
    );
    
    if (version_it == versions_.end()) {
        spdlog::warn("Version '{}' not found", version_id);
        return false;
    }
    
    // Update is_active flags
    for (auto& v : versions_) {
        v.is_active = (v.version_id == version_id);
    }
    
    active_version_ = version_id;
    spdlog::info("Activated version '{}' for adapter '{}'", version_id, adapter_id_);
    
    return true;
}

std::optional<AdapterVersionSnapshot> AdapterVersionManager::getSnapshot(
    const std::string& version_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = snapshots_.find(version_id);
    if (it != snapshots_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool AdapterVersionManager::rollback(const std::string& version_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get snapshot
    auto snapshot_it = snapshots_.find(version_id);
    if (snapshot_it == snapshots_.end()) {
        spdlog::error("No snapshot available for rollback to version '{}'", version_id);
        return false;
    }
    
    // Validate rollback eligibility
    auto version_it = std::find_if(
        versions_.begin(),
        versions_.end(),
        [&version_id](const AdapterVersionInfo& v) { return v.version_id == version_id; }
    );
    
    if (version_it == versions_.end() || !version_it->can_rollback_to) {
        spdlog::warn("Version '{}' cannot be rolled back to", version_id);
        return false;
    }
    
    // Validate snapshot integrity if configured
    if (config_.verify_checksum_on_rollback) {
        if (!validateSnapshot(snapshot_it->second)) {
            spdlog::error("Snapshot validation failed for version '{}'", version_id);
            return false;
        }
    }
    
    // Set as active
    active_version_ = version_id;
    for (auto& v : versions_) {
        v.is_active = (v.version_id == version_id);
    }
    
    spdlog::info("Rolled back to version '{}' for adapter '{}'", version_id, adapter_id_);
    return true;
}

std::optional<VersionComparison> AdapterVersionManager::compareVersions(
    const std::string& version_a,
    const std::string& version_b
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto info_a = std::find_if(
        versions_.begin(),
        versions_.end(),
        [&version_a](const AdapterVersionInfo& v) { return v.version_id == version_a; }
    );
    
    auto info_b = std::find_if(
        versions_.begin(),
        versions_.end(),
        [&version_b](const AdapterVersionInfo& v) { return v.version_id == version_b; }
    );
    
    if (info_a == versions_.end() || info_b == versions_.end()) {
        return std::nullopt;
    }
    
    VersionComparison comparison;
    comparison.version_a = version_a;
    comparison.version_b = version_b;
    
    comparison.accuracy_delta = info_b->avg_accuracy - info_a->avg_accuracy;
    comparison.latency_delta_ms = info_b->avg_latency_ms - info_a->avg_latency_ms;
    comparison.error_rate_delta = 
        (static_cast<float>(info_b->errors) / std::max(1, info_b->total_inferences)) -
        (static_cast<float>(info_a->errors) / std::max(1, info_a->total_inferences));
    
    // Determine if B is better
    comparison.is_b_better = (comparison.accuracy_delta > 0.01f) && 
                             (comparison.latency_delta_ms < 50.0f);
    comparison.improvement_confidence = 
        std::abs(comparison.accuracy_delta) > 0.05f ? 0.9f : 0.5f;
    
    return comparison;
}

bool AdapterVersionManager::markVersionAsStable(const std::string& version_id, bool is_stable) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(
        versions_.begin(),
        versions_.end(),
        [&version_id](const AdapterVersionInfo& v) { return v.version_id == version_id; }
    );
    
    if (it == versions_.end()) {
        return false;
    }
    
    it->is_stable = is_stable;
    if (is_stable) {
        it->deployment_status = "production";
    } else {
        it->deployment_status = "staging";
    }
    
    return true;
}

bool AdapterVersionManager::deleteVersion(const std::string& version_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Don't delete active version
    if (active_version_ == version_id) {
        spdlog::warn("Cannot delete active version '{}'", version_id);
        return false;
    }
    
    // Remove version
    auto it = std::remove_if(
        versions_.begin(),
        versions_.end(),
        [&version_id](const AdapterVersionInfo& v) { return v.version_id == version_id; }
    );
    
    if (it != versions_.end()) {
        versions_.erase(it, versions_.end());
        
        // Remove snapshot if exists
        auto snap_it = snapshots_.find(version_id);
        if (snap_it != snapshots_.end()) {
            try {
                fs::remove(snap_it->second.storage_path);
                snapshots_.erase(snap_it);
            } catch (const std::exception& e) {
                spdlog::warn("Failed to delete snapshot file: {}", e.what());
            }
        }
        
        spdlog::info("Deleted version '{}'", version_id);
        return true;
    }
    
    return false;
}

int AdapterVersionManager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int deleted_count = 0;
    auto cutoff_time = std::chrono::system_clock::now() - config_.version_retention_period;
    
    // Remove old versions
    auto it = std::remove_if(
        versions_.begin(),
        versions_.end(),
        [cutoff_time, &deleted_count, this](const AdapterVersionInfo& v) {
            if (v.created_at < cutoff_time && v.version_id != active_version_) {
                deleted_count++;
                return true;
            }
            return false;
        }
    );
    
    if (it != versions_.end()) {
        versions_.erase(it, versions_.end());
    }
    
    // Keep only max versions
    while (versions_.size() > config_.max_versions_kept) {
        auto old_it = std::min_element(
            versions_.begin(),
            versions_.end(),
            [](const AdapterVersionInfo& a, const AdapterVersionInfo& b) {
                return a.created_at < b.created_at;
            }
        );
        
        if (old_it != versions_.end() && old_it->version_id != active_version_) {
            versions_.erase(old_it);
            deleted_count++;
        } else {
            break;
        }
    }
    
    return deleted_count;
}

json AdapterVersionManager::exportVersionMetadata() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json result;
    result["adapter_id"] = adapter_id_;
    result["active_version"] = active_version_;
    result["versions"] = json::array();
    
    for (const auto& v : versions_) {
        result["versions"].push_back(v.toJSON());
    }
    
    return result;
}

bool AdapterVersionManager::importVersionMetadata(const json& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!metadata.contains("adapter_id") || metadata["adapter_id"] != adapter_id_) {
        spdlog::error("Adapter ID mismatch in metadata import");
        return false;
    }
    
    versions_.clear();
    
    if (metadata.contains("versions")) {
        for (const auto& v_json : metadata["versions"]) {
            versions_.push_back(AdapterVersionInfo::fromJSON(v_json));
        }
    }
    
    if (metadata.contains("active_version")) {
        active_version_ = metadata["active_version"];
    }
    
    return true;
}

// ============================================================================
// Private Methods
// ============================================================================

std::string AdapterVersionManager::computeHash(const json& data) const {
    std::string data_str = data.dump();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data_str.c_str(), data_str.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool AdapterVersionManager::validateSnapshot(const AdapterVersionSnapshot& snapshot) const {
    // Recompute hash and compare
    std::string computed_hash = computeHash(snapshot.adapter_config);
    return computed_hash == snapshot.checksum;
}

std::optional<json> AdapterVersionManager::loadSnapshot(const std::string& storage_path) const {
    try {
        std::ifstream file(storage_path);
        if (!file.is_open()) {
            return std::nullopt;
        }
        
        json data;
        file >> data;
        return data;
    } catch (const std::exception& e) {
        spdlog::error("Failed to load snapshot: {}", e.what());
        return std::nullopt;
    }
}

bool AdapterVersionManager::saveSnapshot(const std::string& version_id, const json& data) {
    try {
        std::string filename = adapter_id_ + "_" + version_id + ".json";
        if (config_.compress_snapshots) {
            filename += ".gz";
        }
        
        std::string path = config_.snapshot_storage_path + "/" + filename;
        std::ofstream file(path);
        
        if (!file.is_open()) {
            spdlog::error("Failed to open snapshot file for writing: {}", path);
            return false;
        }
        
        file << data.dump(2);
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save snapshot: {}", e.what());
        return false;
    }
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
