/**
 * @file dataset_snapshot_manifest.cpp
 * @brief Implementation of dataset snapshot manifest for RAG training data governance
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/dataset_snapshot_manifest.h"
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <openssl/sha.h>

namespace themis {
namespace training {

// ============================================================================
// EligibilityPolicy Implementation
// ============================================================================

std::string EligibilityPolicy::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"policy_version\": \"" << policy_version << "\",\n";
    oss << "  \"min_quality_score\": " << std::fixed << std::setprecision(2) << min_quality_score << ",\n";
    oss << "  \"max_difficulty_score\": " << max_difficulty_score << ",\n";
    oss << "  \"required_languages\": [";
    for (size_t i = 0; i < required_languages.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << required_languages[i] << "\"";
    }
    oss << "],\n";
    oss << "  \"eligible_domains\": [";
    for (size_t i = 0; i < eligible_domains.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << eligible_domains[i] << "\"";
    }
    oss << "],\n";
    oss << "  \"dedup_threshold\": " << std::fixed << std::setprecision(3) << dedup_threshold << ",\n";
    oss << "  \"pii_handling\": \"" << pii_handling << "\",\n";
    oss << "  \"toxicity_check_enabled\": " << (toxicity_check_enabled ? "true" : "false") << ",\n";
    oss << "  \"max_toxicity_score\": " << std::fixed << std::setprecision(2) << max_toxicity_score << "\n";
    oss << "}";
    return oss.str();
}

EligibilityPolicy EligibilityPolicy::fromJSON(const std::string& json_str) {
    EligibilityPolicy policy;
    // Simplified JSON parsing for core fields
    // In production, use a JSON library like nlohmann/json
    size_t pos = 0;
    
    // Parse policy_version
    if ((pos = json_str.find("\"policy_version\": \"")) != std::string::npos) {
        pos += 20;
        size_t end = json_str.find("\"", pos);
        policy.policy_version = json_str.substr(pos, end - pos);
    }
    
    // Parse min_quality_score
    if ((pos = json_str.find("\"min_quality_score\": ")) != std::string::npos) {
        pos += 21;
        size_t end = json_str.find_first_of(",\n", pos);
        policy.min_quality_score = std::stod(json_str.substr(pos, end - pos));
    }
    
    return policy;
}

// ============================================================================
// SampleLineage Implementation
// ============================================================================

std::string SampleLineage::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"sample_id\": \"" << sample_id << "\",\n";
    oss << "  \"source_document_id\": \"" << source_document_id << "\",\n";
    
    // Format timestamp as ISO 8601
    auto t = std::chrono::system_clock::to_time_t(extraction_timestamp);
    auto tm = std::gmtime(&t);
    oss << "  \"extraction_timestamp\": \"" << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ") << "\",\n";
    
    oss << "  \"processing_version\": \"" << processing_version << "\",\n";
    oss << "  \"modality\": \"" << modality << "\",\n";
    oss << "  \"enrichment_query_hashes\": [";
    for (size_t i = 0; i < enrichment_query_hashes.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << enrichment_query_hashes[i] << "\"";
    }
    oss << "],\n";
    oss << "  \"upstream_sample_ids\": [";
    for (size_t i = 0; i < upstream_sample_ids.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << upstream_sample_ids[i] << "\"";
    }
    oss << "]\n";
    oss << "}";
    return oss.str();
}

SampleLineage SampleLineage::fromJSON(const std::string& json_str) {
    SampleLineage lineage;
    // Simplified JSON parsing
    size_t pos = 0;
    
    // Parse sample_id
    if ((pos = json_str.find("\"sample_id\": \"")) != std::string::npos) {
        pos += 15;
        size_t end = json_str.find("\"", pos);
        lineage.sample_id = json_str.substr(pos, end - pos);
    }
    
    // Parse source_document_id
    if ((pos = json_str.find("\"source_document_id\": \"")) != std::string::npos) {
        pos += 23;
        size_t end = json_str.find("\"", pos);
        lineage.source_document_id = json_str.substr(pos, end - pos);
    }
    
    return lineage;
}

// ============================================================================
// SplitAssignment Implementation
// ============================================================================

std::string SplitAssignment::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"sample_id\": \"" << sample_id << "\",\n";
    oss << "  \"split\": \"" << split << "\",\n";
    oss << "  \"fold_index\": " << fold_index << ",\n";
    oss << "  \"determinism_seed\": " << determinism_seed << ",\n";
    oss << "  \"sample_weight\": " << std::fixed << std::setprecision(4) << sample_weight << "\n";
    oss << "}";
    return oss.str();
}

SplitAssignment SplitAssignment::fromJSON(const std::string& json_str) {
    SplitAssignment assignment;
    size_t pos = 0;
    
    // Parse sample_id
    if ((pos = json_str.find("\"sample_id\": \"")) != std::string::npos) {
        pos += 15;
        size_t end = json_str.find("\"", pos);
        assignment.sample_id = json_str.substr(pos, end - pos);
    }
    
    // Parse split
    if ((pos = json_str.find("\"split\": \"")) != std::string::npos) {
        pos += 10;
        size_t end = json_str.find("\"", pos);
        assignment.split = json_str.substr(pos, end - pos);
    }
    
    return assignment;
}

// ============================================================================
// DatasetSnapshotManifest Implementation
// ============================================================================

std::string DatasetSnapshotManifest::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"snapshot_id\": \"" << snapshot_id << "\",\n";
    oss << "  \"name\": \"" << name << "\",\n";
    
    auto ct = std::chrono::system_clock::to_time_t(created_at);
    auto tm = std::gmtime(&ct);
    oss << "  \"created_at\": \"" << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ") << "\",\n";
    
    oss << "  \"content_checksum\": \"" << content_checksum << "\",\n";
    oss << "  \"description\": \"" << description << "\",\n";
    oss << "  \"selection_config_hash\": \"" << selection_config_hash << "\",\n";
    oss << "  \"governance_policy_id\": \"" << governance_policy_id << "\",\n";
    
    auto mt = std::chrono::system_clock::to_time_t(last_modified);
    tm = std::gmtime(&mt);
    oss << "  \"last_modified\": \"" << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ") << "\",\n";
    
    oss << "  \"total_samples\": " << total_samples << ",\n";
    oss << "  \"train_samples\": " << train_samples << ",\n";
    oss << "  \"validation_samples\": " << validation_samples << ",\n";
    oss << "  \"test_samples\": " << test_samples << ",\n";
    oss << "  \"avg_quality_score\": " << std::fixed << std::setprecision(4) << avg_quality_score << ",\n";
    oss << "  \"avg_difficulty_score\": " << avg_difficulty_score << ",\n";
    oss << "  \"filtered_by_quality\": " << filtered_by_quality << ",\n";
    oss << "  \"filtered_by_dedup\": " << filtered_by_dedup << "\n";
    oss << "}";
    return oss.str();
}

std::string DatasetSnapshotManifest::toYAML() const {
    std::ostringstream oss;
    oss << "# ThemisDB Dataset Snapshot Manifest v" << MANIFEST_VERSION << "\n";
    oss << "snapshot_id: " << snapshot_id << "\n";
    oss << "name: " << name << "\n";
    
    auto ct = std::chrono::system_clock::to_time_t(created_at);
    auto tm = std::gmtime(&ct);
    oss << "created_at: " << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ") << "\n";
    
    oss << "content_checksum: " << content_checksum << "\n";
    oss << "description: " << description << "\n";
    oss << "selection_config_hash: " << selection_config_hash << "\n";
    oss << "governance_policy_id: " << governance_policy_id << "\n";
    
    oss << "\ndata_statistics:\n";
    oss << "  total_samples: " << total_samples << "\n";
    oss << "  train_samples: " << train_samples << "\n";
    oss << "  validation_samples: " << validation_samples << "\n";
    oss << "  test_samples: " << test_samples << "\n";
    oss << "  avg_quality_score: " << std::fixed << std::setprecision(4) << avg_quality_score << "\n";
    oss << "  avg_difficulty_score: " << avg_difficulty_score << "\n";
    oss << "  filtered_by_quality: " << filtered_by_quality << "\n";
    oss << "  filtered_by_dedup: " << filtered_by_dedup << "\n";
    
    oss << "\neligibility_policy:\n";
    oss << "  policy_version: " << eligibility_policy.policy_version << "\n";
    oss << "  min_quality_score: " << eligibility_policy.min_quality_score << "\n";
    oss << "  max_difficulty_score: " << eligibility_policy.max_difficulty_score << "\n";
    oss << "  pii_handling: " << eligibility_policy.pii_handling << "\n";
    
    return oss.str();
}

DatasetSnapshotManifest DatasetSnapshotManifest::fromJSON(const std::string& json_str) {
    DatasetSnapshotManifest manifest;
    // Simplified JSON parsing
    size_t pos = 0;
    
    // Parse snapshot_id
    if ((pos = json_str.find("\"snapshot_id\": \"")) != std::string::npos) {
        pos += 16;
        size_t end = json_str.find("\"", pos);
        manifest.snapshot_id = json_str.substr(pos, end - pos);
    }
    
    // Parse name
    if ((pos = json_str.find("\"name\": \"")) != std::string::npos) {
        pos += 9;
        size_t end = json_str.find("\"", pos);
        manifest.name = json_str.substr(pos, end - pos);
    }
    
    return manifest;
}

DatasetSnapshotManifest DatasetSnapshotManifest::fromYAML(const std::string& yaml_str) {
    DatasetSnapshotManifest manifest;
    // Simplified YAML parsing
    std::istringstream iss(yaml_str);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("snapshot_id:") != std::string::npos) {
            manifest.snapshot_id = line.substr(line.find(':') + 1);
            // Trim whitespace
            manifest.snapshot_id.erase(0, manifest.snapshot_id.find_first_not_of(" \t"));
        } else if (line.find("name:") != std::string::npos) {
            manifest.name = line.substr(line.find(':') + 1);
            manifest.name.erase(0, manifest.name.find_first_not_of(" \t"));
        }
    }
    
    return manifest;
}

bool DatasetSnapshotManifest::saveToFile(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    file << toJSON();
    file.close();
    return true;
}

DatasetSnapshotManifest DatasetSnapshotManifest::loadFromFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open manifest file: " + file_path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return fromJSON(buffer.str());
}

bool DatasetSnapshotManifest::verifyIntegrity() const {
    // Recompute checksum and compare
    DatasetSnapshotManifest temp = *this;
    temp.content_checksum.clear();
    temp.updateChecksum();
    return temp.content_checksum == content_checksum;
}

void DatasetSnapshotManifest::updateChecksum() {
    // Compute SHA-256 of serialized manifest without checksum
    std::string json = toJSON();
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(json.c_str()),
           json.length(), hash);
    
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    content_checksum = oss.str();
}

std::string DatasetSnapshotManifest::getSplitStatistics() const {
    std::ostringstream oss;
    oss << "Dataset Split Statistics:\n";
    oss << "  Total Samples: " << total_samples << "\n";
    oss << "  Training Set:   " << train_samples << " (" 
        << (total_samples > 0 ? (100.0 * train_samples / total_samples) : 0.0) << "%)\n";
    oss << "  Validation Set: " << validation_samples << " ("
        << (total_samples > 0 ? (100.0 * validation_samples / total_samples) : 0.0) << "%)\n";
    oss << "  Test Set:       " << test_samples << " ("
        << (total_samples > 0 ? (100.0 * test_samples / total_samples) : 0.0) << "%)\n";
    return oss.str();
}

std::string DatasetSnapshotManifest::getDomainStatistics() const {
    std::ostringstream oss;
    oss << "Domain Distribution:\n";
    for (const auto& [domain, count] : domain_distribution) {
        oss << "  " << domain << ": " << count << " ("
            << (total_samples > 0 ? (100.0 * count / total_samples) : 0.0) << "%)\n";
    }
    return oss.str();
}

} // namespace training
} // namespace themis
