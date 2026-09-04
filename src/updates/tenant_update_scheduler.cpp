/**
 * @file tenant_update_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/tenant_update_scheduler.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TenantUpdateScheduler::TenantUpdateScheduler()
    : clock_fn_([]() { return std::chrono::system_clock::now(); })
{}

TenantUpdateScheduler::TenantUpdateScheduler(ClockFn clock_fn)
    : clock_fn_(std::move(clock_fn))
{
    if (!clock_fn_) {
        throw std::invalid_argument(
            "TenantUpdateScheduler: clock_fn must not be null");
    }
}

// ---------------------------------------------------------------------------
// Maintenance windows
// ---------------------------------------------------------------------------

void TenantUpdateScheduler::setMaintenanceWindow(const std::string& tenant_id,
                                                  const MaintenanceWindow& window)
{
    std::lock_guard<std::mutex> lock(mutex_);
    tenants_[tenant_id].window = window;
    LOG_DEBUG("TenantUpdateScheduler: set maintenance window for '{}'", tenant_id);
}

void TenantUpdateScheduler::removeMaintenanceWindow(const std::string& tenant_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it != tenants_.end()) {
        it->second.window.reset();
    }
}

std::optional<MaintenanceWindow>
TenantUpdateScheduler::getMaintenanceWindow(const std::string& tenant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return std::nullopt;
    }
    return it->second.window;
}

// ---------------------------------------------------------------------------
// Blackout periods
// ---------------------------------------------------------------------------

void TenantUpdateScheduler::addBlackoutPeriod(const std::string& tenant_id,
                                               const BlackoutPeriod& period)
{
    std::lock_guard<std::mutex> lock(mutex_);
    tenants_[tenant_id].blackouts.push_back(period);
    LOG_DEBUG("TenantUpdateScheduler: added blackout '{}' for '{}'",
              period.id, tenant_id);
}

bool TenantUpdateScheduler::removeBlackoutPeriod(const std::string& tenant_id,
                                                  const std::string& blackout_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return false;
    }
    auto& vec = it->second.blackouts;
    auto before = vec.size();
    vec.erase(std::remove_if(vec.begin(), vec.end(),
                              [&]([[maybe_unused]] const BlackoutPeriod& b) {
                                  return b.id == blackout_id;
                              }),
              vec.end());
    return static_cast<int>(vec.size()) < before;
}

std::vector<BlackoutPeriod>
TenantUpdateScheduler::getBlackoutPeriods(const std::string& tenant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return {};
    }
    return it->second.blackouts;
}

// ---------------------------------------------------------------------------
// Update policy
// ---------------------------------------------------------------------------

void TenantUpdateScheduler::setUpdatePolicy(const std::string& tenant_id,
                                             const UpdatePolicy& policy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    tenants_[tenant_id].policy = policy;
    LOG_DEBUG("TenantUpdateScheduler: set policy for '{}' (auto_update={}, "
              "critical_auto_update={})",
              tenant_id, policy.auto_update, policy.critical_auto_update);
}

UpdatePolicy
TenantUpdateScheduler::getUpdatePolicy(const std::string& tenant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return UpdatePolicy{};
    }
    return it->second.policy;
}

// ---------------------------------------------------------------------------
// Consent management
// ---------------------------------------------------------------------------

void TenantUpdateScheduler::recordNotification(const std::string& tenant_id,
                                                const std::string& pending_version)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto& state = tenants_[tenant_id];
    state.pending_version   = pending_version;
    state.notification_time = clock_fn_();
    state.consent_granted   = false; // fresh notification resets prior consent
    LOG_INFO("TenantUpdateScheduler: recorded notification for tenant '{}' "
             "version '{}'",
             tenant_id, pending_version);
}

bool TenantUpdateScheduler::grantConsent(const std::string& tenant_id,
                                          const std::string& version)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        LOG_WARN("TenantUpdateScheduler: grantConsent – unknown tenant '{}'",
                 tenant_id);
        return false;
    }
    auto& state = it->second;
    if (state.pending_version != version) {
        LOG_WARN("TenantUpdateScheduler: grantConsent – version mismatch for "
                 "'{}': pending='{}' got='{}'",
                 tenant_id, state.pending_version, version);
        return false;
    }
    state.consent_granted = true;
    LOG_INFO("TenantUpdateScheduler: consent granted for tenant '{}' version '{}'",
             tenant_id, version);
    return true;
}

void TenantUpdateScheduler::revokeConsent(const std::string& tenant_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it != tenants_.end()) {
        it->second.consent_granted = false;
    }
}

bool TenantUpdateScheduler::hasConsent(const std::string& tenant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 7510 Fix: Explicit null check order for readability
    // Check if key exists BEFORE dereferencing
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return false;
    }
    // Now safe to access it->second
    return it->second.consent_granted;
}

// ---------------------------------------------------------------------------
// Scheduling queries
// ---------------------------------------------------------------------------

bool TenantUpdateScheduler::canUpdateNow(const std::string& tenant_id,
                                          UpdatePriority priority) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        // Unknown tenant: no window configured → disallow.
        return false;
    }
    const TenantState& state = it->second;
    const auto now = clock_fn_();

    const bool is_critical =
        (priority == UpdatePriority::CRITICAL) && state.policy.critical_auto_update;

    // 1. Blackout check (critical updates bypass blackouts).
    if (!is_critical && isInBlackout(state.blackouts, now)) {
        LOG_DEBUG("TenantUpdateScheduler: '{}' is in a blackout period", tenant_id);
        return false;
    }

    // 2. Maintenance window check (critical updates bypass window restriction).
    if (!is_critical) {
        if (!state.window.has_value()) {
            LOG_DEBUG("TenantUpdateScheduler: '{}' has no maintenance window",
                      tenant_id);
            return false;
        }
        if (!isInWindow(*state.window, now)) {
            LOG_DEBUG("TenantUpdateScheduler: '{}' is outside its maintenance window",
                      tenant_id);
            return false;
        }
    }

    // 3. Notification lead time.
    if (state.policy.notification_lead_time.count() > 0) {
        if (!state.notification_time.has_value()) {
            LOG_DEBUG("TenantUpdateScheduler: '{}' requires lead-time notification "
                      "but none recorded",
                      tenant_id);
            return false;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::hours>(
            now - *state.notification_time);
        if (elapsed < state.policy.notification_lead_time) {
            LOG_DEBUG("TenantUpdateScheduler: '{}' lead time not elapsed "
                      "({}h / {}h required)",
                      tenant_id,
                      elapsed.count(),
                      state.policy.notification_lead_time.count());
            return false;
        }
    }

    // 4. Consent check.
    // CRITICAL updates with critical_auto_update bypass consent.
    if (!is_critical && !state.policy.auto_update && !state.consent_granted) {
        LOG_DEBUG("TenantUpdateScheduler: '{}' requires manual consent", tenant_id);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------

namespace {
// Case-fold a string to ASCII lowercase (safe for non-ASCII characters).
inline std::string toLowerAscii(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
} // namespace

std::string
TenantUpdateScheduler::getNextMaintenanceWindow(const std::string& tenant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end() || !it->second.window.has_value()) {
        return "";
    }
    const MaintenanceWindow& win = *it->second.window;
    const int start_min = parseMinutes(win.start_time);
    if (start_min < 0) {
        return "";
    }

    const auto now = clock_fn_();

    // Search up to 7 * 24 * 60 minutes = 10 080 minutes ahead.
    // Step in 1-minute increments over the "start-of-window" candidates
    // (one per qualifying day per week).
    // For efficiency we advance day by day and check if the start time on
    // that day is in the future and the day is valid.

    // Convert now to UTC broken-down time.
    const std::time_t now_t =
        std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
#ifdef _WIN32
    gmtime_s(&utc_tm, &now_t);
#else
    gmtime_r(&now_t, &utc_tm);
#endif

    // Current minute-of-day.
    const int current_min_of_day =
        utc_tm.tm_hour * 60 + utc_tm.tm_min;

    // Day-of-week names (tm_wday: 0=Sunday).
    static const char* const kDayNames[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };

    // Helper: does the window include a given day name?
    auto dayAllowed = [&]([[maybe_unused]] int wday) -> bool {
        for (const auto& d : win.days) {
            if (toLowerAscii(d) == "daily") {
                return true;
            }
            if (toLowerAscii(d) == toLowerAscii(kDayNames[wday])) {
                return true;
            }
        }
        return false;
    };

    // Search day-by-day for up to 7 days.
    for (int offset = 0; offset <= 7; ++offset) {
        const int wday = (utc_tm.tm_wday + offset) % 7;
        if (!dayAllowed(wday)) {
            continue;
        }
        // The candidate start time is offset days from now, at start_min.
        if (offset == 0 && start_min <= current_min_of_day) {
            // Window start already passed today; skip to next occurrence.
            continue;
        }

        // Build the candidate time_point: midnight of (now + offset days)
        // plus start_min minutes.
        // CRITICAL: multiplication_overflow fix - use int64_t for intermediate calculations
        const int64_t hour_seconds = static_cast<int64_t>(utc_tm.tm_hour) * 3600;
        const int64_t min_seconds = static_cast<int64_t>(utc_tm.tm_min) * 60;
        const int64_t sec_seconds = static_cast<int64_t>(utc_tm.tm_sec);
        const int64_t day_offset_seconds = static_cast<int64_t>(offset) * 24 * 3600;
        const int64_t start_min_seconds = static_cast<int64_t>(start_min) * 60;
        
        const std::time_t midnight_t =
            now_t - static_cast<std::time_t>(hour_seconds + min_seconds + sec_seconds);
        const std::time_t candidate_t =
            midnight_t
            + static_cast<std::time_t>(day_offset_seconds)  // advance days
            + static_cast<std::time_t>(start_min_seconds);   // add start time

        const auto candidate_tp =
            std::chrono::system_clock::from_time_t(candidate_t);

        return formatUtc(candidate_tp);
    }
    return "";
}

// ---------------------------------------------------------------------------
// Update application & rollback
// ---------------------------------------------------------------------------

ReloadResult TenantUpdateScheduler::applyUpdate(const std::string& tenant_id,
                                                  const std::string& version,
                                                  HotReloadEngine& engine,
                                                  UpdatePriority priority)
{
    // canUpdateNow acquires the mutex; we must not hold it while calling
    // the engine (which may block).
    if (!canUpdateNow(tenant_id, priority)) {
        ReloadResult r;
        r.success = false;
        r.error_message =
            "TenantUpdateScheduler: update not allowed for tenant '" +
            tenant_id + "' at this time";
        LOG_WARN("TenantUpdateScheduler: blocked update for '{}' version '{}'",
                 tenant_id, version);
        return r;
    }

    // Version validation: when manual approval is required, confirm that the
    // version being applied matches the version for which consent was granted.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tenants_.find(tenant_id);
        if (it != tenants_.end()) {
            const TenantState& state = it->second;
            const bool is_critical =
                (priority == UpdatePriority::CRITICAL) &&
                state.policy.critical_auto_update;
            if (!is_critical && !state.policy.auto_update && state.consent_granted) {
                if (!state.pending_version.empty() &&
                    state.pending_version != version) {
                    ReloadResult r;
                    r.success = false;
                    r.error_message =
                        "TenantUpdateScheduler: consent version mismatch for "
                        "tenant '" + tenant_id + "': consented to '" +
                        state.pending_version + "' but requested '" + version + "'";
                    LOG_WARN("TenantUpdateScheduler: version mismatch blocked "
                             "update for '{}': consented='{}' requested='{}'",
                             tenant_id, state.pending_version, version);
                    return r;
                }
            }
        }
    }

    ReloadResult result = engine.applyHotReload(version);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tenants_.find(tenant_id);
        if (it == tenants_.end()) {
            // Tenant was removed concurrently while the engine was running.
            LOG_WARN("TenantUpdateScheduler: tenant '{}' removed during apply; "
                     "ignoring result",
                     tenant_id);
            return result;
        }
        TenantState& state = it->second;
        if (result.success) {
            state.current_version  = version;
            state.last_rollback_id = result.rollback_id;
            state.consent_granted  = false; // reset consent after successful apply
            state.pending_version.clear();
            state.notification_time.reset();
            LOG_INFO("TenantUpdateScheduler: applied version '{}' for tenant '{}' "
                     "(rollback_id='{}')",
                     version, tenant_id, result.rollback_id);
        } else {
            LOG_WARN("TenantUpdateScheduler: engine failed to apply '{}' for '{}': {}",
                     version, tenant_id, result.error_message);
        }
    }
    return result;
}

bool TenantUpdateScheduler::rollbackTenant(const std::string& tenant_id,
                                            HotReloadEngine& engine)
{
    std::string rollback_id = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tenants_.find(tenant_id);
        if (it == tenants_.end() || it->second.last_rollback_id.empty()) {
            LOG_INFO("TenantUpdateScheduler: no rollback state for '{}'", tenant_id);
            return true; // nothing to roll back
        }
        rollback_id = it->second.last_rollback_id;
    }

    const bool ok = engine.rollback(rollback_id);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tenants_.find(tenant_id);
        if (it != tenants_.end()) {
            if (ok) {
                it->second.last_rollback_id.clear();
                LOG_INFO("TenantUpdateScheduler: rollback succeeded for '{}'",
                         tenant_id);
            } else {
                LOG_WARN("TenantUpdateScheduler: rollback FAILED for '{}'",
                         tenant_id);
            }
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

std::optional<TenantUpdateStatus>
TenantUpdateScheduler::getTenantStatus(const std::string& tenant_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return std::nullopt;
    }
    const TenantState& state = it->second;
    const auto now = clock_fn_();

    TenantUpdateStatus s;
    s.tenant_id             = tenant_id;
    s.current_version       = state.current_version;
    s.pending_version       = state.pending_version;
    s.last_rollback_id      = state.last_rollback_id;
    s.consent_granted       = state.consent_granted;
    s.in_blackout_period    = isInBlackout(state.blackouts, now);
    s.in_maintenance_window = state.window.has_value() &&
                              isInWindow(*state.window, now);
    return s;
}

std::vector<TenantUpdateStatus>
TenantUpdateScheduler::getAllTenantStatuses() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = clock_fn_();
    std::vector<TenantUpdateStatus> result = {};

    result.reserve(tenants_.size());
    for (const auto& [tid, state] : tenants_) {
        TenantUpdateStatus s;
        s.tenant_id             = tid;
        s.current_version       = state.current_version;
        s.pending_version       = state.pending_version;
        s.last_rollback_id      = state.last_rollback_id;
        s.consent_granted       = state.consent_granted;
        s.in_blackout_period    = isInBlackout(state.blackouts, now);
        s.in_maintenance_window = state.window.has_value() &&
                                  isInWindow(*state.window, now);
        result.push_back(std::move(s));
    }
    return result;
}

void TenantUpdateScheduler::removeTenant(const std::string& tenant_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    tenants_.erase(tenant_id);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int TenantUpdateScheduler::parseMinutes(const std::string& hhmm)
{
    if (static_cast<int>(hhmm.size()) != 5 || hhmm[2] != ':') {
        return -1;
    }
    try {
        const int hh = std::stoi(hhmm.substr(0, 2));
        const int mm = std::stoi(hhmm.substr(3, 2));
        if (hh < 0 || hh > 23 || mm < 0 || mm > 59) {
            return -1;
        }
        return hh * 60 + mm;
    } catch (...) {
        return -1;
    }
}

bool TenantUpdateScheduler::isInWindow(const MaintenanceWindow& win,
                                        std::chrono::system_clock::time_point tp)
{
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif

    const int start_min = parseMinutes(win.start_time);
    const int end_min   = parseMinutes(win.end_time);
    if (start_min < 0 || end_min < 0) {
        return false;
    }

    const int cur_min = utc.tm_hour * 60 + utc.tm_min;

    // Determine whether we are in the time range and which calendar day to
    // test against the allowed-days list.
    //
    // For same-day windows (start < end, e.g., 02:00 – 06:00):
    //   - The window day is today.
    //
    // For cross-midnight windows (start > end, e.g., 23:00 – 05:00):
    //   - Before midnight (cur_min >= start_min): the window started today.
    //   - After midnight  (cur_min < end_min):    the window started yesterday.
    int  check_wday = {};
    bool in_time_range = 0;

    if (start_min <= end_min) {
        // Same-day window.
        in_time_range = (cur_min >= start_min && cur_min < end_min);
        check_wday    = utc.tm_wday;
    } else {
        // Cross-midnight window.
        if (cur_min >= start_min) {
            // Before-midnight portion: window started on today.
            in_time_range = true;
            check_wday    = utc.tm_wday;
        } else if (cur_min < end_min) {
            // After-midnight portion: window started on the previous calendar day.
            in_time_range = true;
            check_wday    = (utc.tm_wday + 6) % 7;
        } else {
            in_time_range = false;
            check_wday    = utc.tm_wday;
        }
    }

    if (!in_time_range) {
        return false;
    }

    // Day-of-week check (tm_wday: 0=Sunday).
    static const char* const kDayNames[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };
    for (const auto& d : win.days) {
        if (toLowerAscii(d) == "daily") {
            return true;
        }
        if (toLowerAscii(d) == toLowerAscii(kDayNames[check_wday])) {
            return true;
        }
    }
    return false;
}

bool TenantUpdateScheduler::isInBlackout(
    const std::vector<BlackoutPeriod>& blackouts,
    std::chrono::system_clock::time_point tp)
{
    for (const auto& b : blackouts) {
        if (tp >= b.start && tp < b.end) {
            return true;
        }
    }
    return false;
}

std::string
TenantUpdateScheduler::formatUtc(std::chrono::system_clock::time_point tp)
{
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    std::ostringstream ss = {};
    ss << std::setfill('0')
       << std::setw(4) << (utc.tm_year + 1900) << '-'
       << std::setw(2) << (utc.tm_mon + 1)     << '-'
       << std::setw(2) << utc.tm_mday           << 'T'
       << std::setw(2) << utc.tm_hour           << ':'
       << std::setw(2) << utc.tm_min            << ':'
       << std::setw(2) << utc.tm_sec            << 'Z';
    return ss.str();
}

} // namespace updates
} // namespace themis
