#include "api/graphql.h"
#include <cctype>
#include <sstream>
#include <algorithm>

namespace themis {
namespace graphql {

// ============================================================================
// Parser Implementation
// ============================================================================

Parser::Parser(std::string_view query) : source_(query) {}

Parser::Result Parser::parse(std::string_view query) {
    Parser parser(query);
    return parser.parseDocument();
}

Parser::Result Parser::parseDocument() {
    Result result;
    result.success = true;
    
    skipWhitespace();
    
    while (pos_ < source_.size()) {
        auto op = parseOperation();
        if (op) {
            result.document.operations.push_back(std::move(*op));
        } else if (!errors_.empty()) {
            result.success = false;
            break;
        }
        skipWhitespace();
    }
    
    result.errors = std::move(errors_);
    if (!result.errors.empty()) {
        result.success = false;
    }
    
    return result;
}

std::optional<Operation> Parser::parseOperation() {
    Operation op;
    
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
        error("Expected 'query', 'mutation', 'subscription', or '{'");
        return std::nullopt;
    }
    
    skipWhitespace();
    
    // Optional operation name
    if (op.type != OperationType::Query || !peek('{') && !peek('(')) {
        auto name = parseName();
        if (name) {
            op.name = *name;
        }
    }
    
    skipWhitespace();
    
    // Optional variable definitions
    if (match('(')) {
        while (!peek(')') && pos_ < source_.size()) {
            skipWhitespace();
            auto varDef = parseVariableDefinition();
            if (varDef) {
                op.variables.push_back(std::move(*varDef));
            }
            skipWhitespace();
            match(',');
        }
        if (!match(')')) {
            error("Expected ')'");
            return std::nullopt;
        }
    }
    
    skipWhitespace();
    
    // Selection set
    if (!match('{')) {
        error("Expected '{'");
        return std::nullopt;
    }
    
    while (!peek('}') && pos_ < source_.size()) {
        skipWhitespace();
        auto field = parseField();
        if (field) {
            op.selections.push_back(std::move(*field));
        }
        skipWhitespace();
    }
    
    if (!match('}')) {
        error("Expected '}'");
        return std::nullopt;
    }
    
    return op;
}

std::optional<Field> Parser::parseField() {
    Field field;
    
    skipWhitespace();
    
    // Field name or alias
    auto nameOrAlias = parseName();
    if (!nameOrAlias) {
        error("Expected field name");
        return std::nullopt;
    }
    
    skipWhitespace();
    
    // Check for alias
    if (match(':')) {
        field.alias = *nameOrAlias;
        skipWhitespace();
        auto fieldName = parseName();
        if (!fieldName) {
            error("Expected field name after alias");
            return std::nullopt;
        }
        field.name = *fieldName;
    } else {
        field.name = *nameOrAlias;
    }
    
    skipWhitespace();
    
    // Arguments
    if (match('(')) {
        while (!peek(')') && pos_ < source_.size()) {
            skipWhitespace();
            auto argName = parseName();
            if (!argName) {
                error("Expected argument name");
                return std::nullopt;
            }
            skipWhitespace();
            if (!match(':')) {
                error("Expected ':' after argument name");
                return std::nullopt;
            }
            skipWhitespace();
            auto argValue = parseValue();
            if (!argValue) {
                return std::nullopt;
            }
            field.arguments[*argName] = *argValue;
            skipWhitespace();
            match(',');
        }
        if (!match(')')) {
            error("Expected ')'");
            return std::nullopt;
        }
    }
    
    skipWhitespace();
    
    // Nested selection set
    if (match('{')) {
        while (!peek('}') && pos_ < source_.size()) {
            skipWhitespace();
            auto nestedField = parseField();
            if (nestedField) {
                field.selections.push_back(std::move(*nestedField));
            }
            skipWhitespace();
        }
        if (!match('}')) {
            error("Expected '}'");
            return std::nullopt;
        }
    }
    
    return field;
}

std::optional<std::shared_ptr<Value>> Parser::parseValue() {
    skipWhitespace();
    
    // Null
    if (match("null")) {
        return Value::null();
    }
    
    // Boolean
    if (match("true")) {
        return Value::boolean(true);
    }
    if (match("false")) {
        return Value::boolean(false);
    }
    
    // String
    if (peek('"')) {
        auto str = parseString();
        if (str) {
            return Value::string(std::move(*str));
        }
        return std::nullopt;
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
            return Value::floating(std::stod(numStr));
        } else {
            return Value::integer(std::stoll(numStr));
        }
    }
    
    // List
    if (match('[')) {
        ValueList list;
        while (!peek(']') && pos_ < source_.size()) {
            skipWhitespace();
            auto val = parseValue();
            if (!val) {
                return std::nullopt;
            }
            list.push_back(*val);
            skipWhitespace();
            match(',');
        }
        if (!match(']')) {
            error("Expected ']'");
            return std::nullopt;
        }
        return Value::list(std::move(list));
    }
    
    // Object
    if (match('{')) {
        ValueMap obj;
        while (!peek('}') && pos_ < source_.size()) {
            skipWhitespace();
            auto key = parseName();
            if (!key) {
                error("Expected object key");
                return std::nullopt;
            }
            skipWhitespace();
            if (!match(':')) {
                error("Expected ':' in object");
                return std::nullopt;
            }
            skipWhitespace();
            auto val = parseValue();
            if (!val) {
                return std::nullopt;
            }
            obj[*key] = *val;
            skipWhitespace();
            match(',');
        }
        if (!match('}')) {
            error("Expected '}'");
            return std::nullopt;
        }
        return Value::object(std::move(obj));
    }
    
    // Variable reference ($name)
    if (match('$')) {
        auto name = parseName();
        if (!name) {
            error("Expected variable name after '$'");
            return std::nullopt;
        }
        // Return as special string value (will be resolved at execution)
        return Value::string("$" + *name);
    }
    
    // Enum value (bare name)
    auto enumVal = parseName();
    if (enumVal) {
        return Value::enumValue(std::move(*enumVal));
    }
    
    error("Expected value");
    return std::nullopt;
}

std::optional<VariableDefinition> Parser::parseVariableDefinition() {
    VariableDefinition def;
    
    if (!match('$')) {
        error("Expected '$' for variable definition");
        return std::nullopt;
    }
    
    auto name = parseName();
    if (!name) {
        error("Expected variable name");
        return std::nullopt;
    }
    def.name = *name;
    
    skipWhitespace();
    if (!match(':')) {
        error("Expected ':' after variable name");
        return std::nullopt;
    }
    
    skipWhitespace();
    
    // Type (with optional list and non-null modifiers)
    if (match('[')) {
        def.is_list = true;
        skipWhitespace();
        auto typeName = parseName();
        if (!typeName) {
            error("Expected type name");
            return std::nullopt;
        }
        def.type_name = *typeName;
        skipWhitespace();
        match('!');  // Inner non-null
        skipWhitespace();
        if (!match(']')) {
            error("Expected ']'");
            return std::nullopt;
        }
    } else {
        auto typeName = parseName();
        if (!typeName) {
            error("Expected type name");
            return std::nullopt;
        }
        def.type_name = *typeName;
    }
    
    skipWhitespace();
    if (match('!')) {
        def.is_non_null = true;
    }
    
    skipWhitespace();
    
    // Default value
    if (match('=')) {
        skipWhitespace();
        auto val = parseValue();
        if (!val) {
            return std::nullopt;
        }
        def.default_value = *val;
    }
    
    return def;
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

std::optional<std::string> Parser::parseName() {
    size_t start = pos_;
    
    // Name must start with letter or underscore
    if (pos_ < source_.size() && (std::isalpha(source_[pos_]) || source_[pos_] == '_')) {
        ++pos_;
        ++column_;
        while (pos_ < source_.size() && (std::isalnum(source_[pos_]) || source_[pos_] == '_')) {
            ++pos_;
            ++column_;
        }
        return std::string(source_.substr(start, pos_ - start));
    }
    return std::nullopt;
}

std::optional<std::string> Parser::parseString() {
    if (!match('"')) {
        return std::nullopt;
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
            error("Unterminated string");
            return std::nullopt;
        } else {
            result += source_[pos_];
            ++pos_;
            ++column_;
        }
    }
    
    if (!match('"')) {
        error("Unterminated string");
        return std::nullopt;
    }
    
    return result;
}

void Parser::error(std::string message) {
    ParseError err;
    err.message = std::move(message);
    err.line = line_;
    err.column = column_;
    errors_.push_back(std::move(err));
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
        result.errors.push_back("Operation not found");
        return result;
    }
    
    try {
        result.data = executeOperation(*op, context);
    } catch (const std::exception& e) {
        result.errors.push_back(std::string("Execution error: ") + e.what());
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
    
    addDocumentTypes(schema);
    addGraphTypes(schema);
    addVectorTypes(schema);
    addTimeseriesTypes(schema);
    addQueryType(schema);
    addMutationType(schema);
    
    return schema;
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
