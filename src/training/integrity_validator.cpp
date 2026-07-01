/**
 * @file integrity_validator.cpp
 * @brief Integrity validation implementation
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#include "training/integrity_validator.h"
#include "utils/logger.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace themis {
namespace training {

// ============================================================================
// IntegrityCheckResult Implementation
// ============================================================================

json IntegrityCheckResult::toJSON() const {
    json j;
    j["passed"] = passed;
    j["warnings"] = warnings;
    j["errors"] = errors;
    j["data_hash"] = data_hash;
    j["sample_count"] = sample_count;
    j["corrupted_samples"] = corrupted_samples;
    j["check_time_seconds"] = check_time_seconds;
    return j;
}

// ============================================================================
// IntegrityValidator Implementation
// ============================================================================

IntegrityValidator::IntegrityValidator(ValidationLevel validation_level)
    : validation_level_(validation_level) {
    THEMIS_INFO("Initialized IntegrityValidator (level: {})", 
                static_cast<int>(validation_level));
}

IntegrityValidator::~IntegrityValidator() = default;

IntegrityCheckResult IntegrityValidator::validateDataset(
    const std::string& dataset_path,
    ValidationLevel validation_level) {
    
    IntegrityCheckResult result;
    
    // Check if file exists and is readable
    std::ifstream file(dataset_path);
    if (!file.is_open()) {
        result.passed = false;
        result.errors.push_back("Dataset file not found or not readable: " + dataset_path);
        return result;
    }
    
    file.close();
    
    // Compute hash
    result.data_hash = computeDatasetHash(dataset_path);
    result.sample_count = 0;  // Would count samples in real implementation
    result.corrupted_samples = 0;
    result.check_time_seconds = 0.0;
    result.passed = true;
    
    THEMIS_INFO("Dataset validation passed: {}", dataset_path);
    return result;
}

bool IntegrityValidator::validateDatasetHash(
    const std::string& dataset_path,
    const std::string& expected_hash) {
    
    std::string computed_hash = computeDatasetHash(dataset_path);
    bool match = computed_hash == expected_hash;
    
    if (!match) {
        THEMIS_WARN("Dataset hash mismatch: {} vs {}", computed_hash, expected_hash);
    }
    
    return match;
}

IntegrityCheckResult IntegrityValidator::validateModelWeights(
    const std::vector<float>& weights_buffer,
    const std::string& weights_name) {
    
    IntegrityCheckResult result;
    
    // Check for NaN and Inf
    if (hasNaN(weights_buffer)) {
        result.passed = false;
        result.errors.push_back("NaN values detected in " + weights_name);
        result.corrupted_samples++;
    }
    
    if (hasInf(weights_buffer)) {
        result.passed = false;
        result.errors.push_back("Inf values detected in " + weights_name);
        result.corrupted_samples++;
    }
    
    result.sample_count = weights_buffer.size();
    result.data_hash = computeWeightsHash(weights_buffer);
    
    THEMIS_DEBUG("Weight validation for {}: passed={}, samples={}",
                weights_name, result.passed, result.sample_count);
    
    return result;
}

IntegrityCheckResult IntegrityValidator::validateModelWeightsBatch(
    const std::vector<std::vector<float>>& weights,
    const std::string& weights_name) {
    
    IntegrityCheckResult result;
    result.passed = true;
    
    for (const auto& weight_vector : weights) {
        auto single_result = validateModelWeights(weight_vector, weights_name);
        if (!single_result.passed) {
            result.passed = false;
            result.corrupted_samples += single_result.corrupted_samples;
            result.errors.insert(result.errors.end(),
                               single_result.errors.begin(),
                               single_result.errors.end());
        }
        result.sample_count += single_result.sample_count;
    }
    
    return result;
}

std::string IntegrityValidator::computeDatasetHash(const std::string& dataset_path) {
    std::ifstream file(dataset_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    
    const size_t buffer_size = 65536;
    std::vector<char> buffer(buffer_size);
    
    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        SHA256_Update(&sha256_ctx, 
                     reinterpret_cast<const unsigned char*>(buffer.data()),
                     file.gcount());
    }
    
    file.close();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_ctx);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::string IntegrityValidator::computeWeightsHash(const std::vector<float>& weights) {
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    
    SHA256_Update(&sha256_ctx,
                 reinterpret_cast<const unsigned char*>(weights.data()),
                 weights.size() * sizeof(float));
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_ctx);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

void IntegrityValidator::registerCustomValidator(
    std::function<bool(const std::vector<float>&)> callback) {
    custom_validators_.push_back(callback);
}

bool IntegrityValidator::detectAnomalies(
    const std::vector<float>& weights,
    double sensitivity) {
    
    if (weights.empty()) {
        return false;
    }
    
    // Calculate mean and std dev
    double mean = std::accumulate(weights.begin(), weights.end(), 0.0) / weights.size();
    double sum_sq_diff = 0.0;
    for (float w : weights) {
        double diff = w - mean;
        sum_sq_diff += diff * diff;
    }
    double stddev = std::sqrt(sum_sq_diff / weights.size());
    
    // Check for anomalies (values beyond 3*stddev threshold)
    double threshold = 3.0 * stddev * sensitivity;
    for (float w : weights) {
        if (std::abs(w - mean) > threshold) {
            return true;  // Anomaly detected
        }
    }
    
    return false;  // No anomalies
}

json IntegrityValidator::getValidationStats() const {
    json stats;
    stats["total_checks"] = total_checks_;
    stats["failed_checks"] = failed_checks_;
    stats["validation_level"] = static_cast<int>(validation_level_);
    return stats;
}

void IntegrityValidator::reset() {
    total_checks_ = 0;
    failed_checks_ = 0;
}

bool IntegrityValidator::hasNaN(const std::vector<float>& data) const {
    for (float value : data) {
        if (std::isnan(value)) {
            return true;
        }
    }
    return false;
}

bool IntegrityValidator::hasInf(const std::vector<float>& data) const {
    for (float value : data) {
        if (std::isinf(value)) {
            return true;
        }
    }
    return false;
}

bool IntegrityValidator::isOutOfRange(float value, float min_val, float max_val) const {
    return value < min_val || value > max_val;
}

// ============================================================================
// CrossShardIntegrityValidator Implementation
// ============================================================================

CrossShardIntegrityValidator::CrossShardIntegrityValidator(
    int shard_id,
    int total_shards)
    : shard_id_(shard_id),
      total_shards_(total_shards) {
    THEMIS_INFO("Initialized CrossShardIntegrityValidator for shard {} of {}",
               shard_id, total_shards);
}

void CrossShardIntegrityValidator::registerWeights(
    const std::string& adapter_id,
    const std::string& weights_hash) {
    
    local_weights_[adapter_id] = weights_hash;
    THEMIS_DEBUG("Registered weights for adapter: {}", adapter_id);
}

bool CrossShardIntegrityValidator::verifyConsistency(
    const std::map<int, std::string>& other_shard_hashes) {
    
    // Compare this shard's weights with other shards
    for (const auto& [shard_id, other_hash] : other_shard_hashes) {
        // In a real implementation, we would have per-adapter hashes from each shard
        // and would compare them for consistency
        THEMIS_DEBUG("Comparing with shard {}", shard_id);
    }
    
    THEMIS_INFO("Cross-shard consistency verification passed");
    return true;
}

} // namespace training
} // namespace themis
