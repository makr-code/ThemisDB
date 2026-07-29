/**
 * @file test_content_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the content module.
 * @note Test IDs: CNT-01..CNT-08
 */

#include <gtest/gtest.h>
#include "content/content_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace themis::content;

TEST(ContentContractTest, CNT01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(ContentError::kScanError),
        static_cast<int32_t>(ContentError::kUnsupportedFormat),
        static_cast<int32_t>(ContentError::kSizeLimit),
        static_cast<int32_t>(ContentError::kEncodingInvalid),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

TEST(ContentContractTest, CNT02_ErrorCodesInRange) {
    auto check = [](ContentError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 8000); EXPECT_LE(v, 8099);
    };
    check(ContentError::kScanError);
    check(ContentError::kUnsupportedFormat);
    check(ContentError::kSizeLimit);
    check(ContentError::kEncodingInvalid);
}

TEST(ContentContractTest, CNT03_ScanVerdictAllowIsZero) {
    EXPECT_EQ(static_cast<int32_t>(ScanVerdict::kAllow), 0);
}

TEST(ContentContractTest, CNT04_ScanVerdictBlockIsOne) {
    EXPECT_EQ(static_cast<int32_t>(ScanVerdict::kBlock), 1);
}

TEST(ContentContractTest, CNT05_ScanResultDefaultIsAllow) {
    ScanResult r;
    EXPECT_EQ(r.verdict, ScanVerdict::kAllow);
    EXPECT_TRUE(r.reason.empty());
}

TEST(ContentContractTest, CNT06_BlockHasNonEmptyReason) {
    ScanResult r;
    r.verdict = ScanVerdict::kBlock;
    r.reason  = "Prohibited content detected";
    EXPECT_EQ(r.verdict, ScanVerdict::kBlock);
    EXPECT_FALSE(r.reason.empty());
}

TEST(ContentContractTest, CNT07_ErrorSwitchDispatch) {
    ContentError err = ContentError::kSizeLimit;
    bool handled = false;
    switch (err) {
        case ContentError::kScanError:          break;
        case ContentError::kUnsupportedFormat:  break;
        case ContentError::kSizeLimit:          handled = true; break;
        case ContentError::kEncodingInvalid:    break;
    }
    EXPECT_TRUE(handled);
}

TEST(ContentContractTest, CNT08_VerdictSwitchDispatch) {
    ScanVerdict v = ScanVerdict::kBlock;
    bool blocked = false;
    switch (v) {
        case ScanVerdict::kAllow: blocked = false; break;
        case ScanVerdict::kBlock: blocked = true;  break;
    }
    EXPECT_TRUE(blocked);
}
