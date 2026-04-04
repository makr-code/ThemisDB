# Const-Correctness Guide für ThemisDB

## Übersicht

Dieses Dokument beschreibt Best Practices für Const-Correctness in ThemisDB. Const-Correctness verbessert die API-Klarheit, ermöglicht Compiler-Optimierungen und verhindert unbeabsichtigte Zustandsänderungen.

## Warum Const-Correctness?

### Vorteile

1. **API-Klarheit**: Sofort erkennbar, ob eine Methode den Objektzustand ändert
2. **Compiler-Optimierungen**: Const ermöglicht aggressive Optimierungen
3. **Fehlerprävention**: Verhindert unbeabsichtigte Modifikationen
4. **Verwendbarkeit**: Const-Objekte können const-Methoden aufrufen
5. **Thread-Safety**: Const-Methoden können oft sicher parallel ausgeführt werden

### Probleme ohne Const-Correctness

```cpp
// PROBLEM: Unklar ob diese Methode den Zustand ändert
class Node {
    std::string id_;
public:
    std::string getId() { return id_; }  // ⚠️ Kann nicht auf const Node aufgerufen werden
};

void process(const Node& node) {
    // std::string id = node.getId();  // ❌ Compilerfehler!
}
```

## Best Practices

### 1. Getter-Methoden sollten const sein

```cpp
class Node {
    std::string id_;
    std::vector<Edge> edges_;
    
public:
    // ✅ Const-qualifizierte Getter
    const std::string& getId() const { return id_; }
    size_t edgeCount() const { return edges_.size(); }
    bool hasEdges() const { return !edges_.empty(); }
};
```

### 2. Pass-by-const-reference für komplexe Typen

```cpp
// ❌ SCHLECHT: Kopiert das ganze Objekt
void processNode(Node node) {
    // ...
}

// ✅ GUT: Keine Kopie, read-only
void processNode(const Node& node) {
    // ...
}

// ✅ AUCH GUT: Wenn Ownership übernommen wird
void storeNode(Node node) {
    nodes_.push_back(std::move(node));
}
```

### 3. Return by const-reference wo möglich

```cpp
class Config {
    std::string database_path_;
    
public:
    // ❌ SCHLECHT: Kopiert bei jedem Aufruf
    std::string getDatabasePath() const { return database_path_; }
    
    // ✅ GUT: Keine Kopie
    const std::string& getDatabasePath() const { return database_path_; }
};
```

### 4. Mutable für Cache und Thread-Safety

```cpp
class ExpensiveCalculator {
    mutable double cached_result_;
    mutable bool cache_valid_ = false;
    
public:
    double calculate() const {  // ✅ Kann const sein trotz Cache
        if (!cache_valid_) {
            cached_result_ = expensiveComputation();
            cache_valid_ = true;
        }
        return cached_result_;
    }
};

class ThreadSafeContainer {
    std::vector<int> data_;
    mutable std::mutex mutex_;  // ✅ Mutable für lock in const-Methoden
    
public:
    int get(size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_[index];
    }
};
```

### 5. Const und Non-Const Overloads

```cpp
class Container {
    std::vector<Item> items_;
    
public:
    // Non-const version
    Item* find(const std::string& id) {
        return const_cast<Item*>(
            std::as_const(*this).find(id)  // Ruft const-Version auf
        );
    }
    
    // Const version (die eigentliche Implementierung)
    const Item* find(const std::string& id) const {
        for (const auto& item : items_) {
            if (item.id() == id) return &item;
        }
        return nullptr;
    }
};
```

## Patterns in ThemisDB

### Cache Interfaces

```cpp
// include/core/concerns/i_cache.h
class ICache {
public:
    // ✅ Get ist const - ändert logischen Zustand nicht
    virtual std::optional<CacheEntry> get(std::string_view key) const = 0;
    
    // Statistiken sind const - nur lesend
    virtual size_t size() const = 0;
    virtual uint64_t hitCount() const = 0;
    virtual double hitRate() const = 0;
    
    // Const und Non-Const Overloads für Strategy
    virtual IEvictionStrategy* getEvictionStrategy() { return nullptr; }
    virtual const IEvictionStrategy* getEvictionStrategy() const { return nullptr; }
};
```

### Vector Index

```cpp
// include/index/vector_index.h
class VectorIndexManager {
public:
    // ✅ String-Getter per const-reference
    const std::string& getObjectName() const { return objectName_; }
    const std::string& getSavePath() const { return savePath_; }
    
    // ✅ Query-Methoden sind const
    size_t getVectorCount() const { return pkToId_.size(); }
    bool isHnswEnabled() const { return useHnsw_; }
};
```

### Database Adapter

```cpp
// include/chimera/database_adapter.hpp
class IDatabaseAdapter {
public:
    // ✅ System-Info Queries sind const
    virtual Result<SystemInfo> get_system_info() const = 0;
    virtual Result<SystemMetrics> get_metrics() const = 0;
    virtual bool has_capability(Capability cap) const = 0;
};
```

## Checkliste für neue Klassen

Wenn Sie eine neue Klasse erstellen:

- [ ] Alle Getter-Methoden haben `const`
- [ ] Query-Methoden (die nur lesen) haben `const`
- [ ] String/Container-Returns sind `const&` wo sinnvoll
- [ ] Function-Parameter sind `const&` für non-primitives (außer bei move)
- [ ] Cache-Member sind `mutable`
- [ ] Mutex-Member sind `mutable`
- [ ] Iterator `begin()`/`end()` haben const-Versionen
- [ ] Dokumentation erklärt const vs non-const

## Code Review Guidelines

### Was zu prüfen ist

1. **Getter ohne const**: Sollte diese Methode const sein?
2. **Pass-by-value**: Sollte das ein `const&` sein?
3. **String-Kopien**: Kann das per `const&` zurückgegeben werden?
4. **Fehlende const-Overloads**: Brauchen wir const und non-const Versionen?

### Häufige Fehler

```cpp
// ❌ FEHLER 1: Getter ohne const
class User {
    std::string name_;
public:
    std::string getName() { return name_; }  // Sollte const sein
};

// ❌ FEHLER 2: Unnötige Kopien
std::string getConfigPath() const { return config_path_; }  // Sollte const& zurückgeben

// ❌ FEHLER 3: Pass-by-value für komplexe Typen
void process(std::vector<int> data) { }  // Sollte const& sein (wenn nicht moved)

// ❌ FEHLER 4: Vergessenes mutable für Cache
class Calculator {
    double cache_;  // Sollte mutable sein
public:
    double calculate() const {
        // cache_ = ...;  // ❌ Compilerfehler!
    }
};
```

## Testing Const-Correctness

```cpp
// Test dass const-Objekte verwendet werden können
TEST(ConstCorrectnessTest, CanCallOnConstObject) {
    const Node node("test");
    
    // Diese sollten alle kompilieren:
    std::string id = node.getId();
    size_t count = node.edgeCount();
    bool has = node.hasEdges();
}

// Test dass const-references funktionieren
TEST(ConstCorrectnessTest, PassByConstReference) {
    Node node("test");
    
    auto process = [](const Node& n) {
        return n.getId();
    };
    
    EXPECT_EQ(process(node), "test");
}
```

## Compiler-Unterstützung

### Compiler Warnings

```cmake
# CMakeLists.txt
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wsuggest-override
        -Wsuggest-final-types
        -Wsuggest-final-methods
    )
endif()
```

### Clang-Tidy

```yaml
# .clang-tidy
Checks: '
  readability-const-return-type,
  readability-make-member-function-const,
  cppcoreguidelines-avoid-const-or-ref-data-members
'
```

## Migration bestehenden Codes

### Phase 1: Interfaces und Public API

Beginnen Sie mit öffentlichen Schnittstellen:

1. Cache-Interfaces (`ICache`, `ResultCache`, etc.)
2. Database Adapter Interfaces
3. Manager-Klassen Public API

### Phase 2: Getter und Query-Methoden

Fügen Sie const zu allen Getter- und Query-Methoden hinzu:

```bash
# Finden Sie Kandidaten:
grep -rn "get[A-Z].*{.*return.*_;" include/ | grep -v "const"
```

### Phase 3: Return-Types optimieren

Ändern Sie String-Returns zu const-references:

```bash
# Finden Sie Kandidaten:
grep -rn "std::string get.*const {" include/ | grep "return.*_;"
```

### Phase 4: Function-Parameters

Überprüfen Sie Pass-by-value vs Pass-by-const-reference.

## Referenzen

- [C++ Core Guidelines: Con](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-const)
- [Effective C++ by Scott Meyers](https://www.aristeia.com/books.html)
- [Const Correctness in C++](https://isocpp.org/wiki/faq/const-correctness)

## Änderungshistorie

| Version | Datum | Änderungen |
|---------|-------|-----------|
| 1.0.0 | 2026-02-12 | Initiale Version mit Best Practices |
