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

/**
 * @brief GraphQL Value representation
 * 
 * Supports all GraphQL value types: Null, Boolean, Int, Float, String, Enum, List, Object
 */
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
        Object
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
    static std::shared_ptr<Value> null() {
        return std::make_shared<Value>();
    }
    
    static std::shared_ptr<Value> boolean(bool v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Boolean;
        val->data = v;
        return val;
    }
    
    static std::shared_ptr<Value> integer(int64_t v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Int;
        val->data = v;
        return val;
    }
    
    static std::shared_ptr<Value> floating(double v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Float;
        val->data = v;
        return val;
    }
    
    static std::shared_ptr<Value> string(std::string v) {
        auto val = std::make_shared<Value>();
        val->type = Type::String;
        val->data = std::move(v);
        return val;
    }
    
    static std::shared_ptr<Value> enumValue(std::string v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Enum;
        val->data = std::move(v);
        return val;
    }
    
    static std::shared_ptr<Value> list(ValueList v) {
        auto val = std::make_shared<Value>();
        val->type = Type::List;
        val->data = std::move(v);
        return val;
    }
    
    static std::shared_ptr<Value> object(ValueMap v) {
        auto val = std::make_shared<Value>();
        val->type = Type::Object;
        val->data = std::move(v);
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
    
    // Value getters
    bool asBool() const { return std::get<bool>(data); }
    int64_t asInt() const { return std::get<int64_t>(data); }
    double asFloat() const { return std::get<double>(data); }
    const std::string& asString() const { return std::get<std::string>(data); }
    const ValueList& asList() const { return std::get<ValueList>(data); }
    const ValueMap& asObject() const { return std::get<ValueMap>(data); }
};

/**
 * @brief GraphQL Field Selection
 */
struct Field {
    std::string name;
    std::string alias;  // Optional alias
    std::unordered_map<std::string, std::shared_ptr<Value>> arguments;
    std::vector<Field> selections;  // Nested selections
    
    const std::string& responseName() const {
        return alias.empty() ? name : alias;
    }
};

/**
 * @brief GraphQL Operation Type
 */
enum class OperationType {
    Query,
    Mutation,
    Subscription
};

/**
 * @brief GraphQL Variable Definition
 */
struct VariableDefinition {
    std::string name;
    std::string type_name;
    bool is_non_null = false;
    bool is_list = false;
    std::shared_ptr<Value> default_value;
};

/**
 * @brief GraphQL Operation (parsed query)
 */
struct Operation {
    OperationType type = OperationType::Query;
    std::string name;  // Optional operation name
    std::vector<VariableDefinition> variables;
    std::vector<Field> selections;
};

/**
 * @brief GraphQL Document (can contain multiple operations)
 */
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

/**
 * @brief GraphQL Parse Error
 */
struct ParseError {
    std::string message;
    size_t line = 0;
    size_t column = 0;
    
    std::string toString() const {
        return "Line " + std::to_string(line) + ", Column " + std::to_string(column) + ": " + message;
    }
};

/**
 * @brief GraphQL Parser
 * 
 * Parses GraphQL query strings into Document structures.
 * 
 * Supported features:
 * - Query, Mutation, Subscription operations
 * - Field selections with aliases
 * - Arguments (all value types)
 * - Variables and variable definitions
 * - Nested selections
 * - Comments (# to end of line)
 * 
 * Not yet supported:
 * - Fragments
 * - Directives
 * - Inline fragments
 */
class Parser {
public:
    struct Result {
        bool success = false;
        Document document;
        std::vector<ParseError> errors;
    };
    
    static Result parse(std::string_view query);
    
private:
    Parser(std::string_view query);
    
    Result parseDocument();
    themis::Result<Operation> parseOperation();
    themis::Result<Field> parseField();
    themis::Result<std::shared_ptr<Value>> parseValue();
    themis::Result<VariableDefinition> parseVariableDefinition();
    
    // Tokenization helpers
    void skipWhitespace();
    void skipComment();
    bool match(char c);
    bool match(std::string_view s);
    bool peek(char c) const;
    themis::Result<std::string> parseName();
    themis::Result<std::string> parseString();
    
    // Helper methods
    std::string getLocationContext() const;
    ParseError convertToParseError(const themis::Error& error);
    
    // Deprecated: Use Result<T> return types instead of error() method
    void error(std::string message);
    
    std::string_view source_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    std::vector<ParseError> errors_;
};

/**
 * @brief GraphQL Execution Context
 */
struct ExecutionContext {
    std::unordered_map<std::string, std::shared_ptr<Value>> variables;
    std::string tenant_id;
    std::string user_id;
    
    // Resolver function type
    using Resolver = std::function<std::shared_ptr<Value>(
        const Field& field,
        const std::shared_ptr<Value>& parent,
        const ExecutionContext& ctx
    )>;
    
    std::unordered_map<std::string, Resolver> resolvers;
};

/**
 * @brief GraphQL Executor
 * 
 * Executes parsed GraphQL documents against the ThemisDB data layer.
 */
class Executor {
public:
    struct Result {
        std::shared_ptr<Value> data;
        std::vector<std::string> errors;
        
        bool hasErrors() const { return !errors.empty(); }
    };
    
    Result execute(
        const Document& document,
        const ExecutionContext& context,
        std::string_view operation_name = ""
    );
    
private:
    std::shared_ptr<Value> executeOperation(
        const Operation& operation,
        const ExecutionContext& context
    );
    
    std::shared_ptr<Value> executeSelections(
        const std::vector<Field>& selections,
        const std::shared_ptr<Value>& parent,
        const ExecutionContext& context
    );
    
    std::shared_ptr<Value> executeField(
        const Field& field,
        const std::shared_ptr<Value>& parent,
        const ExecutionContext& context
    );
};

/**
 * @brief GraphQL Schema Type
 */
struct TypeRef {
    std::string name;
    bool is_non_null = false;
    bool is_list = false;
    std::shared_ptr<TypeRef> of_type;  // For list/non-null wrapping
};

/**
 * @brief GraphQL Field Definition
 */
struct FieldDefinition {
    std::string name;
    std::string description;
    TypeRef type;
    std::unordered_map<std::string, TypeRef> arguments;
};

/**
 * @brief GraphQL Type Definition
 */
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

/**
 * @brief GraphQL Schema
 * 
 * Defines the types and operations available in the API.
 */
class Schema {
public:
    Schema();
    
    void addType(TypeDefinition type);
    const TypeDefinition* getType(std::string_view name) const;
    
    void setQueryType(std::string_view name) { query_type_ = name; }
    void setMutationType(std::string_view name) { mutation_type_ = name; }
    void setSubscriptionType(std::string_view name) { subscription_type_ = name; }
    
    const std::string& queryType() const { return query_type_; }
    const std::string& mutationType() const { return mutation_type_; }
    const std::string& subscriptionType() const { return subscription_type_; }
    
    // Generate SDL (Schema Definition Language)
    std::string toSDL() const;
    
    // Introspection support
    std::shared_ptr<Value> introspect(const Field& field) const;
    
private:
    std::unordered_map<std::string, TypeDefinition> types_;
    std::string query_type_ = "Query";
    std::string mutation_type_ = "Mutation";
    std::string subscription_type_ = "Subscription";
};

/**
 * @brief ThemisDB GraphQL Schema Builder
 * 
 * Creates the default GraphQL schema for ThemisDB operations.
 */
class ThemisSchemaBuilder {
public:
    static Schema build();
    
private:
    static void addDocumentTypes(Schema& schema);
    static void addGraphTypes(Schema& schema);
    static void addVectorTypes(Schema& schema);
    static void addTimeseriesTypes(Schema& schema);
    static void addQueryType(Schema& schema);
    static void addMutationType(Schema& schema);
};

} // namespace graphql
} // namespace themis
