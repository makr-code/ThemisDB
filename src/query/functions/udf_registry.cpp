/**
 * @file udf_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/functions/udf_registry.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Utilities
// ============================================================================

static std::string utcNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// ============================================================================
// UdfDefinition
// ============================================================================

ArgType UdfDefinition::parseArgType(const std::string& s) {
    if (s == "ANY")      return ArgType::ANY;
    if (s == "STRING")   return ArgType::STRING;
    if (s == "NUMBER")   return ArgType::NUMBER;
    if (s == "INTEGER")  return ArgType::INTEGER;
    if (s == "BOOLEAN")  return ArgType::BOOLEAN;
    if (s == "ARRAY")    return ArgType::ARRAY;
    if (s == "OBJECT")   return ArgType::OBJECT;
    if (s == "GEOMETRY") return ArgType::GEOMETRY;
    if (s == "VECTOR")   return ArgType::VECTOR;
    if (s == "DOCUMENT") return ArgType::DOCUMENT;
    if (s == "NULLABLE") return ArgType::NULLABLE;
    throw std::runtime_error("Unknown argument type: " + s);
}

std::string UdfDefinition::argTypeToString(ArgType t) {
    switch (t) {
        case ArgType::ANY:      return "ANY";
        case ArgType::STRING:   return "STRING";
        case ArgType::NUMBER:   return "NUMBER";
        case ArgType::INTEGER:  return "INTEGER";
        case ArgType::BOOLEAN:  return "BOOLEAN";
        case ArgType::ARRAY:    return "ARRAY";
        case ArgType::OBJECT:   return "OBJECT";
        case ArgType::GEOMETRY: return "GEOMETRY";
        case ArgType::VECTOR:   return "VECTOR";
        case ArgType::DOCUMENT: return "DOCUMENT";
        case ArgType::NULLABLE: return "NULLABLE";
        default:                return "ANY";
    }
}

nlohmann::json UdfDefinition::toJson() const {
    nlohmann::json args_json = nlohmann::json::array();
    for (const auto& a : arguments) {
        args_json.push_back({
            {"name", a.name},
            {"type", argTypeToString(a.type)},
            {"required", a.required},
            {"description", a.description}
        });
    }
    return {
        {"name",             name},
        {"description",      description},
        {"arguments",        args_json},
        {"return_type",      argTypeToString(return_type)},
        {"is_deterministic", is_deterministic},
        {"body",             body},
        {"created_at",       created_at},
        {"updated_at",       updated_at}
    };
}

std::string UdfDefinition::validateBody(const nlohmann::json& expr, int depth) {
    constexpr int kMaxValidateDepth = 64;
    if (depth > kMaxValidateDepth) {
        return "expression body exceeds maximum nesting depth";
    }
    if (!expr.is_object()) {
        return "expression node must be a JSON object";
    }
    if (!expr.contains("type") || !expr["type"].is_string()) {
        return "expression node must have a string 'type' field";
    }

    const std::string type = expr["type"].get<std::string>();

    if (type == "const") {
        if (!expr.contains("value")) return "'const' node requires 'value'";
    } else if (type == "arg") {
        if (!expr.contains("index") || !expr["index"].is_number_integer())
            return "'arg' node requires integer 'index'";
    } else if (type == "call") {
        if (!expr.contains("function") || !expr["function"].is_string())
            return "'call' node requires string 'function'";
        if (expr.contains("args")) {
            if (!expr["args"].is_array()) return "'call' node 'args' must be an array";
            for (const auto& a : expr["args"]) {
                auto err = validateBody(a, depth + 1);
                if (!err.empty()) return err;
            }
        }
    } else if (type == "op") {
        if (!expr.contains("op") || !expr["op"].is_string())
            return "'op' node requires string 'op'";
        if (!expr.contains("left") || !expr.contains("right"))
            return "'op' node requires 'left' and 'right'";
        auto err = validateBody(expr["left"], depth + 1);
        if (!err.empty()) return err;
        err = validateBody(expr["right"], depth + 1);
        if (!err.empty()) return err;
    } else if (type == "if") {
        if (!expr.contains("cond") || !expr.contains("then") || !expr.contains("else"))
            return "'if' node requires 'cond', 'then', 'else'";
        auto err = validateBody(expr["cond"], depth + 1);
        if (!err.empty()) return err;
        err = validateBody(expr["then"], depth + 1);
        if (!err.empty()) return err;
        err = validateBody(expr["else"], depth + 1);
        if (!err.empty()) return err;
    } else {
        return "unknown expression type '" + type + "'";
    }
    return "";
}

// ============================================================================
// UdfFunction
// ============================================================================

UdfFunction::UdfFunction(UdfDefinition def) : def_(std::move(def)) {
    sig_.name             = def_.name;
    sig_.category         = "UDF";
    sig_.description      = def_.description;
    sig_.arguments        = def_.arguments;
    sig_.return_type      = def_.return_type;
    sig_.is_deterministic = def_.is_deterministic;
    sig_.is_aggregate     = false;
    sig_.cost             = FunctionCost{CostComplexity::EXTERNAL, 5.0, 0.0, false, false};
}

FunctionSignature UdfFunction::signature() const {
    return sig_;
}

nlohmann::json UdfFunction::execute(
    const std::vector<nlohmann::json>& args,
    const FunctionContext& context) const
{
    return evalExpr(def_.body, args, context, /*depth=*/0);
}

nlohmann::json UdfFunction::evalExpr(
    const nlohmann::json& expr,
    const std::vector<nlohmann::json>& args,
    const FunctionContext& context,
    int depth) const
{
    if (depth > kMaxExprDepth) {
        throw std::runtime_error(def_.name + ": expression depth limit exceeded");
    }

    if (!expr.is_object()) {
        throw std::runtime_error(def_.name + ": body expression must be an object");
    }

    if (!expr.contains("type") || !expr["type"].is_string()) {
        throw std::runtime_error(def_.name + ": expression node must have a string 'type' field");
    }

    const std::string type = expr["type"].get<std::string>();

    // ── const: literal value ─────────────────────────────────────────────────
    if (type == "const") {
        if (!expr.contains("value")) {
            throw std::runtime_error(def_.name + ": 'const' node requires 'value'");
        }
        return expr["value"];
    }

    // ── arg: positional argument ─────────────────────────────────────────────
    if (type == "arg") {
        if (!expr.contains("index") || !expr["index"].is_number_integer()) {
            throw std::runtime_error(def_.name + ": 'arg' node requires integer 'index'");
        }
        int idx = expr["index"].get<int>();
        if (idx < 0 || static_cast<size_t>(idx) >= args.size()) {
            throw std::runtime_error(def_.name + ": argument index " +
                                     std::to_string(idx) + " out of range");
        }
        return args[static_cast<size_t>(idx)];
    }

    // ── call: built-in or UDF function call ──────────────────────────────────
    if (type == "call") {
        if (!expr.contains("function") || !expr["function"].is_string()) {
            throw std::runtime_error(def_.name + ": 'call' node requires string 'function'");
        }
        const std::string fname = expr["function"].get<std::string>();

        std::vector<nlohmann::json> callArgs;
        if (expr.contains("args") && expr["args"].is_array()) {
            for (const auto& a : expr["args"]) {
                callArgs.push_back(evalExpr(a, args, context, depth + 1));
            }
        }
        return FunctionRegistry::instance().call(fname, callArgs, context);
    }

    // ── op: binary operator ──────────────────────────────────────────────────
    if (type == "op") {
        if (!expr.contains("op") || !expr["op"].is_string()) {
            throw std::runtime_error(def_.name + ": 'op' node requires string 'op'");
        }
        if (!expr.contains("left") || !expr.contains("right")) {
            throw std::runtime_error(def_.name + ": 'op' node requires 'left' and 'right'");
        }

        const std::string op    = expr["op"].get<std::string>();
        nlohmann::json    left  = evalExpr(expr["left"],  args, context, depth + 1);
        nlohmann::json    right = evalExpr(expr["right"], args, context, depth + 1);

        // Arithmetic (numeric)
        auto toNum = [](const nlohmann::json& v) -> double {
            if (v.is_number()) return v.get<double>();
            throw std::runtime_error("Expected numeric operand");
        };

        if (op == "+") {
            if (left.is_string() || right.is_string())
                return left.get<std::string>() + right.get<std::string>();
            return toNum(left) + toNum(right);
        }
        if (op == "-")  return toNum(left) - toNum(right);
        if (op == "*")  return toNum(left) * toNum(right);
        if (op == "/") {
            double r = toNum(right);
            if (r == 0.0) throw std::runtime_error(def_.name + ": division by zero");
            return toNum(left) / r;
        }
        if (op == "%") {
            double r = toNum(right);
            if (r == 0.0) throw std::runtime_error(def_.name + ": modulo by zero");
            return std::fmod(toNum(left), r);
        }

        // Comparison
        if (op == "==") return left == right;
        if (op == "!=") return left != right;
        if (op == "<") {
            if (left.is_number() && right.is_number())
                return toNum(left) < toNum(right);
            return left.get<std::string>() < right.get<std::string>();
        }
        if (op == "<=") {
            if (left.is_number() && right.is_number())
                return toNum(left) <= toNum(right);
            return left.get<std::string>() <= right.get<std::string>();
        }
        if (op == ">") {
            if (left.is_number() && right.is_number())
                return toNum(left) > toNum(right);
            return left.get<std::string>() > right.get<std::string>();
        }
        if (op == ">=") {
            if (left.is_number() && right.is_number())
                return toNum(left) >= toNum(right);
            return left.get<std::string>() >= right.get<std::string>();
        }

        // Logical
        if (op == "&&") {
            auto toBool = [](const nlohmann::json& v) -> bool {
                if (v.is_boolean()) return v.get<bool>();
                if (v.is_null())    return false;
                if (v.is_number())  return v.get<double>() != 0;
                if (v.is_string())  return !v.get<std::string>().empty();
                return true;
            };
            return toBool(left) && toBool(right);
        }
        if (op == "||") {
            auto toBool = [](const nlohmann::json& v) -> bool {
                if (v.is_boolean()) return v.get<bool>();
                if (v.is_null())    return false;
                if (v.is_number())  return v.get<double>() != 0;
                if (v.is_string())  return !v.get<std::string>().empty();
                return true;
            };
            return toBool(left) || toBool(right);
        }

        throw std::runtime_error(def_.name + ": unknown operator '" + op + "'");
    }

    // ── if: conditional ──────────────────────────────────────────────────────
    if (type == "if") {
        if (!expr.contains("cond") || !expr.contains("then") || !expr.contains("else")) {
            throw std::runtime_error(def_.name + ": 'if' node requires 'cond', 'then', 'else'");
        }
        nlohmann::json cond = evalExpr(expr["cond"], args, context, depth + 1);
        bool condVal = false;
        if (cond.is_boolean()) condVal = cond.get<bool>();
        else if (!cond.is_null()) condVal = true;

        return evalExpr(condVal ? expr["then"] : expr["else"], args, context, depth + 1);
    }

    throw std::runtime_error(def_.name + ": unknown expression type '" + type + "'");
}

// ============================================================================
// UdfRegistry
// ============================================================================

void UdfRegistry::registerUdf(UdfDefinition def) {
    // Basic name validation: non-empty, no spaces
    if (def.name.empty()) {
        throw std::runtime_error("UDF name must not be empty");
    }
    for (char c : def.name) {
        if (c == ' ' || c == '\t') {
            throw std::runtime_error("UDF name must not contain whitespace");
        }
    }

    // Validate expression body before storing
    auto bodyErr = UdfDefinition::validateBody(def.body);
    if (!bodyErr.empty()) {
        throw std::runtime_error("Invalid UDF body: " + bodyErr);
    }

    auto& freg = FunctionRegistry::instance();

    std::lock_guard<std::mutex> lock(mutex_);

    // If a name already exists in FunctionRegistry but is NOT a known UDF,
    // it is a built-in; refuse to overwrite.
    if (freg.hasFunction(def.name) && udfs_.find(def.name) == udfs_.end()) {
        throw std::runtime_error("Cannot override built-in function: " + def.name);
    }

    const std::string now = utcNow();
    if (def.created_at.empty()) {
        def.created_at = now;
    }
    def.updated_at = now;

    // Register in global function registry (overwrites previous UDF with same name)
    freg.registerFunction(std::make_unique<UdfFunction>(def));

    udfs_[def.name] = std::move(def);
}

void UdfRegistry::unregisterUdf(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = udfs_.find(name);
    if (it == udfs_.end()) {
        throw std::runtime_error("UDF not found: " + name);
    }

    FunctionRegistry::instance().unregisterFunction(name);
    udfs_.erase(it);
}

UdfDefinition UdfRegistry::getUdf(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = udfs_.find(name);
    if (it == udfs_.end()) {
        throw std::runtime_error("UDF not found: " + name);
    }
    return it->second;
}

bool UdfRegistry::hasUdf(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return udfs_.find(name) != udfs_.end();
}

std::vector<UdfDefinition> UdfRegistry::listUdfs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<UdfDefinition> result;
    result.reserve(udfs_.size());
    for (const auto& kv : udfs_) {
        result.push_back(kv.second);
    }
    return result;
}

} // namespace functions
} // namespace query
} // namespace themis

