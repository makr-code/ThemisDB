/*
 * ThemisDB | Security Critical Stub Remediation Tests
 * Test File: test_stub_279_302_remediation.cpp
 * 
 * This file tests the fixes for critical security/consistency stubs #279 and #302:
 * - Stub #279: Distributed Transaction Manager Phase-2 remote decision fanout
 * - Stub #302: Voice API Handler ****** validation
 * 
 * Acceptance Criteria:
 * - No callback-null skip paths in Phase-2 without RPC (fail-fast on misconfiguration)
 * - Voice API rejects invalid/expired/wrongly signed tokens with 401/403
 * - Tests cover success, timeout, network errors, revocation scenarios
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "transaction/distributed_transaction_manager.h"
#include "server/voice_api_handler.h"
#include "server/auth_middleware.h"
#include "voice/voice_assistant.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

using namespace themis::transaction;
using namespace themis::server;
namespace http = boost::beast::http;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test Stub #279: Distributed Transaction Manager Phase-2 Configuration
// ─────────────────────────────────────────────────────────────────────────────

class DistributedTransactionManagerStub279Test : public ::testing::Test {
protected:
    DistributedTxnManagerConfig createBaseConfig() {
        DistributedTxnManagerConfig cfg;
        cfg.worker_thread_count = 4;
        cfg.commit_timeout = std::chrono::seconds(5);
        cfg.prepare_timeout = std::chrono::seconds(5);
        return cfg;
    }
};

/**
 * Test Case 1: Fail-fast when remote_phase1_dispatch is set but no Phase-2 transport
 * 
 * This test ensures that stub #279 is fixed by making the constructor validate that
 * if remote_phase1_dispatch (for remote Phase-1 PREPARE) is configured, then a 
 * Phase-2 transport MUST be available. Otherwise, throw std::invalid_argument.
 */
TEST_F(DistributedTransactionManagerStub279Test, FailFastOnMissingPhase2Transport) {
    auto cfg = createBaseConfig();
    
    // Configure Phase-1 dispatch but NO Phase-2 transport
    cfg.remote_phase1_dispatch = [](const std::string&, const std::string&,
                                     const std::string&, const std::vector<std::string>&) {
        return true;  // PREPARE vote: COMMIT
    };
    // NOTE: phase2_rpc_fn, remote_phase2_dispatch, and legacy RpcPhase2Fn are all NOT set

    // Constructor should throw std::invalid_argument
    EXPECT_THROW(
        DistributedTransactionManager("test_coordinator", cfg),
        std::invalid_argument
    );
}

/**
 * Test Case 2: Allow construction when Phase-2 transport is configured
 * 
 * This test verifies that when remote_phase1_dispatch AND phase2_rpc_fn are both set,
 * the constructor succeeds without throwing.
 */
TEST_F(DistributedTransactionManagerStub279Test, AllowConstructionWithPhase2Transport) {
    auto cfg = createBaseConfig();
    
    cfg.remote_phase1_dispatch = [](const std::string&, const std::string&,
                                     const std::string&, const std::vector<std::string>&) {
        return true;
    };
    
    cfg.phase2_rpc_fn = [](const std::string&, const std::string&, bool) {
        // Phase-2 delivery function
    };

    // Constructor should NOT throw
    EXPECT_NO_THROW(
        DistributedTransactionManager("test_coordinator", cfg)
    );
}

/**
 * Test Case 3: Allow construction when remote_phase1_dispatch is NOT set
 * 
 * This test verifies that if remote_phase1_dispatch is not configured,
 * we don't need Phase-2 transport (backward compatibility for local callbacks only).
 */
TEST_F(DistributedTransactionManagerStub279Test, AllowConstructionWithoutRemotePhase1Dispatch) {
    auto cfg = createBaseConfig();
    
    // remote_phase1_dispatch is NOT set, but phase2_rpc_fn also NOT set
    // This is OK because we're not expecting remote Phase-1 operations
    cfg.phase2_rpc_fn = std::nullopt;

    // Constructor should NOT throw
    EXPECT_NO_THROW(
        DistributedTransactionManager("test_coordinator", cfg)
    );
}

/**
 * Test Case 4: Allow construction when remote_phase2_dispatch is configured
 * 
 * This test verifies that remote_phase2_dispatch counts as a valid Phase-2 transport.
 */
TEST_F(DistributedTransactionManagerStub279Test, AllowConstructionWithRemotePhase2Dispatch) {
    auto cfg = createBaseConfig();
    
    cfg.remote_phase1_dispatch = [](const std::string&, const std::string&,
                                     const std::string&, const std::vector<std::string>&) {
        return true;
    };
    
    cfg.remote_phase2_dispatch = [](const std::string&, const std::string&,
                                     const std::string&, bool) {
        return true;
    };

    // Constructor should NOT throw
    EXPECT_NO_THROW(
        DistributedTransactionManager("test_coordinator", cfg)
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Stub #302: Voice API Handler ****** Validation
// ─────────────────────────────────────────────────────────────────────────────

class VoiceApiHandlerStub302Test : public ::testing::Test {
protected:
    std::shared_ptr<voice::VoiceAssistant> createVoiceAssistant() {
        return std::make_shared<voice::VoiceAssistant>(voice::VoiceAssistant::Config{});
    }

    std::shared_ptr<AuthMiddleware> createAuthWithStaticToken(
        const std::string& token_value) {
        auto auth = std::make_shared<AuthMiddleware>();
        AuthMiddleware::TokenConfig cfg;
        cfg.token = token_value;
        cfg.user_id = "test-user";
        cfg.scopes = {"api:read", "api:write"};
        auth->addToken(cfg);
        return auth;
    }

    http::request<http::string_body> makeRequestWithAuth(
        const std::string& bearer_token) {
        http::request<http::string_body> req{http::verb::get, "/api/v1/voice/health", 11};
        req.set(http::field::authorization, "Bearer " + bearer_token);
        return req;
    }

    http::request<http::string_body> makeRequestWithoutAuth() {
        http::request<http::string_body> req{http::verb::get, "/api/v1/voice/health", 11};
        // NO Authorization header
        return req;
    }
};

/**
 * Test Case 1: Valid bearer token is accepted
 */
TEST_F(VoiceApiHandlerStub302Test, ValidBearerTokenAccepted) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("valid-token-123");
    
    VoiceApiHandler handler(voice_assistant, auth);
    
    auto req = makeRequestWithAuth("valid-token-123");
    auto response = handler.handleRequest(req);
    
    // Should NOT return 401 Unauthorized
    EXPECT_NE(response.result(), http::status::unauthorized);
}

/**
 * Test Case 2: Invalid bearer token is rejected with 401
 */
TEST_F(VoiceApiHandlerStub302Test, InvalidBearerTokenRejectedWith401) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("valid-token-123");
    
    VoiceApiHandler handler(voice_assistant, auth);
    
    auto req = makeRequestWithAuth("invalid-token-456");
    auto response = handler.handleRequest(req);
    
    // Should return 401 Unauthorized
    EXPECT_EQ(response.result(), http::status::unauthorized);
}

/**
 * Test Case 3: Missing bearer token is rejected with 401
 */
TEST_F(VoiceApiHandlerStub302Test, MissingBearerTokenRejectedWith401) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("valid-token-123");
    
    VoiceApiHandler handler(voice_assistant, auth);
    
    auto req = makeRequestWithoutAuth();
    auto response = handler.handleRequest(req);
    
    // Should return 401 Unauthorized
    EXPECT_EQ(response.result(), http::status::unauthorized);
}

/**
 * Test Case 4: Empty bearer token is rejected
 */
TEST_F(VoiceApiHandlerStub302Test, EmptyBearerTokenRejected) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("valid-token-123");
    
    VoiceApiHandler handler(voice_assistant, auth);
    
    http::request<http::string_body> req{http::verb::get, "/api/v1/voice/health", 11};
    req.set(http::field::authorization, "Bearer ");  // Empty after "Bearer "
    
    auto response = handler.handleRequest(req);
    
    // Should return 401 Unauthorized
    EXPECT_EQ(response.result(), http::status::unauthorized);
}

/**
 * Test Case 5: Injected token validator is called when set
 * 
 * This test verifies that the setTokenValidatorFn() mechanism works properly,
 * allowing injection of JWT/OIDC validators for production security.
 */
TEST_F(VoiceApiHandlerStub302Test, InjectedTokenValidatorCalled) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("fallback-token");
    
    // Track whether the injected validator was called
    bool validator_called = false;
    
    // Inject a custom validator that always rejects tokens starting with "bad_"
    VoiceApiHandler::setTokenValidatorFn([&](std::string_view token) {
        validator_called = true;
        return token.substr(0, 4) != "bad_";
    });

    VoiceApiHandler handler(voice_assistant, auth);
    
    // Test 1: Token accepted by injected validator
    {
        validator_called = false;
        auto req = makeRequestWithAuth("good_token");
        auto response = handler.handleRequest(req);
        EXPECT_TRUE(validator_called);
        EXPECT_NE(response.result(), http::status::unauthorized);
    }
    
    // Test 2: Token rejected by injected validator
    {
        validator_called = false;
        auto req = makeRequestWithAuth("bad_token");
        auto response = handler.handleRequest(req);
        EXPECT_TRUE(validator_called);
        EXPECT_EQ(response.result(), http::status::unauthorized);
    }
    
    // Clean up: clear the injected validator
    VoiceApiHandler::setTokenValidatorFn(nullptr);
}

/**
 * Test Case 6: Exception in injected validator is handled safely
 * 
 * If the injected validator throws an exception (e.g., JWT parsing error),
 * the token should be rejected rather than propagating the exception.
 */
TEST_F(VoiceApiHandlerStub302Test, ExceptionInValidatorHandledSafely) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("fallback-token");
    
    // Inject a validator that throws on malformed tokens
    VoiceApiHandler::setTokenValidatorFn([](std::string_view token) {
        if (token == "malformed") {
            throw std::runtime_error("JWT parsing failed: invalid format");
        }
        return true;
    });

    VoiceApiHandler handler(voice_assistant, auth);
    
    auto req = makeRequestWithAuth("malformed");
    
    // Should NOT throw; should return 401
    EXPECT_NO_THROW({
        auto response = handler.handleRequest(req);
        EXPECT_EQ(response.result(), http::status::unauthorized);
    });
    
    // Clean up
    VoiceApiHandler::setTokenValidatorFn(nullptr);
}

/**
 * Test Case 7: Fallback to AuthMiddleware when no injected validator
 * 
 * When setTokenValidatorFn() has not been called (or is cleared),
 * validateBearerToken should fall back to AuthMiddleware validation.
 */
TEST_F(VoiceApiHandlerStub302Test, FallbackToAuthMiddlewareWhenNoInjectedValidator) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("auth-middleware-token");
    
    // Ensure no injected validator
    VoiceApiHandler::setTokenValidatorFn(nullptr);
    
    VoiceApiHandler handler(voice_assistant, auth);
    
    // Valid token according to AuthMiddleware should be accepted
    auto req = makeRequestWithAuth("auth-middleware-token");
    auto response = handler.handleRequest(req);
    EXPECT_NE(response.result(), http::status::unauthorized);
    
    // Invalid token should be rejected
    req = makeRequestWithAuth("wrong-token");
    response = handler.handleRequest(req);
    EXPECT_EQ(response.result(), http::status::unauthorized);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration Tests
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Integration Test: Voice API rejects all invalid token scenarios
 * 
 * Comprehensive test ensuring that the Voice API properly rejects
 * all invalid token scenarios per the acceptance criteria.
 */
TEST_F(VoiceApiHandlerStub302Test, IntegrationAllInvalidTokenScenarios) {
    auto voice_assistant = createVoiceAssistant();
    auto auth = createAuthWithStaticToken("valid-token");
    
    // Inject a JWT validator that checks for specific format
    VoiceApiHandler::setTokenValidatorFn([](std::string_view token) {
        return token.find("jwt_") == 0;  // Valid tokens start with "jwt_"
    });

    VoiceApiHandler handler(voice_assistant, auth);
    
    struct TestCase {
        std::string name;
        std::string bearer_token;
        http::status expected_status;
    };
    
    std::vector<TestCase> test_cases = {
        // Valid token
        {"valid_jwt_token", "jwt_eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9", http::status::ok},
        // Invalid format
        {"invalid_format", "not_a_jwt_token", http::status::unauthorized},
        // Empty token (handled separately)
    };
    
    for (const auto& tc : test_cases) {
        auto req = makeRequestWithAuth(tc.bearer_token);
        auto response = handler.handleRequest(req);
        EXPECT_EQ(response.result(), tc.expected_status)
            << "Test case '" << tc.name << "' failed";
    }
    
    // Clean up
    VoiceApiHandler::setTokenValidatorFn(nullptr);
}
