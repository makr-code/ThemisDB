/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            edition_manager.cpp                                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:20:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     238                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a4de3d12cc  2026-03-01  feat(themis): implement dynamic feature flag override API... ║
    • 0cbb725b3a  2026-02-23  feat(themis): implement edition_manager.cpp for Community... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB Edition Manager – Implementation
 * ==========================================
 * Community / Enterprise / Hyperscaler edition feature gating.
 *
 * See include/themis/edition_manager.h for full API documentation.
 */

#include "themis/edition_manager.h"
#include "themis/runtime_license_gate.h"

#include <optional>
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
    // Step 0: Check dynamic override first (if any).
    {
        std::lock_guard<std::mutex> lock(overrides_mutex_);
        auto it = overrides_.find(std::string(feature_name));
        if (it != overrides_.end() && !it->second) {
            // Override explicitly set to false → always blocked by admin.
            std::ostringstream msg;
            msg << "Feature '" << feature_name
                << "' has been administratively disabled at runtime.";
            error_out = msg.str();
            return false;
        }
        // Override=true means "allow if edition+license also allow" — fall through.
    }

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

// ============================================================================
// Dynamic feature-flag overrides
// ============================================================================

void EditionManager::setFeatureOverride(std::string_view feature_name, bool enabled) {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    overrides_[std::string(feature_name)] = enabled;
}

void EditionManager::clearFeatureOverride(std::string_view feature_name) {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    overrides_.erase(std::string(feature_name));
}

void EditionManager::clearAllFeatureOverrides() {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    overrides_.clear();
}

bool EditionManager::hasFeatureOverride(std::string_view feature_name) const {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    return overrides_.find(std::string(feature_name)) != overrides_.end();
}

std::optional<bool> EditionManager::getFeatureOverride(
        std::string_view feature_name) const {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    auto it = overrides_.find(std::string(feature_name));
    if (it == overrides_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace edition
} // namespace themis
