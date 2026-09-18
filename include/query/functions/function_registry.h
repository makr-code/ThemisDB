/**
 * @file function_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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


// Forward declarations
class FunctionContext;
class IFunction;

// ============================================================================
// Function Argument Types
// ============================================================================

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

struct ArgSpec {
    std::string name;
    ArgType type = ArgType::ANY;
    bool required = true;
    nlohmann::json default_value = nullptr;
    std::string description;
};

enum class CostComplexity {
    CONSTANT,       ///< O(1) - LENGTH, NOW, simple math
    LINEAR,         ///< O(n) - SUM, FLATTEN, UNIQUE
    LINEARITHMIC,   ///< O(n log n) - SORTED, MEDIAN
    QUADRATIC,      ///< O(n²) - LEVENSHTEIN on long strings
    INDEXED,        ///< Uses index - DOCUMENT, GEO_DISTANCE with spatial index
    EXTERNAL        ///< External I/O - HOLIDAYS (calendar loading)
};

struct FunctionCost {
    CostComplexity complexity = CostComplexity::CONSTANT;
    double base_cost = 1.0;         ///< Base cost in abstract units
    double per_element_cost = 0.0;  ///< Additional cost per input element
    bool can_use_index = false;     ///< Can leverage database indexes
    bool is_parallelizable = false; ///< Can be parallelized across documents
    std::string index_type;         ///< Required index type (geo, vector, fulltext)
};

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

class FunctionContext {
public:
    FunctionContext() = default;
    explicit FunctionContext(const nlohmann::json& doc) : current_doc_(doc) {}
    
    // Current document
    const nlohmann::json& currentDocument() const { return current_doc_; }
    /**
     * @brief Set Current Document.
     * @param[in] doc Input parameter.
     * @details Implements setCurrentDocument without additional internal calls.
     */
    void setCurrentDocument(const nlohmann::json& doc) { current_doc_ = doc; }
    
    // Variable bindings
    nlohmann::json getVariable(const std::string& name) const {
        auto it = variables_.find(name);
        return it != variables_.end() ? it->second : nlohmann::json(nullptr);
    }
    /**
     * @brief Set Variable.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     * @details Implements setVariable without additional internal calls.
     */
    void setVariable(const std::string& name, const nlohmann::json& value) {
        variables_[name] = value;
    }
    
    // Database access callback (for DOCUMENT, COLLECTION, etc.)
    using DocumentLoader = std::function<nlohmann::json(const std::string&, const std::string&)>;
    /**
     * @brief Set Document Loader.
     * @param[in] loader Input parameter.
     * @details Calls: std::move().
     */
    void setDocumentLoader(DocumentLoader loader) { doc_loader_ = std::move(loader); }
    nlohmann::json loadDocument(const std::string& collection, const std::string& key) const {
        if (doc_loader_) {
          return doc_loader_(collection, key);
        }
        throw std::runtime_error("Document loader not configured");
    }
    
    // User context (for permission checks)
    const std::string& userId() const { return user_id_; }
    /**
     * @brief Set User Id.
     * @param[in] id Input parameter.
     * @details Implements setUserId without additional internal calls.
     */
    void setUserId(const std::string& id) { user_id_ = id; }

    /**
     * @brief Set Graph Index Manager.
     * @param[in,out] mgr Input/output parameter.
     * @details Implements setGraphIndexManager without additional internal calls.
     */
    void setGraphIndexManager(themis::GraphIndexManager* mgr) { graph_mgr_ = mgr; }
    themis::GraphIndexManager* getGraphIndexManager() const { return graph_mgr_; }
    
    /**
     * @brief Set Graph Analytics.
     * @param[in,out] analytics Input/output parameter.
     * @details Implements setGraphAnalytics without additional internal calls.
     */
    void setGraphAnalytics(themis::GraphAnalytics* analytics) { graph_analytics_ = analytics; }
    themis::GraphAnalytics* getGraphAnalytics() const { return graph_analytics_; }

    /**
     * @brief Set Secondary Index Manager.
     * @param[in,out] mgr Input/output parameter.
     * @details Implements setSecondaryIndexManager without additional internal calls.
     */
    void setSecondaryIndexManager(themis::SecondaryIndexManager* mgr) { secondary_idx_mgr_ = mgr; }
    themis::SecondaryIndexManager* getSecondaryIndexManager() const { return secondary_idx_mgr_; }

    /**
     * @brief Set Process Mining.
     * @param[in,out] pm Input/output parameter.
     * @details Implements setProcessMining without additional internal calls.
     */
    void setProcessMining(themis::ProcessMining* pm) { process_mining_ = pm; }
    themis::ProcessMining* getProcessMining() const { return process_mining_; }

    // Collection scan callback (for functions that query collections by filter)
    // Returns an array of matching document objects. An empty array on no match.
    using CollectionScanner = std::function<std::vector<nlohmann::json>(
        const std::string& collection,
        const std::function<bool(const nlohmann::json&)>& predicate)>;
    /**
     * @brief Set Collection Scanner.
     * @param[in] scanner Input parameter.
     * @details Calls: std::move().
     */
    void setCollectionScanner(CollectionScanner scanner) { collection_scanner_ = std::move(scanner); }
    std::vector<nlohmann::json> scanCollection(
        const std::string& collection,
        const std::function<bool(const nlohmann::json&)>& predicate) const
    {
        if (collection_scanner_) {
          return collection_scanner_(collection, predicate);
        }
        return {};
    }

    // ── Administrative process-model registry (stub #283) ─────────────────────

    using AdminModelLoadFn = std::function<nlohmann::json(const std::string& model_id)>;

    using AdminModelListFn = std::function<nlohmann::json()>;

    /**
     * @brief Set Admin Model Load Fn.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setAdminModelLoadFn(AdminModelLoadFn fn) { admin_model_load_fn_ = std::move(fn); }

    /**
     * @brief Set Admin Model List Fn.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setAdminModelListFn(AdminModelListFn fn) { admin_model_list_fn_ = std::move(fn); }

    const AdminModelLoadFn& adminModelLoadFn() const { return admin_model_load_fn_; }

    const AdminModelListFn& adminModelListFn() const { return admin_model_list_fn_; }

    // ── Task scheduler injection bridge ───────────────────────────────────────

    using RegisterTaskFn = std::function<std::string(const nlohmann::json& task_config)>;

    using ListTasksFn = std::function<nlohmann::json()>;

    using CancelTaskFn = std::function<bool(const std::string& task_id)>;

    /**
     * @brief Set Register Task Fn.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setRegisterTaskFn(RegisterTaskFn fn) { register_task_fn_ = std::move(fn); }

    /**
     * @brief Set List Tasks Fn.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setListTasksFn(ListTasksFn fn) { list_tasks_fn_ = std::move(fn); }

    /**
     * @brief Set Cancel Task Fn.
     * @param[in] fn Input parameter.
     * @details Calls: std::move().
     */
    void setCancelTaskFn(CancelTaskFn fn) { cancel_task_fn_ = std::move(fn); }

    const RegisterTaskFn& registerTaskFn() const { return register_task_fn_; }

    const ListTasksFn& listTasksFn() const { return list_tasks_fn_; }

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

class IFunction {
public:
    /**
     * @brief IFunction.
     * @return Return value.
     */
    virtual ~IFunction() = default;
    
    /**
     * @brief Signature.
     * @return Return value.
     */
    virtual FunctionSignature signature() const = 0;
    
    /**
     * @brief Execute.
     * @param[in] args Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
     */
    virtual nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& context
    ) const = 0;
    
    virtual void validateArgs(const std::vector<nlohmann::json>& args) const {
        const auto& sig = signature();
        
        // Check required argument count
        size_t requiredCount = 0;
        for (const auto& arg : sig.arguments) {
            if (arg.required) {
              requiredCount++;
            }
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
    /**
     * @brief Validate Arg Type.
     * @param[in] arg Input parameter.
     * @param[in] spec Input parameter.
     * @param[in] funcName Input parameter.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: is_null(), is_string(), is_number(), is_number_integer(), is_boolean(), is_array(), is_object(), isGeometry().
     */
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
    
    /**
     * @brief Is Geometry.
     * @param[in] val Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: is_object(), contains().
     */
    static bool isGeometry(const nlohmann::json& val) {
        if (!val.is_object()) {
          return false;
        }
        return val.contains("type") && val.contains("coordinates");
    }
    
    /**
     * @brief Is Vector.
     * @param[in] val Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: is_array(), is_number().
     */
    static bool isVector(const nlohmann::json& val) {
        if (!val.is_array()) {
          return false;
        }
        for (const auto& elem : val) {
            if (!elem.is_number()) {
              return false;
            }
        }
        return true;
    }
    
    /**
     * @brief To Number.
     * @param[in] val Input parameter.
     * @return Return value.
     * @throws std::runtime_error if an error occurs.
     * @details Calls: is_number(), is_string(), std::stod(), is_boolean().
     */
    static double toNumber(const nlohmann::json& val) {
        if (val.is_number()) {
          return val.get<double>();
        }
        if (val.is_string()) {
          return std::stod(val.get<std::string>());
        }
        if (val.is_boolean()) {
          return val.get<bool>() ? 1.0 : 0.0;
        }
        throw std::runtime_error("Cannot convert value to number");
    }
    
    /**
     * @brief To String.
     * @param[in] val Input parameter.
     * @return Return value.
     * @details Calls: is_string(), dump().
     */
    static std::string toString(const nlohmann::json& val) {
        if (val.is_string()) {
          return val.get<std::string>();
        }
        return val.dump();
    }
    
    /**
     * @brief To Bool.
     * @param[in] val Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: is_boolean(), is_null(), is_number(), is_string(), empty(), is_array(), is_object().
     */
    static bool toBool(const nlohmann::json& val) {
        if (val.is_boolean()) {
          return val.get<bool>();
        }
        if (val.is_null()) {
          return false;
        }
        if (val.is_number()) {
          return val.get<double>() != 0;
        }
        if (val.is_string()) {
          return !val.get<std::string>().empty();
        }
        if (val.is_array() || val.is_object()) {
          return !val.empty();
        }
        return true;
    }
};

// ============================================================================
// Function Registry (Singleton)
// ============================================================================

class FunctionRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static FunctionRegistry& instance() {
        static FunctionRegistry registry;
        return registry;
    }
    
    /**
     * @brief Register Function.
     * @param[in] func Input parameter.
     * @details Calls: signature(), std::move().
     */
    void registerFunction(std::unique_ptr<IFunction> func) {
        auto sig = func->signature();
        functions_[sig.name] = std::move(func);
    }
    
    const IFunction& getFunction(const std::string& name) const {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw std::runtime_error("Unknown function: " + name);
        }
        return *it->second;
    }
    
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
    
    bool hasFunction(const std::string& name) const {
        std::string resolvedName = resolveAlias(name);
        return functions_.find(resolvedName) != functions_.end();
    }
    
    std::vector<FunctionSignature> getAllSignatures() const {
        std::vector<FunctionSignature> sigs = {};

        for (const auto& [name, func] : functions_) {
            sigs.push_back(func->signature());
        }
        return sigs;
    }
    
    std::vector<FunctionSignature> getByCategory(const std::string& category) const {
        std::vector<FunctionSignature> sigs = {};

        for (const auto& [name, func] : functions_) {
            auto sig = func->signature();
            if (sig.category == category) {
                sigs.push_back(sig);
            }
        }
        return sigs;
    }
    
    std::vector<std::string> getCategories() const {
        std::unordered_map<std::string, bool> cats = {};

        for (const auto& [name, func] : functions_) {
            cats[func->signature().category] = true;
        }
        std::vector<std::string> result = {};

        for (const auto& [cat, _] : cats) {
            result.push_back(cat);
        }
        return result;
    }
    
    /**
     * @brief ======================================================================== Function Aliasing System ======================================================================== Aliases allow multiple function names to use the same implementation.
     * @param[in] alias Input parameter.
     * @param[in] target Input parameter.
     * @details This consolidates Excel-compatible names with native names: CEILING -> CEIL (Excel compatibility) ROUNDUP -> CEIL (Excel compatibility) ROUNDDOWN -> FLOOR (Excel compatibility) CONCATENATE -> CONCAT (Excel compatibility) LEN -> LENGTH (Excel/SQL compatibility) MID -> SUBSTRING (Excel compatibility) LOWER -> LOWER (self, native) LCASE -> LOWER (SQL compatibility) UCASE -> UPPER (SQL compatibility) POWER -> POW (SQL compatibility) OBJECT -> DICT (alternative name) MAP -> DICT (Python-style) ======================================================================== Implements registerAlias without additional internal calls.
     */
    
    void registerAlias(const std::string& alias, const std::string& target) {
        aliases_[alias] = target;
    }
    
    std::string resolveAlias(const std::string& name) const {
        auto it = aliases_.find(name);
        return it != aliases_.end() ? it->second : name;
    }
    
    std::unordered_map<std::string, std::string> getAliases() const {
        return aliases_;
    }
    
    bool isAlias(const std::string& name) const {
        return aliases_.find(name) != aliases_.end();
    }

    /**
     * @brief Unregister Function.
     * @param[in] name Input parameter.
     * @details Calls: erase().
     */
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

#define REGISTER_AQL_FUNCTION(FuncClass) \
    FunctionRegistry::instance().registerFunction(std::make_unique<FuncClass>())

/**
 * @brief Register Builtin Functions.
 */
void registerBuiltinFunctions();

} // namespace functions
} // namespace query
} // namespace themis
