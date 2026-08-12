#include <gtest/gtest.h>
#include "server/request_validation_middleware.h"
#include <nlohmann/json.hpp>

using namespace themis::server;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class RequestValidationMiddlewareTest : public ::testing::Test {
protected:
    RequestValidationMiddleware mw_;

    static nlohmann::json entitySchema() {
        return {
            {"type", "object"},
            {"required", {"key", "data"}},
            {"properties", {
                {"key",  {{"type", "string"}, {"minLength", 1}, {"maxLength", 256}}},
                {"data", {{"type", "object"}}},
                {"ttl",  {{"type", "integer"}, {"minimum", 0}}}
            }}
        };
    }

    static nlohmann::json strictSchema() {
        return {
            {"type", "object"},
            {"required", {"name"}},
            {"properties", {
                {"name", {{"type", "string"}}}
            }},
            {"additionalProperties", false}
        };
    }
};

// ---------------------------------------------------------------------------
// registerSchema / schemaCount / hasSchema
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, RegisterAndCount) {
    EXPECT_EQ(mw_.schemaCount(), 0u);
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    EXPECT_EQ(mw_.schemaCount(), 1u);
    mw_.registerSchema("PUT", "/api/v1/entities", entitySchema());
    EXPECT_EQ(mw_.schemaCount(), 2u);
}

TEST_F(RequestValidationMiddlewareTest, RegisterOverwritesExisting) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    mw_.registerSchema("POST", "/api/v1/entities", strictSchema()); // overwrite
    EXPECT_EQ(mw_.schemaCount(), 1u);
}

TEST_F(RequestValidationMiddlewareTest, HasSchema_Exact) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    EXPECT_TRUE(mw_.hasSchema("POST", "/api/v1/entities"));
    EXPECT_FALSE(mw_.hasSchema("GET",  "/api/v1/entities"));
    EXPECT_FALSE(mw_.hasSchema("POST", "/api/v1/other"));
}

TEST_F(RequestValidationMiddlewareTest, HasSchema_PrefixMatch) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    EXPECT_TRUE(mw_.hasSchema("POST", "/api/v1/entities/abc123"));
}

TEST_F(RequestValidationMiddlewareTest, HasSchema_WildcardMethod) {
    mw_.registerSchema("*", "/api/v1/config", entitySchema());
    EXPECT_TRUE(mw_.hasSchema("GET",    "/api/v1/config"));
    EXPECT_TRUE(mw_.hasSchema("POST",   "/api/v1/config"));
    EXPECT_TRUE(mw_.hasSchema("DELETE", "/api/v1/config"));
}

TEST_F(RequestValidationMiddlewareTest, MethodNormalization) {
    mw_.registerSchema("post", "/api/v1/entities", entitySchema());
    EXPECT_TRUE(mw_.hasSchema("POST", "/api/v1/entities"));
    EXPECT_TRUE(mw_.hasSchema("post", "/api/v1/entities"));
}

// ---------------------------------------------------------------------------
// removeSchema / clearSchemas
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, RemoveSchema) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    EXPECT_TRUE(mw_.removeSchema("POST", "/api/v1/entities"));
    EXPECT_EQ(mw_.schemaCount(), 0u);
    EXPECT_FALSE(mw_.removeSchema("POST", "/api/v1/entities")); // already gone
}

TEST_F(RequestValidationMiddlewareTest, ClearSchemas) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    mw_.registerSchema("PUT",  "/api/v1/entities", entitySchema());
    mw_.clearSchemas();
    EXPECT_EQ(mw_.schemaCount(), 0u);
}

// ---------------------------------------------------------------------------
// No schema registered → always pass
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, NoSchema_AnyBodyPasses) {
    auto r = mw_.validate("POST", "/api/v1/entities", std::string{R"({"random": 42})"});
    EXPECT_TRUE(r.valid);

    auto r2 = mw_.validate("POST", "/api/v1/entities", nlohmann::json{{"x", 1}});
    EXPECT_TRUE(r2.valid);
}

TEST_F(RequestValidationMiddlewareTest, NoSchema_EmptyBodyPasses) {
    auto r = mw_.validate("POST", "/api/v1/entities", std::string{});
    EXPECT_TRUE(r.valid);
}

// ---------------------------------------------------------------------------
// String body overload – valid JSON
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, ValidBody_StringOverload) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    std::string body = R"({"key": "abc", "data": {"x": 1}})";
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_TRUE(r.valid) << r.error_message;
}

TEST_F(RequestValidationMiddlewareTest, InvalidJSON_StringOverload) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    auto r = mw_.validate("POST", "/api/v1/entities", std::string{"{ not valid json }"});
    EXPECT_FALSE(r.valid);
    EXPECT_FALSE(r.error_message.empty());
}

TEST_F(RequestValidationMiddlewareTest, EmptyBody_RequiredFieldMissing) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    auto r = mw_.validate("POST", "/api/v1/entities", std::string{});
    EXPECT_FALSE(r.valid);
    EXPECT_NE(r.error_message.find("missing required"), std::string::npos);
}

// ---------------------------------------------------------------------------
// JSON body overload
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, ValidBody_JsonOverload) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    nlohmann::json body = {{"key", "abc"}, {"data", {{"x", 1}}}};
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_TRUE(r.valid) << r.error_message;
}

TEST_F(RequestValidationMiddlewareTest, MissingRequiredField) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    nlohmann::json body = {{"key", "abc"}}; // missing "data"
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_FALSE(r.valid);
    EXPECT_NE(r.error_message.find("data"), std::string::npos);
}

TEST_F(RequestValidationMiddlewareTest, WrongType) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    nlohmann::json body = {{"key", 42}, {"data", {{"x", 1}}}}; // key should be string
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_FALSE(r.valid);
    EXPECT_NE(r.error_message.find("key"), std::string::npos);
}

TEST_F(RequestValidationMiddlewareTest, MinLengthViolation) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    nlohmann::json body = {{"key", ""}, {"data", {}}}; // key: minLength 1
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_FALSE(r.valid);
}

TEST_F(RequestValidationMiddlewareTest, MaxLengthViolation) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    std::string long_key(300, 'x'); // exceeds maxLength 256
    nlohmann::json body = {{"key", long_key}, {"data", {}}};
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_FALSE(r.valid);
}

TEST_F(RequestValidationMiddlewareTest, NumericMinimumViolation) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    nlohmann::json body = {{"key", "x"}, {"data", {}}, {"ttl", -1}}; // ttl min 0
    auto r = mw_.validate("POST", "/api/v1/entities", body);
    EXPECT_FALSE(r.valid);
}

TEST_F(RequestValidationMiddlewareTest, NumericMaximumViolation) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"score", {{"type", "integer"}, {"minimum", 0}, {"maximum", 100}}}
        }}
    };
    mw_.registerSchema("POST", "/api/v1/score", schema);
    nlohmann::json over  = {{"score", 101}};
    nlohmann::json under = {{"score", 50}};
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/score", over).valid);
    EXPECT_TRUE(mw_.validate("POST", "/api/v1/score", under).valid);
}

TEST_F(RequestValidationMiddlewareTest, ExclusiveMinimumViolation) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"value", {{"type", "number"}, {"exclusiveMinimum", 0.0}}}
        }}
    };
    mw_.registerSchema("POST", "/api/v1/exclusive", schema);
    // 0 should fail (must be strictly > 0), 0.001 should pass
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/exclusive", nlohmann::json{{"value", 0}}).valid);
    EXPECT_TRUE(mw_.validate("POST", "/api/v1/exclusive", nlohmann::json{{"value", 0.001}}).valid);
}

TEST_F(RequestValidationMiddlewareTest, ExclusiveMaximumViolation) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"ratio", {{"type", "number"}, {"exclusiveMaximum", 1.0}}}
        }}
    };
    mw_.registerSchema("POST", "/api/v1/ratio", schema);
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/ratio", nlohmann::json{{"ratio", 1.0}}).valid);
    EXPECT_TRUE(mw_.validate("POST", "/api/v1/ratio", nlohmann::json{{"ratio", 0.99}}).valid);
}

TEST_F(RequestValidationMiddlewareTest, EnumViolation) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"status", {{"type", "string"}, {"enum", {"active", "inactive", "pending"}}}}
        }}
    };
    mw_.registerSchema("POST", "/api/v1/item", schema);
    EXPECT_TRUE(mw_.validate("POST", "/api/v1/item", nlohmann::json{{"status", "active"}}).valid);
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/item", nlohmann::json{{"status", "deleted"}}).valid);
}

TEST_F(RequestValidationMiddlewareTest, PatternViolation) {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"code", {{"type", "string"}, {"pattern", "^[A-Z]{3}-[0-9]{4}$"}}}
        }}
    };
    mw_.registerSchema("POST", "/api/v1/code", schema);
    EXPECT_TRUE(mw_.validate("POST", "/api/v1/code", nlohmann::json{{"code", "ABC-1234"}}).valid);
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/code", nlohmann::json{{"code", "abc-1234"}}).valid);
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/code", nlohmann::json{{"code", "AB-12"}}).valid);
}

TEST_F(RequestValidationMiddlewareTest, AdditionalPropertyRejected) {
    mw_.registerSchema("POST", "/api/v1/strict", strictSchema());
    nlohmann::json body = {{"name", "Alice"}, {"extra", "disallowed"}};
    auto r = mw_.validate("POST", "/api/v1/strict", body);
    EXPECT_FALSE(r.valid);
    EXPECT_NE(r.error_message.find("extra"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Path prefix matching
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, PrefixMatchUsed) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    // Request to a sub-path should use the prefix schema
    nlohmann::json body = {{"key", "x"}, {"data", {}}};
    auto r = mw_.validate("POST", "/api/v1/entities/extra/sub", body);
    EXPECT_TRUE(r.valid) << r.error_message;
}

TEST_F(RequestValidationMiddlewareTest, TrailingSlashPrefix_MatchesSubPath) {
    // A registered path ending with '/' should match any sub-path that follows
    nlohmann::json body_schema = {
        {"type", "object"},
        {"properties", {
            {"data", {{"type", "object"}}}
        }}
    };
    mw_.registerSchema("PUT", "/entities/", body_schema);
    nlohmann::json body = {{"data", {{"x", 1}}}};
    auto r = mw_.validate("PUT", "/entities/somekey", body);
    EXPECT_TRUE(r.valid) << r.error_message;
    auto r2 = mw_.validate("PUT", "/entities/another/deeply/nested", body);
    EXPECT_TRUE(r2.valid) << r2.error_message;
}

TEST_F(RequestValidationMiddlewareTest, LongerPrefixWins) {
    // Register two prefixes; longer one should be used
    nlohmann::json loose = {{"type", "object"}};
    nlohmann::json strict = strictSchema();
    mw_.registerSchema("POST", "/api/v1", loose);
    mw_.registerSchema("POST", "/api/v1/entities", strict);

    // Request to /api/v1/entities/42 should use strict schema
    nlohmann::json missing_name = {{"other", "field"}};
    auto r = mw_.validate("POST", "/api/v1/entities/42", missing_name);
    EXPECT_FALSE(r.valid); // strict requires "name"
}

TEST_F(RequestValidationMiddlewareTest, PrefixDoesNotMatchNonBoundary) {
    // /api/v1/entities should NOT match /api/v1/entities2
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    auto r = mw_.validate("POST", "/api/v1/entities2", std::string{"{}"});
    EXPECT_TRUE(r.valid); // no schema found → skip
}

// ---------------------------------------------------------------------------
// Wildcard method
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, WildcardMethod_ValidatesAllMethods) {
    mw_.registerSchema("*", "/api/v1/config", strictSchema());
    nlohmann::json valid_body = {{"name", "cfg"}};
    nlohmann::json invalid_body = {{"other", "field"}};

    EXPECT_TRUE(mw_.validate("GET",    "/api/v1/config", valid_body).valid);
    EXPECT_TRUE(mw_.validate("POST",   "/api/v1/config", valid_body).valid);
    EXPECT_FALSE(mw_.validate("PUT",   "/api/v1/config", invalid_body).valid);
    EXPECT_FALSE(mw_.validate("PATCH", "/api/v1/config", invalid_body).valid);
}

TEST_F(RequestValidationMiddlewareTest, ExactMethodTakesPrecedenceOverWildcard) {
    nlohmann::json loose = {{"type", "object"}}; // accepts anything
    nlohmann::json strict = strictSchema();        // requires "name"
    mw_.registerSchema("*",    "/api/v1/config", loose);
    mw_.registerSchema("POST", "/api/v1/config", strict);

    // POST should use the stricter schema
    nlohmann::json no_name = {{"other", "val"}};
    EXPECT_FALSE(mw_.validate("POST", "/api/v1/config", no_name).valid);

    // GET should use the loose wildcard schema
    EXPECT_TRUE(mw_.validate("GET", "/api/v1/config", no_name).valid);
}

// ---------------------------------------------------------------------------
// Metrics
// ---------------------------------------------------------------------------

TEST_F(RequestValidationMiddlewareTest, Metrics_Skip) {
    mw_.validate("POST", "/no/schema", std::string{R"({"x":1})"});
    EXPECT_EQ(mw_.getMetrics().validation_skip_total.load(), 1u);
    EXPECT_EQ(mw_.getMetrics().validation_pass_total.load(), 0u);
    EXPECT_EQ(mw_.getMetrics().validation_fail_total.load(), 0u);
}

TEST_F(RequestValidationMiddlewareTest, Metrics_Pass) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    mw_.validate("POST", "/api/v1/entities", nlohmann::json{{"key","x"},{"data",{}}});
    EXPECT_EQ(mw_.getMetrics().validation_pass_total.load(), 1u);
    EXPECT_EQ(mw_.getMetrics().validation_fail_total.load(), 0u);
}

TEST_F(RequestValidationMiddlewareTest, Metrics_Fail) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    mw_.validate("POST", "/api/v1/entities", nlohmann::json{{"bad","field"}});
    EXPECT_EQ(mw_.getMetrics().validation_fail_total.load(), 1u);
    EXPECT_EQ(mw_.getMetrics().validation_pass_total.load(), 0u);
}

TEST_F(RequestValidationMiddlewareTest, Metrics_ParseError) {
    mw_.registerSchema("POST", "/api/v1/entities", entitySchema());
    mw_.validate("POST", "/api/v1/entities", std::string{"not json {{"});
    EXPECT_EQ(mw_.getMetrics().parse_error_total.load(), 1u);
    EXPECT_EQ(mw_.getMetrics().validation_fail_total.load(), 1u);
}
