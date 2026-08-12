/**
 * @file udf_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/functions/function_registry.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <stdexcept>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// UDF Definition
// ============================================================================

/**
 * @brief Descriptor for a user-registered AQL function.
 *
 * Users submit this via `POST /api/v1/query/udfs`.  The body is a JSON
 * expression tree that is evaluated at query time.
 *
 * ### Body expression DSL
 *
 * Each node is a JSON object with a `"type"` field:
 *
 * | type       | description                              | extra fields                  |
 * |------------|------------------------------------------|-------------------------------|
 * | `const`    | literal value                            | `"value"`: any JSON value     |
 * | `arg`      | positional function argument             | `"index"`: integer            |
 * | `call`     | call a built-in or already-registered UDF| `"function"`, `"args"`: array |
 * | `op`       | binary arithmetic / comparison           | `"op"`, `"left"`, `"right"`   |
 * | `if`       | conditional expression                   | `"cond"`, `"then"`, `"else"`  |
 *
 * #### Supported operators for `op`
 * `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`
 *
 * ### Example
 * ```json
 * {
 *   "type": "call",
 *   "function": "UPPER",
 *   "args": [{"type": "arg", "index": 0}]
 * }
 * ```
 */
struct UdfDefinition {
    std::string name;           ///< Unique UPPER_CASE function name
    std::string description;    ///< Human-readable description
    std::vector<ArgSpec> arguments; ///< Argument specifications
    ArgType return_type = ArgType::ANY; ///< Expected return type
    bool is_deterministic = true; ///< Same inputs always yield same output
    nlohmann::json body;        ///< Expression DSL (see above)
    std::string created_at;     ///< ISO-8601 creation timestamp
    std::string updated_at;     ///< ISO-8601 last-update timestamp

    /// Serialise to JSON for API responses.
    nlohmann::json toJson() const;

    /// Parse argument type from string; throws on unknown type.
    static ArgType parseArgType(const std::string& s);

    /// Serialise ArgType to string.
    static std::string argTypeToString(ArgType t);

    /**
     * @brief Validate an expression body before storage.
     *
     * Performs a shallow structural check (type field present, required sub-keys
     * present) without evaluating values.  Returns empty string on success, or
     * an error message on failure.
     *
     * @param expr  The JSON expression node to validate.
     * @param depth Current recursion depth (callers pass 0).
     */
    static std::string validateBody(const nlohmann::json& expr, int depth = 0);
};

// ============================================================================
// UDF Function – IFunction wrapper around a UdfDefinition
// ============================================================================

/**
 * @brief AQL-callable wrapper for a user-defined function.
 *
 * Instances of this class are inserted into the global `FunctionRegistry` so
 * that AQL queries can call user functions exactly like built-ins.
 */
class UdfFunction final : public IFunction {
public:
    explicit UdfFunction(UdfDefinition def);

    FunctionSignature signature() const override;

    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context) const override;

private:
    UdfDefinition def_;
    FunctionSignature sig_;

    /// Recursively evaluate a body expression node.
    /// @param depth  Current recursion depth (starts at 0, max kMaxExprDepth).
    nlohmann::json evalExpr(
        const nlohmann::json& expr,
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context,
        int depth = 0) const;

    /// Maximum recursion depth for expression evaluation (resource exhaustion guard).
    static constexpr int kMaxExprDepth = 64;
};

// ============================================================================
// UDF Registry – tracks which names in FunctionRegistry are UDFs
// ============================================================================

/**
 * @brief Thin bookkeeping layer on top of the global FunctionRegistry.
 *
 * Keeps a set of UDF names so the API can distinguish user-registered
 * functions from built-ins and can safely unregister them.
 *
 * Thread-safe singleton.
 */
class UdfRegistry {
public:
    static UdfRegistry& instance() {
        static UdfRegistry reg;
        return reg;
    }

    /**
     * @brief Register a new UDF.  Overwrites any existing UDF with the same name.
     * @throws std::runtime_error if name conflicts with a built-in function
     *         that was NOT previously registered as a UDF.
     */
    void registerUdf(UdfDefinition def);

    /**
     * @brief Remove a UDF and deregister it from the FunctionRegistry.
     * @throws std::runtime_error if the name is unknown or is a built-in.
     */
    void unregisterUdf(const std::string& name);

    /// Return definition for a registered UDF.
    /// @throws std::runtime_error if not found.
    UdfDefinition getUdf(const std::string& name) const;

    /// True if name is a registered UDF (not a built-in).
    bool hasUdf(const std::string& name) const;

    /// Return definitions for all registered UDFs.
    std::vector<UdfDefinition> listUdfs() const;

private:
    UdfRegistry() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, UdfDefinition> udfs_;
};

} // namespace functions
} // namespace query
} // namespace themis
