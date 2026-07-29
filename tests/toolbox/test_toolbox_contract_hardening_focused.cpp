/**
 * @file test_toolbox_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the toolbox module.
 * @note Test IDs: TBX-01..TBX-08
 */

#include <gtest/gtest.h>
#include "toolbox/toolbox_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

using namespace themis::toolbox;

class ToolboxContractTest : public ::testing::Test {
protected:
    static constexpr uint32_t kSeed = 42;
    std::mt19937 rng_{kSeed};
};

TEST_F(ToolboxContractTest, TBX01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(ToolboxError::kEmptyInput),
        static_cast<int32_t>(ToolboxError::kNoProcessor),
        static_cast<int32_t>(ToolboxError::kProcessorFailed),
        static_cast<int32_t>(ToolboxError::kEncodingUnsupported),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

TEST_F(ToolboxContractTest, TBX02_ErrorCodesInRange) {
    auto check = [](ToolboxError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7500); EXPECT_LE(v, 7599);
    };
    check(ToolboxError::kEmptyInput);
    check(ToolboxError::kNoProcessor);
    check(ToolboxError::kProcessorFailed);
    check(ToolboxError::kEncodingUnsupported);
}

TEST_F(ToolboxContractTest, TBX03_FingerprintSizeIs32) {
    Fingerprint fp{};
    EXPECT_EQ(fp.size(), 32u);
}

TEST_F(ToolboxContractTest, TBX04_FingerprintDefaultAllZero) {
    Fingerprint fp{};
    for (auto b : fp) EXPECT_EQ(b, 0u);
}

TEST_F(ToolboxContractTest, TBX05_FingerprintCanBeSet) {
    Fingerprint fp{};
    fp[0] = 0xDE; fp[31] = 0xAD;
    EXPECT_EQ(fp[0],  0xDE);
    EXPECT_EQ(fp[31], 0xAD);
}

TEST_F(ToolboxContractTest, TBX06_EmptyInputDistinctFromNoProcessor) {
    EXPECT_NE(static_cast<int32_t>(ToolboxError::kEmptyInput),
              static_cast<int32_t>(ToolboxError::kNoProcessor));
}

TEST_F(ToolboxContractTest, TBX07_ErrorSwitchDispatch) {
    ToolboxError err = ToolboxError::kProcessorFailed;
    bool handled = false;
    switch (err) {
        case ToolboxError::kEmptyInput:          break;
        case ToolboxError::kNoProcessor:         break;
        case ToolboxError::kProcessorFailed:     handled = true; break;
        case ToolboxError::kEncodingUnsupported: break;
    }
    EXPECT_TRUE(handled);
}

TEST_F(ToolboxContractTest, TBX08_RandomisedFingerprintCoverage) {
    for (int i = 0; i < 32; ++i) {
        Fingerprint fp{};
        fp[i] = static_cast<uint8_t>(rng_() & 0xFF);
        EXPECT_NE(fp[i], 0u);  // extremely unlikely to be zero
    }
}
