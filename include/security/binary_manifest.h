/**
 * @file binary_manifest.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Binary file entry in manifest
 */
struct BinaryFileEntry {
    std::string path;              // Relative path to binary
    std::string sha256_hash;       // SHA-256 hash of file
    size_t size_bytes;             // File size in bytes
    std::string version;           // Binary version (optional)
    
    nlohmann::json to_json() const;
    static BinaryFileEntry from_json(const nlohmann::json& j);
};

/**
 * @brief Release manifest for binary integrity verification
 * 
 * Contains SHA-256 hashes of all release binaries and is signed with
 * RSA-4096 to ensure authenticity and integrity.
 * 
 * Security features:
 * - RSA-4096 signature for non-repudiation
 * - SHA-256 file hashing for integrity
 * - Timestamp for freshness verification
 * - Version tracking for update validation
 * 
 * Compliance: SOC 2 CC7.1, NIST SP 800-218 (SSDF)
 */
class BinaryManifest {
public:
    /**
     * @brief Manifest metadata
     */
    struct Metadata {
        std::string version;          // ThemisDB version (e.g., "1.4.0")
        std::string build_id;         // Build identifier (e.g., git commit hash)
        std::chrono::system_clock::time_point timestamp;
        std::string release_type;     // "release", "rc", "alpha", "beta"
        std::string platform;         // "linux-x64", "windows-x64", etc.
    };
    
    BinaryManifest() = default;
    explicit BinaryManifest(const Metadata& metadata);
    
    /**
     * @brief Add binary file to manifest
     * 
     * @param entry Binary file entry with path and hash
     */
    void addFile(const BinaryFileEntry& entry);
    
    /**
     * @brief Get all files in manifest
     * 
     * @return Vector of all binary file entries
     */
    const std::vector<BinaryFileEntry>& getFiles() const { return files_; }
    
    /**
     * @brief Get manifest metadata
     * 
     * @return Manifest metadata
     */
    const Metadata& getMetadata() const { return metadata_; }
    
    /**
     * @brief Set manifest metadata
     * 
     * @param metadata Manifest metadata
     */
    void setMetadata(const Metadata& metadata) { metadata_ = metadata; }
    
    /**
     * @brief Serialize manifest to JSON
     * 
     * @return JSON representation of manifest
     */
    nlohmann::json to_json() const;
    
    /**
     * @brief Deserialize manifest from JSON
     * 
     * @param j JSON representation
     * @return BinaryManifest object
     */
    static BinaryManifest from_json(const nlohmann::json& j);
    
    /**
     * @brief Get canonical JSON string for signing
     * 
     * Produces deterministic JSON output with sorted keys for consistent hashing.
     * 
     * @return Canonical JSON string
     */
    std::string getCanonicalJson() const;

private:
    Metadata metadata_;
    std::vector<BinaryFileEntry> files_;
};

/**
 * @brief Signed manifest with RSA-4096 signature
 */
struct SignedManifest {
    BinaryManifest manifest;
    std::string signature_base64;    // RSA-4096 signature (base64 encoded)
    std::string signature_algorithm; // "RSA-4096-SHA256"
    std::string signer_id;          // Key ID used for signing
    
    nlohmann::json to_json() const;
    static SignedManifest from_json(const nlohmann::json& j);
    
    /**
     * @brief Save signed manifest to file
     * 
     * @param path File path
     * @return true if successful
     */
    bool saveToFile(const std::string& path) const;
    
    /**
     * @brief Load signed manifest from file
     * 
     * @param path File path
     * @return SignedManifest object
     */
    static SignedManifest loadFromFile(const std::string& path);
};

} // namespace security
} // namespace themis
