/**
 * @file edition_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: edition_manager.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 223
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * PR History (last 5): #3646 fix(themis): complete build... (2026-03-12) | #3598 feat(themis): complete Phas... (2026-03-12) | #3429 [WIP] Add full modularizati... (2026-03-12) | #3411 [themis] Add getRegisteredM... (2026-03-12) | #3410 feat(themis): Dynamic featu... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
#include "utils/logger.h"

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
    const int ceiling = SHARDING_MAX_NODES;  // compile-time, absolute ceiling

    // Step 1: Compile-time ceiling (Defense in Depth — never bypassed).
    if (ceiling >= 0 && requested_nodes > ceiling) {
        std::ostringstream msg;
        msg << "Requested node count (" << requested_nodes
            << ") exceeds the compile-time ceiling for the " << EDITION_STRING
            << " edition (" << ceiling << " nodes maximum).";
        if (GetEditionType() == EditionType::COMMUNITY) {
            msg << " Upgrade to Enterprise or Hyperscaler for higher limits.";
        }
        error_out = msg.str();
        return false;
    }

    // Step 2: Consult installed shard-limit policy (if any).
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        if (shard_policy_) {
            if (!shard_policy_->canExpand(requested_nodes)) {
                std::ostringstream msg;
                msg << "Requested node count (" << requested_nodes
                    << ") exceeds the active shard-limit policy bound ("
                    << effective_shard_nodes_ << " nodes).";
                error_out = msg.str();
                return false;
            }
            return true;
        }
    }

    // Step 3: No policy installed — use compile-time default.
    if (ceiling < 0) {
        // Unlimited (HYPERSCALER)
        return true;
    }
    if (requested_nodes <= ceiling) {
        return true;
    }
    std::ostringstream msg;
    msg << "Requested node count (" << requested_nodes
        << ") exceeds the limit for the " << EDITION_STRING
        << " edition (" << ceiling << " nodes maximum).";
    if (GetEditionType() == EditionType::COMMUNITY) {
        msg << " Upgrade to Enterprise or Hyperscaler for higher limits.";
    }
    error_out = msg.str();
    return false;
}

bool EditionManager::checkVRAMLimit(int requested_vram_gb,
                                    std::string& error_out) const {
    const int ceiling = GPU_MAX_VRAM_GB;  // compile-time, absolute ceiling

    // Step 1: Compile-time ceiling (Defense in Depth — never bypassed).
    if (ceiling >= 0 && requested_vram_gb > ceiling) {
        std::ostringstream msg;
        msg << "Requested GPU VRAM (" << requested_vram_gb
            << " GB) exceeds the compile-time ceiling for the " << EDITION_STRING
            << " edition (" << ceiling << " GB maximum).";
        if (GetEditionType() == EditionType::COMMUNITY) {
            msg << " Upgrade to Enterprise or Hyperscaler for higher GPU VRAM limits.";
        }
        error_out = msg.str();
        return false;
    }

    // Step 2: Consult installed VRAM policy (if any).
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        if (vram_policy_) {
            const size_t requested_bytes =
                static_cast<size_t>(requested_vram_gb) * 1024ULL * 1024ULL * 1024ULL;
            if (!vram_policy_->canAllocate(requested_bytes)) {
                std::ostringstream msg;
                msg << "Requested GPU VRAM (" << requested_vram_gb
                    << " GB) exceeds the active VRAM policy bound ("
                    << effective_vram_gb_ << " GB).";
                error_out = msg.str();
                return false;
            }
            return true;
        }
    }

    // Step 3: No policy installed — use compile-time default.
    if (ceiling < 0) {
        // Unlimited (HYPERSCALER)
        return true;
    }
    if (requested_vram_gb <= ceiling) {
        return true;
    }
    std::ostringstream msg;
    msg << "Requested GPU VRAM (" << requested_vram_gb
        << " GB) exceeds the limit for the " << EDITION_STRING
        << " edition (" << ceiling << " GB maximum).";
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

// ============================================================================
// Runtime resource-limit policies
// ============================================================================

bool EditionManager::installVRAMPolicy(std::shared_ptr<gpu::IVRAMPolicy> policy,
                                       int claimed_max_vram_gb)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installVRAMPolicy: null policy rejected.");
        return false;
    }

    // Defense in Depth: reject if the claimed limit exceeds the compile-time ceiling.
    // A ceiling of -1 means Hyperscaler (unlimited) — always accept.
    if (GPU_MAX_VRAM_GB >= 0 && claimed_max_vram_gb > GPU_MAX_VRAM_GB) {
        THEMIS_WARN(
            "EditionManager::installVRAMPolicy: claimed limit {} GB exceeds "
            "compile-time ceiling {} GB for edition '{}'. Policy rejected.",
            claimed_max_vram_gb, GPU_MAX_VRAM_GB, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    vram_policy_     = std::move(policy);
    effective_vram_gb_ = claimed_max_vram_gb;
    THEMIS_INFO(
        "EditionManager::installVRAMPolicy: VRAM policy installed "
        "(effective limit: {} GB).",
        effective_vram_gb_);
    return true;
}

bool EditionManager::installShardPolicy(
    std::shared_ptr<sharding::IShardLimitPolicy> policy,
    int claimed_max_nodes)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installShardPolicy: null policy rejected.");
        return false;
    }

    // Defense in Depth: reject if the claimed limit exceeds the compile-time ceiling.
    if (SHARDING_MAX_NODES >= 0 && claimed_max_nodes > SHARDING_MAX_NODES) {
        THEMIS_WARN(
            "EditionManager::installShardPolicy: claimed limit {} nodes exceeds "
            "compile-time ceiling {} nodes for edition '{}'. Policy rejected.",
            claimed_max_nodes, SHARDING_MAX_NODES, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    shard_policy_          = std::move(policy);
    effective_shard_nodes_ = claimed_max_nodes;
    THEMIS_INFO(
        "EditionManager::installShardPolicy: shard-limit policy installed "
        "(effective limit: {} nodes).",
        effective_shard_nodes_);
    return true;
}

void EditionManager::clearVRAMPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    vram_policy_.reset();
    effective_vram_gb_ = -2;
    THEMIS_INFO("EditionManager::clearVRAMPolicy: VRAM policy removed; "
                "reverted to compile-time default.");
}

void EditionManager::clearShardPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    shard_policy_.reset();
    effective_shard_nodes_ = -2;
    THEMIS_INFO("EditionManager::clearShardPolicy: shard-limit policy removed; "
                "reverted to compile-time default.");
}

} // namespace edition
} // namespace themis
