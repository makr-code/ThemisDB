/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_injection_detector.h                           ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:28:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     260                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 4e39463a86  2026-03-21  feat(security): implement AQL read-only context validatio... ║
    • eb75d79f53  2026-03-12  feat(security): implement AQL AST-level injection validat... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <unordered_set>
#include "query/aql_parser.h"
#include "utils/expected.h"

namespace themis {
namespace security {

/**
 * @brief AQL Injection Detection using AST-based validation
 * 
 * This class provides robust protection against AQL/SQL injection attacks by:
 * 1. Parsing queries into Abstract Syntax Trees (AST) for structural validation
 * 2. Validating parameterized queries with strict parameter checking
 * 3. Detecting suspicious patterns that indicate injection attempts
 * 4. Blocking dangerous operations (UPDATE, DELETE, INSERT, DROP, etc.)
 * 
 * Security Features:
 * - AST-based parsing ensures query structure is valid
 * - Case-insensitive keyword detection prevents bypasses
 * - Comment marker detection blocks comment-based attacks
 * - String literal validation within AST nodes
 * - Parameterized query enforcement with parameter validation
 * 
 * CWE Coverage: CWE-89 (SQL Injection), CWE-94 (Code Injection)
 */
class AQLInjectionDetector {
public:
    /**
     * @brief Result of an injection check operation
     */
    struct InjectionCheckResult {
        bool is_safe = true;                          // True if query is safe to execute
        std::string error_message;                     // Error message if unsafe
        std::vector<std::string> detected_patterns;    // Patterns detected during validation
        
        // Implicit conversion to bool for convenient checking
        explicit operator bool() const { return is_safe; }
    };
    
    /**
     * @brief Validate a parameterized AQL query with bound parameters
     * 
     * This is the RECOMMENDED way to execute AQL queries. Parameterized queries
     * separate query structure from user data, preventing injection attacks.
     * 
     * @param aql_template The AQL query template with parameter placeholders
     * @param parameters Vector of parameter values to bind
     * @return InjectionCheckResult indicating if the query is safe
     * 
     * Example:
     *   auto result = detector.validateParameterizedQuery(
     *       "FOR doc IN users FILTER doc.name == @param0 RETURN doc",
     *       {"Alice"}
     *   );
     *   if (result.is_safe) {
     *       // Execute with bound parameters
     *   }
     */
    InjectionCheckResult validateParameterizedQuery(
        const std::string& aql_template,
        const std::vector<std::string>& parameters
    );
    
    /**
     * @brief Validate an AQL query using AST-based analysis
     * 
     * This method parses the AQL query into an AST and validates:
     * - Query structure is valid and parseable
     * - No dangerous operations (UPDATE, DELETE, INSERT, DROP)
     * - String literals don't contain SQL keywords
     * - No comment markers or suspicious patterns
     * 
     * Use this for queries that cannot be parameterized, but prefer
     * validateParameterizedQuery() when possible.
     * 
     * @param aql The AQL query string to validate
     * @return InjectionCheckResult indicating if the query is safe
     * 
     * Example:
     *   auto result = detector.validateAQLAST(
     *       "FOR doc IN users FILTER doc.age > 18 RETURN doc"
     *   );
     */
    InjectionCheckResult validateAQLAST(const std::string& aql);

    /**
     * @brief Validate an AQL query for use in a read-only context
     *
     * Extends the base AST validation with an additional layer that explicitly
     * rejects any DDL or write operations.  This is useful when a query is
     * executed against a read-only replica or inside a read-only transaction
     * where write operations would either be silently ignored or cause an error
     * at a later stage.
     *
     * Detected write / DDL patterns (regex + AST):
     *   - AQL write clauses: INSERT, UPDATE, REPLACE, UPSERT, REMOVE
     *   - SQL-style DDL: CREATE COLLECTION, DROP COLLECTION, CREATE INDEX
     *   - SQL-style DML: DELETE FROM, INSERT INTO, UPDATE SET
     *
     * @param aql The AQL query string to validate.
     * @return InjectionCheckResult with is_safe == false if any write/DDL
     *         operation is detected.
     *
     * Example:
     *   auto r = detector.validateForReadOnlyContext(
     *       "FOR doc IN users FILTER doc.age > 18 RETURN doc");
     *   // r.is_safe == true – pure read query is allowed
     *
     *   auto r2 = detector.validateForReadOnlyContext(
     *       "INSERT {name:'evil'} INTO users");
     *   // r2.is_safe == false – write operation rejected
     */
    InjectionCheckResult validateForReadOnlyContext(const std::string& aql);

    /**
     * @brief Validate that an AQL query does not contain unbounded FOR loops
     *
     * An unbounded FOR loop iterates over an entire collection without a LIMIT
     * clause and can cause full-collection scans leading to DoS or data
     * exfiltration.  This method rejects queries that contain at least one FOR
     * clause but no top-level LIMIT clause.
     *
     * Exceptions (always allowed):
     *   - Queries without any FOR clause (constant expressions, etc.)
     *   - Queries that aggregate with COLLECT (result is bounded by distinct
     *     group count, not collection size)
     *
     * @param aql The AQL query string to validate.
     * @return InjectionCheckResult with is_safe == false when the query has a
     *         FOR loop but no LIMIT clause.
     *
     * Example:
     *   auto r = detector.validateUnboundedForLoops(
     *       "FOR doc IN users LIMIT 100 RETURN doc");
     *   // r.is_safe == true – bounded by LIMIT
     *
     *   auto r2 = detector.validateUnboundedForLoops(
     *       "FOR doc IN users RETURN doc");
     *   // r2.is_safe == false – unbounded FOR loop rejected
     */
    InjectionCheckResult validateUnboundedForLoops(const std::string& aql);

private:
    // ============================================================================
    // Helper Functions
    // ============================================================================
    
    /**
     * @brief Check if the AQL template has valid syntax
     */
    bool isValidAQLTemplate(const std::string& template_str);
    
    /**
     * @brief Validate a single parameter value
     */
    InjectionCheckResult validateParameter(const std::string& param);
    
    /**
     * @brief Check if string contains suspicious injection patterns
     */
    bool containsSuspiciousPatterns(const std::string& str);
    
    /**
     * @brief Extract detected patterns from string
     */
    std::vector<std::string> extractPatterns(const std::string& str);
    
    /**
     * @brief Check if string contains SQL/AQL keywords (case-insensitive)
     */
    bool containsSQLKeywords(const std::string& str);
    
    /**
     * @brief Check if AST contains dangerous operations
     * 
     * Recursively traverse the AST checking for:
     * - DELETE, UPDATE, INSERT, REPLACE, UPSERT, REMOVE operations
     * - DROP, EXEC, SYSTEM calls (not valid in AQL but check for SQL injection)
     */
    bool containsDangerousOperations(const query::Query& ast);
    
    /**
     * @brief Extract all string literals from AST
     * 
     * Recursively traverse AST and collect all string literal values
     */
    std::vector<std::string> extractStringLiterals(const query::Query& ast);
    
    /**
     * @brief Extract string literals from an expression
     */
    void extractStringLiteralsFromExpression(
        const std::shared_ptr<query::Expression>& expr,
        std::vector<std::string>& literals
    );
    
    /**
     * @brief Parse AQL query into AST
     */
    Result<std::shared_ptr<query::Query>> parseAQL(const std::string& aql);

    /**
     * @brief Recursively scan an expression node for dangerous operations
     *
     * Traverses every node in the expression tree and returns true if any
     * FunctionCallExpr has a name that belongs to the disallowed-operations
     * list (EXECUTE, EXEC, SYSTEM, SHELL, etc.).  Sub-queries embedded inside
     * ANY/ALL/SubqueryExpr are delegated back to containsDangerousOperations().
     */
    bool scanExpressionForDangerousOps(const std::shared_ptr<query::Expression>& expr);

    /**
     * @brief Check whether a raw query string contains write or DDL operations
     *
     * Used by validateForReadOnlyContext() to provide a regex-level defence in
     * addition to AST analysis.  Detects AQL write clauses (INSERT, UPDATE,
     * REPLACE, UPSERT, REMOVE) and SQL-style DDL (CREATE/DROP COLLECTION,
     * CREATE INDEX, DELETE FROM, INSERT INTO).
     *
     * @param aql         Query string to scan.
     * @param matched_out If non-null and a match is found, receives the first
     *                    matched keyword (e.g. "INSERT", "DROP COLLECTION").
     * @return true if a write or DDL operation was detected.
     */
    bool containsWriteOrDDLOperations(const std::string& aql,
                                      std::string* matched_out = nullptr);
};

} // namespace security
} // namespace themis
