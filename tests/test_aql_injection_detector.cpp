#include <gtest/gtest.h>
#include "security/aql_injection_detector.h"
#include <iostream>

using namespace themis::security;

// ============================================================================
// Basic Validation Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, LegitimateQueryAccepted) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER doc.age > 18 RETURN doc"
    );
    
    EXPECT_TRUE(result.is_safe);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_TRUE(result.detected_patterns.empty());
}

TEST(AQLInjectionDetectorTest, LegitimateQueryWithString) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER doc.name == \"Alice\" RETURN doc"
    );
    
    EXPECT_TRUE(result.is_safe);
    EXPECT_TRUE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, LegitimateComplexQuery) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER doc.age > 18 AND doc.city == \"Berlin\" "
        "SORT doc.name ASC LIMIT 10 RETURN {name: doc.name, age: doc.age}"
    );
    
    EXPECT_TRUE(result.is_safe);
    EXPECT_TRUE(result.error_message.empty());
}

// ============================================================================
// SQL Injection Pattern Detection
// ============================================================================

TEST(AQLInjectionDetectorTest, RejectDropTablePattern) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER doc.name == \"x'; DROP TABLE users; --\" RETURN doc"
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_FALSE(result.detected_patterns.empty());
}

TEST(AQLInjectionDetectorTest, RejectDropWithoutQuotes) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; DROP TABLE users"
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, RejectCaseVariationDROP) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; DrOp TaBlE users"
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, RejectDeletePattern) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; DELETE FROM users WHERE 1=1"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectUpdatePattern) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; UPDATE users SET admin=1"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectInsertPattern) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; INSERT INTO backdoor VALUES('pwned')"
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Comment-Based Injection Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, RejectSQLCommentInjection) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER doc.name == \"admin'--\" RETURN doc"
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, RejectMultilineCommentInjection) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users /* DROP TABLE users */ RETURN doc"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectCommentObfuscation) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; DROP/**/TABLE/**/users"
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Union-Based Injection Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, RejectUnionSelect) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users UNION SELECT * FROM admin_users"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectUnionAll) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users UNION ALL SELECT password FROM users"
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Stacked Query Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, RejectStackedQueries) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN doc; DELETE FROM users"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectStackedInsert) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; INSERT INTO backdoor VALUES('pwned')"
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Command Execution Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, RejectExecCommand) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; EXEC sp_executesql"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectSystemCommand) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; SYSTEM('rm -rf /')"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectShellCommand) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; SHELL('cat /etc/passwd')"
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Time-Based Blind Injection Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, RejectWaitForDelay) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; WAITFOR DELAY '00:00:05'"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectBenchmark) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users; BENCHMARK(1000000, MD5('test'))"
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Parameterized Query Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, ValidParameterizedQuery) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @param0 RETURN doc",
        {"Alice"}
    );
    
    EXPECT_TRUE(result.is_safe);
    EXPECT_TRUE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, ValidParameterizedQueryMultipleParams) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @param0 AND doc.age > @param1 RETURN doc",
        {"Alice", "25"}
    );
    
    EXPECT_TRUE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, RejectParameterWithSQLKeywords) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @param0 RETURN doc",
        {"Alice'; DROP TABLE users; --"}
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, RejectParameterWithComments) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @param0 RETURN doc",
        {"Alice--"}
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, RejectExcessivelyLongParameter) {
    AQLInjectionDetector detector;
    
    std::string long_param(20000, 'A');  // 20000 bytes (exceeds MAX_PARAM_LENGTH of 10240 bytes = 10 KiB)
    
    auto result = detector.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @param0 RETURN doc",
        {long_param}
    );
    
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, RejectTemplateWithDangerousPattern) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateParameterizedQuery(
        "FOR doc IN users; DROP TABLE users",
        {}
    );
    
    EXPECT_FALSE(result.is_safe);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, EmptyQueryRejected) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST("");
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, MalformedQueryRejected) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN FILTER RETURN"
    );
    
    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, StringLiteralWithSafeContent) {
    AQLInjectionDetector detector;
    
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER doc.description == \"This is a test description\" RETURN doc"
    );
    
    EXPECT_TRUE(result.is_safe);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(AQLInjectionDetectorTest, MultipleValidQueries) {
    AQLInjectionDetector detector;
    
    std::vector<std::string> valid_queries = {
        "FOR doc IN users RETURN doc",
        "FOR doc IN users FILTER doc.age > 18 RETURN doc",
        "FOR doc IN users SORT doc.name ASC RETURN doc",
        "FOR doc IN users LIMIT 10 RETURN doc",
        "FOR doc IN users FILTER doc.city == \"Berlin\" LIMIT 100 RETURN doc"
    };
    
    for (const auto& query : valid_queries) {
        auto result = detector.validateAQLAST(query);
        EXPECT_TRUE(result.is_safe) << "Query rejected: " << query;
    }
}

TEST(AQLInjectionDetectorTest, MultipleInvalidQueries) {
    AQLInjectionDetector detector;
    
    std::vector<std::string> invalid_queries = {
        "FOR doc IN users; DROP TABLE users",
        "FOR doc IN users; DELETE FROM users",
        "FOR doc IN users UNION SELECT * FROM admin",
        "FOR doc IN users; EXEC sp_executesql",
        "FOR doc IN users; WAITFOR DELAY '00:00:05'"
    };
    
    for (const auto& query : invalid_queries) {
        auto result = detector.validateAQLAST(query);
        EXPECT_FALSE(result.is_safe) << "Query accepted: " << query;
    }
}

// ============================================================================
// AST-Level Validation Tests
//
// These tests specifically exercise the AST-level dangerous-operation check
// introduced in v1.4.0 (containsDangerousOperations / scanExpressionForDangerousOps).
// They document injection patterns that might evade a pure regex scan but are
// reliably caught once the query is parsed into an AST.
// ============================================================================

TEST(AQLInjectionDetectorTest, ASTLevel_RejectExecuteFunctionCall) {
    AQLInjectionDetector detector;

    // EXECUTE() is a dangerous function; the AST-level check detects it
    // regardless of whitespace variations that could trip up a regex.
    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN EXECUTE(\"DROP TABLE users\")"
    );

    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectExecFunctionCall) {
    AQLInjectionDetector detector;

    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN EXEC(\"sp_executesql\")"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectSystemFunctionCall) {
    AQLInjectionDetector detector;

    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER SYSTEM(\"rm -rf /\") == 0 RETURN doc"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectShellFunctionCall) {
    AQLInjectionDetector detector;

    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN SHELL(\"cat /etc/passwd\")"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectSleepFunctionCall) {
    AQLInjectionDetector detector;

    // SLEEP() is used in time-based blind injection attacks.
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER SLEEP(5) == 0 RETURN doc"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectBenchmarkFunctionCall) {
    AQLInjectionDetector detector;

    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER BENCHMARK(1000000, doc.id) RETURN doc"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectCaseInsensitiveFunctionCall) {
    AQLInjectionDetector detector;

    // Mixed-case function names must still be detected at AST level.
    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN eXeCuTe(\"payload\")"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectNestedDangerousFunctionCall) {
    AQLInjectionDetector detector;

    // Dangerous call nested inside a safe-looking outer call.
    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN CONCAT(doc.name, EXECUTE(\"payload\"))"
    );

    EXPECT_FALSE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_SafeBuiltinFunctionCallAccepted) {
    AQLInjectionDetector detector;

    // Legitimate built-in functions must not be rejected.
    auto result = detector.validateAQLAST(
        "FOR doc IN users FILTER LOWER(doc.name) == \"alice\" RETURN doc"
    );

    EXPECT_TRUE(result.is_safe);
}

TEST(AQLInjectionDetectorTest, ASTLevel_ParseFailureFallsBackToRegex) {
    AQLInjectionDetector detector;

    // The query is syntactically invalid, so the parser will fail.
    // The detector should still run the regex fallback and report
    // the suspicious pattern found in the raw string.
    auto result = detector.validateAQLAST(
        "FOR doc IN users !! DROP TABLE users"
    );

    // Must be rejected: either because of the parse failure itself or
    // because the regex fallback detects "DROP".
    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectXpCmdshellFunctionCall) {
    AQLInjectionDetector detector;

    // XP_CMDSHELL is detected by both the AST-level check and the regex layer.
    // This test validates the AST-level path: when XP_CMDSHELL appears as a
    // parsed function-call node the detector must reject the query.
    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN XP_CMDSHELL(\"dir c:\\\\\")"
    );

    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(AQLInjectionDetectorTest, ASTLevel_RejectSpExecuteSqlFunctionCall) {
    AQLInjectionDetector detector;

    // SP_EXECUTESQL is detected by both the AST-level check and the regex layer.
    // This test validates the AST-level path: when SP_EXECUTESQL appears as a
    // parsed function-call node the detector must reject the query.
    auto result = detector.validateAQLAST(
        "FOR doc IN users RETURN SP_EXECUTESQL(doc.query)"
    );

    EXPECT_FALSE(result.is_safe);
    EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// Main
// ============================================================================

// GoogleTest main is provided by gtest_main library

