#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <stdexcept>

namespace themis {
namespace query {
namespace functions {

/**
 * @brief AQL Function Registry - Modulares OOP-basiertes Funktionssystem
 * 
 * ## Design-Prinzipien
 * 
 * 1. **Single Responsibility**: Jede Funktion ist eine eigene Klasse
 * 2. **Open/Closed**: Neue Funktionen ohne Änderung bestehenden Codes
 * 3. **Dependency Inversion**: Funktionen hängen von Abstraktionen ab
 * 4. **Plugin-fähig**: Externe Funktionen können registriert werden
 * 
 * ## Architektur
 * 
 * ```
 * ┌─────────────────────────────────────────────────────────────┐
 * │                    FunctionRegistry                         │
 * │  ┌─────────────┬─────────────┬─────────────┬─────────────┐ │
 * │  │ StringFuncs │ MathFuncs   │ ArrayFuncs  │ DateFuncs   │ │
 * │  ├─────────────┼─────────────┼─────────────┼─────────────┤ │
 * │  │ GeoFuncs    │ VectorFuncs │ GraphFuncs  │ DocFuncs    │ │
 * │  └─────────────┴─────────────┴─────────────┴─────────────┘ │
 * └─────────────────────────────────────────────────────────────┘
 * ```
 * 
 * ## Verwendung
 * 
 * ```cpp
 * // Funktion aufrufen
 * auto& registry = FunctionRegistry::instance();
 * auto result = registry.call("UPPER", {jsonString}, context);
 * 
 * // Eigene Funktion registrieren
 * registry.registerFunction("MY_FUNC", std::make_unique<MyFunction>());
 * ```
 */

// Forward declarations
class FunctionContext;
class IFunction;

// ============================================================================
// Function Argument Types
// ============================================================================

/**
 * @brief Argument type constraints for function validation
 */
enum class ArgType {
    ANY,        ///< Any type accepted
    STRING,     ///< String required
    NUMBER,     ///< Number (int or double) required
    INTEGER,    ///< Integer required
    BOOLEAN,    ///< Boolean required
    ARRAY,      ///< Array required
    OBJECT,     ///< Object required
    GEOMETRY,   ///< GeoJSON geometry required
    VECTOR,     ///< Numeric array (vector) required
    DOCUMENT,   ///< Document reference required
    NULLABLE    ///< Can be null
};

/**
 * @brief Function argument specification
 */
struct ArgSpec {
    std::string name;
    ArgType type = ArgType::ANY;
    bool required = true;
    nlohmann::json default_value = nullptr;
    std::string description;
};

/**
 * @brief Function signature for validation and documentation
 */
struct FunctionSignature {
    std::string name;
    std::string category;           ///< String, Math, Array, Date, Geo, Vector, Graph, Document
    std::string description;
    std::vector<ArgSpec> arguments;
    ArgType return_type = ArgType::ANY;
    bool is_deterministic = true;   ///< Same input always produces same output
    bool is_aggregate = false;      ///< Aggregation function (works on multiple rows)
    std::vector<std::string> examples;
};

// ============================================================================
// Function Context
// ============================================================================

/**
 * @brief Execution context passed to functions
 * 
 * Provides access to:
 * - Current document being processed
 * - Variable bindings
 * - Database access for DOCUMENT() etc.
 * - User/session information
 */
class FunctionContext {
public:
    FunctionContext() = default;
    explicit FunctionContext(const nlohmann::json& doc) : current_doc_(doc) {}
    
    // Current document
    const nlohmann::json& currentDocument() const { return current_doc_; }
    void setCurrentDocument(const nlohmann::json& doc) { current_doc_ = doc; }
    
    // Variable bindings
    nlohmann::json getVariable(const std::string& name) const {
        auto it = variables_.find(name);
        return it != variables_.end() ? it->second : nlohmann::json(nullptr);
    }
    void setVariable(const std::string& name, const nlohmann::json& value) {
        variables_[name] = value;
    }
    
    // Database access callback (for DOCUMENT, COLLECTION, etc.)
    using DocumentLoader = std::function<nlohmann::json(const std::string&, const std::string&)>;
    void setDocumentLoader(DocumentLoader loader) { doc_loader_ = std::move(loader); }
    nlohmann::json loadDocument(const std::string& collection, const std::string& key) const {
        if (doc_loader_) return doc_loader_(collection, key);
        throw std::runtime_error("Document loader not configured");
    }
    
    // User context (for permission checks)
    const std::string& userId() const { return user_id_; }
    void setUserId(const std::string& id) { user_id_ = id; }

private:
    nlohmann::json current_doc_;
    std::unordered_map<std::string, nlohmann::json> variables_;
    DocumentLoader doc_loader_;
    std::string user_id_;
};

// ============================================================================
// Function Interface
// ============================================================================

/**
 * @brief Base interface for all AQL functions
 * 
 * Each function must implement:
 * - signature(): Return function metadata
 * - execute(): Perform the actual computation
 */
class IFunction {
public:
    virtual ~IFunction() = default;
    
    /// Get function signature for validation and documentation
    virtual FunctionSignature signature() const = 0;
    
    /// Execute the function with given arguments
    virtual nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const = 0;
    
    /// Validate arguments before execution (optional override)
    virtual void validateArgs(const std::vector<nlohmann::json>& args) const {
        const auto& sig = signature();
        
        // Check required argument count
        size_t requiredCount = 0;
        for (const auto& arg : sig.arguments) {
            if (arg.required) requiredCount++;
        }
        
        if (args.size() < requiredCount) {
            throw std::runtime_error(sig.name + " requires at least " + 
                                     std::to_string(requiredCount) + " arguments");
        }
        
        if (args.size() > sig.arguments.size()) {
            throw std::runtime_error(sig.name + " accepts at most " + 
                                     std::to_string(sig.arguments.size()) + " arguments");
        }
        
        // Type validation
        for (size_t i = 0; i < args.size(); i++) {
            validateArgType(args[i], sig.arguments[i], sig.name);
        }
    }

protected:
    /// Helper for type validation
    static void validateArgType(const nlohmann::json& arg, const ArgSpec& spec, 
                                 const std::string& funcName) {
        if (arg.is_null() && spec.type != ArgType::NULLABLE && spec.type != ArgType::ANY) {
            if (spec.required) {
                throw std::runtime_error(funcName + ": argument '" + spec.name + "' cannot be null");
            }
            return;
        }
        
        bool valid = true;
        switch (spec.type) {
            case ArgType::STRING:   valid = arg.is_string(); break;
            case ArgType::NUMBER:   valid = arg.is_number(); break;
            case ArgType::INTEGER:  valid = arg.is_number_integer(); break;
            case ArgType::BOOLEAN:  valid = arg.is_boolean(); break;
            case ArgType::ARRAY:    valid = arg.is_array(); break;
            case ArgType::OBJECT:   valid = arg.is_object(); break;
            case ArgType::GEOMETRY: valid = isGeometry(arg); break;
            case ArgType::VECTOR:   valid = isVector(arg); break;
            case ArgType::ANY:
            case ArgType::NULLABLE:
            case ArgType::DOCUMENT:
                valid = true; break;
        }
        
        if (!valid) {
            throw std::runtime_error(funcName + ": argument '" + spec.name + 
                                     "' has invalid type");
        }
    }
    
    /// Check if value is a GeoJSON geometry
    static bool isGeometry(const nlohmann::json& val) {
        if (!val.is_object()) return false;
        return val.contains("type") && val.contains("coordinates");
    }
    
    /// Check if value is a numeric vector
    static bool isVector(const nlohmann::json& val) {
        if (!val.is_array()) return false;
        for (const auto& elem : val) {
            if (!elem.is_number()) return false;
        }
        return true;
    }
    
    /// Convert to number helper
    static double toNumber(const nlohmann::json& val) {
        if (val.is_number()) return val.get<double>();
        if (val.is_string()) return std::stod(val.get<std::string>());
        if (val.is_boolean()) return val.get<bool>() ? 1.0 : 0.0;
        throw std::runtime_error("Cannot convert value to number");
    }
    
    /// Convert to string helper
    static std::string toString(const nlohmann::json& val) {
        if (val.is_string()) return val.get<std::string>();
        return val.dump();
    }
    
    /// Convert to bool helper
    static bool toBool(const nlohmann::json& val) {
        if (val.is_boolean()) return val.get<bool>();
        if (val.is_null()) return false;
        if (val.is_number()) return val.get<double>() != 0;
        if (val.is_string()) return !val.get<std::string>().empty();
        if (val.is_array() || val.is_object()) return !val.empty();
        return true;
    }
};

// ============================================================================
// Function Registry (Singleton)
// ============================================================================

/**
 * @brief Central registry for all AQL functions
 * 
 * Thread-safe singleton that manages function registration and lookup.
 */
class FunctionRegistry {
public:
    /// Get singleton instance
    static FunctionRegistry& instance() {
        static FunctionRegistry registry;
        return registry;
    }
    
    /// Register a function
    void registerFunction(std::unique_ptr<IFunction> func) {
        auto sig = func->signature();
        functions_[sig.name] = std::move(func);
    }
    
    /// Check if function exists
    bool hasFunction(const std::string& name) const {
        return functions_.find(name) != functions_.end();
    }
    
    /// Get function (throws if not found)
    const IFunction& getFunction(const std::string& name) const {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw std::runtime_error("Unknown function: " + name);
        }
        return *it->second;
    }
    
    /// Call a function by name
    nlohmann::json call(
        const std::string& name,
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const {
        const auto& func = getFunction(name);
        func.validateArgs(args);
        return func.execute(args, context);
    }
    
    /// Get all function signatures (for documentation)
    std::vector<FunctionSignature> getAllSignatures() const {
        std::vector<FunctionSignature> sigs;
        for (const auto& [name, func] : functions_) {
            sigs.push_back(func->signature());
        }
        return sigs;
    }
    
    /// Get functions by category
    std::vector<FunctionSignature> getByCategory(const std::string& category) const {
        std::vector<FunctionSignature> sigs;
        for (const auto& [name, func] : functions_) {
            auto sig = func->signature();
            if (sig.category == category) {
                sigs.push_back(sig);
            }
        }
        return sigs;
    }
    
    /// List all categories
    std::vector<std::string> getCategories() const {
        std::unordered_map<std::string, bool> cats;
        for (const auto& [name, func] : functions_) {
            cats[func->signature().category] = true;
        }
        std::vector<std::string> result;
        for (const auto& [cat, _] : cats) {
            result.push_back(cat);
        }
        return result;
    }

private:
    FunctionRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<IFunction>> functions_;
};

// ============================================================================
// Function Registration Helpers
// ============================================================================

/**
 * @brief Helper macro for function registration
 */
#define REGISTER_AQL_FUNCTION(FuncClass) \
    FunctionRegistry::instance().registerFunction(std::make_unique<FuncClass>())

/**
 * @brief Initialize all built-in functions
 * Call this at application startup.
 */
void registerBuiltinFunctions();

} // namespace functions
} // namespace query
} // namespace themis
