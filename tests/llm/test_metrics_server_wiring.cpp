/**
 * @file test_metrics_server_wiring.cpp
 * @brief Tests for LLMPluginManager::wireMetricsServerCallbacks() (MSW-01..10)
 *
 * Root-cause fix: MetricsServer admin endpoints (reload, simulate, session-delete)
 * previously returned {"status":"not_implemented"} because
 * LLMPluginManager::wireMetricsServerCallbacks() did not exist.  After the fix
 * the three endpoints are connected to:
 *
 *   POST /admin/models/reload    → mgr.loadModel(model_id, path)
 *   POST /admin/prompt/simulate  → estimateTokens(prompt)
 *   DELETE /admin/sessions/{id}  → cancel_session_cb_(id)
 *
 * Test strategy:
 *   MetricsServer::handlePost() and handleDelete() are now public (they have
 *   always been called from the Impl route handlers; we just exposed them so
 *   unit tests can drive them without starting an HTTP listener).
 *
 *  MSW-01: wireMetricsServerCallbacks() does not throw.
 *  MSW-02: reload with valid JSON → {"status":"ok"} + model_id field.
 *  MSW-03: reload with malformed JSON → {"status":"error"}.
 *  MSW-04: reload with missing model_id field → {"status":"error"}.
 *  MSW-05: simulate with valid prompt → {"status":"ok"} + estimated_tokens >= 1.
 *  MSW-06: simulate with empty/missing prompt → {"status":"error"}.
 *  MSW-07: simulate token count scales with prompt length.
 *  MSW-08: session-delete without cancel_cb → {"status":"not_configured"}.
 *  MSW-09: session-delete with cancel_cb, known session → {"status":"ok"}.
 *  MSW-10: session-delete with cancel_cb, unknown session → {"status":"not_found"}.
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_manager.h"
#include "llm/grafana_metrics.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>

using json = nlohmann::json;

namespace themis {
namespace llm {
namespace monitoring {

// ── Minimal fake plugin ───────────────────────────────────────────────────────

struct FakePlugin final : public ILLMPlugin {
    bool   load_called  = false;
    std::string last_path;

    bool loadModel(const std::string& path,
                   [[maybe_unused]] const nlohmann::json&) override {
        load_called = true;
        last_path   = path;
        return true;
    }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override { return std::nullopt; }
    bool isModelLoaded() const override { return false; }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    InferenceResponse generate(const InferenceRequest&) override { return {}; }
    InferenceResponse generateRAG(const RAGContext&,
                                  const InferenceRequest&) override { return {}; }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    nlohmann::json getMemoryStats() const override { return {}; }
    nlohmann::json getPerformanceStats() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
};

// ── Helpers ───────────────────────────────────────────────────────────────────

static MetricsServer makeServer()
{
    MetricsServer::ServerConfig cfg;
    return MetricsServer{cfg, nullptr};
}

static std::string reloadPost(MetricsServer& srv, const std::string& body)
{
    std::string resp;
    srv.handlePost(srv.serverConfig().admin_reload_path, body, resp);
    return resp;
}

static std::string simulatePost(MetricsServer& srv, const std::string& body)
{
    std::string resp;
    srv.handlePost(srv.serverConfig().admin_simulate_path, body, resp);
    return resp;
}

static std::string sessionDelete(MetricsServer& srv, const std::string& sid)
{
    std::string resp;
    srv.handleDelete(srv.serverConfig().admin_sessions_path, sid, resp);
    return resp;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class MSW_Tests : public ::testing::Test {
protected:
    LLMPluginManager mgr;
    FakePlugin*      plugin = nullptr;  // non-owning

    void SetUp() override {
        auto p = std::make_unique<FakePlugin>();
        plugin = p.get();
        mgr.registerPlugin("fake", std::move(p));
    }
};

// ── MSW-01: wiring does not throw ────────────────────────────────────────────

TEST_F(MSW_Tests, MSW_01_WiringDoesNotThrow)
{
    auto srv = makeServer();
    EXPECT_NO_THROW(mgr.wireMetricsServerCallbacks(srv));
}

// ── MSW-02: valid reload body → ok ───────────────────────────────────────────

TEST_F(MSW_Tests, MSW_02_ValidReloadBodyReturnsOk)
{
    auto srv = makeServer();
    mgr.wireMetricsServerCallbacks(srv);

    const std::string body =
        json{{"model_id", "my-model"}, {"path", "my-model.gguf"}}.dump();
    const json resp = json::parse(reloadPost(srv, body));

    EXPECT_EQ(resp.value("status", ""), "ok");
    EXPECT_EQ(resp.value("model_id", ""), "my-model");
    EXPECT_TRUE(plugin->load_called);
    EXPECT_EQ(plugin->last_path, "my-model.gguf");
}

// ── MSW-03: malformed JSON body → error ──────────────────────────────────────

TEST_F(MSW_Tests, MSW_03_MalformedReloadBodyReturnsError)
{
    auto srv = makeServer();
    mgr.wireMetricsServerCallbacks(srv);

    const json err = json::parse(reloadPost(srv, "not-valid-json{{"));
    EXPECT_EQ(err.value("status", ""), "error");
    EXPECT_FALSE(plugin->load_called);
}

// ── MSW-04: missing model_id field → error ───────────────────────────────────

TEST_F(MSW_Tests, MSW_04_MissingModelIdReturnsError)
{
    auto srv = makeServer();
    mgr.wireMetricsServerCallbacks(srv);

    // Valid JSON but no "model_id" or "model" key.
    const json err = json::parse(reloadPost(srv, json{{"path", "some.gguf"}}.dump()));
    EXPECT_EQ(err.value("status", ""), "error");
    EXPECT_FALSE(plugin->load_called);
}

// ── MSW-05: valid prompt → ok + estimated_tokens ≥ 1 ────────────────────────

TEST_F(MSW_Tests, MSW_05_SimulateValidPromptReturnsOk)
{
    auto srv = makeServer();
    mgr.wireMetricsServerCallbacks(srv);

    const std::string prompt = "Hello, this is a test prompt for token estimation.";
    const json resp = json::parse(
        simulatePost(srv, json{{"prompt", prompt}}.dump()));

    EXPECT_EQ(resp.value("status", ""), "ok");
    ASSERT_TRUE(resp.contains("estimated_tokens"));
    EXPECT_GE(resp["estimated_tokens"].get<size_t>(), 1u);
    EXPECT_EQ(resp.value("method", ""), "CHAR_HEURISTIC");
    EXPECT_EQ(resp.value("prompt_chars", size_t{0}), prompt.size());
}

// ── MSW-06: missing/empty prompt → error ─────────────────────────────────────

TEST_F(MSW_Tests, MSW_06_SimulateMissingPromptReturnsError)
{
    auto srv = makeServer();
    mgr.wireMetricsServerCallbacks(srv);

    // No "prompt" key.
    const json err1 = json::parse(
        simulatePost(srv, json{{"model_id", "m1"}}.dump()));
    EXPECT_EQ(err1.value("status", ""), "error");

    // Invalid JSON.
    const json err2 = json::parse(simulatePost(srv, "garbage"));
    EXPECT_EQ(err2.value("status", ""), "error");
}

// ── MSW-07: simulate token count scales with prompt length ────────────────────

TEST_F(MSW_Tests, MSW_07_SimulateTokenCountScalesWithLength)
{
    auto srv = makeServer();
    mgr.wireMetricsServerCallbacks(srv);

    const json short_resp = json::parse(
        simulatePost(srv, json{{"prompt", "Hi"}}.dump()));
    const json long_resp = json::parse(
        simulatePost(srv, json{{"prompt", std::string(300, 'A')}}.dump()));

    EXPECT_EQ(short_resp.value("status", ""), "ok");
    EXPECT_EQ(long_resp.value("status", ""), "ok");

    EXPECT_LT(short_resp["estimated_tokens"].get<size_t>(),
              long_resp["estimated_tokens"].get<size_t>())
        << "Longer prompts must produce more estimated tokens";
}

// ── MSW-08: no cancel_cb set → not_configured ────────────────────────────────

TEST_F(MSW_Tests, MSW_08_SessionDeleteNotConfiguredWhenNoCancelCb)
{
    auto srv = makeServer();
    // Wire WITHOUT setting a cancel session callback.
    mgr.wireMetricsServerCallbacks(srv);

    const json resp = json::parse(sessionDelete(srv, "req-42"));
    EXPECT_EQ(resp.value("status", ""), "not_configured");
    EXPECT_TRUE(resp.contains("message"))
        << "Should explain how to wire the cancel callback";
}

// ── MSW-09: cancel_cb set, known session → ok ────────────────────────────────

TEST_F(MSW_Tests, MSW_09_SessionDeleteDelegatesToCancelCbKnownSession)
{
    auto srv = makeServer();

    bool called = false;
    std::string seen_sid;
    mgr.setCancelSessionCallback(
        [&called, &seen_sid](const std::string& sid) -> bool {
            called   = true;
            seen_sid = sid;
            return sid == "req-99";
        });
    mgr.wireMetricsServerCallbacks(srv);

    const json resp = json::parse(sessionDelete(srv, "req-99"));
    EXPECT_TRUE(called);
    EXPECT_EQ(seen_sid, "req-99");
    EXPECT_EQ(resp.value("status", ""), "ok");
    EXPECT_EQ(resp.value("session_id", ""), "req-99");
}

// ── MSW-10: cancel_cb set, unknown session → not_found ───────────────────────

TEST_F(MSW_Tests, MSW_10_SessionDeleteDelegatesToCancelCbUnknownSession)
{
    auto srv = makeServer();

    mgr.setCancelSessionCallback(
        [](const std::string& sid) -> bool {
            return sid == "req-99";  // only this one "exists"
        });
    mgr.wireMetricsServerCallbacks(srv);

    const json resp = json::parse(sessionDelete(srv, "req-404"));
    EXPECT_EQ(resp.value("status", ""), "not_found");
}

} // namespace monitoring
} // namespace llm
} // namespace themis
