// SPDX-License-Identifier: Apache-2.0

/**
 * @file test_scheduler_contract_hardening_focused.cpp
 * @brief Focused contract tests for scheduler API error enums.
 */

#include "gtest/gtest.h"
#include "scheduler/scheduler_api_contract.h"

#include <set>

namespace themis::scheduler::test {

TEST(SchedulerContractHardening, ErrorCodeUniqueness) {
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
        EXPECT_TRUE(seen.insert(c).second);
    }
    EXPECT_EQ(seen.size(), 8u);
}

TEST(SchedulerContractHardening, ErrorCodeRange) {
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
        EXPECT_GE(c, 8400);
        EXPECT_LE(c, 8499);
    }
}

}  // namespace themis::scheduler::test
