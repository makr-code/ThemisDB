# Kapitel 32: AQL v1.3.1 OOP-Implementierung

> *"Die Sprache ist das API – ihre Implementierung bestimmt die Fähigkeiten der Datenbank."*

---

## 32.1 Executive Summary

AQL v1.3.1 erweitert die Sprache um objektorientierte Konstrukte (47 neue Keywords, 360 Funktionen bleiben konstant). Dieses Kapitel beschreibt, wie die Erweiterungen in ThemisDB umgesetzt werden – vom Lexer über den Parser bis zur Ausführungsebene.

**Ziele:**
- Lexer/Tokenizer: 47 neue Keywords, Tokens für Klassen/Methoden/Namespaces
- Parser/AST: Neue Node-Typen für `CLASS`, `FUNCTION`, `METHOD`, `NAMESPACE`
- Type-System: Statisches Type-Checking, Inference, generische Typen
- Namespace-Resolution: Symbol-Tables, Visibility, Imports
- Runtime: Objekt-Instanziierung, Methodenaufrufe, Visibility-Checks
- Tests: Parser-Golden-Files, Type-Checker-Property-Tests

---

## 32.2 Betroffene Codebereiche

| Datei | Zweck | Änderung |
|-------|-------|----------|
| `/src/query/aql_parser.cpp` | Lexer/Parser | 47 neue Keywords, AST Nodes | 
| `/src/query/aql_translator.cpp` | AST → Execution Plan | OOP Nodes übersetzen |
| `/include/query/aql_parser.h` | Parser-Interface | Neue Strukturen/Enums |
| `/src/query/aql_type_checker.cpp` | Type-System | Neue Typregeln, Inference |
| `/src/query/aql_namespace_resolver.cpp` | Symbol-Tables | Scopes, Visibility |
| `/src/aql/llm_aql_handler.cpp` | LLM-Befehle | Vision/OOP-Kommandos |

Neue Dateien:
- `/include/query/aql_type_checker.h`, `/src/query/aql_type_checker.cpp`
- `/include/query/aql_namespace_resolver.h`, `/src/query/aql_namespace_resolver.cpp`
- `/include/aql/aql_vision_handler.h`, `/src/aql/aql_vision_handler.cpp`

---

## 32.3 Lexer/Tokenizer (Woche 1-2)

### 32.3.1 TokenType-Erweiterung

- Namespace: `NAMESPACE`, `IMPORT`
- Typen: `STRING_TYPE`, `INT_TYPE`, `FLOAT_TYPE`, `BOOL_TYPE`, `ANY_TYPE`
- Klassen/Funktionen: `FUNCTION`, `CLASS`, `PUBLIC`, `PRIVATE`, `CONSTRUCTOR`, `METHOD`, `NEW`
- Referenzen/Vererbung: `THIS`, `SELF`, `EXTENDS`
- Control Flow: `IF`, `THEN`, `ELSE`, `ELSEIF`, `ENDIF`, `TRY`, `CATCH`, `THROW`, `CASE`, `END`
- Pattern Matching: `MATCH`, `WHEN`

**Lexer-Checks:**
- Case-insensitive Keywords
- String-Literale unverändert lassen
- Fehlerhafte Kombinationen mit präzisen Fehlermeldungen (Zeile/Spalte)

### 32.3.2 Keyword-Detection

```cpp
// Keyword-Detection mit unordered_set (119 Keywords)
bool isKeyword(std::string_view tok) {
    static const std::unordered_set<std::string_view> kw = {
        "FOR", "CLASS", "METHOD", "NAMESPACE", /* ... 115 weitere */
    };
    return kw.contains(tok);
}
```

---

## 32.4 Parser & AST (Woche 2-4)

### 32.4.1 Neue AST-Nodes

- `AstClassDecl { name, base?, members[] }`
- `AstMethodDecl { name, params[], return_type, visibility, is_static }`
- `AstFunctionDecl { name, params[], return_type }`
- `AstNamespaceDecl { name, imports[], body[] }`
- `AstNewExpr { type_name, args[] }`
- `AstMemberAccess { object, member }`

### 32.4.2 Grammatik (Auszug)

```
translation_unit
  : namespace_decl* class_decl* function_decl* statement*
  ;

class_decl
  : CLASS IDENT (EXTENDS IDENT)? LBRACE class_member* RBRACE
  ;

class_member
  : visibility? (method_decl | property_decl | constructor_decl)
  ;

method_decl
  : METHOD IDENT LPAREN param_list? RPAREN (ARROW type)? block
  ;
```

### 32.4.3 Fehlerbehandlung

- Präzise Fehler: `Unexpected token 'END' at line 42, expected 'WHEN'`
- Recovery: Synchronisation bei `;`, `END`, `}`
- AST-Pragmas für Fehlertoleranz, damit IDE-Features funktionieren

---

## 32.5 Type-System (Woche 4-6)

### 32.5.1 Typen & Inference

| Typ | Beschreibung |
|-----|--------------|
| `Any` | Fallback/Unbekannt |
| `String`, `Int`, `Float`, `Bool` | Primitive |
| `Class<T>` | Klasseninstanzen |
| `Vector<T>` | Generische Container |
| `Func<Args, Ret>` | Funktions-Typ |

**Inference-Regeln:**
- Assignment: `lhs = rhs` → `type(lhs) = type(rhs)`
- Member Access: `obj.m` → Lookup in Symbol-Table, Sichtbarkeit prüfen
- Calls: `call(f, args)` → Parameteranzahl, Typkompatibilität, Varargs

### 32.5.2 Visibility & Access Control

- `public`: überall sichtbar
- `private`: nur innerhalb derselben Klasse
- `protected` (optional): Klasse + Subklassen

### 32.5.3 Exceptions & TRY/CATCH

- Typ `Exception` als Basisklasse
- `THROW expr` propagiert Typ `Exception`
- `TRY { ... } CATCH (e: Exception) { ... }`

---

## 32.6 Namespace-Resolution (Woche 6-7)

### 32.6.1 Symbol-Tables

- Hierarchische Tabellen pro Scope
- Eintrag: `{name, kind (var|func|class|namespace), type, visibility}`
- Shadowing nach Java/C#-Modell (innere Scopes haben Vorrang)

### 32.6.2 Imports

```aql
NAMESPACE themis.app
IMPORT themis.llm.*
IMPORT themis.vision.embed
```

- Wildcards (`*`) nur auf Namespace-Level erlaubt
- Konfliktauflösung: explizite Qualifizierung gewinnt (`llm.stats`)

### 32.6.3 Resolution-Algorithmus

1. Prüfe lokalen Scope
2. Prüfe Namespace-Imports
3. Prüfe globale Symbole
4. Fehler, wenn nicht gefunden → `Unknown symbol 'X'`

---

## 32.7 Runtime & Execution (Woche 7-10)

### 32.7.1 New/Constructor

```cpp
Value evalNew(const AstNewExpr& node, Context& ctx) {
    auto* cls = ctx.lookupClass(node.type_name);
    return cls->invokeConstructor(node.args);  // Alloc + Init
}
```

### 32.7.2 Methodenaufrufe

- Dynamische Dispatch-Tabelle je Klasse
- Inline-Cache (optional) für Hot-Paths
- Visibility-Checks zur Laufzeit (Guard)

### 32.7.3 Exceptions

- Stack-Unwinding mit RAII-Gurten
- `defer`/`finally`-Support über Scope Guards

---

## 32.8 Vision & LLM-Befehle (Add-On)

Neue Kommandos im LLM-Handler:
- `VISION.EMBED(image|text, model)` → Vektor
- `VISION.DESCRIBE(image)` → Text
- `VISION.COMPARE(imgA, imgB)` → Similarity
- Safety: Max Input Size, MIME-Checks, Model-Whitelist

---

## 32.9 Test-Strategie

### 32.9.1 Parser-Golden-Files

- Input → erwarteter AST (JSON) → Diffen in CI
- Beispiele: Klassen mit Vererbung, Methoden mit Default-Args, Namespaces

### 32.9.2 Property-Tests (Type Checker)

- QuickCheck/rapidcheck: zufällige Ausdrucksbäume generieren
- Invariante: Keine illegalen Typen nach erfolgreichem Check
- Fuzzer: ungültige Tokens → Parser muss robust fehlschlagen

### 32.9.3 Runtime-Tests

- Konstruktor/Methoden-Dispatch
- Visibility-Fehler (private/public)
- Exception-Pfade und Unwinding

---

## 32.10 Migrations-Plan

| Phase | Dauer | Ergebnis |
|-------|-------|----------|
| Lexer/Parser | 2 Wochen | Keywords + AST-Nodes | 
| Type-System | 2-3 Wochen | Checker + Inference |
| Namespace | 1 Woche | Symbol-Tables, Imports |
| Runtime | 2-3 Wochen | Dispatch, Exceptions |
| Tests/Hardening | 2 Wochen | Golden-Files, Property-Tests |

**Risiken & Mitigation:**
- Parser-Regressions → Golden-Files in CI
- Performance-Einbruch → Inline-Caches, Hot-Path-Profiler
- Kompatibilität → Feature-Flag `enable_oop` bis stabil

---

## 32.11 Checkliste Go-Live

- [ ] `enable_oop` Feature-Flag default ON
- [ ] Parser-Tests (100% der neuen Grammatik) grün
- [ ] Type-Checker-Property-Tests 10k Runs
- [ ] Runtime-Benchmarks: <10% Overhead ggü. v1.3.0
- [ ] LLM/Vision-Commands whitelisted, Input-Limits gesetzt
- [ ] Docs aktualisiert (AQL Referenz, Examples)

---

## 32.10 Implementierungs-Timeline

### Phase 1: Lexer & Parser (Wochen 1-4)
**Woche 1-2: Lexer**
- Tag 1-2: Keyword-Set erweitern (47 neue)
- Tag 3-4: Token-Recognition Tests
- Tag 5-6: Error Messages verbessern
- Tag 7-8: Performance Benchmarks (Lexer sollte <5% langsamer sein)

**Woche 3-4: Parser**
- Tag 1-4: AST Node-Typen implementieren
- Tag 5-7: Grammatik-Regeln für CLASS/METHOD/NAMESPACE
- Tag 8-10: Golden-File Tests (10+ Parser-Test-Cases)

### Phase 2: Type System (Wochen 5-7)
**Woche 5-6: Type Checker**
- Tag 1-3: Typ-Inference-Algorithmus
- Tag 4-6: Visitor Pattern für AST-Traversal
- Tag 7-10: Generics Support (Vector<T>, Map<K,V>)

**Woche 7: Validation**
- Tag 1-3: Type Error Messages
- Tag 4-6: Property-Based Tests
- Tag 7: Integration mit Parser

### Phase 3: Namespace Resolution (Wochen 8-9)
- Tag 1-3: Symbol-Table Implementierung
- Tag 4-6: Import-Resolution
- Tag 7-9: Shadowing & Visibility
- Tag 10-12: Test-Suite (50+ Test-Cases)

### Phase 4: Runtime (Wochen 10-14)
**Woche 10-11: Object Model**
- Tag 1-5: Class/Instance Representation
- Tag 6-10: Constructor/Method Dispatch

**Woche 12-13: Execution**
- Tag 1-5: NEW Expression Execution
- Tag 6-10: Member Access Resolution
- Tag 11-14: Exception Handling

**Woche 14: Integration**
- Tag 1-3: End-to-End Tests
- Tag 4-5: Performance Profiling
- Tag 6-7: Documentation

### Phase 5: Vision & Rollout (Wochen 15-16)
- Woche 15: Vision.EMBED/DESCRIBE/COMPARE
- Woche 16: Production Deployment, Monitoring

---

## 32.11 Code-Beispiele

### 32.11.1 Lexer Test

```cpp
// test_lexer_oop.cpp
TEST(AQLLexer, RecognizesClassKeyword) {
    std::string input = "CLASS User { }";
    Lexer lexer(input);
    
    Token t1 = lexer.next();
    EXPECT_EQ(t1.type, TokenType::CLASS);
    
    Token t2 = lexer.next();
    EXPECT_EQ(t2.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t2.value, "User");
    
    Token t3 = lexer.next();
    EXPECT_EQ(t3.type, TokenType::LBRACE);
}

TEST(AQLLexer, KeywordCaseInsensitive) {
    Lexer lexer("class CLASS Class");
    
    for (int i = 0; i < 3; i++) {
        Token t = lexer.next();
        EXPECT_EQ(t.type, TokenType::CLASS);
    }
}
```

### 32.11.2 Parser Test mit Golden Files

```cpp
// test_parser_golden.cpp
TEST(AQLParser, ClassDeclarationGolden) {
    std::string input = R"(
        CLASS User {
            PRIVATE email: STRING
            PUBLIC name: STRING
            
            CONSTRUCTOR(email, name) {
                THIS.email = email
                THIS.name = name
            }
            
            PUBLIC METHOD greet() {
                RETURN CONCAT("Hello, ", THIS.name)
            }
        }
    )";
    
    Parser parser(input);
    AstNode* ast = parser.parse();
    
    // Serialize AST to JSON
    std::string ast_json = serializeAST(ast);
    
    // Compare with Golden File
    std::string expected = readFile("golden/class_user_ast.json");
    EXPECT_EQ(ast_json, expected);
}
```

### 32.11.3 Type Checker Unit Test

```cpp
// test_type_checker.cpp
TEST(TypeChecker, InfersReturnType) {
    std::string code = R"(
        FUNCTION add(a: INT, b: INT) {
            RETURN a + b  // Should infer INT
        }
    )";
    
    TypeChecker checker;
    FunctionType func_type = checker.inferFunctionType(code);
    
    EXPECT_EQ(func_type.return_type, Type::INT);
    EXPECT_EQ(func_type.param_types.size(), 2);
    EXPECT_EQ(func_type.param_types[0], Type::INT);
}

TEST(TypeChecker, DetectsMismatch) {
    std::string code = R"(
        FUNCTION broken(x: STRING) -> INT {
            RETURN x  // ❌ Type Error: expected INT, got STRING
        }
    )";
    
    TypeChecker checker;
    EXPECT_THROW(
        checker.check(code),
        TypeMismatchException
    );
}
```

### 32.11.4 Namespace Resolution Test

```cpp
// test_namespace.cpp
TEST(NamespaceResolver, ImportsResolve) {
    std::string code = R"(
        NAMESPACE myapp
        IMPORT themis.llm.*
        
        FUNCTION analyze(text) {
            RETURN LLM.CHAT(text)  // Should resolve to themis.llm.LLM.CHAT
        }
    )";
    
    NamespaceResolver resolver;
    SymbolTable symbols = resolver.resolve(code);
    
    Symbol* llm_chat = symbols.lookup("LLM.CHAT");
    EXPECT_NE(llm_chat, nullptr);
    EXPECT_EQ(llm_chat->fully_qualified_name, "themis.llm.LLM.CHAT");
}
```

### 32.11.5 Runtime NEW Expression

```cpp
// aql_runtime.cpp: NEW Implementierung
Value AQLRuntime::evalNew(const AstNewExpr& node) {
    // 1. Lookup Class Definition
    ClassDef* cls = symbol_table_.lookupClass(node.type_name);
    if (!cls) {
        throw RuntimeError(fmt::format("Class '{}' not found", node.type_name));
    }
    
    // 2. Allocate Instance
    ObjectInstance* instance = allocateObject(cls);
    
    // 3. Find Constructor
    Constructor* ctor = cls->findConstructor(node.args.size());
    if (!ctor) {
        throw RuntimeError(fmt::format(
            "No constructor found for {} args", node.args.size()
        ));
    }
    
    // 4. Evaluate Arguments
    std::vector<Value> arg_values;
    for (const auto& arg : node.args) {
        arg_values.push_back(eval(arg));
    }
    
    // 5. Invoke Constructor
    ctor->invoke(instance, arg_values);
    
    return Value::fromObject(instance);
}
```

---

## 32.12 Performance-Auswirkungen

### Erwartete Overhead
| Operation | Baseline (ohne OOP) | Mit OOP | Overhead |
|-----------|---------------------|---------|----------|
| Lexing | 100 ms/10k lines | 105 ms | +5% |
| Parsing | 250 ms/10k lines | 280 ms | +12% |
| Type Check | N/A | 150 ms | Neu |
| Execution (Simple FOR) | 10 ms | 10 ms | 0% |
| Execution (Method Call) | N/A | +2 ms | Neu |

**Optimierungen:**
- **Inline Caching:** Häufig aufgerufene Methoden cachen
- **JIT für Hot Methods:** Bytecode-Kompilierung für >1000 Calls
- **Lazy Type Checking:** Nur bei Bedarf (Development-Mode)

### Benchmarks

```cpp
// benchmark_oop.cpp
static void BM_MethodCall(benchmark::State& state) {
    std::string code = R"(
        CLASS Counter {
            PRIVATE count: INT
            
            CONSTRUCTOR() { THIS.count = 0 }
            METHOD increment() { THIS.count = THIS.count + 1 }
        }
        
        LET c = NEW Counter()
        FOR i IN 1..1000000
            c.increment()
    )";
    
    for (auto _ : state) {
        AQLRuntime runtime;
        runtime.execute(code);
    }
}
BENCHMARK(BM_MethodCall);

// Ergebnis: ~2.5 µs pro Method Call (inkl. Dispatch)
```

---

## 32.13 Migration & Backward Compatibility

### Rückwärtskompatibilität
- ✅ **Alle AQL v1.3.0 Queries funktionieren unverändert**
- ✅ **Neue Keywords nur in OOP-Kontext reserviert**
- ✅ **Legacy-Code benötigt keine Änderungen**

### Opt-In für OOP Features

```aql
-- Legacy Mode (Default)
FOR u IN users RETURN u

-- OOP Mode (via Pragma)
#pragma aql_version 1.3.1

CLASS User { ... }
```

### Feature Flag

```yaml
# themis.conf
aql:
  oop_features_enabled: true  # Default: false bis v1.4.0
  strict_type_checking: false  # Development: true, Production: false
```

---

## 32.14 Zusammenfassung

AQL v1.3.1 führt OOP-Konstrukte ein und erfordert Änderungen auf allen Ebenen der Query-Engine. Die Implementierung gliedert sich in fünf Phasen: Lexer/Parser, Type-System, Namespace-Resolution, Runtime, Tests. Mit klaren Guardrails (Feature-Flag, Tests, Benchmarks) kann die Erweiterung ohne Regressionen ausgerollt werden.
