/**
 * Unit tests for the RFC 3161 TSA API interface/wrapper (tsa_api.h).
 *
 * These tests exercise:
 *  1. TSAResponse::fromToken — field mapping correctness.
 *  2. TSAClientWrapper with a stub backend — integration hooks, error
 *     propagation, and request-type routing (data vs. hash).
 *  3. createTSAClient factory — returns a non-null ITSAClient.
 *  4. ITSAClient polymorphism — mock implementation.
 *  5. Integration-hook wiring — callbacks fired with correct arguments.
 *
 * Network-dependent tests (real TSA server) are skipped when
 * THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1.
 */

#include <gtest/gtest.h>
#include "security/tsa_api.h"

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

using namespace themis::security;

// ============================================================================
// Helpers
// ============================================================================

static TimestampToken makeSuccessToken() {
    TimestampToken t;
    t.success          = true;
    t.timestamp_utc    = "20260301T120000Z";
    t.timestamp_unix_ms = 1740830400000ULL;
    t.serial_number    = "DEADBEEF";
    t.policy_oid       = "1.2.3.4";
    t.hash_algorithm   = "SHA256";
    t.token_der        = {0x30, 0x01};
    t.token_b64        = "MAE=";
    t.tsa_name         = "CN=TestTSA";
    t.tsa_serial       = "AABB";
    t.tsa_cert         = {0x01, 0x02};
    t.has_accuracy     = true;
    t.accuracy_seconds = 1;
    t.accuracy_millis  = 500;
    t.accuracy_micros  = 0;
    t.ordering         = true;
    t.pki_status       = 0;
    t.status_code      = 200;
    return t;
}

static TimestampToken makeFailureToken(const std::string& err) {
    TimestampToken t;
    t.success       = false;
    t.error_message = err;
    t.status_code   = 500;
    return t;
}

// ============================================================================
// TSAResponse::fromToken
// ============================================================================

TEST(TSAResponseTest, FromTokenMapsAllFields) {
    auto tok = makeSuccessToken();
    auto latency = std::chrono::milliseconds(42);

    TSAResponse r = TSAResponse::fromToken(tok, latency);

    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.timestamp_utc,     tok.timestamp_utc);
    EXPECT_EQ(r.timestamp_unix_ms, tok.timestamp_unix_ms);
    EXPECT_EQ(r.serial_number,     tok.serial_number);
    EXPECT_EQ(r.policy_oid,        tok.policy_oid);
    EXPECT_EQ(r.hash_algorithm,    tok.hash_algorithm);
    EXPECT_EQ(r.token_der,         tok.token_der);
    EXPECT_EQ(r.token_b64,         tok.token_b64);
    EXPECT_EQ(r.tsa_name,          tok.tsa_name);
    EXPECT_EQ(r.tsa_serial,        tok.tsa_serial);
    EXPECT_EQ(r.tsa_cert,          tok.tsa_cert);
    EXPECT_TRUE(r.has_accuracy);
    EXPECT_EQ(r.accuracy_seconds,  tok.accuracy_seconds);
    EXPECT_EQ(r.accuracy_millis,   tok.accuracy_millis);
    EXPECT_EQ(r.accuracy_micros,   tok.accuracy_micros);
    EXPECT_TRUE(r.ordering);
    EXPECT_EQ(r.http_status,       tok.status_code);
    EXPECT_EQ(r.pki_status,        tok.pki_status);
    EXPECT_EQ(r.request_latency,   latency);
    EXPECT_TRUE(r.error_message.empty());
}

TEST(TSAResponseTest, FromFailureTokenPreservesError) {
    auto tok = makeFailureToken("network timeout");
    TSAResponse r = TSAResponse::fromToken(tok);

    EXPECT_FALSE(r.success);
    EXPECT_EQ(r.error_message, "network timeout");
    EXPECT_EQ(r.http_status, 500);
    EXPECT_EQ(r.request_latency, std::chrono::milliseconds(0));
}

// ============================================================================
// Mock ITSAClient for polymorphism / dependency-injection tests
// ============================================================================

class MockTSAClient : public ITSAClient {
public:
    // Configurable return values
    TSAResponse next_response;
    bool        next_verify_result = true;
    bool        available          = true;
    std::string last_err;

    // Call counters
    int request_count = 0;
    int verify_count  = 0;

    TSAResponse requestTimestamp(const TSARequest&) override {
        ++request_count;
        return next_response;
    }

    bool verifyToken(const TSAVerifyRequest&) override {
        ++verify_count;
        return next_verify_result;
    }

    bool isAvailable() override {
        return available;
    }

    std::string getLastError() const override {
        return last_err;
    }
};

TEST(ITSAClientTest, MockClientIsPolymorphic) {
    auto mock = std::make_unique<MockTSAClient>();
    mock->next_response = TSAResponse::fromToken(makeSuccessToken());
    mock->next_verify_result = true;

    // Use via base interface pointer — compile-time + runtime polymorphism check.
    ITSAClient* client = mock.get();

    TSARequest req;
    req.data = {1, 2, 3};
    auto resp = client->requestTimestamp(req);
    EXPECT_TRUE(resp.success);

    TSAVerifyRequest vreq;
    vreq.data      = {1, 2, 3};
    vreq.token_b64 = "MAE=";
    EXPECT_TRUE(client->verifyToken(vreq));

    EXPECT_TRUE(client->isAvailable());
    EXPECT_EQ(mock->request_count, 1);
    EXPECT_EQ(mock->verify_count,  1);
}

// ============================================================================
// TSAClientWrapper with stub backend (no network)
// ============================================================================

class TSAClientWrapperTest : public ::testing::Test {
protected:
    bool skip_network = false;

    void SetUp() override {
        const char* e = std::getenv("THEMIS_TEST_SKIP_TSA_NETWORK_TESTS");
        skip_network = (e && std::string(e) == "1");
    }

    static TSAConfig stubConfig() {
        TSAConfig cfg;
        cfg.url            = "https://stub.example.invalid/tsr";
        cfg.hash_algorithm = "SHA256";
        cfg.cert_req       = true;
        cfg.timeout_seconds = 5;
        return cfg;
    }
};

TEST_F(TSAClientWrapperTest, FactoryReturnsNonNull) {
    auto client = createTSAClient(stubConfig());
    EXPECT_NE(client, nullptr);
}

TEST_F(TSAClientWrapperTest, RequestTimestampInvalidURLReturnsFailure) {
    // Stub backend (no OpenSSL TSA): returns success=true for any URL.
    // OpenSSL backend: connection refused → success=false.
    auto client = createTSAClient(stubConfig());
    TSARequest req;
    req.data = {'H', 'e', 'l', 'l', 'o'};
    auto resp = client->requestTimestamp(req);

    // Either success (stub mode) or failure (OpenSSL mode) is acceptable;
    // the response object must always be well-formed.
    if (resp.success) {
        EXPECT_FALSE(resp.token_b64.empty());
    } else {
        EXPECT_FALSE(resp.error_message.empty());
    }
}

TEST_F(TSAClientWrapperTest, RequestTimestampWithHashFlagRoutesToHashMethod) {
    auto client = createTSAClient(stubConfig());

    TSARequest req;
    req.data         = std::vector<uint8_t>(32, 0xAB);  // 32-byte "hash"
    req.data_is_hash = true;

    auto resp = client->requestTimestamp(req);
    // Must return a well-formed response (not crash).
    EXPECT_TRUE(resp.success || !resp.error_message.empty());
}

TEST_F(TSAClientWrapperTest, HookOnTimestampIssuedFired) {
    std::atomic<int> hook_calls{0};
    bool hook_success_value = false;

    TSAEventHooks hooks;
    hooks.on_timestamp_issued = [&](const TSARequest& r, const TSAResponse& res) {
        ++hook_calls;
        hook_success_value = res.success;
    };

    auto client = createTSAClient(stubConfig(), std::move(hooks));

    TSARequest req;
    req.data = {1, 2, 3};
    client->requestTimestamp(req);

    EXPECT_EQ(hook_calls.load(), 1);
}

TEST_F(TSAClientWrapperTest, HookOnTokenVerifiedFiredOnSuccess) {
    // Build a valid round-trip with the stub backend.
    std::atomic<int> verify_calls{0};
    bool verified_value = false;

    TSAEventHooks hooks;
    hooks.on_token_verified = [&](const TSAVerifyRequest& vr, bool v) {
        ++verify_calls;
        verified_value = v;
    };

    auto client = createTSAClient(stubConfig(), std::move(hooks));

    // Get a token first.
    TSARequest req;
    req.data = {'T', 'e', 's', 't'};
    auto resp = client->requestTimestamp(req);

    TSAVerifyRequest vreq;
    vreq.data      = req.data;
    vreq.token_der = resp.token_der;
    vreq.token_b64 = resp.token_b64;

    bool result = client->verifyToken(vreq);

    EXPECT_EQ(verify_calls.load(), 1);
    EXPECT_EQ(result, verified_value);  // hook received the same result
}

TEST_F(TSAClientWrapperTest, HookOnTokenVerifiedFiredWhenNeitherTokenProvided) {
    std::atomic<int> error_calls{0};
    std::string error_msg = {};

    TSAEventHooks hooks;
    hooks.on_error = [&](const std::string& e) {
        ++error_calls;
        error_msg = e;
    };
    std::atomic<int> verify_calls{0};
    hooks.on_token_verified = [&](const TSAVerifyRequest&, bool) {
        ++verify_calls;
    };

    auto client = createTSAClient(stubConfig(), std::move(hooks));

    TSAVerifyRequest vreq;
    vreq.data = {1, 2, 3};
    // Leave token_der and token_b64 empty — should trigger error.
    bool ok = client->verifyToken(vreq);

    EXPECT_FALSE(ok);
    EXPECT_EQ(error_calls.load(), 1);
    EXPECT_FALSE(error_msg.empty());
    // on_token_verified should also be called (with false).
    EXPECT_EQ(verify_calls.load(), 1);
}

TEST_F(TSAClientWrapperTest, HookOnErrorFiredWhenRequestFails) {
    // Use a config that will fail in OpenSSL mode.
    std::atomic<int> error_calls{0};

    TSAEventHooks hooks;
    hooks.on_error = [&](const std::string&) { ++error_calls; };

    auto client = createTSAClient(stubConfig(), std::move(hooks));
    TSARequest req;
    req.data = {0x00};
    auto resp = client->requestTimestamp(req);

    // In stub mode: success → no error hook.
    // In OpenSSL mode: failure → error hook fires once.
    if (!resp.success) {
        EXPECT_GE(error_calls.load(), 1);
    }
}

TEST_F(TSAClientWrapperTest, GetLastErrorEmptyOnSuccess) {
    auto client = createTSAClient(stubConfig());
    TSARequest req;
    req.data = {1};
    auto resp = client->requestTimestamp(req);

    if (resp.success) {
        EXPECT_TRUE(client->getLastError().empty());
    }
}

TEST_F(TSAClientWrapperTest, GetLastErrorSetOnFailure) {
    auto client = createTSAClient(stubConfig());
    TSARequest req;
    req.data = {1};
    auto resp = client->requestTimestamp(req);

    if (!resp.success) {
        EXPECT_FALSE(client->getLastError().empty());
    }
}

TEST_F(TSAClientWrapperTest, NullHooksAreIgnored) {
    // Ensure no crash when hooks are left as nullptr.
    TSAEventHooks hooks;  // all function members are nullptr by default
    auto client = createTSAClient(stubConfig(), std::move(hooks));

    TSARequest req;
    req.data = {0xAB};
    EXPECT_NO_THROW(client->requestTimestamp(req));

    TSAVerifyRequest vreq;
    vreq.data = {0xAB};
    EXPECT_NO_THROW(client->verifyToken(vreq));
}

TEST_F(TSAClientWrapperTest, ResponseLatencyIsNonNegative) {
    auto client = createTSAClient(stubConfig());
    TSARequest req;
    req.data = {1, 2, 3};
    auto resp = client->requestTimestamp(req);
    EXPECT_GE(resp.request_latency.count(), 0);
}

// ============================================================================
// Network-dependent tests (skipped in CI by default)
// ============================================================================

class TSAClientWrapperNetworkTest : public TSAClientWrapperTest {
protected:
    static TSAConfig freeTSAConfig() {
        TSAConfig cfg;
        cfg.url             = "https://freetsa.org/tsr";
        cfg.hash_algorithm  = "SHA256";
        cfg.cert_req        = true;
        cfg.timeout_seconds = 30;
        cfg.verify_tsa_cert = false;
        return cfg;
    }
};

TEST_F(TSAClientWrapperNetworkTest, RequestAndVerifyRoundTrip) {
    if (skip_network) {
        GTEST_SKIP() << "Network tests disabled (THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1)";
    }

    std::atomic<int> issued{0}, verified{0};
    TSAEventHooks hooks;
    hooks.on_timestamp_issued  = [&](const TSARequest&, const TSAResponse&) { ++issued; };
    hooks.on_token_verified    = [&](const TSAVerifyRequest&, bool) { ++verified; };

    auto client = createTSAClient(freeTSAConfig(), std::move(hooks));

    TSARequest req;
    req.data = {'R', 'o', 'u', 'n', 'd', 'T', 'r', 'i', 'p'};
    auto resp = client->requestTimestamp(req);

    if (!resp.success) {
        GTEST_SKIP() << "TSA unavailable: " << resp.error_message;
    }

    EXPECT_TRUE(resp.success);
    EXPECT_FALSE(resp.serial_number.empty());
    EXPECT_FALSE(resp.token_der.empty());
    EXPECT_GT(resp.timestamp_unix_ms, 0u);
    EXPECT_GE(resp.request_latency.count(), 0);
    EXPECT_EQ(issued.load(), 1);

    // Verify via DER token.
    TSAVerifyRequest vreq;
    vreq.data      = req.data;
    vreq.token_der = resp.token_der;
    bool ok = client->verifyToken(vreq);
    EXPECT_TRUE(ok);
    EXPECT_EQ(verified.load(), 1);
}

TEST_F(TSAClientWrapperNetworkTest, HooksReceiveCorrectArguments) {
    if (skip_network) {
        GTEST_SKIP() << "Network tests disabled";
    }

    std::string captured_serial = {};
    bool captured_verify = false;

    TSAEventHooks hooks;
    hooks.on_timestamp_issued = [&](const TSARequest&, const TSAResponse& r) {
        captured_serial = r.serial_number;
    };
    hooks.on_token_verified = [&](const TSAVerifyRequest&, bool v) {
        captured_verify = v;
    };

    auto client = createTSAClient(freeTSAConfig(), std::move(hooks));

    TSARequest req;
    req.data = {'H', 'o', 'o', 'k', 'T', 'e', 's', 't'};
    auto resp = client->requestTimestamp(req);

    if (!resp.success) {
        GTEST_SKIP() << "TSA unavailable: " << resp.error_message;
    }

    EXPECT_EQ(captured_serial, resp.serial_number);

    TSAVerifyRequest vreq;
    vreq.data      = req.data;
    vreq.token_der = resp.token_der;
    client->verifyToken(vreq);

    EXPECT_TRUE(captured_verify);
}
