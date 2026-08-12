/**
 * @file manifest_signer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/binary_manifest.h"
#include "security/signing.h"
#include <memory>
#include <string>
#include <filesystem>
#include <mutex>

namespace themis {
namespace security {

/**
 * @brief Utility for signing and verifying binary manifests
 * 
 * Implements RSA-4096 signing for release manifest integrity verification.
 * Provides tools for:
 * - Generating manifests from directory trees
 * - Signing manifests with RSA-4096
 * - Verifying manifest signatures
 * - Validating binary file integrity
 * 
 * Security best practices:
 * - Private keys stored in HSM or secure key store
 * - SHA-256 for file hashing
 * - RSA-4096 for digital signatures
 * - Canonical JSON for deterministic signing
 * 
 * Compliance: NIST SP 800-218 (SSDF), SOC 2 CC7.1
 */
class ManifestSigner {
public:
    /**
     * @brief Configuration for manifest signing
     */
    struct Config {
        std::string key_id;              // Key ID for signing/verification
        std::string algorithm = "RSA-4096-SHA256";
        bool verify_on_load = true;      // Verify signature when loading
    };
    
    /**
     * @brief Construct manifest signer
     * 
     * @param signing_service Signing service for RSA operations
     * @param config Configuration
     */
    ManifestSigner(
        std::shared_ptr<SigningService> signing_service,
        const Config& config
    );
    
    /**
     * @brief Generate manifest from directory tree
     * 
     * Recursively scans directory and computes SHA-256 hashes for all files.
     * 
     * @param root_path Root directory path
     * @param version ThemisDB version
     * @param build_id Build identifier (e.g., git commit)
     * @param include_patterns Glob patterns for files to include (e.g., "*.exe", "*.so")
     * @return BinaryManifest with all files
     */
    BinaryManifest generateManifest(
        const std::string& root_path,
        const std::string& version,
        const std::string& build_id,
        const std::vector<std::string>& include_patterns = {"*"}
    );
    
    /**
     * @brief Sign manifest with RSA-4096
     * 
     * @param manifest Manifest to sign
     * @return Signed manifest with signature
     */
    SignedManifest signManifest(const BinaryManifest& manifest);
    
    /**
     * @brief Verify manifest signature
     * 
     * @param signed_manifest Signed manifest to verify
     * @return true if signature is valid
     */
    bool verifySignature(const SignedManifest& signed_manifest);
    
    /**
     * @brief Verify binary files against manifest
     * 
     * Checks that all files in manifest exist and have correct SHA-256 hashes.
     * 
     * @param signed_manifest Signed manifest
     * @param root_path Root directory for binary files
     * @return Verification result with details
     */
    struct VerificationResult {
        bool signature_valid = false;
        bool files_valid = false;
        std::vector<std::string> missing_files;
        std::vector<std::string> modified_files;
        std::string error_message;
    };
    
    VerificationResult verifyBinaries(
        const SignedManifest& signed_manifest,
        const std::string& root_path
    );
    
    /**
     * @brief Compute SHA-256 hash of file
     * 
     * @param file_path Path to file
     * @return SHA-256 hash (hex string)
     */
    static std::string computeFileSHA256(const std::string& file_path);

private:
    std::shared_ptr<SigningService> signing_service_;
    Config config_;
    mutable std::mutex mtx_;
    
    bool matchesPattern(const std::string& filename, const std::string& pattern);
};

/**
 * @brief Startup verification utility
 * 
 * Verifies binary integrity on application startup to detect tampering.
 */
class StartupVerifier {
public:
    struct Config {
        std::string manifest_path;       // Path to release manifest
        std::string binaries_root;       // Root directory for binaries
        bool fail_on_invalid = true;     // Exit if verification fails
        bool log_results = true;         // Log verification results
    };
    
    StartupVerifier(
        std::shared_ptr<SigningService> signing_service,
        const Config& config
    );
    
    /**
     * @brief Verify binaries on startup
     * 
     * @return true if all binaries are valid
     */
    bool verify();
    
    /**
     * @brief Get verification result
     * 
     * @return Detailed verification result
     */
    const ManifestSigner::VerificationResult& getResult() const {
        return result_;
    }

private:
    std::shared_ptr<SigningService> signing_service_;
    Config config_;
    ManifestSigner::VerificationResult result_;
};

} // namespace security
} // namespace themis
