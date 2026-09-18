/**
 * @file udf_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct UdfDefinition {
    std::string name;           ///< Unique UPPER_CASE function name
    std::string description;    ///< Human-readable description
    std::vector<ArgSpec> arguments; ///< Argument specifications
    ArgType return_type = ArgType::ANY; ///< Expected return type
    bool is_deterministic = true; ///< Same inputs always yield same output
    nlohmann::json body;        ///< Expression DSL (see above)
    std::string created_at;     ///< ISO-8601 creation timestamp
    std::string updated_at;     ///< ISO-8601 last-update timestamp

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;

    /**
     * @brief Parse Arg Type.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static ArgType parseArgType(const std::string& s);

    /**
     * @brief Arg Type To String.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    static std::string argTypeToString(ArgType t);

    static std::string validateBody(const nlohmann::json& expr, int depth = 0);
};

// ============================================================================
// UDF Function – IFunction wrapper around a UdfDefinition
// ============================================================================

class UdfFunction final : public IFunction {
public:
    /**
     * @brief Udf Function.
     * @param[in] def Input parameter.
     * @return Return value.
     */
    explicit UdfFunction(UdfDefinition def);

    FunctionSignature signature() const override;

    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context) const override;

private:
    UdfDefinition def_;
    FunctionSignature sig_;

    nlohmann::json evalExpr(
        const nlohmann::json& expr,
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context,
        int depth = 0) const;

    static constexpr int kMaxExprDepth = 64;
};

// ============================================================================
// UDF Registry – tracks which names in FunctionRegistry are UDFs
// ============================================================================

class UdfRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static UdfRegistry& instance() {
        static UdfRegistry reg;
        return reg;
    }

    /**
     * @brief Register Udf.
     * @param[in] def Input parameter.
     */
    void registerUdf(UdfDefinition def);

    /**
     * @brief Unregister Udf.
     * @param[in] name Input parameter.
     */
    void unregisterUdf(const std::string& name);

    /**
     * @brief Get Udf.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    UdfDefinition getUdf(const std::string& name) const;

    /**
     * @brief Has Udf.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasUdf(const std::string& name) const;

    /**
     * @brief List Udfs.
     * @return Return value.
     */
    std::vector<UdfDefinition> listUdfs() const;

private:
    UdfRegistry() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, UdfDefinition> udfs_;
};

} // namespace functions
} // namespace query
} // namespace themis
