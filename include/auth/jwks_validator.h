/**
 * @file jwks_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace themis {
namespace auth {

/**
 * @brief JWKS (JSON Web Key Set) Schema Validator
 * 
 * Validates JWKS documents according to RFC 7517 (JSON Web Key)
 * and RFC 7518 (JSON Web Algorithms) before caching.
 * 
 * Security: Prevents malformed or malicious JWKS from being cached,
 * which could lead to authentication bypasses or DoS attacks.
 * 
 * P1 (High Priority) security hardening feature.
 */
class JWKSValidator {
public:
    struct ValidationResult {
        bool valid = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        size_t key_count = 0;
        
        std::string getErrorSummary() const {
            std::string summary;
            for (const auto& err : errors) {
                summary += "ERROR: " + err + "\n";
            }
            for (const auto& warn : warnings) {
                summary += "WARNING: " + warn + "\n";
            }
            return summary;
        }
    };
    
    struct Config {
        // Strict mode: reject on warnings
        bool strict_mode = false;
        
        // Maximum number of keys in JWKS (prevent memory exhaustion)
        size_t max_keys = 100;
        
        // Allowed key types (kty)
        std::vector<std::string> allowed_key_types = {"RSA", "EC", "oct"};
        
        // Allowed algorithms (alg)
        std::vector<std::string> allowed_algorithms = {
            "RS256", "RS384", "RS512",
            "ES256", "ES384", "ES512",
            "HS256", "HS384", "HS512"
        };
        
        // Minimum RSA key size (bits)
        size_t min_rsa_key_size = 2048;
        
        // Require kid (key ID) for each key
        bool require_kid = true;
        
        // Require use field
        bool require_use = false;

        static Config defaults() { return {}; }
    };
    
    explicit JWKSValidator(const Config& config = Config::defaults());
    
    /**
     * @brief Validate a JWKS document
     * 
     * @param jwks JSON object containing JWKS
     * @return ValidationResult with errors/warnings
     */
    ValidationResult validate(const nlohmann::json& jwks) const;
    
    /**
     * @brief Validate and throw on error
     * 
     * @param jwks JSON object containing JWKS
     * @throws std::runtime_error if validation fails
     */
    void validateOrThrow(const nlohmann::json& jwks) const;

private:
    Config config_;
    
    // Validate JWKS structure
    bool validateStructure(const nlohmann::json& jwks, ValidationResult& result) const;
    
    // Validate individual JWK
    bool validateKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Validate RSA key
    bool validateRSAKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Validate EC key
    bool validateECKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Validate symmetric key
    bool validateSymmetricKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Check for duplicate kids
    bool checkDuplicateKids(const nlohmann::json& jwks, ValidationResult& result) const;
};

} // namespace auth
} // namespace themis
