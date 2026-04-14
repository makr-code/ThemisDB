/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tenant_update_scheduler.h                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 06:58:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     457                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bcd8bdb1e3  2026-03-14  fix(updates): address all PR review comments for TenantUp... ║
    • a2504b0259  2026-03-13  feat(updates): implement Multi-Tenant Update Scheduling (... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file tenant_update_scheduler.h
 * @brief Multi-tenant update scheduling (v1.8.0, Issue #262).
 *
 * Implements per-tenant maintenance windows, blackout periods, priority
 * tiers, update consent, and per-tenant rollback tracking.
 *
 * Usage:
 * @code
 *   TenantUpdateScheduler scheduler;
 *
 *   scheduler.setMaintenanceWindow("tenant-123", {
 *       .days = {"Saturday", "Sunday"},
 *       .start_time = "02:00",
 *       .end_time   = "06:00",
 *       .timezone   = "America/New_York"
 *   });
 *
 *   scheduler.setUpdatePolicy("tenant-123", {
 *       .auto_update = false,
 *       .critical_auto_update = true,
 *       .notification_lead_time = std::chrono::hours(24)
 *   });
 *
 *   if (scheduler.canUpdateNow("tenant-123")) {
 *       engine->applyHotReload("1.5.0");
 *   } else {
 *       auto next = scheduler.getNextMaintenanceWindow("tenant-123");
 *       LOG_INFO("Next maintenance window: {}", next);
 *   }
 * @endcode
 */

#pragma once

#include "updates/hot_reload_engine.h"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Priority tiers
// ---------------------------------------------------------------------------

/**
 * @brief Priority tier for an update.
 *
 * Determines scheduling and consent rules:
 *  - CRITICAL : when `UpdatePolicy::critical_auto_update` is true, bypasses
 *               blackout periods, maintenance-window restrictions, and the
 *               manual-consent requirement.
 *  - NORMAL   : follows regular maintenance windows and consent rules.
 *  - LOW      : treated identically to NORMAL by the scheduler (follows
 *               maintenance windows and consent rules); callers may use this
 *               tier as a semantic hint, but no special deferral logic is
 *               applied automatically.
 */
enum class UpdatePriority {
    CRITICAL, ///< Security patches and data-integrity fixes.
    NORMAL,   ///< Feature releases and minor patches.
    LOW,      ///< Optional enhancements; applied only when explicitly approved.
};

// ---------------------------------------------------------------------------
// MaintenanceWindow
// ---------------------------------------------------------------------------

/**
 * @brief Time-of-week window during which updates are allowed.
 *
 * Time values use 24-hour "HH:MM" format.  Days are matched
 * case-insensitively against: "Monday", "Tuesday", "Wednesday",
 * "Thursday", "Friday", "Saturday", "Sunday", and the special token
 * "Daily" (which matches every day of the week).
 */
struct MaintenanceWindow {
    /// Days on which updates are permitted.  Use {"Daily"} for every day.
    std::vector<std::string> days;

    /// Half-open interval [start, end) in "HH:MM" format.
    /// Cross-midnight ranges (e.g., "23:00" → "05:00") are supported.
    std::string start_time; ///< e.g. "02:00"
    std::string end_time;   ///< e.g. "06:00"

    /// IANA timezone name (informational; scheduling uses UTC wall-clock by
    /// default because most test environments lack a timezone database).
    std::string timezone;
};

// ---------------------------------------------------------------------------
// BlackoutPeriod
// ---------------------------------------------------------------------------

/**
 * @brief Absolute time range during which no updates are allowed for a tenant.
 *
 * Useful for freezing changes around business-critical events (e.g., year-end
 * processing, quarterly releases).
 */
struct BlackoutPeriod {
    /// Unique identifier assigned by the caller; used to remove the period.
    std::string id;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    /// Optional human-readable reason.
    std::string reason;
};

// ---------------------------------------------------------------------------
// UpdatePolicy
// ---------------------------------------------------------------------------

/**
 * @brief Per-tenant consent and notification policy.
 */
struct UpdatePolicy {
    /// When false, all updates require explicit approval via `grantConsent()`.
    bool auto_update = true;

    /// When true, CRITICAL updates are auto-applied even if `auto_update` is
    /// false (tenant approves critical security patches implicitly).
    bool critical_auto_update = true;

    /// How far in advance tenants must be notified before a scheduled update
    /// can proceed.  The scheduler will not apply an update until at least
    /// this much time has elapsed since the notification was recorded via
    /// `recordNotification()`.
    std::chrono::hours notification_lead_time{0};
};

// ---------------------------------------------------------------------------
// TenantUpdateStatus
// ---------------------------------------------------------------------------

/**
 * @brief Status snapshot for a single tenant.
 */
struct TenantUpdateStatus {
    std::string tenant_id;

    /// Version currently deployed for this tenant.
    std::string current_version;

    /// Pending update version (non-empty when an update has been staged but
    /// not yet applied).
    std::string pending_version;

    /// Latest rollback ID stored for this tenant (enables per-tenant rollback).
    std::string last_rollback_id;

    /// True when a pending update has been explicitly consented.
    bool consent_granted = false;

    /// True when the tenant is currently inside a maintenance window.
    bool in_maintenance_window = false;

    /// True when the tenant is inside an active blackout period.
    bool in_blackout_period = false;
};

// ---------------------------------------------------------------------------
// TenantUpdateScheduler
// ---------------------------------------------------------------------------

/**
 * @brief Manages per-tenant update schedules and maintenance windows.
 *
 * Thread-safe: all public methods are protected by an internal mutex.
 *
 * Design notes:
 *  - The scheduler is transport-agnostic: it only decides *whether* an
 *    update can proceed.  Callers use `HotReloadEngine` directly after
 *    `canUpdateNow()` returns true.
 *  - Timezone-aware scheduling requires a tz database.  Without one the
 *    scheduler operates on UTC.  The `MaintenanceWindow::timezone` field
 *    is stored for documentation purposes; full tz support can be added
 *    as a later enhancement.
 *  - For cross-midnight windows (e.g., 23:00 → 05:00) the arithmetic is
 *    handled correctly by comparing modular minute-of-day values.
 */
class TenantUpdateScheduler {
public:
    /**
     * @brief Clock function injected for unit-testability.
     *
     * Defaults to `std::chrono::system_clock::now`.
     */
    using ClockFn = std::function<std::chrono::system_clock::time_point()>;

    // Construction / destruction

    TenantUpdateScheduler();
    explicit TenantUpdateScheduler(ClockFn clock_fn);
    ~TenantUpdateScheduler() = default;

    // Non-copyable
    TenantUpdateScheduler(const TenantUpdateScheduler&) = delete;
    TenantUpdateScheduler& operator=(const TenantUpdateScheduler&) = delete;

    // ---------- Maintenance windows ----------

    /**
     * @brief Configure (or replace) the maintenance window for a tenant.
     * @param tenant_id  Stable tenant identifier.
     * @param window     Window configuration.
     */
    void setMaintenanceWindow(const std::string& tenant_id,
                              const MaintenanceWindow& window);

    /**
     * @brief Remove the maintenance window for a tenant.
     *
     * After removal, `canUpdateNow()` returns false for the tenant unless
     * the CRITICAL bypass applies (`critical_auto_update` is true and the
     * update priority is CRITICAL).
     */
    void removeMaintenanceWindow(const std::string& tenant_id);

    /**
     * @brief Retrieve the maintenance window for a tenant.
     * @return std::nullopt if no window is configured.
     */
    std::optional<MaintenanceWindow>
    getMaintenanceWindow(const std::string& tenant_id) const;

    // ---------- Blackout periods ----------

    /**
     * @brief Add a blackout period for a tenant.
     *
     * A tenant with an active blackout is never eligible for updates
     * regardless of the maintenance window (blackouts take precedence).
     *
     * @param tenant_id Stable tenant identifier.
     * @param period    Blackout period with a caller-supplied unique `id`.
     */
    void addBlackoutPeriod(const std::string& tenant_id,
                           const BlackoutPeriod& period);

    /**
     * @brief Remove a specific blackout period by its `id`.
     * @return true if the period was found and removed.
     */
    bool removeBlackoutPeriod(const std::string& tenant_id,
                              const std::string& blackout_id);

    /**
     * @brief Return all configured blackout periods for a tenant.
     *
     * Returns every stored period regardless of whether it is currently
     * active or has already expired.
     */
    std::vector<BlackoutPeriod>
    getBlackoutPeriods(const std::string& tenant_id) const;

    // ---------- Update policy ----------

    /**
     * @brief Set the update policy for a tenant.
     */
    void setUpdatePolicy(const std::string& tenant_id,
                         const UpdatePolicy& policy);

    /**
     * @brief Get the update policy for a tenant.
     * @return Default policy if none has been explicitly set.
     */
    UpdatePolicy getUpdatePolicy(const std::string& tenant_id) const;

    // ---------- Consent management ----------

    /**
     * @brief Record that the tenant has been notified of a pending update.
     *
     * The notification timestamp is stored and used to enforce
     * `UpdatePolicy::notification_lead_time`.
     *
     * @param tenant_id      Tenant identifier.
     * @param pending_version Version that will be applied.
     */
    void recordNotification(const std::string& tenant_id,
                            const std::string& pending_version);

    /**
     * @brief Grant explicit consent for the pending update.
     *
     * Required when `UpdatePolicy::auto_update` is false.
     *
     * @param tenant_id Tenant identifier.
     * @param version   Version being consented to (must match pending).
     * @return true if consent was accepted.
     */
    bool grantConsent(const std::string& tenant_id,
                      const std::string& version);

    /**
     * @brief Revoke previously granted consent (e.g., tenant changed mind).
     */
    void revokeConsent(const std::string& tenant_id);

    /**
     * @brief Check whether consent is currently granted for a tenant.
     */
    bool hasConsent(const std::string& tenant_id) const;

    // ---------- Scheduling queries ----------

    /**
     * @brief Check whether a tenant update may proceed right now.
     *
     * Evaluation order:
     *  1. Active blackout period → false (unless CRITICAL + critical_auto_update).
     *  2. Outside maintenance window → false (unless CRITICAL + critical_auto_update).
     *  3. Notification lead time not yet elapsed → false.
     *  4. `auto_update` false and consent not granted → false.
     *  5. All checks pass → true.
     *
     * @param tenant_id Tenant identifier.
     * @param priority  Priority tier of the update (default: NORMAL).
     * @return true when the update may be applied now.
     */
    bool canUpdateNow(const std::string& tenant_id,
                      UpdatePriority priority = UpdatePriority::NORMAL) const;

    /**
     * @brief Return the start time of the next maintenance window.
     *
     * Searches up to 7 days into the future.  Returns an empty string if no
     * maintenance window is configured for the tenant.
     *
     * @return ISO-8601 UTC datetime string (e.g., "2026-03-14T02:00:00Z")
     *         or an empty string when no window can be found.
     */
    std::string getNextMaintenanceWindow(const std::string& tenant_id) const;

    // ---------- Update application & rollback ----------

    /**
     * @brief Apply an update for a specific tenant using the provided engine.
     *
     * Validates scheduling constraints before delegating to
     * `HotReloadEngine::applyHotReload`.  Stores the resulting rollback ID
     * for later use by `rollbackTenant()`.
     *
     * @param tenant_id Tenant identifier.
     * @param version   Version to apply.
     * @param engine    Engine used for the actual file update.
     * @param priority  Priority tier of this update.
     * @return ReloadResult from the engine; on scheduling refusal
     *         `success` is false and `error_message` explains why.
     */
    ReloadResult applyUpdate(const std::string& tenant_id,
                             const std::string& version,
                             HotReloadEngine& engine,
                             UpdatePriority priority = UpdatePriority::NORMAL);

    /**
     * @brief Rollback the last update applied for a tenant.
     *
     * Uses the rollback ID stored by the most recent successful
     * `applyUpdate()` call for this tenant.
     *
     * @param tenant_id Tenant identifier.
     * @param engine    Engine used to perform the rollback.
     * @return true if the rollback succeeded or there was nothing to roll back.
     */
    bool rollbackTenant(const std::string& tenant_id,
                        HotReloadEngine& engine);

    // ---------- Status ----------

    /**
     * @brief Return a status snapshot for a tenant.
     * @return std::nullopt if the tenant is not registered.
     */
    std::optional<TenantUpdateStatus>
    getTenantStatus(const std::string& tenant_id) const;

    /**
     * @brief Return status snapshots for all known tenants.
     */
    std::vector<TenantUpdateStatus> getAllTenantStatuses() const;

    /**
     * @brief Remove all state for a tenant (de-provisioning).
     */
    void removeTenant(const std::string& tenant_id);

private:
    // Per-tenant state record.
    struct TenantState {
        std::optional<MaintenanceWindow>    window;
        std::vector<BlackoutPeriod>         blackouts;
        UpdatePolicy                        policy;
        std::string                         current_version;
        std::string                         pending_version;
        std::string                         last_rollback_id;
        bool                                consent_granted = false;
        std::optional<std::chrono::system_clock::time_point> notification_time;
    };

    // Parse "HH:MM" → minutes since midnight.  Returns -1 on parse error.
    static int parseMinutes(const std::string& hhmm);

    // Return true when the given UTC time falls within the window.
    static bool isInWindow(const MaintenanceWindow& win,
                           std::chrono::system_clock::time_point tp);

    // Return true when tp falls inside any active blackout period.
    static bool isInBlackout(const std::vector<BlackoutPeriod>& blackouts,
                             std::chrono::system_clock::time_point tp);

    // Format a time_point as "YYYY-MM-DDTHH:MM:SSZ".
    static std::string formatUtc(std::chrono::system_clock::time_point tp);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, TenantState> tenants_;
    ClockFn clock_fn_;
};

} // namespace updates
} // namespace themis
