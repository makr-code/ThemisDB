#include "security/aql_injection_detector.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <fmt/format.h>

namespace themis {
namespace security {

// ============================================================================
// Public Methods
// ============================================================================

AQLInjectionDetector::InjectionCheckResult 
AQLInjectionDetector::validateParameterizedQuery(
    const std::string& aql_template,
    const std::vector<std::string>& parameters
) {
    InjectionCheckResult result;
    
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
    InjectionCheckResult result;
    
    // Step 1: Parse AQL into AST
    auto parse_result = parseAQL(aql);
    if (!parse_result) {
        result.is_safe = false;
        result.error_message = fmt::format("Parse error: {}", parse_result.error().message());
        return result;
    }
    
    const auto& ast = *parse_result.value();
    
    // Step 2: Check for suspicious patterns in the original query string
    // This catches injection attempts before they can be parsed
    if (containsSuspiciousPatterns(aql)) {
        result.is_safe = false;
        result.error_message = "Query contains suspicious patterns";
        result.detected_patterns = extractPatterns(aql);
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
        param.find("/*") != std::string::npos ||
        param.find("*/") != std::string::npos) {
        result.is_safe = false;
        result.error_message = "Parameter contains comment markers";
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
    
    static const std::vector<std::regex> pattern_list = {
        std::regex(R"(\b(DROP|DELETE|UPDATE|INSERT)\b)", std::regex::icase),
        std::regex(R"(-{2}|/\*|\*/)"),
        std::regex(R"(\bUNION\s+SELECT\b)", std::regex::icase),
    };
    
    for (const auto& pattern : pattern_list) {
        std::smatch match;
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
        "BENCHMARK", "LOAD_FILE", "INTO OUTFILE", "UNION SELECT"
    };
    
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
    // TODO (v1.4.0): Implement AST-level operation validation
    // 
    // Note: Standard AQL (ArangoDB-style) uses FOR...RETURN for read-only queries.
    // Write operations (UPDATE, DELETE, INSERT, etc.) use separate AQL syntax
    // and are not valid within FOR...RETURN query context.
    // 
    // Dangerous operations are currently detected via:
    // 1. containsSuspiciousPatterns() - regex-based pattern detection
    // 2. containsSQLKeywords() - keyword detection in string literals
    // 
    // This function returns false as pattern-based detection is more reliable
    // for the standard AQL dialect. When dialect support expands, implement
    // recursive AST traversal here to check for dangerous operation nodes.
    
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
    if (!expr) return;
    
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
