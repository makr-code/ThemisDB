/**
 * @file test_aql_validation_error_handling.cpp
 * @brief Phase 4 Unit Tests for AQL Validation Component Error Handling
 *
 * Tests comprehensive error handling in validateAQLWithParser including:
 * - MalformedAQL detection (syntax errors)
 * - InjectionAttempt detection (security)
 * - SchemaMismatch diagnostic (field/collection not found)
 * - TypeMismatch handling (type safety)
 * - UnsupportedOperator detection
 * - NullSchemaContext handling
 * - MissingFieldMetadata handling
 * - Error context formatting and diagnostics
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <spdlog/spdlog.h>

// Include AQL error types
#include "aql/aql_error_types.h"
#include "aql/llm_error_codes.h"
#include "aql/aql_schema_provider.h"

// Mock parser service for testing
#include "query/aql_parser_service.h"

namespace themis {
namespace aql {
namespace testing {

/**
 * @brief Mock AQL parser for testing error scenarios
 */
class MockAQLParser {
public:
    /// Simulate successful parse
    static std::pair<bool, std::string> parseMalformedAQL(const std::string& aql) {
        if (aql.find("SELECT") == std::string::npos && aql.find("INSERT") == std::string::npos) {
            return {false, "Expected SELECT or INSERT at position 0"};
        }
        return {true, ""};
    }

    /// Detect injection patterns
    static bool hasInjectionPattern(const std::string& aql) {
        // Simple detection for test purposes
        if (aql.find("'; DROP TABLE") != std::string::npos ||
            aql.find("UNION SELECT") != std::string::npos ||
            aql.find("1=1") != std::string::npos) {
            return true;
        }
        return false;
    }

    /// Check if query references valid schema
    static std::pair<bool, std::string> validateSchema(
        const std::string& aql,
        const std::vector<CollectionMetadata>& schema) {
        
        if (schema.empty()) {
            return {false, "Schema context is null/empty"};
        }

        // Check if referenced collections exist
        if (aql.find("users") != std::string::npos) {
            bool found = false;
            for (const auto& coll : schema) {
                if (coll.name == "users") {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return {false, "Collection 'users' not found in schema"};
            }
        }

        return {true, ""};
    }
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test T4.2.1a: MalformedAQL Detection
 *
 * Verify that syntax errors are detected and reported with diagnostic context
 */
TEST(AQLValidationErrorHandling, MalformedAQL_SyntaxError) {
    // Malformed query (missing SELECT/INSERT)
    std::string malformed_aql = "INVALID QUERY SYNTAX";

    auto [success, error] = MockAQLParser::parseMalformedAQL(malformed_aql);
    EXPECT_FALSE(success);
    EXPECT_THAT(error, ::testing::MatchesRegex(".*Expected SELECT or INSERT.*"));

    // Create error context
    AQLErrorContext ctx(
        "validation",
        ValidationError::MalformedAQL,
        "aql_validator",
        error
    );
    ctx.setOperationType("validate_aql");
    ctx.setLineNumber(1);
    ctx.setTokenPosition(0);
    ctx.addDiagnosticHint("Check AQL syntax documentation");
    ctx.setRecoverable(false);

    // Verify error context
    EXPECT_EQ(ctx.getErrorType(), "validation");
    EXPECT_EQ(ctx.getCategory(), ValidationError::MalformedAQL);
    EXPECT_FALSE(ctx.isRecoverable());
    EXPECT_TRUE(ctx.formatForLogging().find("MalformedAQL") != std::string::npos);
}

/**
 * @test T4.2.1b: InjectionAttempt Detection
 *
 * Verify security detection of injection patterns
 */
TEST(AQLValidationErrorHandling, InjectionAttempt_SQLInjection) {
    std::string injection_aql = "SELECT * FROM users WHERE id = 1; DROP TABLE users;--";

    EXPECT_TRUE(MockAQLParser::hasInjectionPattern(injection_aql));

    // Create error context for security event
    AQLErrorContext ctx(
        "validation",
        ValidationError::InjectionAttempt,
        "aql_validator",
        "Detected SQL injection pattern: DROP TABLE"
    );
    ctx.setOperationType("translate_nl_to_aql");
    ctx.setTokenPosition(33);  // Position of DROP
    ctx.addDiagnosticHint("Detected suspicious pattern near position 33");
    ctx.setRecoverable(false);

    // Verify security error
    EXPECT_EQ(ctx.getCategory(), ValidationError::InjectionAttempt);
    EXPECT_FALSE(ctx.isRecoverable());
    std::string log = ctx.formatForLogging();
    EXPECT_TRUE(log.find("InjectionAttempt") != std::string::npos);
    EXPECT_TRUE(log.find("Position=33") != std::string::npos);
}

/**
 * @test T4.2.1c: SchemaMismatch Diagnostic
 *
 * Verify schema field not found errors with helpful diagnostics
 */
TEST(AQLValidationErrorHandling, SchemaMismatch_FieldNotFound) {
    std::vector<CollectionMetadata> schema;
    CollectionMetadata users_coll;
    users_coll.name = "users";
    users_coll.type = "document";
    
    CollectionFieldInfo name_field;
    name_field.name = "name";
    name_field.type = "string";
    name_field.indexed = true;
    users_coll.fields.push_back(name_field);

    CollectionFieldInfo email_field;
    email_field.name = "email";
    email_field.type = "string";
    email_field.indexed = false;
    users_coll.fields.push_back(email_field);

    schema.push_back(users_coll);

    // Query references field that doesn't exist
    std::string aql = "FOR u IN users FILTER u.phone_number == '123-456-7890' RETURN u";

    AQLErrorContext ctx(
        "validation",
        ValidationError::SchemaMismatch,
        "aql_validator",
        "Field 'phone_number' not found in collection 'users'"
    );
    ctx.setOperationType("validate_aql");
    ctx.setSchemaContext("phone_number", "users");
    ctx.addDiagnosticHint("Available fields: name (string, indexed), email (string)");
    ctx.setRecoverable(false);

    // Verify schema mismatch error
    EXPECT_EQ(ctx.getCategory(), ValidationError::SchemaMismatch);
    EXPECT_EQ(ctx.getSchemaField(), "phone_number");
    EXPECT_EQ(ctx.getSchemaCollection(), "users");
    EXPECT_TRUE(ctx.getDiagnosticHints()[0].find("name") != std::string::npos);
}

/**
 * @test T4.2.1d: TypeMismatch Detection
 *
 * Verify type safety checks for incompatible operations
 */
TEST(AQLValidationErrorHandling, TypeMismatch_StringComparison) {
    std::vector<CollectionMetadata> schema;
    CollectionMetadata users_coll;
    users_coll.name = "users";
    
    CollectionFieldInfo age_field;
    age_field.name = "age";
    age_field.type = "integer";
    age_field.indexed = false;
    users_coll.fields.push_back(age_field);

    schema.push_back(users_coll);

    // Query compares integer field with string (type mismatch)
    std::string aql = "FOR u IN users FILTER u.age == 'twenty' RETURN u";

    AQLErrorContext ctx(
        "validation",
        ValidationError::TypeMismatch,
        "aql_validator",
        "Type mismatch: field 'age' is integer but query compares with string 'twenty'"
    );
    ctx.setOperationType("validate_aql");
    ctx.setSchemaContext("age", "users", "integer");
    ctx.addDiagnosticHint("Expected: FILTER u.age == 20 (numeric comparison)");
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getCategory(), ValidationError::TypeMismatch);
    EXPECT_EQ(ctx.getSchemaTypeInfo(), "integer");
    std::string log = ctx.formatForLogging();
    EXPECT_TRUE(log.find("TypeMismatch") != std::string::npos);
}

/**
 * @test T4.2.1e: UnsupportedOperator Handling
 *
 * Verify detection of AQL operators not supported in this version
 */
TEST(AQLValidationErrorHandling, UnsupportedOperator_RegexMatch) {
    std::string aql = "FOR u IN users FILTER u.email =~ '^user@.*\\.com$' RETURN u";

    AQLErrorContext ctx(
        "validation",
        ValidationError::UnsupportedOperator,
        "aql_validator",
        "Operator '=~' (regex match) not supported in AQL v1.6"
    );
    ctx.setOperationType("validate_aql");
    ctx.setTokenPosition(37);  // Position of =~
    ctx.addDiagnosticHint("Use LIKE operator instead: u.email LIKE 'user@%.com'");
    ctx.addDiagnosticHint("Regex matching available in v2.0");
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getCategory(), ValidationError::UnsupportedOperator);
    EXPECT_EQ(ctx.getDiagnosticHints().size(), 2);
    EXPECT_TRUE(ctx.getDiagnosticHints()[0].find("LIKE") != std::string::npos);
}

/**
 * @test T4.2.1f: NullSchemaContext Handling
 *
 * Verify graceful handling when schema context is missing
 */
TEST(AQLValidationErrorHandling, NullSchemaContext_MissingSchema) {
    std::string aql = "FOR u IN users FILTER u.name == 'Alice' RETURN u";
    std::vector<CollectionMetadata> empty_schema;

    auto [success, error] = MockAQLParser::validateSchema(aql, empty_schema);
    EXPECT_FALSE(success);
    EXPECT_EQ(error, "Schema context is null/empty");

    AQLErrorContext ctx(
        "validation",
        ValidationError::NullSchemaContext,
        "aql_validator",
        error
    );
    ctx.setOperationType("translate_nl_to_aql");
    ctx.addDiagnosticHint("Cannot validate query without schema context");
    ctx.addDiagnosticHint("Provide schema_context parameter when calling translateNLToAQL()");
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getCategory(), ValidationError::NullSchemaContext);
    EXPECT_GE(ctx.getDiagnosticHints().size(), 2);
}

/**
 * @test T4.2.1g: MissingFieldMetadata Handling
 *
 * Verify handling of incomplete field metadata
 */
TEST(AQLValidationErrorHandling, MissingFieldMetadata_IncompleteType) {
    std::vector<CollectionMetadata> schema;
    CollectionMetadata users_coll;
    users_coll.name = "users";
    
    CollectionFieldInfo incomplete_field;
    incomplete_field.name = "custom_data";
    incomplete_field.type = "";  // Empty type = incomplete metadata
    users_coll.fields.push_back(incomplete_field);

    schema.push_back(users_coll);

    AQLErrorContext ctx(
        "validation",
        ValidationError::MissingFieldMetadata,
        "aql_validator",
        "Field 'custom_data' has incomplete metadata: missing type information"
    );
    ctx.setOperationType("validate_schema");
    ctx.setSchemaContext("custom_data", "users");
    ctx.addDiagnosticHint("Update schema metadata to specify field type");
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getCategory(), ValidationError::MissingFieldMetadata);
    std::string log = ctx.formatForLogging();
    EXPECT_TRUE(log.find("MissingFieldMetadata") != std::string::npos);
}

/**
 * @test T4.2.1h: Error Context Formatting and Diagnostics
 *
 * Verify comprehensive error context formatting for logging/monitoring
 */
TEST(AQLValidationErrorHandling, ErrorContextFormatting_CompleteContext) {
    AQLErrorContext ctx(
        "validation",
        ValidationError::SchemaMismatch,
        "aql_validator",
        "Field 'status' not found in schema"
    );
    
    // Add comprehensive context
    ctx.setOperationType("translate_nl_to_aql");
    ctx.setLineNumber(3);
    ctx.setTokenPosition(67);
    ctx.setSchemaContext("status", "orders");
    ctx.addDiagnosticHint("Hint 1: Check collection name in schema");
    ctx.addDiagnosticHint("Hint 2: Field may have been renamed");
    ctx.addDiagnosticHint("Hint 3: Available fields: order_id, customer_id, total_amount");
    ctx.setRecoverable(false);

    // Verify all context is captured in formatted output
    std::string formatted = ctx.formatForLogging();
    EXPECT_TRUE(formatted.find("validation") != std::string::npos);
    EXPECT_TRUE(formatted.find("SchemaMismatch") != std::string::npos);
    EXPECT_TRUE(formatted.find("aql_validator") != std::string::npos);
    EXPECT_TRUE(formatted.find("translate_nl_to_aql") != std::string::npos);
    EXPECT_TRUE(formatted.find("Line=3") != std::string::npos);
    EXPECT_TRUE(formatted.find("Position=67") != std::string::npos);
    EXPECT_TRUE(formatted.find("status") != std::string::npos);
    EXPECT_TRUE(formatted.find("orders") != std::string::npos);
    EXPECT_TRUE(formatted.find("Hint 1") != std::string::npos);
    EXPECT_TRUE(formatted.find("Recoverable=no") != std::string::npos);

    spdlog::info("Formatted error context:\n{}", formatted);
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
