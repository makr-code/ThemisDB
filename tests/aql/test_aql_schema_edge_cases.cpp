/**
 * @file test_aql_schema_edge_cases.cpp
 * @brief Phase 4 Block 4.2 Unit Tests — Schema Context Edge Cases
 *
 * Tests validation behavior for schema edge cases including:
 * - Null and empty schema contexts
 * - Missing collection metadata
 * - Type mismatch in field definitions
 * - Large schema handling
 * - Diagnostic hint presence in error context
 *
 * All tests are self-contained using inline mock implementations.
 * No external dependencies beyond GTest and aql/aql_error_types.h.
 */


#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <chrono>

#include "aql/aql_error_types.h"

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Mock Schema Validator
// Simulates the schema validation component without real AQL infrastructure
// ============================================================================

struct MockFieldDef {
    std::string name;
    std::string type;      // e.g. "string", "int", "float", "array", "object"
    bool required = false;
};

struct MockCollectionSchema {
    std::string collection_name;
    std::vector<MockFieldDef> fields;
    bool exists = true;
};

class MockSchemaValidator {
public:
    struct ValidationResult {
        bool success;
        std::string error_category;   // One of: MalformedAQL, SchemaMismatch, TypeMismatch, ...
        std::string error_message;
        std::string schema_field;     // Field involved (if applicable)
        std::string collection;       // Collection involved (if applicable)
    };

    /// Register a collection schema
    void registerCollection(const MockCollectionSchema& schema) {
        schemas_[schema.collection_name] = schema;
    }

    /// Validate a query against a known schema context
    ValidationResult validateQueryAgainstSchema(
        const std::string& aql_query,
        const std::string& schema_context) const
    {
        // Null/empty schema context check
        if (schema_context.empty()) {
            return {false, ValidationError::NullSchemaContext,
                    "[VALIDATION:NullSchemaContext] Schema context is null or empty: "
                    "collection scope cannot be verified",
                    "", ""};
        }

        // Empty AQL query check
        if (aql_query.empty()) {
            return {false, ValidationError::MalformedAQL,
                    "[VALIDATION:MalformedAQL] AQL query is empty or null: "
                    "provide a non-empty AQL query string",
                    "", ""};
        }

        // Simulate schema mismatch check: look for unknown field references
        for (const auto& [col_name, schema] : schemas_) {
            if (schema_context.find(col_name) == std::string::npos) {
                continue;  // Collection not in scope
            }
            if (!schema.exists) {
                return {false, ValidationError::SchemaMismatch,
                        "[VALIDATION:SchemaMismatch] Collection '" + col_name +
                        "' referenced in query does not exist in schema",
                        "", col_name};
            }
        }

        return {true, "", "", "", ""};
    }

    /// Check type consistency for a given field filter expression
    ValidationResult checkFieldType(const std::string& collection,
                                    const std::string& field,
                                    const std::string& filter_type) const
    {
        auto it = schemas_.find(collection);
        if (it == schemas_.end()) {
            return {false, ValidationError::MissingFieldMetadata,
                    "[VALIDATION:MissingFieldMetadata] Collection '" + collection +
                    "' not found in registered schemas",
                    field, collection};
        }
        const auto& schema = it->second;
        for (const auto& fd : schema.fields) {
            if (fd.name == field) {
                if (fd.type != filter_type) {
                    return {false, ValidationError::TypeMismatch,
                            "[VALIDATION:TypeMismatch] Field '" + field +
                            "' in '" + collection + "' has type '" + fd.type +
                            "' but filter expects '" + filter_type + "'",
                            field, collection};
                }
                return {true, "", "", field, collection};
            }
        }
        return {false, ValidationError::MissingFieldMetadata,
                "[VALIDATION:MissingFieldMetadata] Field '" + field +
                "' not found in collection '" + collection + "' schema",
                field, collection};
    }

private:
    std::unordered_map<std::string, MockCollectionSchema> schemas_;
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test SchemaEdgeCases_NullSchemaContextIsRejected
 *
 * Verify that a null (empty) schema context is explicitly rejected
 * with fail-closed semantics and a structured error category.
 */
TEST(AQLSchemaEdgeCases, NullSchemaContextIsRejected) {
    MockSchemaValidator validator;
    validator.registerCollection({"users", {{"id", "string", true}, {"name", "string", false}}, true});

    auto result = validator.validateQueryAgainstSchema(
        "FOR u IN users RETURN u",
        ""  // null/empty schema context
    );

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_category, ValidationError::NullSchemaContext);
    EXPECT_TRUE(result.error_message.find("[VALIDATION:NullSchemaContext]") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("scope cannot be verified") != std::string::npos);

    // Verify fail-closed recovery strategy
    auto strategy = getRecoveryStrategy("validation", ValidationError::NullSchemaContext);
    // NullSchemaContext: no schema means we cannot validate — FAIL_CLOSED
    EXPECT_NE(strategy, RecoveryStrategy::DEGRADE_GRACEFULLY);
}

/**
 * @test SchemaEdgeCases_EmptySchemaContextHandledGracefully
 *
 * Verify that an empty AQL query is also rejected with a clear diagnostic hint.
 */
TEST(AQLSchemaEdgeCases, EmptySchemaContextHandledGracefully) {
    MockSchemaValidator validator;

    auto result = validator.validateQueryAgainstSchema(
        "",   // empty AQL query
        "users_collection"
    );

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_category, ValidationError::MalformedAQL);
    EXPECT_TRUE(result.error_message.find("[VALIDATION:MalformedAQL]") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("non-empty AQL query") != std::string::npos);

    // Create an AQLErrorContext to verify structured reporting
    AQLErrorContext ctx(
        "validation",
        ValidationError::MalformedAQL,
        "mock_schema_validator",
        result.error_message
    );
    ctx.addDiagnosticHint("Provide a valid AQL query string before invoking validation");
    ctx.setRecoverable(false);

    EXPECT_FALSE(ctx.isRecoverable());
    EXPECT_TRUE(ctx.formatForLogging().find("MalformedAQL") != std::string::npos);
}

/**
 * @test SchemaEdgeCases_MissingCollectionMetadataReturnsError
 *
 * Verify that referencing an unregistered collection returns
 * MissingFieldMetadata with the collection name in the error context.
 */
TEST(AQLSchemaEdgeCases, MissingCollectionMetadataReturnsError) {
    MockSchemaValidator validator;
    // 'orders' collection is NOT registered — only 'users' is

    validator.registerCollection({"users", {{"id", "string", true}, {"name", "string", false}}, true});

    // Attempt to check a field in an unregistered collection
    auto result = validator.checkFieldType("orders", "order_id", "string");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_category, ValidationError::MissingFieldMetadata);
    EXPECT_TRUE(result.error_message.find("[VALIDATION:MissingFieldMetadata]") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("orders") != std::string::npos);
    EXPECT_EQ(result.collection, "orders");

    // Create structured error context
    AQLErrorContext ctx(
        "validation",
        ValidationError::MissingFieldMetadata,
        "mock_schema_validator",
        result.error_message
    );
    ctx.setSchemaContext(result.schema_field, result.collection);
    ctx.addDiagnosticHint("Register 'orders' collection schema before executing queries");
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getSchemaCollection(), "orders");
    EXPECT_FALSE(ctx.isRecoverable());
}

/**
 * @test SchemaEdgeCases_TypeMismatchInFieldDefinitionDetected
 *
 * Verify that type mismatch between a field definition and a filter
 * expression is detected and reported with the TypeMismatch category.
 */
TEST(AQLSchemaEdgeCases, TypeMismatchInFieldDefinitionDetected) {
    MockSchemaValidator validator;
    validator.registerCollection({
        "products",
        {
            {"id",    "string", true},
            {"price", "float",  false},
            {"count", "int",    false}
        },
        true
    });

    // Try to filter 'price' (float) as if it were 'string' — type mismatch
    auto result = validator.checkFieldType("products", "price", "string");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_category, ValidationError::TypeMismatch);
    EXPECT_TRUE(result.error_message.find("[VALIDATION:TypeMismatch]") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("price") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("float") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("string") != std::string::npos);
    EXPECT_EQ(result.schema_field, "price");
    EXPECT_EQ(result.collection, "products");

    // Verify that a correct type passes
    auto ok_result = validator.checkFieldType("products", "price", "float");
    EXPECT_TRUE(ok_result.success);
    EXPECT_EQ(ok_result.schema_field, "price");
}

/**
 * @test SchemaEdgeCases_LargeSchemaHandledEfficiently
 *
 * Verify that schema validation handles a large number of fields
 * without performance degradation (basic wall-clock smoke test).
 */
TEST(AQLSchemaEdgeCases, LargeSchemaHandledEfficiently) {
    MockSchemaValidator validator;

    // Build a collection with 500 fields
    MockCollectionSchema large_schema;
    large_schema.collection_name = "wide_events";
    large_schema.exists = true;

    for (int i = 0; i < 500; ++i) {
        large_schema.fields.push_back(
            {"field_" + std::to_string(i), (i % 3 == 0) ? "int" : (i % 3 == 1) ? "string" : "float", false}
        );
    }
    validator.registerCollection(large_schema);

    // Build a schema context string that includes the collection
    std::ostringstream schema_ctx;
    schema_ctx << "collection:wide_events fields:[";
    for (int i = 0; i < 10; ++i) {  // include a few field refs
        schema_ctx << "field_" << i << ",";
    }
    schema_ctx << "]";

    // Time the validation
    auto t0 = std::chrono::steady_clock::now();
    for (int iter = 0; iter < 100; ++iter) {
        auto result = validator.validateQueryAgainstSchema(
            "FOR e IN wide_events FILTER e.field_0 == 1 RETURN e",
            schema_ctx.str()
        );
        EXPECT_TRUE(result.success);
    }
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();

    // 100 iterations over a 500-field schema should complete in well under 500ms
    EXPECT_LT(elapsed_us, 500'000) << "Large schema validation is too slow: "
                                   << elapsed_us << " µs for 100 iterations";

    // Also verify type lookup still works correctly at scale
    auto type_result = validator.checkFieldType("wide_events", "field_0", "int");
    EXPECT_TRUE(type_result.success);

    auto mismatch_result = validator.checkFieldType("wide_events", "field_1", "int");
    EXPECT_FALSE(mismatch_result.success);  // field_1 is "string"
    EXPECT_EQ(mismatch_result.error_category, ValidationError::TypeMismatch);
}

/**
 * @test SchemaEdgeCases_ErrorContextContainsDiagnosticHint
 *
 * Verify that AQLErrorContext produced during schema validation
 * contains actionable diagnostic hints in its formatted log output.
 */
TEST(AQLSchemaEdgeCases, ErrorContextContainsDiagnosticHint) {
    // Simulate the enriched error context that Block 4.2 produces
    AQLErrorContext ctx(
        "validation",
        ValidationError::SchemaMismatch,
        "validateAQLWithParser",
        "[VALIDATION:SchemaMismatch] Parser error: collection 'invoices' not found in schema"
    );
    ctx.setOperationType("translate_nl_to_aql");
    ctx.setSchemaContext("", "invoices", "");
    ctx.addDiagnosticHint("Verify collection 'invoices' exists in the schema context passed to the API");
    ctx.addDiagnosticHint("Check schema_context string for typos or missing collection declarations");
    ctx.setRecoverable(false);
    ctx.setLineNumber(1);
    ctx.setTokenPosition(14);

    std::string log_output = ctx.formatForLogging();

    // Must contain the structured category tag in the message
    EXPECT_TRUE(log_output.find("SchemaMismatch") != std::string::npos);
    EXPECT_TRUE(log_output.find("VALIDATION") != std::string::npos);
    EXPECT_TRUE(log_output.find("invoices") != std::string::npos);

    // Must contain at least one diagnostic hint
    EXPECT_TRUE(log_output.find("Hints=[") != std::string::npos);
    EXPECT_TRUE(log_output.find("Verify collection") != std::string::npos);

    // Must be marked non-recoverable (schema mismatch → fail-closed)
    EXPECT_TRUE(log_output.find("Recoverable=no") != std::string::npos);

    // Recovery strategy should be FAIL_CLOSED for SchemaMismatch
    auto strategy = getRecoveryStrategy("validation", ValidationError::SchemaMismatch);
    EXPECT_EQ(strategy, RecoveryStrategy::FAIL_CLOSED);
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
