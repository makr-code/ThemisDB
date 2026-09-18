/**
 * @file password_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>

namespace themis {
namespace auth {

class PasswordPolicy {
public:
    struct Config {
        size_t min_length = 12;

        size_t max_length = 128;

        bool require_uppercase = true;

        bool require_lowercase = true;

        bool require_digit = true;

        bool require_special = true;

        std::string special_chars = "!@#$%^&*()_+-=[]{}|;':\",./<>?";

        size_t min_unique_chars = 0;

        size_t max_consecutive_identical = 0;

        double min_entropy_bits = 0.0;

        std::vector<std::string> forbidden_patterns;
    };

    struct ValidationResult {
        bool valid = false;

        std::vector<std::string> violations;

        explicit operator bool() const { return valid; }
    };

    PasswordPolicy();

    /**
     * @brief Password Policy.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PasswordPolicy(const Config& config);

    /**
     * @brief Validate.
     * @param[in] password Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const std::string& password) const;

    /**
     * @brief Is Compliant.
     * @param[in] password Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCompliant(const std::string& password) const;

    const Config& getConfig() const { return config_; }

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const Config& config) { config_ = config; }

    /**
     * @brief Compute Entropy.
     * @param[in] password Input parameter.
     * @return Return value.
     */
    static double computeEntropy(const std::string& password);

    /**
     * @brief ---- Preset factories ------------------------------------------------
     * @return Return value.
     */

    static PasswordPolicy nistGuidelines();

    /**
     * @brief Strict.
     * @return Return value.
     */
    static PasswordPolicy strict();

    /**
     * @brief Basic.
     * @return Return value.
     */
    static PasswordPolicy basic();

private:
    Config config_;
};

} // namespace auth
} // namespace themis
