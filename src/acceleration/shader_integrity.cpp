/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shader_integrity.cpp                               ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-04-14 06:59:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     194                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 326c1184f7  2026-02-21  feat(acceleration): Phase 4.1 — ShaderIntegrityVerifier S... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/shader_integrity.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// SHA-256 via OpenSSL (consistent with plugin_security.cpp)
#include <openssl/evp.h>

namespace themis {
namespace acceleration {

// ============================================================================
// Singleton
// ============================================================================

ShaderIntegrityVerifier& ShaderIntegrityVerifier::instance() {
    static ShaderIntegrityVerifier inst;
    return inst;
}

// ============================================================================
// Registration
// ============================================================================

void ShaderIntegrityVerifier::registerExpectedHash(const std::string& name,
                                                    const std::string& hexHash) {
    std::lock_guard<std::mutex> lk(mutex_);
    // Normalise to lower-case
    std::string lower = hexHash;
    for (char& c : lower) {
        if (c >= 'A' && c <= 'F') c = static_cast<char>(c - 'A' + 'a');
    }
    expectedHashes_[name] = std::move(lower);
}

size_t ShaderIntegrityVerifier::loadManifest(const std::string& manifestPath) {
    std::ifstream f(manifestPath);
    if (!f.is_open()) return 0;

    size_t count = 0;
    std::string line;
    while (std::getline(f, line)) {
        // Strip comments and whitespace
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string name, hash;
        if (ss >> name >> hash && name.size() > 0 && hash.size() == 64) {
            registerExpectedHash(name, hash);
            ++count;
        }
    }
    return count;
}

void ShaderIntegrityVerifier::clearRegistry() {
    std::lock_guard<std::mutex> lk(mutex_);
    expectedHashes_.clear();
}

// ============================================================================
// Verification
// ============================================================================

ShaderIntegrityVerifier::VerifyResult
ShaderIntegrityVerifier::verify(const std::string& name,
                                 const std::vector<uint32_t>& spvWords) const {
    return verify(name,
                  reinterpret_cast<const uint8_t*>(spvWords.data()),
                  spvWords.size() * sizeof(uint32_t));
}

ShaderIntegrityVerifier::VerifyResult
ShaderIntegrityVerifier::verify(const std::string& name,
                                 const uint8_t* data,
                                 size_t byteLen) const {
    VerifyResult result;
    result.name       = name;
    result.actualHash = sha256Hex(data, byteLen);

    std::lock_guard<std::mutex> lk(mutex_);

    auto it = expectedHashes_.find(name);
    if (it == expectedHashes_.end()) {
        // No hash registered for this shader
        if (strict_) {
            result.passed  = false;
            result.message = "Shader '" + name + "' has no registered expected hash "
                             "(strict mode enabled)";
        } else {
            result.passed  = true;
            result.message = "Shader '" + name + "' not in integrity registry "
                             "(no-op in non-strict mode)";
        }
        return result;
    }

    result.expectedHash = it->second;
    if (result.actualHash == result.expectedHash) {
        result.passed  = true;
        result.message = "Shader '" + name + "' integrity OK";
    } else {
        result.passed  = false;
        result.message = "Shader '" + name + "' integrity FAILED: "
                         "expected " + result.expectedHash +
                         " got " + result.actualHash;
    }
    return result;
}

// ============================================================================
// SHA-256 utility
// ============================================================================

std::string ShaderIntegrityVerifier::sha256Hex(const uint8_t* data, size_t len) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    if (len > 0 && EVP_DigestUpdate(ctx, data, len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hashLen = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream ss;
    for (unsigned int i = 0; i < hashLen; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    return ss.str();
}

std::string ShaderIntegrityVerifier::sha256Hex(const std::vector<uint32_t>& spvWords) {
    return sha256Hex(reinterpret_cast<const uint8_t*>(spvWords.data()),
                     spvWords.size() * sizeof(uint32_t));
}

// ============================================================================
// Misc
// ============================================================================

void ShaderIntegrityVerifier::setStrictMode(bool strict) {
    std::lock_guard<std::mutex> lk(mutex_);
    strict_ = strict;
}

bool ShaderIntegrityVerifier::strictMode() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return strict_;
}

bool ShaderIntegrityVerifier::isRegistered(const std::string& name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return expectedHashes_.count(name) > 0;
}

} // namespace acceleration
} // namespace themis
