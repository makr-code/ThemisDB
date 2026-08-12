// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_multi_tenant_update_scheduling.cpp
 * @brief Focused unit tests for TenantUpdateScheduler (Issue #262 / v1.8.0)
 *
 * Acceptance criteria covered:
 *   AC1 – Tenant-specific maintenance windows
 *   AC2 – Update blackout periods
 *   AC3 – Priority tiers (critical, normal, low)
 *   AC4 – Tenant consent for updates
 *   AC5 – Rollback per tenant
 */

#include <gtest/gtest.h>
#include "updates/tenant_update_scheduler.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

using namespace themis::updates;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// A minimal stub of HotReloadEngine used in rollback / applyUpdate tests.
class StubEngine : public HotReloadEngine {
public:
    explicit StubEngine(bool apply_ok = true, bool rollback_ok = true)
        : HotReloadEngine(nullptr, nullptr,
                           []() {
                               HotReloadEngine::Config c;
                               c.download_directory = "/tmp/stub_mt_dl";
                               c.backup_directory   = "/tmp/stub_mt_bak";
                               c.verify_signatures  = false;
                               c.create_backup      = false;
                               return c;
                           }())
        , apply_ok_(apply_ok)
        , rollback_ok_(rollback_ok)
    {}

    ReloadResult applyHotReload(const std::string& version,
                                bool /*verify_only*/ = false) override {
        ++apply_count;
        ReloadResult r;
        r.success     = apply_ok_;
        r.rollback_id = "stub_rid_" + version;
        if (!apply_ok_) {
            r.error_message = "stub: apply failed";
        }
        return r;
    }

    bool rollback(const std::string& /*rid*/) override {
        ++rollback_count;
        return rollback_ok_;
    }

    std::atomic<int> apply_count{0};
    std::atomic<int> rollback_count{0};

private:
    bool apply_ok_;
    bool rollback_ok_;
};

/// Return a fixed UTC time point built from broken-down components.
/// day_of_week is 0=Sunday, but is ignored by from_time_t as long as we
/// set the other fields correctly (tm_wday is informational in mktime but
/// we use gmtime path in the scheduler, so we pass it explicitly).
static std::chrono::system_clock::time_point makeUtcTime(
    int year, int month, int day,
    int hour, int minute, int second = 0)
{
    std::tm t{};
    t.tm_year  = year - 1900;
    t.tm_mon   = month - 1;
    t.tm_mday  = day;
    t.tm_hour  = hour;
    t.tm_min   = minute;
    t.tm_sec   = second;
    t.tm_isdst = 0;
    // Use timegm / _mkgmtime for UTC interpretation.
#ifdef _WIN32
    const std::time_t tt = _mkgmtime(&t);
#else
    const std::time_t tt = timegm(&t);
#endif
    return std::chrono::system_clock::from_time_t(tt);
}

} // namespace

// ===========================================================================
// AC1 – Maintenance windows
// ===========================================================================

class MaintenanceWindowTest : public ::testing::Test {};

// A scheduler whose clock is fixed to Saturday 03:00 UTC.
// tenant-123 has a Saturday/Sunday 02:00–06:00 window → should be IN window.
TEST_F(MaintenanceWindowTest, InWindow_Saturday_0300_AllowsUpdate) {
    const auto fixed = makeUtcTime(2026, 3, 14, 3, 0); // 2026-03-14 is Saturday
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Saturday", "Sunday"},
        .start_time = "02:00",
        .end_time   = "06:00",
        .timezone   = "UTC"
    });

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, OutsideWindow_Monday_0300_BlocksUpdate) {
    const auto fixed = makeUtcTime(2026, 3, 16, 3, 0); // 2026-03-16 is Monday
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Saturday", "Sunday"},
        .start_time = "02:00",
        .end_time   = "06:00",
        .timezone   = "UTC"
    });

    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, DailyWindow_AnyDay_Allows) {
    const auto fixed = makeUtcTime(2026, 3, 16, 0, 30); // Monday 00:30
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Daily"},
        .start_time = "00:00",
        .end_time   = "06:00",
        .timezone   = "UTC"
    });

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, CrossMidnightWindow_BeforeMidnight_Allows) {
    const auto fixed = makeUtcTime(2026, 3, 14, 23, 30); // Saturday 23:30
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Saturday"},
        .start_time = "23:00",
        .end_time   = "05:00",
        .timezone   = "UTC"
    });

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, CrossMidnightWindow_AfterMidnight_Allows) {
    const auto fixed = makeUtcTime(2026, 3, 15, 2, 0); // Sunday 02:00
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Saturday"},
        .start_time = "23:00",
        .end_time   = "05:00",
        .timezone   = "UTC"
    });

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, CrossMidnightWindow_Outside_Blocks) {
    const auto fixed = makeUtcTime(2026, 3, 14, 12, 0); // Saturday 12:00
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Saturday"},
        .start_time = "23:00",
        .end_time   = "05:00",
        .timezone   = "UTC"
    });

    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, NoWindowConfigured_Blocks) {
    TenantUpdateScheduler sched;
    // Tenant exists but has no window.
    sched.setUpdatePolicy("t1", {});
    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, UnknownTenant_Blocks) {
    TenantUpdateScheduler sched;
    EXPECT_FALSE(sched.canUpdateNow("nobody"));
}

TEST_F(MaintenanceWindowTest, RemoveWindow_Blocks) {
    const auto fixed = makeUtcTime(2026, 3, 14, 3, 0); // Saturday 03:00
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Saturday"},
        .start_time = "02:00",
        .end_time   = "06:00",
        .timezone   = "UTC"
    });
    ASSERT_TRUE(sched.canUpdateNow("t1"));

    sched.removeMaintenanceWindow("t1");
    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(MaintenanceWindowTest, GetMaintenanceWindow_RoundTrip) {
    TenantUpdateScheduler sched;
    MaintenanceWindow win;
    win.days       = {"Monday", "Wednesday"};
    win.start_time = "01:00";
    win.end_time   = "03:00";
    win.timezone   = "Europe/London";

    sched.setMaintenanceWindow("t1", win);
    auto got = sched.getMaintenanceWindow("t1");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->days,       win.days);
    EXPECT_EQ(got->start_time, win.start_time);
    EXPECT_EQ(got->end_time,   win.end_time);
    EXPECT_EQ(got->timezone,   win.timezone);
}

TEST_F(MaintenanceWindowTest, GetNextMaintenanceWindow_FutureWeekday) {
    // Clock: Saturday 2026-03-14 10:00 UTC.
    // Window: Monday 01:00–03:00.
    // Next occurrence: Monday 2026-03-16 01:00 UTC.
    const auto fixed = makeUtcTime(2026, 3, 14, 10, 0);
    TenantUpdateScheduler sched([&]() { return fixed; });

    sched.setMaintenanceWindow("t1", {
        .days       = {"Monday"},
        .start_time = "01:00",
        .end_time   = "03:00",
        .timezone   = "UTC"
    });

    const std::string next = sched.getNextMaintenanceWindow("t1");
    EXPECT_FALSE(next.empty());
    // Must contain "2026-03-16T01:00"
    EXPECT_NE(next.find("2026-03-16T01:00"), std::string::npos) << "next=" << next;
}

TEST_F(MaintenanceWindowTest, GetNextMaintenanceWindow_NoWindow_ReturnsEmpty) {
    TenantUpdateScheduler sched;
    EXPECT_EQ(sched.getNextMaintenanceWindow("t1"), "");
}

// ===========================================================================
// AC2 – Blackout periods
// ===========================================================================

class BlackoutPeriodTest : public ::testing::Test {};

TEST_F(BlackoutPeriodTest, ActiveBlackout_Blocks_NormalUpdate) {
    const auto now = makeUtcTime(2026, 3, 14, 3, 0); // Saturday 03:00
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.addBlackoutPeriod("t1", {
        .id    = "bo1",
        .start = makeUtcTime(2026, 3, 14, 0, 0),
        .end   = makeUtcTime(2026, 3, 15, 0, 0),
        .reason = "Year-end freeze"
    });

    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(BlackoutPeriodTest, ExpiredBlackout_DoesNotBlock) {
    const auto now = makeUtcTime(2026, 3, 14, 3, 0); // Saturday 03:00
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    // Blackout ended yesterday.
    sched.addBlackoutPeriod("t1", {
        .id    = "bo1",
        .start = makeUtcTime(2026, 3, 12, 0, 0),
        .end   = makeUtcTime(2026, 3, 13, 0, 0)
    });

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(BlackoutPeriodTest, RemoveBlackout_UnblocksUpdate) {
    const auto now = makeUtcTime(2026, 3, 14, 3, 0);
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.addBlackoutPeriod("t1", {
        .id    = "bo1",
        .start = makeUtcTime(2026, 3, 14, 0, 0),
        .end   = makeUtcTime(2026, 3, 15, 0, 0)
    });
    ASSERT_FALSE(sched.canUpdateNow("t1"));

    EXPECT_TRUE(sched.removeBlackoutPeriod("t1", "bo1"));
    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(BlackoutPeriodTest, RemoveNonExistentBlackout_ReturnsFalse) {
    TenantUpdateScheduler sched;
    EXPECT_FALSE(sched.removeBlackoutPeriod("t1", "no-such-id"));
}

TEST_F(BlackoutPeriodTest, GetBlackoutPeriods_RoundTrip) {
    TenantUpdateScheduler sched;
    BlackoutPeriod b;
    b.id     = "bo1";
    b.start  = makeUtcTime(2026, 3, 14, 0, 0);
    b.end    = makeUtcTime(2026, 3, 15, 0, 0);
    b.reason = "Q1 freeze";
    sched.addBlackoutPeriod("t1", b);

    auto periods = sched.getBlackoutPeriods("t1");
    ASSERT_EQ(periods.size(), 1u);
    EXPECT_EQ(periods[0].id,     b.id);
    EXPECT_EQ(periods[0].reason, b.reason);
}

TEST_F(BlackoutPeriodTest, MultipleBlackouts_AllActiveBlocksTenant) {
    const auto now = makeUtcTime(2026, 3, 14, 3, 0);
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    // One active, one expired.
    sched.addBlackoutPeriod("t1", {
        .id = "bo_expired",
        .start = makeUtcTime(2026, 3, 12, 0, 0),
        .end   = makeUtcTime(2026, 3, 13, 0, 0)
    });
    sched.addBlackoutPeriod("t1", {
        .id = "bo_active",
        .start = makeUtcTime(2026, 3, 14, 0, 0),
        .end   = makeUtcTime(2026, 3, 15, 0, 0)
    });

    EXPECT_FALSE(sched.canUpdateNow("t1"));

    // Remove active blackout – should unblock.
    sched.removeBlackoutPeriod("t1", "bo_active");
    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

// ===========================================================================
// AC3 – Priority tiers
// ===========================================================================

class PriorityTierTest : public ::testing::Test {};

TEST_F(PriorityTierTest, Critical_WithCriticalAutoUpdate_BypassesBlackout) {
    const auto now = makeUtcTime(2026, 3, 14, 3, 0);
    TenantUpdateScheduler sched([&]() { return now; });

    // No maintenance window + active blackout.
    sched.addBlackoutPeriod("t1", {
        .id    = "bo1",
        .start = makeUtcTime(2026, 3, 14, 0, 0),
        .end   = makeUtcTime(2026, 3, 15, 0, 0)
    });
    sched.setUpdatePolicy("t1", {
        .auto_update          = false,
        .critical_auto_update = true
    });

    EXPECT_FALSE(sched.canUpdateNow("t1", UpdatePriority::NORMAL));
    EXPECT_TRUE(sched.canUpdateNow("t1", UpdatePriority::CRITICAL));
}

TEST_F(PriorityTierTest, Critical_WithCriticalAutoUpdate_BypassesWindow) {
    const auto now = makeUtcTime(2026, 3, 16, 12, 0); // Monday 12:00 - outside window
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {
        .auto_update          = false,
        .critical_auto_update = true
    });

    EXPECT_FALSE(sched.canUpdateNow("t1", UpdatePriority::NORMAL));
    EXPECT_TRUE(sched.canUpdateNow("t1", UpdatePriority::CRITICAL));
}

TEST_F(PriorityTierTest, Critical_WithoutCriticalAutoUpdate_DoesNotBypass) {
    const auto now = makeUtcTime(2026, 3, 16, 12, 0);
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {
        .auto_update          = false,
        .critical_auto_update = false // explicitly disabled
    });

    EXPECT_FALSE(sched.canUpdateNow("t1", UpdatePriority::CRITICAL));
}

TEST_F(PriorityTierTest, Low_FollowsNormalWindowRules) {
    const auto in_window = makeUtcTime(2026, 3, 14, 3, 0);
    TenantUpdateScheduler sched([&]() { return in_window; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    // LOW priority should be allowed during the window when auto_update is true.
    EXPECT_TRUE(sched.canUpdateNow("t1", UpdatePriority::LOW));
}

// ===========================================================================
// AC4 – Tenant consent
// ===========================================================================

class TenantConsentTest : public ::testing::Test {
protected:
    // Saturday 2026-03-14 03:00 UTC – inside Saturday 02:00-06:00 window.
    const std::chrono::system_clock::time_point now_{makeUtcTime(2026, 3, 14, 3, 0)};
};

TEST_F(TenantConsentTest, AutoUpdateTrue_NoConsentRequired) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {.auto_update = true});

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(TenantConsentTest, AutoUpdateFalse_BlocksWithoutConsent) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {.auto_update = false, .critical_auto_update = false});

    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(TenantConsentTest, GrantConsent_AllowsUpdate) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {.auto_update = false, .critical_auto_update = false});

    sched.recordNotification("t1", "1.8.0");
    ASSERT_TRUE(sched.grantConsent("t1", "1.8.0"));
    EXPECT_TRUE(sched.hasConsent("t1"));
    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(TenantConsentTest, GrantConsent_WrongVersion_Rejected) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.recordNotification("t1", "1.8.0");
    EXPECT_FALSE(sched.grantConsent("t1", "1.9.0"));
    EXPECT_FALSE(sched.hasConsent("t1"));
}

TEST_F(TenantConsentTest, RevokeConsent_BlocksUpdate) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {.auto_update = false, .critical_auto_update = false});

    sched.recordNotification("t1", "1.8.0");
    ASSERT_TRUE(sched.grantConsent("t1", "1.8.0"));
    ASSERT_TRUE(sched.canUpdateNow("t1"));

    sched.revokeConsent("t1");
    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(TenantConsentTest, RecordNotification_ResetsConsent) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.recordNotification("t1", "1.8.0");
    ASSERT_TRUE(sched.grantConsent("t1", "1.8.0"));
    ASSERT_TRUE(sched.hasConsent("t1"));

    // New notification resets consent.
    sched.recordNotification("t1", "1.9.0");
    EXPECT_FALSE(sched.hasConsent("t1"));
}

TEST_F(TenantConsentTest, NotificationLeadTime_NotElapsed_Blocks) {
    // Notification was recorded at now_; clock IS still now_.
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {
        .auto_update           = true,
        .notification_lead_time = std::chrono::hours(24)
    });

    sched.recordNotification("t1", "1.8.0");

    // 0 hours elapsed → blocked.
    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

TEST_F(TenantConsentTest, NotificationLeadTime_Elapsed_Allows) {
    // Notification was recorded 25 hours ago; clock is now_.
    const auto notif_time = now_ - std::chrono::hours(25);
    bool use_notif_time = false;
    auto clock = [&]() {
        return use_notif_time ? notif_time : now_;
    };

    TenantUpdateScheduler sched(clock);

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {
        .auto_update           = true,
        .notification_lead_time = std::chrono::hours(24)
    });

    use_notif_time = true;  // Record notification with past clock.
    sched.recordNotification("t1", "1.8.0");
    use_notif_time = false; // Switch clock back to "now" (25h later).

    EXPECT_TRUE(sched.canUpdateNow("t1"));
}

TEST_F(TenantConsentTest, NoNotificationRecorded_WhenLeadTimeRequired_Blocks) {
    TenantUpdateScheduler sched([&]() { return now_; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {
        .auto_update           = true,
        .notification_lead_time = std::chrono::hours(1)
    });

    // No recordNotification call → blocked.
    EXPECT_FALSE(sched.canUpdateNow("t1"));
}

// ===========================================================================
// AC5 – Rollback per tenant
// ===========================================================================

class PerTenantRollbackTest : public ::testing::Test {
protected:
    const std::chrono::system_clock::time_point now_{makeUtcTime(2026, 3, 14, 3, 0)};
};

TEST_F(PerTenantRollbackTest, ApplyUpdate_StoresRollbackId) {
    TenantUpdateScheduler sched([&]() { return now_; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    StubEngine engine;
    auto result = sched.applyUpdate("t1", "1.8.0", engine);
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.rollback_id, "stub_rid_1.8.0");

    auto status = sched.getTenantStatus("t1");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->current_version, "1.8.0");
    EXPECT_EQ(status->last_rollback_id, "stub_rid_1.8.0");
}

TEST_F(PerTenantRollbackTest, RollbackTenant_CallsEngine) {
    TenantUpdateScheduler sched([&]() { return now_; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    StubEngine engine;
    sched.applyUpdate("t1", "1.8.0", engine);
    ASSERT_EQ(engine.apply_count.load(), 1);

    bool ok = sched.rollbackTenant("t1", engine);
    EXPECT_TRUE(ok);
    EXPECT_EQ(engine.rollback_count.load(), 1);

    // Rollback ID is cleared after successful rollback.
    auto status = sched.getTenantStatus("t1");
    ASSERT_TRUE(status.has_value());
    EXPECT_TRUE(status->last_rollback_id.empty());
}

TEST_F(PerTenantRollbackTest, RollbackTenant_NoRollbackState_ReturnsTrue) {
    TenantUpdateScheduler sched;
    StubEngine engine;
    EXPECT_TRUE(sched.rollbackTenant("unknown-tenant", engine));
    EXPECT_EQ(engine.rollback_count.load(), 0);
}

TEST_F(PerTenantRollbackTest, RollbackTenant_EngineFailure_ReturnsFalse) {
    TenantUpdateScheduler sched([&]() { return now_; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    StubEngine engine(/*apply_ok=*/true, /*rollback_ok=*/false);
    sched.applyUpdate("t1", "1.8.0", engine);

    bool ok = sched.rollbackTenant("t1", engine);
    EXPECT_FALSE(ok);
}

TEST_F(PerTenantRollbackTest, ApplyUpdate_SchedulingBlocked_DoesNotCallEngine) {
    const auto outside_window = makeUtcTime(2026, 3, 16, 12, 0); // Monday
    TenantUpdateScheduler sched([&]() { return outside_window; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    StubEngine engine;
    auto result = sched.applyUpdate("t1", "1.8.0", engine);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_EQ(engine.apply_count.load(), 0);
}

TEST_F(PerTenantRollbackTest, ApplyUpdate_EngineFails_RollbackIdNotStored) {
    TenantUpdateScheduler sched([&]() { return now_; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    StubEngine engine(/*apply_ok=*/false);
    auto result = sched.applyUpdate("t1", "1.8.0", engine);
    EXPECT_FALSE(result.success);

    auto status = sched.getTenantStatus("t1");
    ASSERT_TRUE(status.has_value());
    EXPECT_TRUE(status->last_rollback_id.empty());
}

TEST_F(PerTenantRollbackTest, ApplyUpdate_ClearsConsentAfterSuccess) {
    TenantUpdateScheduler sched([&]() { return now_; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {.auto_update = false, .critical_auto_update = false});
    sched.recordNotification("t1", "1.8.0");
    ASSERT_TRUE(sched.grantConsent("t1", "1.8.0"));
    ASSERT_TRUE(sched.hasConsent("t1"));

    StubEngine engine;
    auto result = sched.applyUpdate("t1", "1.8.0", engine);
    ASSERT_TRUE(result.success);

    // Consent was consumed.
    EXPECT_FALSE(sched.hasConsent("t1"));
}

TEST_F(PerTenantRollbackTest, ApplyUpdate_ConsentVersionMismatch_Blocks) {
    // Consent was granted for 1.8.0 but caller attempts to apply 1.9.0.
    TenantUpdateScheduler sched([&]() { return now_; });
    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });
    sched.setUpdatePolicy("t1", {.auto_update = false, .critical_auto_update = false});
    sched.recordNotification("t1", "1.8.0");
    ASSERT_TRUE(sched.grantConsent("t1", "1.8.0"));

    StubEngine engine;
    auto result = sched.applyUpdate("t1", "1.9.0", engine);  // wrong version!
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_EQ(engine.apply_count.load(), 0);  // engine must not be called

    // Consent should remain (it was not consumed).
    EXPECT_TRUE(sched.hasConsent("t1"));
}

// ===========================================================================
// Status & lifecycle
// ===========================================================================

class TenantStatusTest : public ::testing::Test {};

TEST_F(TenantStatusTest, GetTenantStatus_UnknownTenant_NullOpt) {
    TenantUpdateScheduler sched;
    EXPECT_FALSE(sched.getTenantStatus("nobody").has_value());
}

TEST_F(TenantStatusTest, GetTenantStatus_InWindow_Reported) {
    const auto now = makeUtcTime(2026, 3, 14, 3, 0);
    TenantUpdateScheduler sched([&]() { return now; });

    sched.setMaintenanceWindow("t1", {
        .days = {"Saturday"}, .start_time = "02:00", .end_time = "06:00"
    });

    auto status = sched.getTenantStatus("t1");
    ASSERT_TRUE(status.has_value());
    EXPECT_TRUE(status->in_maintenance_window);
    EXPECT_FALSE(status->in_blackout_period);
}

TEST_F(TenantStatusTest, GetAllTenantStatuses_ReturnsAll) {
    TenantUpdateScheduler sched;
    sched.setMaintenanceWindow("t1", {.days = {"Daily"}, .start_time = "00:00", .end_time = "23:59"});
    sched.setMaintenanceWindow("t2", {.days = {"Daily"}, .start_time = "00:00", .end_time = "23:59"});

    auto statuses = sched.getAllTenantStatuses();
    EXPECT_EQ(statuses.size(), 2u);
}

TEST_F(TenantStatusTest, RemoveTenant_ClearsState) {
    TenantUpdateScheduler sched;
    sched.setMaintenanceWindow("t1", {.days = {"Daily"}, .start_time = "00:00", .end_time = "23:59"});
    ASSERT_TRUE(sched.getTenantStatus("t1").has_value());

    sched.removeTenant("t1");
    EXPECT_FALSE(sched.getTenantStatus("t1").has_value());
    EXPECT_EQ(sched.getAllTenantStatuses().size(), 0u);
}

TEST_F(TenantStatusTest, GetUpdatePolicy_DefaultForUnknownTenant) {
    TenantUpdateScheduler sched;
    const auto p = sched.getUpdatePolicy("nobody");
    EXPECT_TRUE(p.auto_update);
    EXPECT_TRUE(p.critical_auto_update);
    EXPECT_EQ(p.notification_lead_time.count(), 0);
}

// ===========================================================================
// Construction
// ===========================================================================

TEST(TenantUpdateSchedulerConstructionTest, NullClock_Throws) {
    EXPECT_THROW(TenantUpdateScheduler(nullptr), std::invalid_argument);
}

TEST(TenantUpdateSchedulerConstructionTest, DefaultConstruct_DoesNotThrow) {
    EXPECT_NO_THROW(TenantUpdateScheduler());
}
