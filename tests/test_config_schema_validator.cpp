#include <gtest/gtest.h>
#include "config/config_schema_validator.h"
#include "config/config_errors.h"
#include <chrono>
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
// loadAsJson(content, is_yaml) – string overload
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, LoadJsonStringOverload) {
    auto json = ConfigSchemaValidator::loadAsJson(R"({"port": 9090, "name": "test"})", false);
    EXPECT_EQ(json["port"].get<int>(), 9090);
    EXPECT_EQ(json["name"].get<std::string>(), "test");
}

TEST_F(ConfigSchemaValidatorTest, LoadYamlStringOverload) {
    auto json = ConfigSchemaValidator::loadAsJson("port: 8080\nhost: localhost\n", true);
    EXPECT_EQ(json["port"].get<int>(), 8080);
    EXPECT_EQ(json["host"].get<std::string>(), "localhost");
}

TEST_F(ConfigSchemaValidatorTest, LoadYamlStringBoolAndIntOverload) {
    auto json = ConfigSchemaValidator::loadAsJson("enabled: true\ncount: 3\n", true);
    EXPECT_TRUE(json["enabled"].get<bool>());
    EXPECT_EQ(json["count"].get<int>(), 3);
}

TEST_F(ConfigSchemaValidatorTest, LoadInvalidJsonStringThrows) {
    EXPECT_THROW(ConfigSchemaValidator::loadAsJson("{not valid json", false),
                 SchemaValidationException);
}

TEST_F(ConfigSchemaValidatorTest, LoadInvalidYamlStringThrows) {
    // A tab character at the start of a YAML block is a parse error.
    EXPECT_THROW(ConfigSchemaValidator::loadAsJson("\tkey: bad\n", true),
                 SchemaValidationException);
}

// ═══════════════════════════════════════════════════════════
// validateFromString – in-memory validation
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, ValidateFromYamlStringPass) {
    const std::string yaml = "port: 8080\nhost: localhost\n";
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer" },
            "host": { "type": "string" }
        },
        "required": ["port", "host"]
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(yaml, true, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromYamlStringFail) {
    const std::string yaml = "port: not_a_number\nhost: localhost\n";
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "type": "integer" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(yaml, true, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromJsonStringPass) {
    const std::string json_str = R"({"name": "themis", "version": 2})";
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string" },
            "version": { "type": "integer" }
        },
        "required": ["name", "version"]
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(json_str, false, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromJsonStringFail) {
    const std::string json_str = R"({"name": 42})";
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "name": { "type": "string" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(json_str, false, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromStringMissingRequiredFail) {
    const std::string yaml = "host: localhost\n";
    nlohmann::json schema = R"({
        "type": "object",
        "required": ["host", "port"]
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(yaml, true, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromStringInvalidYamlReportsError) {
    const std::string bad_yaml = "\tkey: bad\n";
    nlohmann::json schema = R"({"type": "object"})"_json;
    auto result = ConfigSchemaValidator::validateFromString(bad_yaml, true, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromStringInvalidJsonReportsError) {
    const std::string bad_json = "{not valid json";
    nlohmann::json schema = R"({"type": "object"})"_json;
    auto result = ConfigSchemaValidator::validateFromString(bad_json, false, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromStringConfigPathIsString) {
    const std::string json_str = R"({"x": 1})";
    nlohmann::json schema = R"({"type": "object"})"_json;
    auto result = ConfigSchemaValidator::validateFromString(json_str, false, schema);
    EXPECT_EQ(result.config_path, "<string>");
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromStringWithSchemaComposition) {
    const std::string yaml = "type: tcp\nport: 9090\n";
    nlohmann::json schema = R"({
        "allOf": [
            { "type": "object" },
            { "required": ["type", "port"] },
            { "properties": { "port": { "minimum": 1, "maximum": 65535 } } }
        ]
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(yaml, true, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, ValidateFromStringWithRefDefs) {
    const std::string json_str = R"({"host": "db.example.com", "port": 5432})";
    nlohmann::json schema = R"({
        "$defs": {
            "Port": { "type": "integer", "minimum": 1, "maximum": 65535 }
        },
        "type": "object",
        "properties": {
            "host": { "type": "string" },
            "port": { "$ref": "#/$defs/Port" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validateFromString(json_str, false, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
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
        if (e.find("host") != std::string::npos) {
          has_host = true;
        }
        if (e.find("ssl")  != std::string::npos) {
          has_ssl  = true;
        }
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

// ═══════════════════════════════════════════════════════════
// validate – $ref / $defs
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, RefDefsResolutionPass) {
    // $ref pointing to a $defs entry; value satisfies the referenced schema.
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "$defs": {
            "Port": { "type": "integer", "minimum": 1, "maximum": 65535 }
        },
        "type": "object",
        "properties": {
            "port": { "$ref": "#/$defs/Port" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, RefDefsResolutionFail) {
    // Port value out of range according to the $defs entry.
    auto path = writeFile("cfg.json", R"({"port": 99999})");
    nlohmann::json schema = R"({
        "$defs": {
            "Port": { "type": "integer", "minimum": 1, "maximum": 65535 }
        },
        "type": "object",
        "properties": {
            "port": { "$ref": "#/$defs/Port" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("maximum"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, RefDefinitionsResolutionPass) {
    // Draft 4/6/7 style: "definitions" instead of "$defs".
    auto path = writeFile("cfg.json", R"({"name": "themis"})");
    nlohmann::json schema = R"({
        "definitions": {
            "NonEmptyString": { "type": "string", "minLength": 1 }
        },
        "type": "object",
        "properties": {
            "name": { "$ref": "#/definitions/NonEmptyString" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, RefDefinitionsResolutionFail) {
    // Empty string fails the NonEmptyString definition.
    auto path = writeFile("cfg.json", R"({"name": ""})");
    nlohmann::json schema = R"({
        "definitions": {
            "NonEmptyString": { "type": "string", "minLength": 1 }
        },
        "type": "object",
        "properties": {
            "name": { "$ref": "#/definitions/NonEmptyString" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, RefEnforcesRequiredPropertiesPass) {
    // $ref is used as a top-level schema for a required check.
    auto path = writeFile("cfg.json", R"({"host": "localhost", "port": 8080})");
    nlohmann::json schema = R"({
        "$defs": {
            "ServerConfig": {
                "type": "object",
                "required": ["host", "port"],
                "properties": {
                    "host": { "type": "string" },
                    "port": { "type": "integer" }
                }
            }
        },
        "$ref": "#/$defs/ServerConfig"
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, RefDetectsMissingRequiredProperty) {
    // Missing required property detected through $ref resolution.
    auto path = writeFile("cfg.json", R"({"host": "localhost"})");
    nlohmann::json schema = R"({
        "$defs": {
            "ServerConfig": {
                "type": "object",
                "required": ["host", "port"]
            }
        },
        "$ref": "#/$defs/ServerConfig"
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("port"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, NestedRefResolutionPass) {
    // $defs entry that itself references another $defs entry.
    auto path = writeFile("cfg.json", R"({"level": "info"})");
    nlohmann::json schema = R"({
        "$defs": {
            "LogLevel": { "enum": ["trace", "debug", "info", "warn", "error"] },
            "LogConfig": {
                "type": "object",
                "properties": {
                    "level": { "$ref": "#/$defs/LogLevel" }
                }
            }
        },
        "$ref": "#/$defs/LogConfig"
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, UnresolvableRefReportsError) {
    // $ref that points to a non-existent definition.
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "$ref": "#/$defs/DoesNotExist" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("Cannot resolve"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, ExternalRefReportsError) {
    // External URI refs are not supported (SSRF guard).
    auto path = writeFile("cfg.json", R"({"port": 8080})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "$ref": "https://example.com/schema.json#/Port" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("External $ref"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, CyclicRefReportsError) {
    // Directly self-referencing $ref must be detected as a cycle.
    auto path = writeFile("cfg.json", R"({"value": 1})");
    nlohmann::json schema = R"({
        "$defs": {
            "Cyclic": { "$ref": "#/$defs/Cyclic" }
        },
        "type": "object",
        "properties": {
            "value": { "$ref": "#/$defs/Cyclic" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("Cyclic"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// validate – uniqueItems
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, UniqueItemsPassWhenAllDistinct) {
    auto path = writeFile("cfg.json", R"({"tags": ["a", "b", "c"]})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "tags": { "type": "array", "uniqueItems": true }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, UniqueItemsFailWhenDuplicateExists) {
    auto path = writeFile("cfg.json", R"({"tags": ["a", "b", "a"]})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "tags": { "type": "array", "uniqueItems": true }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].find("unique"), std::string::npos);
}

TEST_F(ConfigSchemaValidatorTest, UniqueItemsFalseDoesNotEnforceUniqueness) {
    auto path = writeFile("cfg.json", R"([1, 1, 2])");
    nlohmann::json schema = R"({ "type": "array", "uniqueItems": false })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, UniqueItemsPassOnEmptyArray) {
    auto path = writeFile("cfg.json", R"([])");
    nlohmann::json schema = R"({ "type": "array", "uniqueItems": true })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, UniqueItemsWithIntegerDuplicates) {
    auto path = writeFile("cfg.json", R"([1, 2, 1])");
    nlohmann::json schema = R"({ "type": "array", "uniqueItems": true })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

// ═══════════════════════════════════════════════════════════
// validate – format
// ═══════════════════════════════════════════════════════════

TEST_F(ConfigSchemaValidatorTest, FormatDatePass) {
    auto path = writeFile("cfg.json", R"({"created": "2026-03-11"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "created": { "type": "string", "format": "date" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, FormatDateFail) {
    auto path = writeFile("cfg.json", R"({"created": "11/03/2026"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "created": { "type": "string", "format": "date" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatDateTimePass) {
    auto path = writeFile("cfg.json", R"({"ts": "2026-03-11T09:30:00Z"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "ts": { "type": "string", "format": "date-time" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, FormatDateTimeFail) {
    auto path = writeFile("cfg.json", R"({"ts": "not-a-datetime"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "ts": { "type": "string", "format": "date-time" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatEmailPass) {
    auto path = writeFile("cfg.json", R"({"email": "user@example.com"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "email": { "type": "string", "format": "email" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, FormatEmailFail) {
    auto path = writeFile("cfg.json", R"({"email": "not-an-email"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "email": { "type": "string", "format": "email" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatUriPass) {
    auto path = writeFile("cfg.json", R"({"endpoint": "https://api.example.com/v1"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "endpoint": { "type": "string", "format": "uri" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, FormatUriFail) {
    auto path = writeFile("cfg.json", R"({"endpoint": "not a uri"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "endpoint": { "type": "string", "format": "uri" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatIpv4Pass) {
    auto path = writeFile("cfg.json", R"({"host": "192.168.1.1"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "host": { "type": "string", "format": "ipv4" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, FormatIpv4Fail) {
    auto path = writeFile("cfg.json", R"({"host": "999.999.999.999"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "host": { "type": "string", "format": "ipv4" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatIpv4FailNotDottedDecimal) {
    auto path = writeFile("cfg.json", R"({"host": "not-an-ip"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "host": { "type": "string", "format": "ipv4" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatIpv6Pass) {
    auto path = writeFile("cfg.json", R"({"host": "2001:db8::1"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "host": { "type": "string", "format": "ipv6" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, FormatIpv6Fail) {
    auto path = writeFile("cfg.json", R"({"host": "192.168.1.1"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "host": { "type": "string", "format": "ipv6" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, FormatUnknownIsAccepted) {
    // Unknown format identifiers must not cause a validation error.
    auto path = writeFile("cfg.json", R"({"value": "anything"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": { "type": "string", "format": "custom-format" }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

// ============================================================================
// not keyword
// ============================================================================

TEST_F(ConfigSchemaValidatorTest, NotKeyword_PassWhenValueDoesNotMatchSubSchema) {
    // A string value does not match {type: "integer"}, so "not" passes.
    auto path = writeFile("cfg.json", R"({"value": "hello"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": { "not": { "type": "integer" } }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_FailWhenValueMatchesSubSchema) {
    // An integer value matches {type: "integer"}, so "not" fails.
    auto path = writeFile("cfg.json", R"({"value": 42})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": { "not": { "type": "integer" } }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_PassWithEnumSubSchema) {
    // Value "c" is not in ["a","b"], so "not" of {enum: ["a","b"]} passes.
    auto path = writeFile("cfg.json", R"({"value": "c"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": { "not": { "enum": ["a", "b"] } }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_FailWithEnumSubSchema) {
    // Value "a" is in ["a","b"], so "not" of {enum: ["a","b"]} fails.
    auto path = writeFile("cfg.json", R"({"value": "a"})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "value": { "not": { "enum": ["a", "b"] } }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_TopLevelNotPass) {
    // Top-level "not": value is a string, not an integer — passes.
    auto path = writeFile("cfg.json", R"("hello")");
    nlohmann::json schema = R"({ "not": { "type": "integer" } })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_TopLevelNotFail) {
    // Top-level "not": value is an integer — fails.
    auto path = writeFile("cfg.json", R"(123)");
    nlohmann::json schema = R"({ "not": { "type": "integer" } })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_NotWithComplexSubSchema) {
    // "not" sub-schema with type + minimum: value 5 matches {type:integer,
    // minimum:1}, so "not" fails.
    auto path = writeFile("cfg.json", R"({"port": 5})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "not": { "type": "integer", "minimum": 1 } }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_FALSE(result.valid);
}

TEST_F(ConfigSchemaValidatorTest, NotKeyword_NotWithComplexSubSchemaPass) {
    // Value 0 does not match {type:integer, minimum:1} (minimum fails), so
    // "not" passes.
    auto path = writeFile("cfg.json", R"({"port": 0})");
    nlohmann::json schema = R"({
        "type": "object",
        "properties": {
            "port": { "not": { "type": "integer", "minimum": 1 } }
        }
    })"_json;
    auto result = ConfigSchemaValidator::validate(path, schema);
    EXPECT_TRUE(result.valid) << result.formatErrors();
}

// ============================================================================
// Performance acceptance criterion: validate() for a 100-field JSON config
// against a 200-rule schema must complete in < 5 ms on a single thread.
// ============================================================================

TEST_F(ConfigSchemaValidatorTest, Performance_100FieldConfig_200RuleSchema_Under5ms) {
    // Build a 100-field JSON object: fields field_0 … field_99, each an integer.
    nlohmann::json data = nlohmann::json::object();
    for (int i = 0; i < 100; ++i) {
        data["field_" + std::to_string(i)] = i;
    }
    auto path = writeFile("perf_cfg.json", data.dump());

    // Build a schema with ~200 constraint rules spread across properties and
    // allOf / anyOf / not sub-schemas.
    nlohmann::json schema = nlohmann::json::object();
    schema["type"] = "object";

    // 100 property definitions, each with type + minimum + maximum (300 keywords total
    // but concentrated into 100 "rules" for properties).
    nlohmann::json props = nlohmann::json::object();
    nlohmann::json required_arr = nlohmann::json::array();
    for (int i = 0; i < 100; ++i) {
        const std::string key = "field_" + std::to_string(i);
        props[key] = {{"type", "integer"}, {"minimum", 0}, {"maximum", 999}};
        required_arr.push_back(key);
    }
    schema["properties"] = props;
    schema["required"] = required_arr;

    // Add 100 allOf sub-schemas (each checks one field is >= 0), bringing the
    // total number of distinct schema nodes well above 200.
    nlohmann::json all_of = nlohmann::json::array();
    for (int i = 0; i < 100; ++i) {
        nlohmann::json sub = nlohmann::json::object();
        sub["properties"] = nlohmann::json::object();
        sub["properties"]["field_" + std::to_string(i)] = {{"minimum", 0}};
        all_of.push_back(sub);
    }
    schema["allOf"] = all_of;

    // Warm up (ensure regex/JIT etc. are initialised).
    ConfigSchemaValidator::validate(path, schema);

    // Measure 10 iterations and check the *average* is under the 5 ms target.
    constexpr int iterations = 10;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int n = 0; n < iterations; ++n) {
        auto result = ConfigSchemaValidator::validate(path, schema);
        ASSERT_TRUE(result.valid) << result.formatErrors();
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    double avg_ms = std::chrono::duration<double, std::milli>(t1 - t0).count() /
                    static_cast<double>(iterations);

    EXPECT_LT(avg_ms, 5.0)
        << "Average validation time " << avg_ms
        << " ms exceeds the 5 ms acceptance criterion";
}

} // namespace test
} // namespace config
} // namespace themis
