/*
 * ThemisDB Edition Manager – Implementation
 * ==========================================
 * Community / Enterprise / Hyperscaler edition feature gating.
 *
 * See include/themis/edition_manager.h for full API documentation.
 */

#include "themis/edition_manager.h"
#include "themis/runtime_license_gate.h"

#include <sstream>

namespace themis {
namespace edition {

// ============================================================================
// Singleton
// ============================================================================

EditionManager& EditionManager::instance() {
    static EditionManager mgr;
    return mgr;
}

// ============================================================================
// Feature availability
// ============================================================================

bool EditionManager::isFeatureAvailable(std::string_view feature_name) const {
    std::string unused;
    return isFeatureAvailable(feature_name, unused);
}

bool EditionManager::isFeatureAvailable(std::string_view feature_name,
                                        std::string& error_out) const {
    // Delegate to RuntimeLicenseGate which already implements the two-stage
    // (compile-time + runtime-license) check.
    return license::RuntimeLicenseGate::instance()
               .isFeatureAllowed(feature_name, error_out);
}

// ============================================================================
// Resource-limit checks
// ============================================================================

bool EditionManager::checkNodeLimit(int requested_nodes,
                                    std::string& error_out) const {
    const int max = getMaxNodes();
    if (max < 0) {
        // Unlimited (HYPERSCALER)
        return true;
    }
    if (requested_nodes <= max) {
        return true;
    }
    std::ostringstream msg;
    msg << "Requested node count (" << requested_nodes
        << ") exceeds the limit for the " << EDITION_STRING
        << " edition (" << max << " nodes maximum).";
    if (GetEditionType() == EditionType::COMMUNITY) {
        msg << " Upgrade to Enterprise or Hyperscaler for higher limits.";
    }
    error_out = msg.str();
    return false;
}

bool EditionManager::checkVRAMLimit(int requested_vram_gb,
                                    std::string& error_out) const {
    const int max = getMaxVRAMGB();
    if (max < 0) {
        // Unlimited (HYPERSCALER)
        return true;
    }
    if (requested_vram_gb <= max) {
        return true;
    }
    std::ostringstream msg;
    msg << "Requested GPU VRAM (" << requested_vram_gb
        << " GB) exceeds the limit for the " << EDITION_STRING
        << " edition (" << max << " GB maximum).";
    if (GetEditionType() == EditionType::COMMUNITY) {
        msg << " Upgrade to Enterprise or Hyperscaler for higher GPU VRAM limits.";
    }
    error_out = msg.str();
    return false;
}

// ============================================================================
// Edition information
// ============================================================================

EditionType EditionManager::getEditionType() const noexcept {
    return GetEditionType();
}

std::string_view EditionManager::getEditionName() const noexcept {
    return EDITION_STRING;
}

int EditionManager::getMaxNodes() const noexcept {
    return SHARDING_MAX_NODES;
}

int EditionManager::getMaxVRAMGB() const noexcept {
    return GPU_MAX_VRAM_GB;
}

// ============================================================================
// Feature enumeration
// ============================================================================

std::vector<std::string> EditionManager::getAvailableFeatures() const {
    std::vector<std::string> result;
    for (std::string_view feat : kGatedFeatureNames) {
        if (isFeatureAvailable(feat)) {
            result.emplace_back(feat);
        }
    }
    return result;
}

std::vector<std::string> EditionManager::getUnavailableFeatures() const {
    std::vector<std::string> result;
    for (std::string_view feat : kGatedFeatureNames) {
        if (!isFeatureAvailable(feat)) {
            result.emplace_back(feat);
        }
    }
    return result;
}

// ============================================================================
// Upgrade guidance
// ============================================================================

std::string EditionManager::getUpgradeMessage(std::string_view feature_name) const {
    if (isFeatureAvailable(feature_name)) {
        return {};
    }

    std::ostringstream msg;
    msg << "Feature '" << feature_name << "' is not available in the "
        << EDITION_STRING << " edition.";

    switch (GetEditionType()) {
        case EditionType::COMMUNITY:
            msg << " This feature requires Enterprise or Hyperscaler Edition."
                   " Visit https://themisdb.io/upgrade for licensing options.";
            break;
        case EditionType::ENTERPRISE:
            msg << " Please verify your license is active and has not expired."
                   " Contact support@themisdb.io for assistance.";
            break;
        default:
            msg << " Please contact your license provider.";
            break;
    }

    return msg.str();
}

} // namespace edition
} // namespace themis
