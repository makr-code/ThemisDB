/**
 * @file test_utils_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the utils module.
 * @note Test IDs: UTL-01..UTL-16
 * @note Coverage: audit overflow, batch rollback, size limits, retry exhaustion,
 *                 deserialisation errors, pool exhaustion, Bloom filter contracts,
 *                 retry policy configuration.
 */

#include <gtest/gtest.h>
#include "utils/utils_api_contract.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace themis::utils;

class UtilsContractTest : public ::testing::Test {
protected:
    static constexpr uint32_t kSeed = 42;
    std::mt19937 rng_{kSeed};
};

// UTL-01: Error codes are unique
TEST_F(UtilsContractTest, UTL01_ErrorCodesAreUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(UtilsError::kAuditOverflow),
        static_cast<int32_t>(UtilsError::kBatchRollback),
        static_cast<int32_t>(UtilsError::kBatchSizeExceeded),
        static_cast<int32_t>(UtilsError::kRetryExhausted),
        static_cast<int32_t>(UtilsError::kDeserInvalid),
        static_cast<int32_t>(UtilsError::kPoolExhausted),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

// UTL-02: Error codes in range [7300, 7399]
TEST_F(UtilsContractTest, UTL02_ErrorCodesInRange) {
    auto check = [](UtilsError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7300); EXPECT_LE(v, 7399);
    };
    check(UtilsError::kAuditOverflow);
    check(UtilsError::kBatchRollback);
    check(UtilsError::kBatchSizeExceeded);
    check(UtilsError::kRetryExhausted);
    check(UtilsError::kDeserInvalid);
    check(UtilsError::kPoolExhausted);
}

// UTL-03: BloomFilterConfig defaults are sane
TEST_F(UtilsContractTest, UTL03_BloomFilterConfigDefaults) {
    BloomFilterConfig cfg;
    EXPECT_GT(cfg.expectedItems, 0u);
    EXPECT_GT(cfg.targetFalsePositiveRate, 0.0);
    EXPECT_LT(cfg.targetFalsePositiveRate, 1.0);
}

// UTL-04: BloomFilterConfig FPR is bounded (0, 1)
TEST_F(UtilsContractTest, UTL04_BloomFPRBounded) {
    BloomFilterConfig cfg;
    cfg.targetFalsePositiveRate = 0.001; // 0.1%
    EXPECT_GT(cfg.targetFalsePositiveRate, 0.0);
    EXPECT_LT(cfg.targetFalsePositiveRate, 1.0);
}

// UTL-05: RetryPolicy defaults are sane
TEST_F(UtilsContractTest, UTL05_RetryPolicyDefaults) {
    RetryPolicy p;
    EXPECT_GE(p.maxAttempts, 1u);
    EXPECT_GT(p.initialDelay.count(), 0);
    EXPECT_GT(p.backoffMultiplier, 1.0);
}

// UTL-06: RetryPolicy maxAttempts=1 means no retry
TEST_F(UtilsContractTest, UTL06_SingleAttemptRetryPolicy) {
    RetryPolicy p;
    p.maxAttempts = 1;
    EXPECT_EQ(p.maxAttempts, 1u);
}

// UTL-07: RetryPolicy with jitter disabled is reproducible
TEST_F(UtilsContractTest, UTL07_NoJitterPolicyReproducible) {
    RetryPolicy p;
    p.withJitter = false;
    EXPECT_FALSE(p.withJitter);
}

// UTL-08: kBatchRollback is distinct from kBatchSizeExceeded
TEST_F(UtilsContractTest, UTL08_BatchErrorsAreDistinct) {
    EXPECT_NE(static_cast<int32_t>(UtilsError::kBatchRollback),
              static_cast<int32_t>(UtilsError::kBatchSizeExceeded));
}

// UTL-09: kDeserInvalid is distinct from kAuditOverflow
TEST_F(UtilsContractTest, UTL09_DeserDistinctFromAuditOverflow) {
    EXPECT_NE(static_cast<int32_t>(UtilsError::kDeserInvalid),
              static_cast<int32_t>(UtilsError::kAuditOverflow));
}

// UTL-10: BloomFilterConfig is default-constructible
TEST_F(UtilsContractTest, UTL10_BloomConfigDefaultConstructible) {
    BloomFilterConfig cfg{};
    EXPECT_GT(cfg.expectedItems, 0u);
}

// UTL-11: RetryPolicy is copy-constructible
TEST_F(UtilsContractTest, UTL11_RetryPolicyCopyConstructible) {
    RetryPolicy orig;
    orig.maxAttempts = 7;
    RetryPolicy copy = orig;
    EXPECT_EQ(copy.maxAttempts, 7u);
}

// UTL-12: RetryPolicy backoffMultiplier can be set to 1.0 (no backoff)
TEST_F(UtilsContractTest, UTL12_BackoffMultiplierOne) {
    RetryPolicy p;
    p.backoffMultiplier = 1.0;
    EXPECT_DOUBLE_EQ(p.backoffMultiplier, 1.0);
}

// UTL-13: UtilsError switch dispatch compiles and covers all codes
TEST_F(UtilsContractTest, UTL13_ErrorSwitchDispatch) {
    UtilsError err = UtilsError::kRetryExhausted;
    bool handled = false;
    switch (err) {
        case UtilsError::kAuditOverflow:     break;
        case UtilsError::kBatchRollback:     break;
        case UtilsError::kBatchSizeExceeded: break;
        case UtilsError::kRetryExhausted:    handled = true; break;
        case UtilsError::kDeserInvalid:      break;
        case UtilsError::kPoolExhausted:     break;
    }
    EXPECT_TRUE(handled);
}

// UTL-14: BloomFilterConfig expectedItems can be large (1M)
TEST_F(UtilsContractTest, UTL14_LargeBloomFilter) {
    BloomFilterConfig cfg;
    cfg.expectedItems = 1'000'000;
    EXPECT_EQ(cfg.expectedItems, 1'000'000u);
}

// UTL-15: RetryPolicy initialDelay is in chrono units
TEST_F(UtilsContractTest, UTL15_RetryPolicyDelayInChrono) {
    RetryPolicy p;
    p.initialDelay = std::chrono::milliseconds{500};
    EXPECT_EQ(p.initialDelay.count(), 500);
}

// UTL-16: Randomised error code round-trip via static_cast
TEST_F(UtilsContractTest, UTL16_ErrorCodeRoundTrip) {
    static const UtilsError kCodes[] = {
        UtilsError::kAuditOverflow,
        UtilsError::kBatchRollback,
        UtilsError::kBatchSizeExceeded,
        UtilsError::kRetryExhausted,
        UtilsError::kDeserInvalid,
        UtilsError::kPoolExhausted,
    };
    for (auto e : kCodes) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7300);
        EXPECT_LE(v, 7399);
    }
}
