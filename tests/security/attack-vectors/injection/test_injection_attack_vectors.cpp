/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_injection_attack_vectors.cpp                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-16 04:22:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     319                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9f7d34b9d  2026-03-09  feat(security): add attack vector tests and promote PQ cr... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_injection_attack_vectors.cpp
 * @brief Injection attack vector tests for ThemisDB security module.
 *
 * Systematically validates that the AQL injection detector correctly blocks
 * known injection patterns while permitting well-formed queries.
 *
 * Attack categories covered:
 *   - Classic AQL / SQL-style injection via string concatenation
 *   - Comment-marker injection (-- , /*, #)
 *   - Dangerous operation injection (UPDATE, DELETE, DROP, INSERT)
 *   - Boolean-based blind injection ('OR 1=1', 'AND 1=2')
 *   - Union-based injection
 *   - Stacked-query attempts (semicolons)
 *   - Parameter pollution / oversized parameters
 *   - Unicode and case-variation bypass attempts
 *   - Parameterized safe queries (must be allowed)
 *
 * CWE mapping:
 *   CWE-89  – SQL/AQL Injection
 *   CWE-94  – Code Injection
 *   CWE-116 – Improper Encoding or Escaping of Output
 *
 * OWASP ASVS:
 *   V5.2  – Sanitization and Sandboxing Requirements
 *   V5.3  – Output Encoding and Injection Prevention
 */

#include <gtest/gtest.h>
#include "security/aql_injection_detector.h"

#include <string>
#include <vector>

using namespace themis::security;

// ─── Test Fixture ─────────────────────────────────────────────────────────

class InjectionAttackVectorTest : public ::testing::Test {
protected:
    AQLInjectionDetector detector_;
};

// ============================================================================
// Positive tests: legitimate queries must pass
// ============================================================================

TEST_F(InjectionAttackVectorTest, Positive_SimpleReadQuery) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @name RETURN doc",
        {"Alice"}
    );
    EXPECT_TRUE(result.is_safe)
        << "Legitimate parameterized read query must pass. Error: "
        << result.error_message;
}

TEST_F(InjectionAttackVectorTest, Positive_NumericParameter) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN orders FILTER doc.amount >= @min RETURN doc",
        {"100"}
    );
    EXPECT_TRUE(result.is_safe)
        << "Numeric parameter must be accepted. Error: " << result.error_message;
}

TEST_F(InjectionAttackVectorTest, Positive_MultipleParameters) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN logs FILTER doc.level == @lvl AND doc.code == @code RETURN doc",
        {"error", "404"}
    );
    EXPECT_TRUE(result.is_safe)
        << "Multiple safe parameters must be accepted. Error: " << result.error_message;
}

// ============================================================================
// Attack Vector: Comment-Marker Injection
// Attackers append -- or /* to terminate the template and inject new clauses.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_SQLLineCommentInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @name RETURN doc",
        {"Alice' -- injected comment"}
    );
    EXPECT_FALSE(result.is_safe)
        << "SQL line comment (--) in parameter must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_CBlockCommentInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.id == @id RETURN doc",
        {"1 /* overwrite filter */"}
    );
    EXPECT_FALSE(result.is_safe)
        << "C-style block comment (/* */) in parameter must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_HashCommentInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN items FILTER doc.sku == @sku RETURN doc",
        {"X100 # bypass"}
    );
    EXPECT_FALSE(result.is_safe)
        << "Hash comment (#) in parameter must be rejected";
}

// ============================================================================
// Attack Vector: Dangerous Operation Injection
// Attackers embed UPDATE, DELETE, DROP etc. inside a parameter value.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_DeleteKeywordInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN documents FILTER doc.title == @title RETURN doc",
        {"'; FOR d IN documents DELETE d IN documents //"}
    );
    EXPECT_FALSE(result.is_safe)
        << "DELETE keyword in parameter must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_UpdateKeywordInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.username == @user RETURN doc",
        {"admin' UPDATE users WITH {role:'superadmin'} IN users //"}
    );
    EXPECT_FALSE(result.is_safe)
        << "UPDATE keyword in parameter must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_DropKeywordInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN collections FILTER doc.name == @name RETURN doc",
        {"anything DROP COLLECTION users"}
    );
    EXPECT_FALSE(result.is_safe)
        << "DROP keyword in parameter must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_InsertKeywordInParam) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.email == @email RETURN doc",
        {"attacker@evil.com' INSERT {role:'admin'} INTO admins //"}
    );
    EXPECT_FALSE(result.is_safe)
        << "INSERT keyword in parameter must be rejected";
}

// ============================================================================
// Attack Vector: Boolean-Based Blind Injection
// Classic 'OR 1=1' patterns that bypass filter conditions.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_BooleanBlind_Or1Eq1) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.id == @id RETURN doc",
        {"1 OR 1==1"}
    );
    EXPECT_FALSE(result.is_safe)
        << "Boolean-blind injection 'OR 1==1' in parameter must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_BooleanBlind_OrTrueBypass) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN accounts FILTER doc.token == @tok RETURN doc",
        {"'' OR true"}
    );
    EXPECT_FALSE(result.is_safe)
        << "OR true bypass in parameter must be rejected";
}

// ============================================================================
// Attack Vector: Union-Based Injection
// Appending UNION to extend the query result set.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_UnionBasedInjection) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN products FILTER doc.category == @cat RETURN doc",
        {"books UNION FOR sec IN passwords RETURN sec"}
    );
    EXPECT_FALSE(result.is_safe)
        << "UNION-based injection in parameter must be rejected";
}

// ============================================================================
// Attack Vector: Stacked Queries (Semicolons)
// Terminates the current statement and appends a new one.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_StackedQuery_Semicolon) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN logs FILTER doc.source == @src RETURN doc",
        {"web; FOR d IN secrets RETURN d"}
    );
    EXPECT_FALSE(result.is_safe)
        << "Stacked query via semicolon in parameter must be rejected";
}

// ============================================================================
// Attack Vector: Template Injection (malicious template, not parameter)
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_MaliciousTemplate_DirectDelete) {
    auto result = detector_.validateParameterizedQuery(
        "FOR d IN users REMOVE d IN users",
        {}
    );
    EXPECT_FALSE(result.is_safe)
        << "Template containing REMOVE operation must be rejected";
}

TEST_F(InjectionAttackVectorTest, Attack_MaliciousTemplate_DirectUpdate) {
    auto result = detector_.validateParameterizedQuery(
        "FOR d IN users UPDATE d WITH {password: 'hacked'} IN users",
        {}
    );
    EXPECT_FALSE(result.is_safe)
        << "Template containing UPDATE operation must be rejected";
}

// ============================================================================
// Attack Vector: Case-Variation Bypass
// Uppercase / mixed-case attempts to bypass case-sensitive keyword matching.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_CaseVariation_LowercaseDelete) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.id == @id RETURN doc",
        {"1 delete doc in users"}
    );
    EXPECT_FALSE(result.is_safe)
        << "Lowercase 'delete' keyword in parameter must be rejected (case-insensitive check)";
}

TEST_F(InjectionAttackVectorTest, Attack_CaseVariation_MixedCaseUpdate) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.username == @u RETURN doc",
        {"admin uPdAtE doc WITH {admin:true} IN users"}
    );
    EXPECT_FALSE(result.is_safe)
        << "Mixed-case 'uPdAtE' in parameter must be rejected";
}

// ============================================================================
// Attack Vector: Oversized / Boundary-Value Parameters
// Extremely long parameter values can trigger buffer overruns or ReDoS.
// ============================================================================

TEST_F(InjectionAttackVectorTest, Attack_OversizedParameter_10KBytes) {
    std::string huge_param(10 * 1024, 'A');  // 10 KB of 'A'
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.name == @name RETURN doc",
        {huge_param}
    );
    // Must not crash; the result (safe/unsafe) depends on implementation policy,
    // but the detector must handle it without throwing or segfaulting.
    SUCCEED() << "Oversized parameter must not crash the detector (result: "
              << (result.is_safe ? "safe" : "blocked") << ")";
}

TEST_F(InjectionAttackVectorTest, Attack_EmptyParameter_IsAllowed) {
    auto result = detector_.validateParameterizedQuery(
        "FOR doc IN users FILTER doc.tag == @tag RETURN doc",
        {""}
    );
    EXPECT_TRUE(result.is_safe)
        << "Empty string parameter must be accepted as a valid bound value";
}

// ============================================================================
// Attack Vector: AQL AST direct validation
// ============================================================================

TEST_F(InjectionAttackVectorTest, ASTValidation_SafeFilterQuery) {
    auto result = detector_.validateAQLAST(
        "FOR doc IN orders FILTER doc.status == 'pending' RETURN doc"
    );
    EXPECT_TRUE(result.is_safe)
        << "Well-formed AQL filter query must pass AST validation";
}

TEST_F(InjectionAttackVectorTest, ASTValidation_RemoveOperationBlocked) {
    auto result = detector_.validateAQLAST(
        "FOR doc IN users REMOVE doc IN users"
    );
    EXPECT_FALSE(result.is_safe)
        << "AQL REMOVE operation must be blocked by AST validator";
}

TEST_F(InjectionAttackVectorTest, ASTValidation_InsertOperationBlocked) {
    auto result = detector_.validateAQLAST(
        "INSERT {name: 'admin', role: 'superuser'} INTO users"
    );
    EXPECT_FALSE(result.is_safe)
        << "AQL INSERT operation must be blocked by AST validator";
}
