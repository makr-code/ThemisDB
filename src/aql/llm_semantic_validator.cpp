/**
 * @file llm_semantic_validator.cpp
 * @brief Implementation of semantic validation for LLM-generated AQL queries
 * @version 0.1.0
 */

#include "aql/llm_semantic_validator.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <fmt/format.h>
#include "query/aql_parser.h"

namespace themis {
namespace aql {

LLMSemanticValidator::LLMSemanticValidator(
    std::shared_ptr<SemanticSchemaContext> schema_context,
    const Config& config)
    : schema_context_(std::move(schema_context)), config_(config)
{
    logger_ = spdlog::get("aql");
    if (!logger_) {
        logger_ = spdlog::stdout_color_mt("aql");
    }
}

SemanticValidationResult LLMSemanticValidator::validate(const query::ASTNode* ast)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    SemanticValidationResult result = {};

    if (!ast) {
        result.status = SemanticValidationResult::Status::UNKNOWN_ERROR;
        result.error_message = "AST is null";
        spdlog::warn("[aql_semantic_validator] Null AST provided");
        return result;
    }

    try {
        // Type checking
        if (config_.enable_type_checking) {
            checkAttributeTypes(ast, result);
            if (!result.isValid()) {
                goto compute_score;
            }
        }

        // Join validation
        if (config_.enable_join_validation) {
            validateJoins(ast, result);
            if (!result.isValid()) {
                goto compute_score;
            }
        }

        // Cardinality estimation
        if (config_.enable_cardinality_estimation) {
            estimateCardinality(ast, result);
        }

        // Function signature validation
        if (config_.enable_function_validation) {
            validateFunctionSignatures(ast, result);
        }

    compute_score:
        computeConfidenceScore(result);

        // Record validation latency
        auto end_time = std::chrono::high_resolution_clock::now();
        result.validation_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        spdlog::debug(
            "[aql_semantic_validator] Validation complete: status={}, confidence={:.2f}, latency_ms={}",
            static_cast<int>(result.status),
            result.confidence_score,
            result.validation_latency_ms.count());

        return result;

    } catch (const std::exception& e) {
        result.status = SemanticValidationResult::Status::UNKNOWN_ERROR;
        result.error_message = fmt::format("Semantic validation exception: {}", e.what());
        spdlog::error("[aql_semantic_validator] Exception during validation: {}", e.what());
        return result;
    }
}

void LLMSemanticValidator::configure(const Config& config)
{
    config_ = config;
    spdlog::info("[aql_semantic_validator] Configuration updated");
}

const LLMSemanticValidator::Config& LLMSemanticValidator::getConfig() const
{
    return config_;
}

/**
 * @brief Helper: Extract variable bindings from FOR and LET clauses
 * @param ast Query AST
 * @return Map from variable name to collection (for FOR) or inferred type
 */
static std::unordered_map<std::string, std::string> extractVariableBindings(
    const query::ASTNode* ast)
{
    std::unordered_map<std::string, std::string> bindings = {};

    if (!ast) {
      return bindings;
    }
    
    // Try to cast ast to Query*
    // Note: In the semantic validator context, ast should be a Query struct
    auto query_ptr = reinterpret_cast<const query::Query*>(ast);
    
    if (!query_ptr) {
      return bindings;
    }
    
    // Extract FOR bindings: FOR var IN collection
    for (const auto& for_node : query_ptr->for_nodes) {
        bindings[for_node.variable] = for_node.collection;
    }
    
    // Extract legacy FOR binding for backwards compatibility
    if (!query_ptr->for_node.variable.empty()) {
        bindings[query_ptr->for_node.variable] = query_ptr->for_node.collection;
    }
    
    return bindings;
}

/**
 * @brief Helper: Infer AQL type from an expression
 * @param expr Expression to analyze
 * @return Type string: "string", "number", "boolean", "array", "object", "unknown"
 */
static std::string inferTypeFromExpression(const std::shared_ptr<query::Expression>& expr)
{
    if (!expr) {
      return "unknown";
    }
    
    auto type = expr->getType();
    switch (type) {
        case query::ASTNodeType::Literal: {
            auto lit = std::dynamic_pointer_cast<query::LiteralExpr>(expr);
            if (lit) {
                if (std::holds_alternative<std::string>(lit->value)) {
                  return "string";
                }
                if (std::holds_alternative<bool>(lit->value)) {
                  return "boolean";
                }
                if (std::holds_alternative<int64_t>(lit->value) ||
                    std::holds_alternative<double>(lit->value)) return "number";
                if (std::holds_alternative<nlohmann::json>(lit->value)) {
                    auto& j = std::get<nlohmann::json>(lit->value);
                    if (j.is_array()) {
                      return "array";
                    }
                    if (j.is_object()) {
                      return "object";
                    }
                }
                return "unknown";
            }
            return "unknown";
        }
        case query::ASTNodeType::ArrayLiteral:
            return "array";
        case query::ASTNodeType::ObjectConstruct:
            return "object";
        case query::ASTNodeType::FunctionCall: {
            // Common functions return known types
            auto fc = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
            if (fc) {
                if (fc->name == "CONCAT" || fc->name == "LOWER" || fc->name == "UPPER" ||
                    fc->name == "TRIM" || fc->name == "SUBSTR") return "string";
                if (fc->name == "SUM" || fc->name == "AVG" || fc->name == "COUNT" ||
                    fc->name == "MIN" || fc->name == "MAX" || fc->name == "FLOOR" ||
                    fc->name == "CEIL" || fc->name == "ROUND") return "number";
                if (fc->name == "LENGTH" || fc->name == "SIZE") {
                  return "number";
                }
                // Default: unknown
            }
            return "unknown";
        }
        default:
            return "unknown";
    }
}

/**
 * @brief Helper: Check binary operator type compatibility
 * @param op Operator
 * @param left_type Type of left operand
 * @param right_type Type of right operand
 * @return Pair: (is_compatible, error_message)
 */
static std::pair<bool, std::string> checkBinaryOpTypeCompatibility(
    query::BinaryOperator op,
    const std::string& left_type,
    const std::string& right_type)
{
    using Op = query::BinaryOperator;
    
    // Comparison operators: most types comparable
    if (op == Op::Eq || op == Op::Neq || op == Op::Lt || op == Op::Lte ||
        op == Op::Gt || op == Op::Gte) {
        // Allow same-type comparisons
        if (left_type == right_type) return {true, ""};
        // Allow numeric comparisons between int/double
        if (((left_type == "number" || left_type == "unknown") &&
            (right_type == "number" || right_type == "unknown"))) return {true, ""};
        // Allow string/unknown comparisons
        if (((left_type == "string" || left_type == "unknown") &&
            (right_type == "string" || right_type == "unknown"))) return {true, ""};
        return {false, fmt::format("Comparison of {} with {}", left_type, right_type)};
    }
    
    // Logical operators require boolean-like values
    if (op == Op::And || op == Op::Or || op == Op::Xor) {
        // Accept boolean or unknown (conservative)
        return {true, ""};
    }
    
    // Arithmetic operators require numeric types
    if (op == Op::Add || op == Op::Sub || op == Op::Mul || op == Op::Div || op == Op::Mod) {
        if (((left_type == "number" || left_type == "unknown") &&
            (right_type == "number" || right_type == "unknown"))) return {true, ""};
        // ADD can also concatenate strings
        if (op == Op::Add && left_type == "string" && right_type == "string")
            return {true, ""};
        return {false, fmt::format("Arithmetic operation on {} and {}", left_type, right_type)};
    }
    
    // IN operator: left can be any, right must be array
    if (op == Op::In) {
        if (right_type == "array" || right_type == "unknown") return {true, ""};
        return {false, fmt::format("IN expects array on right side, got {}", right_type)};
    }
    
    return {true, ""};  // Default: allow
}

/**
 * @brief Walk expressions to find and validate attribute accesses
 * @param expr Expression to walk
 * @param result Output: validation issues
 * @param schema_context Optional schema for type info
 * @param variable_bindings FOR/LET variable map
 */
static void walkExpressionForTypeChecks(
    const std::shared_ptr<query::Expression>& expr,
    SemanticValidationResult& result,
    const std::shared_ptr<SemanticSchemaContext>& schema_context,
    const std::unordered_map<std::string, std::string>& variable_bindings)
{
    if (!expr) {
      return;
    }
    
    auto type = expr->getType();
    
    // Check FieldAccess expressions for type compatibility
    if (type == query::ASTNodeType::FieldAccess) {
        auto fa = std::dynamic_pointer_cast<query::FieldAccessExpr>(expr);
        if (fa && fa->object) {
            auto obj_type_str = inferTypeFromExpression(fa->object);
            
            // If accessing field on non-object, warn
            if (obj_type_str != "object" && obj_type_str != "unknown") {
                result.warnings.push_back(fmt::format(
                    "Field access on non-object type ({})", obj_type_str));
                result.confidence_score -= 0.15;
            }
            
            // Check with schema context if available
            if (schema_context && fa->object->getType() == query::ASTNodeType::Variable) {
                auto var_expr = std::dynamic_pointer_cast<query::VariableExpr>(fa->object);
                if (var_expr && variable_bindings.count(var_expr->name)) {
                    auto collection = variable_bindings.at(var_expr->name);
                    if (!schema_context->hasAttribute(collection, fa->field)) {
                        result.warnings.push_back(fmt::format(
                            "Field '{}.{}' not found in schema", var_expr->name, fa->field));
                        result.confidence_score -= 0.2;
                    }
                }
            }
        }
        // Recursively check nested expressions
        if (fa && fa->object) {
            walkExpressionForTypeChecks(fa->object, result, schema_context, variable_bindings);
        }
    }
    
    // Check BinaryOp expressions for type compatibility
    else if (type == query::ASTNodeType::BinaryOp) {
        auto bo = std::dynamic_pointer_cast<query::BinaryOpExpr>(expr);
        if (bo) {
            auto left_type = inferTypeFromExpression(bo->left);
            auto right_type = inferTypeFromExpression(bo->right);
            
            auto [compatible, error_msg] = checkBinaryOpTypeCompatibility(bo->op, left_type, right_type);
            if (!compatible) {
                result.issues.push_back({error_msg, SemanticValidationResult::Severity::ERROR});
                result.confidence_score -= 0.3;
            }
            
            // Recursively check operands
            walkExpressionForTypeChecks(bo->left, result, schema_context, variable_bindings);
            walkExpressionForTypeChecks(bo->right, result, schema_context, variable_bindings);
        }
    }
    
    // Check FunctionCall expressions
    else if (type == query::ASTNodeType::FunctionCall) {
        auto fc = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
        if (fc) {
            // Common function signature checks
            if (fc->name == "CONCAT" && !fc->arguments.empty()) {
                for (size_t i = 0; i < fc-> static_cast<int>(arguments.size()); ++i) {
                    auto arg_type = inferTypeFromExpression(fc->arguments[i]);
                    if (arg_type != "string" && arg_type != "unknown") {
                        result.warnings.push_back(fmt::format(
                            "CONCAT argument {} is {}, expected string", i, arg_type));
                        result.confidence_score -= 0.1;
                    }
                }
            }
            
            // Recursively check arguments
            for (const auto& arg : fc->arguments) {
                walkExpressionForTypeChecks(arg, result, schema_context, variable_bindings);
            }
        }
    }
    
    // Check UnaryOp expressions
    else if (type == query::ASTNodeType::UnaryOp) {
        auto uo = std::dynamic_pointer_cast<query::UnaryOpExpr>(expr);
        if (uo && uo->operand) {
            // NOT expects boolean-like; Minus/Plus expect numeric
            if (uo->op == query::UnaryOperator::Not) {
                // Boolean check: lenient
            } else {
                auto operand_type = inferTypeFromExpression(uo->operand);
                if (operand_type != "number" && operand_type != "unknown") {
                    result.warnings.push_back(fmt::format(
                        "Unary operator expects numeric, got {}", operand_type));
                    result.confidence_score -= 0.1;
                }
            }
            walkExpressionForTypeChecks(uo->operand, result, schema_context, variable_bindings);
        }
    }
}

void LLMSemanticValidator::checkAttributeTypes(
    const query::ASTNode* ast,
    SemanticValidationResult& result)
{
    spdlog::debug("[aql_semantic_validator] Type checking phase started");
    
    if (!ast) {
        return;
    }
    
    // Extract variable bindings from FOR/LET clauses
    auto variable_bindings = extractVariableBindings(ast);
    
    // If no variable bindings found, schema context isn't available
    if (variable_bindings.empty()) {
        result.warnings.push_back("Schema context unavailable; type inference limited to basic checks");
        result.confidence_score -= 0.1;
    }
    
    // Cast AST to Query to access clause expressions
    // In production, would check ASTNode::getType() == ASTNodeType::Query first
    auto query_ptr = reinterpret_cast<const query::Query*>(ast);
    if (!query_ptr) {
        spdlog::warn("[aql_semantic_validator] Failed to cast AST to Query");
        return;
    }
    
    // Walk FILTER expressions (zero or more filters in the query)
    for (const auto& filter_node : query_ptr->filters) {
        if (filter_node && filter_node->condition) {
            spdlog::debug("[aql_semantic_validator] Checking FILTER clause");
            walkExpressionForTypeChecks(
                filter_node->condition,
                result,
                schema_context_,
                variable_bindings);
        }
    }
    
    // Walk SORT expressions (zero or one sort clause)
    if (query_ptr->sort && !query_ptr->sort->specifications.empty()) {
        for (const auto& spec : query_ptr->sort->specifications) {
            if (spec.expression) {
                spdlog::debug("[aql_semantic_validator] Checking SORT clause");
                walkExpressionForTypeChecks(
                    spec.expression,
                    result,
                    schema_context_,
                    variable_bindings);
            }
        }
    }
    
    // Walk RETURN expression (should be present in most queries)
    if (query_ptr->return_node && query_ptr->return_node->expression) {
        spdlog::debug("[aql_semantic_validator] Checking RETURN clause");
        walkExpressionForTypeChecks(
            query_ptr->return_node->expression,
            result,
            schema_context_,
            variable_bindings);
    }
    
    // Walk LET expressions
    for (const auto& let_node : query_ptr->let_nodes) {
        if (let_node.expression) {
            spdlog::debug("[aql_semantic_validator] Checking LET clause");
            walkExpressionForTypeChecks(
                let_node.expression,
                result,
                schema_context_,
                variable_bindings);
        }
    }
    
    spdlog::debug("[aql_semantic_validator] Type checking phase complete");
}

void LLMSemanticValidator::validateJoins(
    const query::ASTNode* ast,
    SemanticValidationResult& result)
{
    spdlog::debug("[aql_semantic_validator] Join validation phase started");
    
    if (!ast) {
        return;
    }
    
    auto query_ptr = reinterpret_cast<const query::Query*>(ast);
    if (!query_ptr) {
        spdlog::warn("[aql_semantic_validator] Failed to cast AST to Query in validateJoins");
        return;
    }
    
    // PHASE 4.6: Join Detection
    // Detect JOIN operations (multiple FOR clauses)
    if (query_ptr-> static_cast<int>(for_nodes.size()) <= 1 && query_ptr->for_node.variable.empty()) {
        // No join detected (single collection query)
        spdlog::debug("[aql_semantic_validator] Single collection query (no JOIN)");
        return;
    }
    
    // Extract all FOR variables and collections
    std::vector<std::pair<std::string, std::string>> for_bindings;  // variable -> collection
    
    for (const auto& for_node : query_ptr->for_nodes) {
        for_bindings.push_back({for_node.variable, for_node.collection});
    }
    
    // Add legacy for_node if present
    if (!query_ptr->for_node.variable.empty()) {
        for_bindings.push_back({query_ptr->for_node.variable, query_ptr->for_node.collection});
    }
    
    // Detect join: multiple FOR bindings
    if (static_cast<int>(for_bindings.size()) <= 1) {
        spdlog::debug("[aql_semantic_validator] Single FOR clause (no JOIN)");
        return;
    }
    
    spdlog::debug("[aql_semantic_validator] JOIN detected with {} clauses",static_cast<int>(for_bindings.size()));
    result.warnings.push_back(
        fmt::format("JOIN detected: {} collections involved",static_cast<int>(for_bindings.size()))
    );
    
    // Step 1: Verify all referenced collections exist
    for (const auto& [var, collection] : for_bindings) {
        if (!schema_context_ || schema_context_->getCollectionCardinality(collection) == 0) {
            spdlog::warn("[aql_semantic_validator] Collection '{}' not found or has zero cardinality", collection);
            result.warnings.push_back(
                fmt::format("Collection '{}' referenced in JOIN not found in schema", collection)
            );
            result.confidence_score -= 0.2;
        }
    }
    
    // Step 2: Check for circular join dependencies (using dependency graph)
    // In AQL, circular joins typically involve self-referential or graph traversal
    // This is a simplified check: verify all collection names are distinct (or explicitly self-join)
    std::unordered_set<std::string> collection_names = {};

    for (const auto& [var, collection] : for_bindings) {
        if (collection_names.count(collection) > 0) {
            // Duplicate collection (possible self-join)
            spdlog::debug("[aql_semantic_validator] Self-join detected on collection '{}'", collection);
            result.warnings.push_back(
                fmt::format("Self-join on collection '{}' detected; verify join condition", collection)
            );
            result.confidence_score -= 0.1;
        }
        collection_names.insert(collection);
    }
    
    // Step 3: Verify join condition exists in FILTER clauses
    // Joins require at least one filter that references multiple FOR variables
    bool join_condition_found = false;
    
    for (const auto& filter_node : query_ptr->filters) {
        if (!filter_node || !filter_node->condition) {
          continue;
        }
        
        // Check if filter references multiple FOR variables
        std::unordered_set<std::string> referenced_vars = extractVariablesFromExpression(filter_node->condition);
        
        if (static_cast<int>(referenced_vars.size()) >= 2) {
            spdlog::debug("[aql_semantic_validator] Join condition found with {} variables",static_cast<int>(referenced_vars.size()));
            join_condition_found = true;
            break;
        }
    }
    
    if (!join_condition_found && static_cast<int>(for_bindings.size()) > 1) {
        spdlog::warn("[aql_semantic_validator] JOIN without explicit join condition (Cartesian product)");
        result.warnings.push_back(
            "JOIN without explicit join condition may result in Cartesian product (performance impact)"
        );
        result.confidence_score -= 0.25;
    }
    
    // Step 4: Estimate join selectivity
    estimateJoinSelectivity(query_ptr, result);
    
    spdlog::debug("[aql_semantic_validator] Join validation phase complete");
}

/**
 * @brief Helper: Extract variable names referenced in an expression
 * @param expr Expression node
 * @return Set of variable names found
 */
static std::unordered_set<std::string> extractVariablesFromExpression(
    const std::shared_ptr<query::Expression>& expr)
{
    std::unordered_set<std::string> variables;
    
    if (!expr) {
      return variables;
    }
    
    // Recursively traverse expression tree
    auto expr_type = expr->getType();
    
    if (expr_type == query::ASTNodeType::Variable) {
        auto var_expr = std::dynamic_pointer_cast<query::VariableExpr>(expr);
        if (var_expr) {
            variables.insert(var_expr->name);
        }
    } else if (expr_type == query::ASTNodeType::FieldAccess) {
        auto field_expr = std::dynamic_pointer_cast<query::FieldAccessExpr>(expr);
        if (field_expr) {
            variables.insert(field_expr->object);
        }
    } else if (expr_type == query::ASTNodeType::BinaryOp) {
        auto binop = std::dynamic_pointer_cast<query::BinaryOpExpr>(expr);
        if (binop) {
            auto left_vars = extractVariablesFromExpression(binop->left);
            auto right_vars = extractVariablesFromExpression(binop->right);
            variables.insert(left_vars.begin(), left_vars.end());
            variables.insert(right_vars.begin(), right_vars.end());
        }
    } else if (expr_type == query::ASTNodeType::UnaryOp) {
        auto unary = std::dynamic_pointer_cast<query::UnaryOpExpr>(expr);
        if (unary) {
            auto operand_vars = extractVariablesFromExpression(unary->operand);
            variables.insert(operand_vars.begin(), operand_vars.end());
        }
    } else if (expr_type == query::ASTNodeType::FunctionCall) {
        auto func_call = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
        if (func_call) {
            for (const auto& arg : func_call->arguments) {
                auto arg_vars = extractVariablesFromExpression(arg);
                variables.insert(arg_vars.begin(), arg_vars.end());
            }
        }
    } else if (expr_type == query::ASTNodeType::ArrayLiteral) {
        auto array_lit = std::dynamic_pointer_cast<query::ArrayLiteralExpr>(expr);
        if (array_lit) {
            for (const auto& elem : array_lit->elements) {
                auto elem_vars = extractVariablesFromExpression(elem);
                variables.insert(elem_vars.begin(), elem_vars.end());
            }
        }
    }
    
    return variables;
}

/**
 * @brief Helper: Estimate join selectivity
 * @param query Query AST with multiple FOR clauses
 * @param result Result structure to update
 */
static void estimateJoinSelectivity(
    const query::Query* query_ptr,
    SemanticValidationResult& result)
{
    if (!query_ptr || query_ptr-> static_cast<int>(for_nodes.size()) <= 1) {
        return;
    }
    
    // Estimate output rows based on collection cardinalities
    // For a JOIN: output_rows ≈ product of all cardinalities * selectivity factor
    
    uint64_t estimated_output = 1;
    uint64_t max_output = 1000000;  // 1M row SLA threshold
    
    std::vector<uint64_t> cardinalities;
    
    for (const auto& for_node : query_ptr->for_nodes) {
        // Note: In real implementation, would query schema_context for collection cardinality
        // For now, use a conservative estimate
        uint64_t cardinality = 10000;  // Default estimate
        cardinalities.push_back(cardinality);
    }
    
    // Cartesian product estimate (without join condition selectivity)
    for (uint64_t card : cardinalities) {
        // Avoid overflow
        if (estimated_output > max_output / card) {
            estimated_output = max_output * 10;
            break;
        }
        estimated_output *= card;
    }
    
    // Apply selectivity factor if join condition exists
    // Typical selectivity: 0.1% for equality joins
    double selectivity = 0.001;  // 0.1% default
    estimated_output = static_cast<uint64_t>(estimated_output * selectivity);
    
    // Ensure at least 1 row
    if (estimated_output == 0) {
        estimated_output = 1;
    }
    
    result.estimated_output_rows = estimated_output;
    
    spdlog::debug("[aql_semantic_validator] Estimated join output rows: {}", estimated_output);
    
    // Check against SLA threshold
    if (estimated_output > max_output) {
        spdlog::warn("[aql_semantic_validator] JOIN may produce {} rows (exceeds {} row threshold)", 
                     estimated_output, max_output);
        result.warnings.push_back(
            fmt::format("JOIN estimated to produce {} rows (SLA threshold: {} rows)", 
                       estimated_output, max_output)
        );
        result.confidence_score -= 0.15;
    }
}
    if (schema_context_) {
        auto collections = schema_context_->listCollections();
        spdlog::debug("[aql_semantic_validator] Available collections: {}",static_cast<int>(collections.size()));
    }
}

void LLMSemanticValidator::estimateCardinality(
    const query::ASTNode* ast,
    SemanticValidationResult& result)
{
    // PHASE 4.5 Implementation
    // This method will:
    // 1. Traverse AST to find collection scans
    // 2. Apply filter selectivity estimates
    // 3. Account for joins and aggregations
    // 4. Compute estimated result rows
    //
    // For now, conservative estimate
    spdlog::debug("[aql_semantic_validator] Cardinality estimation phase started");

    // Default conservative estimate: assume result size is manageable
    result.estimated_output_rows = 10000;

    if (result.estimated_output_rows.value() > config_.cardinality_warning_threshold) {
        result.warnings.push_back(
            fmt::format("Estimated output cardinality {} exceeds warning threshold {}",
                       result.estimated_output_rows.value(),
                       config_.cardinality_warning_threshold));
    }
}

/**
 * @brief Helper: Built-in AQL function signatures
 * 
 * Maps function name to (parameter_count_min, parameter_count_max, return_type)
 * Uses -1 for variadic (any number of parameters)
 */
struct FunctionSignature {
    size_t min_params = 0;
    size_t max_params;  // -1 = variadic
    std::string return_type;
    std::vector<std::string> param_types;  // Empty = any type
};

static const std::unordered_map<std::string, FunctionSignature> BUILTIN_FUNCTIONS = {
    // String functions
    {"CONCAT", {2, static_cast<size_t>(-1), "string", {}}},
    {"UPPER", {1, 1, "string", {"string"}}},
    {"LOWER", {1, 1, "string", {"string"}}},
    {"SUBSTRING", {2, 3, "string", {"string"}}},
    {"LENGTH", {1, 1, "number", {}}},  // Works on both string and array
    {"TRIM", {1, 1, "string", {"string"}}},
    {"LTRIM", {1, 1, "string", {"string"}}},
    {"RTRIM", {1, 1, "string", {"string"}}},
    {"CONTAINS", {2, 2, "boolean", {"string", "string"}}},
    {"LIKE", {2, 2, "boolean", {"string", "string"}}},
    
    // Numeric functions
    {"SUM", {1, 1, "number", {"array"}}},
    {"AVG", {1, 1, "number", {"array"}}},
    {"MIN", {1, 1, "number", {"array"}}},
    {"MAX", {1, 1, "number", {"array"}}},
    {"COUNT", {0, static_cast<size_t>(-1), "number", {}}},  // Variadic, flexible params
    {"FLOOR", {1, 1, "number", {"number"}}},
    {"CEIL", {1, 1, "number", {"number"}}},
    {"ROUND", {1, 2, "number", {"number"}}},
    {"ABS", {1, 1, "number", {"number"}}},
    
    // Array functions
    {"APPEND", {2, 2, "array", {"array", ""}}},
    {"REVERSE", {1, 1, "array", {"array"}}},
    {"FIRST", {1, 1, "", {"array"}}},
    {"LAST", {1, 1, "", {"array"}}},
    {"NTH", {2, 2, "", {"array", "number"}}},
    
    // Type functions
    {"TYPEOF", {1, 1, "string", {}}},
    {"IS_STRING", {1, 1, "boolean", {}}},
    {"IS_NUMBER", {1, 1, "boolean", {}}},
    {"IS_BOOL", {1, 1, "boolean", {}}},
    {"IS_ARRAY", {1, 1, "boolean", {}}},
    {"IS_OBJECT", {1, 1, "boolean", {}}},
    
    // Conditional functions
    {"IF", {3, 3, "", {}}},  // Return type depends on branches
    {"CASE", {2, static_cast<size_t>(-1), "", {}}},  // Variadic
};

void LLMSemanticValidator::validateFunctionSignatures(
    const query::ASTNode* ast,
    SemanticValidationResult& result)
{
    spdlog::debug("[aql_semantic_validator] Function signature validation phase started");
    
    if (!ast) {
        return;
    }
    
    auto query_ptr = reinterpret_cast<const query::Query*>(ast);
    if (!query_ptr) {
        spdlog::warn("[aql_semantic_validator] Failed to cast AST to Query in validateFunctionSignatures");
        return;
    }
    
    // Collect all function calls from different clauses
    std::vector<std::shared_ptr<query::FunctionCallExpr>> function_calls;
    
    // Extract from FILTER clauses
    for (const auto& filter_node : query_ptr->filters) {
        if (filter_node && filter_node->condition) {
            auto calls = extractFunctionCalls(filter_node->condition);
            function_calls.insert(function_calls.end(), calls.begin(), calls.end());
        }
    }
    
    // Extract from SORT clauses
    if (query_ptr->sort && !query_ptr->sort->specifications.empty()) {
        for (const auto& spec : query_ptr->sort->specifications) {
            if (spec.expression) {
                auto calls = extractFunctionCalls(spec.expression);
                function_calls.insert(function_calls.end(), calls.begin(), calls.end());
            }
        }
    }
    
    // Extract from RETURN clause
    if (query_ptr->return_node && query_ptr->return_node->expression) {
        auto calls = extractFunctionCalls(query_ptr->return_node->expression);
        function_calls.insert(function_calls.end(), calls.begin(), calls.end());
    }
    
    // Extract from LET clauses
    for (const auto& let_node : query_ptr->let_nodes) {
        if (let_node.expression) {
            auto calls = extractFunctionCalls(let_node.expression);
            function_calls.insert(function_calls.end(), calls.begin(), calls.end());
        }
    }
    
    spdlog::debug("[aql_semantic_validator] Found {} function calls",static_cast<int>(function_calls.size()));
    
    // Validate each function call
    for (const auto& func_call : function_calls) {
        if (!func_call) {
          continue;
        }
        
        const std::string& func_name = func_call->name;
        size_t param_count = func_call-> static_cast<int>(arguments.size());
        
        spdlog::debug("[aql_semantic_validator] Validating function call: {}({})", func_name, param_count);
        
        // Check if function is defined (either built-in or in schema)
        bool is_defined = false;
        bool is_builtin = BUILTIN_FUNCTIONS.count(func_name) > 0;
        
        if (is_builtin) {
            is_defined = true;
        } else if (schema_context_) {
            is_defined = schema_context_->isFunctionDefined(func_name);
        }
        
        if (!is_defined) {
            spdlog::warn("[aql_semantic_validator] Undefined function: {}", func_name);
            result.status = SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR;
            result.error_message = fmt::format("Undefined function: {}", func_name);
            result.warnings.push_back(
                fmt::format("Function '{}' is not defined", func_name)
            );
            result.confidence_score -= 0.4;
            continue;
        }
        
        // Validate parameter count for built-in functions
        if (is_builtin) {
            const auto& sig = BUILTIN_FUNCTIONS.at(func_name);
            
            if (param_count < sig.min_params) {
                spdlog::warn("[aql_semantic_validator] Function {} expects at least {} params, got {}", 
                             func_name, sig.min_params, param_count);
                result.status = SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR;
                result.error_message = fmt::format("Function {} expects at least {} parameters, got {}", 
                                                   func_name, sig.min_params, param_count);
                result.warnings.push_back(
                    fmt::format("Function '{}' expects at least {} parameters, got {}", 
                               func_name, sig.min_params, param_count)
                );
                result.confidence_score -= 0.3;
                continue;
            }
            
            if (sig.max_params != static_cast<size_t>(-1) && param_count > sig.max_params) {
                spdlog::warn("[aql_semantic_validator] Function {} expects at most {} params, got {}", 
                             func_name, sig.max_params, param_count);
                result.status = SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR;
                result.error_message = fmt::format("Function {} expects at most {} parameters, got {}", 
                                                   func_name, sig.max_params, param_count);
                result.warnings.push_back(
                    fmt::format("Function '{}' expects at most {} parameters, got {}", 
                               func_name, sig.max_params, param_count)
                );
                result.confidence_score -= 0.3;
                continue;
            }
            
            // Validate parameter types if specified
            if (!sig.param_types.empty()) {
                for (size_t i = 0; i < param_count  && static_cast<size_t>(i) <static_cast<int>(sig.param_types.size()); ++i) {
                    if (sig.param_types[i].empty()) continue;  // Any type allowed
                    
                    auto param_type = inferTypeFromExpression(func_call->arguments[i]);
                    if (param_type != sig.param_types[i] && param_type != "unknown") {
                        spdlog::debug("[aql_semantic_validator] Parameter {} of {} has type {}, expected {}", 
                                     i, func_name, param_type, sig.param_types[i]);
                        result.warnings.push_back(
                            fmt::format("Parameter {} of function '{}' has type '{}', expected '{}'", 
                                       i + 1, func_name, param_type, sig.param_types[i])
                        );
                        result.confidence_score -= 0.1;
                    }
                }
            }
        }
    }
    
    spdlog::debug("[aql_semantic_validator] Function signature validation phase complete");
}

/**
 * @brief Helper: Extract all function calls from an expression
 * @param expr Expression to traverse
 * @return Vector of function call expressions found
 */
static std::vector<std::shared_ptr<query::FunctionCallExpr>> extractFunctionCalls(
    const std::shared_ptr<query::Expression>& expr)
{
    std::vector<std::shared_ptr<query::FunctionCallExpr>> calls;
    
    if (!expr) {
      return calls;
    }
    
    auto expr_type = expr->getType();
    
    if (expr_type == query::ASTNodeType::FunctionCall) {
        auto func_call = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
        if (func_call) {
            calls.push_back(func_call);
            
            // Recursively extract from arguments
            for (const auto& arg : func_call->arguments) {
                auto nested_calls = extractFunctionCalls(arg);
                calls.insert(calls.end(), nested_calls.begin(), nested_calls.end());
            }
        }
    } else if (expr_type == query::ASTNodeType::BinaryOp) {
        auto binop = std::dynamic_pointer_cast<query::BinaryOpExpr>(expr);
        if (binop) {
            auto left_calls = extractFunctionCalls(binop->left);
            auto right_calls = extractFunctionCalls(binop->right);
            calls.insert(calls.end(), left_calls.begin(), left_calls.end());
            calls.insert(calls.end(), right_calls.begin(), right_calls.end());
        }
    } else if (expr_type == query::ASTNodeType::UnaryOp) {
        auto unary = std::dynamic_pointer_cast<query::UnaryOpExpr>(expr);
        if (unary) {
            auto operand_calls = extractFunctionCalls(unary->operand);
            calls.insert(calls.end(), operand_calls.begin(), operand_calls.end());
        }
    } else if (expr_type == query::ASTNodeType::ArrayLiteral) {
        auto array_lit = std::dynamic_pointer_cast<query::ArrayLiteralExpr>(expr);
        if (array_lit) {
            for (const auto& elem : array_lit->elements) {
                auto elem_calls = extractFunctionCalls(elem);
                calls.insert(calls.end(), elem_calls.begin(), elem_calls.end());
            }
        }
    }
    
    return calls;
}

void LLMSemanticValidator::computeConfidenceScore(SemanticValidationResult& result)
{
    // Compute overall confidence based on warnings and validation status
    double score = 1.0;

    // Deduct for status issues
    if (result.status != SemanticValidationResult::Status::VALID) {
        score = 0.1;  // Low confidence if validation failed
    }

    // Deduct for cardinality warnings
    if (result.status == SemanticValidationResult::Status::CARDINALITY_WARNING) {
        score = std::max(0.5, score - 0.2);
    }

    // Deduct per warning (up to 0.1 per warning)
    size_t warning_penalty = std::min(result.warnings.size(), size_t(2));
    score = std::max(config_.min_confidence_score, score - (warning_penalty * 0.05));

    result.confidence_score = std::max(0.0, std::min(1.0, score));

    spdlog::debug("[aql_semantic_validator] Confidence score computed: {:.2f} "
                 "(warnings={}, status_penalty={:.2f})",
                 result.confidence_score,
                 result.warnings.size(),
                 (1.0 - score));
}

} // namespace aql
} // namespace themis
