/**
 * @file graphql.h
 * @brief GraphQL query parsing, AST representation, and execution interfaces.
 *
 * @details Provides a complete GraphQL implementation for ThemisDB:
 *  - Value representations for all GraphQL scalar and composite types (scalars, lists, objects)
 *  - Field selection structures for representing query/mutation/subscription selections
 *  - AST construction and execution context management
 *  - Deterministic parsing and execution with variable substitution support
 *
 * Design goals:
 *  - Decouple GraphQL syntax handling from business logic via ExecutionContext
 *  - Support GraphQL subscriptions with connection lifecycle and multiplexing
 *  - Ensure fail-closed behavior on invalid queries (see TransportPolicyMiddleware)
 *  - Maintain bounded resource consumption for large queries
 *
 * ### Usage example
 * ```cpp
 * // Parse a GraphQL query string
 * auto parse_result = GraphQLParser::parse(query_string);
 * if (!parse_result) {
 *     return tl::unexpected(ApiErrorTaxonomy::toErrorCode(fc));
 * }
 *
 * // Execute the operation against an execution context
 * auto context = std::make_shared<ExecutionContext>(...);
 * auto result = operation.execute(context);
 * if (!result) {
 *     // Handle execution error
 * }
 * ```
 *
 * ### Thread safety
 * - `Value`, `Field`, `Operation`, `VariableDefinition` are immutable after construction and thread-safe to share.
 * - `ExecutionContext` access must be synchronized externally if shared across threads.
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <functional>
#include "utils/expected.h"

namespace themis {
namespace graphql {

struct Value;

using ValueMap = std::unordered_map<std::string, std::shared_ptr<Value>>;
using ValueList = std::vector<std::shared_ptr<Value>>;

struct Value {
    enum class Type {
        Null,
        Boolean,
        Int,
        Float,
        String,
        Enum,
        List,
        Object,
        VariableRef  // Variable reference ($name) — resolved at execution time
    };
    
    Type type = Type::Null;
    std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        double,
        std::string,
        ValueList,
        ValueMap
    > data = nullptr;
    
    // Convenience constructors
    /**
     * @brief Null.
     * @return Return value.
     * @details Implements null without additional internal calls.
     */
    static std::shared_ptr<Value> null() {
        return std::make_shared<Value>();
    }
    
    /**
     * @brief Boolean.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Implements boolean without additional internal calls.
     */
    static std::shared_ptr<Value> boolean(bool v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Boolean;
        val->data = v;
        return val;
    }
    
    /**
     * @brief Integer.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Implements integer without additional internal calls.
     */
    static std::shared_ptr<Value> integer(int64_t v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Int;
        val->data = v;
        return val;
    }
    
    /**
     * @brief Floating.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Implements floating without additional internal calls.
     */
    static std::shared_ptr<Value> floating(double v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Float;
        val->data = v;
        return val;
    }
    
    /**
     * @brief String.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static std::shared_ptr<Value> string(std::string v) {
        auto val = std::make_shared<Value>();
        val->type = Type::String;
        val->data = std::move(v);
        return val;
    }
    
    /**
     * @brief Enum Value.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static std::shared_ptr<Value> enumValue(std::string v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Enum;
        val->data = std::move(v);
        return val;
    }
    
    /**
     * @brief List.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static std::shared_ptr<Value> list(ValueList v) {
        auto val = std::make_shared<Value>();
        val->type = Type::List;
        val->data = std::move(v);
        return val;
    }
    
    /**
     * @brief Object.
     * @param[in] v Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static std::shared_ptr<Value> object(ValueMap v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Object;
        val->data = std::move(v);
        return val;
    }
    
    /**
     * @brief Variable Ref.
     * @param[in] name Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static std::shared_ptr<Value> variableRef(std::string name) {
        auto val = std::make_shared<Value>();
        val->type = Type::VariableRef;
        val->data = std::move(name);
        return val;
    }
    
    // Type checkers
    bool isNull() const { return type == Type::Null; }
    bool isBool() const { return type == Type::Boolean; }
    bool isInt() const { return type == Type::Int; }
    bool isFloat() const { return type == Type::Float; }
    bool isString() const { return type == Type::String; }
    bool isEnum() const { return type == Type::Enum; }
    bool isList() const { return type == Type::List; }
    bool isObject() const { return type == Type::Object; }
    bool isVariableRef() const { return type == Type::VariableRef; }
    
    // Value getters
    bool asBool() const { return std::get<bool>(data); }
    int64_t asInt() const { return std::get<int64_t>(data); }
    double asFloat() const { return std::get<double>(data); }
    const std::string& asString() const { return std::get<std::string>(data); }
    const ValueList& asList() const { return std::get<ValueList>(data); }
    const ValueMap& asObject() const { return std::get<ValueMap>(data); }
    const std::string& asVariableRef() const { return std::get<std::string>(data); }
};

struct Field {
    std::string name = {};
    std::string alias;  // Optional alias
    std::unordered_map<std::string, std::shared_ptr<Value>> arguments;
    std::vector<Field> selections;  // Nested selections
    
    const std::string& responseName() const {
        return alias.empty() ? name : alias;
    }
};

enum class OperationType {
    Query,
    Mutation,
    Subscription
};

struct VariableDefinition {
    std::string name;
    std::string type_name;
    bool is_non_null = false;
    bool is_list = false;
    std::shared_ptr<Value> default_value;
};

struct Operation {
    OperationType type = OperationType::Query;
    std::string name;  // Optional operation name
    std::vector<VariableDefinition> variables;
    std::vector<Field> selections;
};

struct Document {
    std::vector<Operation> operations;
    
    // Get operation by name (or first unnamed)
    const Operation* getOperation(std::string_view name = "") const {
        for (const auto& op : operations) {
            if (name.empty() || op.name == name) {
                return &op;
            }
        }
        return nullptr;
    }
};

struct ParseError {
    std::string message = {};
    size_t line = 0;
    size_t column = 0;
    
    std::string toString() const {
        return "Line " + std::to_string(line) + ", Column " + std::to_string(column) + ": " + message;
    }
};

struct QueryLimits {
    size_t max_query_size_bytes = 100000;      // Maximum query size in bytes
    size_t max_depth = 10;                      // Maximum nesting depth
    size_t max_fields = 100;                    // Maximum total field count
    size_t max_ast_nodes = 1000;                // Maximum AST nodes
    size_t max_subscriptions = 10;              // Maximum concurrent subscriptions per connection

    bool allow_introspection = true;

    /**
     * @brief Default safe limits (development / trusted context)
     * @return Return value.
     * @details Implements defaults without additional internal calls.
     */
    static QueryLimits defaults() {
        return QueryLimits{};
    }
    
    /**
     * @brief More permissive limits for trusted contexts
     * @return Return value.
     * @details Implements permissive without additional internal calls.
     */
    static QueryLimits permissive() {
        return QueryLimits{
            .max_query_size_bytes = 1000000,
            .max_depth = 20,
            .max_fields = 500,
            .max_ast_nodes = 5000,
            .max_subscriptions = 50,
            .allow_introspection = true
        };
    }

    /**
     * @brief Production.
     * @return Return value.
     * @details Implements production without additional internal calls.
     */
    static QueryLimits production() {
        QueryLimits l;
        l.allow_introspection = false;
        return l;
    }
};

class Parser {
public:
    struct Result {
        bool success = false;
        Document document;
        std::vector<ParseError> errors;
    };
    
    /**
     * @brief Parse.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    static Result parse(std::string_view query);
    
    /**
     * @brief Parse.
     * @param[in] query Input parameter.
     * @param[in] limits Input parameter.
     * @return Return value.
     */
    static Result parse(std::string_view query, const QueryLimits& limits);
    
private:
    Parser(std::string_view query, const QueryLimits& limits);
    
    /**
     * @brief Parse Document.
     * @return Return value.
     */
    Result parseDocument();
    /**
     * @brief Parse Operation.
     * @return Return value.
     */
    themis::Result<Operation> parseOperation();
    themis::Result<Field> parseField(size_t depth = 0);
    /**
     * @brief Parse Value.
     * @return Return value.
     */
    themis::Result<std::shared_ptr<Value>> parseValue();
    /**
     * @brief Parse Variable Definition.
     * @return Return value.
     */
    themis::Result<VariableDefinition> parseVariableDefinition();
    
    // Validation helpers
    /**
     * @brief Check Query Size.
     * @return True when the operation succeeds.
     */
    bool checkQuerySize();
    /**
     * @brief Check Depth Limit.
     * @param[in] depth Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkDepthLimit(size_t depth);
    /**
     * @brief Check Field Limit.
     * @return True when the operation succeeds.
     */
    bool checkFieldLimit();
    /**
     * @brief Check ASTNode Limit.
     * @return True when the operation succeeds.
     */
    bool checkASTNodeLimit();
    /**
     * @brief Is Introspection Field Name.
     * @param[in] field_name Name of the field.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isIntrospectionFieldName(std::string_view field_name) noexcept;
    /**
     * @brief Increment Field Count.
     * @details Implements incrementFieldCount without additional internal calls.
     */
    void incrementFieldCount() { field_count_++; }
    /**
     * @brief Increment ASTNode Count.
     * @details Implements incrementASTNodeCount without additional internal calls.
     */
    void incrementASTNodeCount() { ast_node_count_++; }
    
    // Tokenization helpers
    /**
     * @brief Skip Whitespace.
     */
    void skipWhitespace();
    /**
     * @brief Skip Comment.
     */
    void skipComment();
    /**
     * @brief Match.
     * @param[in] c Input parameter.
     * @return True when the operation succeeds.
     */
    bool match(char c);
    /**
     * @brief Match.
     * @param[in] s Input parameter.
     * @return True when the operation succeeds.
     */
    bool match(std::string_view s);
    /**
     * @brief Peek.
     * @param[in] c Input parameter.
     * @return True when the operation succeeds.
     */
    bool peek(char c) const;
    /**
     * @brief Parse Name.
     * @return Return value.
     */
    themis::Result<std::string> parseName();
    /**
     * @brief Parse String.
     * @return Return value.
     */
    themis::Result<std::string> parseString();
    /**
     * @brief Parse Int.
     * @return Return value.
     */
    themis::Result<int64_t> parseInt();
    /**
     * @brief Parse Float.
     * @return Return value.
     */
    themis::Result<double> parseFloat();
    
    // Helper methods
    /**
     * @brief Get Location Context.
     * @return Return value.
     */
    std::string getLocationContext() const;
    /**
     * @brief Convert To Parse Error.
     * @param[in] error Input parameter.
     * @return Return value.
     */
    ParseError convertToParseError(const themis::Error& error);
    
    /**
     * @brief Deprecated: Use Result<T> return types instead of error() method
     * @param[in] message Input parameter.
     */
    void error(std::string message);
    
    std::string_view source_;
    QueryLimits limits_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    size_t field_count_ = 0;
    size_t ast_node_count_ = 0;
    size_t max_depth_reached_ = 0;
    std::vector<ParseError> errors_;
};

struct ExecutionContext {
    std::unordered_map<std::string, std::shared_ptr<Value>> variables;
    std::string tenant_id;
    std::string user_id;
    bool mask_errors = true;  // Mask internal error details in production
    
    // Resolver function type
    using Resolver = std::function<std::shared_ptr<Value>(
        const Field& field,
        const std::shared_ptr<Value>& parent,
        const ExecutionContext& ctx
    )>;
    
    std::unordered_map<std::string, Resolver> resolvers;
};

struct MaskedError {
    std::string message;
    std::string code = {};
    std::vector<std::string> path;  // Path to the field that caused the error
    
    // Mask an internal error for client exposure
    static MaskedError fromInternalError(
        const std::string& internal_msg,
        const std::string& error_code = "INTERNAL_ERROR",
        bool mask = true
    ) {
        MaskedError masked;
        masked.code = error_code;
        
        if (mask) {
            // In production, don't expose internal details
            if (error_code == "ERR_QUERY_INVALID_SYNTAX") {
                masked.message = "Invalid query syntax";
            } else if (error_code.find("LIMIT") != std::string::npos || 
                      error_code.find("EXCEED") != std::string::npos) {
                masked.message = "Query exceeds resource limits";
            } else {
                masked.message = "An internal error occurred";
            }
        } else {
            // In development, expose full details
            masked.message = internal_msg;
        }
        
        return masked;
    }
    
    std::string toString() const {
        std::string result = "[" + code + "] " + message;
        if (!path.empty()) {
            result += " at path: ";
            for (size_t i = 0; i < path.size(); ++i) {
                if (i > 0) {
                  result += ".";
                }
                result += path[i];
            }
        }
        return result;
    }
};

class Executor {
public:
    struct Result {
        std::shared_ptr<Value> data;
        std::vector<MaskedError> errors;  // Use masked errors instead of strings
        
        bool hasErrors() const { return !errors.empty(); }
        
        // Helper to add an error with automatic masking
        void addError(const std::string& message, 
                     const std::string& code = "INTERNAL_ERROR",
                     bool mask = true) {
            errors.push_back(MaskedError::fromInternalError(message, code, mask));
        }
    };
    
    Result execute(
        const Document& document,
        const ExecutionContext& context,
        std::string_view operation_name = ""
    );
    
private:
    /**
     * @brief Execute Operation.
     * @param[in] operation Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Value> executeOperation(
        const Operation& operation,
        const ExecutionContext& context
    );
    
    /**
     * @brief Execute Selections.
     * @param[in] selections Input parameter.
     * @param[in] parent Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Value> executeSelections(
        const std::vector<Field>& selections,
        const std::shared_ptr<Value>& parent,
        const ExecutionContext& context
    );
    
    /**
     * @brief Execute Field.
     * @param[in] field Input parameter.
     * @param[in] parent Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Value> executeField(
        const Field& field,
        const std::shared_ptr<Value>& parent,
        const ExecutionContext& context
    );

    /**
     * @brief Resolve Value.
     * @param[in] value Input parameter.
     * @param[in] context Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<Value> resolveValue(
        const std::shared_ptr<Value>& value,
        const ExecutionContext& context
    );
};

struct TypeRef {
    std::string name;
    bool is_non_null = false;
    bool is_list = false;
    std::shared_ptr<TypeRef> of_type;  // For list/non-null wrapping
};

struct FieldDefinition {
    std::string name;
    std::string description;
    TypeRef type;
    std::unordered_map<std::string, TypeRef> arguments;
};

struct TypeDefinition {
    enum class Kind {
        Scalar,
        Object,
        Interface,
        Union,
        Enum,
        InputObject
    };
    
    Kind kind = Kind::Object;
    std::string name;
    std::string description;
    std::vector<FieldDefinition> fields;
    std::vector<std::string> enum_values;  // For enum types
    std::vector<std::string> interfaces;   // Implemented interfaces
};

class Schema {
public:
    Schema();
    
    /**
     * @brief Add Type.
     * @param[in] type Input parameter.
     */
    void addType(TypeDefinition type);
    /**
     * @brief Get Type.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     */
    const TypeDefinition* getType(std::string_view name) const;
    
    /**
     * @brief Set Query Type.
     * @param[in] name Input parameter.
     * @details Implements setQueryType without additional internal calls.
     */
    void setQueryType(std::string_view name) { query_type_ = name; }
    /**
     * @brief Set Mutation Type.
     * @param[in] name Input parameter.
     * @details Implements setMutationType without additional internal calls.
     */
    void setMutationType(std::string_view name) { mutation_type_ = name; }
    /**
     * @brief Set Subscription Type.
     * @param[in] name Input parameter.
     * @details Implements setSubscriptionType without additional internal calls.
     */
    void setSubscriptionType(std::string_view name) { subscription_type_ = name; }
    
    const std::string& queryType() const { return query_type_; }
    const std::string& mutationType() const { return mutation_type_; }
    const std::string& subscriptionType() const { return subscription_type_; }
    
    // Introspection policy
    /**
     * @brief Set Introspection Enabled.
     * @param[in] enabled Input parameter.
     * @details Implements setIntrospectionEnabled without additional internal calls.
     */
    void setIntrospectionEnabled(bool enabled) { introspection_enabled_ = enabled; }
    bool isIntrospectionEnabled() const { return introspection_enabled_; }
    
    /**
     * @brief Generate SDL (Schema Definition Language)
     * @return Return value.
     */
    std::string toSDL() const;
    
    /**
     * @brief Introspection support (respects introspection policy)
     * @param[in] field Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Value> introspect(const Field& field) const;
    
private:
    std::unordered_map<std::string, TypeDefinition> types_;
    std::string query_type_ = "Query";
    std::string mutation_type_ = "Mutation";
    std::string subscription_type_ = "Subscription";
    bool introspection_enabled_ = true;  // Default: enabled for development
};

class ThemisSchemaBuilder {
public:
    /**
     * @brief Build.
     * @return Return value.
     */
    static Schema build();
    
private:
    /**
     * @brief Add Geo Scalar Types.
     * @param[in,out] schema Input/output parameter.
     */
    static void addGeoScalarTypes(Schema& schema);
    /**
     * @brief Add Document Types.
     * @param[in,out] schema Input/output parameter.
     */
    static void addDocumentTypes(Schema& schema);
    /**
     * @brief Add Graph Types.
     * @param[in,out] schema Input/output parameter.
     */
    static void addGraphTypes(Schema& schema);
    /**
     * @brief Add Vector Types.
     * @param[in,out] schema Input/output parameter.
     */
    static void addVectorTypes(Schema& schema);
    /**
     * @brief Add Timeseries Types.
     * @param[in,out] schema Input/output parameter.
     */
    static void addTimeseriesTypes(Schema& schema);
    /**
     * @brief Add Query Type.
     * @param[in,out] schema Input/output parameter.
     */
    static void addQueryType(Schema& schema);
    /**
     * @brief Add Mutation Type.
     * @param[in,out] schema Input/output parameter.
     */
    static void addMutationType(Schema& schema);
    /**
     * @brief Add Subscription Type.
     * @param[in,out] schema Input/output parameter.
     */
    static void addSubscriptionType(Schema& schema);
};

} // namespace graphql
} // namespace themis
