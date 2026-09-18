/**
 * @file shader_integrity.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    /**
     * @brief Instance.
     * @return Return value.
     */
    static ShaderIntegrityVerifier& instance();

    // Non-copyable, non-movable
    ShaderIntegrityVerifier(const ShaderIntegrityVerifier&) = delete;
    ShaderIntegrityVerifier& operator=(const ShaderIntegrityVerifier&) = delete;

    // -------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------

    /**
     * @brief Register Expected Hash.
     * @param[in] name Input parameter.
     * @param[in] hexHash Input parameter.
     */
    void registerExpectedHash(const std::string& name, const std::string& hexHash);

    /**
     * @brief Load Manifest.
     * @param[in] manifestPath Input parameter.
     * @return Return value.
     */
    size_t loadManifest(const std::string& manifestPath);

    /**
     * @brief Clear Registry.
     */
    void clearRegistry();

    // -------------------------------------------------------------------------
    // Verification
    // -------------------------------------------------------------------------

    /**
     * @brief Verify identity and enforce network policies for a request.
     * @param[in] name Input parameter.
     * @param[in] spvWords Input parameter.
     * @return Verification result.
     */
    VerifyResult verify(const std::string& name,
                        const std::vector<uint32_t>& spvWords) const;

    /**
     * @brief Verify identity and enforce network policies for a request.
     * @param[in] name Input parameter.
     * @param[in] data Input parameter.
     * @param[in] byteLen Input parameter.
     * @return Verification result.
     */
    VerifyResult verify(const std::string& name,
                        const uint8_t* data,
                        size_t byteLen) const;

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    /**
     * @brief Sha256 Hex.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
     */
    static std::string sha256Hex(const uint8_t* data, size_t len);
    /**
     * @brief Sha256 Hex.
     * @param[in] spvWords Input parameter.
     * @return Return value.
     */
    static std::string sha256Hex(const std::vector<uint32_t>& spvWords);

    /**
     * @brief Set Strict Mode.
     * @param[in] strict Input parameter.
     */
    void setStrictMode(bool strict);
    /**
     * @brief Strict Mode.
     * @return True when the operation succeeds.
     */
    bool strictMode() const;

    /**
     * @brief Is Registered.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
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
