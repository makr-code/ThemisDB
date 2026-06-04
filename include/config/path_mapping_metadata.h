/**
 * @file path_mapping_metadata.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: path_mapping_metadata.h | Version: 0.0.10 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 92
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4953 docs(config): expand includ... (2026-05-11) | #4479 feat(config): create includ... (2026-04-12) | #2976 [config] Automatic legacy p... (2026-03-12) | #2838 feat(config): Multi-environ... (2026-03-12) | #2834 feat(config): runtime hot-r... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
