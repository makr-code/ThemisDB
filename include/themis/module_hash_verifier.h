/**
 * @file module_hash_verifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SHA-256 hash verification for loaded ThemisDB modules.
// Provides standalone hash computation and manifest-based expected-hash
// validation, complementing the signature-centric PluginSecurityVerifier.
//
// Roadmap item: Phase 3 – SHA-256 hash verification for loaded modules
// Issue: #2471

#pragma once

#include <map>
#include <optional>
#include <string>
#include "themis/export.h"

namespace themis {
namespace modules {

/**
 * @brief Result of a single module hash verification attempt.
 */
struct ModuleHashVerificationResult {
    bool        success      = false;
    std::string computedHash;   ///< Actual SHA-256 of the module file on disk
    std::string expectedHash;   ///< Hash from the manifest (may be empty)
    std::string errorMessage;
};

/**
 * @brief Standalone SHA-256 integrity verifier for ThemisDB modules.
 *
 * Two usage patterns are supported:
 *
 *   1. **Ad-hoc** – call the static helpers to hash or compare a single file
 *      without constructing an instance.
 *
 *   2. **Manifest-driven** – load a JSON manifest that maps module names to
 *      their expected SHA-256 hashes and verify each module against it.
 *      The manifest format is a flat JSON object:
 *      @code
 *        {
 *          "themis_storage":  "e3b0c44298fc1c149…",
 *          "themis_query":    "a87ff679a2f3e71d9…"
 *        }
 *      @endcode
 *
 * Thread-safety: the static helpers are thread-safe.  The manifest methods
 * are NOT thread-safe; callers must synchronise external access if needed.
 */
class THEMIS_BASE_API ModuleHashVerifier {
public:
    // ---- static helpers (no manifest required) -------------------------

    /**
     * @brief Compute the SHA-256 hash of any file.
     *
     * @param filePath Absolute path to the file.
     * @return Lowercase hex-encoded 64-character SHA-256 digest, or an empty
     *         string on I/O or OpenSSL error.
     */
    static std::string computeSHA256(const std::string& filePath);

    /**
     * @brief Verify a module file against a known-good hash.
     *
     * @param modulePath   Absolute path to the module file.
     * @param expectedHash Lowercase hex-encoded expected SHA-256 digest.
     * @return true if the computed hash matches @p expectedHash exactly.
     */
    static bool verifyHash(const std::string& modulePath,
                           const std::string& expectedHash);

    // ---- manifest-driven API -------------------------------------------

    /**
     * @brief Load (or replace) the in-memory hash manifest from a JSON file.
     *
     * The file must contain a JSON object whose keys are module names and
     * whose values are hex-encoded SHA-256 hash strings.
     *
     * @param manifestPath Path to the JSON manifest file.
     * @return true on success, false on I/O or parse error.
     */
    bool loadManifest(const std::string& manifestPath);

    /**
     * @brief Save the current in-memory manifest to a JSON file.
     *
     * @param outputPath Destination file (created or overwritten).
     * @return true on success, false on I/O error.
     */
    bool saveManifest(const std::string& outputPath) const;

    /**
     * @brief Add or update an expected hash in the in-memory manifest.
     *
     * @param moduleName  Logical module name (e.g., "themis_storage").
     * @param expectedHash Lowercase hex-encoded SHA-256 hash.
     */
    void addExpectedHash(const std::string& moduleName,
                         const std::string& expectedHash);

    /**
     * @brief Verify a module file against the entry in the loaded manifest.
     *
     * Fails with an error message if @p moduleName is absent from the
     * manifest or if the hash cannot be computed.
     *
     * @param moduleName Module name (key in the manifest).
     * @param modulePath Absolute path to the module file to check.
     * @return Verification result with success flag, hashes, and any error.
     */
    ModuleHashVerificationResult verifyModule(const std::string& moduleName,
                                              const std::string& modulePath) const;

    /**
     * @brief Look up the expected hash for @p moduleName in the manifest.
     *
     * Does NOT open any file; queries only the in-memory manifest loaded via
     * loadManifest() or populated via addExpectedHash().  Intended for use by
     * callers (e.g. ModuleLoader) that have already computed the file hash and
     * want to avoid hashing the file a second time.
     *
     * @param moduleName Module name (key in the manifest).
     * @return The expected hex-encoded SHA-256 string, or an empty optional if
     *         @p moduleName is not in the manifest.
     */
    std::optional<std::string> getExpectedHash(const std::string& moduleName) const;

    /**
     * @brief Number of entries currently in the in-memory manifest.
     */
    size_t manifestSize() const;

    /**
     * @brief Remove all entries from the in-memory manifest.
     */
    void clearManifest();

private:
    /// module-name → expected SHA-256 (hex, lowercase)
    std::map<std::string, std::string> manifest_;
};

} // namespace modules
} // namespace themis
