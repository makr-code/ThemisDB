// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Unit tests for UdfApiHandler and UdfRegistry:
//  - UDF registration (POST /api/v1/query/udfs)
//  - UDF listing     (GET  /api/v1/query/udfs)
//  - UDF retrieval   (GET  /api/v1/query/udfs/{name})
//  - UDF deletion    (DELETE /api/v1/query/udfs/{name})
//  - Expression DSL: const, arg, call, op, if

#include <gtest/gtest.h>
#include "server/udf_api_handler.h"
#include "query/functions/udf_registry.h"
#include "query/functions/function_registry.h"
#include "query/functions/math_functions.h"
#include "query/functions/string_functions.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;
using namespace themis::server;
using namespace themis::query::functions;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> makeReq(
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

// ---------------------------------------------------------------------------
// Fixture – cleans up UDFs registered during each test
// ---------------------------------------------------------------------------

class UdfApiHandlerTest : public ::testing::Test {
protected:
    UdfApiHandler handler;

    void SetUp() override {
        // Register string functions so call-based UDFs work
        registerStringFunctions(FunctionRegistry::instance());
        registerMathFunctions(FunctionRegistry::instance());
    }

    void TearDown() override {
        // Remove any UDFs registered during this test
        for (const auto& d : UdfRegistry::instance().listUdfs()) {
            try { UdfRegistry::instance().unregisterUdf(d.name); } catch (...) {}
        }
    }
};

// ---------------------------------------------------------------------------
// POST /api/v1/query/udfs – registration
// ---------------------------------------------------------------------------

TEST_F(UdfApiHandlerTest, Register_ValidConst_Returns201) {
    json body = {
        {"name", "TEST_CONST_42"},
        {"body", {{"type", "const"}, {"value", 42}}}
    };
    auto res = handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));
    EXPECT_EQ(res.result(), http::status::created);
    auto resp = parseBody(res);
    EXPECT_EQ(resp["name"].get<std::string>(), "TEST_CONST_42");
    EXPECT_TRUE(resp.contains("created_at"));
}

TEST_F(UdfApiHandlerTest, Register_MissingName_Returns400) {
    json body = {{"body", {{"type", "const"}, {"value", 1}}}};
    auto res = handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(UdfApiHandlerTest, Register_MissingBody_Returns400) {
    json body = {{"name", "TEST_NO_BODY"}};
    auto res = handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(UdfApiHandlerTest, Register_InvalidJson_Returns400) {
    auto res = handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", "not json"));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(UdfApiHandlerTest, Register_WithArguments_Returns201) {
    json body = {
        {"name", "TEST_ARG_ECHO"},
        {"description", "returns first argument"},
        {"arguments", json::array({
            json{{"name", "val"}, {"type", "ANY"}, {"required", true}}
        })},
        {"return_type", "ANY"},
        {"body", {{"type", "arg"}, {"index", 0}}}
    };
    auto res = handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));
    EXPECT_EQ(res.result(), http::status::created);
    auto resp = parseBody(res);
    ASSERT_TRUE(resp.contains("arguments"));
    EXPECT_EQ(resp["arguments"].size(), 1u);
    EXPECT_EQ(resp["arguments"][0]["name"].get<std::string>(), "val");
}

TEST_F(UdfApiHandlerTest, Register_CannotOverrideBuiltin) {
    // LENGTH is a built-in; attempting to register a UDF with the same name
    // must fail.
    json body = {
        {"name", "LENGTH"},
        {"body", {{"type", "const"}, {"value", 0}}}
    };
    auto res = handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ---------------------------------------------------------------------------
// GET /api/v1/query/udfs – list
// ---------------------------------------------------------------------------

TEST_F(UdfApiHandlerTest, List_Empty_ReturnsEmptyArray) {
    auto res = handler.handleList(makeReq(http::verb::get, "/api/v1/query/udfs"));
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_TRUE(resp.contains("udfs"));
    EXPECT_EQ(resp["count"].get<size_t>(), 0u);
}

TEST_F(UdfApiHandlerTest, List_AfterRegister_ReturnsOne) {
    json body = {{"name", "TEST_LIST_FN"}, {"body", {{"type", "const"}, {"value", "ok"}}}};
    handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));

    auto res = handler.handleList(makeReq(http::verb::get, "/api/v1/query/udfs"));
    EXPECT_EQ(res.result(), http::status::ok);
    auto resp = parseBody(res);
    EXPECT_GE(resp["count"].get<size_t>(), 1u);
}

// ---------------------------------------------------------------------------
// GET /api/v1/query/udfs/{name} – single UDF
// ---------------------------------------------------------------------------

TEST_F(UdfApiHandlerTest, Get_Existing_Returns200) {
    json body = {{"name", "TEST_GET_FN"}, {"body", {{"type", "const"}, {"value", true}}}};
    handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));

    auto res = handler.handleGet(makeReq(http::verb::get, "/api/v1/query/udfs/TEST_GET_FN"),
                                 "TEST_GET_FN");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(parseBody(res)["name"].get<std::string>(), "TEST_GET_FN");
}

TEST_F(UdfApiHandlerTest, Get_NotFound_Returns404) {
    auto res = handler.handleGet(makeReq(http::verb::get, "/api/v1/query/udfs/NONEXISTENT"),
                                 "NONEXISTENT");
    EXPECT_EQ(res.result(), http::status::not_found);
}

// ---------------------------------------------------------------------------
// DELETE /api/v1/query/udfs/{name}
// ---------------------------------------------------------------------------

TEST_F(UdfApiHandlerTest, Delete_Existing_Returns204) {
    json body = {{"name", "TEST_DEL_FN"}, {"body", {{"type", "const"}, {"value", 0}}}};
    handler.handleRegister(makeReq(http::verb::post, "/api/v1/query/udfs", body.dump()));

    auto res = handler.handleDelete(makeReq(http::verb::delete_, "/api/v1/query/udfs/TEST_DEL_FN"),
                                    "TEST_DEL_FN");
    EXPECT_EQ(res.result(), http::status::no_content);

    // Should no longer be found
    auto res2 = handler.handleGet(makeReq(http::verb::get, "/api/v1/query/udfs/TEST_DEL_FN"),
                                  "TEST_DEL_FN");
    EXPECT_EQ(res2.result(), http::status::not_found);
}

TEST_F(UdfApiHandlerTest, Delete_NotFound_Returns404) {
    auto res = handler.handleDelete(makeReq(http::verb::delete_, "/api/v1/query/udfs/GHOST"),
                                    "GHOST");
    EXPECT_EQ(res.result(), http::status::not_found);
}

// ---------------------------------------------------------------------------
// Expression DSL – via UdfRegistry directly
// ---------------------------------------------------------------------------

class UdfExprTest : public ::testing::Test {
protected:
    FunctionContext ctx;

    void SetUp() override {
        registerStringFunctions(FunctionRegistry::instance());
        registerMathFunctions(FunctionRegistry::instance());
    }

    void TearDown() override {
        for (const auto& d : UdfRegistry::instance().listUdfs()) {
            try { UdfRegistry::instance().unregisterUdf(d.name); } catch (...) {}
        }
    }

    nlohmann::json callUdf(const std::string& name,
                           const std::vector<nlohmann::json>& args) {
        return FunctionRegistry::instance().call(name, args, ctx);
    }
};

TEST_F(UdfExprTest, ConstExpression) {
    UdfDefinition def;
    def.name = "EXPR_CONST";
    def.body = {{"type", "const"}, {"value", 99}};
    UdfRegistry::instance().registerUdf(def);

    EXPECT_EQ(callUdf("EXPR_CONST", {}), 99);
}

TEST_F(UdfExprTest, ArgExpression) {
    UdfDefinition def;
    def.name = "EXPR_ARG";
    def.arguments = {ArgSpec{"x", ArgType::ANY, true}};
    def.body = {{"type", "arg"}, {"index", 0}};
    UdfRegistry::instance().registerUdf(def);

    EXPECT_EQ(callUdf("EXPR_ARG", {"hello"}), "hello");
}

TEST_F(UdfExprTest, CallBuiltin) {
    UdfDefinition def;
    def.name = "EXPR_CALL";
    def.arguments = {ArgSpec{"s", ArgType::STRING, true}};
    def.body = {
        {"type", "call"},
        {"function", "UPPER"},
        {"args", json::array({json{{"type", "arg"}, {"index", 0}}})}
    };
    UdfRegistry::instance().registerUdf(def);

    EXPECT_EQ(callUdf("EXPR_CALL", {"hello"}), "HELLO");
}

TEST_F(UdfExprTest, OpAddNumbers) {
    UdfDefinition def;
    def.name = "EXPR_ADD";
    def.arguments = {
        ArgSpec{"a", ArgType::NUMBER, true},
        ArgSpec{"b", ArgType::NUMBER, true}
    };
    def.body = {
        {"type", "op"},
        {"op", "+"},
        {"left",  json{{"type", "arg"}, {"index", 0}}},
        {"right", json{{"type", "arg"}, {"index", 1}}}
    };
    UdfRegistry::instance().registerUdf(def);

    EXPECT_EQ(callUdf("EXPR_ADD", {3, 4}), 7.0);
}

TEST_F(UdfExprTest, OpStringConcat) {
    UdfDefinition def;
    def.name = "EXPR_CONCAT";
    def.arguments = {
        ArgSpec{"a", ArgType::STRING, true},
        ArgSpec{"b", ArgType::STRING, true}
    };
    def.body = {
        {"type", "op"},
        {"op", "+"},
        {"left",  json{{"type", "arg"}, {"index", 0}}},
        {"right", json{{"type", "arg"}, {"index", 1}}}
    };
    UdfRegistry::instance().registerUdf(def);

    EXPECT_EQ(callUdf("EXPR_CONCAT", {"foo", "bar"}), "foobar");
}

TEST_F(UdfExprTest, IfExpression) {
    UdfDefinition def;
    def.name = "EXPR_IF";
    def.arguments = {ArgSpec{"x", ArgType::NUMBER, true}};
    def.body = {
        {"type", "if"},
        {"cond", json{{"type", "op"}, {"op", ">"}, {"left", json{{"type", "arg"}, {"index", 0}}}, {"right", json{{"type", "const"}, {"value", 0}}}}},
        {"then", json{{"type", "const"}, {"value", "positive"}}},
        {"else", json{{"type", "const"}, {"value", "non-positive"}}}
    };
    UdfRegistry::instance().registerUdf(def);

    EXPECT_EQ(callUdf("EXPR_IF", {5}), "positive");
    EXPECT_EQ(callUdf("EXPR_IF", {-1}), "non-positive");
}

TEST_F(UdfExprTest, ReplaceExistingUdf) {
    UdfDefinition def1;
    def1.name = "EXPR_REPLACE";
    def1.body = {{"type", "const"}, {"value", "v1"}};
    UdfRegistry::instance().registerUdf(def1);
    EXPECT_EQ(callUdf("EXPR_REPLACE", {}), "v1");

    UdfDefinition def2;
    def2.name = "EXPR_REPLACE";
    def2.body = {{"type", "const"}, {"value", "v2"}};
    UdfRegistry::instance().registerUdf(def2);
    EXPECT_EQ(callUdf("EXPR_REPLACE", {}), "v2");
}
