/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            production_mode.h                                  ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:34:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f82bf2ae9f  2026-03-04  Refactor tenant manager tests and add new test cases ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <cstdlib>
#include <stdexcept>

namespace themis {
namespace core {

/**
 * @brief Production mode detection and enforcement utilities
 */
class ProductionMode {
public:
    /**
     * @brief Check if production mode is enabled
     * 
     * Production mode is enabled when:
     * - THEMIS_PRODUCTION_MODE=1 (or true, yes, on)
     * - OR THEMIS_ENVIRONMENT=production
     * 
     * @return true if production mode is enabled
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
     * @brief Enforce production mode requirement
     * 
     * @param condition The security condition that must be met
     * @param error_message Error message if condition fails in production
     * @throws std::runtime_error if production mode is enabled and condition is false
     */
    static void enforce(bool condition, const std::string& error_message) {
        if (isEnabled() && !condition) {
            throw std::runtime_error("Production mode violation: " + error_message);
        }
    }
    
    /**
     * @brief Get the current mode name for logging
     * @return "production" or "development"
     */
    static std::string modeName() {
        return isEnabled() ? "production" : "development";
    }
};

} // namespace core
} // namespace themis
