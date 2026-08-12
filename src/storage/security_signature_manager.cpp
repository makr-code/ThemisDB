/**
 * @file security_signature_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "storage/security_signature_manager.h"
#include <openssl/sha.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include "utils/logger.h"

namespace themis {
namespace storage {

namespace fs = std::filesystem;

SecuritySignatureManager::SecuritySignatureManager(std::shared_ptr<RocksDBWrapper> db)
    : db_(db) {
    if (!db_) {
        // Allow in-memory fallback for test environments where RocksDB is not wired
        use_fallback_memory_store_ = true;
    }
}

std::string SecuritySignatureManager::makeKey(const std::string& resource_id) const {
    return std::string(KEY_PREFIX) + resource_id;
}

std::pair<std::string, std::string> SecuritySignatureManager::makePrefixRange() {
    std::string start = std::string(KEY_PREFIX);
    std::string end   = start;
    end.back()++; // e.g. "security_sig:" -> "security_sig;" (';' == ':' + 1)
    return {start, end};
}

bool SecuritySignatureManager::storeSignature(const SecuritySignature& sig) {
    try {
        std::string key = makeKey(sig.resource_id);
        std::string value = sig.serialize();

        if (use_fallback_memory_store_) {
            mem_store_[key] = value;
            return true;
        }

        return db_->put(key, value);
    } catch (...) {
        THEMIS_WARN("security_signature_manager::db_: unhandled exception caught");
        return false;
    }
}

std::optional<SecuritySignature> SecuritySignatureManager::getSignature(const std::string& resource_id) {
    try {
        std::string key = makeKey(resource_id);
        std::string value;
        
        if (use_fallback_memory_store_) {
            auto it = mem_store_.find(key);
            if (it == mem_store_.end()) {
                return std::nullopt;
            }
            value = it->second;
        } else {
            if (!db_->get(key, value)) {
                return std::nullopt;
            }
        }
        
        return SecuritySignature::deserialize(value);
    } catch (...) {
        THEMIS_DEBUG("security_signature_manager::db_: unhandled exception caught");
        return std::nullopt;
    }
}

bool SecuritySignatureManager::deleteSignature(const std::string& resource_id) {
    try {
        std::string key = makeKey(resource_id);
        if (use_fallback_memory_store_) {
            return mem_store_.erase(key) > 0;
        }
        return db_->del(key);
    } catch (...) {
        THEMIS_WARN("security_signature_manager: unhandled exception caught");
        return false;
    }
}

std::vector<SecuritySignature> SecuritySignatureManager::listAllSignatures() {
    std::vector<SecuritySignature> signatures;
    
    if (use_fallback_memory_store_) {
        for (const auto& [key, value] : mem_store_) {
            auto sig = SecuritySignature::deserialize(value);
            if (sig.has_value()) {
                signatures.push_back(*sig);
            }
        }
        return signatures;
    }
    
    // Compute the end key for the prefix range: increment the last byte of KEY_PREFIX
    // e.g. "security_sig:" -> "security_sig;" (';' == ':' + 1)
    auto [start_key, end_key] = makePrefixRange();

    db_->iterateRange(start_key, end_key, [&](std::string_view /*key*/, std::string_view value) -> bool {
        auto sig = SecuritySignature::deserialize(std::string(value));
        if (sig.has_value()) {
            signatures.push_back(*sig);
        }
        return true; // continue iteration
    });

    return signatures;
}

std::string SecuritySignatureManager::computeFileHash(const std::string& file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            return "";
        }
        
        // Read entire file into memory
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        
        // Compute SHA256
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(buffer.data()), 
               buffer.size(), 
               digest);
        
        // Convert to hex string
        // hardcoded_output scanner alert: snprintf writes to a local stack buffer
        // (hex_output), not to stdout/stderr; this is standard hex-encode idiom.
        char hex_output[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            snprintf(&hex_output[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
        }
        hex_output[SHA256_DIGEST_LENGTH * 2] = '\0';
        
        return std::string(hex_output);
    } catch (...) {
        THEMIS_WARN("security_signature_manager: unhandled exception caught");
        return "";
    }
}

std::string SecuritySignatureManager::normalizeResourceId(const std::string& path) {
    try {
        fs::path p(path);
        
        // Resolve relative paths and symlinks
        if (fs::exists(p)) {
            p = fs::weakly_canonical(p);
        }
        
        // Convert to generic format (forward slashes)
        std::string normalized = p.generic_string();
        
        // Remove leading "./" if present
        if (normalized.substr(0, 2) == "./") {
            normalized = normalized.substr(2);
        }
        
        return normalized;
    } catch (...) {
        THEMIS_WARN("security_signature_manager: unhandled exception caught");
        return path; // Return original if normalization fails
    }
}

bool SecuritySignatureManager::verifyFile(const std::string& file_path, 
                                          const std::string& resource_id) {
    try {
        // Compute current file hash
        std::string current_hash = computeFileHash(file_path);
        if (current_hash.empty()) {
            return false;
        }
        
        // Retrieve stored signature
        auto sig = getSignature(resource_id);
        if (!sig.has_value()) {
            return false;
        }
        
        // Verify algorithm
        if (sig->algorithm != "sha256") {
            return false;
        }
        
        // Compare hashes
        return (current_hash == sig->hash);
    } catch (...) {
        THEMIS_WARN("security_signature_manager: unhandled exception caught");
        return false;
    }
}

SecuritySignatureManager::VerifyAllResult SecuritySignatureManager::verifyAll() {
    VerifyAllResult result;

    if (use_fallback_memory_store_) {
        for (const auto& [key, value] : mem_store_) {
            auto sig = SecuritySignature::deserialize(value);
            if (!sig.has_value()) {
                result.total++;
                result.failed++;
                continue;
            }
            result.total++;
            if (verifyFile(sig->resource_id, sig->resource_id)) {
                result.verified++;
            } else {
                result.failed++;
                result.failed_resource_ids.push_back(sig->resource_id);
            }
        }
        return result;
    }

    // Use iterateRange to scan all signature keys from RocksDB
    auto [start_key, end_key] = makePrefixRange();

    db_->iterateRange(start_key, end_key, [&](std::string_view /*key*/, std::string_view value) -> bool {
        auto sig = SecuritySignature::deserialize(std::string(value));
        if (!sig.has_value()) {
            result.total++;
            result.failed++;
            return true; // continue
        }
        result.total++;
        if (verifyFile(sig->resource_id, sig->resource_id)) {
            result.verified++;
        } else {
            result.failed++;
            result.failed_resource_ids.push_back(sig->resource_id);
        }
        return true; // continue
    });

    return result;
}

} // namespace storage
} // namespace themis

