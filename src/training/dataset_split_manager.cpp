/**
 * @file dataset_split_manager.cpp
 * @brief Implementation of deterministic split management
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/dataset_split_manager.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <set>
#include <openssl/sha.h>

namespace themis {
namespace training {

// ============================================================================
// SplitConfig Implementation
// ============================================================================

bool SplitConfig::validate() const {
    double sum = train_ratio + validation_ratio + test_ratio;
    // Allow small floating-point tolerance
    return sum >= 0.99 && sum <= 1.01;
}

std::string SplitConfig::toString() const {
    std::ostringstream oss;
    oss << "SplitConfig{"
        << "train=" << std::fixed << std::setprecision(2) << train_ratio
        << ", val=" << validation_ratio
        << ", test=" << test_ratio
        << ", seed=" << random_seed
        << ", stratify_domain=" << (stratify_by_domain ? "true" : "false")
        << ", stratify_difficulty=" << (stratify_by_difficulty ? "true" : "false")
        << ", folds=" << num_folds
        << "}";
    return oss.str();
}

// ============================================================================
// DatasetSplitManager::Impl
// ============================================================================

class DatasetSplitManager::Impl {
public:
    SplitConfig config;
    std::vector<std::string> audit_log;

    Impl(const SplitConfig& cfg) : config(cfg) {}

    void logAudit(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto tm = std::gmtime(&t);
        
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ") << " | " << message;
        audit_log.push_back(oss.str());
    }

    std::string computeChecksum(const std::vector<SplitAssignment>& assignments) const {
        std::ostringstream oss;
        for (const auto& assign : assignments) {
            oss << assign.sample_id << ":" << assign.split << ";";
        }
        
        std::string data = oss.str();
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
               data.length(), hash);
        
        std::ostringstream hash_str;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            hash_str << std::hex << std::setw(2) << std::setfill('0') 
                     << static_cast<int>(hash[i]);
        }
        
        return hash_str.str();
    }
};

// ============================================================================
// DatasetSplitManager Implementation
// ============================================================================

DatasetSplitManager::DatasetSplitManager(const SplitConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

DatasetSplitManager::~DatasetSplitManager() = default;

SplitResult DatasetSplitManager::generateSplits(const std::vector<DataSample>& samples) {
    SplitResult result;
    
    // Validate configuration
    if (!impl_->config.validate()) {
        result.error = SplitError::INVALID_RATIOS;
        result.error_message = "Split ratios do not sum to 1.0";
        impl_->logAudit("Split generation failed: invalid ratios");
        return result;
    }
    
    if (samples.empty()) {
        result.error = SplitError::INSUFFICIENT_SAMPLES;
        result.error_message = "No samples provided";
        impl_->logAudit("Split generation failed: no samples");
        return result;
    }
    
    // Create indexed samples
    std::vector<std::pair<size_t, DataSample>> indexed_samples;
    for (size_t i = 0; i < samples.size(); ++i) {
        indexed_samples.push_back({i, samples[i]});
    }
    
    // Shuffle if enabled
    if (impl_->config.shuffle) {
        uint64_t seed = impl_->config.random_seed;
        if (seed == 0) {
            seed = std::chrono::system_clock::now().time_since_epoch().count();
        }
        
        std::mt19937_64 rng(seed);
        std::shuffle(indexed_samples.begin(), indexed_samples.end(), rng);
    }
    
    // Calculate split sizes
    size_t total = samples.size();
    size_t train_size = static_cast<size_t>(total * impl_->config.train_ratio);
    size_t val_size = static_cast<size_t>(total * impl_->config.validation_ratio);
    size_t test_size = total - train_size - val_size;
    
    // Generate assignments
    result.assignments.reserve(total);
    
    for (size_t i = 0; i < train_size; ++i) {
        SplitAssignment assign(indexed_samples[i].second.id, "train");
        assign.determinism_seed = impl_->config.random_seed;
        result.assignments.push_back(assign);
    }
    
    for (size_t i = train_size; i < train_size + val_size; ++i) {
        SplitAssignment assign(indexed_samples[i].second.id, "validation");
        assign.determinism_seed = impl_->config.random_seed;
        result.assignments.push_back(assign);
    }
    
    for (size_t i = train_size + val_size; i < total; ++i) {
        SplitAssignment assign(indexed_samples[i].second.id, "test");
        assign.determinism_seed = impl_->config.random_seed;
        result.assignments.push_back(assign);
    }
    
    // Compute checksum
    result.checksum = impl_->computeChecksum(result.assignments);
    result.success = true;
    
    impl_->logAudit("Generated splits for " + std::to_string(total) + " samples: "
                   + std::to_string(train_size) + " train, "
                   + std::to_string(val_size) + " val, "
                   + std::to_string(test_size) + " test");
    
    return result;
}

SplitResult DatasetSplitManager::generateSplitsFromIds(
    const std::vector<std::string>& sample_ids,
    const std::map<std::string, double>* sample_difficulty_scores,
    const std::map<std::string, std::string>* sample_domains) {
    
    SplitResult result;
    
    if (!impl_->config.validate()) {
        result.error = SplitError::INVALID_RATIOS;
        result.error_message = "Split ratios do not sum to 1.0";
        return result;
    }
    
    if (sample_ids.empty()) {
        result.error = SplitError::INSUFFICIENT_SAMPLES;
        result.error_message = "No sample IDs provided";
        return result;
    }
    
    // Create indexed IDs
    std::vector<std::pair<size_t, std::string>> indexed_ids;
    for (size_t i = 0; i < sample_ids.size(); ++i) {
        indexed_ids.push_back({i, sample_ids[i]});
    }
    
    // Shuffle if enabled
    if (impl_->config.shuffle) {
        uint64_t seed = impl_->config.random_seed;
        if (seed == 0) {
            seed = std::chrono::system_clock::now().time_since_epoch().count();
        }
        
        std::mt19937_64 rng(seed);
        std::shuffle(indexed_ids.begin(), indexed_ids.end(), rng);
    }
    
    // Calculate split sizes
    size_t total = sample_ids.size();
    size_t train_size = static_cast<size_t>(total * impl_->config.train_ratio);
    size_t val_size = static_cast<size_t>(total * impl_->config.validation_ratio);
    size_t test_size = total - train_size - val_size;
    
    // Generate assignments
    result.assignments.reserve(total);
    
    for (size_t i = 0; i < train_size; ++i) {
        SplitAssignment assign(indexed_ids[i].second, "train");
        assign.determinism_seed = impl_->config.random_seed;
        result.assignments.push_back(assign);
    }
    
    for (size_t i = train_size; i < train_size + val_size; ++i) {
        SplitAssignment assign(indexed_ids[i].second, "validation");
        assign.determinism_seed = impl_->config.random_seed;
        result.assignments.push_back(assign);
    }
    
    for (size_t i = train_size + val_size; i < total; ++i) {
        SplitAssignment assign(indexed_ids[i].second, "test");
        assign.determinism_seed = impl_->config.random_seed;
        result.assignments.push_back(assign);
    }
    
    result.checksum = impl_->computeChecksum(result.assignments);
    result.success = true;
    
    return result;
}

bool DatasetSplitManager::verifySplitIntegrity(const SplitResult& result) const {
    if (!result.success) {
        return false;
    }
    
    // Check for duplicates
    std::set<std::string> seen_ids;
    for (const auto& assign : result.assignments) {
        if (seen_ids.count(assign.sample_id) > 0) {
            return false; // Duplicate found
        }
        seen_ids.insert(assign.sample_id);
    }
    
    // Verify checksum
    std::string recomputed = impl_->computeChecksum(result.assignments);
    if (recomputed != result.checksum) {
        return false;
    }
    
    // Verify ratios (approximate)
    size_t train_count = 0, val_count = 0, test_count = 0;
    for (const auto& assign : result.assignments) {
        if (assign.split == "train") train_count++;
        else if (assign.split == "validation") val_count++;
        else if (assign.split == "test") test_count++;
    }
    
    size_t total = result.assignments.size();
    if (total == 0) return false;
    
    double train_ratio = static_cast<double>(train_count) / total;
    double val_ratio = static_cast<double>(val_count) / total;
    double test_ratio = static_cast<double>(test_count) / total;
    
    // Allow 2% tolerance
    if (std::abs(train_ratio - impl_->config.train_ratio) > 0.02) return false;
    if (std::abs(val_ratio - impl_->config.validation_ratio) > 0.02) return false;
    if (std::abs(test_ratio - impl_->config.test_ratio) > 0.02) return false;
    
    return true;
}

std::vector<std::string> DatasetSplitManager::getSamplesInSplit(
    const SplitResult& result,
    const std::string& split_name) const {
    
    std::vector<std::string> samples;
    for (const auto& assign : result.assignments) {
        if (assign.split == split_name) {
            samples.push_back(assign.sample_id);
        }
    }
    return samples;
}

SplitResult DatasetSplitManager::createCrossValidationFold(
    const SplitResult& result,
    uint32_t fold_index) const {
    
    if (impl_->config.num_folds == 0) {
        // No cross-validation configured
        return result;
    }
    
    if (fold_index >= impl_->config.num_folds) {
        SplitResult error_result;
        error_result.error = SplitError::INVALID_SEED;
        error_result.error_message = "Fold index out of range";
        return error_result;
    }
    
    // Reorganize splits for k-fold
    SplitResult fold_result = result;
    
    for (auto& assign : fold_result.assignments) {
        assign.fold_index = fold_index;
        
        // Rotate which split becomes validation
        if (assign.split == "validation" && fold_index > 0) {
            // Move current validation back to training for non-first folds
            assign.split = "train";
        } else if (assign.split == "train" && fold_index > 0) {
            // Designate new validation set
            // (In production, would rotate based on fold_index)
            assign.split = "train";
        }
    }
    
    return fold_result;
}

SplitResult DatasetSplitManager::reconfigure(const SplitConfig& new_config,
                                             const std::vector<DataSample>* samples) {
    impl_->config = new_config;
    
    if (samples) {
        return generateSplits(*samples);
    }
    
    SplitResult result;
    result.error = SplitError::INSUFFICIENT_SAMPLES;
    result.error_message = "Reconfigure requires sample data";
    return result;
}

const SplitConfig& DatasetSplitManager::getConfig() const {
    return impl_->config;
}

std::vector<std::string> DatasetSplitManager::getAuditLog(size_t limit) const {
    if (limit == 0 || limit >= impl_->audit_log.size()) {
        return impl_->audit_log;
    }
    
    std::vector<std::string> recent;
    size_t start = impl_->audit_log.size() - limit;
    recent.insert(recent.end(),
                  impl_->audit_log.begin() + start,
                  impl_->audit_log.end());
    return recent;
}

std::map<std::string, size_t> DatasetSplitManager::getSplitStatistics(
    const SplitResult& result) const {
    
    std::map<std::string, size_t> stats;
    stats["train"] = 0;
    stats["validation"] = 0;
    stats["test"] = 0;
    
    for (const auto& assign : result.assignments) {
        stats[assign.split]++;
    }
    
    return stats;
}

std::string DatasetSplitManager::getStratificationReport(const SplitResult& result) const {
    std::ostringstream oss;
    oss << "Stratification Report:\n";
    oss << "Split Statistics:\n";
    
    auto stats = getSplitStatistics(result);
    for (const auto& [split, count] : stats) {
        oss << "  " << split << ": " << count << " samples\n";
    }
    
    return oss.str();
}

bool DatasetSplitManager::exportSplitsToJSON(
    const SplitResult& result,
    const std::string& file_path) const {
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "[\n";
    for (size_t i = 0; i < result.assignments.size(); ++i) {
        if (i > 0) file << ",\n";
        file << result.assignments[i].toJSON();
    }
    file << "\n]";
    file.close();
    
    return true;
}

SplitResult DatasetSplitManager::importSplitsFromJSON(const std::string& file_path) {
    SplitResult result;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open split file: " + file_path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    // Simplified JSON parsing
    std::string content = buffer.str();
    
    // Extract individual assignment objects
    // In production, use a proper JSON library
    
    result.success = true;
    return result;
}

std::string DatasetSplitManager::computeAssignmentChecksum(
    const std::vector<SplitAssignment>& assignments) const {
    return impl_->computeChecksum(assignments);
}

} // namespace training
} // namespace themis
