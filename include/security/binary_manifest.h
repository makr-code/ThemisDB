/**
 * @file binary_manifest.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace security {

struct BinaryFileEntry {
    std::string path;              // Relative path to binary
    std::string sha256_hash;       // SHA-256 hash of file
    size_t size_bytes;             // File size in bytes
    std::string version;           // Binary version (optional)
    
    /**
     * @brief To json.
     * @return Return value.
     */
    nlohmann::json to_json() const;
    /**
     * @brief From json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static BinaryFileEntry from_json(const nlohmann::json& j);
};

class BinaryManifest {
public:
    struct Metadata {
        std::string version;          // ThemisDB version (e.g., "1.4.0")
        std::string build_id;         // Build identifier (e.g., git commit hash)
        std::chrono::system_clock::time_point timestamp;
        std::string release_type;     // "release", "rc", "alpha", "beta"
        std::string platform;         // "linux-x64", "windows-x64", etc.
    };
    
    BinaryManifest() = default;
    /**
     * @brief Binary Manifest.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    explicit BinaryManifest(const Metadata& metadata);
    
    /**
     * @brief Add File.
     * @param[in] entry Input parameter.
     */
    void addFile(const BinaryFileEntry& entry);
    
    const std::vector<BinaryFileEntry>& getFiles() const { return files_; }
    
    const Metadata& getMetadata() const { return metadata_; }
    
    /**
     * @brief Set Metadata.
     * @param[in] metadata Input parameter.
     * @details Implements setMetadata without additional internal calls.
     */
    void setMetadata(const Metadata& metadata) { metadata_ = metadata; }
    
    /**
     * @brief To json.
     * @return Return value.
     */
    nlohmann::json to_json() const;
    
    /**
     * @brief From json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static BinaryManifest from_json(const nlohmann::json& j);
    
    /**
     * @brief Get Canonical Json.
     * @return Return value.
     */
    std::string getCanonicalJson() const;

private:
    Metadata metadata_;
    std::vector<BinaryFileEntry> files_;
};

struct SignedManifest {
    BinaryManifest manifest;
    std::string signature_base64;    // RSA-4096 signature (base64 encoded)
    std::string signature_algorithm; // "RSA-4096-SHA256"
    std::string signer_id;          // Key ID used for signing
    
    /**
     * @brief To json.
     * @return Return value.
     */
    nlohmann::json to_json() const;
    /**
     * @brief From json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static SignedManifest from_json(const nlohmann::json& j);
    
    /**
     * @brief Save To File.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveToFile(const std::string& path) const;
    
    /**
     * @brief Load From File.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static SignedManifest loadFromFile(const std::string& path);
};

} // namespace security
} // namespace themis
