/**
 * @file function_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace themis {
namespace query {
namespace functions {

class FunctionAdapter {
public:
    /**
     * @brief Initialize.
     * @details Calls: std::call_once(), registerBuiltinFunctions().
     */
    static void initialize() {
        std::call_once(init_flag_, []() {
            registerBuiltinFunctions();
            initialized_ = true;
        });
    }
    
    /**
     * @brief Has Function.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: initialize(), FunctionRegistry::instance().
     */
    static bool hasFunction(const std::string& name) {
        initialize();
        return FunctionRegistry::instance().hasFunction(name);
    }
    
    /**
     * @brief Try Call.
     * @param[in] name Input parameter.
     * @param[in] args Input parameter.
     * @param[in] currentDoc Input parameter.
     * @param[in,out] result Input/output parameter.
     * @return True when the operation succeeds.
     * @details Calls: initialize(), FunctionRegistry::instance(), hasFunction(), ctx(), call().
     */
    static bool tryCall(
        const std::string& name,
        const std::vector<nlohmann::json>& args,
        const nlohmann::json& currentDoc,
        nlohmann::json& result
    ) {
        initialize();
        
        auto& registry = FunctionRegistry::instance();
        if (!registry.hasFunction(name)) {
            return false;
        }
        
        try {
            FunctionContext ctx(currentDoc);
            result = registry.call(name, args, ctx);
            return true;
        } catch (const std::exception& e) {
            // Let the caller handle errors or fall back to legacy
            throw;
        }
    }
    
    /**
     * @brief Call.
     * @param[in] name Input parameter.
     * @param[in] args Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
     * @details Calls: initialize(), FunctionRegistry::instance().
     */
    static nlohmann::json call(
        const std::string& name,
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) {
        initialize();
        return FunctionRegistry::instance().call(name, args, context);
    }
    
    static FunctionContext createContext(
        const nlohmann::json& currentDoc,
        const std::unordered_map<std::string, nlohmann::json>* variables = nullptr
    ) {
        /**
         * @brief Ctx.
         * @param[in] currentDoc Input parameter.
         * @return Return value.
         */
        FunctionContext ctx(currentDoc);
        if (variables) {
            for (const auto& [name, value] : *variables) {
                ctx.setVariable(name, value);
            }
        }
        return ctx;
    }
    
    /**
     * @brief Get Available Functions.
     * @return Return value.
     * @details Calls: initialize(), FunctionRegistry::instance(), getAllSignatures(), push_back().
     */
    static std::vector<std::string> getAvailableFunctions() {
        initialize();
        std::vector<std::string> names = {};

        for (const auto& sig : FunctionRegistry::instance().getAllSignatures()) {
            names.push_back(sig.name);
        }
        return names;
    }
    
    /**
     * @brief Get Signature.
     * @param[in] name Input parameter.
     * @return Return value.
     * @details Calls: initialize(), FunctionRegistry::instance(), hasFunction(), getFunction(), signature().
     */
    static std::optional<FunctionSignature> getSignature(const std::string& name) {
        initialize();
        auto& registry = FunctionRegistry::instance();
        if (!registry.hasFunction(name)) {
            return std::nullopt;
        }
        return registry.getFunction(name).signature();
    }
    
    /**
     * @brief Is Initialized.
     * @return True when the operation succeeds.
     * @details Implements isInitialized without additional internal calls.
     */
    static bool isInitialized() {
        return initialized_;
    }

private:
    static std::once_flag init_flag_;
    static bool initialized_;
};

// Static member initialization (in header for header-only usage)
inline std::once_flag FunctionAdapter::init_flag_;
inline bool FunctionAdapter::initialized_ = false;

} // namespace functions
} // namespace query
} // namespace themis
