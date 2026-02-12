# Const-Correctness Guide für ThemisDB

## Einleitung

Dieses Dokument beschreibt die Const-Correctness-Richtlinien für ThemisDB. Const-correctness ist ein fundamentales C++-Prinzip, das zu besserem Code führt durch:

- **Bessere API-Klarheit**: Methoden-Signaturen dokumentieren selbst, ob sie den Objektzustand ändern
- **Compile-Time Safety**: Compiler verhindert versehentliche Modifikationen
- **Self-documenting Code**: Leser können sofort erkennen, welche Operationen read-only sind
- **Compiler-Optimierungen**: const ermöglicht bessere Optimierungen

## Grundprinzipien

### 1. Const Member Functions

**Regel**: Markiere alle Member-Funktionen als `const`, die den Objektzustand nicht ändern.

```cpp
// ✅ RICHTIG - Getter sind const
class GPUMemoryManager {
public:
    size_t getTotalVRAM() const { return total_vram_; }
    size_t getFreeVRAM() const { return max_vram_ - total_vram_; }
    
    bool canAllocate(size_t bytes) const {
        return (total_vram_ + bytes) <= max_vram_;
    }

private:
    size_t total_vram_;
    size_t max_vram_;
};

// ❌ FALSCH - Getter ohne const
class GPUMemoryManager {
public:
    size_t getTotalVRAM() { return total_vram_; }  // ⚠️ Nicht const!
};
```

**Warum ist das wichtig?**
```cpp
// Mit const kann man const-Referenzen verwenden:
void printStats(const GPUMemoryManager& mgr) {
    std::cout << "VRAM: " << mgr.getTotalVRAM();  // ✅ Funktioniert!
}
```

### 2. Const Parameters (Input)

**Regel**: Verwende `const&` für Parameter, die nicht modifiziert werden und nicht trivial kopierbar sind.

```cpp
// ✅ RICHTIG - const reference für read-only
Result<void> putDataPoint(const DataPoint& point);
Status addNode(const BaseEntity& node);
bool contains(const std::string& key) const;

// ❌ FALSCH - Pass-by-value ohne Grund
Result<void> putDataPoint(DataPoint point);  // ⚠️ Unnötige Kopie!
bool contains(std::string key) const;        // ⚠️ Kopiert String!
```

**Ausnahmen**:
- Trivial kopierbare Typen: `int`, `bool`, `size_t`, `double`, Pointer
- Move-only Semantik gewünscht: `std::unique_ptr`, wenn Ownership übertragen wird

```cpp
// ✅ OK - trivial kopierbare Typen by value
bool allocate(size_t bytes);
void setTemperature(float celsius);

// ✅ OK - Move-Semantik für Ownership-Transfer
void setData(std::vector<float> data) {
    data_ = std::move(data);
}
```

### 3. Const Return Values

**Regel**: Verwende `const&` für Return-Values von großen internen Objekten, aber nur wenn sicher.

```cpp
// ✅ RICHTIG - const reference auf stabiles Objekt
class TSStore {
    Config config_;
public:
    const Config& getConfig() const { return config_; }  // Safe, keine Locks nötig
};

// ⚠️ VORSICHT - Bei Locks: Return by value!
class QueryCache {
    mutable std::mutex mutex_;
    Config config_;
public:
    // ✅ RICHTIG - Copy wegen Thread-Safety
    Config getConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;  // Muss kopieren, Lock wird released
    }
};
```

**Const/Non-const Overloads** für Container-Access:

```cpp
// ✅ RICHTIG - Beide Versionen
class Cache {
    std::map<std::string, Data> data_;
public:
    // Read-only access
    const Data* get(const std::string& key) const {
        auto it = data_.find(key);
        return (it != data_.end()) ? &it->second : nullptr;
    }
    
    // Mutable access
    Data* get(const std::string& key) {
        auto it = data_.find(key);
        return (it != data_.end()) ? &it->second : nullptr;
    }
};
```

### 4. Mutable für Caching

**Regel**: Verwende `mutable` für Cache-Variablen, die in `const`-Methoden modifiziert werden.

```cpp
// ✅ RICHTIG - mutable für Cache
class Database {
    mutable std::map<std::string, QueryResult> cache_;
    mutable std::mutex cache_mutex_;
public:
    QueryResult query(const std::string& sql) const {
        std::lock_guard lock(cache_mutex_);
        
        auto it = cache_.find(sql);
        if (it != cache_.end()) {
            return it->second;  // Cache hit
        }
        
        QueryResult result = executeQueryInternal(sql);
        cache_[sql] = result;  // ✅ OK, cache_ ist mutable
        return result;
    }
};
```

**Wann mutable verwenden?**
- Caching von berechneten Werten
- Mutex/Locks für Thread-Safety
- Logging/Statistics Counter
- **NICHT** für Geschäftslogik-State!

### 5. Const-Correctness in Vererbungshierarchien

**Regel**: Überschriebene Methoden müssen const-Kompatibilität wahren.

```cpp
// ✅ RICHTIG - const bleibt erhalten
class IFunction {
public:
    virtual nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx) const = 0;
};

class SinFunction : public IFunction {
public:
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx) const override {  // ✅ const!
        return std::sin(args[0].get<double>());
    }
};
```

## Best Practices aus ThemisDB

### Beispiel 1: BlobRedundancyManager

**Vorher**:
```cpp
std::vector<std::string> getDegradedBlobs();
std::vector<std::string> getCriticalBlobs();
BlobMetadata getBlobMetadata(const std::string& blob_id);
```

**Nachher**:
```cpp
std::vector<std::string> getDegradedBlobs() const;
std::vector<std::string> getCriticalBlobs() const;
BlobMetadata getBlobMetadata(const std::string& blob_id) const;
```

**Warum**: Diese Methoden lesen nur Daten (mit `shared_lock`), ändern aber nicht den Objektzustand.

### Beispiel 2: PropertyGraphManager

**Gut gemacht** ✅:
```cpp
std::pair<Status, BaseEntity> getNode(
    std::string_view pk,
    std::string_view graph_id = "default"
) const;  // ✅ const!

std::pair<Status, std::vector<EdgeInfo>> getOutgoingEdges(
    std::string_view fromPk,
    std::string_view graph_id = "default"
) const;  // ✅ const!
```

Diese Klasse hat bereits exzellente const-correctness!

### Beispiel 3: GPUMemoryManager

**Gut gemacht** ✅:
```cpp
size_t getTotalVRAM() const;
size_t getFreeVRAM() const;
Stats getStats() const;
bool canAllocate(size_t vram_bytes, size_t ram_bytes) const;
```

Alle Query-Methoden sind korrekt als `const` markiert.

## Code-Review Checkliste

Bei Code-Reviews prüfen:

- [ ] Sind alle Getter-Methoden `const`?
- [ ] Sind read-only Parameter als `const&` deklariert?
- [ ] Gibt es const/non-const Overloads wo sinnvoll?
- [ ] Ist `mutable` nur für Caching/Logging verwendet?
- [ ] Sind virtual functions const-konsistent?
- [ ] Werden unnötige Kopien vermieden?

## Häufige Fehler

### ❌ Const-cast verwenden

```cpp
// NIEMALS:
void badFunction(const Data& data) {
    Data& mutable_data = const_cast<Data&>(data);  // ⚠️ UB!
    mutable_data.modify();
}

// Stattdessen: API korrigieren
void goodFunction(Data& data) {
    data.modify();
}
```

### ❌ Const bei Return-by-Value

```cpp
// Nutzlos - verhindert move
const std::string badGetter() const {
    return name_;  // ⚠️ const verhindert RVO/move
}

// Gut - ermöglicht move
std::string goodGetter() const {
    return name_;  // ✅ Kann moved werden
}
```

### ❌ Mutable Missbrauch

```cpp
// BAD - mutable für Business-Logic
class Counter {
    mutable int count_ = 0;  // ⚠️ Missbrauch!
public:
    void increment() const {
        count_++;  // Ändert State in const method!
    }
};

// GOOD
class Counter {
    int count_ = 0;
public:
    void increment() {  // Nicht const!
        count_++;
    }
};
```

## Zusammenfassung

**Const-correctness in ThemisDB**:
- ✅ Großteils bereits sehr gut implementiert
- ✅ GPU Memory Manager: exzellent const-correct
- ✅ Property Graph Manager: exzellent const-correct
- ✅ Storage Layer: gute const-Nutzung
- ✅ Query System: const-correct mit Thread-Safety

**Verbesserungen umgesetzt**:
- BlobRedundancyManager: 4 Getter-Methoden → const
- WriteBatchWithIndexWrapper: 2 Get-Methoden → const
- GGUFLoader: 2 Helper-Methoden → const

**Fortsetzung**:
- Laufende Code-Reviews mit const-Checkliste
- Neue Klassen von Anfang an const-correct designen
- Dokumentation weiter ausbauen
