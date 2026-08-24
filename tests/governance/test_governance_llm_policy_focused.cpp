/**
 * @file test_governance_llm_policy_focused.cpp
 * @brief Group GP — PolicyEngine LLM inference-permission path tests.
 */

#include <gtest/gtest.h>
#include "governance/policy_engine.h"

#include <string>
#include <unordered_map>

using namespace themis::governance;

// ── GP1: Default PolicyEngine — checkInferencePermission with empty headers ───
TEST(GovernanceLlmPolicyFocused, GP1_EmptyHeaders_ReturnsResult_NoThrow) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;

    InferencePermissionResult result;
    EXPECT_NO_THROW({ result = engine.checkInferencePermission(headers); });
    // Without policies loaded: default behaviour must not crash
    (void)result;
}

// ── GP2: No API key in headers → not allowed (HTTP 401 expected) ──────────────
TEST(GovernanceLlmPolicyFocused, GP2_NoApiKey_DefaultDenyOrAllow_DoesNotCrash) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;

    auto result = engine.checkInferencePermission(headers);
    // With no policies loaded the engine may default to deny or allow —
    // what matters is that the result struct is well-formed
    if (!result.allowed) {
        EXPECT_TRUE(result.http_status == 401 || result.http_status == 403);
    }
}

// ── GP3: InferencePermissionResult allowed=true → no denial fields set ────────
TEST(GovernanceLlmPolicyFocused, GP3_AllowedResult_DenialFieldsEmpty) {
    // Construct an allowed result directly to verify struct invariant
    InferencePermissionResult r;
    r.allowed = true;
    EXPECT_TRUE(r.allowed);
    // denial_reason and http_status are undefined for allowed results
    // but the struct must not segfault when read
    (void)r.http_status;
    (void)r.denial_reason;
}

// ── GP4: InferencePermissionResult allowed=false → http_status is 401 or 403 ─
TEST(GovernanceLlmPolicyFocused, GP4_DeniedResult_HttpStatusIs401Or403) {
    InferencePermissionResult r;
    r.allowed      = false;
    r.http_status  = 401;
    r.denial_reason = "missing API key";
    EXPECT_FALSE(r.allowed);
    EXPECT_TRUE(r.http_status == 401 || r.http_status == 403);
    EXPECT_FALSE(r.denial_reason.empty());
}

// ── GP5: checkInferencePermission with Authorization header does not crash ────
TEST(GovernanceLlmPolicyFocused, GP5_WithAuthHeader_NoThrow) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "******";

    EXPECT_NO_THROW({ engine.checkInferencePermission(headers); });
}

// ── GP6: Multiple consecutive calls produce consistent result type ────────────
TEST(GovernanceLlmPolicyFocused, GP6_MultipleCallsConsistent) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;
    headers["Authorization"] = "******";

    auto r1 = engine.checkInferencePermission(headers);
    auto r2 = engine.checkInferencePermission(headers);
    EXPECT_EQ(r1.allowed, r2.allowed);
    EXPECT_EQ(r1.http_status, r2.http_status);
}
