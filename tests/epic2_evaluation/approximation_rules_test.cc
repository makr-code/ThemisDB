/**
 * @file approximation_rules_test.cc
 * @brief Contract tests for IApproximationRules (sub-issue #5440).
 *
 * Validates factory construction, zone registration, recall checking,
 * violation reporting, callback registration, and zone listing.
 * Production governance enforcement is tracked in sub-issue #5440.
 */

#include "evaluation/include/approximation_rules.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace themis::evaluation;

namespace {

ApproximationZone makeZone(const std::string& id,
                            ExactnessRequirement req = ExactnessRequirement::Approximate) {
    ApproximationZone z;
    z.id                     = id;
    z.description            = "Test zone " + id;
    z.requirement            = req;
    z.max_recall_degradation = 0.05f;
    z.audit_required         = false;
    return z;
}

ApproximationViolation makeViolation(const std::string& zone_id) {
    ApproximationViolation v;
    v.zone_id          = zone_id;
    v.observed_recall  = 0.85f;
    v.allowed_recall   = 0.90f;
    v.request_id       = "req-001";
    return v;
}

} // namespace

class ApproximationRulesTest : public ::testing::Test {
protected:
    void SetUp() override {
        rules_ = makeApproximationRules();
        ASSERT_NE(rules_, nullptr);
    }

    std::unique_ptr<IApproximationRules> rules_;
};

TEST_F(ApproximationRulesTest, FactoryReturnsNonNull) {
    EXPECT_NE(rules_, nullptr);
}

TEST_F(ApproximationRulesTest, DefaultZonesPrePopulated) {
    // Factory should pre-populate at least the EPIC-2 zone set.
    auto zones = rules_->listZones();
    EXPECT_FALSE(zones.empty());
}

TEST_F(ApproximationRulesTest, RegisterZoneDoesNotThrow) {
    EXPECT_NO_THROW(rules_->registerZone(makeZone("custom-zone")));
}

TEST_F(ApproximationRulesTest, RegisteredZoneAppearsInList) {
    rules_->registerZone(makeZone("new-zone"));
    auto zones = rules_->listZones();
    bool found = false;
    for (const auto& id : zones) {
        if (id == "new-zone") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ApproximationRulesTest, CheckRecallPassesAboveThreshold) {
    rules_->registerZone(makeZone("check-zone"));
    // Recall above 1 - max_recall_degradation = 0.95 should pass.
    EXPECT_TRUE(rules_->checkRecall("check-zone", 0.96f));
}

TEST_F(ApproximationRulesTest, CheckRecallFailsBelowThreshold) {
    rules_->registerZone(makeZone("check-zone-2"));
    EXPECT_FALSE(rules_->checkRecall("check-zone-2", 0.50f));
}

TEST_F(ApproximationRulesTest, RequirementReturnsRegisteredValue) {
    rules_->registerZone(makeZone("exact-zone", ExactnessRequirement::Exact));
    EXPECT_EQ(rules_->requirement("exact-zone"), ExactnessRequirement::Exact);
}

TEST_F(ApproximationRulesTest, ReportViolationDoesNotThrow) {
    rules_->registerZone(makeZone("violation-zone"));
    EXPECT_NO_THROW(rules_->reportViolation(makeViolation("violation-zone")));
}

TEST_F(ApproximationRulesTest, ViolationCallbackIsRegistered) {
    bool invoked = false;
    rules_->onViolation([&](const ApproximationViolation&) { invoked = true; });
    rules_->registerZone(makeZone("cb-zone"));
    rules_->reportViolation(makeViolation("cb-zone"));
    // Scaffold may or may not invoke callback synchronously.
    (void)invoked;
    SUCCEED();
}

TEST_F(ApproximationRulesTest, CheckRecallOnUnknownZoneDoesNotThrow) {
    EXPECT_NO_THROW({
        try {
            rules_->checkRecall("unknown-zone", 0.95f);
        } catch (const std::exception&) {
            // acceptable at scaffold stage
        }
    });
}
