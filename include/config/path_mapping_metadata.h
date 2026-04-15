/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            path_mapping_metadata.h                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:06:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     106                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • aeb43de031  2026-04-12  feat(config): create include/config/ public header direct... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace themis {
namespace config {

/**
 * Metadata about a config path mapping, including deprecation information.
 */
struct PathMappingMetadata {
    std::string legacy_path;
    std::string new_path;
    std::string category;
    
    // Deprecation information
    std::optional<std::chrono::system_clock::time_point> deprecated_date;
    std::optional<std::chrono::system_clock::time_point> removal_date;
    std::optional<std::string> migration_guide_url;
    
    /**
     * Check if this mapping is currently deprecated.
     */
    bool isDeprecated() const {
        if (!deprecated_date.has_value()) {
            return false;
        }
        return std::chrono::system_clock::now() >= *deprecated_date;
    }
    
    /**
     * Check if removal deadline has passed.
     */
    bool isRemovalDue() const {
        if (!removal_date.has_value()) {
            return false;
        }
        return std::chrono::system_clock::now() >= *removal_date;
    }
    
    /**
     * Get days until removal (negative if already past removal date).
     */
    int daysUntilRemoval() const {
        if (!removal_date.has_value()) {
            return -1;
        }
        
        auto now = std::chrono::system_clock::now();
        auto duration = *removal_date - now;
        return std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    }
    
    /**
     * Get formatted deprecation warning message.
     */
    std::string getDeprecationMessage() const {
        std::string msg = "Config path '" + legacy_path + "' is deprecated. ";
        msg += "Please migrate to: '" + new_path + "'.";
        
        if (removal_date.has_value()) {
            int days = daysUntilRemoval();
            if (days > 0) {
                msg += " This path will be removed in " + std::to_string(days) + " days.";
            } else if (days == 0) {
                msg += " This path is scheduled for removal TODAY.";
            } else {
                msg += " This path was scheduled for removal " + std::to_string(-days) + " days ago.";
            }
        }
        
        if (migration_guide_url.has_value()) {
            msg += " Migration guide: " + *migration_guide_url;
        }
        
        return msg;
    }
};

} // namespace config
} // namespace themis
