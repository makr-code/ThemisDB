/**
 * @file llm_semantic_validator.cpp
 * @brief Implementation of semantic validation for LLM-generated AQL queries
 * @version 0.1.0
 */

#include "aql/llm_semantic_validator.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <utility>

#include <spdlog/spdlog.h>

namespace themis {
namespace aql {
namespace {

std::string toUpperAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

std::unordered_map<std::string, std::string> extractVariableBindings(const query::Query* q) {
    std::unordered_map<std::string, std::string> out;
    if (!q) {
        return out;
    }

    for (const auto& node : q->for_nodes) {
        if (!node.variable.empty() && !node.collection.empty()) {
            out[node.variable] = node.collection;
        }
    }

    if (!q->for_node.variable.empty() && !q->for_node.collection.empty()) {
        out[q->for_node.variable] = q->for_node.collection;
    }

    return out;
}

std::string inferExprType(const std::shared_ptr<query::Expression>& expr) {
    if (!expr) {
        return "unknown";
    }

    switch (expr->getType()) {
        case query::ASTNodeType::Literal: {
            auto lit = std::dynamic_pointer_cast<query::LiteralExpr>(expr);
            if (!lit) {
                return "unknown";
            }
            if (std::holds_alternative<std::string>(lit->value)) {
                return "string";
            }
            if (std::holds_alternative<bool>(lit->value)) {
                return "boolean";
            }
            if (std::holds_alternative<int64_t>(lit->value) || std::holds_alternative<double>(lit->value)) {
                return "number";
            }
            if (std::holds_alternative<nlohmann::json>(lit->value)) {
                const auto& j = std::get<nlohmann::json>(lit->value);
                if (j.is_array()) {
                    return "array";
                }
                if (j.is_object()) {
                    return "object";
                }
            }
            return "unknown";
        }
        case query::ASTNodeType::ArrayLiteral:
            return "array";
        case query::ASTNodeType::ObjectConstruct:
            return "object";
        case query::ASTNodeType::FunctionCall: {
            auto fc = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
            if (!fc) {
                return "unknown";
            }
            const std::string fn = toUpperAscii(fc->name);
            if (fn == "COUNT" || fn == "SUM" || fn == "AVG" || fn == "MIN" || fn == "MAX" || fn == "LENGTH") {
                return "number";
            }
            if (fn == "CONCAT" || fn == "LOWER" || fn == "UPPER" || fn == "SUBSTRING") {
                return "string";
            }
            if (fn == "IS_STRING" || fn == "IS_NUMBER" || fn == "IS_BOOL" || fn == "IS_ARRAY" || fn == "IS_OBJECT") {
                return "boolean";
            }
            return "unknown";
        }
        default:
            return "unknown";
    }
}

void collectReferencedVariables(const std::shared_ptr<query::Expression>& expr,
                               std::unordered_set<std::string>& vars) {
    if (!expr) {
        return;
    }

    switch (expr->getType()) {
        case query::ASTNodeType::Variable: {
            auto v = std::dynamic_pointer_cast<query::VariableExpr>(expr);
            if (v && !v->name.empty()) {
                vars.insert(v->name);
            }
            break;
        }
        case query::ASTNodeType::FieldAccess: {
            auto f = std::dynamic_pointer_cast<query::FieldAccessExpr>(expr);
            if (f) {
                collectReferencedVariables(f->object, vars);
            }
            break;
        }
        case query::ASTNodeType::BinaryOp: {
            auto b = std::dynamic_pointer_cast<query::BinaryOpExpr>(expr);
            if (b) {
                collectReferencedVariables(b->left, vars);
                collectReferencedVariables(b->right, vars);
            }
            break;
        }
        case query::ASTNodeType::UnaryOp: {
            auto u = std::dynamic_pointer_cast<query::UnaryOpExpr>(expr);
            if (u) {
                collectReferencedVariables(u->operand, vars);
            }
            break;
        }
        case query::ASTNodeType::FunctionCall: {
            auto c = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
            if (c) {
                for (const auto& arg : c->arguments) {
                    collectReferencedVariables(arg, vars);
                }
            }
            break;
        }
        case query::ASTNodeType::ArrayLiteral: {
            auto a = std::dynamic_pointer_cast<query::ArrayLiteralExpr>(expr);
            if (a) {
                for (const auto& e : a->elements) {
                    collectReferencedVariables(e, vars);
                }
            }
            break;
        }
        case query::ASTNodeType::ObjectConstruct: {
            auto o = std::dynamic_pointer_cast<query::ObjectConstructExpr>(expr);
            if (o) {
                for (const auto& kv : o->fields) {
                    collectReferencedVariables(kv.second, vars);
                }
            }
            break;
        }
        default:
            break;
    }
}

void markHardFailure(SemanticValidationResult& result,
                     SemanticValidationResult::Status status,
                     std::string message) {
    if (result.status == SemanticValidationResult::Status::VALID) {
        result.status = status;
        result.error_message = std::move(message);
    }
}

} // namespace

LLMSemanticValidator::LLMSemanticValidator(
    std::shared_ptr<SemanticSchemaContext> schema_context,
    const Config& config)
    : schema_context_(std::move(schema_context)), config_(config) {
    logger_ = spdlog::get("aql");
    if (!logger_) {
        logger_ = spdlog::default_logger();
    }
}

SemanticValidationResult LLMSemanticValidator::validate(const query::ASTNode* ast) {
    const auto start = std::chrono::high_resolution_clock::now();
    SemanticValidationResult result;

    if (!ast) {
        result.status = SemanticValidationResult::Status::UNKNOWN_ERROR;
        result.error_message = "AST is null";
        result.validation_latency_ms = std::chrono::milliseconds{0};
        return result;
    }

    try {
        if (config_.enable_type_checking) {
            checkAttributeTypes(ast, result);
        }
        if (config_.enable_join_validation) {
            validateJoins(ast, result);
        }
        if (config_.enable_cardinality_estimation) {
            estimateCardinality(ast, result);
        }
        if (config_.enable_function_validation) {
            validateFunctionSignatures(ast, result);
        }

        computeConfidenceScore(result);
    } catch (const std::exception& e) {
        result.status = SemanticValidationResult::Status::UNKNOWN_ERROR;
        result.error_message = e.what();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    result.validation_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    return result;
}

void LLMSemanticValidator::configure(const Config& config) {
    config_ = config;
}

const LLMSemanticValidator::Config& LLMSemanticValidator::getConfig() const {
    return config_;
}

void LLMSemanticValidator::checkAttributeTypes(const query::ASTNode* ast,
                                               SemanticValidationResult& result) {
    const auto* q = reinterpret_cast<const query::Query*>(ast);
    if (!q) {
        markHardFailure(result, SemanticValidationResult::Status::UNKNOWN_ERROR, "AST cast to Query failed");
        return;
    }

    const auto bindings = extractVariableBindings(q);

    std::function<void(const std::shared_ptr<query::Expression>&)> walk;
    walk = [&](const std::shared_ptr<query::Expression>& expr) {
        if (!expr) {
            return;
        }

        if (expr->getType() == query::ASTNodeType::FieldAccess) {
            auto fa = std::dynamic_pointer_cast<query::FieldAccessExpr>(expr);
            if (fa) {
                if (schema_context_ && fa->object && fa->object->getType() == query::ASTNodeType::Variable) {
                    auto v = std::dynamic_pointer_cast<query::VariableExpr>(fa->object);
                    if (v && bindings.count(v->name) > 0) {
                        const auto& collection = bindings.at(v->name);
                        if (!schema_context_->getAttributeType(collection, fa->field).has_value()) {
                            result.warnings.push_back("Unknown attribute '" + collection + "." + fa->field + "'");
                        }
                    }
                }
                walk(fa->object);
            }
            return;
        }

        if (expr->getType() == query::ASTNodeType::BinaryOp) {
            auto bo = std::dynamic_pointer_cast<query::BinaryOpExpr>(expr);
            if (bo) {
                const std::string left = inferExprType(bo->left);
                const std::string right = inferExprType(bo->right);

                const bool numeric_mismatch =
                    (bo->op == query::BinaryOperator::Add || bo->op == query::BinaryOperator::Sub ||
                     bo->op == query::BinaryOperator::Mul || bo->op == query::BinaryOperator::Div ||
                     bo->op == query::BinaryOperator::Mod) &&
                    left != "unknown" && right != "unknown" &&
                    !(left == "number" && right == "number") &&
                    !(bo->op == query::BinaryOperator::Add && left == "string" && right == "string");

                if (numeric_mismatch) {
                    markHardFailure(result,
                                    SemanticValidationResult::Status::TYPE_MISMATCH,
                                    "Incompatible operand types for binary operation: " + left + " vs " + right);
                }

                walk(bo->left);
                walk(bo->right);
            }
            return;
        }

        if (expr->getType() == query::ASTNodeType::UnaryOp) {
            auto uo = std::dynamic_pointer_cast<query::UnaryOpExpr>(expr);
            if (uo) {
                walk(uo->operand);
            }
            return;
        }

        if (expr->getType() == query::ASTNodeType::FunctionCall) {
            auto fc = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
            if (fc) {
                for (const auto& arg : fc->arguments) {
                    walk(arg);
                }
            }
            return;
        }

        if (expr->getType() == query::ASTNodeType::ArrayLiteral) {
            auto arr = std::dynamic_pointer_cast<query::ArrayLiteralExpr>(expr);
            if (arr) {
                for (const auto& e : arr->elements) {
                    walk(e);
                }
            }
            return;
        }

        if (expr->getType() == query::ASTNodeType::ObjectConstruct) {
            auto obj = std::dynamic_pointer_cast<query::ObjectConstructExpr>(expr);
            if (obj) {
                for (const auto& kv : obj->fields) {
                    walk(kv.second);
                }
            }
        }
    };

    for (const auto& f : q->filters) {
        if (f && f->condition) {
            walk(f->condition);
        }
    }
    if (q->sort) {
        for (const auto& spec : q->sort->specifications) {
            walk(spec.expression);
        }
    }
    if (q->return_node) {
        walk(q->return_node->expression);
    }
    for (const auto& let_node : q->let_nodes) {
        walk(let_node.expression);
    }
}

void LLMSemanticValidator::estimateCardinality(const query::ASTNode* ast,
                                               SemanticValidationResult& result) {
    const auto* q = reinterpret_cast<const query::Query*>(ast);
    if (!q) {
        return;
    }

    const auto bindings = extractVariableBindings(q);
    if (bindings.empty()) {
        result.estimated_output_rows = static_cast<size_t>(10000);
        return;
    }

    size_t estimate = 1;
    for (const auto& kv : bindings) {
        size_t card = 10000;
        if (schema_context_) {
            auto c = schema_context_->getCollectionCardinality(kv.second);
            if (c.has_value()) {
                card = *c;
            }
        }

        if (card == 0) {
            card = 1;
        }

        if (estimate > (std::numeric_limits<size_t>::max() / card)) {
            estimate = std::numeric_limits<size_t>::max();
            break;
        }
        estimate *= card;
    }

    if (!q->filters.empty()) {
        estimate = std::max<size_t>(1, estimate / 10);
    }

    result.estimated_output_rows = estimate;
    if (estimate > config_.cardinality_warning_threshold) {
        result.warnings.push_back("Estimated output cardinality exceeds configured warning threshold");
    }
}

void LLMSemanticValidator::validateJoins(const query::ASTNode* ast,
                                         SemanticValidationResult& result) {
    const auto* q = reinterpret_cast<const query::Query*>(ast);
    if (!q) {
        return;
    }

    const auto bindings = extractVariableBindings(q);
    if (bindings.size() <= 1) {
        return;
    }

    bool missing_collection = false;
    if (schema_context_) {
        for (const auto& kv : bindings) {
            if (!schema_context_->getCollectionCardinality(kv.second).has_value()) {
                missing_collection = true;
                result.warnings.push_back("JOIN references collection without cardinality metadata: " + kv.second);
            }
        }
    }

    bool found_join_predicate = false;
    for (const auto& filter : q->filters) {
        if (!filter || !filter->condition) {
            continue;
        }
        std::unordered_set<std::string> vars;
        collectReferencedVariables(filter->condition, vars);
        if (vars.size() >= 2) {
            found_join_predicate = true;
            break;
        }
    }

    if (!found_join_predicate) {
        result.warnings.push_back("JOIN without explicit join predicate may create a Cartesian product");
    }

    if (missing_collection) {
        markHardFailure(result,
                        SemanticValidationResult::Status::JOIN_IMPOSSIBLE,
                        "JOIN references unknown collections or missing collection statistics");
    }
}

void LLMSemanticValidator::validateFunctionSignatures(const query::ASTNode* ast,
                                                      SemanticValidationResult& result) {
    const auto* q = reinterpret_cast<const query::Query*>(ast);
    if (!q) {
        return;
    }

    auto validateCall = [&](const std::shared_ptr<query::FunctionCallExpr>& fc) {
        if (!fc) {
            return;
        }

        const std::string fn = toUpperAscii(fc->name);
        if (schema_context_ && !schema_context_->isFunctionDefined(fn)) {
            markHardFailure(result,
                            SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR,
                            "Unknown function: " + fn);
            return;
        }

        const size_t argc = fc->arguments.size();
        if ((fn == "LOWER" || fn == "UPPER" || fn == "LENGTH") && argc != 1) {
            markHardFailure(result,
                            SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR,
                            "Function " + fn + " expects exactly 1 argument");
        }
        if (fn == "SUBSTRING" && (argc < 2 || argc > 3)) {
            markHardFailure(result,
                            SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR,
                            "Function SUBSTRING expects 2 or 3 arguments");
        }
        if (fn == "CONCAT" && argc < 2) {
            markHardFailure(result,
                            SemanticValidationResult::Status::FUNCTION_SIGNATURE_ERROR,
                            "Function CONCAT expects at least 2 arguments");
        }
    };

    std::function<void(const std::shared_ptr<query::Expression>&)> walk;
    walk = [&](const std::shared_ptr<query::Expression>& expr) {
        if (!expr) {
            return;
        }
        switch (expr->getType()) {
            case query::ASTNodeType::FunctionCall: {
                auto fc = std::dynamic_pointer_cast<query::FunctionCallExpr>(expr);
                validateCall(fc);
                if (fc) {
                    for (const auto& arg : fc->arguments) {
                        walk(arg);
                    }
                }
                break;
            }
            case query::ASTNodeType::BinaryOp: {
                auto bo = std::dynamic_pointer_cast<query::BinaryOpExpr>(expr);
                if (bo) {
                    walk(bo->left);
                    walk(bo->right);
                }
                break;
            }
            case query::ASTNodeType::UnaryOp: {
                auto uo = std::dynamic_pointer_cast<query::UnaryOpExpr>(expr);
                if (uo) {
                    walk(uo->operand);
                }
                break;
            }
            case query::ASTNodeType::FieldAccess: {
                auto fa = std::dynamic_pointer_cast<query::FieldAccessExpr>(expr);
                if (fa) {
                    walk(fa->object);
                }
                break;
            }
            case query::ASTNodeType::ArrayLiteral: {
                auto ar = std::dynamic_pointer_cast<query::ArrayLiteralExpr>(expr);
                if (ar) {
                    for (const auto& e : ar->elements) {
                        walk(e);
                    }
                }
                break;
            }
            case query::ASTNodeType::ObjectConstruct: {
                auto obj = std::dynamic_pointer_cast<query::ObjectConstructExpr>(expr);
                if (obj) {
                    for (const auto& kv : obj->fields) {
                        walk(kv.second);
                    }
                }
                break;
            }
            default:
                break;
        }
    };

    for (const auto& f : q->filters) {
        if (f) {
            walk(f->condition);
        }
    }
    if (q->sort) {
        for (const auto& spec : q->sort->specifications) {
            walk(spec.expression);
        }
    }
    if (q->return_node) {
        walk(q->return_node->expression);
    }
    for (const auto& let_node : q->let_nodes) {
        walk(let_node.expression);
    }
}

void LLMSemanticValidator::computeConfidenceScore(SemanticValidationResult& result) {
    double score = 1.0;
    score -= 0.07 * static_cast<double>(result.warnings.size());

    if (result.status != SemanticValidationResult::Status::VALID) {
        score -= 0.35;
    }

    if (result.estimated_output_rows.has_value() &&
        result.estimated_output_rows.value() > config_.cardinality_warning_threshold) {
        score -= 0.1;
    }

    score = std::clamp(score, 0.0, 1.0);
    result.confidence_score = score;
}

} // namespace aql
} // namespace themis
