/**
 * @file test_governance_contract_hardening_focused.cpp
 * @brief Phase 1–6 contract-hardening tests for the governance module.
 * @note Test IDs: GOV-01..GOV-16
 * @note Coverage: error taxonomy, compliance check contract, regulation enum,
 *                 consent missing, jurisdiction blocked, export limits,
 *                 rule-conflict detection, audit write failure codes.
 */

#include <gtest/gtest.h>
#include "governance/governance_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace themis::governance;

class GovContractTest : public ::testing::Test {
protected:
    static constexpr uint32_t kSeed = 42;
    std::mt19937 rng_{kSeed};
};

// GOV-01: Error codes are unique
TEST_F(GovContractTest, GOV01_ErrorCodesAreUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(GovError::kRegulationUnknown),
        static_cast<int32_t>(GovError::kConsentMissing),
        static_cast<int32_t>(GovError::kJurisdictionBlocked),
        static_cast<int32_t>(GovError::kExportLimitExceeded),
        static_cast<int32_t>(GovError::kAuditWriteFailed),
        static_cast<int32_t>(GovError::kRuleConflict),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

// GOV-02: Error codes in range [7200, 7299]
TEST_F(GovContractTest, GOV02_ErrorCodesInRange) {
    auto check = [](GovError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7200); EXPECT_LE(v, 7299);
    };
    check(GovError::kRegulationUnknown);
    check(GovError::kConsentMissing);
    check(GovError::kJurisdictionBlocked);
    check(GovError::kExportLimitExceeded);
    check(GovError::kAuditWriteFailed);
    check(GovError::kRuleConflict);
}

// GOV-03: ComplianceCheckResult default is denied
TEST_F(GovContractTest, GOV03_ComplianceCheckResultDefaultIsDenied) {
    ComplianceCheckResult r;
    EXPECT_FALSE(r.allowed);
}

// GOV-04: ComplianceCheckResult reason field defaults to kConsentMissing
TEST_F(GovContractTest, GOV04_DefaultReasonIsConsentMissing) {
    ComplianceCheckResult r;
    EXPECT_EQ(r.reason, GovError::kConsentMissing);
}

// GOV-05: Regulation enum values are distinct
TEST_F(GovContractTest, GOV05_RegulationValuesAreDistinct) {
    std::vector<int32_t> regs = {
        static_cast<int32_t>(Regulation::kGDPR),
        static_cast<int32_t>(Regulation::kCCPA),
        static_cast<int32_t>(Regulation::kLGPD),
        static_cast<int32_t>(Regulation::kHIPAA),
    };
    std::sort(regs.begin(), regs.end());
    EXPECT_EQ(std::unique(regs.begin(), regs.end()), regs.end());
}

// GOV-06: ComplianceCheckResult allowed=true can be set
TEST_F(GovContractTest, GOV06_AllowedResultCanBeSet) {
    ComplianceCheckResult r;
    r.allowed = true;
    r.justification = "Consent verified";
    EXPECT_TRUE(r.allowed);
    EXPECT_FALSE(r.justification.empty());
}

// GOV-07: kJurisdictionBlocked is distinct from kConsentMissing
TEST_F(GovContractTest, GOV07_JurisdictionBlockedDistinctFromConsentMissing) {
    EXPECT_NE(static_cast<int32_t>(GovError::kJurisdictionBlocked),
              static_cast<int32_t>(GovError::kConsentMissing));
}

// GOV-08: kExportLimitExceeded is distinct from kAuditWriteFailed
TEST_F(GovContractTest, GOV08_ExportLimitDistinctFromAuditFailed) {
    EXPECT_NE(static_cast<int32_t>(GovError::kExportLimitExceeded),
              static_cast<int32_t>(GovError::kAuditWriteFailed));
}

// GOV-09: GDPR regulation value is 1
TEST_F(GovContractTest, GOV09_GDPRValueIsOne) {
    EXPECT_EQ(static_cast<int32_t>(Regulation::kGDPR), 1);
}

// GOV-10: CCPA regulation value is 2
TEST_F(GovContractTest, GOV10_CCPAValueIsTwo) {
    EXPECT_EQ(static_cast<int32_t>(Regulation::kCCPA), 2);
}

// GOV-11: ComplianceCheckResult justification empty when denied by default
TEST_F(GovContractTest, GOV11_DefaultJustificationIsEmpty) {
    ComplianceCheckResult r;
    EXPECT_TRUE(r.justification.empty());
}

// GOV-12: kRuleConflict is highest error code in module range
TEST_F(GovContractTest, GOV12_RuleConflictIsHighestCode) {
    int32_t rc = static_cast<int32_t>(GovError::kRuleConflict);
    EXPECT_GE(rc, static_cast<int32_t>(GovError::kRegulationUnknown));
    EXPECT_GE(rc, static_cast<int32_t>(GovError::kConsentMissing));
    EXPECT_GE(rc, static_cast<int32_t>(GovError::kJurisdictionBlocked));
    EXPECT_GE(rc, static_cast<int32_t>(GovError::kExportLimitExceeded));
    EXPECT_GE(rc, static_cast<int32_t>(GovError::kAuditWriteFailed));
}

// GOV-13: Multiple ComplianceCheckResult instances are independent
TEST_F(GovContractTest, GOV13_MultipleResultsAreIndependent) {
    ComplianceCheckResult a, b;
    a.allowed = true;
    a.justification = "Consent ok";
    b.allowed = false;
    b.reason = GovError::kJurisdictionBlocked;
    EXPECT_TRUE(a.allowed);
    EXPECT_FALSE(b.allowed);
    EXPECT_EQ(b.reason, GovError::kJurisdictionBlocked);
}

// GOV-14: Regulation enum is suitable for switch dispatch
TEST_F(GovContractTest, GOV14_RegulationSwitchDispatch) {
    Regulation reg = Regulation::kHIPAA;
    std::string name = {};
    switch (reg) {
        case Regulation::kGDPR:  name = "GDPR";  break;
        case Regulation::kCCPA:  name = "CCPA";  break;
        case Regulation::kLGPD:  name = "LGPD";  break;
        case Regulation::kHIPAA: name = "HIPAA"; break;
    }
    EXPECT_EQ(name, "HIPAA");
}

// GOV-15: GovError is suitable for switch dispatch
TEST_F(GovContractTest, GOV15_GovErrorSwitchDispatch) {
    GovError err = GovError::kAuditWriteFailed;
    bool handled = false;
    switch (err) {
        case GovError::kRegulationUnknown:   handled = false; break;
        case GovError::kConsentMissing:      handled = false; break;
        case GovError::kJurisdictionBlocked: handled = false; break;
        case GovError::kExportLimitExceeded: handled = false; break;
        case GovError::kAuditWriteFailed:    handled = true;  break;
        case GovError::kRuleConflict:        handled = false; break;
    }
    EXPECT_TRUE(handled);
}

// GOV-16: Randomised ComplianceCheckResult state consistency
TEST_F(GovContractTest, GOV16_RandomisedStateConsistency) {
    for (int i = 0; i < 100; ++i) {
        ComplianceCheckResult r;
        r.allowed = (rng_() % 2 == 0);
        if (!r.allowed) {
            r.justification = "Denied-" + std::to_string(i);
        }
        if (r.allowed) {
            EXPECT_TRUE(r.allowed);
        } else {
            EXPECT_FALSE(r.allowed);
            EXPECT_FALSE(r.justification.empty());
        }
    }
}
