/**
 * @file binary_manifest.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/binary_manifest.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>

namespace themis {
namespace security {

// ============================================================================
// BinaryFileEntry Implementation
// ============================================================================

nlohmann::json BinaryFileEntry::to_json() const {
    nlohmann::json j;
    j["path"] = path;
    j["sha256_hash"] = sha256_hash;
    j["size_bytes"] = size_bytes;
    if (!version.empty()) {
        j["version"] = version;
    }
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: at(), contains().
 */
BinaryFileEntry BinaryFileEntry::from_json(const nlohmann::json& j) {
    BinaryFileEntry entry;
    entry.path = j.at("path").get<std::string>();
    entry.sha256_hash = j.at("sha256_hash").get<std::string>();
    entry.size_bytes = j.at("size_bytes").get<size_t>();
    if (j.contains("version")) {
        entry.version = j.at("version").get<std::string>();
    }
    return entry;
}

// ============================================================================
// BinaryManifest Implementation
// ============================================================================

BinaryManifest::BinaryManifest(const Metadata& metadata)
    : metadata_(metadata) {
}

/**
 * @brief Add File.
 * @param[in] entry Input parameter.
 * @details Calls: push_back().
 */
void BinaryManifest::addFile(const BinaryFileEntry& entry) {
    files_.push_back(entry);
}

nlohmann::json BinaryManifest::to_json() const {
    nlohmann::json j;
    
    // Metadata
    j["metadata"]["version"] = metadata_.version;
    j["metadata"]["build_id"] = metadata_.build_id;
    j["metadata"]["timestamp"] = std::chrono::system_clock::to_time_t(metadata_.timestamp);
    j["metadata"]["release_type"] = metadata_.release_type;
    j["metadata"]["platform"] = metadata_.platform;
    
    // Files
    j["files"] = nlohmann::json::array();
    for (const auto& file : files_) {
        j["files"].push_back(file.to_json());
    }
    
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: at(), std::chrono::system_clock::from_time_t(), push_back().
 */
BinaryManifest BinaryManifest::from_json(const nlohmann::json& j) {
    BinaryManifest manifest;
    
    // Metadata
    manifest.metadata_.version = j.at("metadata").at("version").get<std::string>();
    manifest.metadata_.build_id = j.at("metadata").at("build_id").get<std::string>();
    manifest.metadata_.timestamp = std::chrono::system_clock::from_time_t(
        j.at("metadata").at("timestamp").get<time_t>()
    );
    manifest.metadata_.release_type = j.at("metadata").at("release_type").get<std::string>();
    manifest.metadata_.platform = j.at("metadata").at("platform").get<std::string>();
    
    // Files
    for (const auto& file_json : j.at("files")) {
        manifest.files_.push_back(BinaryFileEntry::from_json(file_json));
    }
    
    return manifest;
}

std::string BinaryManifest::getCanonicalJson() const {
    // Use sorted JSON with 2-space indentation for canonical format
    nlohmann::json j = to_json();
    return j.dump(2);
}

// ============================================================================
// SignedManifest Implementation
// ============================================================================

nlohmann::json SignedManifest::to_json() const {
    nlohmann::json j;
    j["manifest"] = manifest.to_json();
    j["signature"] = signature_base64;
    j["signature_algorithm"] = signature_algorithm;
    j["signer_id"] = signer_id;
    return j;
}

/**
 * @brief From json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: at().
 */
SignedManifest SignedManifest::from_json(const nlohmann::json& j) {
    SignedManifest sm;
    sm.manifest = BinaryManifest::from_json(j.at("manifest"));
    sm.signature_base64 = j.at("signature").get<std::string>();
    sm.signature_algorithm = j.at("signature_algorithm").get<std::string>();
    sm.signer_id = j.at("signer_id").get<std::string>();
    return sm;
}

bool SignedManifest::saveToFile(const std::string& path) const {
    try {
        /**
         * @brief File.
         * @param[in] path Input parameter.
         * @return Return value.
         */
        std::ofstream file(path);
        if (!file.is_open()) {
            spdlog::error("Failed to open file for writing: {}", path);
            return false;
        }
        
        nlohmann::json j = to_json();
        file << j.dump(2);
        file.close();
        
        spdlog::info("Signed manifest saved to: {}", path);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save signed manifest: {}", e.what());
        return false;
    }
}

/**
 * @brief Load From File.
 * @param[in] path Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: file(), is_open(), close(), SignedManifest::from_json().
 */
SignedManifest SignedManifest::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open manifest file: " + path);
    }
    
    nlohmann::json j;
    file >> j;
    file.close();
    
    return SignedManifest::from_json(j);
}

} // namespace security
} // namespace themis
