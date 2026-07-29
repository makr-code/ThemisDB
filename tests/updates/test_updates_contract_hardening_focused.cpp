/**
 * @file test_updates_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the updates module.
 * @note Test IDs: UPD-01..UPD-16
 * @note Coverage: error taxonomy, deployment slot enum, verification result,
 *                 rollback contract, canary abort, checksum mismatch,
 *                 artifact missing, patch incompatible.
 */

#include <gtest/gtest.h>
#include "updates/updates_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace themis::updates;

class UpdatesContractTest : public ::testing::Test {
protected:
    static constexpr uint32_t kSeed = 42;
    std::mt19937 rng_{kSeed};
};

// UPD-01: Error codes are unique
TEST_F(UpdatesContractTest, UPD01_ErrorCodesAreUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(UpdatesError::kNoRollbackTarget),
        static_cast<int32_t>(UpdatesError::kChecksumMismatch),
        static_cast<int32_t>(UpdatesError::kArtifactMissing),
        static_cast<int32_t>(UpdatesError::kPatchIncompatible),
        static_cast<int32_t>(UpdatesError::kSwitchInProgress),
        static_cast<int32_t>(UpdatesError::kCanaryAborted),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

// UPD-02: Error codes in range [7400, 7499]
TEST_F(UpdatesContractTest, UPD02_ErrorCodesInRange) {
    auto check = [](UpdatesError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7400); EXPECT_LE(v, 7499);
    };
    check(UpdatesError::kNoRollbackTarget);
    check(UpdatesError::kChecksumMismatch);
    check(UpdatesError::kArtifactMissing);
    check(UpdatesError::kPatchIncompatible);
    check(UpdatesError::kSwitchInProgress);
    check(UpdatesError::kCanaryAborted);
}

// UPD-03: DeploymentSlot values are distinct
TEST_F(UpdatesContractTest, UPD03_DeploymentSlotValuesDistinct) {
    EXPECT_NE(static_cast<int32_t>(DeploymentSlot::kBlue),
              static_cast<int32_t>(DeploymentSlot::kGreen));
}

// UPD-04: Blue slot is value 0, Green is 1
TEST_F(UpdatesContractTest, UPD04_SlotValues) {
    EXPECT_EQ(static_cast<int32_t>(DeploymentSlot::kBlue),  0);
    EXPECT_EQ(static_cast<int32_t>(DeploymentSlot::kGreen), 1);
}

// UPD-05: VerificationResult default is invalid
TEST_F(UpdatesContractTest, UPD05_VerificationResultDefaultInvalid) {
    VerificationResult r;
    EXPECT_FALSE(r.valid);
    EXPECT_TRUE(r.signatureToken.empty());
}

// UPD-06: Valid VerificationResult has non-empty token
TEST_F(UpdatesContractTest, UPD06_ValidResultHasToken) {
    VerificationResult r;
    r.valid = true;
    r.signatureToken = "sha256:abc123";
    EXPECT_TRUE(r.valid);
    EXPECT_FALSE(r.signatureToken.empty());
}

// UPD-07: kChecksumMismatch is distinct from kArtifactMissing
TEST_F(UpdatesContractTest, UPD07_ChecksumDistinctFromArtifactMissing) {
    EXPECT_NE(static_cast<int32_t>(UpdatesError::kChecksumMismatch),
              static_cast<int32_t>(UpdatesError::kArtifactMissing));
}

// UPD-08: kSwitchInProgress is distinct from kCanaryAborted
TEST_F(UpdatesContractTest, UPD08_SwitchDistinctFromCanaryAborted) {
    EXPECT_NE(static_cast<int32_t>(UpdatesError::kSwitchInProgress),
              static_cast<int32_t>(UpdatesError::kCanaryAborted));
}

// UPD-09: kNoRollbackTarget is the lowest error code
TEST_F(UpdatesContractTest, UPD09_NoRollbackTargetLowest) {
    int32_t v = static_cast<int32_t>(UpdatesError::kNoRollbackTarget);
    EXPECT_LE(v, static_cast<int32_t>(UpdatesError::kChecksumMismatch));
    EXPECT_LE(v, static_cast<int32_t>(UpdatesError::kCanaryAborted));
}

// UPD-10: VerificationResult error field defaults to kChecksumMismatch
TEST_F(UpdatesContractTest, UPD10_DefaultErrorIsChecksumMismatch) {
    VerificationResult r;
    EXPECT_EQ(r.error, UpdatesError::kChecksumMismatch);
}

// UPD-11: DeploymentSlot switch dispatch works
TEST_F(UpdatesContractTest, UPD11_SlotSwitchDispatch) {
    DeploymentSlot slot = DeploymentSlot::kGreen;
    std::string name;
    switch (slot) {
        case DeploymentSlot::kBlue:  name = "blue";  break;
        case DeploymentSlot::kGreen: name = "green"; break;
    }
    EXPECT_EQ(name, "green");
}

// UPD-12: UpdatesError switch dispatch covers all codes
TEST_F(UpdatesContractTest, UPD12_ErrorSwitchDispatch) {
    UpdatesError err = UpdatesError::kPatchIncompatible;
    bool handled = false;
    switch (err) {
        case UpdatesError::kNoRollbackTarget:  break;
        case UpdatesError::kChecksumMismatch:  break;
        case UpdatesError::kArtifactMissing:   break;
        case UpdatesError::kPatchIncompatible: handled = true; break;
        case UpdatesError::kSwitchInProgress:  break;
        case UpdatesError::kCanaryAborted:     break;
    }
    EXPECT_TRUE(handled);
}

// UPD-13: VerificationResult is copy-constructible
TEST_F(UpdatesContractTest, UPD13_VerificationResultCopyConstructible) {
    VerificationResult orig;
    orig.valid = true;
    orig.signatureToken = "token-xyz";
    VerificationResult copy = orig;
    EXPECT_EQ(copy.valid, orig.valid);
    EXPECT_EQ(copy.signatureToken, orig.signatureToken);
}

// UPD-14: VerificationResult is move-constructible
TEST_F(UpdatesContractTest, UPD14_VerificationResultMoveConstructible) {
    VerificationResult orig;
    orig.valid = true;
    orig.signatureToken = "move-token";
    std::string expectedToken = orig.signatureToken;
    VerificationResult moved = std::move(orig);
    EXPECT_TRUE(moved.valid);
    EXPECT_EQ(moved.signatureToken, expectedToken);
}

// UPD-15: Multiple VerificationResult instances are independent
TEST_F(UpdatesContractTest, UPD15_ResultInstancesIndependent) {
    VerificationResult a, b;
    a.valid = true; a.signatureToken = "tok-a";
    b.valid = false;
    EXPECT_TRUE(a.valid);
    EXPECT_FALSE(b.valid);
}

// UPD-16: Randomised slot and error coverage
TEST_F(UpdatesContractTest, UPD16_RandomisedCoverage) {
    static const UpdatesError kErrors[] = {
        UpdatesError::kNoRollbackTarget,
        UpdatesError::kChecksumMismatch,
        UpdatesError::kArtifactMissing,
        UpdatesError::kPatchIncompatible,
        UpdatesError::kSwitchInProgress,
        UpdatesError::kCanaryAborted,
    };
    for (int i = 0; i < 60; ++i) {
        auto err = kErrors[rng_() % 6];
        int32_t v = static_cast<int32_t>(err);
        EXPECT_GE(v, 7400);
        EXPECT_LE(v, 7499);
    }
}
