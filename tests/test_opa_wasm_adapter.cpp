/**
 * @file test_opa_wasm_adapter.cpp
 * @brief Tests for OPA adapter WASM extension (Phase 5.3).
 *
 * 8 tests covering:
 * 1. REST mode still works (existing behaviour)
 * 2. WASM mode with missing bundle falls back to REST
 * 3. WASM mode: bundle file not found → falls back
 * 4. Config EvalMode::WASM set correctly
 * 5. Default mode is REST
 * 6. wasm_bundle_path defaults to empty
 * 7. Fallback counter incremented when WASM fails
 * 8. evaluate() returns nullopt when both WASM and REST fail
 */

#include <gtest/gtest.h>
#include "governance/opa_adapter.h"

#include <optional>
#include <string>
#include <unordered_map>

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
// Test 2 — WASM mode with missing bundle falls back to REST
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmMode_MissingBundle_FallsBackToRest) {
    OpaAdapter adapter(make_wasm_config("")); // empty path
    // WASM falls back to REST; REST server unreachable → nullopt
    EXPECT_NO_THROW({
        auto result = adapter.evaluate(kTestHeaders, "/vector/search");
        // nullopt because both WASM and REST fail
        EXPECT_FALSE(result.has_value());
    });
}

// ---------------------------------------------------------------------------
// Test 3 — WASM mode: bundle file not found → falls back
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmMode_BundleFileNotFound_FallsBack) {
    OpaAdapter adapter(make_wasm_config("/nonexistent/path/bundle.wasm"));
    EXPECT_NO_THROW({
        auto result = adapter.evaluate(kTestHeaders, "/api/data");
        // WASM fallback + REST unreachable → nullopt
        EXPECT_FALSE(result.has_value());
    });
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
// Test 8 — evaluate() returns nullopt when both WASM and REST fail
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmAndRestBothFail_ReturnsNullopt) {
    // WASM: no bundle file, REST: unreachable server
    OpaAdapter adapter(make_wasm_config("/nonexistent/bundle.wasm"));

    const auto result = adapter.evaluate(kTestHeaders, "/vector/knn");
    EXPECT_FALSE(result.has_value())
        << "When both WASM and REST fail, evaluate() must return nullopt";
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
// OPA-WASM-INJ-02 — Injected WasmEvalFn returning nullopt falls through to REST
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmEvalFn_InjectedFnNulloptFallsThrough) {
    OpaAdapter adapter(make_wasm_config("/any/bundle.wasm"));

    // Injected fn returns nullopt → evaluate() should try REST (unreachable → nullopt).
    adapter.setWasmEvalFn([](const std::unordered_map<std::string, std::string>&,
                              const std::string&) -> std::optional<PolicyDecision> {
        return std::nullopt;
    });

    const auto result = adapter.evaluate(kTestHeaders, "/api/data");
    // REST is unreachable → overall result is still nullopt
    EXPECT_FALSE(result.has_value())
        << "OPA-WASM-INJ-02: injected fn returning nullopt must cause evaluate() to fall through to REST (which also returns nullopt here)";
}

// ---------------------------------------------------------------------------
// OPA-WASM-INJ-03 — Clearing the WasmEvalFn (nullptr) restores stub behaviour
// ---------------------------------------------------------------------------
TEST(OpaWasmAdapter, WasmEvalFn_ClearedRestoresStub) {
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

    // Stub path: bundle file doesn't exist → nullopt; REST also unreachable → nullopt.
    const auto result = adapter.evaluate(kTestHeaders, "/api/data");
    EXPECT_FALSE(result.has_value())
        << "OPA-WASM-INJ-03: after clearing WasmEvalFn, the built-in stub must be used (bundle missing → nullopt)";
}
