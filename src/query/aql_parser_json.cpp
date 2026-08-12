/**
 * @file aql_parser_json.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/aql_parser.h"

namespace themis {
namespace query {

// ============================================================================
// JSON Serialization for AST Nodes (moved from aql_parser.cpp)
// ============================================================================

nlohmann::json LiteralExpr::toJSON() const {
    nlohmann::json j = {{"type", "literal"}};
    
    std::visit([&j](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            j["value"] = nullptr;
        } else if constexpr (std::is_same_v<T, bool>) {
            j["value"] = arg;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            j["value"] = arg;
        } else if constexpr (std::is_same_v<T, double>) {
            j["value"] = arg;
        } else if constexpr (std::is_same_v<T, std::string>) {
            j["value"] = arg;
        } else if constexpr (std::is_same_v<T, nlohmann::json>) {
            j["value"] = arg; // Preserve complex JSON objects (GeoJSON, etc.)
        }
    }, value);
    
    return j;
}

nlohmann::json FieldAccessExpr::toJSON() const {
    return {
        {"type", "field_access"},
        {"object", object->toJSON()},
        {"field", field}
    };
}

nlohmann::json BinaryOpExpr::toJSON() const {
    const char* op_str = "unknown";
    switch (op) {
        case BinaryOperator::Eq: op_str = "=="; break;
        case BinaryOperator::Neq: op_str = "!="; break;
        case BinaryOperator::Lt: op_str = "<"; break;
        case BinaryOperator::Lte: op_str = "<="; break;
        case BinaryOperator::Gt: op_str = ">"; break;
        case BinaryOperator::Gte: op_str = ">="; break;
        case BinaryOperator::And: op_str = "AND"; break;
        case BinaryOperator::Or: op_str = "OR"; break;
        case BinaryOperator::Xor: op_str = "XOR"; break;
        case BinaryOperator::Add: op_str = "+"; break;
        case BinaryOperator::Sub: op_str = "-"; break;
        case BinaryOperator::Mul: op_str = "*"; break;
        case BinaryOperator::Div: op_str = "/"; break;
        case BinaryOperator::Mod: op_str = "%"; break;
        case BinaryOperator::In: op_str = "IN"; break;
    }
    
    return {
        {"type", "binary_op"},
        {"operator", op_str},
        {"left", left->toJSON()},
        {"right", right->toJSON()}
    };
}

nlohmann::json UnaryOpExpr::toJSON() const {
    const char* op_str = "unknown";
    switch (op) {
        case UnaryOperator::Not: op_str = "NOT"; break;
        case UnaryOperator::Minus: op_str = "-"; break;
        case UnaryOperator::Plus: op_str = "+"; break;
    }
    
    return {
        {"type", "unary_op"},
        {"operator", op_str},
        {"operand", operand->toJSON()}
    };
}

nlohmann::json FunctionCallExpr::toJSON() const {
    nlohmann::json args_json = nlohmann::json::array();
    for (const auto& arg : arguments) {
        args_json.push_back(arg->toJSON());
    }
    
    return {
        {"type", "function_call"},
        {"name", name},
        {"arguments", args_json}
    };
}

nlohmann::json ArrayLiteralExpr::toJSON() const {
    nlohmann::json elems_json = nlohmann::json::array();
    for (const auto& elem : elements) {
        elems_json.push_back(elem->toJSON());
    }
    
    return {
        {"type", "array_literal"},
        {"elements", elems_json}
    };
}

nlohmann::json ObjectConstructExpr::toJSON() const {
    nlohmann::json fields_json = nlohmann::json::object();
    for (const auto& [key, value] : fields) {
        fields_json[key] = value->toJSON();
    }
    
    return {
        {"type", "object_construct"},
        {"fields", fields_json}
    };
}

// Out-of-line toJSON for CTEDefinition (requires complete Query)
nlohmann::json CTEDefinition::toJSON() const {
    return {
        {"type", "cte_definition"},
        {"name", name},
        {"subquery", subquery ? subquery->toJSON() : nlohmann::json()}
    };
}

// Out-of-line toJSON for SubqueryExpr to ensure Query is complete
nlohmann::json SubqueryExpr::toJSON() const {
    return {
        {"type", "subquery"},
        {"query", subquery ? subquery->toJSON() : nlohmann::json()}
    };
}

} // namespace query
} // namespace themis
