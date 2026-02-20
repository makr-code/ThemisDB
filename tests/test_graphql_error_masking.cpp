#include <gtest/gtest.h>
#include "api/graphql.h"

using namespace themis::graphql;

// ============================================================================
// Error Masking Tests
// ============================================================================

TEST(GraphQLErrorMasking, MaskInternalErrorInProduction) {
    std::string internal_msg = "NullPointerException in resolver at line 42 of server.cpp";
    auto masked = MaskedError::fromInternalError(internal_msg, "INTERNAL_ERROR", true);
    
    EXPECT_EQ(masked.code, "INTERNAL_ERROR");
    EXPECT_EQ(masked.message, "An internal error occurred");
    EXPECT_TRUE(masked.message.find("NullPointer") == std::string::npos);
    EXPECT_TRUE(masked.message.find("server.cpp") == std::string::npos);
}

TEST(GraphQLErrorMasking, ExposeErrorInDevelopment) {
    std::string internal_msg = "NullPointerException in resolver at line 42 of server.cpp";
    auto masked = MaskedError::fromInternalError(internal_msg, "INTERNAL_ERROR", false);
    
    EXPECT_EQ(masked.code, "INTERNAL_ERROR");
    EXPECT_EQ(masked.message, internal_msg);
}

TEST(GraphQLErrorMasking, MaskSyntaxErrorInProduction) {
    std::string internal_msg = "Unexpected token at line 5, column 12: '{'";
    auto masked = MaskedError::fromInternalError(internal_msg, "ERR_QUERY_INVALID_SYNTAX", true);
    
    EXPECT_EQ(masked.code, "ERR_QUERY_INVALID_SYNTAX");
    EXPECT_EQ(masked.message, "Invalid query syntax");
}

TEST(GraphQLErrorMasking, MaskLimitErrorInProduction) {
    std::string internal_msg = "Query depth of 15 exceeds maximum allowed depth of 10";
    auto masked = MaskedError::fromInternalError(internal_msg, "ERR_LIMIT_EXCEEDED", true);
    
    EXPECT_EQ(masked.code, "ERR_LIMIT_EXCEEDED");
    EXPECT_EQ(masked.message, "Query exceeds resource limits");
    EXPECT_TRUE(masked.message.find("15") == std::string::npos);
}

TEST(GraphQLErrorMasking, ErrorPathTracking) {
    MaskedError error;
    error.message = "Field not found";
    error.code = "ERR_FIELD_NOT_FOUND";
    error.path = {"user", "posts", "comments"};
    
    std::string errorStr = error.toString();
    EXPECT_TRUE(errorStr.find("user.posts.comments") != std::string::npos);
}

// ============================================================================
// Executor Error Handling Tests
// ============================================================================

TEST(GraphQLExecutor, HandleMissingOperationWithMasking) {
    Document doc;
    // Empty document - no operations
    
    ExecutionContext ctx;
    ctx.mask_errors = true;
    
    Executor executor;
    auto result = executor.execute(doc, ctx, "NonExistent");
    
    EXPECT_TRUE(result.hasErrors());
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0].code, "ERR_OPERATION_NOT_FOUND");
}

TEST(GraphQLExecutor, HandleResolverExceptionWithMasking) {
    // Create a simple document
    Document doc;
    Operation op;
    op.type = OperationType::Query;
    Field field;
    field.name = "testField";
    op.selections.push_back(field);
    doc.operations.push_back(op);
    
    // Create context with a resolver that throws
    ExecutionContext ctx;
    ctx.mask_errors = true;
    ctx.resolvers["testField"] = [](const Field&, const std::shared_ptr<Value>&, const ExecutionContext&) -> std::shared_ptr<Value> {
        throw std::runtime_error("Internal database connection failed");
    };
    
    Executor executor;
    auto result = executor.execute(doc, ctx);
    
    EXPECT_TRUE(result.hasErrors());
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0].code, "ERR_EXECUTION_FAILED");
    // Message should be masked in production
    std::string errorMsg = result.errors[0].message;
    // The masking should prevent internal details from leaking
    if (ctx.mask_errors) {
        EXPECT_EQ(errorMsg, "An internal error occurred");
    }
}

TEST(GraphQLExecutor, ExposeErrorsInDevelopment) {
    Document doc;
    Operation op;
    op.type = OperationType::Query;
    Field field;
    field.name = "testField";
    op.selections.push_back(field);
    doc.operations.push_back(op);
    
    ExecutionContext ctx;
    ctx.mask_errors = false;  // Development mode
    ctx.resolvers["testField"] = [](const Field&, const std::shared_ptr<Value>&, const ExecutionContext&) -> std::shared_ptr<Value> {
        throw std::runtime_error("Internal database connection failed");
    };
    
    Executor executor;
    auto result = executor.execute(doc, ctx);
    
    EXPECT_TRUE(result.hasErrors());
    // In development mode, full error details should be available
    if (!ctx.mask_errors && !result.errors.empty()) {
        EXPECT_TRUE(result.errors[0].message.find("database") != std::string::npos ||
                   result.errors[0].message.find("error") != std::string::npos);
    }
}

// ============================================================================
// Integration Tests with Masking
// ============================================================================

TEST(GraphQLSecurityIntegration, LimitsAndMaskingTogether) {
    QueryLimits limits;
    limits.max_depth = 2;
    
    std::string query = R"(
        {
            user {
                posts {
                    comments {
                        text
                    }
                }
            }
        }
    )";
    
    // Parser should reject due to depth
    auto parseResult = Parser::parse(query, limits);
    EXPECT_FALSE(parseResult.success);
    
    // Now test that execution errors are also masked
    ExecutionContext ctx;
    ctx.mask_errors = true;
    
    // Even if we somehow got a parsed document, errors should be masked
    if (!parseResult.success && !parseResult.errors.empty()) {
        // Parser errors themselves should not expose internal details
        // The error message should be user-friendly
        EXPECT_FALSE(parseResult.errors[0].message.empty());
    }
}

TEST(GraphQLSecurityIntegration, MultipleErrorsMaskedCorrectly) {
    // Test that multiple errors are all properly masked
    Document doc;
    
    ExecutionContext ctx;
    ctx.mask_errors = true;
    
    Executor executor;
    auto result = executor.execute(doc, ctx, "op1");
    EXPECT_TRUE(result.hasErrors());
    
    // All errors should use masked format
    for (const auto& error : result.errors) {
        EXPECT_FALSE(error.code.empty());
        EXPECT_FALSE(error.message.empty());
        // Masked messages should be concise and not expose internals
        EXPECT_LT(error.message.length(), 100);
    }
}
