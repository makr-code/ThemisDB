/**
 * @file module_signature_verifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Authenticode (Windows) and GPG (Linux) signature verification for ThemisDB modules.
// Provides a standalone, platform-specific signature checker that complements the
// hash-based ModuleHashVerifier (Phase 3) with cryptographic signature validation.
//
// Roadmap item: Phase 4 – Authenticode (Windows) and GPG (Linux) signature verification
// Issue: #2473

#pragma once

#include <string>
#include "themis/export.h"

namespace themis {
namespace modules {

/**
 * @brief Result of a single module signature verification attempt.
 */
struct ModuleSignatureVerificationResult {
    bool        success      = false;
    std::string signerInfo;     ///< Certificate CN (Windows) or GPG uid (Linux)
    std::string platform;       ///< "windows_authenticode" | "linux_gpg"
    std::string errorMessage;
};

/**
 * @brief Standalone platform-specific signature verifier for ThemisDB modules.
 *
 * On Windows this uses the WinVerifyTrust / Wintrust API to validate the
 * embedded Authenticode (PE) digital signature of a module DLL.
 *
 * On Linux this uses posix_spawn to invoke `gpg --verify` against a detached
 * signature file (.asc, .sig, or .gpg) without going through a shell, avoiding
 * the injection risk of popen().
 *
 * A cross-platform verifySignature() dispatcher is always available; the
 * platform-specific helpers are conditionally compiled.
 *
 * Thread-safety: all methods are stateless and thread-safe.
 */
class THEMIS_BASE_API ModuleSignatureVerifier {
public:
    /**
     * @brief Verify the digital signature of a module file.
     *
     * Dispatches to verifyAuthenticodeSignature() on Windows or
     * verifyGPGSignature() on Linux.
     *
     * @param modulePath   Absolute path to the module file (.dll / .so).
     * @param signaturePath Path to detached signature (Linux only; ignored on
     *                     Windows).  Pass an empty string to auto-detect.
     * @return Verification result with success flag, signer info, and any error.
     */
    static ModuleSignatureVerificationResult verifySignature(
        const std::string& modulePath,
        const std::string& signaturePath = "");

#ifdef _WIN32
    /**
     * @brief Verify the Authenticode (PE) signature embedded in a Windows DLL.
     *
     * Uses WinVerifyTrust with the generic verify action.  When verification
     * succeeds the subject CN of the end-entity certificate is returned via
     * @p signerInfo.
     *
     * @param modulePath  Path to the .dll file.
     * @param signerInfo  Output: certificate subject CN on success, empty on failure.
     * @return true if the Authenticode signature is valid and trusted.
     */
    static bool verifyAuthenticodeSignature(const std::string& modulePath,
                                            std::string& signerInfo);
#endif

#ifdef __linux__
    /**
     * @brief Verify a detached GPG signature for a Linux shared-object module.
     *
     * Spawns `gpg --verify <sigFile> <modulePath>` via posix_spawn (no shell).
     * The signature file is auto-detected by appending .asc, .sig, or .gpg
     * to @p modulePath if @p signaturePath is empty.
     *
     * @param modulePath    Absolute path to the .so file.
     * @param signaturePath Path to the detached .asc / .sig file, or empty for
     *                      auto-detection.
     * @param signerInfo    Output: GPG uid extracted from "Good signature from"
     *                      line, empty if not found.
     * @return true if gpg exits 0 and reports "Good signature".
     */
    static bool verifyGPGSignature(const std::string& modulePath,
                                   const std::string& signaturePath,
                                   std::string& signerInfo);
#endif
};

} // namespace modules
} // namespace themis
