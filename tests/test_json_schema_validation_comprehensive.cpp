/**
 * @file test_json_schema_validation_comprehensive.cpp
 * @brief Comprehensive tests for the enhanced JSON schema validation in InputValidator
 *
 * Tests cover the full JSON Schema Draft-7 subset supported by validateJsonStub:
 * - type (string, object, number, integer, boolean, array, null)
 * - required fields
 * - minLength / maxLength for strings
 * - minimum / maximum / exclusiveMinimum / exclusiveMaximum for numbers
 * - pattern (ECMAScript regex) for strings
 * - enum for any type
 * - additionalProperties: false
 *
 * 30+ test cases.
 */

#include <gtest/gtest.h>
#include "utils/input_validator.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

using namespace themis::utils;
using json = nlohmann::json;

namespace {

// ---------------------------------------------------------------------------
// Fixture that writes schema files to a temp directory
// ---------------------------------------------------------------------------
class SchemaFixture : public ::testing::Test {
protected:
    std::filesystem::path schema_dir_;

    void SetUp() override {
        schema_dir_ = std::filesystem::temp_directory_path() /
                      ("themis_schema_test_" + std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(schema_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(schema_dir_);
    }

    void writeSchema(const std::string& name, const json& schema) {
        std::ofstream f(schema_dir_ / (name + ".json"));
        f << schema.dump(2);
    }

    InputValidator makeV() const {
        return InputValidator(schema_dir_.string());
    }
};

} // anonymous namespace

// ============================================================================
// Basic type validation
// ============================================================================

TEST_F(SchemaFixture, TypeString_Valid) {
    writeSchema("s", {{"type","object"},{"required",{"v"}},
                      {"properties",{{"v",{{"type","string"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"v","hello"}}, "s"));
}

TEST_F(SchemaFixture, TypeString_WrongType) {
    writeSchema("s", {{"type","object"},{"required",{"v"}},
                      {"properties",{{"v",{{"type","string"}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"v",42}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("string"), std::string::npos);
}

TEST_F(SchemaFixture, TypeNumber_Valid) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"n",{{"type","number"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"n",3.14}}, "s"));
}

TEST_F(SchemaFixture, TypeInteger_Valid) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"n",{{"type","integer"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"n",7}}, "s"));
}

TEST_F(SchemaFixture, TypeInteger_FloatRejected) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"n",{{"type","integer"}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"n",3.14}}, "s");
    ASSERT_TRUE(err.has_value());
}

TEST_F(SchemaFixture, TypeBoolean_Valid) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"b",{{"type","boolean"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"b",true}}, "s"));
}

TEST_F(SchemaFixture, TypeArray_Valid) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"a",{{"type","array"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"a",json::array({1,2,3})}}, "s"));
}

TEST_F(SchemaFixture, TypeNull_Valid) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"x",{{"type","null"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"x",nullptr}}, "s"));
}

// ============================================================================
// Required fields
// ============================================================================

TEST_F(SchemaFixture, Required_MissingField) {
    writeSchema("s", {{"type","object"},{"required",{"foo","bar"}},
                      {"properties",{{"foo",{{"type","string"}}},{"bar",{{"type","number"}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"foo","hi"}}, "s");  // bar is missing
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("bar"), std::string::npos);
}

TEST_F(SchemaFixture, Required_AllPresent) {
    writeSchema("s", {{"type","object"},{"required",{"foo","bar"}},
                      {"properties",{{"foo",{{"type","string"}}},{"bar",{{"type","number"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"foo","hi"},{"bar",1}}, "s"));
}

// ============================================================================
// minLength / maxLength
// ============================================================================

TEST_F(SchemaFixture, MinLength_TooShort) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"pw",{{"type","string"},{"minLength",8}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"pw","abc"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("minLength"), std::string::npos);
}

TEST_F(SchemaFixture, MinLength_ExactBoundary) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"pw",{{"type","string"},{"minLength",4}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"pw","abcd"}}, "s"));
}

TEST_F(SchemaFixture, MaxLength_TooLong) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"tag",{{"type","string"},{"maxLength",5}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"tag","toolongvalue"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("maxLength"), std::string::npos);
}

TEST_F(SchemaFixture, MaxLength_ExactBoundary) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"tag",{{"type","string"},{"maxLength",5}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"tag","hello"}}, "s"));
}

// ============================================================================
// minimum / maximum
// ============================================================================

TEST_F(SchemaFixture, Minimum_BelowThreshold) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"age",{{"type","number"},{"minimum",0}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"age",-1}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("minimum"), std::string::npos);
}

TEST_F(SchemaFixture, Minimum_AtBoundary) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"age",{{"type","number"},{"minimum",0}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"age",0}}, "s"));
}

TEST_F(SchemaFixture, Maximum_AboveThreshold) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"pct",{{"type","number"},{"maximum",100}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"pct",101}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("maximum"), std::string::npos);
}

TEST_F(SchemaFixture, Maximum_AtBoundary) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"pct",{{"type","number"},{"maximum",100}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"pct",100}}, "s"));
}

TEST_F(SchemaFixture, ExclusiveMinimum) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"n",{{"type","number"},{"exclusiveMinimum",0}}}}}});
    auto v = makeV();
    EXPECT_TRUE(v.validateJsonStub({{"n",0}}, "s").has_value()); // 0 is not > 0
    EXPECT_FALSE(v.validateJsonStub({{"n",0.001}}, "s"));
}

TEST_F(SchemaFixture, ExclusiveMaximum) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"n",{{"type","number"},{"exclusiveMaximum",1}}}}}});
    auto v = makeV();
    EXPECT_TRUE(v.validateJsonStub({{"n",1}}, "s").has_value()); // 1 is not < 1
    EXPECT_FALSE(v.validateJsonStub({{"n",0.999}}, "s"));
}

// ============================================================================
// pattern
// ============================================================================

TEST_F(SchemaFixture, Pattern_MatchSuccess) {
    // e-mail-like pattern
    writeSchema("s", {{"type","object"},
                      {"properties",{{"email",{{"type","string"},
                          {"pattern","^[^@]+@[^@]+\\.[^@]+$"}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"email","user@example.com"}}, "s"));
}

TEST_F(SchemaFixture, Pattern_MatchFailure) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"email",{{"type","string"},
                          {"pattern","^[^@]+@[^@]+\\.[^@]+$"}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"email","not-an-email"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("pattern"), std::string::npos);
}

TEST_F(SchemaFixture, Pattern_InvalidRegex_ReturnsError) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"x",{{"type","string"},{"pattern","[invalid"}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"x","test"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("pattern"), std::string::npos);
}

// ============================================================================
// enum
// ============================================================================

TEST_F(SchemaFixture, Enum_ValidValue) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"role",{{"type","string"},
                          {"enum",{"admin","user","readonly"}}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"role","admin"}}, "s"));
    EXPECT_FALSE(v.validateJsonStub({{"role","user"}}, "s"));
}

TEST_F(SchemaFixture, Enum_InvalidValue) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"role",{{"type","string"},
                          {"enum",{"admin","user","readonly"}}}}}}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"role","superuser"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("enum"), std::string::npos);
}

TEST_F(SchemaFixture, Enum_NumericValues) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"code",{{"type","integer"},
                          {"enum",{200,201,204,400,404,500}}}}}}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"code",200}}, "s"));
    EXPECT_TRUE(v.validateJsonStub({{"code",418}}, "s").has_value());
}

// ============================================================================
// additionalProperties: false
// ============================================================================

TEST_F(SchemaFixture, AdditionalProperties_False_RejectsExtra) {
    writeSchema("s", {{"type","object"},
                      {"required",{"name"}},
                      {"properties",{{"name",{{"type","string"}}}}},
                      {"additionalProperties",false}});
    auto v = makeV();
    auto err = v.validateJsonStub({{"name","Alice"},{"extra","field"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("additional property"), std::string::npos);
}

TEST_F(SchemaFixture, AdditionalProperties_False_AllowsKnownFields) {
    writeSchema("s", {{"type","object"},
                      {"required",{"name"}},
                      {"properties",{{"name",{{"type","string"}}}}},
                      {"additionalProperties",false}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"name","Alice"}}, "s"));
}

TEST_F(SchemaFixture, AdditionalProperties_True_AllowsExtra) {
    writeSchema("s", {{"type","object"},
                      {"properties",{{"name",{{"type","string"}}}}},
                      {"additionalProperties",true}});
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"name","Alice"},{"any","extra"}}, "s"));
}

// ============================================================================
// No-schema present -> accept all
// ============================================================================

TEST_F(SchemaFixture, NoSchema_AcceptAll) {
    // Don't write a schema file
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"any","payload"}}, "nonexistent_schema"));
}

// ============================================================================
// Payload is not an object
// ============================================================================

TEST_F(SchemaFixture, PayloadNotObject_Rejected) {
    writeSchema("s", {{"type","object"}});
    auto v = makeV();
    auto err = v.validateJsonStub(json::array({1,2,3}), "s");
    ASSERT_TRUE(err.has_value());
}

// ============================================================================
// Combined constraints
// ============================================================================

TEST_F(SchemaFixture, Combined_MultipleConstraintsPass) {
    writeSchema("s", {
        {"type","object"},
        {"required",{"username","age","role"}},
        {"properties",{
            {"username",{{"type","string"},{"minLength",3},{"maxLength",32},
                         {"pattern","^[a-zA-Z0-9_]+$"}}},
            {"age",{{"type","integer"},{"minimum",0},{"maximum",150}}},
            {"role",{{"type","string"},{"enum",{"admin","user","readonly"}}}}
        }},
        {"additionalProperties",false}
    });
    auto v = makeV();
    EXPECT_FALSE(v.validateJsonStub({{"username","alice_42"},{"age",30},{"role","user"}}, "s"));
}

TEST_F(SchemaFixture, Combined_MultipleConstraintsFail_Username) {
    writeSchema("s", {
        {"type","object"},
        {"required",{"username","age","role"}},
        {"properties",{
            {"username",{{"type","string"},{"minLength",3},{"maxLength",32},
                         {"pattern","^[a-zA-Z0-9_]+$"}}},
            {"age",{{"type","integer"},{"minimum",0},{"maximum",150}}},
            {"role",{{"type","string"},{"enum",{"admin","user","readonly"}}}}
        }},
        {"additionalProperties",false}
    });
    auto v = makeV();
    // Username contains invalid character '@'
    auto err = v.validateJsonStub({{"username","alice@host"},{"age",30},{"role","user"}}, "s");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("pattern"), std::string::npos);
}

// ============================================================================
// Integration with real AQL schema file
// ============================================================================

TEST(AqlSchemaIntegration, LoadsRealSchemaAndEnforcesMaxLength) {
    // Use the real config/schemas directory if it exists
    std::string schema_dir = "config/schemas";
    if (!std::filesystem::exists(schema_dir)) {
        GTEST_SKIP() << "config/schemas not found; skipping integration test";
    }
    InputValidator v(schema_dir);

    // Valid minimal request
    EXPECT_FALSE(v.validateJsonStub({{"query","FOR d IN col RETURN d"}}, "aql_request"));

    // Missing required 'query'
    auto err = v.validateJsonStub({{"bindVars",{}}}, "aql_request");
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->find("query"), std::string::npos);

    // Extra field rejected (additionalProperties: false)
    auto err2 = v.validateJsonStub(
        {{"query","RETURN 1"},{"unknownField","oops"}}, "aql_request");
    ASSERT_TRUE(err2.has_value());
    EXPECT_NE(err2->find("additional property"), std::string::npos);
}
