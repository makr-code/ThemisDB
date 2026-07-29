/**
 * @file test_process_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the process module.
 * @note Test IDs: PRC-01..PRC-08
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace themis::process;

TEST(ProcessContractTest, PRC01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(ProcError::kUnsupportedElement),
        static_cast<int32_t>(ProcError::kInvalidTransition),
        static_cast<int32_t>(ProcError::kSerialiserFailed),
        static_cast<int32_t>(ProcError::kDeserialiserFailed),
        static_cast<int32_t>(ProcError::kExecutionTimeout),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

TEST(ProcessContractTest, PRC02_ErrorCodesInRange) {
    auto check = [](ProcError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7600); EXPECT_LE(v, 7699);
    };
    check(ProcError::kUnsupportedElement);
    check(ProcError::kInvalidTransition);
    check(ProcError::kSerialiserFailed);
    check(ProcError::kDeserialiserFailed);
    check(ProcError::kExecutionTimeout);
}

TEST(ProcessContractTest, PRC03_UnsupportedDistinctFromInvalidTransition) {
    EXPECT_NE(static_cast<int32_t>(ProcError::kUnsupportedElement),
              static_cast<int32_t>(ProcError::kInvalidTransition));
}

TEST(ProcessContractTest, PRC04_SerialiserDistinctFromDeserialiser) {
    EXPECT_NE(static_cast<int32_t>(ProcError::kSerialiserFailed),
              static_cast<int32_t>(ProcError::kDeserialiserFailed));
}

TEST(ProcessContractTest, PRC05_ExecutionTimeoutIsHighestCode) {
    int32_t to = static_cast<int32_t>(ProcError::kExecutionTimeout);
    EXPECT_GE(to, static_cast<int32_t>(ProcError::kUnsupportedElement));
    EXPECT_GE(to, static_cast<int32_t>(ProcError::kInvalidTransition));
}

TEST(ProcessContractTest, PRC06_ErrorSwitchDispatch) {
    ProcError err = ProcError::kDeserialiserFailed;
    bool handled = false;
    switch (err) {
        case ProcError::kUnsupportedElement:  break;
        case ProcError::kInvalidTransition:   break;
        case ProcError::kSerialiserFailed:    break;
        case ProcError::kDeserialiserFailed:  handled = true; break;
        case ProcError::kExecutionTimeout:    break;
    }
    EXPECT_TRUE(handled);
}

TEST(ProcessContractTest, PRC07_UnsupportedElementLowestCode) {
    int32_t v = static_cast<int32_t>(ProcError::kUnsupportedElement);
    EXPECT_EQ(v, 7600);
}

TEST(ProcessContractTest, PRC08_ExecutionTimeoutCode) {
    int32_t v = static_cast<int32_t>(ProcError::kExecutionTimeout);
    EXPECT_EQ(v, 7604);
}
