/**
 * @file production_mode.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <cstdlib>
#include <stdexcept>

namespace themis {
namespace core {

class ProductionMode {
public:
    /**
     * @brief Is Enabled.
     * @return True when the operation succeeds.
     * @details Calls: std::getenv(), mode_str(), env_str().
     */
    static bool isEnabled() {
        const char* prod_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        const char* environment = std::getenv("THEMIS_ENVIRONMENT");
        
        // Check THEMIS_PRODUCTION_MODE
        if (prod_mode) {
            std::string mode_str(prod_mode);
            if (mode_str == "1" || mode_str == "true" || 
                mode_str == "True" || mode_str == "TRUE" ||
                mode_str == "yes" || mode_str == "Yes" ||
                mode_str == "on" || mode_str == "On") {
                return true;
            }
        }
        
        // Check THEMIS_ENVIRONMENT
        if (environment) {
            std::string env_str(environment);
            if (env_str == "production" || env_str == "prod") {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Enforce.
     * @param[in] condition Input parameter.
     * @param[in] error_message Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: isEnabled().
     */
    static void enforce(bool condition, const std::string& error_message) {
        if (isEnabled() && !condition) {
            throw std::runtime_error("Production mode violation: " + error_message);
        }
    }
    
    /**
     * @brief Mode Name.
     * @return Return value.
     * @details Calls: isEnabled().
     */
    static std::string modeName() {
        return isEnabled() ? "production" : "development";
    }
};

} // namespace core
} // namespace themis
