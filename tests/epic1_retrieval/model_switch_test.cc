/**
 * @file model_switch_test.cc
 * @brief Contract tests for IModelSwitchController (sub-issue #5419).
 *
 * Validates factory construction, initial active model, compatibility check,
 * execute returning a SwitchResult, rollback, and observer registration.
 * Production zero-downtime switching is tracked in sub-issue #5419.
 */

#include "retrieval/include/model_switch.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace themis::retrieval;

namespace {

ModelVersion makeModel(const std::string& id,
                        const std::string& arch = "llama-3-8b") {
    ModelVersion mv;
    mv.id           = id;
    mv.architecture = arch;
    mv.quantization = "fp16";
    mv.compatible_adapter_tokens = {"tok-" + id};
    return mv;
}

CompatibilityPolicy strictPolicy() {
    CompatibilityPolicy p;
    p.require_exact_arch_match       = true;
    p.allow_quantization_mismatch    = false;
    p.min_eval_score_delta           = -0.05f;
    return p;
}

} // namespace

class ModelSwitchControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        initial_ = makeModel("model-v1");
        ctrl_ = makeModelSwitchController(initial_, strictPolicy());
        ASSERT_NE(ctrl_, nullptr);
    }

    ModelVersion initial_;
    std::unique_ptr<IModelSwitchController> ctrl_;
};

TEST_F(ModelSwitchControllerTest, FactoryReturnsNonNull) {
    EXPECT_NE(ctrl_, nullptr);
}

TEST_F(ModelSwitchControllerTest, ActiveModelMatchesInitial) {
    ModelVersion active = ctrl_->active();
    EXPECT_EQ(active.id, initial_.id);
    EXPECT_EQ(active.architecture, initial_.architecture);
}

TEST_F(ModelSwitchControllerTest, ExecuteReturnsSwitchResult) {
    SwitchRequest req;
    req.target_model   = makeModel("model-v2");
    req.allow_rollback = true;
    req.deadline       = std::chrono::milliseconds{5000};

    SwitchResult result = ctrl_->execute(req);
    // Scaffold: may return any outcome; must not throw.
    (void)result;
    SUCCEED();
}

TEST_F(ModelSwitchControllerTest, ExecuteSuccessOutcomeInScaffold) {
    SwitchRequest req;
    req.target_model = makeModel("model-v2");

    SwitchResult result = ctrl_->execute(req);
    EXPECT_EQ(result.outcome, SwitchOutcome::Success);
}

TEST_F(ModelSwitchControllerTest, ActiveUpdatedAfterSuccessfulSwitch) {
    SwitchRequest req;
    req.target_model = makeModel("model-v2");
    SwitchResult result = ctrl_->execute(req);

    if (result.outcome == SwitchOutcome::Success) {
        EXPECT_EQ(ctrl_->active().id, "model-v2");
    }
}

TEST_F(ModelSwitchControllerTest, RollbackDoesNotThrow) {
    EXPECT_NO_THROW(ctrl_->rollback());
}

TEST_F(ModelSwitchControllerTest, CheckCompatibilityDoesNotThrow) {
    ModelVersion target = makeModel("model-v3");
    EXPECT_NO_THROW(ctrl_->checkCompatibility(target, strictPolicy()));
}

TEST_F(ModelSwitchControllerTest, SameArchPassesCompatibility) {
    ModelVersion target = makeModel("model-v2", "llama-3-8b");
    bool compat = ctrl_->checkCompatibility(target, strictPolicy());
    EXPECT_TRUE(compat);
}

TEST_F(ModelSwitchControllerTest, DifferentArchFailsStrictCompatibility) {
    ModelVersion target = makeModel("model-v2", "mistral-7b");
    bool compat = ctrl_->checkCompatibility(target, strictPolicy());
    EXPECT_FALSE(compat);
}

TEST_F(ModelSwitchControllerTest, ObserverRegistrationDoesNotThrow) {
    EXPECT_NO_THROW(ctrl_->onSwitch([](const SwitchResult&) {}));
}
