# Null Pointer Safety Guide

## Übersicht

Dieser Leitfaden beschreibt Best Practices zur Vermeidung von Null-Pointer-Dereferenzierungen in ThemisDB. In einem Datenbanksystem ist Stabilität und Datenintegrität von höchster Priorität, daher müssen alle Pointer-Zugriffe sorgfältig validiert werden.

## Kritische Problemmuster

### 1. Raw Pointer ohne Null-Check

**❌ GEFÄHRLICH:**
```cpp
SomeType* ptr = get_pointer();
ptr->method();  // CRASH wenn nullptr!
```

**✅ SICHER - Option 1: Expliziter Check**
```cpp
SomeType* ptr = get_pointer();
if (ptr != nullptr) {
    ptr->method();
} else {
    handle_null_case();
}
```

**✅ SICHER - Option 2: Early return**
```cpp
SomeType* ptr = get_pointer();
if (!ptr) {
    THEMIS_ERROR("Null pointer received");
    return;
}
ptr->method();
```

**✅ BEST - Smart pointer (falls möglich)**
```cpp
auto ptr = get_smart_pointer();  // shared_ptr/unique_ptr
if (ptr) {
    ptr->method();
}
```

### 2. Unsafe dynamic_cast

**❌ GEFÄHRLICH:**
```cpp
Base* base = get_base_pointer();
Derived* derived = dynamic_cast<Derived*>(base);
derived->specific_method();  // CRASH wenn cast fehlschlägt!
```

**✅ SICHER - Check nach cast**
```cpp
Base* base = get_base_pointer();
if (Derived* derived = dynamic_cast<Derived*>(base)) {
    derived->specific_method();
} else {
    THEMIS_ERROR("Invalid cast from Base to Derived");
    handle_error();
}
```

**✅ ODER - Mit safe_cast Utility**
```cpp
#include "utils/safe_access.h"

Base* base = get_base_pointer();
if (auto derived = themis::utils::safe_cast<Derived>(base)) {
    derived.value()->specific_method();
}
```

### 3. Unchecked std::optional

**❌ GEFÄHRLICH:**
```cpp
std::optional<Config> config = load_config();
process(config.value());  // CRASH wenn empty!
```

**✅ SICHER - Option 1: has_value() check**
```cpp
std::optional<Config> config = load_config();
if (config.has_value()) {
    process(config.value());
} else {
    use_default_config();
}
```

**✅ SICHER - Option 2: value_or()**
```cpp
std::optional<Config> config = load_config();
process(config.value_or(get_default_config()));
```

**✅ SICHER - Option 3: C++17 pattern**
```cpp
if (auto config = load_config()) {
    process(*config);
}
```

### 4. Array/Vector Access ohne Bounds-Check

**❌ GEFÄHRLICH:**
```cpp
std::vector<int> data = get_data();
int value = data[index];  // CRASH wenn index >= size!
```

**✅ SICHER - Option 1: at() mit exception**
```cpp
try {
    int value = data.at(index);
    process(value);
} catch (const std::out_of_range& e) {
    handle_out_of_bounds(index);
}
```

**✅ SICHER - Option 2: Expliziter Check**
```cpp
if (index < data.size()) {
    int value = data[index];
    process(value);
} else {
    handle_out_of_bounds(index);
}
```

**✅ BEST - Safe accessor utility**
```cpp
#include "utils/safe_access.h"

auto value = themis::utils::safe_get(data, index);
if (value) {
    process(value->get());
}
```

### 5. Iterator Dereference ohne Validierung

**❌ GEFÄHRLICH:**
```cpp
auto it = map.find(key);
process(it->second);  // CRASH wenn key nicht existiert!
```

**✅ SICHER - Check gegen end()**
```cpp
auto it = map.find(key);
if (it != map.end()) {
    process(it->second);
} else {
    handle_missing_key(key);
}
```

**✅ ODER - safe_get Utility**
```cpp
#include "utils/safe_access.h"

if (auto value = themis::utils::safe_get(map, key)) {
    process(value->get());
}
```

### 6. Smart Pointer .get() Missbrauch

**❌ GEFÄHRLICH:**
```cpp
std::shared_ptr<Resource> resource = get_resource();
Resource* raw = resource.get();
raw->method();  // CRASH wenn resource == nullptr!
```

**✅ SICHER - Direkt smart pointer nutzen**
```cpp
std::shared_ptr<Resource> resource = get_resource();
if (resource) {
    resource->method();
}
```

**✅ ODER - checked_get für kritische Pfade**
```cpp
#include "utils/safe_access.h"

std::shared_ptr<Resource> resource = get_resource();
try {
    Resource* raw = themis::utils::checked_get(resource, "resource access");
    raw->method();
} catch (const std::runtime_error& e) {
    THEMIS_ERROR("Resource validation failed: {}", e.what());
    handle_error();
}
```

## ThemisDB Safe Access Utilities

ThemisDB bietet eine Reihe von Utility-Funktionen in `include/utils/safe_access.h`:

### safe_get (Vector)
```cpp
#include "utils/safe_access.h"

std::vector<int> data = {1, 2, 3};
if (auto val = themis::utils::safe_get(data, 1)) {
    std::cout << "Value: " << val->get() << std::endl;
}
```

### safe_get (Map)
```cpp
std::map<std::string, Config> configs;
if (auto cfg = themis::utils::safe_get(configs, "prod")) {
    use(cfg->get());
}
```

### safe_deref (Raw Pointer)
```cpp
Node* node = getNode();
if (auto n = themis::utils::safe_deref(node)) {
    process(n->get());
}
```

### checked_get (Smart Pointer)
```cpp
auto resource = themis::utils::checked_get(resourcePtr, "initialization");
resource->use();  // Wirft exception wenn null
```

### safe_cast (Dynamic Cast)
```cpp
Base* base = getBase();
if (auto derived = themis::utils::safe_cast<Derived>(base)) {
    derived.value()->specificMethod();
}
```

## Null-Safety Patterns

### Pattern A: Input Validation
```cpp
class GraphDatabase {
public:
    void add_edge(Node* from, Node* to, const EdgeData& data) {
        // ✅ Validierung am Anfang
        if (!from || !to) {
            throw std::invalid_argument("Null node pointers not allowed");
        }
        
        // Rest der Funktion kann Pointer sicher nutzen
        from->add_outgoing_edge(to, data);
        to->add_incoming_edge(from, data);
    }
};
```

### Pattern B: Defensive Returns
```cpp
std::optional<Result> process_query(const Query& query) {
    auto* index = get_index(query.table);
    if (!index) {
        THEMIS_ERROR("Index not found for table: {}", query.table);
        return std::nullopt;
    }
    
    auto* optimizer = get_optimizer();
    if (!optimizer) {
        THEMIS_ERROR("Query optimizer not available");
        return std::nullopt;
    }
    
    // Ab hier sind alle Pointer valid
    return optimizer->execute(query, index);
}
```

### Pattern C: RAII-Guards
```cpp
class ResourceGuard {
    Resource* resource_;
public:
    explicit ResourceGuard(Resource* r) : resource_(r) {
        if (!resource_) {
            throw std::invalid_argument("Null resource");
        }
        resource_->acquire();
    }
    
    ~ResourceGuard() {
        if (resource_) {  // Defense in depth
            resource_->release();
        }
    }
    
    Resource* get() { return resource_; }
};
```

## Code Review Checkliste

Bei Code Reviews auf folgende Punkte achten:

- [ ] **Raw Pointer**: Alle raw pointer haben null-checks vor Dereferenzierung
- [ ] **Dynamic Cast**: Alle `dynamic_cast` Ergebnisse werden validiert
- [ ] **Smart Pointer .get()**: Nur für non-owning observations, mit Validierung
- [ ] **Vector/Map Access**: Bounds checking oder safe accessors verwendet
- [ ] **Iterator**: Alle iterators werden gegen `.end()` geprüft
- [ ] **Optional Types**: Alle `std::optional` haben `.has_value()` checks
- [ ] **Function Returns**: Alle funktionen die pointer/optional zurückgeben haben dokumentierte Kontrakte
- [ ] **Error Handling**: Null-pointer Fälle haben angemessene Fehlerbehandlung

## Testing-Strategien

### Unit Tests
```cpp
TEST(SafetyTest, NullPointerHandling) {
    Node* null_node = nullptr;
    
    // Should not crash
    EXPECT_NO_THROW({
        if (auto n = themis::utils::safe_deref(null_node)) {
            process(n->get());
        }
    });
}
```

### Integration Tests
```cpp
TEST(IntegrationTest, GraphTraversalWithNullNodes) {
    Graph graph;
    graph.add_node(nullptr);  // Should be rejected
    
    EXPECT_THROW(graph.traverse(nullptr), std::invalid_argument);
}
```

### Sanitizer Tests
```bash
# AddressSanitizer
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
cmake --build build
./build/tests/all_tests

# UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
cmake --build build
./build/tests/all_tests
```

## Kritische Bereiche in ThemisDB

### Storage Layer (P0)
- Buffer-Management: Alle buffer pointer validieren
- Page-Pointer: RocksDB iterator checks
- Transaction-Handles: Null checks vor commit/rollback

### Graph Database (P0)
- Node/Edge-Pointer: Validierung bei traversal
- Index-Strukturen: Bounds checking
- Traversal-Algorithmen: Iterator validierung

### LLM Framework (P0)
- Model-Pointer: Smart pointer usage
- GPU-Memory: CUDA error checking
- Batch-Buffers: Allocation validation

### Server (P1)
- Request-Handler: Input validation
- Session-Management: Lifecycle checks
- Connection-Pools: Resource validation

## Häufige Fehler

### ❌ Fehler 1: Vergessene Null-Checks
```cpp
// BAD
auto ptr = map[key];  // Kann nullptr sein
ptr->method();
```

### ✅ Fix:
```cpp
if (auto ptr = themis::utils::safe_get(map, key)) {
    ptr->get().method();
}
```

### ❌ Fehler 2: Ungeprüfte Dynamic Casts
```cpp
// BAD
auto* derived = dynamic_cast<Derived*>(base);
derived->method();  // Kann crashen
```

### ✅ Fix:
```cpp
if (auto derived = themis::utils::safe_cast<Derived>(base)) {
    derived.value()->method();
}
```

### ❌ Fehler 3: Vector Access ohne Bounds
```cpp
// BAD
return vec[index];  // Kann out of bounds sein
```

### ✅ Fix:
```cpp
if (auto val = themis::utils::safe_get(vec, index)) {
    return val->get();
}
return default_value;
```

## Weiterführende Ressourcen

- **C++ Core Guidelines**: [C.149: Use unique_ptr or shared_ptr to avoid forgetting to delete objects](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-unique_ptr)
- **CERT C++**: [EXP34-C: Do not dereference null pointers](https://wiki.sei.cmu.edu/confluence/display/c/EXP34-C.+Do+not+dereference+null+pointers)
- **ThemisDB Documentation**: [Architecture Guide](../../ARCHITECTURE.md)

## Zusammenfassung

1. **Immer validieren** bevor Sie dereferenzieren
2. **Safe Accessor Utilities nutzen** aus `utils/safe_access.h`
3. **Smart Pointers bevorzugen** über raw pointers
4. **Early returns** für bessere Lesbarkeit
5. **Tests schreiben** für null-pointer Szenarien
6. **Code Reviews** mit Fokus auf null-safety

Bei Fragen oder Unsicherheiten wenden Sie sich an das Core Team oder erstellen Sie ein Issue auf GitHub.
