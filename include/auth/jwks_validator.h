/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jwks_validator.h                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:50:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     135                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 92608937d0  2026-02-26  fix: GCC default-arg error in 18 headers - add ::defaults... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
