# AQL v1.3.1 OOP Extensions - Implementation Guide

**Datum:** 22. Dezember 2025  
**Version:** v1.3.1 (479 Sprachumfang: 119 Keywords + 360 Funktionen)  
**Zweck:** Technischer Implementierungsleitfaden für die Integration der OOP-Erweiterungen in die ThemisDB-Codebasis  
**Zielgruppe:** C++ Entwickler, die den AQL-Parser und -Compiler erweitern

---

## Executive Summary

Dieser Leitfaden beschreibt die **konkrete Code-Implementierung** der AQL v1.3.1 OOP-Erweiterungen in der bestehenden ThemisDB-Codebasis. Er ergänzt die Spezifikationsdokumente mit präzisen Anweisungen für:

- **Lexer/Tokenizer-Erweiterung** (47 neue Keywords)
- **Parser-Grammatik-Erweiterung** (neue AST-Nodes)
- **Type-System-Implementation** (Type-Checker, Type-Inference)
- **Namespace-Resolution** (Symbol-Tables, Scope-Management)
- **Vision-Command-Handler** (llama.cpp vision Integration)

**Geschätzte Implementierungszeit:** 8-12 Wochen (phasenweise)

---

## 1. Übersicht der betroffenen Codebereiche

### 1.1 Kerndateien für Erweiterung

| Datei | Pfad | Zweck | Änderungsumfang |
|-------|------|-------|-----------------|
| **aql_parser.cpp** | `/src/query/aql_parser.cpp` | Lexer & Parser | **Hoch** - 47 neue Keywords, neue AST-Nodes |
| **aql_parser.h** | `/include/query/aql_parser.h` | Parser-Interface | **Mittel** - neue Datenstrukturen |
| **aql_translator.cpp** | `/src/query/aql_translator.cpp` | AST → Execution | **Hoch** - neue Node-Typen übersetzen |
| **llm_aql_handler.cpp** | `/src/aql/llm_aql_handler.cpp` | LLM-Kommandos | **Mittel** - Vision-Befehle hinzufügen |
| **llm_aql_handler.h** | `/include/aql/llm_aql_handler.h` | LLM-Interface | **Mittel** - Vision-Methoden |

### 1.2 Neue Dateien erstellen

| Datei | Pfad | Zweck |
|-------|------|-------|
| **aql_type_checker.h** | `/include/query/aql_type_checker.h` | Type-System Interface |
| **aql_type_checker.cpp** | `/src/query/aql_type_checker.cpp` | Type-Checking-Logik |
| **aql_namespace_resolver.h** | `/include/query/aql_namespace_resolver.h` | Namespace-Resolution |
| **aql_namespace_resolver.cpp** | `/src/query/aql_namespace_resolver.cpp` | Symbol-Table-Management |
| **aql_vision_handler.h** | `/include/aql/aql_vision_handler.h` | Vision-spezifische Befehle |
| **aql_vision_handler.cpp** | `/src/aql/aql_vision_handler.cpp` | Vision-Command-Ausführung |

---

## 2. Phase 1: Lexer/Tokenizer-Erweiterung (Woche 1-2)

### 2.1 TokenType Enum erweitern

**Datei:** `/src/query/aql_parser.cpp` (Zeilen 15-49)

**Aktuelle Struktur:**
```cpp
enum class TokenType {
    // Keywords (v1.3.0 - aktuell 72)
    FOR, IN, FILTER, SORT, LIMIT, RETURN, LET,
    ASC, DESC, AND, OR, XOR, NOT,
    // ... weitere v1.3.0 Keywords ...
};
```

**Zu ergänzen (47 neue Keywords):**

```cpp
enum class TokenType {
    // ============= v1.3.0 Keywords (72) =============
    FOR, IN, FILTER, SORT, LIMIT, RETURN, LET,
    ASC, DESC, AND, OR, XOR, NOT,
    GRAPH, OUTBOUND, INBOUND, ANY,
    TYPE,  // bereits vorhanden
    COLLECT, AGGREGATE,
    TRUE, FALSE, NULL_LITERAL,
    SIMILARITY, PROXIMITY, SHORTEST_PATH, TO,
    WITH, AS, ALL, SATISFIES,
    // ... restliche v1.3.0 Keywords ...
    
    // ============= v1.3.1 OOP Extensions (47 neue) =============
    
    // Namespace System (2)
    NAMESPACE,              // NAMESPACE themis.vision
    IMPORT,                 // IMPORT themis.llm.*
    
    // Type System (6)
    // TYPE bereits vorhanden in v1.3.0
    STRING_TYPE,            // String
    INT_TYPE,               // Int
    FLOAT_TYPE,             // Float
    BOOL_TYPE,              // Bool
    ANY_TYPE,               // Any
    
    // Functions & Classes (7)
    FUNCTION,               // FUNCTION name(params) -> RetType
    CLASS,                  // CLASS ClassName
    PUBLIC,                 // PUBLIC visibility
    PRIVATE,                // PRIVATE visibility
    CONSTRUCTOR,            // CONSTRUCTOR for classes
    METHOD,                 // METHOD in classes
    NEW,                    // NEW for instantiation
    
    // References (3)
    THIS,                   // THIS reference
    SELF,                   // SELF reference
    EXTENDS,                // EXTENDS for inheritance
    
    // Control Flow (10)
    IF,                     // IF condition
    THEN,                   // THEN branch
    ELSE,                   // ELSE branch
    ELSEIF,                 // ELSEIF branch
    ENDIF,                  // ENDIF terminator
    TRY,                    // TRY block
    CATCH,                  // CATCH handler
    THROW,                  // THROW exception
    CASE,                   // CASE for pattern match
    END,                    // END terminator
    
    // Pattern Matching (2)
    MATCH,                  // MATCH expression
    WHEN,                   // WHEN case
    
    // Async Operations (4)
    ASYNC,                  // ASYNC function
    AWAIT,                  // AWAIT promise
    PARALLEL,               // PARALLEL execution
    TIMEOUT,                // TIMEOUT limit
    
    // Vision Commands (9)
    VISION,                 // LLM VISION command
    ANALYZE,                // VISION ANALYZE
    DETECT,                 // DETECT features
    QUESTION,               // VISION QUESTION
    ABOUT,                  // ABOUT context
    IMAGE,                  // IMAGE reference
    IMAGES,                 // IMAGES batch
    TRANSFORM,              // TRANSFORM operation
    COMPARE,                // COMPARE images
    // Hinweis: BATCH, OPERATIONS, OUTPUT, METRIC sind möglicherweise
    // bereits vorhanden oder können als IDENTIFIER behandelt werden
    
    // Additional Type Keywords (4)
    ARRAY_TYPE,             // Array<T>
    MAP_TYPE,               // Map<K,V>
    RESULT_TYPE,            // Result<T,E>
    OBJECT_TYPE,            // Object
    
    // Macro System (1)
    MACRO,                  // MACRO for code generation
    
    // ============= Existing Operators & Literals =============
    EQ, NEQ, LT, LTE, GT, GTE,
    PLUS, MINUS, STAR, SLASH, MODULO,
    ASSIGN, ARROW,  // ARROW für -> in Funktionssignaturen
    
    IDENTIFIER, STRING, INTEGER, FLOAT,
    DOT, COMMA, COLON, SEMICOLON,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    PIPE,  // für Pipeline-Operator |>
    
    END_OF_FILE, INVALID
};
```

### 2.2 Keyword-Map aktualisieren

**Datei:** `/src/query/aql_parser.cpp` (Funktion `isKeyword()` oder Keyword-Map)

**Zu ergänzen:**
```cpp
// Erweitern Sie die bestehende Keyword-Map
static const std::unordered_map<std::string, TokenType> keywords = {
    // ============= v1.3.0 Keywords =============
    {"FOR", TokenType::FOR},
    {"IN", TokenType::IN},
    {"FILTER", TokenType::FILTER},
    // ... alle bestehenden Keywords ...
    
    // ============= v1.3.1 neue Keywords =============
    {"NAMESPACE", TokenType::NAMESPACE},
    {"IMPORT", TokenType::IMPORT},
    {"String", TokenType::STRING_TYPE},
    {"Int", TokenType::INT_TYPE},
    {"Float", TokenType::FLOAT_TYPE},
    {"Bool", TokenType::BOOL_TYPE},
    {"Any", TokenType::ANY_TYPE},
    {"FUNCTION", TokenType::FUNCTION},
    {"CLASS", TokenType::CLASS},
    {"PUBLIC", TokenType::PUBLIC},
    {"PRIVATE", TokenType::PRIVATE},
    {"CONSTRUCTOR", TokenType::CONSTRUCTOR},
    {"METHOD", TokenType::METHOD},
    {"NEW", TokenType::NEW},
    {"THIS", TokenType::THIS},
    {"SELF", TokenType::SELF},
    {"EXTENDS", TokenType::EXTENDS},
    {"IF", TokenType::IF},
    {"THEN", TokenType::THEN},
    {"ELSE", TokenType::ELSE},
    {"ELSEIF", TokenType::ELSEIF},
    {"ENDIF", TokenType::ENDIF},
    {"TRY", TokenType::TRY},
    {"CATCH", TokenType::CATCH},
    {"THROW", TokenType::THROW},
    {"CASE", TokenType::CASE},
    {"END", TokenType::END},
    {"MATCH", TokenType::MATCH},
    {"WHEN", TokenType::WHEN},
    {"ASYNC", TokenType::ASYNC},
    {"AWAIT", TokenType::AWAIT},
    {"PARALLEL", TokenType::PARALLEL},
    {"TIMEOUT", TokenType::TIMEOUT},
    {"VISION", TokenType::VISION},
    {"ANALYZE", TokenType::ANALYZE},
    {"DETECT", TokenType::DETECT},
    {"QUESTION", TokenType::QUESTION},
    {"ABOUT", TokenType::ABOUT},
    {"IMAGE", TokenType::IMAGE},
    {"IMAGES", TokenType::IMAGES},
    {"TRANSFORM", TokenType::TRANSFORM},
    {"COMPARE", TokenType::COMPARE},
    {"Array", TokenType::ARRAY_TYPE},
    {"Map", TokenType::MAP_TYPE},
    {"Result", TokenType::RESULT_TYPE},
    {"Object", TokenType::OBJECT_TYPE},
    {"MACRO", TokenType::MACRO},
};
```

### 2.3 Pipeline-Operator (|>) Unterstützung

**In der `nextToken()` Methode:**
```cpp
Token Tokenizer::nextToken() {
    // ... bestehender Code ...
    
    // Pipeline-Operator |>
    if (ch == '|') {
        if (peek(1) == '>') {
            advance();  // consume '|'
            advance();  // consume '>'
            return Token(TokenType::PIPE_OPERATOR, "|>", line, col);
        }
        // ... bestehende Logik für '|' ...
    }
    
    // ... restlicher Code ...
}
```

---

## 3. Phase 2: AST-Erweiterung (Woche 2-4)

### 3.1 Neue AST-Node-Typen definieren

**Datei:** `/include/query/aql_parser.h`

**Zu ergänzen:**

```cpp
namespace themis {
namespace query {

// ============= Bestehende AST-Strukturen =============
// (bleiben unverändert)

// ============= v1.3.1 neue AST-Node-Typen =============

// Namespace Declaration
struct NamespaceNode {
    std::string namespace_path;  // z.B. "themis.vision.analysis"
    std::vector<std::unique_ptr<ASTNode>> body;
};

// Import Statement
struct ImportNode {
    std::string import_path;     // z.B. "themis.llm.*"
    bool is_wildcard;            // true für ".*" Import
    std::vector<std::string> symbols;  // spezifische Symbole bei nicht-wildcard
};

// User-Defined Type
struct TypeDefNode {
    std::string type_name;
    std::vector<std::pair<std::string, std::string>> fields;  // (name, type)
    std::vector<std::string> generic_params;  // für generische Typen
};

// Function Definition
struct FunctionDefNode {
    std::string function_name;
    std::vector<std::pair<std::string, std::string>> parameters;  // (name, type)
    std::string return_type;
    std::vector<std::unique_ptr<ASTNode>> body;
    bool is_async;
};

// Class Definition
struct ClassDefNode {
    std::string class_name;
    std::string extends_class;  // Basisklasse (optional)
    std::vector<std::unique_ptr<TypeDefNode>> fields;
    std::vector<std::unique_ptr<FunctionDefNode>> methods;
    std::unique_ptr<FunctionDefNode> constructor;
};

// If-Then-Else Statement
struct IfNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> then_body;
    std::vector<std::pair<std::unique_ptr<ASTNode>, 
                          std::vector<std::unique_ptr<ASTNode>>>> elif_branches;
    std::vector<std::unique_ptr<ASTNode>> else_body;
};

// Try-Catch Statement
struct TryCatchNode {
    std::vector<std::unique_ptr<ASTNode>> try_body;
    std::string exception_var;
    std::vector<std::unique_ptr<ASTNode>> catch_body;
};

// Match Expression (Pattern Matching)
struct MatchNode {
    std::unique_ptr<ASTNode> match_expr;
    std::vector<std::pair<std::unique_ptr<ASTNode>,  // pattern
                          std::unique_ptr<ASTNode>>> cases;  // result
    std::unique_ptr<ASTNode> default_case;
};

// Pipeline Expression
struct PipelineNode {
    std::vector<std::unique_ptr<ASTNode>> stages;
};

// Vision Command
struct VisionCommandNode {
    enum class VisionCommandType {
        ANALYZE,    // LLM VISION ANALYZE
        QUESTION,   // LLM VISION QUESTION
        RAG,        // LLM VISION RAG
        COMPARE,    // LLM VISION COMPARE
        BATCH       // LLM VISION BATCH
    };
    
    VisionCommandType command_type;
    std::string image_path;
    std::vector<std::string> detect_types;  // als Strings: "objects", "text", etc.
    std::string model_id;
    std::string question_text;
    std::string collection_name;
    std::unordered_map<std::string, std::string> options;
};

// Async/Await
struct AsyncNode {
    std::unique_ptr<FunctionDefNode> async_function;
};

struct AwaitNode {
    std::unique_ptr<ASTNode> async_expr;
};

} // namespace query
} // namespace themis
```

### 3.2 Parser-Methoden erweitern

**Datei:** `/src/query/aql_parser.cpp`

**Neue Parse-Methoden hinzufügen:**

```cpp
class AQLParser {
private:
    // Bestehende Methoden...
    
    // ============= v1.3.1 neue Parser-Methoden =============
    
    std::unique_ptr<NamespaceNode> parseNamespace() {
        expect(TokenType::NAMESPACE);
        auto ns = std::make_unique<NamespaceNode>();
        
        // Parse dotted namespace path: themis.vision.analysis
        ns->namespace_path = expectIdentifier();
        while (match(TokenType::DOT)) {
            ns->namespace_path += "." + expectIdentifier();
        }
        
        // Parse namespace body
        // Implementation depends on your design
        
        return ns;
    }
    
    std::unique_ptr<ImportNode> parseImport() {
        expect(TokenType::IMPORT);
        auto import = std::make_unique<ImportNode>();
        
        import->import_path = expectIdentifier();
        while (match(TokenType::DOT)) {
            if (match(TokenType::STAR)) {
                import->is_wildcard = true;
                break;
            }
            import->import_path += "." + expectIdentifier();
        }
        
        return import;
    }
    
    std::unique_ptr<TypeDefNode> parseTypeDef() {
        expect(TokenType::TYPE);
        auto typedef_node = std::make_unique<TypeDefNode>();
        
        typedef_node->type_name = expectIdentifier();
        
        // Parse generic parameters: <T, U>
        if (match(TokenType::LT)) {
            do {
                typedef_node->generic_params.push_back(expectIdentifier());
            } while (match(TokenType::COMMA));
            expect(TokenType::GT);
        }
        
        // Parse struct body: { field1: Type1, field2: Type2 }
        expect(TokenType::LBRACE);
        while (!check(TokenType::RBRACE)) {
            std::string field_name = expectIdentifier();
            expect(TokenType::COLON);
            std::string field_type = parseTypeName();
            typedef_node->fields.emplace_back(field_name, field_type);
            
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::RBRACE);
        
        return typedef_node;
    }
    
    std::unique_ptr<FunctionDefNode> parseFunctionDef() {
        bool is_async = match(TokenType::ASYNC);
        expect(TokenType::FUNCTION);
        
        auto func = std::make_unique<FunctionDefNode>();
        func->is_async = is_async;
        func->function_name = expectIdentifier();
        
        // Parse parameters
        expect(TokenType::LPAREN);
        while (!check(TokenType::RPAREN)) {
            std::string param_name = expectIdentifier();
            expect(TokenType::COLON);
            std::string param_type = parseTypeName();
            
            // Default value support
            if (match(TokenType::ASSIGN)) {
                // Parse default value (store in extended structure)
            }
            
            func->parameters.emplace_back(param_name, param_type);
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::RPAREN);
        
        // Parse return type: -> ReturnType
        if (match(TokenType::ARROW)) {
            func->return_type = parseTypeName();
        }
        
        // Parse body
        expect(TokenType::LBRACE);
        while (!check(TokenType::RBRACE)) {
            func->body.push_back(parseStatement());
        }
        expect(TokenType::RBRACE);
        
        return func;
    }
    
    std::unique_ptr<IfNode> parseIf() {
        expect(TokenType::IF);
        auto if_node = std::make_unique<IfNode>();
        
        if_node->condition = parseExpression();
        expect(TokenType::THEN);
        
        // Parse THEN body
        while (!check(TokenType::ELSEIF) && !check(TokenType::ELSE) && !check(TokenType::ENDIF)) {
            if_node->then_body.push_back(parseStatement());
        }
        
        // Parse ELSEIF branches
        while (match(TokenType::ELSEIF)) {
            auto elif_cond = parseExpression();
            expect(TokenType::THEN);
            std::vector<std::unique_ptr<ASTNode>> elif_body;
            while (!check(TokenType::ELSEIF) && !check(TokenType::ELSE) && !check(TokenType::ENDIF)) {
                elif_body.push_back(parseStatement());
            }
            if_node->elif_branches.emplace_back(std::move(elif_cond), std::move(elif_body));
        }
        
        // Parse ELSE body
        if (match(TokenType::ELSE)) {
            while (!check(TokenType::ENDIF)) {
                if_node->else_body.push_back(parseStatement());
            }
        }
        
        expect(TokenType::ENDIF);
        return if_node;
    }
    
    std::unique_ptr<PipelineNode> parsePipeline(std::unique_ptr<ASTNode> initial_expr) {
        auto pipeline = std::make_unique<PipelineNode>();
        pipeline->stages.push_back(std::move(initial_expr));
        
        while (match(TokenType::PIPE_OPERATOR)) {
            pipeline->stages.push_back(parseExpression());
        }
        
        return pipeline;
    }
    
    std::unique_ptr<VisionCommandNode> parseVisionCommand() {
        expect(TokenType::VISION);
        auto vision = std::make_unique<VisionCommandNode>();
        
        // Determine command type
        if (match(TokenType::ANALYZE)) {
            vision->command_type = VisionCommandNode::VisionCommandType::ANALYZE;
            vision->image_path = expectString();
            
            // USING MODEL
            if (match(TokenType::USING)) {
                expect(TokenType::MODEL);
                vision->model_id = expectString();
            }
            
            // DETECT ['objects', 'text', 'faces']
            if (match(TokenType::DETECT)) {
                expect(TokenType::LBRACKET);
                do {
                    vision->detect_types.push_back(expectString());
                } while (match(TokenType::COMMA));
                expect(TokenType::RBRACKET);
            }
            
        } else if (match(TokenType::QUESTION)) {
            vision->command_type = VisionCommandNode::VisionCommandType::QUESTION;
            vision->question_text = expectString();
            
            expect(TokenType::ABOUT);
            expect(TokenType::IMAGE);
            vision->image_path = expectString();
            
            if (match(TokenType::USING)) {
                expect(TokenType::MODEL);
                vision->model_id = expectString();
            }
            
        } else if (match(TokenType::RAG)) {
            vision->command_type = VisionCommandNode::VisionCommandType::RAG;
            vision->question_text = expectString();
            
            expect(TokenType::FROM);
            expect(TokenType::COLLECTION);
            vision->collection_name = expectIdentifier();
            
            expect(TokenType::WITH);
            expect(TokenType::IMAGE);
            vision->image_path = expectString();
            
            if (match(TokenType::USING)) {
                expect(TokenType::MODEL);
                vision->model_id = expectString();
            }
        }
        
        return vision;
    }
    
    std::string parseTypeName() {
        // Parse type names: Int, String, Array<T>, Map<K,V>, etc.
        std::string type_name = expectIdentifier();
        
        // Generic types
        if (match(TokenType::LT)) {
            type_name += "<";
            do {
                type_name += parseTypeName();
            } while (match(TokenType::COMMA) && (type_name += ",", true));
            expect(TokenType::GT);
            type_name += ">";
        }
        
        return type_name;
    }
};
```

---

## 4. Phase 3: Type-System-Implementation (Woche 4-6)

### 4.1 Type-Checker Interface erstellen

**Neue Datei:** `/include/query/aql_type_checker.h`

```cpp
#pragma once

#include "query/aql_parser.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>

namespace themis {
namespace query {

class TypeChecker {
public:
    struct TypeInfo {
        std::string type_name;
        bool is_generic;
        std::vector<std::string> generic_params;
        std::unordered_map<std::string, std::string> fields;  // für struct types
    };
    
    TypeChecker();
    ~TypeChecker();
    
    // Register user-defined types
    void registerType(const TypeDefNode& type_def);
    
    // Type checking
    bool checkType(const ASTNode& node, const std::string& expected_type);
    std::optional<std::string> inferType(const ASTNode& node);
    bool isAssignable(const std::string& from_type, const std::string& to_type);
    
    // Built-in types
    bool isBuiltinType(const std::string& type_name) const;
    
    // Error reporting
    struct TypeError {
        std::string message;
        size_t line;
        size_t column;
    };
    
    const std::vector<TypeError>& getErrors() const { return errors_; }
    
private:
    std::unordered_map<std::string, TypeInfo> type_registry_;
    std::vector<TypeError> errors_;
    
    void initBuiltinTypes();
};

} // namespace query
} // namespace themis
```

### 4.2 Type-Checker Implementation

**Neue Datei:** `/src/query/aql_type_checker.cpp`

```cpp
#include "query/aql_type_checker.h"

namespace themis {
namespace query {

TypeChecker::TypeChecker() {
    initBuiltinTypes();
}

void TypeChecker::initBuiltinTypes() {
    // Register built-in types
    type_registry_["Int"] = TypeInfo{"Int", false, {}, {}};
    type_registry_["Float"] = TypeInfo{"Float", false, {}, {}};
    type_registry_["String"] = TypeInfo{"String", false, {}, {}};
    type_registry_["Bool"] = TypeInfo{"Bool", false, {}, {}};
    type_registry_["Any"] = TypeInfo{"Any", false, {}, {}};
    
    // Generic types
    type_registry_["Array"] = TypeInfo{"Array", true, {"T"}, {}};
    type_registry_["Map"] = TypeInfo{"Map", true, {"K", "V"}, {}};
    type_registry_["Result"] = TypeInfo{"Result", true, {"T", "E"}, {}};
}

void TypeChecker::registerType(const TypeDefNode& type_def) {
    TypeInfo info;
    info.type_name = type_def.type_name;
    info.is_generic = !type_def.generic_params.empty();
    info.generic_params = type_def.generic_params;
    
    for (const auto& [field_name, field_type] : type_def.fields) {
        info.fields[field_name] = field_type;
    }
    
    type_registry_[type_def.type_name] = std::move(info);
}

bool TypeChecker::isBuiltinType(const std::string& type_name) const {
    static const std::unordered_set<std::string> builtins = {
        "Int", "Float", "String", "Bool", "Any",
        "Array", "Map", "Result", "Object"
    };
    return builtins.count(type_name) > 0;
}

std::optional<std::string> TypeChecker::inferType(const ASTNode& node) {
    // Type inference logic
    // Visit different node types and infer their types
    // This is a simplified example
    
    // TODO: Implement visitor pattern for all AST node types
    return std::nullopt;
}

bool TypeChecker::isAssignable(const std::string& from_type, const std::string& to_type) {
    // Type compatibility checking
    if (from_type == to_type) return true;
    if (to_type == "Any") return true;
    
    // Add more sophisticated type compatibility rules
    // e.g., Int can be assigned to Float
    if (from_type == "Int" && to_type == "Float") return true;
    
    return false;
}

} // namespace query
} // namespace themis
```

---

## 5. Phase 4: Namespace-Resolution (Woche 6-8)

### 5.1 Namespace-Resolver Interface

**Neue Datei:** `/include/query/aql_namespace_resolver.h`

```cpp
#pragma once

#include "query/aql_parser.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace query {

class NamespaceResolver {
public:
    struct Symbol {
        enum class Kind {
            TYPE,
            FUNCTION,
            VARIABLE,
            CLASS
        };
        
        std::string name;
        std::string qualified_name;  // full namespace path
        Kind kind;
        void* definition;  // pointer to AST node
    };
    
    struct Scope {
        std::string namespace_path;
        Scope* parent;
        std::unordered_map<std::string, Symbol> symbols;
        std::vector<std::string> imports;
    };
    
    NamespaceResolver();
    ~NamespaceResolver();
    
    // Scope management
    void enterScope(const std::string& namespace_path);
    void exitScope();
    Scope* currentScope();
    
    // Symbol registration
    void registerSymbol(const std::string& name, Symbol::Kind kind, void* definition);
    
    // Symbol lookup
    Symbol* resolveSymbol(const std::string& name);
    Symbol* resolveQualifiedName(const std::string& qualified_name);
    
    // Import handling
    void addImport(const std::string& import_path, bool is_wildcard);
    
    // Error reporting
    struct ResolutionError {
        std::string message;
        std::string symbol_name;
    };
    
    const std::vector<ResolutionError>& getErrors() const { return errors_; }
    
private:
    Scope* current_scope_;
    Scope* root_scope_;
    std::vector<std::unique_ptr<Scope>> all_scopes_;
    std::vector<ResolutionError> errors_;
    
    Symbol* lookupInScope(Scope* scope, const std::string& name);
    Symbol* lookupViaImports(Scope* scope, const std::string& name);
};

} // namespace query
} // namespace themis
```

### 5.2 Namespace-Resolver Implementation

**Neue Datei:** `/src/query/aql_namespace_resolver.cpp`

```cpp
#include "query/aql_namespace_resolver.h"

namespace themis {
namespace query {

NamespaceResolver::NamespaceResolver() {
    root_scope_ = new Scope{"", nullptr, {}, {}};
    current_scope_ = root_scope_;
    all_scopes_.emplace_back(root_scope_);
}

NamespaceResolver::~NamespaceResolver() {
    // Scopes cleaned up by unique_ptr
}

void NamespaceResolver::enterScope(const std::string& namespace_path) {
    auto new_scope = std::make_unique<Scope>();
    new_scope->namespace_path = namespace_path;
    new_scope->parent = current_scope_;
    
    current_scope_ = new_scope.get();
    all_scopes_.push_back(std::move(new_scope));
}

void NamespaceResolver::exitScope() {
    if (current_scope_->parent) {
        current_scope_ = current_scope_->parent;
    }
}

Scope* NamespaceResolver::currentScope() {
    return current_scope_;
}

void NamespaceResolver::registerSymbol(const std::string& name, Symbol::Kind kind, void* definition) {
    Symbol symbol;
    symbol.name = name;
    symbol.qualified_name = current_scope_->namespace_path.empty() 
        ? name 
        : current_scope_->namespace_path + "." + name;
    symbol.kind = kind;
    symbol.definition = definition;
    
    current_scope_->symbols[name] = symbol;
}

Symbol* NamespaceResolver::resolveSymbol(const std::string& name) {
    // Check if it's a qualified name
    if (name.find('.') != std::string::npos) {
        return resolveQualifiedName(name);
    }
    
    // Search in current scope and parent scopes
    Scope* scope = current_scope_;
    while (scope) {
        auto* symbol = lookupInScope(scope, name);
        if (symbol) return symbol;
        
        // Check imports
        symbol = lookupViaImports(scope, name);
        if (symbol) return symbol;
        
        scope = scope->parent;
    }
    
    errors_.push_back({"Symbol not found", name});
    return nullptr;
}

Symbol* NamespaceResolver::resolveQualifiedName(const std::string& qualified_name) {
    // Search all scopes for qualified name
    for (const auto& scope_ptr : all_scopes_) {
        for (const auto& [name, symbol] : scope_ptr->symbols) {
            if (symbol.qualified_name == qualified_name) {
                return const_cast<Symbol*>(&symbol);
            }
        }
    }
    
    errors_.push_back({"Qualified symbol not found", qualified_name});
    return nullptr;
}

void NamespaceResolver::addImport(const std::string& import_path, bool is_wildcard) {
    current_scope_->imports.push_back(import_path + (is_wildcard ? ".*" : ""));
}

Symbol* NamespaceResolver::lookupInScope(Scope* scope, const std::string& name) {
    auto it = scope->symbols.find(name);
    if (it != scope->symbols.end()) {
        return &it->second;
    }
    return nullptr;
}

Symbol* NamespaceResolver::lookupViaImports(Scope* scope, const std::string& name) {
    for (const std::string& import : scope->imports) {
        if (import.ends_with(".*")) {
            // Wildcard import
            std::string namespace_prefix = import.substr(0, import.size() - 2);
            std::string qualified_name = namespace_prefix + "." + name;
            auto* symbol = resolveQualifiedName(qualified_name);
            if (symbol) return symbol;
        } else {
            // Direct import
            if (import.ends_with("." + name) || import == name) {
                auto* symbol = resolveQualifiedName(import);
                if (symbol) return symbol;
            }
        }
    }
    return nullptr;
}

} // namespace query
} // namespace themis
```

---

## 6. Phase 5: Vision-Command-Handler (Woche 8-10)

### 6.1 Vision-Handler Interface erweitern

**Datei:** `/include/aql/aql_vision_handler.h` (NEU)

```cpp
#pragma once

#include "llm/llm_plugin_interface.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace themis {
namespace aql {

/**
 * @brief Handler for Vision-specific AQL commands (v1.3.1)
 * 
 * Extends LLM capabilities with llama.cpp vision integration:
 * - LLM VISION ANALYZE: Analyze image with detection types
 * - LLM VISION QUESTION: Ask questions about images
 * - LLM VISION RAG: Multimodal RAG with images
 * - LLM VISION COMPARE: Compare multiple images
 * - LLM VISION BATCH: Batch image processing
 */
class VisionHandler {
public:
    VisionHandler();
    ~VisionHandler();
    
    // Vision ANALYZE command
    struct AnalyzeResult {
        std::string scene_description;
        std::vector<std::string> detected_objects;
        std::vector<std::string> detected_text;
        std::vector<std::string> detected_faces;
        std::unordered_map<std::string, std::string> additional_detections;
    };
    
    AnalyzeResult executeAnalyze(
        const std::string& image_path,
        const std::vector<std::string>& detect_types,  // ["objects", "text", "faces"]
        const std::string& model_id = "llava-7b",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
    // Vision QUESTION command
    std::string executeQuestion(
        const std::string& question,
        const std::string& image_path,
        const std::string& model_id = "llava-7b",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
    // Vision RAG command
    std::string executeVisionRAG(
        const std::string& query,
        const std::string& collection_name,
        const std::string& image_path,
        int top_k = 5,
        const std::string& model_id = "llava-7b",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
    // Vision COMPARE command
    struct CompareResult {
        std::string comparison_summary;
        std::vector<std::string> similarities;
        std::vector<std::string> differences;
        float similarity_score;
    };
    
    CompareResult executeCompare(
        const std::vector<std::string>& image_paths,
        const std::string& model_id = "llava-7b",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
    // Vision BATCH command
    std::vector<AnalyzeResult> executeBatch(
        const std::vector<std::string>& image_paths,
        const std::vector<std::string>& detect_types,
        const std::string& model_id = "llava-7b",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
```

### 6.2 LLMAQLHandler erweitern

**Datei:** `/include/aql/llm_aql_handler.h`

**Zu ergänzen:**
```cpp
// In der LLMAQLHandler-Klasse:

public:
    // ============= v1.3.1 Vision Commands =============
    
    // Vision-Handler Integration
    std::shared_ptr<VisionHandler> getVisionHandler();
    
    // Convenience methods for vision commands
    std::string executeVisionAnalyze(
        const std::string& image_path,
        const std::vector<std::string>& detect_types,
        const std::string& model_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
    std::string executeVisionQuestion(
        const std::string& question,
        const std::string& image_path,
        const std::string& model_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );
    
    std::string executeVisionRAG(
        const std::string& query,
        const std::string& collection,
        const std::string& image_path,
        int top_k = 5,
        const std::string& model_id = "",
        const std::unordered_map<std::string, std::string>& options = {}
    );

private:
    std::shared_ptr<VisionHandler> vision_handler_;
```

---

## 7. Phase 6: Translator-Integration (Woche 10-12)

### 7.1 AQL-Translator erweitern

**Datei:** `/src/query/aql_translator.cpp`

**Neue Translate-Methoden hinzufügen:**

```cpp
// In der AQLTranslator-Klasse:

void AQLTranslator::translate(const NamespaceNode& node) {
    // Enter namespace scope
    namespace_resolver_->enterScope(node.namespace_path);
    
    // Translate all declarations in namespace
    for (const auto& decl : node.body) {
        translateNode(*decl);
    }
    
    namespace_resolver_->exitScope();
}

void AQLTranslator::translate(const TypeDefNode& node) {
    // Register type with type checker
    type_checker_->registerType(node);
    
    // Register symbol with namespace resolver
    namespace_resolver_->registerSymbol(
        node.type_name,
        NamespaceResolver::Symbol::Kind::TYPE,
        const_cast<TypeDefNode*>(&node)
    );
}

void AQLTranslator::translate(const FunctionDefNode& node) {
    // Type-check function signature
    for (const auto& [param_name, param_type] : node.parameters) {
        if (!type_checker_->isBuiltinType(param_type)) {
            // Verify custom type exists
        }
    }
    
    // Register function symbol
    namespace_resolver_->registerSymbol(
        node.function_name,
        NamespaceResolver::Symbol::Kind::FUNCTION,
        const_cast<FunctionDefNode*>(&node)
    );
    
    // Translate function body
    for (const auto& stmt : node.body) {
        translateNode(*stmt);
    }
}

void AQLTranslator::translate(const VisionCommandNode& node) {
    // Execute vision command via LLMAQLHandler
    auto llm_handler = getLLMHandler();
    
    switch (node.command_type) {
        case VisionCommandNode::VisionCommandType::ANALYZE:
            llm_handler->executeVisionAnalyze(
                node.image_path,
                node.detect_types,
                node.model_id,
                node.options
            );
            break;
            
        case VisionCommandNode::VisionCommandType::QUESTION:
            llm_handler->executeVisionQuestion(
                node.question_text,
                node.image_path,
                node.model_id,
                node.options
            );
            break;
            
        case VisionCommandNode::VisionCommandType::RAG:
            llm_handler->executeVisionRAG(
                node.question_text,
                node.collection_name,
                node.image_path,
                5,  // top_k default
                node.model_id,
                node.options
            );
            break;
            
        // ... weitere Vision-Kommandos ...
    }
}

void AQLTranslator::translate(const PipelineNode& node) {
    // Pipeline execution: value |> func1 |> func2 |> ...
    
    // Start with initial value
    auto result = evaluateExpression(*node.stages[0]);
    
    // Apply each pipeline stage
    for (size_t i = 1; i < node.stages.size(); ++i) {
        // Replace placeholder '_' with result
        result = applyPipelineStage(result, *node.stages[i]);
    }
    
    return result;
}
```

---

## 8. Testing & Validation (Woche 12+)

### 8.1 Unit-Tests erstellen

**Neue Datei:** `/tests/test_aql_v1_3_1.cpp`

```cpp
#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/aql_type_checker.h"
#include "query/aql_namespace_resolver.h"

using namespace themis::query;

// ============= Tokenizer Tests =============

TEST(AQLv131, TokenizeNamespace) {
    std::string query = "NAMESPACE themis.vision.analysis";
    Tokenizer tokenizer(query);
    auto tokens = tokenizer.tokenize();
    
    ASSERT_EQ(tokens[0].type, TokenType::NAMESPACE);
    ASSERT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    ASSERT_EQ(tokens[1].value, "themis");
    ASSERT_EQ(tokens[2].type, TokenType::DOT);
    ASSERT_EQ(tokens[3].type, TokenType::IDENTIFIER);
    ASSERT_EQ(tokens[3].value, "vision");
}

TEST(AQLv131, TokenizePipelineOperator) {
    std::string query = "value |> func1(_) |> func2(_)";
    Tokenizer tokenizer(query);
    auto tokens = tokenizer.tokenize();
    
    bool found_pipeline = false;
    for (const auto& token : tokens) {
        if (token.type == TokenType::PIPE_OPERATOR) {
            found_pipeline = true;
            break;
        }
    }
    ASSERT_TRUE(found_pipeline);
}

// ============= Parser Tests =============

TEST(AQLv131, ParseTypeDef) {
    std::string query = R"(
        TYPE VisionAnalysis {
            objects: Array<String>,
            description: String
        }
    )";
    
    AQLParser parser(query);
    auto ast = parser.parse();
    
    // Verify type definition was parsed
    // ASSERT_TRUE(ast contains TypeDefNode);
}

TEST(AQLv131, ParseFunctionDef) {
    std::string query = R"(
        FUNCTION analyze_image(path: String, model: String = 'llava-7b') -> VisionAnalysis {
            RETURN LLM VISION ANALYZE path USING MODEL model
        }
    )";
    
    AQLParser parser(query);
    auto ast = parser.parse();
    
    // Verify function definition was parsed
}

TEST(AQLv131, ParseVisionAnalyze) {
    std::string query = R"(
        LLM VISION ANALYZE 'image.jpg'
            USING MODEL 'llava-7b'
            DETECT ['objects', 'text', 'faces']
    )";
    
    AQLParser parser(query);
    auto ast = parser.parse();
    
    // Verify vision command was parsed
}

// ============= Type-Checker Tests =============

TEST(AQLv131, TypeCheckBuiltins) {
    TypeChecker checker;
    ASSERT_TRUE(checker.isBuiltinType("Int"));
    ASSERT_TRUE(checker.isBuiltinType("String"));
    ASSERT_TRUE(checker.isBuiltinType("Array"));
    ASSERT_FALSE(checker.isBuiltinType("MyCustomType"));
}

TEST(AQLv131, TypeCheckAssignability) {
    TypeChecker checker;
    ASSERT_TRUE(checker.isAssignable("Int", "Int"));
    ASSERT_TRUE(checker.isAssignable("Int", "Float"));
    ASSERT_TRUE(checker.isAssignable("String", "Any"));
    ASSERT_FALSE(checker.isAssignable("String", "Int"));
}

// ============= Namespace-Resolver Tests =============

TEST(AQLv131, NamespaceResolution) {
    NamespaceResolver resolver;
    
    resolver.enterScope("themis.vision");
    resolver.registerSymbol("VisionAnalysis", Symbol::Kind::TYPE, nullptr);
    
    auto* symbol = resolver.resolveSymbol("VisionAnalysis");
    ASSERT_NE(symbol, nullptr);
    ASSERT_EQ(symbol->qualified_name, "themis.vision.VisionAnalysis");
}

TEST(AQLv131, ImportResolution) {
    NamespaceResolver resolver;
    
    // Setup: Register symbol in one namespace
    resolver.enterScope("themis.llm");
    resolver.registerSymbol("InferenceModel", Symbol::Kind::TYPE, nullptr);
    resolver.exitScope();
    
    // Use: Import and resolve in another namespace
    resolver.enterScope("themis.vision");
    resolver.addImport("themis.llm", true);  // wildcard import
    
    auto* symbol = resolver.resolveSymbol("InferenceModel");
    ASSERT_NE(symbol, nullptr);
}

// ============= Integration Tests =============

TEST(AQLv131, EndToEndVisionQuery) {
    std::string query = R"(
        NAMESPACE themis.examples
        
        TYPE ImageAnalysis {
            objects: Array<String>,
            description: String
        }
        
        FUNCTION analyze(path: String) -> ImageAnalysis {
            LET result = LLM VISION ANALYZE path
                USING MODEL 'llava-7b'
                DETECT ['objects', 'text']
            RETURN result
        }
        
        RETURN analyze('test.jpg')
    )";
    
    AQLParser parser(query);
    auto ast = parser.parse();
    
    TypeChecker type_checker;
    NamespaceResolver namespace_resolver;
    
    // Validate AST
    // Execute query
    // Verify result
}

```

### 8.2 Integration-Tests mit llama.cpp

**Neue Datei:** `/tests/test_vision_integration.cpp`

```cpp
#include <gtest/gtest.h>
#include "aql/aql_vision_handler.h"

using namespace themis::aql;

TEST(VisionIntegration, AnalyzeImage) {
    VisionHandler handler;
    
    auto result = handler.executeAnalyze(
        "test_images/sample.jpg",
        {"objects", "text"},
        "llava-7b"
    );
    
    ASSERT_FALSE(result.scene_description.empty());
    ASSERT_GT(result.detected_objects.size(), 0);
}

TEST(VisionIntegration, QuestionAnswering) {
    VisionHandler handler;
    
    auto answer = handler.executeQuestion(
        "What objects are visible in this image?",
        "test_images/sample.jpg",
        "llava-7b"
    );
    
    ASSERT_FALSE(answer.empty());
}

// Weitere Tests...
```

---

## 9. CMake-Konfiguration aktualisieren

**Datei:** `/CMakeLists.txt`

**Zu ergänzen:**

```cmake
# AQL v1.3.1 neue Quelldateien
set(AQL_V131_SOURCES
    src/query/aql_type_checker.cpp
    src/query/aql_namespace_resolver.cpp
    src/aql/aql_vision_handler.cpp
)

set(AQL_V131_HEADERS
    include/query/aql_type_checker.h
    include/query/aql_namespace_resolver.h
    include/aql/aql_vision_handler.h
)

# Zu bestehenden Targets hinzufügen
target_sources(themisdb_query PRIVATE ${AQL_V131_SOURCES})
target_sources(themisdb_query PUBLIC ${AQL_V131_HEADERS})

# Tests
add_executable(test_aql_v1_3_1 tests/test_aql_v1_3_1.cpp)
target_link_libraries(test_aql_v1_3_1 themisdb_query gtest gtest_main)
add_test(NAME AQL_v1.3.1_Tests COMMAND test_aql_v1_3_1)
```

---

## 10. Dokumentation & Migration

### 10.1 Migration-Guide für Entwickler

**Datei:** `/docs/de/aql/MIGRATION_GUIDE_v1.3.0_to_v1.3.1.md`

**Inhalt:**
- Breaking Changes (keine für v1.3.1)
- Neue Features und deren Verwendung
- Code-Beispiele für Migration
- Best Practices

### 10.2 API-Dokumentation aktualisieren

- Doxygen-Kommentare für alle neuen Klassen und Methoden
- Beispiele in Header-Dateien
- README-Dateien in relevanten Verzeichnissen

---

## 11. Performance-Optimierung (Optional, Woche 12+)

### 11.1 Parser-Performance

- **Symbol-Table-Caching**: Häufig verwendete Symbole cachen
- **Type-Inference-Caching**: Berechnete Typen speichern
- **Lazy Parsing**: Nur parse wenn nötig (für Namespaces)

### 11.2 Vision-Command-Performance

- **Batch-Processing**: Mehrere Bilder parallel verarbeiten
- **Model-Caching**: Geladene Modelle im Speicher halten
- **Result-Caching**: Häufige Queries cachen

---

## 12. Rollout-Plan

### Phase 1 (Q1 2026): Core Features
- ✅ Lexer/Parser-Erweiterung (Woche 1-4)
- ✅ AST-Nodes (Woche 2-4)
- ✅ Type-System (Woche 4-6)
- ✅ Namespace-System (Woche 6-8)

**Deliverable:** v1.3.1-alpha mit Namespace, UDFs, Types, Pipeline

### Phase 2 (Q2 2026): Vision & Error Handling
- ✅ Vision-Commands (Woche 8-10)
- ✅ Error Handling (TRY-CATCH)
- ✅ Integration-Tests

**Deliverable:** v1.3.1-beta mit Vision-Support

### Phase 3 (Q3 2026): Advanced Features
- ✅ Pattern Matching
- ✅ Classes (optional)
- ✅ Advanced Type-System

**Deliverable:** v1.3.1-rc1

### Phase 4 (Q4 2026): Async & Production
- ✅ Async/Await
- ✅ Performance-Optimierung
- ✅ Production-Hardening

**Deliverable:** v1.3.1 GA (General Availability)

---

## 13. Checkliste für Implementierer

### Before Starting
- [ ] Vertrautmachen mit bestehender Codebasis (aql_parser.cpp, aql_translator.cpp)
- [ ] Entwicklungsumgebung einrichten (C++17/20, CMake, GTest)
- [ ] Spezifikationsdokumente lesen (AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf)

### During Implementation
- [ ] Feature-Branch erstellen: `feature/aql-v1.3.1-implementation`
- [ ] Jede Phase einzeln implementieren und testen
- [ ] Code-Reviews nach jeder Phase
- [ ] Unit-Tests für alle neuen Features schreiben
- [ ] Integration-Tests für End-to-End-Szenarien

### After Implementation
- [ ] Performance-Benchmarks durchführen
- [ ] Dokumentation vervollständigen
- [ ] Migration-Guide erstellen
- [ ] Release Notes schreiben

---

## 14. Häufige Probleme & Lösungen

### Problem: Keyword-Kollisionen mit v1.3.0

**Lösung:** Verwenden Sie context-sensitive Keywords. Beispiel: `TYPE` ist bereits in v1.3.0, aber mit anderer Bedeutung. Prüfen Sie Kontext im Parser.

### Problem: Namespace-Auflösung zu langsam

**Lösung:** Implementieren Sie einen Symbol-Table-Cache mit Hash-Map für O(1) Lookup.

### Problem: Vision-Befehle verursachen OOM

**Lösung:** Implementieren Sie Streaming für große Bilder und Model-Swapping für Speicher-Management.

### Problem: Type-Inference zu komplex

**Lösung:** Beginnen Sie mit einfacher Type-Inference (nur für Literale), erweitern Sie schrittweise.

---

## 15. Ressourcen & Referenzen

### Interne Dokumentation
- `AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf` - Vollständige Grammatik
- `AQL_OOP_EXTENSION_PROPOSAL.md` - Feature-Spezifikation
- `AQL_COMPLETE_LANGUAGE_SCOPE.md` - Sprachumfang-Analyse

### Externe Referenzen
- **ANTLR4**: Parser-Generator (falls Migration gewünscht)
- **llama.cpp**: Vision-Model-Integration
- **C++ Type System**: Für Type-Checker-Design

### Kontakt
- **Team-Lead**: makr-code
- **Code-Review**: [Team-Email]
- **Fragen/Issues**: GitHub Issues im ThemisDB-Repository

---

## Zusammenfassung

Dieser Implementation-Guide bietet einen **praxisnahen Fahrplan** für die Umsetzung der AQL v1.3.1 OOP-Erweiterungen. Die Implementierung ist in **4 Phasen über 12 Wochen** strukturiert und enthält:

1. **Konkrete Code-Beispiele** für alle neuen Features
2. **Genaue Datei-Referenzen** in der ThemisDB-Codebasis
3. **Test-Strategien** für Qualitätssicherung
4. **Performance-Tipps** für Production-Readiness

**Nächster Schritt:** Beginnen Sie mit **Phase 1 (Lexer-Erweiterung)** und arbeiten Sie sich durch die Phasen gemäß Rollout-Plan.

---

**Version:** 1.0  
**Letzte Aktualisierung:** 22. Dezember 2025  
**Status:** Bereit für Implementation
