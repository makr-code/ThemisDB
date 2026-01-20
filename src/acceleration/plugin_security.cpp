#include "acceleration/plugin_security.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <nlohmann/json.hpp>

namespace themis {
namespace acceleration {

using json = nlohmann::json;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Decode hex string to bytes
 * @param hexStr Hex-encoded string (must have even length)
 * @param outBytes Output vector for decoded bytes
 * @return true if successful, false if invalid format
 */
static bool decodeHexString(const std::string& hexStr, std::vector<uint8_t>& outBytes) {
    // Validate hex string length is even
    if (hexStr.size() % 2 != 0) {
        return false;
    }
    
    outBytes.clear();
    outBytes.reserve(hexStr.size() / 2);
    
    try {
        for (size_t i = 0; i < hexStr.size(); i += 2) {
            std::string byteStr = hexStr.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
            outBytes.push_back(byte);
        }
        return true;
    } catch (...) {
        outBytes.clear();
        return false;
    }
}

// ============================================================================
// PluginSecurityVerifier Implementation
// ============================================================================

PluginSecurityVerifier::PluginSecurityVerifier(const PluginSecurityPolicy& policy)
    : policy_(policy) {
}

std::string PluginSecurityVerifier::calculateFileHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return "";
    }
    
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    const size_t bufferSize = 32768;
    std::vector<char> buffer(bufferSize);
    
    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx, buffer.data(), file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    EVP_MD_CTX_free(mdctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::optional<PluginMetadata> PluginSecurityVerifier::loadMetadata(const std::string& pluginPath) {
    // Look for metadata JSON file (pluginPath + ".json")
    std::string metadataPath = pluginPath + ".json";
    
    if (!std::filesystem::exists(metadataPath)) {
        // Try alternative location: same directory, same name
        std::filesystem::path p(pluginPath);
        metadataPath = (p.parent_path() / (p.stem().string() + ".json")).string();
        
        if (!std::filesystem::exists(metadataPath)) {
            return std::nullopt;
        }
    }
    
    try {
        std::ifstream file(metadataPath);
        json j;
        file >> j;
        
        PluginMetadata metadata;
        
        if (j.contains("plugin")) {
            auto& plugin = j["plugin"];
            metadata.name = plugin.value("name", "");
            metadata.version = plugin.value("version", "");
            metadata.author = plugin.value("author", "");
            metadata.description = plugin.value("description", "");
            metadata.license = plugin.value("license", "");
            
            if (plugin.contains("signature")) {
                auto& sig = plugin["signature"];
                metadata.signature.sha256Hash = sig.value("sha256", "");
                metadata.signature.signature = sig.value("signature", "");
                metadata.signature.signingCertificate = sig.value("certificate", "");
                metadata.signature.issuer = sig.value("issuer", "");
                metadata.signature.subject = sig.value("subject", "");
                metadata.signature.timestamp = sig.value("timestamp", 0ULL);
            }
            
            if (plugin.contains("permissions")) {
                metadata.permissions = plugin["permissions"].get<std::vector<std::string>>();
            }
        }
        
        return metadata;
        
    } catch (const std::exception& e) {
        // Failed to parse metadata
        (void)e; // Suppress unused variable warning
        return std::nullopt;
    }
}

bool PluginSecurityVerifier::isBlacklisted(const std::string& fileHash) const {
    return std::find(policy_.blacklistedHashes.begin(), 
                    policy_.blacklistedHashes.end(), 
                    fileHash) != policy_.blacklistedHashes.end();
}

bool PluginSecurityVerifier::isWhitelisted(const std::string& fileHash) const {
    return std::find(policy_.whitelistedHashes.begin(),
                    policy_.whitelistedHashes.end(),
                    fileHash) != policy_.whitelistedHashes.end();
}

PluginTrustLevel PluginSecurityVerifier::getTrustLevel(const PluginMetadata& metadata) {
    // Check whitelist first
    if (isWhitelisted(metadata.signature.sha256Hash)) {
        return PluginTrustLevel::TRUSTED;
    }
    
    // Check blacklist
    if (isBlacklisted(metadata.signature.sha256Hash)) {
        return PluginTrustLevel::BLOCKED;
    }
    
    // Check if signature is verified
    if (!metadata.signature.verified) {
        return PluginTrustLevel::UNTRUSTED;
    }
    
    // Check if issuer is trusted
    bool trustedIssuer = false;
    for (const auto& trustedIssuerDN : policy_.trustedIssuers) {
        if (metadata.signature.issuer.find(trustedIssuerDN) != std::string::npos) {
            trustedIssuer = true;
            break;
        }
    }
    
    if (!trustedIssuer) {
        return PluginTrustLevel::UNTRUSTED;
    }
    
    return PluginTrustLevel::TRUSTED;
}

bool PluginSecurityVerifier::verifyPlugin(const std::string& pluginPath, std::string& errorMessage) {
    auto& auditor = PluginSecurityAuditor::instance();
    
    // Step 1: Check if file exists
    if (!std::filesystem::exists(pluginPath)) {
        errorMessage = "Plugin file does not exist: " + pluginPath;
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
            pluginPath, "", errorMessage,
            static_cast<uint64_t>(std::time(nullptr)),
            "ERROR"
        });
        return false;
    }
    
    // Step 2: Calculate file hash
    std::string fileHash;
    if (policy_.verifyFileHash) {
        fileHash = calculateFileHash(pluginPath);
        if (fileHash.empty()) {
            errorMessage = "Failed to calculate file hash";
            auditor.logEvent({
                PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED,
                pluginPath, "", errorMessage,
                static_cast<uint64_t>(std::time(nullptr)),
                "ERROR"
            });
            return false;
        }
    }
    
    // Step 3: Check blacklist
    if (isBlacklisted(fileHash)) {
        errorMessage = "Plugin is on blacklist (hash: " + fileHash + ")";
        auditor.logEvent({
            PluginSecurityEvent::EventType::BLACKLISTED,
            pluginPath, fileHash, errorMessage,
            static_cast<uint64_t>(std::time(nullptr)),
            "CRITICAL"
        });
        return false;
    }
    
    // Step 4: Load and verify metadata
    auto metadata = loadMetadata(pluginPath);
    
    if (policy_.requireSignature && !metadata.has_value()) {
        if (!policy_.allowUnsigned) {
            errorMessage = "Plugin metadata not found (signature required)";
            auditor.logEvent({
                PluginSecurityEvent::EventType::SIGNATURE_VERIFICATION_FAILED,
                pluginPath, fileHash, errorMessage,
                static_cast<uint64_t>(std::time(nullptr)),
                "ERROR"
            });
            return false;
        }
    }
    
    // Step 5: Verify hash matches metadata
    if (metadata.has_value() && policy_.verifyFileHash) {
        if (!metadata->signature.sha256Hash.empty() && 
            metadata->signature.sha256Hash != fileHash) {
            errorMessage = "File hash mismatch! Expected: " + metadata->signature.sha256Hash + 
                          ", Got: " + fileHash;
            auditor.logEvent({
                PluginSecurityEvent::EventType::HASH_MISMATCH,
                pluginPath, fileHash, errorMessage,
                static_cast<uint64_t>(std::time(nullptr)),
                "CRITICAL"
            });
            return false;
        }
    }
    
    // Step 6: Verify digital signature (if present)
    if (metadata.has_value() && !metadata->signature.signature.empty()) {
        if (!verifySignature(pluginPath, metadata->signature)) {
            errorMessage = "Digital signature verification failed";
            auditor.logEvent({
                PluginSecurityEvent::EventType::SIGNATURE_VERIFICATION_FAILED,
                pluginPath, fileHash, errorMessage,
                static_cast<uint64_t>(std::time(nullptr)),
                "ERROR"
            });
            
            if (policy_.requireSignature) {
                return false;
            }
        } else {
            // Mark as verified in metadata
            const_cast<PluginMetadata&>(*metadata).signature.verified = true;
            
            auditor.logEvent({
                PluginSecurityEvent::EventType::SIGNATURE_VERIFIED,
                pluginPath, fileHash, "Signature verified successfully",
                static_cast<uint64_t>(std::time(nullptr)),
                "INFO"
            });
        }
    }
    
    // Step 7: Check trust level
    if (metadata.has_value()) {
        auto trustLevel = getTrustLevel(*metadata);
        
        if (trustLevel == PluginTrustLevel::BLOCKED) {
            errorMessage = "Plugin trust level is BLOCKED";
            return false;
        }
        
        if (trustLevel < policy_.minTrustLevel) {
            errorMessage = "Plugin trust level insufficient (required: TRUSTED, got: UNTRUSTED)";
            auditor.logEvent({
                PluginSecurityEvent::EventType::POLICY_VIOLATION,
                pluginPath, fileHash, errorMessage,
                static_cast<uint64_t>(std::time(nullptr)),
                "WARNING"
            });
            return false;
        }
    }
    
    // Step 8: Whitelist check (bypass other checks)
    if (isWhitelisted(fileHash)) {
        auditor.logEvent({
            PluginSecurityEvent::EventType::PLUGIN_LOADED,
            pluginPath, fileHash, "Plugin loaded (whitelisted)",
            static_cast<uint64_t>(std::time(nullptr)),
            "INFO"
        });
        return true;
    }
    
    // All checks passed
    auditor.logEvent({
        PluginSecurityEvent::EventType::PLUGIN_LOADED,
        pluginPath, fileHash, "Plugin loaded successfully",
        static_cast<uint64_t>(std::time(nullptr)),
        "INFO"
    });
    
    return true;
}

bool PluginSecurityVerifier::verifySignature(const std::string& filePath, 
                                             const PluginSignature& signature) {
    if (signature.signature.empty() || signature.signingCertificate.empty()) {
        return false;
    }
    
    // Step 1: Calculate file hash
    std::string fileHash = calculateFileHash(filePath);
    if (fileHash.empty()) {
        return false;
    }
    
    // Step 2: Load X.509 certificate from PEM string
    BIO* bio = BIO_new_mem_buf(signature.signingCertificate.data(), 
                                static_cast<int>(signature.signingCertificate.size()));
    if (!bio) {
        return false;
    }
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) {
        return false;
    }
    
    // Step 3: Check certificate expiration
    int notBefore = X509_cmp_current_time(X509_get0_notBefore(cert));
    int notAfter = X509_cmp_current_time(X509_get0_notAfter(cert));
    
    if (notBefore >= 0 || notAfter <= 0) {
        // Certificate is not yet valid or has expired
        X509_free(cert);
        return false;
    }
    
    // Step 4: Extract public key from certificate
    EVP_PKEY* pubKey = X509_get_pubkey(cert);
    if (!pubKey) {
        X509_free(cert);
        return false;
    }
    
    // Step 5: Decode signature from hex
    std::vector<uint8_t> sigBytes;
    if (!decodeHexString(signature.signature, sigBytes)) {
        EVP_PKEY_free(pubKey);
        X509_free(cert);
        return false;
    }
    
    // Step 6: Verify signature using public key
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        EVP_PKEY_free(pubKey);
        X509_free(cert);
        return false;
    }
    
    bool verified = false;
    
    // Initialize verification context
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pubKey) == 1) {
        // Convert file hash from hex string to bytes
        std::vector<uint8_t> hashBytes;
        if (decodeHexString(fileHash, hashBytes)) {
            // Verify signature
            int result = EVP_DigestVerify(mdctx, sigBytes.data(), sigBytes.size(),
                                          hashBytes.data(), hashBytes.size());
            verified = (result == 1);
        }
    }
    
    // Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pubKey);
    X509_free(cert);
    
    return verified;
}

void PluginSecurityVerifier::updatePolicy(const PluginSecurityPolicy& policy) {
    policy_ = policy;
}

// ============================================================================
// PluginSecurityAuditor Implementation
// ============================================================================

PluginSecurityAuditor& PluginSecurityAuditor::instance() {
    static PluginSecurityAuditor instance;
    return instance;
}

void PluginSecurityAuditor::logEvent(const PluginSecurityEvent& event) {
    events_.push_back(event);
    
    // Also log to system logger (spdlog integration)
    // TODO: Integration with existing audit_logger.h
}

std::vector<PluginSecurityEvent> PluginSecurityAuditor::getEventsForPlugin(
    const std::string& pluginPath) const {
    
    std::vector<PluginSecurityEvent> result;
    for (const auto& event : events_) {
        if (event.pluginPath == pluginPath) {
            result.push_back(event);
        }
    }
    return result;
}

void PluginSecurityAuditor::clearEvents() {
    events_.clear();
}

bool PluginSecurityAuditor::exportEvents(const std::string& outputPath) const {
    try {
        json j;
        j["events"] = json::array();
        
        for (const auto& event : events_) {
            json eventJson;
            eventJson["type"] = static_cast<int>(event.type);
            eventJson["pluginPath"] = event.pluginPath;
            eventJson["pluginHash"] = event.pluginHash;
            eventJson["message"] = event.message;
            eventJson["timestamp"] = event.timestamp;
            eventJson["severity"] = event.severity;
            
            j["events"].push_back(eventJson);
        }
        
        std::ofstream file(outputPath);
        file << j.dump(2);
        
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

// ============================================================================
// EnhancedPluginSecurityVerifier Implementation
// ============================================================================

EnhancedPluginSecurityVerifier::EnhancedPluginSecurityVerifier(const PluginSecurityPolicy& policy)
    : policy_(policy) {
}

EnhancedPluginSecurityVerifier::VerificationResult 
EnhancedPluginSecurityVerifier::verifyPlugin(
    const std::string& plugin_path,
    VerificationLevel required_level
) {
    VerificationResult result;
    result.level_achieved = VerificationLevel::LEVEL_1_HASH_ONLY;
    
    // Level 1: Hash verification
    if (!verifyHash(plugin_path, result)) {
        result.passed = false;
        return result;
    }
    result.level_achieved = VerificationLevel::LEVEL_1_HASH_ONLY;
    
    // If only hash is required, we're done
    if (required_level == VerificationLevel::LEVEL_1_HASH_ONLY) {
        result.passed = true;
        return result;
    }
    
    // Level 2: Embedded signature verification
    if (verifyEmbeddedSignature(plugin_path, result)) {
        result.level_achieved = VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE;
        
        // If level 2 is sufficient, we're done
        if (required_level == VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE) {
            result.passed = true;
            return result;
        }
    } else {
        // If embedded signature required but failed
        if (required_level >= VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE) {
            result.passed = false;
            return result;
        }
    }
    
    // Level 3: Platform signature verification
    if (verifyPlatformSignature(plugin_path, result)) {
        result.level_achieved = VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE;
        
        // If level 3 is sufficient, we're done
        if (required_level == VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE) {
            result.passed = true;
            return result;
        }
    } else {
        // If platform signature required but failed
        if (required_level >= VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE) {
            // Only fail if we require platform signature AND it's not development mode
            if (!policy_.allowUnsigned) {
                result.passed = false;
                return result;
            }
        }
    }
    
    // Level 4: Full chain verification
    if (required_level == VerificationLevel::LEVEL_4_FULL_CHAIN) {
        if (verifyFullChain(plugin_path, result)) {
            result.level_achieved = VerificationLevel::LEVEL_4_FULL_CHAIN;
            result.passed = true;
        } else {
            result.passed = false;
        }
        return result;
    }
    
    // If we got here, verification passed at the required level
    result.passed = true;
    return result;
}

bool EnhancedPluginSecurityVerifier::verifyHash(
    const std::string& plugin_path,
    VerificationResult& result
) {
    PluginSecurityVerifier basic_verifier(policy_);
    std::string fileHash = basic_verifier.calculateFileHash(plugin_path);
    
    if (fileHash.empty()) {
        result.error_message = "Failed to calculate file hash";
        return false;
    }
    
    result.hash_verified = true;
    return true;
}

bool EnhancedPluginSecurityVerifier::verifyEmbeddedSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
    // Try to extract embedded certificate from DLL/SO
    auto cert_data = extractEmbeddedCertificate(plugin_path);
    if (!cert_data) {
        result.error_message = "No embedded certificate found";
        return false;
    }
    
    // Parse certificate
    const unsigned char* p = cert_data->data();
    X509* cert = d2i_X509(nullptr, &p, static_cast<long>(cert_data->size()));
    if (!cert) {
        result.error_message = "Failed to parse embedded certificate";
        return false;
    }
    
    // Verify certificate is official ThemisDB certificate
    if (!isOfficialThemisDBCertificate(cert)) {
        result.error_message = "Certificate is not official ThemisDB certificate";
        result.issuer = getCertificateIssuer(cert);
        X509_free(cert);
        return false;
    }
    
    // Check certificate validity
    if (!isCertificateValid(cert)) {
        result.error_message = "Certificate has expired or not yet valid";
        X509_free(cert);
        return false;
    }
    
    // Extract embedded signature
    auto signature_data = extractEmbeddedSignature(plugin_path);
    if (!signature_data) {
        result.error_message = "No embedded signature found";
        X509_free(cert);
        return false;
    }
    
    // Calculate DLL hash (excluding signature section)
    std::vector<uint8_t> dll_hash = calculateHashExcludingSignature(plugin_path);
    
    // Verify signature with certificate public key
    EVP_PKEY* pubkey = X509_get_pubkey(cert);
    bool signature_valid = verifyRSASignature(dll_hash, *signature_data, pubkey);
    EVP_PKEY_free(pubkey);
    
    // Get certificate info
    result.issuer = getCertificateIssuer(cert);
    result.subject = getCertificateSubject(cert);
    result.is_themisdb_official = true;
    
    X509_free(cert);
    
    if (!signature_valid) {
        result.error_message = "Embedded signature verification failed";
        return false;
    }
    
    result.embedded_signature_verified = true;
    return true;
}

bool EnhancedPluginSecurityVerifier::verifyPlatformSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
#ifdef _WIN32
    return verifyAuthenticodeSignature(plugin_path, result);
#elif defined(__APPLE__)
    return verifyMacOSCodeSignature(plugin_path, result);
#else
    return verifyGPGSignature(plugin_path, result);
#endif
}

bool EnhancedPluginSecurityVerifier::verifyFullChain(
    const std::string& plugin_path,
    VerificationResult& result
) {
    // First verify embedded signature
    if (!verifyEmbeddedSignature(plugin_path, result)) {
        return false;
    }
    
    // Then verify platform signature
    if (!verifyPlatformSignature(plugin_path, result)) {
        // Platform signature is optional for full chain in development mode
        if (!policy_.allowUnsigned) {
            return false;
        }
    }
    
    // TODO: Implement full certificate chain validation
    // TODO: Implement CRL/OCSP checking
    result.certificate_chain_verified = true;
    result.certificate_not_revoked = true;
    
    return true;
}

std::optional<std::vector<uint8_t>> 
EnhancedPluginSecurityVerifier::extractEmbeddedCertificate(
    const std::string& plugin_path
) {
    // TODO: Implement PE/ELF/Mach-O parsing to extract embedded certificate
    // For now, return empty to indicate no embedded certificate found
    (void)plugin_path;  // Suppress unused parameter warning
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> 
EnhancedPluginSecurityVerifier::extractEmbeddedSignature(
    const std::string& plugin_path
) {
    // TODO: Implement PE/ELF/Mach-O parsing to extract embedded signature
    // For now, return empty to indicate no embedded signature found
    (void)plugin_path;  // Suppress unused parameter warning
    return std::nullopt;
}

bool EnhancedPluginSecurityVerifier::isOfficialThemisDBCertificate(X509* cert) {
    if (!cert) {
        return false;
    }
    
    std::string issuer = getCertificateIssuer(cert);
    
    // Check if issuer matches ThemisDB Official Plugins CA
    return issuer.find("ThemisDB Official Plugins CA") != std::string::npos ||
           issuer.find("ThemisDB.org") != std::string::npos;
}

#ifdef _WIN32
bool EnhancedPluginSecurityVerifier::verifyAuthenticodeSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
    // TODO: Implement Windows Authenticode verification using WinVerifyTrust API
    // For now, mark as not verified
    (void)plugin_path;
    result.error_message = "Authenticode verification not yet implemented";
    return false;
}
#elif defined(__APPLE__)
bool EnhancedPluginSecurityVerifier::verifyMacOSCodeSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
    // TODO: Implement macOS codesign verification
    // For now, mark as not verified
    (void)plugin_path;
    result.error_message = "macOS codesign verification not yet implemented";
    return false;
}
#else
bool EnhancedPluginSecurityVerifier::verifyGPGSignature(
    const std::string& plugin_path,
    VerificationResult& result
) {
    // TODO: Implement GPG signature verification for Linux
    // Look for .sig or .asc file alongside the plugin
    (void)plugin_path;
    result.error_message = "GPG signature verification not yet implemented";
    return false;
}
#endif

std::vector<uint8_t> 
EnhancedPluginSecurityVerifier::calculateHashExcludingSignature(
    const std::string& plugin_path
) {
    // For now, just calculate hash of entire file
    // TODO: Implement PE/ELF parsing to exclude signature section
    std::ifstream file(plugin_path, std::ios::binary);
    if (!file) {
        return {};
    }
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return {};
    }
    
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    
    const size_t bufferSize = 32768;
    std::vector<char> buffer(bufferSize);
    
    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx, buffer.data(), file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return {};
        }
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    EVP_MD_CTX_free(mdctx);
    
    return std::vector<uint8_t>(hash, hash + hashLen);
}

bool EnhancedPluginSecurityVerifier::verifyRSASignature(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    EVP_PKEY* pubkey
) {
    if (!pubkey || data.empty() || signature.empty()) {
        return false;
    }
    
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return false;
    }
    
    bool verified = false;
    
    // Initialize verification context
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pubkey) == 1) {
        // Verify signature
        int result = EVP_DigestVerify(mdctx, signature.data(), signature.size(),
                                      data.data(), data.size());
        verified = (result == 1);
    }
    
    EVP_MD_CTX_free(mdctx);
    return verified;
}

std::string EnhancedPluginSecurityVerifier::getCertificateIssuer(X509* cert) {
    if (!cert) {
        return "";
    }
    
    X509_NAME* issuer_name = X509_get_issuer_name(cert);
    if (!issuer_name) {
        return "";
    }
    
    char* issuer_str = X509_NAME_oneline(issuer_name, nullptr, 0);
    if (!issuer_str) {
        return "";
    }
    
    std::string result(issuer_str);
    OPENSSL_free(issuer_str);
    return result;
}

std::string EnhancedPluginSecurityVerifier::getCertificateSubject(X509* cert) {
    if (!cert) {
        return "";
    }
    
    X509_NAME* subject_name = X509_get_subject_name(cert);
    if (!subject_name) {
        return "";
    }
    
    char* subject_str = X509_NAME_oneline(subject_name, nullptr, 0);
    if (!subject_str) {
        return "";
    }
    
    std::string result(subject_str);
    OPENSSL_free(subject_str);
    return result;
}

bool EnhancedPluginSecurityVerifier::isCertificateValid(X509* cert) {
    if (!cert) {
        return false;
    }
    
    // Check certificate has not expired and is already valid
    int notBefore = X509_cmp_current_time(X509_get0_notBefore(cert));
    int notAfter = X509_cmp_current_time(X509_get0_notAfter(cert));
    
    // notBefore should be < 0 (in the past)
    // notAfter should be > 0 (in the future)
    return (notBefore < 0 && notAfter > 0);
}

void EnhancedPluginSecurityVerifier::updatePolicy(const PluginSecurityPolicy& policy) {
    policy_ = policy;
}

} // namespace acceleration
} // namespace themis
