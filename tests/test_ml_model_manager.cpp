#include <gtest/gtest.h>
#include "llm/ml_model_manager.h"

namespace {

using namespace themis::llm;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::unique_ptr<MLModelManager> makeManager() {
    MLModelManager::Config cfg;
    return std::make_unique<MLModelManager>(cfg);
}

static void registerAndDeploy(MLModelManager& mgr) {
    MLModelConfig mc;
    mc.model_id   = "test-model";
    mc.model_name = "Test Model";
    mc.version    = "1.0";
    mc.type       = MLModelType::CUSTOM;
    mc.format     = "dummy";
    auto reg = mgr.registerModel(mc);
    ASSERT_TRUE(reg.has_value()) << reg.error().message();

    auto dep = mgr.deployModel("test-model", 1);
    ASSERT_TRUE(dep.has_value()) << dep.error().message();
}

static MLInferenceRequest makeRequest() {
    MLInferenceRequest req;
    req.model_id   = "test-model";
    req.input_data = {{"prompt", "hello"}};
    return req;
}

// ---------------------------------------------------------------------------
// MMM-01: Without an injected dispatch fn, infer() returns the simulated stub
// ---------------------------------------------------------------------------
TEST(MlModelManagerInjectTest, MMM01_SimulatedFallbackWithNoDispatchFn) {
    auto mgr = makeManager();
    registerAndDeploy(*mgr);

    auto result = mgr->infer(makeRequest());
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->output_data.value("result", std::string{}), "simulated");
}

// ---------------------------------------------------------------------------
// MMM-02: With an injected dispatch fn, infer() calls the fn and returns its output
// ---------------------------------------------------------------------------
TEST(MlModelManagerInjectTest, MMM02_InjectedDispatchFnIsCalledAndResultReturned) {
    auto mgr = makeManager();
    registerAndDeploy(*mgr);

    bool fn_called = false;
    mgr->setInferenceDispatchFn(
        [&fn_called](const MLInferenceRequest& req, MLModelInstance& /*inst*/) -> json {
            fn_called = true;
            return json{{"answer", 42}, {"model", req.model_id}};
        });

    auto result = mgr->infer(makeRequest());
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    EXPECT_TRUE(fn_called) << "Injected dispatch fn was not called";
    EXPECT_EQ(result->output_data.value("answer", 0), 42);
    EXPECT_EQ(result->output_data.value("model", std::string{}), "test-model");
    EXPECT_NE(result->output_data.value("result", std::string{}), "simulated");
}

// ---------------------------------------------------------------------------
// MMM-03: Resetting dispatch fn to nullptr reverts to the simulated fallback
// ---------------------------------------------------------------------------
TEST(MlModelManagerInjectTest, MMM03_NullptrResetRevertsToSimulatedFallback) {
    auto mgr = makeManager();
    registerAndDeploy(*mgr);

    mgr->setInferenceDispatchFn(
        [](const MLInferenceRequest&, MLModelInstance&) -> json {
            return json{{"real", true}};
        });

    auto r1 = mgr->infer(makeRequest());
    ASSERT_TRUE(r1.has_value());
    EXPECT_TRUE(r1->output_data.value("real", false));

    mgr->setInferenceDispatchFn(nullptr);

    auto r2 = mgr->infer(makeRequest());
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(r2->success);
    EXPECT_EQ(r2->output_data.value("result", std::string{}), "simulated");
}

} // namespace
