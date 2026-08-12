/**
 * @file test_input_validation_comprehensive.cpp
 * @brief Comprehensive tests for the InputValidator security component
 *
 * Tests cover:
 * - Path segment validation (clean paths, traversal attacks)
 * - AQL query validation (valid queries, injection patterns)
 * - JSON schema stub validation
 * - Log sanitization (control chars, length limits)
 * - Edge cases (empty input, very long input, special characters)
 */

#include <gtest/gtest.h>
#include "utils/input_validator.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace themis::utils;

namespace {
// Helper: create validator with an empty schema dir (no schemas available)
InputValidator makeValidator(const std::string& schema_dir = "") {
    return InputValidator(schema_dir);
}
} // anonymous namespace

// ============================================================================
// Path Segment Validation Tests
// ============================================================================

class PathValidationTest : public ::testing::Test {
protected:
    InputValidator v_{makeValidator()};
};

TEST_F(PathValidationTest, ValidSegment_SimpleKey_Accepted) {
    EXPECT_TRUE(v_.validatePathSegment("abc123"));
    EXPECT_TRUE(v_.validatePathSegment("user-id-42"));
    EXPECT_TRUE(v_.validatePathSegment("record_001"));
    EXPECT_TRUE(v_.validatePathSegment("entity_name"));
}

TEST_F(PathValidationTest, ValidSegment_AlphanumericWithDashes_Accepted) {
    EXPECT_TRUE(v_.validatePathSegment("my-document"));
    EXPECT_TRUE(v_.validatePathSegment("doc-v1-2024"));
    EXPECT_TRUE(v_.validatePathSegment("user_profile_123"));
}

TEST_F(PathValidationTest, EmptySegment_Rejected) {
    EXPECT_FALSE(v_.validatePathSegment(""));
}

TEST_F(PathValidationTest, PathTraversal_DotDot_Rejected) {
    EXPECT_FALSE(v_.validatePathSegment(".."));
    EXPECT_FALSE(v_.validatePathSegment("../etc"));
    EXPECT_FALSE(v_.validatePathSegment("../../secret"));
    EXPECT_FALSE(v_.validatePathSegment("foo/../bar"));
}

TEST_F(PathValidationTest, PathSeparators_ForwardSlash_Rejected) {
    EXPECT_FALSE(v_.validatePathSegment("foo/bar"));
    EXPECT_FALSE(v_.validatePathSegment("/absolute"));
    EXPECT_FALSE(v_.validatePathSegment("dir/file.txt"));
}

TEST_F(PathValidationTest, PathSeparators_BackSlash_Rejected) {
    EXPECT_FALSE(v_.validatePathSegment("foo\\bar"));
    EXPECT_FALSE(v_.validatePathSegment("C:\\Windows\\system32"));
}

TEST_F(PathValidationTest, URLEncoded_TraversalAttempt_Rejected) {
    // %2e is URL-encoded dot
    EXPECT_FALSE(v_.validatePathSegment("%2e%2e"));
    EXPECT_FALSE(v_.validatePathSegment("..%2fpasswd"));
    EXPECT_FALSE(v_.validatePathSegment("%2e%2e%2fetc"));
}

TEST_F(PathValidationTest, ControlCharacters_Rejected) {
    EXPECT_FALSE(v_.validatePathSegment("null\x00byte"));
    EXPECT_FALSE(v_.validatePathSegment("newline\ninjection"));
    EXPECT_FALSE(v_.validatePathSegment("tab\tinjection"));
    EXPECT_FALSE(v_.validatePathSegment("carriage\rreturn"));
}

TEST_F(PathValidationTest, ExcessiveLength_Rejected) {
    std::string very_long(1025, 'a');
    EXPECT_FALSE(v_.validatePathSegment(very_long));
}

TEST_F(PathValidationTest, MaxLength_Accepted) {
    std::string at_limit(1024, 'a');
    EXPECT_TRUE(v_.validatePathSegment(at_limit));
}

// ============================================================================
// AQL Query Validation Tests
// ============================================================================

class AQLValidationTest : public ::testing::Test {
protected:
    InputValidator v_{makeValidator()};
};

TEST_F(AQLValidationTest, ValidQuery_SimpleFor_Accepted) {
    nlohmann::json req = {{"query", "FOR u IN users RETURN u"}};
    EXPECT_FALSE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, ValidQuery_WithFilter_Accepted) {
    nlohmann::json req = {
        {"query", "FOR u IN users FILTER u.name == @name RETURN u"},
        {"bindVars", {{"name", "Alice"}}}
    };
    EXPECT_FALSE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, ValidQuery_WithBindVars_Accepted) {
    nlohmann::json req = {
        {"query", "FOR doc IN @@collection FILTER doc.id == @id RETURN doc"},
        {"bindVars", {{"@collection", "users"}, {"id", "123"}}}
    };
    EXPECT_FALSE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, NotAnObject_Rejected) {
    nlohmann::json req = "not-an-object";
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, MissingQueryField_Rejected) {
    nlohmann::json req = {{"bindVars", {}}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, EmptyQuery_Rejected) {
    nlohmann::json req = {{"query", ""}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, ExcessivelyLongQuery_Rejected) {
    std::string huge_query(100001, 'A');
    nlohmann::json req = {{"query", huge_query}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, MultipleStatements_Rejected) {
    nlohmann::json req = {{"query", "FOR u IN users RETURN u;; DROP COLLECTION users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, InjectionPattern_Drop_Rejected) {
    nlohmann::json req = {{"query", "FOR u IN users FILTER u.name == 'x' drop collection users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, InjectionPattern_Truncate_Rejected) {
    nlohmann::json req = {{"query", "truncate users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, InjectionPattern_Grant_Rejected) {
    nlohmann::json req = {{"query", "grant admin to user1"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, InjectionPattern_Insert_Rejected) {
    nlohmann::json req = {{"query", "insert {evil: true} into users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, InjectionPattern_Update_Rejected) {
    nlohmann::json req = {{"query", "update u with {admin: true} in users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, InjectionPattern_Delete_Rejected) {
    nlohmann::json req = {{"query", "delete u in users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, ControlCharactersInQuery_Rejected) {
    nlohmann::json req = {{"query", "FOR u IN users\x01 RETURN u"}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, BindVarsNotObject_Rejected) {
    nlohmann::json req = {
        {"query", "FOR u IN users RETURN u"},
        {"bindVars", "not-an-object"}
    };
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, QueryFieldNotString_Rejected) {
    nlohmann::json req = {{"query", 12345}};
    EXPECT_TRUE(v_.validateAqlRequest(req).has_value());
}

TEST_F(AQLValidationTest, CaseInsensitiveInjection_Rejected) {
    // Injection in uppercase/mixed case
    nlohmann::json req1 = {{"query", "FOR u IN users RETURN u; DROP collection users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req1).has_value());

    nlohmann::json req2 = {{"query", "TRUNCATE users"}};
    EXPECT_TRUE(v_.validateAqlRequest(req2).has_value());
}

// ============================================================================
// Log Sanitization Tests
// ============================================================================

class LogSanitizationTest : public ::testing::Test {
protected:
    InputValidator v_{makeValidator()};
};

TEST_F(LogSanitizationTest, PlainText_Unchanged) {
    std::string input = "Normal log message";
    auto result = v_.sanitizeForLogs(input);
    EXPECT_EQ(result, input);
}

TEST_F(LogSanitizationTest, ControlChars_Stripped) {
    std::string input = "line1\nline2\r\nline3\t";
    auto result = v_.sanitizeForLogs(input);
    // Control chars should be removed
    EXPECT_TRUE(result.find('\n') == std::string::npos);
    EXPECT_TRUE(result.find('\r') == std::string::npos);
    EXPECT_TRUE(result.find('\t') == std::string::npos);
}

TEST_F(LogSanitizationTest, NullByte_Stripped) {
    std::string input = "prefix\x00suffix";
    // Note: string with embedded NUL
    std::string s{"prefix\x00suffix", 13};
    auto result = v_.sanitizeForLogs(s);
    EXPECT_TRUE(result.find('\x00') == std::string::npos);
}

TEST_F(LogSanitizationTest, LengthLimit_Truncated) {
    std::string long_input(1000, 'X');
    auto result = v_.sanitizeForLogs(long_input, 100);
    EXPECT_EQ(result.size(), 100u);
}

TEST_F(LogSanitizationTest, DefaultLimit_512Chars) {
    std::string long_input(1000, 'A');
    auto result = v_.sanitizeForLogs(long_input);
    EXPECT_EQ(result.size(), 512u);
}

TEST_F(LogSanitizationTest, ShortInput_NotTruncated) {
    std::string input = "Short message";
    auto result = v_.sanitizeForLogs(input, 512);
    EXPECT_EQ(result, input);
}

TEST_F(LogSanitizationTest, FormatStringChars_Preserved_NoExpansion) {
    // Format string characters should be preserved as literals (no expansion)
    std::string input = "%s %d %n %x";
    auto result = v_.sanitizeForLogs(input);
    EXPECT_EQ(result, input); // sanitizeForLogs just strips control chars
}

// ============================================================================
// JSON Schema Stub Validation Tests
// ============================================================================

class JSONSchemaValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "validator_schema_test";
        std::filesystem::create_directories(tmp_dir_);
        v_ = std::make_unique<InputValidator>(tmp_dir_.string());
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    void writeSchema(const std::string& name, const nlohmann::json& schema) {
        std::ofstream f((tmp_dir_ / (name + ".json")).string());
        f << schema.dump();
    }

    std::filesystem::path tmp_dir_;
    std::unique_ptr<InputValidator> v_;
};

TEST_F(JSONSchemaValidationTest, NoSchema_RejectsFailClosed) {
    // No schema file -> fail-closed: request is rejected with an error message
    // that names the missing schema.
    nlohmann::json payload = {{"anything", "goes"}};
    auto err = v_->validateJsonStub(payload, "nonexistent_schema");
    EXPECT_TRUE(err.has_value());
    EXPECT_NE(err->find("nonexistent_schema"), std::string::npos);
    // The error message should reference the schema directory so the operator
    // can determine where to place the missing file.
    EXPECT_NE(err->find(tmp_dir_.string()), std::string::npos);
}

TEST_F(JSONSchemaValidationTest, RequiredField_Present_Accepted) {
    writeSchema("test_schema", {
        {"type", "object"},
        {"required", {"name", "age"}}
    });
    nlohmann::json payload = {{"name", "Alice"}, {"age", 30}};
    auto err = v_->validateJsonStub(payload, "test_schema");
    EXPECT_FALSE(err.has_value());
}

TEST_F(JSONSchemaValidationTest, RequiredField_Missing_Rejected) {
    writeSchema("test_schema2", {
        {"type", "object"},
        {"required", {"name"}}
    });
    nlohmann::json payload = {{"age", 30}};
    auto err = v_->validateJsonStub(payload, "test_schema2");
    EXPECT_TRUE(err.has_value());
    EXPECT_NE(err->find("name"), std::string::npos);
}

TEST_F(JSONSchemaValidationTest, TypeValidation_StringField_Correct) {
    writeSchema("typed_schema", {
        {"type", "object"},
        {"properties", {
            {"username", {{"type", "string"}}}
        }}
    });
    nlohmann::json payload = {{"username", "alice"}};
    EXPECT_FALSE(v_->validateJsonStub(payload, "typed_schema").has_value());
}

TEST_F(JSONSchemaValidationTest, TypeValidation_StringField_WrongType) {
    writeSchema("typed_schema2", {
        {"type", "object"},
        {"properties", {
            {"username", {{"type", "string"}}}
        }}
    });
    nlohmann::json payload = {{"username", 42}};  // number, not string
    auto err = v_->validateJsonStub(payload, "typed_schema2");
    EXPECT_TRUE(err.has_value());
}
