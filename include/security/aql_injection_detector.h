/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_injection_detector.h                           ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <regex>
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
};

} // namespace security
} // namespace themis
