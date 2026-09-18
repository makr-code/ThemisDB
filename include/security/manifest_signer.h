/**
 * @file manifest_signer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ManifestSigner {
public:
    struct Config {
        std::string key_id;              // Key ID for signing/verification
        std::string algorithm = "RSA-4096-SHA256";
        bool verify_on_load = true;      // Verify signature when loading
    };
    
    ManifestSigner(
        std::shared_ptr<SigningService> signing_service,
        const Config& config
    );
    
    BinaryManifest generateManifest(
        const std::string& root_path,
        const std::string& version,
        const std::string& build_id,
        const std::vector<std::string>& include_patterns = {"*"}
    );
    
    /**
     * @brief Sign Manifest.
     * @param[in] manifest Input parameter.
     * @return Return value.
     */
    SignedManifest signManifest(const BinaryManifest& manifest);
    
    /**
     * @brief Verify Signature.
     * @param[in] signed_manifest Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignature(const SignedManifest& signed_manifest);
    
    struct VerificationResult {
        bool signature_valid = false;
        bool files_valid = false;
        std::vector<std::string> missing_files;
        std::vector<std::string> modified_files;
        std::string error_message;
    };
    
    /**
     * @brief Verify Binaries.
     * @param[in] signed_manifest Input parameter.
     * @param[in] root_path Path to the root.
     * @return Return value.
     */
    VerificationResult verifyBinaries(
        const SignedManifest& signed_manifest,
        const std::string& root_path
    );
    
    /**
     * @brief Compute File SHA256.
     * @param[in] file_path Path to the file.
     * @return Return value.
     */
    static std::string computeFileSHA256(const std::string& file_path);

private:
    std::shared_ptr<SigningService> signing_service_;
    Config config_;
    mutable std::mutex mtx_;
    
    /**
     * @brief Matches Pattern.
     * @param[in] filename Input parameter.
     * @param[in] pattern Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchesPattern(const std::string& filename, const std::string& pattern);
};

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
     * @brief Verify identity and enforce network policies for a request.
     * @return Verification result.
     */
    bool verify();
    
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
