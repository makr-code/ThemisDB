/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql.cpp                                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     1214                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "api/graphql.h"
#include "utils/error_registry.h"
#include <cctype>
#include <sstream>
#include <algorithm>

namespace themis {
namespace graphql {

using errors::ErrorCode;

// ============================================================================
// Parser Implementation
// ============================================================================

Parser::Parser(std::string_view query, const QueryLimits& limits) 
    : source_(query), limits_(limits) {}

Parser::Result Parser::parse(std::string_view query) {
    return parse(query, QueryLimits::defaults());
}

Parser::Result Parser::parse(std::string_view query, const QueryLimits& limits) {
    Parser parser(query, limits);
    return parser.parseDocument();
}

bool Parser::checkQuerySize() {
    if (source_.size() > limits_.max_query_size_bytes) {
        error("Query size exceeds maximum allowed size of " + 
              std::to_string(limits_.max_query_size_bytes) + " bytes");
        return false;
    }
    return true;
}

bool Parser::checkDepthLimit(size_t depth) {
    if (depth > max_depth_reached_) {
        max_depth_reached_ = depth;
    }
    if (depth > limits_.max_depth) {
        error("Query depth exceeds maximum allowed depth of " + 
              std::to_string(limits_.max_depth));
        return false;
    }
    return true;
}

bool Parser::checkFieldLimit() {
    if (field_count_ > limits_.max_fields) {
        error("Query field count exceeds maximum allowed fields of " + 
              std::to_string(limits_.max_fields));
        return false;
    }
    return true;
}

bool Parser::checkASTNodeLimit() {
    if (ast_node_count_ > limits_.max_ast_nodes) {
        error("Query AST node count exceeds maximum allowed nodes of " + 
              std::to_string(limits_.max_ast_nodes));
        return false;
    }
    return true;
}

Parser::Result Parser::parseDocument() {
    Result result;
    result.success = true;
    
    // Check query size first
    if (!checkQuerySize()) {
        result.success = false;
        result.errors = errors_;
        return result;
    }
    
    skipWhitespace();
    
    while (pos_ < source_.size()) {
        auto opResult = parseOperation();
        if (opResult) {
            result.document.operations.push_back(std::move(*opResult));
        } else {
            result.errors.push_back(convertToParseError(opResult.error()));
            result.success = false;
            break;
        }
        skipWhitespace();
    }
    
    // Add any accumulated errors from the error() method
    if (!errors_.empty()) {
        result.errors.insert(result.errors.end(), errors_.begin(), errors_.end());
        result.success = false;
    }
    
    return result;
}

themis::Result<Operation> Parser::parseOperation() {
    Operation op;
    incrementASTNodeCount();
    
    if (!checkASTNodeLimit()) {
        return themis::Err<Operation>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            "Query exceeds maximum AST node limit");
    }
    
    skipWhitespace();
    
    // Check for operation type keyword
    if (match("query")) {
        op.type = OperationType::Query;
    } else if (match("mutation")) {
        op.type = OperationType::Mutation;
    } else if (match("subscription")) {
        op.type = OperationType::Subscription;
    } else if (peek('{')) {
        // Anonymous query
        op.type = OperationType::Query;
    } else {
        return themis::Err<Operation>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            getLocationContext() + ": Expected 'query', 'mutation', 'subscription', or '{'");
    }
    
    skipWhitespace();
    
    // Optional operation name
    if (op.type != OperationType::Query || !peek('{') && !peek('(')) {
        auto nameResult = parseName();
        if (nameResult) {
            op.name = *nameResult;
        }
    }
    
    skipWhitespace();
    
    // Optional variable definitions
    if (match('(')) {
        while (!peek(')') && pos_ < source_.size()) {
            skipWhitespace();
            auto varDefResult = parseVariableDefinition();
            if (!varDefResult) {
                return themis::Err<Operation>(varDefResult.error().code(), varDefResult.error().context());
            }
            op.variables.push_back(std::move(*varDefResult));
            incrementASTNodeCount();
            skipWhitespace();
            match(',');
        }
        if (!match(')')) {
            return themis::Err<Operation>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected ')'");
        }
    }
    
    skipWhitespace();
    
    // Selection set
    if (!match('{')) {
        return themis::Err<Operation>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected '{'");
    }
    
    while (!peek('}') && pos_ < source_.size()) {
        skipWhitespace();
        auto fieldResult = parseField(1);  // Start at depth 1
        if (!fieldResult) {
            return themis::Err<Operation>(fieldResult.error().code(), fieldResult.error().context());
        }
        op.selections.push_back(std::move(*fieldResult));
        skipWhitespace();
    }
    
    if (!match('}')) {
        return themis::Err<Operation>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected '}'");
    }
    
    return themis::Ok(std::move(op));
}

themis::Result<Field> Parser::parseField(size_t depth) {
    Field field;
    incrementFieldCount();
    incrementASTNodeCount();
    
    // Check limits
    if (!checkDepthLimit(depth)) {
        return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            "Query exceeds maximum depth limit");
    }
    if (!checkFieldLimit()) {
        return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            "Query exceeds maximum field limit");
    }
    if (!checkASTNodeLimit()) {
        return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            "Query exceeds maximum AST node limit");
    }
    
    skipWhitespace();
    
    // Field name or alias
    auto nameOrAliasResult = parseName();
    if (!nameOrAliasResult) {
        return themis::Err<Field>(nameOrAliasResult.error().code(), nameOrAliasResult.error().context());
    }
    
    skipWhitespace();
    
    // Check for alias
    if (match(':')) {
        field.alias = *nameOrAliasResult;
        skipWhitespace();
        auto fieldNameResult = parseName();
        if (!fieldNameResult) {
            return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
                getLocationContext() + ": Expected field name after alias");
        }
        field.name = *fieldNameResult;
    } else {
        field.name = *nameOrAliasResult;
    }
    
    skipWhitespace();
    
    // Arguments
    if (match('(')) {
        while (!peek(')') && pos_ < source_.size()) {
            skipWhitespace();
            auto argNameResult = parseName();
            if (!argNameResult) {
                return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
                    getLocationContext() + ": Expected argument name");
            }
            skipWhitespace();
            if (!match(':')) {
                return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
                    getLocationContext() + ": Expected ':' after argument name");
            }
            skipWhitespace();
            auto argValueResult = parseValue();
            if (!argValueResult) {
                return themis::Err<Field>(argValueResult.error().code(), argValueResult.error().context());
            }
            field.arguments[*argNameResult] = *argValueResult;
            skipWhitespace();
            match(',');
        }
        if (!match(')')) {
            return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected ')'");
        }
    }
    
    skipWhitespace();
    
    // Nested selection set
    if (match('{')) {
        while (!peek('}') && pos_ < source_.size()) {
            skipWhitespace();
            auto nestedFieldResult = parseField(depth + 1);  // Increment depth for nested fields
            if (!nestedFieldResult) {
                return themis::Err<Field>(nestedFieldResult.error().code(), nestedFieldResult.error().context());
            }
            field.selections.push_back(std::move(*nestedFieldResult));
            skipWhitespace();
        }
        if (!match('}')) {
            return themis::Err<Field>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected '}'");
        }
    }
    
    return themis::Ok(std::move(field));
}

themis::Result<std::shared_ptr<Value>> Parser::parseValue() {
    skipWhitespace();
    
    // Null
    if (match("null")) {
        return themis::Ok(Value::null());
    }
    
    // Boolean
    if (match("true")) {
        return themis::Ok(Value::boolean(true));
    }
    if (match("false")) {
        return themis::Ok(Value::boolean(false));
    }
    
    // String
    if (peek('"')) {
        auto strResult = parseString();
        if (!strResult) {
            return themis::Err<std::shared_ptr<Value>>(strResult.error().code(), strResult.error().context());
        }
        return themis::Ok(Value::string(std::move(*strResult)));
    }
    
    // Number (int or float)
    if (peek('-') || std::isdigit(source_[pos_])) {
        size_t start = pos_;
        bool isFloat = false;
        
        if (peek('-')) {
            ++pos_;
            ++column_;
        }
        
        while (pos_ < source_.size() && std::isdigit(source_[pos_])) {
            ++pos_;
            ++column_;
        }
        
        if (pos_ < source_.size() && source_[pos_] == '.') {
            isFloat = true;
            ++pos_;
            ++column_;
            while (pos_ < source_.size() && std::isdigit(source_[pos_])) {
                ++pos_;
                ++column_;
            }
        }
        
        if (pos_ < source_.size() && (source_[pos_] == 'e' || source_[pos_] == 'E')) {
            isFloat = true;
            ++pos_;
            ++column_;
            if (pos_ < source_.size() && (source_[pos_] == '+' || source_[pos_] == '-')) {
                ++pos_;
                ++column_;
            }
            while (pos_ < source_.size() && std::isdigit(source_[pos_])) {
                ++pos_;
                ++column_;
            }
        }
        
        std::string numStr(source_.substr(start, pos_ - start));
        if (isFloat) {
            return themis::Ok(Value::floating(std::stod(numStr)));
        } else {
            return themis::Ok(Value::integer(std::stoll(numStr)));
        }
    }
    
    // List
    if (match('[')) {
        ValueList list;
        while (!peek(']') && pos_ < source_.size()) {
            skipWhitespace();
            auto valResult = parseValue();
            if (!valResult) {
                return themis::Err<std::shared_ptr<Value>>(valResult.error().code(), valResult.error().context());
            }
            list.push_back(*valResult);
            skipWhitespace();
            match(',');
        }
        if (!match(']')) {
            return themis::Err<std::shared_ptr<Value>>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected ']'");
        }
        return themis::Ok(Value::list(std::move(list)));
    }
    
    // Object
    if (match('{')) {
        ValueMap obj;
        while (!peek('}') && pos_ < source_.size()) {
            skipWhitespace();
            auto keyResult = parseName();
            if (!keyResult) {
                return themis::Err<std::shared_ptr<Value>>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
                    getLocationContext() + ": Expected object key");
            }
            skipWhitespace();
            if (!match(':')) {
                return themis::Err<std::shared_ptr<Value>>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
                    getLocationContext() + ": Expected ':' in object");
            }
            skipWhitespace();
            auto valResult = parseValue();
            if (!valResult) {
                return themis::Err<std::shared_ptr<Value>>(valResult.error().code(), valResult.error().context());
            }
            obj[*keyResult] = *valResult;
            skipWhitespace();
            match(',');
        }
        if (!match('}')) {
            return themis::Err<std::shared_ptr<Value>>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected '}'");
        }
        return themis::Ok(Value::object(std::move(obj)));
    }
    
    // Variable reference ($name)
    if (match('$')) {
        auto nameResult = parseName();
        if (!nameResult) {
            return themis::Err<std::shared_ptr<Value>>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
                getLocationContext() + ": Expected variable name after '$'");
        }
        // Return as special string value (will be resolved at execution)
        return themis::Ok(Value::string("$" + *nameResult));
    }
    
    // Enum value (bare name)
    auto enumValResult = parseName();
    if (enumValResult) {
        return themis::Ok(Value::enumValue(std::move(*enumValResult)));
    }
    
    return themis::Err<std::shared_ptr<Value>>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected value");
}

themis::Result<VariableDefinition> Parser::parseVariableDefinition() {
    VariableDefinition def;
    
    if (!match('$')) {
        return themis::Err<VariableDefinition>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            getLocationContext() + ": Expected '$' for variable definition");
    }
    
    auto nameResult = parseName();
    if (!nameResult) {
        return themis::Err<VariableDefinition>(nameResult.error().code(), nameResult.error().context());
    }
    def.name = *nameResult;
    
    skipWhitespace();
    if (!match(':')) {
        return themis::Err<VariableDefinition>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, 
            getLocationContext() + ": Expected ':' after variable name");
    }
    
    skipWhitespace();
    
    // Type (with optional list and non-null modifiers)
    if (match('[')) {
        def.is_list = true;
        skipWhitespace();
        auto typeNameResult = parseName();
        if (!typeNameResult) {
            return themis::Err<VariableDefinition>(typeNameResult.error().code(), typeNameResult.error().context());
        }
        def.type_name = *typeNameResult;
        skipWhitespace();
        match('!');  // Inner non-null
        skipWhitespace();
        if (!match(']')) {
            return themis::Err<VariableDefinition>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected ']'");
        }
    } else {
        auto typeNameResult = parseName();
        if (!typeNameResult) {
            return themis::Err<VariableDefinition>(typeNameResult.error().code(), typeNameResult.error().context());
        }
        def.type_name = *typeNameResult;
    }
    
    skipWhitespace();
    if (match('!')) {
        def.is_non_null = true;
    }
    
    skipWhitespace();
    
    // Default value
    if (match('=')) {
        skipWhitespace();
        auto valResult = parseValue();
        if (!valResult) {
            return themis::Err<VariableDefinition>(valResult.error().code(), valResult.error().context());
        }
        def.default_value = *valResult;
    }
    
    return themis::Ok(std::move(def));
}

void Parser::skipWhitespace() {
    while (pos_ < source_.size()) {
        char c = source_[pos_];
        if (c == ' ' || c == '\t' || c == '\r' || c == ',') {
            ++pos_;
            ++column_;
        } else if (c == '\n') {
            ++pos_;
            ++line_;
            column_ = 1;
        } else if (c == '#') {
            skipComment();
        } else {
            break;
        }
    }
}

void Parser::skipComment() {
    // Skip from # to end of line
    while (pos_ < source_.size() && source_[pos_] != '\n') {
        ++pos_;
        ++column_;
    }
}

bool Parser::match(char c) {
    if (pos_ < source_.size() && source_[pos_] == c) {
        ++pos_;
        ++column_;
        return true;
    }
    return false;
}

bool Parser::match(std::string_view s) {
    if (pos_ + s.size() <= source_.size()) {
        if (source_.substr(pos_, s.size()) == s) {
            // Make sure it's a complete token (not followed by alphanumeric)
            if (pos_ + s.size() < source_.size()) {
                char next = source_[pos_ + s.size()];
                if (std::isalnum(next) || next == '_') {
                    return false;
                }
            }
            pos_ += s.size();
            column_ += s.size();
            return true;
        }
    }
    return false;
}

bool Parser::peek(char c) const {
    return pos_ < source_.size() && source_[pos_] == c;
}

themis::Result<std::string> Parser::parseName() {
    size_t start = pos_;
    
    // Name must start with letter or underscore
    if (pos_ < source_.size() && (std::isalpha(source_[pos_]) || source_[pos_] == '_')) {
        ++pos_;
        ++column_;
        while (pos_ < source_.size() && (std::isalnum(source_[pos_]) || source_[pos_] == '_')) {
            ++pos_;
            ++column_;
        }
        return themis::Ok(std::string(source_.substr(start, pos_ - start)));
    }
    return themis::Err<std::string>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected name");
}

themis::Result<std::string> Parser::parseString() {
    if (!match('"')) {
        return themis::Err<std::string>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Expected string");
    }
    
    std::string result;
    while (pos_ < source_.size() && source_[pos_] != '"') {
        if (source_[pos_] == '\\') {
            ++pos_;
            ++column_;
            if (pos_ < source_.size()) {
                switch (source_[pos_]) {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    default: result += source_[pos_]; break;
                }
                ++pos_;
                ++column_;
            }
        } else if (source_[pos_] == '\n') {
            return themis::Err<std::string>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Unterminated string");
        } else {
            result += source_[pos_];
            ++pos_;
            ++column_;
        }
    }
    
    if (!match('"')) {
        return themis::Err<std::string>(ErrorCode::ERR_QUERY_INVALID_SYNTAX, getLocationContext() + ": Unterminated string");
    }
    
    return themis::Ok(std::move(result));
}

void Parser::error(std::string message) {
    ParseError err;
    err.message = std::move(message);
    err.line = line_;
    err.column = column_;
    errors_.push_back(std::move(err));
}

std::string Parser::getLocationContext() const {
    return "Line " + std::to_string(line_) + ", Column " + std::to_string(column_);
}

ParseError Parser::convertToParseError(const themis::Error& error) {
    ParseError err;
    err.message = error.message();
    // Note: Location information is already included in the error message's context,
    // which was captured at the point where the error occurred.
    // The current parser line/column are set here to provide position where the error was detected.
    err.line = line_;
    err.column = column_;
    return err;
}

// ============================================================================
// Executor Implementation
// ============================================================================

Executor::Result Executor::execute(
    const Document& document,
    const ExecutionContext& context,
    std::string_view operation_name
) {
    Result result;
    
    const Operation* op = document.getOperation(operation_name);
    if (!op) {
        result.addError(
            "Operation not found: " + std::string(operation_name),
            "ERR_OPERATION_NOT_FOUND",
            context.mask_errors
        );
        return result;
    }
    
    try {
        result.data = executeOperation(*op, context);
    } catch (const std::exception& e) {
        result.addError(
            std::string("Execution error: ") + e.what(),
            "ERR_EXECUTION_FAILED",
            context.mask_errors
        );
    } catch (...) {
        result.addError(
            "Unknown execution error",
            "ERR_EXECUTION_FAILED",
            context.mask_errors
        );
    }
    
    return result;
}

std::shared_ptr<Value> Executor::executeOperation(
    const Operation& operation,
    const ExecutionContext& context
) {
    return executeSelections(operation.selections, nullptr, context);
}

std::shared_ptr<Value> Executor::executeSelections(
    const std::vector<Field>& selections,
    const std::shared_ptr<Value>& parent,
    const ExecutionContext& context
) {
    ValueMap result;
    
    for (const auto& field : selections) {
        auto value = executeField(field, parent, context);
        result[field.responseName()] = value;
    }
    
    return Value::object(std::move(result));
}

std::shared_ptr<Value> Executor::executeField(
    const Field& field,
    const std::shared_ptr<Value>& parent,
    const ExecutionContext& context
) {
    // Look up resolver for this field
    auto it = context.resolvers.find(field.name);
    if (it != context.resolvers.end()) {
        return it->second(field, parent, context);
    }
    
    // Default: try to get from parent object
    if (parent && parent->isObject()) {
        const auto& obj = parent->asObject();
        auto fieldIt = obj.find(field.name);
        if (fieldIt != obj.end()) {
            auto value = fieldIt->second;
            
            // If field has nested selections and value is object/list, recurse
            if (!field.selections.empty()) {
                if (value->isObject()) {
                    return executeSelections(field.selections, value, context);
                } else if (value->isList()) {
                    ValueList resultList;
                    for (const auto& item : value->asList()) {
                        resultList.push_back(executeSelections(field.selections, item, context));
                    }
                    return Value::list(std::move(resultList));
                }
            }
            
            return value;
        }
    }
    
    return Value::null();
}

// ============================================================================
// Schema Implementation
// ============================================================================

Schema::Schema() {
    // Add built-in scalar types
    TypeDefinition stringType;
    stringType.kind = TypeDefinition::Kind::Scalar;
    stringType.name = "String";
    stringType.description = "The `String` scalar type represents textual data.";
    types_["String"] = stringType;
    
    TypeDefinition intType;
    intType.kind = TypeDefinition::Kind::Scalar;
    intType.name = "Int";
    intType.description = "The `Int` scalar type represents non-fractional signed whole numeric values.";
    types_["Int"] = intType;
    
    TypeDefinition floatType;
    floatType.kind = TypeDefinition::Kind::Scalar;
    floatType.name = "Float";
    floatType.description = "The `Float` scalar type represents signed double-precision fractional values.";
    types_["Float"] = floatType;
    
    TypeDefinition boolType;
    boolType.kind = TypeDefinition::Kind::Scalar;
    boolType.name = "Boolean";
    boolType.description = "The `Boolean` scalar type represents `true` or `false`.";
    types_["Boolean"] = boolType;
    
    TypeDefinition idType;
    idType.kind = TypeDefinition::Kind::Scalar;
    idType.name = "ID";
    idType.description = "The `ID` scalar type represents a unique identifier.";
    types_["ID"] = idType;
}

void Schema::addType(TypeDefinition type) {
    types_[type.name] = std::move(type);
}

const TypeDefinition* Schema::getType(std::string_view name) const {
    auto it = types_.find(std::string(name));
    return it != types_.end() ? &it->second : nullptr;
}

std::string Schema::toSDL() const {
    std::ostringstream oss;
    
    // Schema definition
    oss << "schema {\n";
    oss << "  query: " << query_type_ << "\n";
    if (!mutation_type_.empty()) {
        oss << "  mutation: " << mutation_type_ << "\n";
    }
    if (!subscription_type_.empty()) {
        oss << "  subscription: " << subscription_type_ << "\n";
    }
    oss << "}\n\n";
    
    // Type definitions
    for (const auto& [name, type] : types_) {
        // Skip built-in scalars
        if (type.kind == TypeDefinition::Kind::Scalar &&
            (name == "String" || name == "Int" || name == "Float" || 
             name == "Boolean" || name == "ID")) {
            continue;
        }
        
        if (!type.description.empty()) {
            oss << "\"" << type.description << "\"\n";
        }
        
        switch (type.kind) {
            case TypeDefinition::Kind::Scalar:
                oss << "scalar " << name << "\n\n";
                break;
            case TypeDefinition::Kind::Enum:
                oss << "enum " << name << " {\n";
                for (const auto& val : type.enum_values) {
                    oss << "  " << val << "\n";
                }
                oss << "}\n\n";
                break;
            case TypeDefinition::Kind::InputObject:
                oss << "input " << name << " {\n";
                for (const auto& field : type.fields) {
                    oss << "  " << field.name << ": " << field.type.name;
                    if (field.type.is_non_null) oss << "!";
                    oss << "\n";
                }
                oss << "}\n\n";
                break;
            default:
                oss << "type " << name;
                if (!type.interfaces.empty()) {
                    oss << " implements";
                    for (size_t i = 0; i < type.interfaces.size(); ++i) {
                        if (i > 0) oss << " &";
                        oss << " " << type.interfaces[i];
                    }
                }
                oss << " {\n";
                for (const auto& field : type.fields) {
                    oss << "  " << field.name;
                    if (!field.arguments.empty()) {
                        oss << "(";
                        bool first = true;
                        for (const auto& [argName, argType] : field.arguments) {
                            if (!first) oss << ", ";
                            oss << argName << ": " << argType.name;
                            if (argType.is_non_null) oss << "!";
                            first = false;
                        }
                        oss << ")";
                    }
                    oss << ": " << field.type.name;
                    if (field.type.is_list) oss << "]";
                    if (field.type.is_non_null) oss << "!";
                    oss << "\n";
                }
                oss << "}\n\n";
                break;
        }
    }
    
    return oss.str();
}

// ============================================================================
// ThemisSchemaBuilder Implementation
// ============================================================================

Schema ThemisSchemaBuilder::build() {
    Schema schema;
    
    // Add custom geo scalar types
    addGeoScalarTypes(schema);
    
    addDocumentTypes(schema);
    addGraphTypes(schema);
    addVectorTypes(schema);
    addTimeseriesTypes(schema);
    addQueryType(schema);
    addMutationType(schema);
    
    return schema;
}

void ThemisSchemaBuilder::addGeoScalarTypes(Schema& schema) {
    // Latitude scalar type
    TypeDefinition latType;
    latType.kind = TypeDefinition::Kind::Scalar;
    latType.name = "Latitude";
    latType.description = "The `Latitude` scalar type represents a latitude coordinate in decimal degrees. "
                          "Valid range: -90.0 to 90.0 (WGS84).";
    schema.addType(latType);
    
    // Longitude scalar type
    TypeDefinition lonType;
    lonType.kind = TypeDefinition::Kind::Scalar;
    lonType.name = "Longitude";
    lonType.description = "The `Longitude` scalar type represents a longitude coordinate in decimal degrees. "
                          "Valid range: -180.0 to 180.0 (WGS84).";
    schema.addType(lonType);
    
    // GeoPoint type
    TypeDefinition geoPointType;
    geoPointType.kind = TypeDefinition::Kind::Object;
    geoPointType.name = "GeoPoint";
    geoPointType.description = "A geographic point with latitude and longitude coordinates (WGS84).";
    
    FieldDefinition latField;
    latField.name = "lat";
    latField.description = "Latitude coordinate (-90 to 90)";
    latField.type = {"Latitude", true, false, nullptr};
    geoPointType.fields.push_back(latField);
    
    FieldDefinition lonField;
    lonField.name = "lon";
    lonField.description = "Longitude coordinate (-180 to 180)";
    lonField.type = {"Longitude", true, false, nullptr};
    geoPointType.fields.push_back(lonField);
    
    schema.addType(geoPointType);
    
    // GeoPointInput type for mutations
    TypeDefinition geoPointInputType;
    geoPointInputType.kind = TypeDefinition::Kind::InputObject;
    geoPointInputType.name = "GeoPointInput";
    geoPointInputType.description = "Input type for geographic coordinates.";
    
    FieldDefinition latInputField;
    latInputField.name = "lat";
    latInputField.description = "Latitude coordinate (-90 to 90)";
    latInputField.type = {"Latitude", true, false, nullptr};
    geoPointInputType.fields.push_back(latInputField);
    
    FieldDefinition lonInputField;
    lonInputField.name = "lon";
    lonInputField.description = "Longitude coordinate (-180 to 180)";
    lonInputField.type = {"Longitude", true, false, nullptr};
    geoPointInputType.fields.push_back(lonInputField);
    
    schema.addType(geoPointInputType);
    
    // GeoJSON scalar type
    TypeDefinition geoJSONType;
    geoJSONType.kind = TypeDefinition::Kind::Scalar;
    geoJSONType.name = "GeoJSON";
    geoJSONType.description = "The `GeoJSON` scalar type represents GeoJSON geometry objects as defined in RFC 7946.";
    schema.addType(geoJSONType);
}

void ThemisSchemaBuilder::addDocumentTypes(Schema& schema) {
    // Document type
    TypeDefinition docType;
    docType.kind = TypeDefinition::Kind::Object;
    docType.name = "Document";
    docType.description = "A ThemisDB document";
    
    FieldDefinition idField;
    idField.name = "id";
    idField.type = {"ID", true, false, nullptr};
    docType.fields.push_back(idField);
    
    FieldDefinition collectionField;
    collectionField.name = "collection";
    collectionField.type = {"String", true, false, nullptr};
    docType.fields.push_back(collectionField);
    
    FieldDefinition dataField;
    dataField.name = "data";
    dataField.type = {"JSON", true, false, nullptr};
    docType.fields.push_back(dataField);
    
    FieldDefinition createdField;
    createdField.name = "createdAt";
    createdField.type = {"String", false, false, nullptr};
    docType.fields.push_back(createdField);
    
    FieldDefinition updatedField;
    updatedField.name = "updatedAt";
    updatedField.type = {"String", false, false, nullptr};
    docType.fields.push_back(updatedField);
    
    schema.addType(docType);
    
    // JSON scalar
    TypeDefinition jsonType;
    jsonType.kind = TypeDefinition::Kind::Scalar;
    jsonType.name = "JSON";
    jsonType.description = "The `JSON` scalar type represents JSON values.";
    schema.addType(jsonType);
    
    // DocumentInput type
    TypeDefinition docInput;
    docInput.kind = TypeDefinition::Kind::InputObject;
    docInput.name = "DocumentInput";
    
    FieldDefinition dataInputField;
    dataInputField.name = "data";
    dataInputField.type = {"JSON", true, false, nullptr};
    docInput.fields.push_back(dataInputField);
    
    schema.addType(docInput);
}

void ThemisSchemaBuilder::addGraphTypes(Schema& schema) {
    // Node type
    TypeDefinition nodeType;
    nodeType.kind = TypeDefinition::Kind::Object;
    nodeType.name = "Node";
    nodeType.description = "A graph node";
    
    FieldDefinition nodeIdField;
    nodeIdField.name = "id";
    nodeIdField.type = {"ID", true, false, nullptr};
    nodeType.fields.push_back(nodeIdField);
    
    FieldDefinition labelsField;
    labelsField.name = "labels";
    labelsField.type = {"String", false, true, nullptr};
    nodeType.fields.push_back(labelsField);
    
    FieldDefinition propsField;
    propsField.name = "properties";
    propsField.type = {"JSON", false, false, nullptr};
    nodeType.fields.push_back(propsField);
    
    schema.addType(nodeType);
    
    // Edge type
    TypeDefinition edgeType;
    edgeType.kind = TypeDefinition::Kind::Object;
    edgeType.name = "Edge";
    edgeType.description = "A graph edge";
    
    FieldDefinition edgeIdField;
    edgeIdField.name = "id";
    edgeIdField.type = {"ID", true, false, nullptr};
    edgeType.fields.push_back(edgeIdField);
    
    FieldDefinition typeField;
    typeField.name = "type";
    typeField.type = {"String", true, false, nullptr};
    edgeType.fields.push_back(typeField);
    
    FieldDefinition sourceField;
    sourceField.name = "source";
    sourceField.type = {"Node", true, false, nullptr};
    edgeType.fields.push_back(sourceField);
    
    FieldDefinition targetField;
    targetField.name = "target";
    targetField.type = {"Node", true, false, nullptr};
    edgeType.fields.push_back(targetField);
    
    FieldDefinition edgePropsField;
    edgePropsField.name = "properties";
    edgePropsField.type = {"JSON", false, false, nullptr};
    edgeType.fields.push_back(edgePropsField);
    
    schema.addType(edgeType);
}

void ThemisSchemaBuilder::addVectorTypes(Schema& schema) {
    // VectorSearchResult type
    TypeDefinition resultType;
    resultType.kind = TypeDefinition::Kind::Object;
    resultType.name = "VectorSearchResult";
    resultType.description = "Result from vector similarity search";
    
    FieldDefinition resIdField;
    resIdField.name = "id";
    resIdField.type = {"ID", true, false, nullptr};
    resultType.fields.push_back(resIdField);
    
    FieldDefinition scoreField;
    scoreField.name = "score";
    scoreField.type = {"Float", true, false, nullptr};
    resultType.fields.push_back(scoreField);
    
    FieldDefinition docField;
    docField.name = "document";
    docField.type = {"Document", false, false, nullptr};
    resultType.fields.push_back(docField);
    
    schema.addType(resultType);
}

void ThemisSchemaBuilder::addTimeseriesTypes(Schema& schema) {
    // TimeseriesPoint type
    TypeDefinition pointType;
    pointType.kind = TypeDefinition::Kind::Object;
    pointType.name = "TimeseriesPoint";
    pointType.description = "A single timeseries data point";
    
    FieldDefinition tsField;
    tsField.name = "timestamp";
    tsField.type = {"String", true, false, nullptr};
    pointType.fields.push_back(tsField);
    
    FieldDefinition valField;
    valField.name = "value";
    valField.type = {"Float", true, false, nullptr};
    pointType.fields.push_back(valField);
    
    FieldDefinition tagsField;
    tagsField.name = "tags";
    tagsField.type = {"JSON", false, false, nullptr};
    pointType.fields.push_back(tagsField);
    
    schema.addType(pointType);
}

void ThemisSchemaBuilder::addQueryType(Schema& schema) {
    TypeDefinition queryType;
    queryType.kind = TypeDefinition::Kind::Object;
    queryType.name = "Query";
    queryType.description = "ThemisDB Query operations";
    
    // document(collection: String!, id: ID!): Document
    FieldDefinition docQuery;
    docQuery.name = "document";
    docQuery.type = {"Document", false, false, nullptr};
    docQuery.arguments["collection"] = {"String", true, false, nullptr};
    docQuery.arguments["id"] = {"ID", true, false, nullptr};
    queryType.fields.push_back(docQuery);
    
    // documents(collection: String!, limit: Int, offset: Int): [Document!]!
    FieldDefinition docsQuery;
    docsQuery.name = "documents";
    docsQuery.type = {"Document", true, true, nullptr};
    docsQuery.arguments["collection"] = {"String", true, false, nullptr};
    docsQuery.arguments["limit"] = {"Int", false, false, nullptr};
    docsQuery.arguments["offset"] = {"Int", false, false, nullptr};
    queryType.fields.push_back(docsQuery);
    
    // aql(query: String!, variables: JSON): JSON
    FieldDefinition aqlQuery;
    aqlQuery.name = "aql";
    aqlQuery.type = {"JSON", false, false, nullptr};
    aqlQuery.arguments["query"] = {"String", true, false, nullptr};
    aqlQuery.arguments["variables"] = {"JSON", false, false, nullptr};
    queryType.fields.push_back(aqlQuery);
    
    // vectorSearch(collection: String!, vector: [Float!]!, k: Int): [VectorSearchResult!]!
    FieldDefinition vectorQuery;
    vectorQuery.name = "vectorSearch";
    vectorQuery.type = {"VectorSearchResult", true, true, nullptr};
    vectorQuery.arguments["collection"] = {"String", true, false, nullptr};
    vectorQuery.arguments["vector"] = {"Float", true, true, nullptr};
    vectorQuery.arguments["k"] = {"Int", false, false, nullptr};
    queryType.fields.push_back(vectorQuery);
    
    // graphTraversal(startNode: ID!, depth: Int, direction: String): [Node!]!
    FieldDefinition graphQuery;
    graphQuery.name = "graphTraversal";
    graphQuery.type = {"Node", true, true, nullptr};
    graphQuery.arguments["startNode"] = {"ID", true, false, nullptr};
    graphQuery.arguments["depth"] = {"Int", false, false, nullptr};
    graphQuery.arguments["direction"] = {"String", false, false, nullptr};
    queryType.fields.push_back(graphQuery);
    
    schema.addType(queryType);
}

void ThemisSchemaBuilder::addMutationType(Schema& schema) {
    TypeDefinition mutationType;
    mutationType.kind = TypeDefinition::Kind::Object;
    mutationType.name = "Mutation";
    mutationType.description = "ThemisDB Mutation operations";
    
    // createDocument(collection: String!, input: DocumentInput!): Document!
    FieldDefinition createDoc;
    createDoc.name = "createDocument";
    createDoc.type = {"Document", true, false, nullptr};
    createDoc.arguments["collection"] = {"String", true, false, nullptr};
    createDoc.arguments["input"] = {"DocumentInput", true, false, nullptr};
    mutationType.fields.push_back(createDoc);
    
    // updateDocument(collection: String!, id: ID!, input: DocumentInput!): Document
    FieldDefinition updateDoc;
    updateDoc.name = "updateDocument";
    updateDoc.type = {"Document", false, false, nullptr};
    updateDoc.arguments["collection"] = {"String", true, false, nullptr};
    updateDoc.arguments["id"] = {"ID", true, false, nullptr};
    updateDoc.arguments["input"] = {"DocumentInput", true, false, nullptr};
    mutationType.fields.push_back(updateDoc);
    
    // deleteDocument(collection: String!, id: ID!): Boolean!
    FieldDefinition deleteDoc;
    deleteDoc.name = "deleteDocument";
    deleteDoc.type = {"Boolean", true, false, nullptr};
    deleteDoc.arguments["collection"] = {"String", true, false, nullptr};
    deleteDoc.arguments["id"] = {"ID", true, false, nullptr};
    mutationType.fields.push_back(deleteDoc);
    
    // createEdge(source: ID!, target: ID!, type: String!, properties: JSON): Edge!
    FieldDefinition createEdge;
    createEdge.name = "createEdge";
    createEdge.type = {"Edge", true, false, nullptr};
    createEdge.arguments["source"] = {"ID", true, false, nullptr};
    createEdge.arguments["target"] = {"ID", true, false, nullptr};
    createEdge.arguments["type"] = {"String", true, false, nullptr};
    createEdge.arguments["properties"] = {"JSON", false, false, nullptr};
    mutationType.fields.push_back(createEdge);
    
    schema.addType(mutationType);
}

} // namespace graphql
} // namespace themis
