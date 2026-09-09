/**
 * @file jwks_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/jwks_validator.h"
#include "utils/logger.h"
#include <set>
#include <algorithm>

namespace themis {
namespace auth {

JWKSValidator::JWKSValidator(const Config& config)
    : config_(config)
{
    utils::Logger::info("JWKS Validator initialized:");
    utils::Logger::info("  Strict mode: {}", config_.strict_mode);
    utils::Logger::info("  Max keys: {}", config_.max_keys);
    utils::Logger::info("  Min RSA key size: {} bits", config_.min_rsa_key_size);
}

JWKSValidator::ValidationResult JWKSValidator::validate(const nlohmann::json& jwks) const {
    ValidationResult result;
    result.valid = true;  // Assume valid until proven otherwise
    
    // Validate overall structure
    if (!validateStructure(jwks, result)) {
        result.valid = false;
        return result;
    }
    
    // Check for duplicate kids
    if (!checkDuplicateKids(jwks, result)) {
        result.warnings.push_back("Duplicate key IDs found");
        if (config_.strict_mode) {
            result.valid = false;
        }
    }
    
    // Validate each key
    const auto& keys = jwks["keys"];
    result.key_count = keys.size();
    
    for (size_t i = 0; i < keys.size(); i++) {
        if (!validateKey(keys[i], i, result)) {
            result.valid = false;
        }
    }
    
    // In strict mode, warnings also invalidate
    if (config_.strict_mode && !result.warnings.empty()) {
        result.valid = false;
    }
    
    if (result.valid) {
        utils::Logger::info("JWKS validation passed ({} keys)", result.key_count);
    } else {
        utils::Logger::warn("JWKS validation failed: {}", result.getErrorSummary());
    }
    
    return result;
}

void JWKSValidator::validateOrThrow(const nlohmann::json& jwks) const {
    auto result = validate(jwks);
    if (!result.valid) {
        throw std::runtime_error("JWKS validation failed:\n" + result.getErrorSummary());
    }
}

bool JWKSValidator::validateStructure(const nlohmann::json& jwks, ValidationResult& result) const {
    // Must be an object
    if (!jwks.is_object()) {
        result.errors.push_back("JWKS must be a JSON object");
        return false;
    }
    
    // Must contain "keys" array
    if (!jwks.contains("keys")) {
        result.errors.push_back("JWKS missing 'keys' field");
        return false;
    }
    
    if (!jwks["keys"].is_array()) {
        result.errors.push_back("'keys' must be an array");
        return false;
    }
    
    const auto& keys = jwks["keys"];
    
    // Check not empty
    if (keys.empty()) {
        result.warnings.push_back("JWKS contains no keys");
    }
    
    // Check size limit
    if (config_.max_keys > 0 && keys.size() > static_cast<std::size_t>(config_.max_keys)) {
        result.errors.push_back("JWKS contains too many keys (" + 
                               std::to_string(keys.size()) + " > " + 
                               std::to_string(config_.max_keys) + ")");
        return false;
    }
    
    return true;
}

bool JWKSValidator::validateKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const {
    bool valid = true;
    std::string key_prefix = "Key[" + std::to_string(index) + "]: ";
    
    // Must be an object
    if (!jwk.is_object()) {
        result.errors.push_back(key_prefix + "Not a JSON object");
        return false;
    }
    
    // Required: kty (key type)
    if (!jwk.contains("kty")) {
        result.errors.push_back(key_prefix + "Missing 'kty' (key type)");
        return false;
    }
    
    std::string kty = jwk["kty"].get<std::string>();
    
    // Check allowed key types
    if (std::find(config_.allowed_key_types.begin(), 
                  config_.allowed_key_types.end(), 
                  kty) == config_.allowed_key_types.end()) {
        result.errors.push_back(key_prefix + "Unsupported key type: " + kty);
        return false;
    }
    
    // Optional but recommended: kid (key ID)
    if (config_.require_kid && !jwk.contains("kid")) {
        result.errors.push_back(key_prefix + "Missing 'kid' (key ID)");
        valid = false;
    }
    
    // Optional: use (public key use)
    if (config_.require_use && !jwk.contains("use")) {
        result.warnings.push_back(key_prefix + "Missing 'use' field");
    }
    
    if (jwk.contains("use")) {
        std::string use = jwk["use"].get<std::string>();
        if (use != "sig" && use != "enc") {
            result.warnings.push_back(key_prefix + "Unknown 'use' value: " + use);
        }
    }
    
    // Optional: alg (algorithm)
    if (jwk.contains("alg")) {
        std::string alg = jwk["alg"].get<std::string>();
        if (std::find(config_.allowed_algorithms.begin(),
                     config_.allowed_algorithms.end(),
                     alg) == config_.allowed_algorithms.end()) {
            result.warnings.push_back(key_prefix + "Unsupported algorithm: " + alg);
        }
    }
    
    // Validate based on key type
    if (kty == "RSA") {
        if (!validateRSAKey(jwk, index, result)) {
            valid = false;
        }
    } else if (kty == "EC") {
        if (!validateECKey(jwk, index, result)) {
            valid = false;
        }
    } else if (kty == "oct") {
        if (!validateSymmetricKey(jwk, index, result)) {
            valid = false;
        }
    }
    
    return valid;
}

bool JWKSValidator::validateRSAKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const {
    bool valid = true;
    std::string key_prefix = "RSA Key[" + std::to_string(index) + "]: ";
    
    // Required: n (modulus)
    if (!jwk.contains("n")) {
        result.errors.push_back(key_prefix + "Missing 'n' (modulus)");
        valid = false;
    }
    
    // Required: e (exponent)
    if (!jwk.contains("e")) {
        result.errors.push_back(key_prefix + "Missing 'e' (exponent)");
        valid = false;
    }
    
    // Check modulus size (rough estimation from base64url length)
    if (jwk.contains("n")) {
        std::string n = jwk["n"].get<std::string>();
        // Base64url: 4 chars = 3 bytes, so length * 3/4 * 8 = bits
        size_t estimated_bits = (n.length() * 3 / 4) * 8;
        
        if (estimated_bits < config_.min_rsa_key_size) {
            result.errors.push_back(key_prefix + "RSA key size too small (" + 
                                   std::to_string(estimated_bits) + " < " + 
                                   std::to_string(config_.min_rsa_key_size) + " bits)");
            valid = false;
        }
    }
    
    // Private key components should NOT be present in JWKS
    if (jwk.contains("d") || jwk.contains("p") || jwk.contains("q")) {
        result.errors.push_back(key_prefix + "Contains private key material (security violation)");
        valid = false;
    }
    
    return valid;
}

bool JWKSValidator::validateECKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const {
    bool valid = true;
    std::string key_prefix = "EC Key[" + std::to_string(index) + "]: ";
    
    // Required: crv (curve)
    if (!jwk.contains("crv")) {
        result.errors.push_back(key_prefix + "Missing 'crv' (curve)");
        valid = false;
    } else {
        std::string crv = jwk["crv"].get<std::string>();
        // Common curves
        if (crv != "P-256" && crv != "P-384" && crv != "P-521" && 
            crv != "secp256k1" && crv != "Ed25519") {
            result.warnings.push_back(key_prefix + "Uncommon curve: " + crv);
        }
    }
    
    // Required: x (x coordinate)
    if (!jwk.contains("x")) {
        result.errors.push_back(key_prefix + "Missing 'x' coordinate");
        valid = false;
    }
    
    // Required: y (y coordinate)
    if (!jwk.contains("y")) {
        result.errors.push_back(key_prefix + "Missing 'y' coordinate");
        valid = false;
    }
    
    // Private key component should NOT be present
    if (jwk.contains("d")) {
        result.errors.push_back(key_prefix + "Contains private key material (security violation)");
        valid = false;
    }
    
    return valid;
}

bool JWKSValidator::validateSymmetricKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const {
    bool valid = true;
    std::string key_prefix = "Symmetric Key[" + std::to_string(index) + "]: ";
    
    // Required: k (key value)
    if (!jwk.contains("k")) {
        result.errors.push_back(key_prefix + "Missing 'k' (key value)");
        valid = false;
    }
    
    // Warning: symmetric keys in JWKS are unusual
    result.warnings.push_back(key_prefix + "Symmetric key in JWKS (unusual, usually only public keys)");
    
    return valid;
}

bool JWKSValidator::checkDuplicateKids(const nlohmann::json& jwks, ValidationResult& result) const {
    if (!jwks.contains("keys") || !jwks["keys"].is_array()) {
        return true;  // Already checked in validateStructure
    }
    
    std::set<std::string> seen_kids;
    const auto& keys = jwks["keys"];
    
    for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i].contains("kid")) {
            std::string kid = keys[i]["kid"].get<std::string>();
            
            if (seen_kids.count(kid) > 0) {
                result.warnings.push_back("Duplicate key ID: " + kid);
                return false;
            }
            
            seen_kids.insert(kid);
        }
    }
    
    return true;
}

} // namespace auth
} // namespace themis
