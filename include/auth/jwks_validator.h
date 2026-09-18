/**
 * @file jwks_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace themis {
namespace auth {

class JWKSValidator {
public:
    struct ValidationResult {
        bool valid = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        size_t key_count = 0;
        
        std::string getErrorSummary() const {
            std::string summary = {};
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

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit JWKSValidator(const Config& config = Config::defaults());
    
    /**
     * @brief Validate.
     * @param[in] jwks Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const nlohmann::json& jwks) const;
    
    /**
     * @brief Validate Or Throw.
     * @param[in] jwks Input parameter.
     */
    void validateOrThrow(const nlohmann::json& jwks) const;

private:
    Config config_;
    
    // Validate JWKS structure
    /**
     * @brief Validate Structure.
     * @param[in] jwks Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateStructure(const nlohmann::json& jwks, ValidationResult& result) const;
    
    // Validate individual JWK
    /**
     * @brief Validate Key.
     * @param[in] jwk Input parameter.
     * @param[in] index Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Validate RSA key
    /**
     * @brief Validate RSAKey.
     * @param[in] jwk Input parameter.
     * @param[in] index Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateRSAKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Validate EC key
    /**
     * @brief Validate ECKey.
     * @param[in] jwk Input parameter.
     * @param[in] index Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateECKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    // Validate symmetric key
    /**
     * @brief Validate Symmetric Key.
     * @param[in] jwk Input parameter.
     * @param[in] index Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateSymmetricKey(const nlohmann::json& jwk, size_t index, ValidationResult& result) const;
    
    /**
     * @brief Check for duplicate kids
     * @param[in] jwks Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool checkDuplicateKids(const nlohmann::json& jwks, ValidationResult& result) const;
};

} // namespace auth
} // namespace themis
