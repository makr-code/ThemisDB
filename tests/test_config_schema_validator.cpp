/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_config_schema_validator.cpp                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:03:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     539                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 50f623520  2026-02-25  docs(config): update README/FUTURE_ENHANCEMENTS for Confi... ║
    • 51bc83fc2  2026-02-24  feat(config): integrate JSON Schema and YAML schema valid... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "config/config_schema_validator.h"
#include "config/config_errors.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace themis {
namespace config {
namespace test {

// ═══════════════════════════════════════════════════════════
// Test Fixture
// ═══════════════════════════════════════════════════════════

class ConfigSchemaValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() / "themisdb_schema_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    // Write content to a file in the test directory.
    std::string writeFile(const std::string& name, const std::string& content) {
        auto path = test_dir_ / name;
        std::ofstream f(path);
        f << content;
        return path.string();
    }

    std::filesystem::path test_dir_;
};

// ═══════════════════════════════════════════════════════════
// loadAsJson – YAML
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, LoadYamlFileAsJson) {
    auto path = writeFile("cfg.yaml", "port: 8080\nhost: localhost\n");
    auto json = ConfigSchemaValidator::loadAsJson(path);
    EXPECT_EQ(json["port"].get<int>(), 8080);
    EXPECT_EQ(json["host"].get<std::string>(), "localhost");
}

TEST_F(ConfigSchemaValidatorTest, LoadYmlExtensionAsJson) {
    auto path = writeFile("cfg.yml", "enabled: true\ncount: 3\n");
    auto json = ConfigSchemaValidator::loadAsJson(path);
    EXPECT_TRUE(json["enabled"].get<bool>());
    EXPECT_EQ(json["count"].get<int>(), 3);
}

// ═══════════════════════════════════════════════════════════
// loadAsJson – JSON
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, LoadJsonFile) {
    auto path = writeFile("cfg.json", R"({"port": 9090, "name": "test"})");
    auto json = ConfigSchemaValidator::loadAsJson(path);
    EXPECT_EQ(json["port"].get<int>(), 9090);
    EXPECT_EQ(json["name"].get<std::string>(), "test");
}

TEST_F(ConfigSchemaValidatorTest, LoadMissingFileThrows) {
    EXPECT_THROW(ConfigSchemaValidator::loadAsJson("/nonexistent/path/cfg.yaml"),
                 SchemaValidationException);
}

// ═══════════════════════════════════════════════════════════
// validate – type checking
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, ValidateStringTypePass) {
    auto path = writeFile("cfg.json", R"({"name": "themis"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateStringTypeFail) {
    auto path = writeFile("cfg.json", R"({"name": 42})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateIntegerTypePass) {
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ValidateNumberAcceptsFloat) {
    auto path = writeFile("cfg.json", R"({"ratio": 0.75})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "ratio": { "type": "number" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ValidateBooleanTypePass) {
    auto path = writeFile("cfg.json", R"({"enabled": true})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "enabled": { "type": "boolean" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validate – required properties
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, RequiredPropertyPresent) {
    auto path = writeFile("cfg.json", R"({"host": "localhost", "port": 8080})");
    nlohmann::json schema = R"({
        "type": "object",
        "required": ["host", "port"]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, RequiredPropertyMissing) {
    auto path = writeFile("cfg.json", R"({"host": "localhost"})");
    nlohmann::json schema = R"({
        "type": "object",
        "required": ["host", "port"]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("port"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// validate – string constraints
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, StringMinLengthPass) {
    auto path = writeFile("cfg.json", R"({"name": "themis"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string", "minLength": 3 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, StringMinLengthFail) {
    auto path = writeFile("cfg.json", R"({"name": "ab"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string", "minLength": 3 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, StringMaxLengthFail) {
    auto path = writeFile("cfg.json", R"({"name": "this-is-a-very-long-name"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string", "maxLength": 5 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, StringPatternPass) {
    auto path = writeFile("cfg.json", R"({"version": "1.2.3"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "version": { "type": "string", "pattern": "^[0-9]+\\.[0-9]+\\.[0-9]+$" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, StringPatternFail) {
    auto path = writeFile("cfg.json", R"({"version": "not-semver"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "version": { "type": "string", "pattern": "^[0-9]+\\.[0-9]+\\.[0-9]+$" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validate – numeric constraints
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, NumberMinimumPass) {
    auto path = writeFile("cfg.json", R"({"port": 1024})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer", "minimum": 1, "maximum": 65535 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, NumberBelowMinimumFail) {
    auto path = writeFile("cfg.json", R"({"port": 0})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer", "minimum": 1 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, NumberAboveMaximumFail) {
    auto path = writeFile("cfg.json", R"({"port": 99999})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer", "maximum": 65535 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ExclusiveMinimumFail) {
    auto path = writeFile("cfg.json", R"({"value": 0})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": { "type": "number", "exclusiveMinimum": 0 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validate – array constraints
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, ArrayMinItemsPass) {
    auto path = writeFile("cfg.json", R"({"tags": ["a", "b", "c"]})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "tags": { "type": "array", "minItems": 1 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ArrayMinItemsFail) {
    auto path = writeFile("cfg.json", R"({"tags": []})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "tags": { "type": "array", "minItems": 1 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ArrayItemSchemaValidation) {
    auto path = writeFile("cfg.json", R"({"ports": [8080, 9090]})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "ports": {
                "type": "array",
                "items": { "type": "integer", "minimum": 1, "maximum": 65535 }
            }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validate – enum and const
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, EnumPass) {
    auto path = writeFile("cfg.json", R"({"level": "info"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "level": { "enum": ["trace", "debug", "info", "warn", "error"] }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, EnumFail) {
    auto path = writeFile("cfg.json", R"({"level": "verbose"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "level": { "enum": ["trace", "debug", "info", "warn", "error"] }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ConstPass) {
    auto path = writeFile("cfg.json", R"({"version": 1})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "version": { "const": 1 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, ConstFail) {
    auto path = writeFile("cfg.json", R"({"version": 2})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "version": { "const": 1 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validate – additionalProperties
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, AdditionalPropertiesFalsePass) {
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer" }
        },
        "additionalProperties": false
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, AdditionalPropertiesFalseFail) {
    auto path = writeFile("cfg.json", R"({"port": 8080, "unknown_key": "value"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer" }
        },
        "additionalProperties": false
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("unknown_key"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// validate – YAML file with schema
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, ValidateYamlFileAgainstSchema) {
    auto path = writeFile("server.yaml",
                          "host: \"0.0.0.0\"\nport: 18765\nworker_threads: 8\n");
    nlohmann::json schema = R"({
        "type": "object",
        "required": ["host", "port"],
        "properties": {
            "host":           { "type": "string" },
            "port":           { "type": "integer", "minimum": 1, "maximum": 65535 },
            "worker_threads": { "type": "integer", "minimum": 1 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, ValidateYamlFileFailsSchemaViolation) {
    // Port out of valid range.
    auto path = writeFile("server.yaml", "host: \"0.0.0.0\"\nport: 99999\n");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer", "maximum": 65535 }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validateWithSchemaFile
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, ValidateWithSchemaFilePass) {
    auto config_path = writeFile("app.json", R"({"port": 8080, "host": "localhost"})");
    auto schema_path = writeFile("app.schema.json", R"({
        "type": "object",
        "required": ["port", "host"],
        "properties": {
            "port": { "type": "integer" },
            "host": { "type": "string" }
        }
    })");
    auto result = ConfigSchemaValidator::validateWithSchemaFile(config_path, schema_path);
    EXPECT_TRUE(result.valid) << result.formatErrors();
    EXPECT_EQ(result.config_path, config_path);
}

TEST_F(ConfigSchemaValidatorTest, ValidateWithMissingSchemaFileReportsError) {
    auto config_path = writeFile("app.json", R"({"port": 8080})");
    // Schema file does not exist.
    auto result = ConfigSchemaValidator::validateWithSchemaFile(
        config_path, "/nonexistent/schema.json");
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
}

// ═══════════════════════════════════════════════════════════
// SchemaValidationException
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, SchemaValidationExceptionCarriesDetails) {
    SchemaValidationException ex("config/server.yaml", "port is missing");
    EXPECT_NE(std::string(ex.what()).find("server.yaml"), std::string::npos);
    EXPECT_NE(std::string(ex.what()).find("port is missing"), std::string::npos);
    EXPECT_EQ(ex.file_path(), "config/server.yaml");
    EXPECT_EQ(ex.reason(), "port is missing");
}

// ═══════════════════════════════════════════════════════════
// ValidationResult helpers
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, ValidationResultFormatErrors) {
    ConfigSchemaValidator::ValidationResult r;
    r.addError("field 'host' is required");
    r.addWarning("field 'debug' is deprecated");
    EXPECT_FALSE(r.valid);
    std::string fmt = r.formatErrors();
    EXPECT_NE(fmt.find("ERROR:"), std::string::npos);
    EXPECT_NE(fmt.find("WARNING:"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Integration: validate config.json against its JSON Schema
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, MainConfigJsonPassesOfficialSchema) {
    // config/ is copied into the build directory by CMake so tests can find it
    // via relative paths when run from ${CMAKE_BINARY_DIR}.
    const std::string config_path = "config/config.json";
    const std::string schema_path = "config/schema/themisdb.config.schema.json";

    if (!std::filesystem::exists(config_path) || !std::filesystem::exists(schema_path)) {
        GTEST_SKIP() << "config/config.json or config/schema/themisdb.config.schema.json "
                        "not found relative to the current working directory";
    }

    auto result = ConfigSchemaValidator::validateWithSchemaFile(config_path, schema_path);
    EXPECT_TRUE(result.valid) << "config.json does not satisfy themisdb.config.schema.json:\n"
                              << result.formatErrors();
}

// ═══════════════════════════════════════════════════════════
// validate – allOf
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, AllOfPassWhenAllSubschemasMatch) {
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "allOf": [
            { "type": "object" },
            { "required": ["port"] }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, AllOfFailWhenOneSubschemaFails) {
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "allOf": [
            { "type": "object" },
            { "required": ["host"] }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("host"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, AllOfNestedTypeConstraintsPass) {
    auto path = writeFile("cfg.json", R"({"value": 42})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": {
                "allOf": [
                    { "type": "integer" },
                    { "minimum": 1 },
                    { "maximum": 100 }
                ]
            }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, AllOfNestedTypeConstraintsFail) {
    auto path = writeFile("cfg.json", R"({"value": 200})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": {
                "allOf": [
                    { "type": "integer" },
                    { "minimum": 1 },
                    { "maximum": 100 }
                ]
            }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, AllOfCollectsErrorsFromAllFailingSubschemas) {
    // Value violates BOTH sub-schemas; allOf must surface both errors.
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "allOf": [
            { "required": ["host"] },
            { "required": ["ssl"] }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_GE(result.errors.size(), 2u);
    bool has_host = false, has_ssl = false;
    for (const auto& e : result.errors) {
        if (e.find("host") != std::string::npos) has_host = true;
        if (e.find("ssl")  != std::string::npos) has_ssl  = true;
    }
    EXPECT_TRUE(has_host) << "Expected an error about missing 'host'";
    EXPECT_TRUE(has_ssl)  << "Expected an error about missing 'ssl'";
}

// ═══════════════════════════════════════════════════════════
// validate – anyOf
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, AnyOfPassWhenFirstSubschemaMatches) {
    auto path = writeFile("cfg.json", R"("hello")");
    nlohmann::json schema = R"({
        "anyOf": [
            { "type": "string" },
            { "type": "integer" }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, AnyOfPassWhenSecondSubschemaMatches) {
    auto path = writeFile("cfg.json", R"(42)");
    nlohmann::json schema = R"({
        "anyOf": [
            { "type": "string" },
            { "type": "integer" }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, AnyOfFailWhenNoSubschemaMatches) {
    auto path = writeFile("cfg.json", R"([1, 2, 3])");
    nlohmann::json schema = R"({
        "anyOf": [
            { "type": "string" },
            { "type": "integer" }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("anyOf"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, AnyOfWithPropertyConstraints) {
    auto path = writeFile("cfg.json", R"({"level": "info"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "level": {
                "anyOf": [
                    { "type": "string", "minLength": 1 },
                    { "type": "integer", "minimum": 0 }
                ]
            }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

// ═══════════════════════════════════════════════════════════
// validate – oneOf
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, OneOfPassWhenExactlyOneSubschemaMatches) {
    auto path = writeFile("cfg.json", R"("hello")");
    nlohmann::json schema = R"({
        "oneOf": [
            { "type": "string" },
            { "type": "integer" }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, OneOfFailWhenNoSubschemaMatches) {
    auto path = writeFile("cfg.json", R"([1, 2, 3])");
    nlohmann::json schema = R"({
        "oneOf": [
            { "type": "string" },
            { "type": "integer" }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("oneOf"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, OneOfFailWhenMultipleSubschemasMatch) {
    // "abc" satisfies BOTH sub-schemas (string with minLength 1 AND string with maxLength 10),
    // so oneOf fails because exactly one must match.
    auto path = writeFile("cfg.json", R"("abc")");
    nlohmann::json schema = R"({
        "oneOf": [
            { "type": "string", "minLength": 1 },
            { "type": "string", "maxLength": 10 }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("oneOf"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, OneOfWithObjectSchemas) {
    // Object with "type" key: only the first schema matches (has required "type")
    auto path = writeFile("cfg.json", R"({"type": "tcp", "port": 8080})");
    nlohmann::json schema = R"({
        "oneOf": [
            {
                "type": "object",
                "required": ["type", "port"],
                "properties": {
                    "type": { "const": "tcp" },
                    "port": { "type": "integer" }
                },
                "additionalProperties": false
            },
            {
                "type": "object",
                "required": ["type", "path"],
                "properties": {
                    "type": { "const": "unix" },
                    "path": { "type": "string" }
                },
                "additionalProperties": false
            }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

} // namespace test
} // namespace config
} // namespace themis
