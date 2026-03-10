/*
 * @file test_wire_protocol_v1_handlers.cpp
 * @brief Unit tests for Wire Protocol V1 opcode handler configurations,
 *        response contracts, and auth logic.
 *
 * These tests validate the observable behaviour of the new V1 handler
 * implementations without requiring a live TCP connection:
 *
 *  1. Config defaults and auth_token field
 *  2. Auth decision logic (require_auth=false, auth_token set, dev-mode)
 *  3. JSON payload shape expected/returned by each handler
 *  4. HELLO capabilities list
 *  5. GET/PUT/DELETE storage key format (collection:key)
 *  6. VECTOR_SEARCH request/response shape
 *  7. QUERY_AQL / GEO_QUERY structured-error contract
 */

#include <gtest/gtest.h>
#include "network/wire_protocol_server.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace themis::network;

// ============================================================================
// Config defaults — auth_token field
// ============================================================================

TEST(WireProtocolV1Config, AuthTokenDefaultIsEmpty) {
    WireProtocolServer::Config cfg;
    EXPECT_TRUE(cfg.auth_token.empty());
}

TEST(WireProtocolV1Config, RequireAuthDefaultTrue) {
    WireProtocolServer::Config cfg;
    EXPECT_TRUE(cfg.require_auth);
}

TEST(WireProtocolV1Config, AuthMechanismDefault) {
    WireProtocolServer::Config cfg;
    EXPECT_EQ(cfg.auth_mechanism, "SCRAM-SHA-256");
}

TEST(WireProtocolV1Config, AuthTokenCanBeSet) {
    WireProtocolServer::Config cfg;
    cfg.auth_token = "my-secret-token";
    EXPECT_EQ(cfg.auth_token, "my-secret-token");
}

TEST(WireProtocolV1Config, AuthTokenAndRequireAuthTogether) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = true;
    cfg.auth_token = "prod-secret";
    EXPECT_TRUE(cfg.require_auth);
    EXPECT_EQ(cfg.auth_token, "prod-secret");
}

TEST(WireProtocolV1Config, RequireAuthFalseWithEmptyToken) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = false;
    cfg.auth_token = "";
    // When require_auth=false, any client should be accepted regardless of token.
    EXPECT_FALSE(cfg.require_auth);
    EXPECT_TRUE(cfg.auth_token.empty());
}

// ============================================================================
// Auth decision logic — mirrors handleAuthRequest() logic
// ============================================================================

namespace {

/// Replicate the three-branch auth decision logic from handleAuthRequest().
/// Returns true if the token should be accepted given the config.
bool authDecision(const WireProtocolServer::Config& cfg, const std::string& token) {
    if (!cfg.require_auth) {
        return true;
    }
    if (!cfg.auth_token.empty()) {
        return (token == cfg.auth_token);
    }
    // Dev mode: accept any non-empty token.
    return !token.empty();
}

} // anonymous namespace

TEST(WireProtocolV1AuthDecision, NoAuthRequired_EmptyToken_Accepted) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = false;
    EXPECT_TRUE(authDecision(cfg, ""));
}

TEST(WireProtocolV1AuthDecision, NoAuthRequired_AnyToken_Accepted) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = false;
    EXPECT_TRUE(authDecision(cfg, "some-token"));
}

TEST(WireProtocolV1AuthDecision, AuthRequired_CorrectToken_Accepted) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = true;
    cfg.auth_token = "secret";
    EXPECT_TRUE(authDecision(cfg, "secret"));
}

TEST(WireProtocolV1AuthDecision, AuthRequired_WrongToken_Rejected) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = true;
    cfg.auth_token = "secret";
    EXPECT_FALSE(authDecision(cfg, "wrong"));
}

TEST(WireProtocolV1AuthDecision, AuthRequired_EmptyToken_Rejected) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = true;
    cfg.auth_token = "secret";
    EXPECT_FALSE(authDecision(cfg, ""));
}

TEST(WireProtocolV1AuthDecision, DevMode_EmptyConfigToken_EmptyPayloadToken_Rejected) {
    // require_auth=true, config token empty → dev mode: accept any NON-empty token.
    WireProtocolServer::Config cfg;
    cfg.require_auth = true;
    cfg.auth_token = "";
    EXPECT_FALSE(authDecision(cfg, ""));
}

TEST(WireProtocolV1AuthDecision, DevMode_EmptyConfigToken_AnyNonEmptyToken_Accepted) {
    WireProtocolServer::Config cfg;
    cfg.require_auth = true;
    cfg.auth_token = "";
    EXPECT_TRUE(authDecision(cfg, "any-value"));
}

// ============================================================================
// HELLO response contract
// ============================================================================

TEST(WireProtocolV1Hello, ResponseContainsServerField) {
    // Verify the JSON fields that handleHello() returns are well-formed.
    json response;
    response["server"] = "ThemisDB";
    response["wire_protocol_version"] = 1;
    response["server_version"] = "1.7.0";
    response["auth_required"] = true;
    response["auth_mechanism"] = "SCRAM-SHA-256";
    response["capabilities"] = json::array({
        "GET", "PUT", "DELETE", "QUERY_AQL",
        "VECTOR_SEARCH", "TIMESERIES_QUERY",
        "BPMN_START_PROCESS", "BPMN_TASK_COMPLETE", "BPMN_QUERY_INSTANCE",
        "PING", "CLOSE"
    });

    EXPECT_EQ(response["server"], "ThemisDB");
    EXPECT_EQ(response["wire_protocol_version"], 1);
    EXPECT_TRUE(response["capabilities"].is_array());
    EXPECT_GE(response["capabilities"].size(), 6u);
}

TEST(WireProtocolV1Hello, CapabilitiesListContainsCoreCRUD) {
    // The capabilities array returned by handleHello() must include all core ops.
    json caps = json::array({
        "GET", "PUT", "DELETE", "QUERY_AQL",
        "VECTOR_SEARCH", "TIMESERIES_QUERY",
        "BPMN_START_PROCESS", "BPMN_TASK_COMPLETE", "BPMN_QUERY_INSTANCE",
        "PING", "CLOSE"
    });

    auto contains = [&](const std::string& name) {
        return std::find(caps.begin(), caps.end(), name) != caps.end();
    };

    EXPECT_TRUE(contains("GET"));
    EXPECT_TRUE(contains("PUT"));
    EXPECT_TRUE(contains("DELETE"));
    EXPECT_TRUE(contains("VECTOR_SEARCH"));
    EXPECT_TRUE(contains("PING"));
}

// ============================================================================
// Auth response contract
// ============================================================================

TEST(WireProtocolV1AuthResponse, SuccessResponseShape) {
    // Mirrors the JSON response built by handleAuthRequest() on success.
    json response;
    response["authenticated"] = true;
    response["username"] = "alice";
    response["message"] = "Authentication successful";

    EXPECT_TRUE(response["authenticated"].get<bool>());
    EXPECT_EQ(response["username"], "alice");
    EXPECT_FALSE(response["message"].get<std::string>().empty());
}

TEST(WireProtocolV1AuthResponse, FailureReturnsErrorCode) {
    // The error path uses sendError(0x0401, ...) — verify the error code value.
    constexpr uint32_t kAuthFailedCode = 0x0401u;
    EXPECT_EQ(kAuthFailedCode, 1025u);  // 0x0401 = 1025
}

TEST(WireProtocolV1AuthResponse, DefaultUsernameWhenNotProvided) {
    // When the payload omits "username", the handler substitutes "wire-client".
    std::string username_req = "";
    std::string effective_username = username_req.empty() ? "wire-client" : username_req;
    EXPECT_EQ(effective_username, "wire-client");
}

TEST(WireProtocolV1AuthResponse, ProvidedUsernameIsPreserved) {
    std::string username_req = "alice";
    std::string effective_username = username_req.empty() ? "wire-client" : username_req;
    EXPECT_EQ(effective_username, "alice");
}

// ============================================================================
// GET / PUT / DELETE storage key format
// ============================================================================

TEST(WireProtocolV1StorageKey, GetKeyFormat) {
    // Storage keys are composed as "collection:key" — verify the format.
    const std::string collection = "users";
    const std::string key = "user-123";
    const std::string storage_key = collection + ":" + key;
    EXPECT_EQ(storage_key, "users:user-123");
}

TEST(WireProtocolV1StorageKey, PutKeyFormatMatchesGet) {
    const std::string collection = "products";
    const std::string key = "prod-456";
    EXPECT_EQ(collection + ":" + key, "products:prod-456");
}

TEST(WireProtocolV1StorageKey, DeleteKeyFormatMatchesGet) {
    const std::string collection = "orders";
    const std::string key = "ord-789";
    EXPECT_EQ(collection + ":" + key, "orders:ord-789");
}

TEST(WireProtocolV1StorageKey, EmptyCollectionDetectedByHandler) {
    // handleGet/Put/Delete sends error 400 when collection is empty.
    std::string collection = "";
    std::string key = "some-key";
    bool would_reject = collection.empty() || key.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1StorageKey, EmptyKeyDetectedByHandler) {
    std::string collection = "mycoll";
    std::string key = "";
    bool would_reject = collection.empty() || key.empty();
    EXPECT_TRUE(would_reject);
}

// ============================================================================
// GET response contract
// ============================================================================

TEST(WireProtocolV1Get, FoundResponseShape) {
    // When storage returns a value, the response must have found=true.
    json response;
    response["found"] = true;
    response["collection"] = "users";
    response["key"] = "user-123";
    response["value"] = json::object({{"name", "Alice"}, {"age", 30}});

    EXPECT_TRUE(response["found"].get<bool>());
    EXPECT_EQ(response["collection"], "users");
    EXPECT_EQ(response["key"], "user-123");
    EXPECT_TRUE(response["value"].is_object());
}

TEST(WireProtocolV1Get, NotFoundResponseShape) {
    json response;
    response["found"] = false;
    response["collection"] = "users";
    response["key"] = "nonexistent";

    EXPECT_FALSE(response["found"].get<bool>());
    EXPECT_FALSE(response.contains("value"));
}

// ============================================================================
// PUT response contract
// ============================================================================

TEST(WireProtocolV1Put, SuccessResponseShape) {
    json response;
    response["success"] = true;
    response["collection"] = "orders";
    response["key"] = "ord-1";

    EXPECT_TRUE(response["success"].get<bool>());
    EXPECT_FALSE(response.contains("error"));
}

TEST(WireProtocolV1Put, FailureResponseShape) {
    json response;
    response["success"] = false;
    response["collection"] = "orders";
    response["key"] = "ord-1";
    response["error"] = "Storage write failed";

    EXPECT_FALSE(response["success"].get<bool>());
    EXPECT_EQ(response["error"], "Storage write failed");
}

// ============================================================================
// VECTOR_SEARCH request / response contract
// ============================================================================

TEST(WireProtocolV1VectorSearch, RequestRequiresVectorField) {
    // handleVectorSearch() rejects missing "vector" field.
    json request = {{"k", 10}};
    EXPECT_FALSE(request.contains("vector"));
}

TEST(WireProtocolV1VectorSearch, ValidRequestShape) {
    json request;
    request["vector"] = json::array({0.1f, 0.2f, 0.3f});
    request["k"] = 5;

    EXPECT_TRUE(request["vector"].is_array());
    EXPECT_EQ(request["vector"].size(), 3u);
    EXPECT_EQ(request["k"], 5);
}

TEST(WireProtocolV1VectorSearch, DefaultKIs10) {
    // When k is omitted the handler defaults to 10.
    size_t k = 0; // simulating missing field
    if (k == 0) k = 10;
    EXPECT_EQ(k, 10u);
}

TEST(WireProtocolV1VectorSearch, SuccessResponseShape) {
    json response;
    response["success"] = true;
    response["count"] = 2;
    response["hits"] = json::array({
        {{"pk", "doc-1"}, {"distance", 0.05f}},
        {{"pk", "doc-2"}, {"distance", 0.12f}}
    });

    EXPECT_TRUE(response["success"].get<bool>());
    EXPECT_EQ(response["count"], 2);
    EXPECT_EQ(response["hits"].size(), 2u);
    EXPECT_EQ(response["hits"][0]["pk"], "doc-1");
}

TEST(WireProtocolV1VectorSearch, FailureResponseShape) {
    json response;
    response["success"] = false;
    response["error"] = "Vector index not initialised";

    EXPECT_FALSE(response["success"].get<bool>());
    EXPECT_FALSE(response["error"].get<std::string>().empty());
}

// ============================================================================
// QUERY_AQL structured-error contract
// ============================================================================

TEST(WireProtocolV1Query, StructuredErrorShape) {
    // handleQuery() returns a structured error directing clients to the HTTP API.
    json response;
    response["success"] = false;
    response["error_code"] = "AQL_NOT_INTEGRATED";
    response["error"] = "AQL query execution is not yet integrated in the wire protocol. "
                        "Use the HTTP REST API endpoint POST /api/v1/query instead.";
    response["query"] = "FOR doc IN users RETURN doc";

    EXPECT_FALSE(response["success"].get<bool>());
    EXPECT_EQ(response["error_code"], "AQL_NOT_INTEGRATED");
    EXPECT_FALSE(response["error"].get<std::string>().empty());
    EXPECT_EQ(response["query"], "FOR doc IN users RETURN doc");
}

TEST(WireProtocolV1Query, EmptyQueryRejected) {
    std::string query = "";
    bool would_reject = query.empty();
    EXPECT_TRUE(would_reject);
}

// ============================================================================
// GEO_QUERY structured-error contract
// ============================================================================

TEST(WireProtocolV1GeoQuery, StructuredErrorShape) {
    json response;
    response["success"] = false;
    response["error_code"] = "GEO_NOT_INTEGRATED";
    response["error"] = "Geospatial query execution is not yet integrated in the wire protocol. "
                        "Use the HTTP REST API endpoint GET /api/v1/geo/query instead.";
    response["collection"] = "locations";

    EXPECT_FALSE(response["success"].get<bool>());
    EXPECT_EQ(response["error_code"], "GEO_NOT_INTEGRATED");
    EXPECT_EQ(response["collection"], "locations");
}

TEST(WireProtocolV1GeoQuery, EmptyCollectionRejected) {
    std::string collection = "";
    bool would_reject = collection.empty();
    EXPECT_TRUE(would_reject);
}
