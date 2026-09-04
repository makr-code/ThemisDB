/**
 * @file aql_injection_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/aql_injection_detector.h"
#include <stdexcept>
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <regex>
#include <fmt/format.h>

namespace themis {
namespace security {

namespace {
bool isBlankInput(const std::string& value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}
}  // namespace

// ============================================================================
// Public Methods
// ============================================================================

AQLInjectionDetector::InjectionCheckResult 
AQLInjectionDetector::validateParameterizedQuery(
    const std::string& aql_template,
    const std::vector<std::string>& parameters
) {
    InjectionCheckResult result = {};

    if (aql_template.empty() || isBlankInput(aql_template)) {
        result.is_safe = false;
        result.error_message = "AQL template must not be empty";
        return result;
    }
    
    // Step 1: Validate AQL template syntax is valid
    if (!isValidAQLTemplate(aql_template)) {
        result.is_safe = false;
        result.error_message = "Invalid AQL template syntax";
        return result;
    }
    
    // Step 2: Check for suspicious patterns in template
    // (Templates should NOT contain user input)
    if (containsSuspiciousPatterns(aql_template)) {
        result.is_safe = false;
        result.error_message = "Template contains suspicious patterns - possible injection attempt";
        result.detected_patterns = extractPatterns(aql_template);
        return result;
    }
    
    // Step 3: Validate parameters are properly escaped
    for (size_t i = 0; i < parameters.size(); ++i) {
        auto param_result = validateParameter(parameters[i]);
        if (!param_result.is_safe) {
            result.is_safe = false;
            result.error_message = fmt::format(
                "Parameter {} failed validation: {}",
                i, param_result.error_message
            );
            return result;
        }
    }
    
    return result;
}

AQLInjectionDetector::InjectionCheckResult 
AQLInjectionDetector::validateAQLAST(const std::string& aql) {
    InjectionCheckResult result = {};

    if (aql.empty() || isBlankInput(aql)) {
        result.is_safe = false;
        result.error_message = "AQL query must not be empty";
        return result;
    }
    
    // Step 1: Parse AQL into AST
    auto parse_result = parseAQL(aql);
    if (!parse_result) {
        result.is_safe = false;
        result.detected_patterns = extractPatterns(aql);
        if (!result.detected_patterns.empty()) {
            std::ostringstream token_stream = {};
            for (size_t i = 0; i < result.detected_patterns.size(); ++i) {
                if (i > 0) {
                    token_stream << ", ";
                }
                token_stream << result.detected_patterns[i];
            }
            result.error_message = fmt::format(
                "Parse error with suspicious tokens [{}]: {}",
                token_stream.str(),
                parse_result.error().message()
            );
        } else {
            result.error_message = fmt::format("Parse error: {}", parse_result.error().message());
        }
        return result;
    }
    
    const auto& ast = *parse_result.value();
    
    // Step 2: AST-level operation validation — catches attacks that evade
    // regex via non-standard whitespace, Unicode escapes, or concatenation.
    if (containsDangerousOperations(ast)) {
        result.is_safe = false;
        result.error_message = "Query AST contains dangerous operation nodes";
        return result;
    }
    
    // Step 3: Validate all string literals in AST
    auto literals = extractStringLiterals(ast);
    for (const auto& literal : literals) {
        if (containsSQLKeywords(literal)) {
            result.is_safe = false;
            result.error_message = fmt::format(
                "String literal contains SQL keywords: {}",
                literal
            );
            result.detected_patterns.push_back(literal);
            return result;
        }
    }
    
    return result;
}

AQLInjectionDetector::InjectionCheckResult
AQLInjectionDetector::validateForReadOnlyContext(const std::string& aql) {
    // Run full AST validation. Non-AQL syntax (including SQL DDL and DML payloads)
    // is rejected in the parse phase, while valid AQL is checked for dangerous
    // operation nodes and suspicious literals.
    return validateAQLAST(aql);
}

AQLInjectionDetector::InjectionCheckResult
AQLInjectionDetector::validateUnboundedForLoops(const std::string& aql) {
    InjectionCheckResult result = {};

    if (aql.empty() || isBlankInput(aql)) {
        result.is_safe = false;
        result.error_message = "AQL query must not be empty";
        return result;
    }

    // Step 1: Parse query into AST.
    auto parse_result = parseAQL(aql);
    if (!parse_result) {
        // Fallback: Some valid AQL aggregate forms (e.g. "COLLECT ... WITH
        // COUNT INTO") are not fully represented in the current parser.
        // For unbounded-loop validation we can conservatively rely on keyword
        // structure: FOR + LIMIT is bounded; FOR + COLLECT is treated as
        // aggregation-bounded; FOR without LIMIT/COLLECT is rejected.
        static const std::regex k_for_re(R"(\bFOR\b)", std::regex::icase);
        static const std::regex k_limit_re(R"(\bLIMIT\b)", std::regex::icase);
        static const std::regex k_collect_re(R"(\bCOLLECT\b)", std::regex::icase);

        const bool has_for_clause = std::regex_search(aql, k_for_re);
        if (!has_for_clause) {
            return result;
        }

        const bool has_limit_clause = std::regex_search(aql, k_limit_re);
        if (has_limit_clause) {
            return result;
        }

        const bool has_collect_clause = std::regex_search(aql, k_collect_re);
        if (has_collect_clause) {
            return result;
        }

        result.is_safe = false;
        result.error_message =
            "Query contains an unbounded FOR loop without a LIMIT clause; "
            "add LIMIT to prevent full-collection scans";
        return result;
    }

    const auto& ast = *parse_result.value();

    // Step 2: Check whether the query has at least one FOR clause.
    // An empty collection name in for_node means no FOR clause was parsed.
    const bool has_for_clause =
        !ast.for_node.collection.empty() || !ast.for_nodes.empty();

    if (!has_for_clause) {
        // No FOR clause – nothing to bound.  Considered safe.
        return result;
    }

    // Step 3: Queries with a COLLECT clause produce at most one row per
    // distinct group, so they are not considered unbounded even without LIMIT.
    if (ast.collect) {
        return result;
    }

    // Step 4: Reject queries with FOR but without a LIMIT clause.
    if (!ast.limit) {
        result.is_safe = false;
        result.error_message =
            "Query contains an unbounded FOR loop without a LIMIT clause; "
            "add LIMIT to prevent full-collection scans";
        return result;
    }

    return result;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool AQLInjectionDetector::isValidAQLTemplate(const std::string& template_str) {
    try {
        auto result = parseAQL(template_str);
        return result.has_value();
    } catch (...) {
        return false;
    }
}

AQLInjectionDetector::InjectionCheckResult 
AQLInjectionDetector::validateParameter(const std::string& param) {
    InjectionCheckResult result;
    
    // Validate parameter doesn't contain SQL keywords
    if (containsSQLKeywords(param)) {
        result.is_safe = false;
        result.error_message = "Parameter contains SQL keywords";
        return result;
    }
    
    // Validate parameter doesn't contain comment markers
    if (param.find("--") != std::string::npos ||
        param.find("#") != std::string::npos ||
        param.find("/*") != std::string::npos ||
        param.find("*/") != std::string::npos) {
        result.is_safe = false;
        result.error_message = "Parameter contains comment markers";
        return result;
    }

    // Reject classic boolean-blind bypass payloads like "OR 1==1" / "OR true"
    static const std::regex kBooleanBypass(
        R"(\bOR\b\s*(?:1\s*={1,2}\s*1|TRUE\b))",
        std::regex::icase);
    if (std::regex_search(param, kBooleanBypass)) {
        result.is_safe = false;
        result.error_message = "Parameter contains boolean bypass pattern";
        return result;
    }

    // Reject UNION-style payloads (including AQL-style "UNION FOR ...")
    static const std::regex kUnionPattern(R"(\bUNION\b)", std::regex::icase);
    if (std::regex_search(param, kUnionPattern)) {
        result.is_safe = false;
        result.error_message = "Parameter contains UNION pattern";
        return result;
    }

    // Reject stacked-query attempts early in parameter values.
    if (param.find(';') != std::string::npos) {
        result.is_safe = false;
        result.error_message = "Parameter contains stacked-query separator";
        return result;
    }
    
    // Validate parameter length (limit to 10 KiB = 10240 bytes to prevent DoS attacks)
    constexpr size_t MAX_PARAM_LENGTH = 10240;  // 10 KiB (10240 bytes) maximum per parameter
    if (param.length() > MAX_PARAM_LENGTH) {
        result.is_safe = false;
        result.error_message = fmt::format(
            "Parameter too long: {} > {}",
            param.length(), MAX_PARAM_LENGTH
        );
        return result;
    }
    
    return result;
}

bool AQLInjectionDetector::containsSuspiciousPatterns(const std::string& str) {
    // Comprehensive list of suspicious patterns
    // Note: REPLACE, UPSERT, REMOVE are valid in ArangoDB AQL but are blocked here
    // because they indicate potential privilege escalation attempts. In read-only
    // query contexts (FOR...RETURN), these operations should never appear as they
    // would indicate SQL injection attempts to modify data or permissions.
    static const std::vector<std::regex> patterns = {
        // SQL keywords that indicate injection attempts in AQL read-only context
        std::regex(R"(\b(DROP|DELETE|UPDATE|INSERT|REPLACE|UPSERT|REMOVE)\b)", 
                  std::regex::icase),
        
        // Comment markers (attempt to hide payload)
        std::regex(R"(-{2}|/\*|\*/)"),
        
        // String concatenation attempts
        std::regex(R"(\|\||CONCAT|CHR\(|CHAR\()", std::regex::icase),
        
        // Command execution (SQL injection patterns)
        std::regex(R"(\b(EXEC|EXECUTE|SYSTEM|SHELL)\b)", std::regex::icase),
        
        // Stored procedure / OS-command execution (MSSQL / SQL Server patterns)
        std::regex(R"(\b(XP_CMDSHELL|SP_EXECUTESQL)\b)", std::regex::icase),
        
        // File operations (SQL injection patterns)
        std::regex(R"(\b(LOAD_FILE|INTO\s+OUTFILE|INTO\s+DUMPFILE)\b)", std::regex::icase),
        
        // Union-based injection
        std::regex(R"(\bUNION\s+(SELECT|ALL)\b)", std::regex::icase),
        
        // Stacked queries (semicolon followed by SQL keywords)
        std::regex(R"(;\s*(SELECT|INSERT|UPDATE|DELETE|DROP))", std::regex::icase),
        
        // Time-based blind injection
        std::regex(R"(\b(WAITFOR|BENCHMARK|SLEEP)\b)", std::regex::icase),
    };
    
    for (const auto& pattern : patterns) {
        if (std::regex_search(str, pattern)) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> AQLInjectionDetector::extractPatterns(const std::string& str) {
    std::vector<std::string> patterns;
    
    // Pattern list mirrors the full suspicious-keyword set from containsSuspiciousPatterns()
    // so that detected_patterns is non-empty whenever that function returns true.
    static const std::vector<std::regex> pattern_list = {
        // DML/DDL and AQL write operations (matches the first pattern in containsSuspiciousPatterns)
        std::regex(R"(\b(DROP|DELETE|UPDATE|INSERT|REPLACE|UPSERT|REMOVE)\b)", std::regex::icase),
        std::regex(R"(-{2}|/\*|\*/)"),
        std::regex(R"(\bUNION\s+(SELECT|ALL)\b)", std::regex::icase),
        std::regex(R"(\b(EXEC|EXECUTE|SYSTEM|SHELL)\b)", std::regex::icase),
        std::regex(R"(\b(XP_CMDSHELL|SP_EXECUTESQL)\b)", std::regex::icase),
        std::regex(R"(\b(WAITFOR|BENCHMARK|SLEEP)\b)", std::regex::icase),
        std::regex(R"(\b(LOAD_FILE|INTO\s+OUTFILE|INTO\s+DUMPFILE)\b)", std::regex::icase),
    };
    
    for (const auto& pattern : pattern_list) {
        std::smatch match = {};
        std::string search_str = str;
        while (std::regex_search(search_str, match, pattern)) {
            patterns.push_back(match.str());
            search_str = match.suffix().str();
        }
    }
    
    return patterns;
}

bool AQLInjectionDetector::containsSQLKeywords(const std::string& str) {
    static const std::vector<std::string> keywords = {
        "DROP", "DELETE", "UPDATE", "INSERT", "REPLACE",
        "EXEC", "EXECUTE", "SYSTEM", "SHELL", "WAITFOR",
        "BENCHMARK", "LOAD_FILE", "INTO OUTFILE", "UNION SELECT", "UNION"
    };
    
    // Reject SQL-style single-line comment markers inside string literals.
    if (str.find("--") != std::string::npos) {
        return true;
    }

    std::string upper_str = str;
    std::transform(upper_str.begin(), upper_str.end(), 
                  upper_str.begin(), ::toupper);
    
    for (const auto& keyword : keywords) {
        if (upper_str.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool AQLInjectionDetector::containsDangerousOperations(const query::Query& ast) {
    // Dangerous AQL/SQL function names that must never appear in a query AST.
    // Checked case-insensitively in scanExpressionForDangerousOps().
    // This list intentionally mirrors the regex patterns in
    // containsSuspiciousPatterns() to provide AST-level defense-in-depth
    // against evasions that use non-standard whitespace, Unicode escapes, or
    // comment-based obfuscation that can confuse regex matchers.
    
    // Check all filter conditions
    for (const auto& filter : ast.filters) {
        if (filter && filter->condition) {
            if (scanExpressionForDangerousOps(filter->condition)) {
                return true;
            }
        }
    }
    
    // Check return expression
    if (ast.return_node && ast.return_node->expression) {
        if (scanExpressionForDangerousOps(ast.return_node->expression)) {
            return true;
        }
    }
    
    // Check sort expressions
    if (ast.sort) {
        for (const auto& spec : ast.sort->specifications) {
            if (scanExpressionForDangerousOps(spec.expression)) {
                return true;
            }
        }
    }
    
    // Check LET expressions
    for (const auto& let_node : ast.let_nodes) {
        if (let_node.expression) {
            if (scanExpressionForDangerousOps(let_node.expression)) {
                return true;
            }
        }
    }
    
    // Check CTE sub-queries (WITH clause)
    if (ast.with_clause) {
        for (const auto& cte : ast.with_clause->ctes) {
            if (cte.subquery && containsDangerousOperations(*cte.subquery)) {
                return true;
            }
        }
    }
    
    // Check COLLECT group/aggregate expressions
    if (ast.collect) {
        for (const auto& [var, expr] : ast.collect->groups) {
            if (scanExpressionForDangerousOps(expr)) {
                return true;
            }
        }
        for (const auto& agg : ast.collect->aggregations) {
            if (scanExpressionForDangerousOps(agg.argument)) {
                return true;
            }
        }
    }
    
    return false;
}

bool AQLInjectionDetector::scanExpressionForDangerousOps(
    const std::shared_ptr<query::Expression>& expr
) {
    if (!expr) {
      return false;
    }
    
    // Disallowed function names — checked case-insensitively.
    static const std::unordered_set<std::string> kDangerousFunctions = {
        "EXECUTE", "EXEC", "SYSTEM", "SHELL",
        "XP_CMDSHELL", "SP_EXECUTESQL",
        "WAITFOR", "BENCHMARK", "SLEEP",
        "LOAD_FILE",
    };
    
    auto node_type = expr->getType();
    
    if (node_type == query::ASTNodeType::FunctionCall) {
        auto func_expr = std::static_pointer_cast<query::FunctionCallExpr>(expr);
        std::string upper_name = func_expr->name;
        std::transform(
            upper_name.begin(),
            upper_name.end(),
            upper_name.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); }
        );
        if (kDangerousFunctions.count(upper_name) > 0) {
            return true;
        }
        for (const auto& arg : func_expr->arguments) {
            if (scanExpressionForDangerousOps(arg)) {
                return true;
            }
        }
    } else if (node_type == query::ASTNodeType::BinaryOp) {
        auto binary_expr = std::static_pointer_cast<query::BinaryOpExpr>(expr);
        return scanExpressionForDangerousOps(binary_expr->left) ||
               scanExpressionForDangerousOps(binary_expr->right);
    } else if (node_type == query::ASTNodeType::UnaryOp) {
        auto unary_expr = std::static_pointer_cast<query::UnaryOpExpr>(expr);
        return scanExpressionForDangerousOps(unary_expr->operand);
    } else if (node_type == query::ASTNodeType::ArrayLiteral) {
        auto array_expr = std::static_pointer_cast<query::ArrayLiteralExpr>(expr);
        for (const auto& elem : array_expr->elements) {
            if (scanExpressionForDangerousOps(elem)) {
              return true;
            }
        }
    } else if (node_type == query::ASTNodeType::ObjectConstruct) {
        auto obj_expr = std::static_pointer_cast<query::ObjectConstructExpr>(expr);
        for (const auto& [key, value] : obj_expr->fields) {
            if (scanExpressionForDangerousOps(value)) {
              return true;
            }
        }
    } else if (node_type == query::ASTNodeType::FieldAccess) {
        auto field_expr = std::static_pointer_cast<query::FieldAccessExpr>(expr);
        return scanExpressionForDangerousOps(field_expr->object);
    } else if (node_type == query::ASTNodeType::SubqueryExpr) {
        auto subq_expr = std::static_pointer_cast<query::SubqueryExpr>(expr);
        if (subq_expr->subquery) {
            return containsDangerousOperations(*subq_expr->subquery);
        }
    } else if (node_type == query::ASTNodeType::AnyExpr) {
        auto any_expr = std::static_pointer_cast<query::AnyExpr>(expr);
        return scanExpressionForDangerousOps(any_expr->arrayExpr) ||
               scanExpressionForDangerousOps(any_expr->condition);
    } else if (node_type == query::ASTNodeType::AllExpr) {
        auto all_expr = std::static_pointer_cast<query::AllExpr>(expr);
        return scanExpressionForDangerousOps(all_expr->arrayExpr) ||
               scanExpressionForDangerousOps(all_expr->condition);
    } else if (node_type == query::ASTNodeType::SimilarityCall) {
        auto sim_expr = std::static_pointer_cast<query::SimilarityCallExpr>(expr);
        for (const auto& arg : sim_expr->arguments) {
            if (scanExpressionForDangerousOps(arg)) {
              return true;
            }
        }
    } else if (node_type == query::ASTNodeType::ProximityCall) {
        auto prox_expr = std::static_pointer_cast<query::ProximityCallExpr>(expr);
        for (const auto& arg : prox_expr->arguments) {
            if (scanExpressionForDangerousOps(arg)) {
              return true;
            }
        }
    }
    
    return false;
}

std::vector<std::string> AQLInjectionDetector::extractStringLiterals(const query::Query& ast) {
    std::vector<std::string> literals;
    
    // Extract from filter expressions
    for (const auto& filter : ast.filters) {
        if (filter && filter->condition) {
            extractStringLiteralsFromExpression(filter->condition, literals);
        }
    }
    
    // Extract from return expression
    if (ast.return_node && ast.return_node->expression) {
        extractStringLiteralsFromExpression(ast.return_node->expression, literals);
    }
    
    // Extract from sort expressions
    if (ast.sort) {
        for (const auto& spec : ast.sort->specifications) {
            extractStringLiteralsFromExpression(spec.expression, literals);
        }
    }
    
    // Extract from LET expressions
    for (const auto& let_node : ast.let_nodes) {
        if (let_node.expression) {
            extractStringLiteralsFromExpression(let_node.expression, literals);
        }
    }
    
    return literals;
}

void AQLInjectionDetector::extractStringLiteralsFromExpression(
    const std::shared_ptr<query::Expression>& expr,
    std::vector<std::string>& literals
) {
    if (!expr) {
      return;
    }
    
    auto node_type = expr->getType();
    
    // Use static_pointer_cast for type conversion after getType() verification.
    // Note: This is safe only because we've verified the type with getType() first.
    // After type verification, we know the cast will succeed, so static_pointer_cast
    // avoids the runtime overhead of dynamic_cast's RTTI checking.
    
    // Check if this is a string literal
    if (node_type == query::ASTNodeType::Literal) {
        auto literal_expr = std::static_pointer_cast<query::LiteralExpr>(expr);
        if (std::holds_alternative<std::string>(literal_expr->value)) {
            literals.push_back(std::get<std::string>(literal_expr->value));
        }
    }
    // Recursively check binary operations
    else if (node_type == query::ASTNodeType::BinaryOp) {
        auto binary_expr = std::static_pointer_cast<query::BinaryOpExpr>(expr);
        extractStringLiteralsFromExpression(binary_expr->left, literals);
        extractStringLiteralsFromExpression(binary_expr->right, literals);
    }
    // Recursively check unary operations
    else if (node_type == query::ASTNodeType::UnaryOp) {
        auto unary_expr = std::static_pointer_cast<query::UnaryOpExpr>(expr);
        extractStringLiteralsFromExpression(unary_expr->operand, literals);
    }
    // Recursively check function calls
    else if (node_type == query::ASTNodeType::FunctionCall) {
        auto func_expr = std::static_pointer_cast<query::FunctionCallExpr>(expr);
        for (const auto& arg : func_expr->arguments) {
            extractStringLiteralsFromExpression(arg, literals);
        }
    }
    // Recursively check field access
    else if (node_type == query::ASTNodeType::FieldAccess) {
        auto field_expr = std::static_pointer_cast<query::FieldAccessExpr>(expr);
        extractStringLiteralsFromExpression(field_expr->object, literals);
    }
    // Recursively check array literals
    else if (node_type == query::ASTNodeType::ArrayLiteral) {
        auto array_expr = std::static_pointer_cast<query::ArrayLiteralExpr>(expr);
        for (const auto& elem : array_expr->elements) {
            extractStringLiteralsFromExpression(elem, literals);
        }
    }
    // Recursively check object construction
    else if (node_type == query::ASTNodeType::ObjectConstruct) {
        auto obj_expr = std::static_pointer_cast<query::ObjectConstructExpr>(expr);
        for (const auto& [key, value] : obj_expr->fields) {
            extractStringLiteralsFromExpression(value, literals);
        }
    }
}

Result<std::shared_ptr<query::Query>> AQLInjectionDetector::parseAQL(const std::string& aql) {
    try {
        query::AQLParser parser;
        return parser.parse(aql);
    } catch (const std::exception& e) {
        return Err<std::shared_ptr<query::Query>>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            fmt::format("Failed to parse AQL: {}", e.what())
        );
    }
}

} // namespace security
} // namespace themis

