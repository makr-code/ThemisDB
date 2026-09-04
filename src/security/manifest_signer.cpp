/**
 * @file manifest_signer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/manifest_signer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace themis {
namespace security {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct BIO_Deleter {
    void operator()(BIO* p) const { if (p) BIO_free_all(p); }
};

using BIO_ptr = std::unique_ptr<BIO, BIO_Deleter>;

} // anonymous namespace

namespace {
    // Base64 encode helper
    std::string base64Encode(const std::vector<uint8_t>& data) {
        BIO_ptr b64(BIO_new(BIO_f_base64()));
        BIO_ptr bio(BIO_new(BIO_s_mem()));
        BIO_push(b64.get(), bio.release());
        
        BUF_MEM *buffer_ptr;
        
        BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
        BIO_write(b64.get(), data.data(), static_cast<int>(data.size()));
        BIO_flush(b64.get());
        BIO_get_mem_ptr(b64.get(), &buffer_ptr);
        
        std::string result(buffer_ptr->data, buffer_ptr->length);
        
        return result;
    }
    
    // Base64 decode helper
    std::vector<uint8_t> base64Decode(const std::string& encoded) {
        int decode_len = static_cast<int>(encoded.length());
        std::vector<uint8_t> result(decode_len);
        
        BIO_ptr bio(BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.length())));
        BIO_ptr b64(BIO_new(BIO_f_base64()));
        BIO_push(b64.get(), bio.release());
        
        BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
        int length = BIO_read(b64.get(), result.data(), decode_len);
        
        result.resize(length);
        return result;
    }
}

// ============================================================================
// ManifestSigner Implementation
// ============================================================================

ManifestSigner::ManifestSigner(
    std::shared_ptr<SigningService> signing_service,
    const Config& config
) : signing_service_(signing_service), config_(config) {
    if (!signing_service_) {
        throw std::invalid_argument("SigningService cannot be null");
    }
}

std::string ManifestSigner::computeFileSHA256(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    const size_t buffer_size = 8192;
    char buffer[buffer_size];
    
    while (file.read(buffer, buffer_size) || file.gcount() > 0) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return oss.str();
}

bool ManifestSigner::matchesPattern(const std::string& filename, const std::string& pattern) {
    // Simple wildcard matching (* and ?)
    if (pattern == "*") {
        return true;
    }
    
    size_t pattern_idx = 0;
    size_t filename_idx = 0;
    
    while (pattern_idx < pattern.length() && filename_idx < filename.length()) {
        if (pattern[pattern_idx] == '*') {
            // Try to match rest of pattern with rest of filename
            pattern_idx++;
            if (pattern_idx == pattern.length()) {
                return true;  // * at end matches everything
            }
            
            // Find next matching character
            while (filename_idx < filename.length()) {
                if (matchesPattern(filename.substr(filename_idx), pattern.substr(pattern_idx))) {
                    return true;
                }
                filename_idx++;
            }
            return false;
        } else if (pattern[pattern_idx] == '?' || pattern[pattern_idx] == filename[filename_idx]) {
            pattern_idx++;
            filename_idx++;
        } else {
            return false;
        }
    }
    
    // Check if both reached end
    while (pattern_idx < pattern.length() && pattern[pattern_idx] == '*') {
        pattern_idx++;
    }
    
    return pattern_idx == pattern.length() && filename_idx == filename.length();
}

BinaryManifest ManifestSigner::generateManifest(
    const std::string& root_path,
    const std::string& version,
    const std::string& build_id,
    const std::vector<std::string>& include_patterns
) {
    BinaryManifest::Metadata metadata;
    metadata.version = version;
    metadata.build_id = build_id;
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.release_type = "release";
    
    // Detect platform
    #ifdef _WIN32
        metadata.platform = "windows-x64";
    #elif __linux__
        metadata.platform = "linux-x64";
    #elif __APPLE__
        metadata.platform = "macos-x64";
    #else
        metadata.platform = "unknown";
    #endif
    
    BinaryManifest manifest(metadata);
    
    // Recursively scan directory
    namespace fs = std::filesystem;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(root_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // Check if file matches any pattern
                bool matches = false;
                for (const auto& pattern : include_patterns) {
                    if (matchesPattern(filename, pattern)) {
                        matches = true;
                        break;
                    }
                }
                
                if (!matches) {
                    continue;
                }
                
                BinaryFileEntry file_entry;
                file_entry.path = fs::relative(entry.path(), root_path).string();
                file_entry.size_bytes = entry.file_size();
                file_entry.sha256_hash = computeFileSHA256(entry.path().string());
                file_entry.version = version;
                
                manifest.addFile(file_entry);
                
                spdlog::debug("Added to manifest: {} ({})", file_entry.path, file_entry.sha256_hash);
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Error scanning directory: {}", e.what());
        throw;
    }
    
    spdlog::info("Generated manifest with {} files", manifest.getFiles().size());
    return manifest;
}

SignedManifest ManifestSigner::signManifest(const BinaryManifest& manifest) {
    // Get canonical JSON for signing
    std::string canonical_json = manifest.getCanonicalJson();
    std::vector<uint8_t> data(canonical_json.begin(), canonical_json.end());
    
    // Sign with RSA-4096
    std::lock_guard<std::mutex> lock(mtx_);
    SigningResult result = signing_service_->sign(data, config_.key_id);
    
    if (!result.error.empty()) {
        throw std::runtime_error("Failed to sign manifest: " + result.error);
    }
    
    SignedManifest signed_manifest;
    signed_manifest.manifest = manifest;
    signed_manifest.signature_base64 = base64Encode(result.signature);
    signed_manifest.signature_algorithm = config_.algorithm;
    signed_manifest.signer_id = config_.key_id;
    
    spdlog::info("Manifest signed with key: {}", config_.key_id);
    return signed_manifest;
}

bool ManifestSigner::verifySignature(const SignedManifest& signed_manifest) {
    // Get canonical JSON
    std::string canonical_json = signed_manifest.manifest.getCanonicalJson();
    std::vector<uint8_t> data(canonical_json.begin(), canonical_json.end());
    
    // Decode signature
    std::vector<uint8_t> signature = base64Decode(signed_manifest.signature_base64);
    
    // Verify signature
    std::lock_guard<std::mutex> lock(mtx_);
    bool valid = signing_service_->verify(data, signature, signed_manifest.signer_id);
    
    if (valid) {
        spdlog::info("Manifest signature verified successfully");
    } else {
        spdlog::error("Manifest signature verification FAILED");
    }
    
    return valid;
}

ManifestSigner::VerificationResult ManifestSigner::verifyBinaries(
    const SignedManifest& signed_manifest,
    const std::string& root_path
) {
    VerificationResult result;
    
    // First verify signature
    result.signature_valid = verifySignature(signed_manifest);
    if (!result.signature_valid) {
        result.error_message = "Signature verification failed";
        return result;
    }
    
    // Verify each file
    namespace fs = std::filesystem;
    
    for (const auto& file_entry : signed_manifest.manifest.getFiles()) {
        fs::path file_path = fs::path(root_path) / file_entry.path;
        
        // Check if file exists
        if (!fs::exists(file_path)) {
            result.missing_files.push_back(file_entry.path);
            continue;
        }
        
        // Verify file size
        size_t actual_size = fs::file_size(file_path);
        if (actual_size != file_entry.size_bytes) {
            result.modified_files.push_back(file_entry.path + " (size mismatch)");
            continue;
        }
        
        // Verify SHA-256 hash
        try {
            std::string actual_hash = computeFileSHA256(file_path.string());
            if (actual_hash != file_entry.sha256_hash) {
                result.modified_files.push_back(file_entry.path + " (hash mismatch)");
            }
        } catch (const std::exception& e) {
            result.error_message = "Failed to compute hash for " + file_entry.path + ": " + e.what();
            return result;
        }
    }
    
    // Check if all files are valid
    result.files_valid = result.missing_files.empty() && result.modified_files.empty();
    
    if (result.files_valid) {
        spdlog::info("All {} binary files verified successfully", 
                    signed_manifest.manifest.getFiles().size());
    } else {
        spdlog::error("Binary verification failed: {} missing, {} modified",
                     result.missing_files.size(),static_cast<int>(result.modified_files.size()));
    }
    
    return result;
}

// ============================================================================
// StartupVerifier Implementation
// ============================================================================

StartupVerifier::StartupVerifier(
    std::shared_ptr<SigningService> signing_service,
    const Config& config
) : signing_service_(signing_service), config_(config) {
}

bool StartupVerifier::verify() {
    spdlog::info("Starting binary integrity verification...");
    
    try {
        // Load signed manifest
        SignedManifest signed_manifest = SignedManifest::loadFromFile(config_.manifest_path);
        
        // Verify binaries
        ManifestSigner::Config signer_config;
        signer_config.key_id = signed_manifest.signer_id;
        
        ManifestSigner signer(signing_service_, signer_config);
        result_ = signer.verifyBinaries(signed_manifest, config_.binaries_root);
        
        if (config_.log_results) {
            if (result_.signature_valid && result_.files_valid) {
                spdlog::info("✓ Binary integrity verification PASSED");
            } else {
                spdlog::error("✗ Binary integrity verification FAILED");
                
                if (!result_.signature_valid) {
                    spdlog::error("  - Signature verification failed");
                }
                
                if (!result_.missing_files.empty()) {
                    spdlog::error("  - Missing files: {}",static_cast<int>(result_.missing_files.size()));
                    for (const auto& file : result_.missing_files) {
                        spdlog::error("    * {}", file);
                    }
                }
                
                if (!result_.modified_files.empty()) {
                    spdlog::error("  - Modified files: {}",static_cast<int>(result_.modified_files.size()));
                    for (const auto& file : result_.modified_files) {
                        spdlog::error("    * {}", file);
                    }
                }
            }
        }
        
        bool success = result_.signature_valid && result_.files_valid;
        
        if (!success && config_.fail_on_invalid) {
            spdlog::critical("Binary integrity verification failed - exiting");
            std::exit(1);
        }
        
        return success;
        
    } catch (const std::exception& e) {
        spdlog::error("Binary integrity verification error: {}", e.what());
        result_.error_message = e.what();
        
        if (config_.fail_on_invalid) {
            std::exit(1);
        }
        
        return false;
    }
}

} // namespace security
} // namespace themis

