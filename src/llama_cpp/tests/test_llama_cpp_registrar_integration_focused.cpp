/**
 * @file test_llama_cpp_registrar_integration_focused.cpp
 * @brief Group W — Server-startup registrar integration tests.
 *
 * Validates LlamaCppPluginRegistrar::initFromServerConfig() and related
 * helpers that wire the LLM plugin subsystem to the server startup path.
 *
 * All tests run under THEMIS_LLAMA_CPP_STUB_MODE so no real model file is
 * required.
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_registrar.h"
#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/llm_plugin_manager.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace themis::llamacpp;
using namespace themis::llm;

namespace {

template <typename Fn>
class LambdaTest final : public ::testing::Test {
public:
    explicit LambdaTest(Fn fn) : fn_(fn) {}

protected:
    void TestBody() override { fn_(); }

private:
    Fn fn_;
};

template <typename Fn>
::testing::TestInfo* RegisterLambdaTest(const char* suite, const char* name, Fn fn) {
    return ::testing::RegisterTest(
        suite,
        name,
        nullptr,
        nullptr,
        __FILE__,
        __LINE__,
        [fn]() -> ::testing::Test* { return new LambdaTest<Fn>(fn); });
}

void W1_InitFromServerConfig_NoLLMSection() {
    json config = {{"database", {{"path", "/tmp/db"}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

void W2_InitFromServerConfig_EmptyModelPath() {
    json config = {{"llm", {{"model_path", ""}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

void W3_InitFromServerConfig_NoModelPathKey() {
    json config = {{"llm", {{"n_ctx", 4096}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

void W4_DefaultReloadCallback_EmptyConfig() {
    auto cb = LlamaCppPluginRegistrar::defaultReloadCallback();
    LlamaCppPlugin plugin;
    EXPECT_TRUE(cb(plugin, json::object()));
}

void W5_RegisterWithLLMManager_StubMode() {
    auto& mgr = LLMPluginManager::instance();
    const bool ok = LlamaCppPluginRegistrar::registerWithLLMManager(
        mgr, "llama_cpp_test_w5", json::object());
    EXPECT_TRUE(ok);
}

void W6_RegisteredPlugin_CanGenerate() {
    auto plugin = LlamaCppPluginRegistrar::createPlugin(json::object());
    ASSERT_NE(plugin, nullptr);
    plugin->loadModel("", json::object());

    InferenceRequest req;
    req.prompt = "hello from W6";
    auto resp = plugin->generate(req);
    // In stub mode the plugin must either succeed or report an error message —
    // it must not throw or crash.
    EXPECT_TRUE(resp.success || !resp.error_message.empty());
}

void W7_InitFromServerConfig_WithModelPath() {
    json config = {{"llm", {{"model_path", "/stub/model.gguf"}, {"n_ctx", 512}}}};
    // In THEMIS_LLAMA_CPP_STUB_MODE loadModel() always succeeds, so this
    // must return true even though the path is fake.
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

void W8_CreateLlamaWrapper_EmptyModelPath_FailsClosed() {
    auto& mgr = LLMPluginManager::instance();
    const std::string plugin_name = "unloaded_backend_probe";

    const bool ok = createLlamaWrapper(plugin_name, "", json::object());
    EXPECT_FALSE(ok);
    EXPECT_FALSE(mgr.hasPlugin(plugin_name));
}

void W9_InitFromServerConfig_Idempotent() {
    json config = {{"llm", {{"model_path", ""}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

bool RegisterAllTests() {
    bool ok = true;
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W1_InitFromServerConfig_NoLLMSection", &W1_InitFromServerConfig_NoLLMSection) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W2_InitFromServerConfig_EmptyModelPath", &W2_InitFromServerConfig_EmptyModelPath) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W3_InitFromServerConfig_NoModelPathKey", &W3_InitFromServerConfig_NoModelPathKey) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W4_DefaultReloadCallback_EmptyConfig", &W4_DefaultReloadCallback_EmptyConfig) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W5_RegisterWithLLMManager_StubMode", &W5_RegisterWithLLMManager_StubMode) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W6_RegisteredPlugin_CanGenerate", &W6_RegisteredPlugin_CanGenerate) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W7_InitFromServerConfig_WithModelPath", &W7_InitFromServerConfig_WithModelPath) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W8_CreateLlamaWrapper_EmptyModelPath_FailsClosed", &W8_CreateLlamaWrapper_EmptyModelPath_FailsClosed) != nullptr);
    ok = ok && (RegisterLambdaTest("LlamaCppRegistrarIntegrationTests", "W9_InitFromServerConfig_Idempotent", &W9_InitFromServerConfig_Idempotent) != nullptr);
    return ok;
}

void RegisterLlamaCppRegistrarIntegrationTestsAtStartup() {
    std::fprintf(stderr, "[llama_cpp registrar focused] startup initializer reached\n");
    const bool registered = RegisterAllTests();
    std::fprintf(stderr, "[llama_cpp registrar focused] RegisterAllTests=%d total_test_count=%d\n",
                 registered ? 1 : 0,
                 static_cast<int>(::testing::UnitTest::GetInstance()->total_test_count()));
    std::fflush(stderr);
}

}  // namespace

extern "C" void themis_llama_cpp_registrar_integration_tests_force_link() {}
#pragma comment(linker, "/include:themis_llama_cpp_registrar_integration_tests_force_link")

#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void(__cdecl* themis_llama_cpp_registrar_integration_tests_init)(void) =
    &RegisterLlamaCppRegistrarIntegrationTestsAtStartup;

// ── Group W — Server-startup registrar integration ────────────────────────────
