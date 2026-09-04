/**
 * @file shader_integrity.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Acceleration module — GPU Shader Binary Integrity Verification
 * ==============================================================
 * Verifies the SHA-256 hash of SPIR-V / GLSL shader binaries against a
 * pre-registered expected-hash manifest before any GPU pipeline is created.
 * This prevents tampered or corrupted shaders from reaching the GPU driver.
 *
 * Dispatch chain position
 * -----------------------
 *   VulkanVectorBackend::initialize()           (graphics_backends.cpp)
 *       └─► ShaderIntegrityVerifier::verifyShader(name, bytes)   ← this file
 *               ├─ compute SHA-256 of binary blob (OpenSSL EVP_DigestUpdate)
 *               ├─ compare against expectedHashes_[name]
 *               └─ return Verified / HashMismatch / NotRegistered
 *
 *   OpenGLVectorBackend::compileShader()         (graphics_backends.cpp)
 *       └─► ShaderIntegrityVerifier::verifyShader(name, glsl_source)
 *
 * Manifest loading
 * ----------------
 *   ShaderIntegrityVerifier::loadManifest(path)  — parses "<name> <sha256>" lines
 *   ShaderIntegrityVerifier::registerExpectedHash(name, hex)  — register individual hash
 *
 * Key interfaces implemented / exposed
 * -------------------------------------
 *   ShaderIntegrityVerifier::instance()          — singleton access
 *   ShaderIntegrityVerifier::verifyShader()      — verify bytes against registered hash
 *   ShaderIntegrityVerifier::loadManifest()      — load hash manifest file
 *   ShaderIntegrityVerifier::registerExpectedHash() — register a single hash entry
 *
 * Related files
 * -------------
 *   include/acceleration/shader_integrity.h     — ShaderIntegrityVerifier declaration + VerifyResult
 *   src/acceleration/graphics_backends.cpp      — Vulkan / OpenGL backends that call verifyShader()
 *   src/acceleration/vulkan_backend_full.cpp    — additional Vulkan shader consumers
 *   src/acceleration/SECURITY.md               — "Untrusted kernel code execution" threat entry
 *   src/acceleration/ARCHITECTURE.md           — Section 8 (Security Considerations)
 */

// Public interface
#include "acceleration/shader_integrity.h"

// SHA-256 via OpenSSL (consistent with plugin_security.cpp)
#include <fstream>
#include <iomanip>
#include <openssl/evp.h>
#include <sstream>

namespace themis {
namespace acceleration {

// ============================================================================
// Singleton
// ============================================================================

ShaderIntegrityVerifier &ShaderIntegrityVerifier::instance() {
    static ShaderIntegrityVerifier inst;
    return inst;
}

// ============================================================================
// Registration
// ============================================================================

void ShaderIntegrityVerifier::registerExpectedHash(const std::string &name, const std::string &hexHash) {
    std::lock_guard<std::mutex> lk(mutex_);
    // Normalise to lower-case
    std::string lower = hexHash;
    for (char &c : lower) {
        if (c >= 'A' && c <= 'F') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    expectedHashes_[name] = std::move(lower);
}

size_t ShaderIntegrityVerifier::loadManifest(const std::string &manifestPath) {
    std::ifstream f(manifestPath);
    if (!f.is_open()) {
        return 0;
    }

    size_t count = 0;
    std::string line = {};
    while (std::getline(f, line)) {
        // Strip comments and whitespace
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        if (line.empty()) {
            continue;
        }

        std::istringstream ss(line);
        std::string name, hash;
        if (ss >> name >> hash && static_cast<int>(name.size()) > 0 && static_cast<int>(hash.size()) == 64) {
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

ShaderIntegrityVerifier::VerifyResult ShaderIntegrityVerifier::verify(const std::string &name,
                                                                      const std::vector<uint32_t> &spvWords) const {
    return verify(name,
                  reinterpret_cast<const uint8_t*>(spvWords.data()),
                  spvWords.size() * sizeof(uint32_t));
}

ShaderIntegrityVerifier::VerifyResult ShaderIntegrityVerifier::verify(const std::string &name, const uint8_t *data,
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
            result.message = "Shader '" + name
                             + "' has no registered expected hash "
                               "(strict mode enabled)";
        } else {
            result.passed  = true;
            result.message = "Shader '" + name
                             + "' not in integrity registry "
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
        result.message = "Shader '" + name
                         + "' integrity FAILED: "
                           "expected "
                         + result.expectedHash + " got " + result.actualHash;
    }
    return result;
}

// ============================================================================
// SHA-256 utility
// ============================================================================

std::string ShaderIntegrityVerifier::sha256Hex(const uint8_t *data, size_t len) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    if (len > 0 && EVP_DigestUpdate(ctx, data, len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream ss = {};
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string ShaderIntegrityVerifier::sha256Hex(const std::vector<uint32_t> &spvWords) {
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

bool ShaderIntegrityVerifier::isRegistered(const std::string &name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return expectedHashes_.count(name) > 0;
}

} // namespace acceleration
} // namespace themis

