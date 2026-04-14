/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            edition_manager.h                                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:29:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     265                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 4da3502dd0  2026-03-12  feat(themis): add THEMIS_BASE_API export macros to public... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a4de3d12cc  2026-03-01  feat(themis): implement dynamic feature flag override API... ║
    • 0cbb725b3a  2026-02-23  feat(themis): implement edition_manager.cpp for Community... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB Edition Manager
 * ========================
 * Runtime feature-gating for Community / Enterprise / Hyperscaler editions.
 *
 * The EditionManager is the single authoritative source for feature and
 * resource-limit decisions at runtime.  It combines two independent checks:
 *
 *   1. Compile-time gate  – edition::IsFeatureEnabled() from edition.h
 *      If the binary was built for Community edition, Enterprise-only features
 *      are never allowed regardless of any runtime state.
 *
 *   2. Runtime gate – license::RuntimeLicenseGate::instance()
 *      For Enterprise/Hyperscaler binaries, the active license must also be
 *      valid ("active" or "grace") for Enterprise-only features to be used.
 *
 * Usage:
 *
 *   // Feature check (returns bool + optional error message):
 *   std::string err;
 *   if (!EditionManager::instance().isFeatureAvailable("field_encryption", err)) {
 *       return {ErrorCode::LicenseRequired, err};
 *   }
 *
 *   // Resource-limit check:
 *   if (!EditionManager::instance().checkNodeLimit(requested_nodes, err)) {
 *       return {ErrorCode::EditionLimitExceeded, err};
 *   }
 */

#pragma once

#include "themis/edition.h"
#include "themis/export.h"

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace edition {

// ============================================================================
// EditionManager
// ============================================================================

/**
 * @brief Process-wide singleton that enforces edition feature and resource
 *        limits at runtime.
 *
 * All public methods are thread-safe.
 */
class THEMIS_BASE_API EditionManager {
public:
    /// Retrieve the process-wide singleton instance.
    static EditionManager& instance();

    // Non-copyable / non-movable (singleton)
    EditionManager(const EditionManager&)            = delete;
    EditionManager& operator=(const EditionManager&) = delete;

    // -------------------------------------------------------------------------
    // Feature availability
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if the named feature is available at runtime.
     *
     * The check is two-stage:
     *   1. Compile-time: edition::IsFeatureEnabled(feature_name) must be true.
     *   2. Runtime: license::RuntimeLicenseGate::instance() must allow it.
     *
     * Features that are not in the Enterprise gate list (i.e. Community
     * features) are always allowed.
     *
     * @param feature_name  One of: "enterprise_plugins", "multi_master",
     *                      "field_encryption", "rbac", "hsm", or any unknown
     *                      feature name (unknown → always allowed).
     */
    bool isFeatureAvailable(std::string_view feature_name) const;

    /**
     * @brief Like isFeatureAvailable() but also populates @p error_out with a
     *        human-readable explanation when returning false.
     */
    bool isFeatureAvailable(std::string_view feature_name,
                            std::string& error_out) const;

    // -------------------------------------------------------------------------
    // Resource-limit checks
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if @p requested_nodes is within the edition limit.
     *
     * COMMUNITY:    up to SHARDING_MAX_NODES
     * ENTERPRISE:   up to SHARDING_MAX_NODES
     * HYPERSCALER:  unlimited (always returns true)
     *
     * @param requested_nodes  Number of shard nodes the caller wants to use.
     * @param error_out        Populated with an error string on failure.
     */
    bool checkNodeLimit(int requested_nodes, std::string& error_out) const;

    /**
     * @brief Returns true if @p requested_vram_gb is within the edition limit.
     *
     * COMMUNITY:    up to GPU_MAX_VRAM_GB
     * ENTERPRISE:   up to GPU_MAX_VRAM_GB
     * HYPERSCALER:  unlimited (always returns true)
     *
     * @param requested_vram_gb  VRAM to allocate in gigabytes.
     * @param error_out          Populated with an error string on failure.
     */
    bool checkVRAMLimit(int requested_vram_gb, std::string& error_out) const;

    // -------------------------------------------------------------------------
    // Edition information
    // -------------------------------------------------------------------------

    /// Returns the compile-time edition type.
    EditionType getEditionType() const noexcept;

    /// Returns the compile-time edition name string (e.g. "COMMUNITY").
    std::string_view getEditionName() const noexcept;

    /// Maximum shard nodes for this edition (-1 = unlimited).
    int getMaxNodes() const noexcept;

    /// Maximum GPU VRAM in GB for this edition (-1 = unlimited).
    int getMaxVRAMGB() const noexcept;

    // -------------------------------------------------------------------------
    // Feature enumeration
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the names of all known gated features that are currently
     *        available (both compile-time ON and runtime license valid).
     */
    std::vector<std::string> getAvailableFeatures() const;

    /**
     * @brief Returns the names of all known gated features that are currently
     *        unavailable (compile-time OFF or runtime license invalid).
     */
    std::vector<std::string> getUnavailableFeatures() const;

    // -------------------------------------------------------------------------
    // Upgrade guidance
    // -------------------------------------------------------------------------

    /**
     * @brief Returns a human-readable upgrade message for the given feature.
     *
     * The message explains which edition supports the feature and how to
     * obtain it.  Returns an empty string if the feature is already available.
     */
    std::string getUpgradeMessage(std::string_view feature_name) const;

    // -------------------------------------------------------------------------
    // Dynamic feature-flag overrides
    // -------------------------------------------------------------------------

    /**
     * @brief Set a runtime override for a named feature flag.
     *
     * Overrides are layered on top of (and cannot bypass) the compile-time
     * edition gate.  An override of @c true for a Community-edition feature
     * that is compile-time disabled is silently stored but will never allow
     * the feature — the compile-time gate still applies.
     *
     * Typical use-cases:
     *  - Staged rollout: set to @c false while validating, flip to @c true
     *    when ready to expose the feature to this node.
     *  - Maintenance mode: set to @c false to block a feature without
     *    redeployment.
     *
     * Thread-safe.
     *
     * @param feature_name  Feature to override (see edition::kGatedFeatureNames).
     * @param enabled       @c true = allow (if edition/license also allow);
     *                      @c false = always block.
     */
    void setFeatureOverride(std::string_view feature_name, bool enabled);

    /**
     * @brief Remove the runtime override for a named feature flag.
     *
     * After calling this, @c isFeatureAvailable(feature_name) reverts to
     * the standard edition + license decision.  A no-op if no override
     * exists for @p feature_name.
     *
     * Thread-safe.
     */
    void clearFeatureOverride(std::string_view feature_name);

    /**
     * @brief Remove all runtime overrides.
     *
     * After this call every feature reverts to the standard edition + license
     * decision.
     *
     * Thread-safe.
     */
    void clearAllFeatureOverrides();

    /**
     * @brief Returns @c true if a runtime override is set for the feature.
     *
     * Does not indicate whether the feature is available — use
     * @c isFeatureAvailable() for that.
     *
     * Thread-safe.
     */
    bool hasFeatureOverride(std::string_view feature_name) const;

    /**
     * @brief Returns the current runtime override value, if any.
     *
     * @return @c std::nullopt when no override is set; @c true / @c false
     *         when an override has been set via @c setFeatureOverride().
     *
     * Thread-safe.
     */
    std::optional<bool> getFeatureOverride(std::string_view feature_name) const;

private:
    EditionManager()  = default;
    ~EditionManager() = default;

    mutable std::mutex                       overrides_mutex_;
    std::unordered_map<std::string, bool>    overrides_;
};

} // namespace edition
} // namespace themis
