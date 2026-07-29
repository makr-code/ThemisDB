// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scheduler_contract_hardening_focused.cpp
 * @brief Phase 4 focused contract-hardening tests for the scheduler module.
 *
 * Test IDs: SCH-01 through SCH-08
 * No file I/O, no network, deterministic only.
 *
 * @see include/scheduler/scheduler_api_contract.h
 * @see src/scheduler/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "scheduler/scheduler_api_contract.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace themis {
namespace scheduler {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// SCH-01 — Error code uniqueness
// ============================================================================

TEST(SchedulerContractHardening, SCH01_ErrorCodeUniqueness) {
    std::set<int32_t> seen;
    const int32_t codes[] = {
        static_cast<int32_t>(SchedulerError::kTaskNotFound),
        static_cast<int32_t>(SchedulerError::kTaskAlreadyExists),
        static_cast<int32_t>(SchedulerError::kExecutionFailed),
        static_cast<int32_t>(SchedulerError::kCoordinationError),
        static_cast<int32_t>(SchedulerError::kRetentionLimitExceeded),
        static_cast<int32_t>(SchedulerError::kTriggerInvalid),
        static_cast<int32_t>(SchedulerError::kAnomalyDetected),
        static_cast<int32_t>(SchedulerError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_TRUE(seen.insert(c).second) << "Duplicate error code: " << c;
    }
    EXPECT_EQ(seen.size(), 8u);
    (void)kSeed;
}

// ============================================================================
// SCH-02 — Error code range [8400, 8499]
// ============================================================================

TEST(SchedulerContractHardening, SCH02_ErrorCodeRange) {
    const int32_t codes[] = {
        static_cast<int32_t>(SchedulerError::kTaskNotFound),
        static_cast<int32_t>(SchedulerError::kTaskAlreadyExists),
        static_cast<int32_t>(SchedulerError::kExecutionFailed),
        static_cast<int32_t>(SchedulerError::kCoordinationError),
        static_cast<int32_t>(SchedulerError::kRetentionLimitExceeded),
        static_cast<int32_t>(SchedulerError::kTriggerInvalid),
        static_cast<int32_t>(SchedulerError::kAnomalyDetected),
        static_cast<int32_t>(SchedulerError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_GE(c, 8400) << "Code " << c << " below reserved base 8400";
        EXPECT_LE(c, 8499) << "Code " << c << " above reserved max 8499";
    }
}

// ============================================================================
// SCH-03 — Switch dispatch: all cases must be handled
// ============================================================================

TEST(SchedulerContractHardening, SCH03_SwitchDispatch) {
    auto describe = [](SchedulerError e) -> const char* {
        switch (e) {
            case SchedulerError::kSuccess:                return "success";
            case SchedulerError::kTaskNotFound:           return "task_not_found";
            case SchedulerError::kTaskAlreadyExists:      return "task_already_exists";
            case SchedulerError::kExecutionFailed:        return "execution_failed";
            case SchedulerError::kCoordinationError:      return "coordination_error";
            case SchedulerError::kRetentionLimitExceeded: return "retention_limit_exceeded";
            case SchedulerError::kTriggerInvalid:         return "trigger_invalid";
            case SchedulerError::kAnomalyDetected:        return "anomaly_detected";
            case SchedulerError::kInternalError:          return "internal_error";
        }
        return "unknown";
    };

    EXPECT_STREQ(describe(SchedulerError::kSuccess),                "success");
    EXPECT_STREQ(describe(SchedulerError::kTaskNotFound),           "task_not_found");
    EXPECT_STREQ(describe(SchedulerError::kRetentionLimitExceeded), "retention_limit_exceeded");
    EXPECT_STREQ(describe(SchedulerError::kInternalError),          "internal_error");
}

// ============================================================================
// SCH-04 — TaskRegistrationDescriptor default values
// ============================================================================

TEST(SchedulerContractHardening, SCH04_TaskRegistrationDescriptorDefaults) {
    TaskRegistrationDescriptor desc;
    EXPECT_TRUE(desc.task_id.empty());
    EXPECT_TRUE(desc.task_name.empty());
    EXPECT_EQ(desc.execution_timeout.count(), 600);
    EXPECT_FALSE(desc.allow_concurrent);
    EXPECT_EQ(desc.retention_limit,
              static_cast<uint32_t>(kDefaultRetentionLimit));
}

// ============================================================================
// SCH-05 — TaskExecutionResult default values
// ============================================================================

TEST(SchedulerContractHardening, SCH05_TaskExecutionResultDefaults) {
    TaskExecutionResult result;
    EXPECT_TRUE(result.task_id.empty());
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.exit_code, 0);
    EXPECT_TRUE(result.message.empty());
    EXPECT_EQ(result.duration_ms.count(), 0);
}

// ============================================================================
// SCH-06 — Copy semantics for TaskRegistrationDescriptor
// ============================================================================

TEST(SchedulerContractHardening, SCH06_TaskRegistrationDescriptorCopy) {
    TaskRegistrationDescriptor src;
    src.task_id   = "task-42";
    src.task_name = "Compaction";
    src.allow_concurrent = false;

    TaskRegistrationDescriptor copy = src;
    EXPECT_EQ(copy.task_id,          src.task_id);
    EXPECT_EQ(copy.task_name,        src.task_name);
    EXPECT_EQ(copy.allow_concurrent, src.allow_concurrent);
}

// ============================================================================
// SCH-07 — Move semantics for TaskExecutionResult
// ============================================================================

TEST(SchedulerContractHardening, SCH07_TaskExecutionResultMove) {
    TaskExecutionResult src;
    src.task_id = "task-move";
    src.success = true;
    src.message = "ok";

    TaskExecutionResult moved = std::move(src);
    EXPECT_EQ(moved.task_id, "task-move");
    EXPECT_TRUE(moved.success);
    EXPECT_EQ(moved.message, "ok");
}

// ============================================================================
// SCH-08 — isSchedulerFailClosed predicate
// ============================================================================

TEST(SchedulerContractHardening, SCH08_FailClosedPredicate) {
    // Must be fail-closed.
    EXPECT_TRUE(isSchedulerFailClosed(SchedulerError::kCoordinationError));
    EXPECT_TRUE(isSchedulerFailClosed(SchedulerError::kInternalError));

    // Must NOT be fail-closed.
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kSuccess));
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kTaskNotFound));
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kTaskAlreadyExists));
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kExecutionFailed));
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kRetentionLimitExceeded));
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kTriggerInvalid));
    EXPECT_FALSE(isSchedulerFailClosed(SchedulerError::kAnomalyDetected));
}

} // namespace test
} // namespace scheduler
} // namespace themis
