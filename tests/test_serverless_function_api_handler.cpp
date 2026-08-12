// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Unit tests for ServerlessFunctionApiHandler:
//  - Function registration (POST /api/v1/functions)
//  - Function listing (GET /api/v1/functions)
//  - Function retrieval (GET /api/v1/functions/{id})
//  - Function update (PUT /api/v1/functions/{id})
//  - Function deletion (DELETE /api/v1/functions/{id})
//  - Function invocation (POST /api/v1/functions/{id}/invoke)
//  - Version history (GET /api/v1/functions/{id}/versions)

#include <gtest/gtest.h>
#include "server/serverless_function_api_handler.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;
using namespace themis::server;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> makeRequest(
    http::verb method,
    const std::string& target,
    const std::string& body = "")
{
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

static json parseBody(const http::response<http::string_body>& res) {
    return json::parse(res.body());
}

// Minimal valid function code DSL
static json passthroughCode() {
    return json{{"operations", json::array({json{{"type", "passthrough"}}})}};
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class ServerlessFunctionApiHandlerTest : public ::testing::Test {
protected:
    ServerlessFunctionApiHandler handler;
};

// ---------------------------------------------------------------------------
// Registration tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, RegisterValidFunction_Returns201) {
    json body = {
        {"name", "my-fn"},
        {"code", passthroughCode()}
    };
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);

    EXPECT_EQ(res.result(), http::status::created);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("id"));
    EXPECT_EQ(resp["name"].get<std::string>(), "my-fn");
    EXPECT_EQ(resp["version"].get<int>(), 1);
    EXPECT_TRUE(resp.contains("created_at"));
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterMissingName_Returns400) {
    json body = {{"code", passthroughCode()}};
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterMissingCode_Returns400) {
    json body = {{"name", "fn-no-code"}};
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterInvalidJson_Returns400) {
    auto req = makeRequest(http::verb::post, "/api/v1/functions", "not-json{{{");
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterWithTenantId_Stored) {
    json body = {
        {"name", "tenant-fn"},
        {"tenant_id", "tenant-42"},
        {"code", passthroughCode()}
    };
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::created);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["tenant_id"].get<std::string>(), "tenant-42");
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterInvalidTenantId_Returns400) {
    json body = {
        {"name", "tenant-fn"},
        {"tenant_id", "../tenant-42"},
        {"code", passthroughCode()}
    };
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterInvalidDslOperationType_Returns400) {
    json bad_code = {{"operations", json::array({json{{"type", "unknown_op"}}})}};
    json body = {{"name", "bad-fn"}, {"code", bad_code}};
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, RegisterTransformMissingFields_Returns400) {
    json bad_code = {{"operations", json::array({json{{"type", "transform"}}})}};
    json body = {{"name", "bad-transform"}, {"code", bad_code}};
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// List tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, ListEmpty_ReturnsEmptyArray) {
    auto req = makeRequest(http::verb::get, "/api/v1/functions");
    auto res = handler.handleList(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("functions"));
    EXPECT_TRUE(resp["functions"].is_array());
    EXPECT_EQ(resp["functions"].size(), 0u);
}

TEST_F(ServerlessFunctionApiHandlerTest, ListAfterRegister_ContainsFunction) {
    json body = {{"name", "listed-fn"}, {"code", passthroughCode()}};
    handler.handleRegister(makeRequest(http::verb::post, "/api/v1/functions", body.dump()));

    auto res = handler.handleList(makeRequest(http::verb::get, "/api/v1/functions"));
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["functions"].size(), 1u);
    EXPECT_EQ(resp["functions"][0]["name"].get<std::string>(), "listed-fn");
}

TEST_F(ServerlessFunctionApiHandlerTest, ListFilterByTenantId) {
    auto reg = [&](const std::string& name, const std::string& tid) {
        json b = {{"name", name}, {"tenant_id", tid}, {"code", passthroughCode()}};
        handler.handleRegister(makeRequest(http::verb::post, "/api/v1/functions", b.dump()));
    };
    reg("fn-a", "tenant-1");
    reg("fn-b", "tenant-2");
    reg("fn-c", "tenant-1");

    auto res = handler.handleList(
        makeRequest(http::verb::get, "/api/v1/functions?tenant_id=tenant-1"));
    auto resp = parseBody(res);
    EXPECT_EQ(resp["functions"].size(), 2u);
}

TEST_F(ServerlessFunctionApiHandlerTest, ListFilterByInvalidTenantId_Returns400) {
    auto res = handler.handleList(
        makeRequest(http::verb::get, "/api/v1/functions?tenant_id=../tenant-1"));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, ListIgnoresPartialTenantParameterName) {
    auto reg = [&](const std::string& name, const std::string& tid) {
        json b = {{"name", name}, {"tenant_id", tid}, {"code", passthroughCode()}};
        handler.handleRegister(makeRequest(http::verb::post, "/api/v1/functions", b.dump()));
    };
    reg("fn-a", "tenant-1");
    reg("fn-b", "tenant-2");

    auto res = handler.handleList(
        makeRequest(http::verb::get, "/api/v1/functions?xtenant_id=tenant-1"));
    ASSERT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["functions"].size(), 2u);
}

// ---------------------------------------------------------------------------
// Get tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, GetExistingFunction_Returns200) {
    json body = {{"name", "get-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    auto res = handler.handleGet(
        makeRequest(http::verb::get, "/api/v1/functions/" + id), id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["id"].get<std::string>(), id);
}

TEST_F(ServerlessFunctionApiHandlerTest, GetNonExistingFunction_Returns404) {
    auto res = handler.handleGet(
        makeRequest(http::verb::get, "/api/v1/functions/does-not-exist"),
        "does-not-exist");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ServerlessFunctionApiHandlerTest, GetInvalidFunctionId_Returns400) {
    auto res = handler.handleGet(
        makeRequest(http::verb::get, "/api/v1/functions/../bad"),
        "../bad");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Update tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, UpdateExistingFunction_BumpsVersion) {
    json body = {{"name", "upd-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json upd = {{"name", "upd-fn-renamed"}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["name"].get<std::string>(), "upd-fn-renamed");
    EXPECT_EQ(resp["version"].get<int>(), 2);
}

TEST_F(ServerlessFunctionApiHandlerTest, UpdateNonExistingFunction_Returns404) {
    json upd = {{"name", "new-name"}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/missing", upd.dump()),
        "missing");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ServerlessFunctionApiHandlerTest, UpdateWithInvalidCode_Returns400) {
    json body = {{"name", "upd-bad-code"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json bad_code = {{"operations", json::array({json{{"type", "bogus"}}})}};
    json upd = {{"code", bad_code}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, UpdateInvalidFunctionId_Returns400) {
    json upd = {{"name", "renamed"}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/../bad", upd.dump()), "../bad");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Delete tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, DeleteExistingFunction_Returns200) {
    json body = {{"name", "del-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    auto res = handler.handleDelete(
        makeRequest(http::verb::delete_, "/api/v1/functions/" + id), id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp["deleted"].get<bool>());
    EXPECT_EQ(resp["id"].get<std::string>(), id);
}

TEST_F(ServerlessFunctionApiHandlerTest, DeleteThenGet_Returns404) {
    json body = {{"name", "del-get-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    handler.handleDelete(
        makeRequest(http::verb::delete_, "/api/v1/functions/" + id), id);

    auto res = handler.handleGet(
        makeRequest(http::verb::get, "/api/v1/functions/" + id), id);
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ServerlessFunctionApiHandlerTest, DeleteNonExisting_Returns404) {
    auto res = handler.handleDelete(
        makeRequest(http::verb::delete_, "/api/v1/functions/ghost"), "ghost");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ServerlessFunctionApiHandlerTest, DeleteInvalidFunctionId_Returns400) {
    auto res = handler.handleDelete(
        makeRequest(http::verb::delete_, "/api/v1/functions/../bad"), "../bad");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Invocation tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, InvokePassthrough_ReturnsInputUnchanged) {
    json body = {{"name", "pass-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"x", 42}, {"y", "hello"}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["result"]["x"].get<int>(), 42);
    EXPECT_EQ(resp["result"]["y"].get<std::string>(), "hello");
    EXPECT_EQ(resp["function_id"].get<std::string>(), id);
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeInvalidFunctionId_Returns400) {
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/../bad/invoke", json::object().dump()),
        "../bad");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeTransform_RenamesFields) {
    json code = {{"operations", json::array({
        json{{"type", "transform"}, {"fields", json{{"old_key", "new_key"}}}}
    })}};
    json body = {{"name", "transform-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"old_key", "value"}, {"other", 99}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto result = parseBody(res)["result"];
    EXPECT_TRUE(result.contains("new_key"));
    EXPECT_EQ(result["new_key"].get<std::string>(), "value");
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeFilter_KeepsOnlySpecifiedFields) {
    json code = {{"operations", json::array({
        json{{"type", "filter"}, {"keep", json::array({"a", "b"})}}
    })}};
    json body = {{"name", "filter-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"a", 1}, {"b", 2}, {"c", 3}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto result = parseBody(res)["result"];
    EXPECT_TRUE(result.contains("a"));
    EXPECT_TRUE(result.contains("b"));
    EXPECT_FALSE(result.contains("c"));
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeEnrich_AddsMetadata) {
    json code = {{"operations", json::array({
        json{{"type", "enrich"}, {"add", json{{"source", "themisdb"}}}}
    })}};
    json body = {{"name", "enrich-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"payload", "data"}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto result = parseBody(res)["result"];
    EXPECT_EQ(result["source"].get<std::string>(), "themisdb");
    EXPECT_EQ(result["payload"].get<std::string>(), "data");
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeValidate_MissingRequiredField_ReturnsError) {
    json code = {{"operations", json::array({
        json{{"type", "validate"}, {"required", json::array({"must_exist"})}}
    })}};
    json body = {{"name", "validate-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"other_field", "value"}};  // missing "must_exist"
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::internal_server_error);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("error"));
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeValidate_RequiredFieldPresent_Succeeds) {
    json code = {{"operations", json::array({
        json{{"type", "validate"}, {"required", json::array({"must_exist"})}}
    })}};
    json body = {{"name", "validate-ok-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"must_exist", "yes"}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeNonExistingFunction_Returns404) {
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/ghost/invoke", "{}"),
        "ghost");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeEmptyBody_TreatedAsEmptyObject) {
    json body = {{"name", "empty-body-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    auto req = makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke");
    auto res = handler.handleInvoke(req, id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp["result"].is_object());
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeChainedOperations_AppliedInOrder) {
    // Enrich adds "source", then filter keeps only "source" and "a"
    json code = {{"operations", json::array({
        json{{"type", "enrich"}, {"add", json{{"source", "db"}}}},
        json{{"type", "filter"}, {"keep", json::array({"a", "source"})}}
    })}};
    json body = {{"name", "chained-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"a", 1}, {"b", 2}, {"c", 3}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto result = parseBody(res)["result"];
    EXPECT_TRUE(result.contains("a"));
    EXPECT_TRUE(result.contains("source"));
    EXPECT_FALSE(result.contains("b"));
    EXPECT_FALSE(result.contains("c"));
}

TEST_F(ServerlessFunctionApiHandlerTest, InvokeReturnsVersionAndDurationMs) {
    json body = {{"name", "meta-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke", "{}"),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("version"));
    EXPECT_TRUE(resp.contains("duration_ms"));
}

// ---------------------------------------------------------------------------
// Version history tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, VersionsAfterRegister_HasOneEntry) {
    json body = {{"name", "ver-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    auto res = handler.handleVersions(
        makeRequest(http::verb::get, "/api/v1/functions/" + id + "/versions"),
        id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("versions"));
    EXPECT_EQ(resp["versions"].size(), 1u);
    EXPECT_EQ(resp["versions"][0]["version"].get<int>(), 1);
}

TEST_F(ServerlessFunctionApiHandlerTest, VersionsAfterUpdate_HasTwoEntries) {
    json body = {{"name", "ver-upd-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json upd = {{"description", "v2 update"}};
    handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);

    auto res = handler.handleVersions(
        makeRequest(http::verb::get, "/api/v1/functions/" + id + "/versions"),
        id);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["versions"].size(), 2u);
    EXPECT_EQ(resp["versions"][1]["version"].get<int>(), 2);
}

TEST_F(ServerlessFunctionApiHandlerTest, VersionsNonExisting_Returns404) {
    auto res = handler.handleVersions(
        makeRequest(http::verb::get, "/api/v1/functions/ghost/versions"),
        "ghost");
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(ServerlessFunctionApiHandlerTest, VersionsInvalidFunctionId_Returns400) {
    auto res = handler.handleVersions(
        makeRequest(http::verb::get, "/api/v1/functions/../bad/versions"),
        "../bad");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Content-type and response format tests
// ---------------------------------------------------------------------------

TEST_F(ServerlessFunctionApiHandlerTest, RegisterResponse_ContentTypeIsJson) {
    json body = {{"name", "ct-fn"}, {"code", passthroughCode()}};
    auto req = makeRequest(http::verb::post, "/api/v1/functions", body.dump());
    auto res = handler.handleRegister(req);
    EXPECT_NE(res.find(http::field::content_type), res.end());
    const std::string ct{res[http::field::content_type]};
    EXPECT_NE(ct.find("application/json"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Bug-fix regression tests
// ---------------------------------------------------------------------------

// Bug: timeout_ms and memory_limit_kb updates were silently ignored because
// nlohmann::json parses integer literals as number_integer, not number_unsigned,
// so is_number_unsigned() returned false.  Fixed to use is_number() + positive check.
TEST_F(ServerlessFunctionApiHandlerTest, UpdateTimeout_IntegerJson_IsApplied) {
    json body = {{"name", "timeout-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    // 1000 is a JSON integer (number_integer), not number_unsigned
    json upd = {{"timeout_ms", 1000}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["timeout_ms"].get<int>(), 1000);
}

TEST_F(ServerlessFunctionApiHandlerTest, UpdateMemoryLimit_IntegerJson_IsApplied) {
    json body = {{"name", "memlimit-fn"}, {"code", passthroughCode()}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json upd = {{"memory_limit_kb", 8192}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["memory_limit_kb"].get<int>(), 8192);
}

// Bug: executeFunction lambda captured output/error by reference and wrote to
// them from an async thread; the timeout path on the main thread also wrote to
// error, creating a data race.  Fixed by owning all result state inside the
// task (TaskResult) and only propagating after the future is settled.
TEST_F(ServerlessFunctionApiHandlerTest, InvokeValidation_ErrorMessagePropagatedCorrectly) {
    // validate op writes to its local error string; verify it reaches the caller
    json code = {{"operations", json::array({
        json{{"type", "validate"}, {"required", json::array({"required_key"})}}
    })}};
    json body = {{"name", "race-fix-fn"}, {"code", code}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json input = {{"other", "x"}};
    auto res = handler.handleInvoke(
        makeRequest(http::verb::post, "/api/v1/functions/" + id + "/invoke",
                    input.dump()),
        id);
    EXPECT_EQ(res.result(), http::status::internal_server_error);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("error"));
    // Error message should mention the missing field name
    EXPECT_NE(resp["error"].get<std::string>().find("required_key"), std::string::npos);
}

// Overflow guard: values > UINT32_MAX should be ignored (not silently wrap)
TEST_F(ServerlessFunctionApiHandlerTest, UpdateTimeout_OverflowValue_IsIgnored) {
    json body = {{"name", "overflow-fn"}, {"code", passthroughCode()},
                 {"timeout_ms", 5000}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    // 2^33 exceeds UINT32_MAX – must be ignored, leaving the previous value
    json upd = {{"timeout_ms", static_cast<int64_t>(1LL << 33)}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::ok);
    // Should still be 5000 (unchanged), not a wrapped-around value
    EXPECT_EQ(parseBody(res)["timeout_ms"].get<int>(), 5000);
}

TEST_F(ServerlessFunctionApiHandlerTest, UpdateTimeout_ZeroValue_IsIgnored) {
    json body = {{"name", "zero-timeout-fn"}, {"code", passthroughCode()},
                 {"timeout_ms", 3000}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    json upd = {{"timeout_ms", 0}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::ok);
    // 0 is not a positive value – should be ignored
    EXPECT_EQ(parseBody(res)["timeout_ms"].get<int>(), 3000);
}

// ===========================================================================
// GAP-022 — memory_limit_kb DoS cap (CWE-400)
// ===========================================================================

// GAP-022-01: Creating a function with memory_limit_kb at the cap (16 GB)
// succeeds and stores exactly the cap value.
TEST_F(ServerlessFunctionApiHandlerTest, GAP022_MemoryLimitAtCap_Stored) {
    constexpr uint32_t kCap = 16'777'216u; // 16 GB in KB
    json body = {{"name", "cap-fn"}, {"code", passthroughCode()},
                 {"memory_limit_kb", kCap}};
    auto res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    ASSERT_EQ(res.result(), http::status::created);
    EXPECT_EQ(parseBody(res)["memory_limit_kb"].get<uint32_t>(), kCap);
}

// GAP-022-02: Creating a function with memory_limit_kb beyond the cap
// results in the limit being clamped to the cap rather than storing an
// unreasonably large value.
TEST_F(ServerlessFunctionApiHandlerTest, GAP022_OversizeMemoryLimit_ClampedOnCreate) {
    constexpr uint32_t kCap = 16'777'216u;
    // Request 1 TB more than the cap (in KB).
    const uint64_t oversized = static_cast<uint64_t>(kCap) + 1'073'741'824ULL;
    json body = {{"name", "oversize-fn"}, {"code", passthroughCode()},
                 {"memory_limit_kb", oversized}};
    auto res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    ASSERT_EQ(res.result(), http::status::created);
    EXPECT_LE(parseBody(res)["memory_limit_kb"].get<uint32_t>(), kCap)
        << "memory_limit_kb must be clamped to cap on create";
}

// GAP-022-03: Updating a function with an over-cap memory_limit_kb clamps
// the value rather than accepting it.
TEST_F(ServerlessFunctionApiHandlerTest, GAP022_OversizeMemoryLimit_ClampedOnUpdate) {
    constexpr uint32_t kCap = 16'777'216u;
    json body = {{"name", "update-mem-fn"}, {"code", passthroughCode()},
                 {"memory_limit_kb", 4096u}};
    auto reg_res = handler.handleRegister(
        makeRequest(http::verb::post, "/api/v1/functions", body.dump()));
    std::string id = parseBody(reg_res)["id"].get<std::string>();

    const uint64_t oversized = static_cast<uint64_t>(kCap) + 1'073'741'824ULL;
    json upd = {{"memory_limit_kb", oversized}};
    auto res = handler.handleUpdate(
        makeRequest(http::verb::put, "/api/v1/functions/" + id, upd.dump()), id);
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_LE(parseBody(res)["memory_limit_kb"].get<uint32_t>(), kCap)
        << "memory_limit_kb must be clamped to cap on update";
}
