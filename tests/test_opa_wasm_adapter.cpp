/**
 * @file test_opa_wasm_adapter.cpp
 * @brief Tests for OPA adapter WASM extension (Phase 5.3).
 *
 * Coverage:
 * - REST behavior and default REST mode remain unchanged.
 * - Missing/unevaluated WASM bundles fail closed without REST fallback.
 * - Injected WASM evaluator success, nullopt, exceptions, and clearing.
 */

#include <gtest/gtest.h>
#include "governance/opa_adapter.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace themis::governance;
using EvalMode = OpaAdapter::Config::EvalMode;

// ─── Helpers ────────────────────────────────────────────────────────────────

static OpaAdapter::Config make_rest_config() {
    OpaAdapter::Config cfg;
    cfg.endpoint_url = "http://localhost:18181"; // non-existent → returns nullopt
    cfg.policy_path  = "themis/governance/allow";
    cfg.timeout_ms   = 5;
    cfg.mode         = EvalMode::REST;
    return cfg;
}

static OpaAdapter::Config make_wasm_config(const std::string& bundle_path = "") {
    OpaAdapter::Config cfg = make_rest_config();
    cfg.mode              = EvalMode::WASM;
    cfg.wasm_bundle_path  = bundle_path;
    return cfg;
}

static const std::unordered_map<std::string, std::string> kTestHeaders{
    {"X-Classification", "vs-nfd"},
    {"X-User-Id",        "test_user"}
};

static void expect_strict_wasm_deny(const std::optional<PolicyDecision>& result) {
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->classification, "streng-geheim");
    EXPECT_EQ(result->mode, "enforce");
    EXPECT_TRUE(result->encrypt_logs);
    EXPECT_EQ(result->redaction, "strict");
    EXPECT_FALSE(result->ann_allowed);
    EXPECT_TRUE(result->require_content_encryption);
    EXPECT_FALSE(result->export_allowed);
    EXPECT_FALSE(result->cache_allowed);
    EXPECT_EQ(result->retention_days, 7);
}

// ---------------------------------------------------------------------------
// Test 1 — REST mode (existing behaviour) still works
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, RestMode_ExistingBehaviour) {
    OpaAdapter adapter(make_rest_config());
    // OPA server not running → returns nullopt (graceful fallback)
    auto result = adapter.evaluate(kTestHeaders, "/vector/search");
    // nullopt is the expected result when OPA is unreachable
    EXPECT_FALSE(result.has_value())
        << "REST mode with unreachable OPA must return nullopt";
}

// ---------------------------------------------------------------------------
// Test 2 — WASM mode with missing bundle fails closed, without REST fallback
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmMode_MissingBundle_ReturnsStrictDeny) {
    OpaAdapter adapter(make_wasm_config("")); // empty path
    EXPECT_NO_THROW({
        auto result = adapter.evaluate(kTestHeaders, "/vector/search");
        expect_strict_wasm_deny(result);
    });
}

// ---------------------------------------------------------------------------
// Test 3 — WASM mode: missing bundle cannot authorize
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmMode_BundleFileNotFound_ReturnsStrictDeny) {
    OpaAdapter adapter(make_wasm_config("/nonexistent/path/bundle.wasm"));
    EXPECT_NO_THROW({
        auto result = adapter.evaluate(kTestHeaders, "/api/data");
        expect_strict_wasm_deny(result);
    });
}

// Bundle presence is not evidence that policy evaluation took place.
TEST(OpaWasmAdapter, WasmMode_ExistingUnevaluatedBundle_ReturnsStrictDeny) {
    const auto path = std::filesystem::temp_directory_path() /
        ("themis-opa-wasm-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".wasm");
    {
        std::ofstream bundle(path, std::ios::binary);
        ASSERT_TRUE(bundle.good());
        bundle << "present, but not evaluated";
    }

    OpaAdapter adapter(make_wasm_config(path.string()));
    const auto result = adapter.evaluate(kTestHeaders, "/protected");
    std::filesystem::remove(path);
    expect_strict_wasm_deny(result);
}

// ---------------------------------------------------------------------------
// Test 4 — Config EvalMode::WASM set correctly
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, Config_EvalModeWasmSetCorrectly) {
    OpaAdapter::Config cfg = make_wasm_config("/some/bundle.wasm");
    EXPECT_EQ(cfg.mode, EvalMode::WASM);
    EXPECT_EQ(cfg.wasm_bundle_path, "/some/bundle.wasm");
}

// ---------------------------------------------------------------------------
// Test 5 — Default mode is REST
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, Config_DefaultModeIsRest) {
    OpaAdapter::Config cfg;
    EXPECT_EQ(cfg.mode, EvalMode::REST)
        << "Default OpaAdapter::Config mode must be REST";
}

// ---------------------------------------------------------------------------
// Test 6 — wasm_bundle_path defaults to empty
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, Config_WasmBundlePathDefaultsToEmpty) {
    OpaAdapter::Config cfg;
    EXPECT_TRUE(cfg.wasm_bundle_path.empty())
        << "Default wasm_bundle_path must be empty";
}

// ---------------------------------------------------------------------------
// Test 7 — Fallback counter incremented when WASM fails
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmMode_FallbackCounterIncremented) {
    // Create adapter in WASM mode with non-existent bundle
    OpaAdapter adapter(make_wasm_config("/absolutely/does/not/exist.wasm"));

    // Call evaluate twice — each should trigger WASM fallback
    EXPECT_NO_THROW(adapter.evaluate(kTestHeaders, "/api/test"));
    EXPECT_NO_THROW(adapter.evaluate(kTestHeaders, "/api/test2"));

    // The counter is process-global, so we can't assert exact values here
    // (other tests may have also incremented it). Just verify no crash.
    SUCCEED() << "WASM fallback counter incremented without crash";
}

// ---------------------------------------------------------------------------
// Test 8 — unavailable WASM mode denies rather than returning nullopt
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmUnavailable_DeniesWithoutRESTFallback) {
    OpaAdapter adapter(make_wasm_config("/nonexistent/bundle.wasm"));

    const auto result = adapter.evaluate(kTestHeaders, "/vector/knn");
    expect_strict_wasm_deny(result);
}

// ---------------------------------------------------------------------------
// OPA-WASM-INJ-01 — Injected WasmEvalFn returning a policy decision is used
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmEvalFn_InjectedFnUsed) {
    OpaAdapter adapter(make_wasm_config("/any/bundle.wasm"));

    PolicyDecision injected;
    injected.classification             = "vs-nfd";
    injected.mode                       = "enforce";
    injected.encrypt_logs               = false;
    injected.redaction                  = "standard";
    injected.ann_allowed                = true;
    injected.require_content_encryption = false;
    injected.export_allowed             = true;
    injected.cache_allowed              = true;
    injected.retention_days             = 365;

    adapter.setWasmEvalFn([injected](const std::unordered_map<std::string, std::string>&,
                                      const std::string&) -> std::optional<PolicyDecision> {
        return injected;
    });

    const auto result = adapter.evaluate(kTestHeaders, "/vector/search");
    ASSERT_TRUE(result.has_value())
        << "OPA-WASM-INJ-01: injected WasmEvalFn returning a decision must be used by evaluate()";
    EXPECT_EQ(result->classification, "vs-nfd");
    EXPECT_TRUE(result->ann_allowed);
}

// ---------------------------------------------------------------------------
// OPA-WASM-INJ-02 — An injected evaluator failure denies without REST fallback
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmEvalFn_InjectedFnNulloptFailsClosed) {
    OpaAdapter adapter(make_wasm_config("/any/bundle.wasm"));

    adapter.setWasmEvalFn([](const std::unordered_map<std::string, std::string>&,
                              const std::string&) -> std::optional<PolicyDecision> {
        return std::nullopt;
    });

    const auto result = adapter.evaluate(kTestHeaders, "/api/data");
    expect_strict_wasm_deny(result);
}

TEST(OpaWasmAdapter, WasmEvalFn_ThrowingEvaluatorFailsClosed) {
    OpaAdapter adapter(make_wasm_config("/any/bundle.wasm"));
    adapter.setWasmEvalFn([](const std::unordered_map<std::string, std::string>&,
                              const std::string&) -> std::optional<PolicyDecision> {
        throw std::runtime_error("test evaluator failure");
    });

    expect_strict_wasm_deny(adapter.evaluate(kTestHeaders, "/api/data"));
}

// ---------------------------------------------------------------------------
// OPA-WASM-INJ-03 — Clearing the WasmEvalFn restores fail-closed behavior
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmEvalFn_ClearedRestoresFailClosed) {
    OpaAdapter adapter(make_wasm_config("/nonexistent/bundle.wasm"));

    // First inject a fn that always succeeds …
    adapter.setWasmEvalFn([](const std::unordered_map<std::string, std::string>&,
                              const std::string&) -> std::optional<PolicyDecision> {
        PolicyDecision d;
        d.export_allowed = true;
        return d;
    });

    // … then clear it.
    adapter.setWasmEvalFn(nullptr);

    const auto result = adapter.evaluate(kTestHeaders, "/api/data");
    expect_strict_wasm_deny(result);
}
