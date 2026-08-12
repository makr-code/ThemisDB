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

// ============================================================================
// Read-Only Context Validation
// validateForReadOnlyContext() must accept pure read queries and reject any
// query that contains write or DDL operations.
// ============================================================================

TEST_F(InjectionAttackVectorTest, ReadOnly_PureReadQuery_Allowed) {
    auto result = detector_.validateForReadOnlyContext(
        "FOR doc IN users FILTER doc.active == true LIMIT 20 RETURN doc"
    );
    EXPECT_TRUE(result.is_safe)
        << "Pure read query must be allowed in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_InsertBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "INSERT {name: 'attacker', role: 'admin'} INTO users"
    );
    EXPECT_FALSE(result.is_safe)
        << "INSERT must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_UpdateBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "FOR doc IN users UPDATE doc WITH {role: 'admin'} IN users"
    );
    EXPECT_FALSE(result.is_safe)
        << "UPDATE must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_ReplaceBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "FOR doc IN users REPLACE doc WITH {role: 'admin'} IN users"
    );
    EXPECT_FALSE(result.is_safe)
        << "REPLACE must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_UpsertBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "UPSERT {name: 'evil'} INSERT {name: 'evil'} UPDATE {} IN users"
    );
    EXPECT_FALSE(result.is_safe)
        << "UPSERT must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_RemoveBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "FOR doc IN users REMOVE doc IN users"
    );
    EXPECT_FALSE(result.is_safe)
        << "REMOVE must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_DeleteKeywordBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "FOR doc IN users DELETE doc IN users"
    );
    EXPECT_FALSE(result.is_safe)
        << "DELETE must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_DropCollectionBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "DROP COLLECTION users"
    );
    EXPECT_FALSE(result.is_safe)
        << "DROP COLLECTION must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_DropCollectionRejectedByAstParse) {
    auto result = detector_.validateForReadOnlyContext(
        "DROP COLLECTION users"
    );
    EXPECT_FALSE(result.is_safe);
    EXPECT_NE(result.error_message.find("Parse error"), std::string::npos)
        << "Read-only validation must reject non-AQL write syntax via parser/AST path";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_CreateCollectionBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "CREATE COLLECTION backdoor"
    );
    EXPECT_FALSE(result.is_safe)
        << "CREATE COLLECTION must be rejected in read-only context";
}

TEST_F(InjectionAttackVectorTest, ReadOnly_CaseInsensitiveWriteBlocked) {
    auto result = detector_.validateForReadOnlyContext(
        "for doc in users insert doc into shadow_users"
    );
    EXPECT_FALSE(result.is_safe)
        << "Lowercase 'insert' must be rejected in read-only context (case-insensitive)";
}

// ============================================================================
// Unbounded FOR Loop Validation
// validateUnboundedForLoops() must reject queries with FOR loops that have no
// LIMIT clause, while allowing bounded and aggregate queries.
// ============================================================================

TEST_F(InjectionAttackVectorTest, UnboundedLoop_BoundedByLimit_Allowed) {
    auto result = detector_.validateUnboundedForLoops(
        "FOR doc IN users LIMIT 100 RETURN doc"
    );
    EXPECT_TRUE(result.is_safe)
        << "FOR loop bounded by LIMIT must be allowed";
}

TEST_F(InjectionAttackVectorTest, UnboundedLoop_WithOffsetLimit_Allowed) {
    auto result = detector_.validateUnboundedForLoops(
        "FOR doc IN orders FILTER doc.status == 'open' LIMIT 0, 50 RETURN doc"
    );
    EXPECT_TRUE(result.is_safe)
        << "FOR loop with LIMIT offset,count must be allowed";
}

TEST_F(InjectionAttackVectorTest, UnboundedLoop_NoLimit_Rejected) {
    auto result = detector_.validateUnboundedForLoops(
        "FOR doc IN users RETURN doc"
    );
    EXPECT_FALSE(result.is_safe)
        << "Unbounded FOR loop without LIMIT must be rejected";
}

TEST_F(InjectionAttackVectorTest, UnboundedLoop_WithFilterButNoLimit_Rejected) {
    auto result = detector_.validateUnboundedForLoops(
        "FOR doc IN logs FILTER doc.level == 'error' RETURN doc"
    );
    EXPECT_FALSE(result.is_safe)
        << "FOR loop with FILTER but no LIMIT must be rejected";
}

TEST_F(InjectionAttackVectorTest, UnboundedLoop_CollectAggregate_Allowed) {
    // COLLECT queries are exempt from the LIMIT requirement because the result
    // set is bounded by the number of distinct grouping-key values, not by
    // the total collection size.  In the worst case a high-cardinality key
    // (e.g., a UUID per document) still produces at most N groups for N input
    // rows — the same order as a LIMIT N would achieve — so the exemption is
    // considered an acceptable trade-off to avoid blocking legitimate analytics
    // queries.
    //
    // Memory note: O(N) memory is required for N distinct groups, which can
    // be significant on large collections.  Callers that need a hard upper
    // bound on memory usage should add a LIMIT clause themselves, even for
    // COLLECT queries.
    auto result = detector_.validateUnboundedForLoops(
        "FOR doc IN events COLLECT status = doc.status WITH COUNT INTO cnt RETURN {status, cnt}"
    );
    EXPECT_TRUE(result.is_safe)
        << "COLLECT aggregate query without LIMIT must be allowed (bounded by group count)";
}
