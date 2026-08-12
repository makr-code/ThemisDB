/**
 * @file shader_integrity.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// ShaderIntegrityVerifier — Phase 4.1 Shader Integrity Verification
//
// Verifies SHA-256 hashes of SPIR-V shader bytes before they are submitted to
// the Vulkan runtime.  This prevents tampered or substituted shaders from
// executing on the GPU.
//
// Usage:
//   // Register expected hashes at startup (e.g. from a signed manifest):
//   ShaderIntegrityVerifier::instance().registerExpectedHash(
//       "l2_distance.comp.spv", "<hex-sha256>");
//
//   // Verify before loading:
//   ShaderIntegrityVerifier::VerifyResult r =
//       ShaderIntegrityVerifier::instance().verify(
//           "l2_distance.comp.spv", spv_bytes);
//   if (!r.passed) throw std::runtime_error(r.message);

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace themis {
namespace acceleration {

/** @brief Shader integrity verifier. */
class ShaderIntegrityVerifier {
public:
    // -------------------------------------------------------------------------
    // Result of a single verification call
    // -------------------------------------------------------------------------
    struct VerifyResult {
        bool        passed  = false;
        std::string name;           // shader file name / identifier
        std::string actualHash;     // SHA-256 hex string of the bytes provided
        std::string expectedHash;   // registered expected hash (empty = not registered)
        std::string message;        // human-readable reason for failure
    };

    // -------------------------------------------------------------------------
    // Singleton access
    // -------------------------------------------------------------------------
    static ShaderIntegrityVerifier& instance();

    // Non-copyable, non-movable
    ShaderIntegrityVerifier(const ShaderIntegrityVerifier&) = delete;
    ShaderIntegrityVerifier& operator=(const ShaderIntegrityVerifier&) = delete;

    // -------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------

    /// Register the expected SHA-256 hex hash for a shader identified by name.
    /// Call this once at startup (e.g. from a signed manifest file).
    /// @param name     Logical shader identifier, e.g. "l2_distance.comp.spv"
    /// @param hexHash  Expected SHA-256 as 64-character lower-case hex string
    void registerExpectedHash(const std::string& name, const std::string& hexHash);

    /// Register hashes from a simple text manifest (one "name sha256hex" per line).
    /// Lines starting with '#' are treated as comments.
    /// Returns the number of hashes successfully parsed.
    size_t loadManifest(const std::string& manifestPath);

    /// Remove all registered expected hashes (useful in tests).
    void clearRegistry();

    // -------------------------------------------------------------------------
    // Verification
    // -------------------------------------------------------------------------

    /// Compute SHA-256 of @p spvBytes and check against the registered hash
    /// for @p name.
    ///
    /// If no hash is registered for @p name the call succeeds with a warning
    /// in @p result.message (to allow graceful operation without a manifest).
    /// Enable strict mode via setStrictMode(true) to fail on unregistered names.
    VerifyResult verify(const std::string& name,
                        const std::vector<uint32_t>& spvWords) const;

    VerifyResult verify(const std::string& name,
                        const uint8_t* data,
                        size_t byteLen) const;

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    /// Compute the SHA-256 hash of raw bytes and return it as a 64-char hex string.
    static std::string sha256Hex(const uint8_t* data, size_t len);
    static std::string sha256Hex(const std::vector<uint32_t>& spvWords);

    /// Enable/disable strict mode.  In strict mode, verify() returns failure
    /// when no hash is registered for the given shader name.
    void setStrictMode(bool strict);
    bool strictMode() const;

    /// Returns true if an expected hash is registered for @p name.
    bool isRegistered(const std::string& name) const;

private:
    ShaderIntegrityVerifier() = default;
    ~ShaderIntegrityVerifier() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> expectedHashes_; // name → sha256hex
    bool strict_ = false;
};

} // namespace acceleration
} // namespace themis
