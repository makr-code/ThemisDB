// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_maintenance_contract_hardening_focused.cpp
 * @brief Phase 4 focused contract-hardening tests for the maintenance module.
 *
 * Test IDs: MTN-01 through MTN-08
 * No file I/O, no network, deterministic only.
 *
 * @see include/maintenance/maintenance_api_contract.h
 * @see src/maintenance/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "maintenance/maintenance_api_contract.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace themis {
namespace maintenance {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// MTN-01 — Error code uniqueness
// ============================================================================

TEST(MaintenanceContractHardening, MTN01_ErrorCodeUniqueness) {
    // All non-zero error codes must be distinct.
    std::set<int32_t> seen = {};

    const int32_t codes[] = {
        static_cast<int32_t>(MaintenanceError::kScheduleNotFound),
        static_cast<int32_t>(MaintenanceError::kHandlerNotRegistered),
        static_cast<int32_t>(MaintenanceError::kPersistenceFailed),
        static_cast<int32_t>(MaintenanceError::kExecutionTimeout),
        static_cast<int32_t>(MaintenanceError::kConcurrentModification),
        static_cast<int32_t>(MaintenanceError::kInvalidSchedule),
        static_cast<int32_t>(MaintenanceError::kOrchestratorDegraded),
        static_cast<int32_t>(MaintenanceError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_TRUE(seen.insert(c).second) << "Duplicate error code: " << c;
    }
    EXPECT_EQ(seen.size(), 8u);
    (void)kSeed; // suppress unused warning
}

// ============================================================================
// MTN-02 — Error code range [8100, 8199]
// ============================================================================

TEST(MaintenanceContractHardening, MTN02_ErrorCodeRange) {
    const int32_t codes[] = {
        static_cast<int32_t>(MaintenanceError::kScheduleNotFound),
        static_cast<int32_t>(MaintenanceError::kHandlerNotRegistered),
        static_cast<int32_t>(MaintenanceError::kPersistenceFailed),
        static_cast<int32_t>(MaintenanceError::kExecutionTimeout),
        static_cast<int32_t>(MaintenanceError::kConcurrentModification),
        static_cast<int32_t>(MaintenanceError::kInvalidSchedule),
        static_cast<int32_t>(MaintenanceError::kOrchestratorDegraded),
        static_cast<int32_t>(MaintenanceError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_GE(c, 8100) << "Code " << c << " below reserved base 8100";
        EXPECT_LE(c, 8199) << "Code " << c << " above reserved max 8199";
    }
}

// ============================================================================
// MTN-03 — Switch dispatch: all cases must be handled
// ============================================================================

TEST(MaintenanceContractHardening, MTN03_SwitchDispatch) {
    auto describe = [](MaintenanceError e) -> const char* {
        switch (e) {
            case MaintenanceError::kSuccess:               return "success";
            case MaintenanceError::kScheduleNotFound:      return "schedule_not_found";
            case MaintenanceError::kHandlerNotRegistered:  return "handler_not_registered";
            case MaintenanceError::kPersistenceFailed:     return "persistence_failed";
            case MaintenanceError::kExecutionTimeout:      return "execution_timeout";
            case MaintenanceError::kConcurrentModification: return "concurrent_modification";
            case MaintenanceError::kInvalidSchedule:       return "invalid_schedule";
            case MaintenanceError::kOrchestratorDegraded:  return "orchestrator_degraded";
            case MaintenanceError::kInternalError:         return "internal_error";
        }
        return "unknown";
    };

    EXPECT_STREQ(describe(MaintenanceError::kSuccess),              "success");
    EXPECT_STREQ(describe(MaintenanceError::kScheduleNotFound),     "schedule_not_found");
    EXPECT_STREQ(describe(MaintenanceError::kHandlerNotRegistered), "handler_not_registered");
    EXPECT_STREQ(describe(MaintenanceError::kOrchestratorDegraded), "orchestrator_degraded");
    EXPECT_STREQ(describe(MaintenanceError::kInternalError),        "internal_error");
}

// ============================================================================
// MTN-04 — MaintenanceScheduleDescriptor default values
// ============================================================================

TEST(MaintenanceContractHardening, MTN04_ScheduleDescriptorDefaults) {
    MaintenanceScheduleDescriptor desc;
    EXPECT_TRUE(desc.schedule_id.empty());
    EXPECT_TRUE(desc.task_type.empty());
    EXPECT_EQ(desc.interval.count(), 0);
    EXPECT_FALSE(desc.enabled);
    EXPECT_EQ(desc.timeout.count(), 0);
}

// ============================================================================
// MTN-05 — MaintenanceHealthSnapshot default values
// ============================================================================

TEST(MaintenanceContractHardening, MTN05_HealthSnapshotDefaults) {
    MaintenanceHealthSnapshot snap;
    EXPECT_FALSE(snap.is_healthy);
    EXPECT_EQ(snap.active_schedule_count, 0u);
    EXPECT_EQ(snap.failed_execution_count, 0u);
    EXPECT_TRUE(snap.last_error_message.empty());
}

// ============================================================================
// MTN-06 — Copy semantics for MaintenanceScheduleDescriptor
// ============================================================================

TEST(MaintenanceContractHardening, MTN06_ScheduleDescriptorCopy) {
    MaintenanceScheduleDescriptor src;
    src.schedule_id = "sched-42";
    src.task_type   = "compaction";
    src.interval    = std::chrono::seconds{3600};
    src.enabled     = true;

    MaintenanceScheduleDescriptor copy = src;
    EXPECT_EQ(copy.schedule_id, src.schedule_id);
    EXPECT_EQ(copy.task_type,   src.task_type);
    EXPECT_EQ(copy.interval,    src.interval);
    EXPECT_EQ(copy.enabled,     src.enabled);
}

// ============================================================================
// MTN-07 — Move semantics for MaintenanceScheduleDescriptor
// ============================================================================

TEST(MaintenanceContractHardening, MTN07_ScheduleDescriptorMove) {
    MaintenanceScheduleDescriptor src;
    src.schedule_id = "sched-move";
    src.task_type   = "vacuum";

    MaintenanceScheduleDescriptor moved = std::move(src);
    EXPECT_EQ(moved.schedule_id, "sched-move");
    EXPECT_EQ(moved.task_type,   "vacuum");
}

// ============================================================================
// MTN-08 — isMaintenanceFailClosed predicate
// ============================================================================

TEST(MaintenanceContractHardening, MTN08_FailClosedPredicate) {
    // Must be fail-closed.
    EXPECT_TRUE(isMaintenanceFailClosed(MaintenanceError::kOrchestratorDegraded));
    EXPECT_TRUE(isMaintenanceFailClosed(MaintenanceError::kInternalError));
    EXPECT_TRUE(isMaintenanceFailClosed(MaintenanceError::kPersistenceFailed));

    // Must NOT be fail-closed.
    EXPECT_FALSE(isMaintenanceFailClosed(MaintenanceError::kSuccess));
    EXPECT_FALSE(isMaintenanceFailClosed(MaintenanceError::kScheduleNotFound));
    EXPECT_FALSE(isMaintenanceFailClosed(MaintenanceError::kHandlerNotRegistered));
    EXPECT_FALSE(isMaintenanceFailClosed(MaintenanceError::kExecutionTimeout));
    EXPECT_FALSE(isMaintenanceFailClosed(MaintenanceError::kConcurrentModification));
    EXPECT_FALSE(isMaintenanceFailClosed(MaintenanceError::kInvalidSchedule));
}

} // namespace test
} // namespace maintenance
} // namespace themis
