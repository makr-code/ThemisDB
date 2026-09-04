/**
 * @file function_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Adapter for integrating FunctionRegistry with LetEvaluator
 * 
 * Provides a bridge between the new modular function system and the
 * existing procedural evaluator. Thread-safe lazy initialization.
 */
class FunctionAdapter {
public:
    /**
     * @brief Initialize the function registry (thread-safe, idempotent)
     * 
     * Call this during application startup or on first use.
     * Multiple calls are safe and have no effect after the first.
     */
    static void initialize() {
        std::call_once(init_flag_, []() {
            registerBuiltinFunctions();
            initialized_ = true;
        });
    }
    
    /**
     * @brief Check if a function exists in the registry
     * 
     * @param name Function name (case-sensitive)
     * @return true if function is registered
     */
    static bool hasFunction(const std::string& name) {
        initialize();
        return FunctionRegistry::instance().hasFunction(name);
    }
    
    /**
     * @brief Try to execute a function from the registry
     * 
     * @param name Function name
     * @param args Evaluated arguments
     * @param currentDoc Current document for context
     * @param result [out] Result if function was executed
     * @return true if function was found and executed, false to fall back
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
     * @brief Execute a function with full context
     * 
     * @param name Function name
     * @param args Evaluated arguments
     * @param context Full execution context
     * @return Result of function execution
     * @throws std::runtime_error if function not found or execution fails
     */
    static nlohmann::json call(
        const std::string& name,
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) {
        initialize();
        return FunctionRegistry::instance().call(name, args, context);
    }
    
    /**
     * @brief Create a FunctionContext from LetEvaluator state
     * 
     * @param currentDoc Current document being processed
     * @param variables Variable bindings (optional)
     * @return Configured FunctionContext
     */
    static FunctionContext createContext(
        const nlohmann::json& currentDoc,
        const std::unordered_map<std::string, nlohmann::json>* variables = nullptr
    ) {
        FunctionContext ctx(currentDoc);
        if (variables) {
            for (const auto& [name, value] : *variables) {
                ctx.setVariable(name, value);
            }
        }
        return ctx;
    }
    
    /**
     * @brief Get all available function names
     * @return Vector of function names
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
     * @brief Get function signature for documentation
     * @param name Function name
     * @return Function signature or nullopt if not found
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
     * @brief Check if functions have been initialized
     * @return true if registerBuiltinFunctions() has been called
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
