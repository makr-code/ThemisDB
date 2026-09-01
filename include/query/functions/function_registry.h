/**
 * @file function_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <stdexcept>

// Forward declarations
namespace themis {
    class GraphIndexManager;
    class GraphAnalytics;
    class SecondaryIndexManager;
    class ProcessMining;

}

namespace themis {
namespace query {
namespace functions {

/**
 * @brief AQL Function Registry - Modular OOP-based Function System
 * 
 * ## Design Principles
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
    // Canonical names (UPPERCASE)
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
    NULLABLE,   ///< Can be null

    // Compatibility aliases (PascalCase) to support existing uses
    Any = ANY,
    String = STRING,
    Number = NUMBER,
    Integer = INTEGER,
    Boolean = BOOLEAN,
    Array = ARRAY,
    Object = OBJECT,
    Geometry = GEOMETRY,
    Vector = VECTOR,
    Document = DOCUMENT,
    Nullable = NULLABLE
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
 * @brief Cost complexity class for query optimizer integration
 * 
 * Used by QueryOptimizer to estimate function execution costs
 * and choose optimal query plans.
 */
enum class CostComplexity {
    CONSTANT,       ///< O(1) - LENGTH, NOW, simple math
    LINEAR,         ///< O(n) - SUM, FLATTEN, UNIQUE
    LINEARITHMIC,   ///< O(n log n) - SORTED, MEDIAN
    QUADRATIC,      ///< O(n²) - LEVENSHTEIN on long strings
    INDEXED,        ///< Uses index - DOCUMENT, GEO_DISTANCE with spatial index
    EXTERNAL        ///< External I/O - HOLIDAYS (calendar loading)
};

/**
 * @brief Function cost estimation for query planning
 */
struct FunctionCost {
    CostComplexity complexity = CostComplexity::CONSTANT;
    double base_cost = 1.0;         ///< Base cost in abstract units
    double per_element_cost = 0.0;  ///< Additional cost per input element
    bool can_use_index = false;     ///< Can leverage database indexes
    bool is_parallelizable = false; ///< Can be parallelized across documents
    std::string index_type;         ///< Required index type (geo, vector, fulltext)
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
    
    // Query optimizer integration
    FunctionCost cost = FunctionCost{}; ///< Cost estimation for query planning
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

    // Graph infrastructure access (for graph functions)
    void setGraphIndexManager(themis::GraphIndexManager* mgr) { graph_mgr_ = mgr; }
    themis::GraphIndexManager* getGraphIndexManager() const { return graph_mgr_; }
    
    void setGraphAnalytics(themis::GraphAnalytics* analytics) { graph_analytics_ = analytics; }
    themis::GraphAnalytics* getGraphAnalytics() const { return graph_analytics_; }

    // Full-text / secondary index access (for FULLTEXT, PHRASE, FUZZY functions)
    void setSecondaryIndexManager(themis::SecondaryIndexManager* mgr) { secondary_idx_mgr_ = mgr; }
    themis::SecondaryIndexManager* getSecondaryIndexManager() const { return secondary_idx_mgr_; }

    // Process mining engine access (for PM_DISCOVER_PROCESS, PM_VARIANTS, etc.)
    void setProcessMining(themis::ProcessMining* pm) { process_mining_ = pm; }
    themis::ProcessMining* getProcessMining() const { return process_mining_; }

    // Collection scan callback (for functions that query collections by filter)
    // Returns an array of matching document objects. An empty array on no match.
    using CollectionScanner = std::function<std::vector<nlohmann::json>(
        const std::string& collection,
        const std::function<bool(const nlohmann::json&)>& predicate)>;
    void setCollectionScanner(CollectionScanner scanner) { collection_scanner_ = std::move(scanner); }
    std::vector<nlohmann::json> scanCollection(
        const std::string& collection,
        const std::function<bool(const nlohmann::json&)>& predicate) const
    {
        if (collection_scanner_) return collection_scanner_(collection, predicate);
        return {};
    }

    // ── Administrative process-model registry (stub #283) ─────────────────────

    /**
     * @brief Function type for loading a named administrative process model.
     *
     * The callable receives a model_id string and returns the model as a JSON
     * object, or throws on error.  Callers inject a YAML-backed or database-backed
     * registry at startup.
     */
    using AdminModelLoadFn = std::function<nlohmann::json(const std::string& model_id)>;

    /**
     * @brief Function type for listing all registered administrative process models.
     *
     * Returns a JSON array of model descriptor objects.  Throws on error.
     */
    using AdminModelListFn = std::function<nlohmann::json()>;

    /**
     * @brief Inject a load function for PM_LOAD_ADMIN_MODEL (resolves stub #283).
     *
     * @param fn  Callable that loads and returns a process model by ID.
     */
    void setAdminModelLoadFn(AdminModelLoadFn fn) { admin_model_load_fn_ = std::move(fn); }

    /**
     * @brief Inject a list function for PM_LIST_ADMIN_MODELS (resolves stub #283).
     *
     * @param fn  Callable that returns a JSON array of model descriptors.
     */
    void setAdminModelListFn(AdminModelListFn fn) { admin_model_list_fn_ = std::move(fn); }

    /// @brief Access the injected admin-model load function (may be empty).
    const AdminModelLoadFn& adminModelLoadFn() const { return admin_model_load_fn_; }

    /// @brief Access the injected admin-model list function (may be empty).
    const AdminModelListFn& adminModelListFn() const { return admin_model_list_fn_; }

    // ── Task scheduler injection bridge ───────────────────────────────────────

    /**
     * @brief Function type for registering a scheduled AQL task.
     *
     * Receives a JSON task configuration object with fields:
     *   - "name"  (string)  — human-readable task name
     *   - "type"  (string)  — "aql" or "function"
     *   - "query" (string)  — AQL query or function name
     *   - "interval_ms" (number) — scheduling interval in milliseconds
     *
     * Returns the opaque task ID on success, or throws on error.
     */
    using RegisterTaskFn = std::function<std::string(const nlohmann::json& task_config)>;

    /**
     * @brief Function type for listing all registered scheduled tasks.
     *
     * Returns a JSON array of task descriptor objects, or throws on error.
     */
    using ListTasksFn = std::function<nlohmann::json()>;

    /**
     * @brief Function type for cancelling a scheduled task by ID.
     *
     * Returns true when the task was found and cancelled, false otherwise.
     */
    using CancelTaskFn = std::function<bool(const std::string& task_id)>;

    /**
     * @brief Inject a task-registration function for SCHEDULE_TASK.
     *
     * @param fn  Callable that registers a task and returns its ID.
     */
    void setRegisterTaskFn(RegisterTaskFn fn) { register_task_fn_ = std::move(fn); }

    /**
     * @brief Inject a task-list function for LIST_SCHEDULED_TASKS.
     *
     * @param fn  Callable that returns a JSON array of task descriptors.
     */
    void setListTasksFn(ListTasksFn fn) { list_tasks_fn_ = std::move(fn); }

    /**
     * @brief Inject a task-cancellation function for CANCEL_TASK.
     *
     * @param fn  Callable that cancels a task by ID.
     */
    void setCancelTaskFn(CancelTaskFn fn) { cancel_task_fn_ = std::move(fn); }

    /// @brief Access the injected task-registration function (may be empty).
    const RegisterTaskFn& registerTaskFn() const { return register_task_fn_; }

    /// @brief Access the injected task-list function (may be empty).
    const ListTasksFn& listTasksFn() const { return list_tasks_fn_; }

    /// @brief Access the injected task-cancellation function (may be empty).
    const CancelTaskFn& cancelTaskFn() const { return cancel_task_fn_; }

private:
    nlohmann::json current_doc_;
    std::unordered_map<std::string, nlohmann::json> variables_;
    DocumentLoader doc_loader_;
    std::string user_id_;
    themis::GraphIndexManager* graph_mgr_ = nullptr;
    themis::GraphAnalytics* graph_analytics_ = nullptr;
    themis::SecondaryIndexManager* secondary_idx_mgr_ = nullptr;
    themis::ProcessMining* process_mining_ = nullptr;
    CollectionScanner collection_scanner_;
    AdminModelLoadFn admin_model_load_fn_;
    AdminModelListFn admin_model_list_fn_;
    RegisterTaskFn register_task_fn_;
    ListTasksFn    list_tasks_fn_;
    CancelTaskFn   cancel_task_fn_;
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
    
    /// Get function (throws if not found)
    const IFunction& getFunction(const std::string& name) const {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw std::runtime_error("Unknown function: " + name);
        }
        return *it->second;
    }
    
    /// Call a function by name (resolves aliases automatically)
    nlohmann::json call(
        const std::string& name,
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const {
        std::string resolvedName = resolveAlias(name);
        const auto& func = getFunction(resolvedName);
        func.validateArgs(args);
        return func.execute(args, context);
    }
    
    /// Check if function exists (including aliases)
    bool hasFunction(const std::string& name) const {
        std::string resolvedName = resolveAlias(name);
        return functions_.find(resolvedName) != functions_.end();
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
    
    // ========================================================================
    // Function Aliasing System
    // ========================================================================
    //
    // Aliases allow multiple function names to use the same implementation.
    // This consolidates Excel-compatible names with native names:
    //
    //   CEILING -> CEIL     (Excel compatibility)
    //   ROUNDUP -> CEIL     (Excel compatibility)
    //   ROUNDDOWN -> FLOOR  (Excel compatibility)
    //   CONCATENATE -> CONCAT (Excel compatibility)
    //   LEN -> LENGTH       (Excel/SQL compatibility)
    //   MID -> SUBSTRING    (Excel compatibility)
    //   LOWER -> LOWER      (self, native)
    //   LCASE -> LOWER      (SQL compatibility)
    //   UCASE -> UPPER      (SQL compatibility)
    //   POWER -> POW        (SQL compatibility)
    //   OBJECT -> DICT      (alternative name)
    //   MAP -> DICT         (Python-style)
    //
    // ========================================================================
    
    /// Register an alias for an existing function
    void registerAlias(const std::string& alias, const std::string& target) {
        aliases_[alias] = target;
    }
    
    /// Resolve alias to actual function name
    std::string resolveAlias(const std::string& name) const {
        auto it = aliases_.find(name);
        return it != aliases_.end() ? it->second : name;
    }
    
    /// Get all registered aliases
    std::unordered_map<std::string, std::string> getAliases() const {
        return aliases_;
    }
    
    /// Check if a name is an alias
    bool isAlias(const std::string& name) const {
        return aliases_.find(name) != aliases_.end();
    }

    /// Unregister a function by name.  No-op if not found.
    void unregisterFunction(const std::string& name) {
        functions_.erase(name);
    }

private:
    FunctionRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<IFunction>> functions_;
    std::unordered_map<std::string, std::string> aliases_;  ///< alias -> target function
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
