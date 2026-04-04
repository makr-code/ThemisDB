# Phase 2: StorageEngine Refactoring - Dependency Injection Implementation

## Übersicht

Diese Phase implementiert das Dependency Inversion Principle (DIP) für die StorageEngine, um zirkuläre Abhängigkeiten zwischen Storage, Query und Security Layern zu brechen.

## Was wurde geändert

### 1. Interface-Definitionen

Neue abstrakte Interfaces wurden in `include/themis/base/interfaces/` erstellt:

- **`query_interface.h`**: `IExpressionEvaluator` - Abstrahiert Query-Evaluierung
- **`security_interface.h`**: `IFieldEncryption`, `IKeyProvider` - Abstrahiert Verschlüsselung
- **`storage_interface.h`**: `IStorageEngine`, `IIndexManager` - Abstrahiert Storage-Operationen

### 2. StorageEngine mit Dependency Injection

**Datei**: `include/storage/storage_engine.h`

```cpp
class StorageEngine : public IStorageEngine {
public:
    // Constructor mit Dependency Injection
    StorageEngine(
        IExpressionEvaluatorPtr evaluator,
        IFieldEncryptionPtr encryption,
        IKeyProviderPtr key_provider,
        IIndexManagerPtr index_manager = nullptr
    );
    
    // Factory für Rückwärtskompatibilität
    static std::shared_ptr<StorageEngine> createDefault();
    
    // Verwendet injizierte Dependencies
    bool apply_filter(const std::string& filter_expr, const void* context);
    std::vector<uint8_t> encrypt_field(const std::string& field_name, 
                                        const std::vector<uint8_t>& plaintext);
};
```

### 3. Builder Pattern

**Datei**: `include/core/storage_initialization.h`

```cpp
class StorageEngineBuilder {
public:
    StorageEngineBuilder& withEvaluator(IExpressionEvaluatorPtr eval);
    StorageEngineBuilder& withEncryption(IFieldEncryptionPtr enc);
    StorageEngineBuilder& withKeyProvider(IKeyProviderPtr provider);
    StorageEngineBuilder& withIndexManager(IIndexManagerPtr index);
    
    std::shared_ptr<StorageEngine> build();
    static StorageEngineBuilder standard();
};
```

## Vorteile

### ✅ Testbarkeit
- Mock-Implementierungen können für Unit-Tests injiziert werden
- Keine Abhängigkeit von konkreten Implementierungen

### ✅ Flexibilität
- Implementierungen können zur Laufzeit ausgetauscht werden
- Keine Neukompilierung bei Änderungen

### ✅ Zirkuläre Dependencies gebrochen
- Storage kennt nur noch Interfaces, keine konkreten Implementierungen
- Query und Security sind entkoppelt

### ✅ Explizite Abhängigkeiten
- Dependencies sind klar im Constructor dokumentiert
- Keine versteckten Abhängigkeiten

## Migration Guide

### Vorher (Legacy)

```cpp
// Direkte Instanziierung mit hardcoded Dependencies
auto storage = std::make_shared<RocksDBWrapper>();
storage->initialize();
```

### Nachher (Empfohlen)

#### Option 1: Factory Method (Einfachste Migration)
```cpp
auto storage = StorageEngine::createDefault();
```

#### Option 2: Builder Pattern (Flexible Konfiguration)
```cpp
auto storage = StorageEngineBuilder::standard()
    .withEvaluator(custom_evaluator)
    .withEncryption(custom_encryption)
    .build();
```

#### Option 3: Direkte Injection (Volle Kontrolle)
```cpp
auto evaluator = std::make_shared<QueryEngine>();
auto encryption = std::make_shared<FieldEncryption>();
auto key_provider = std::make_shared<VaultKeyProvider>();

auto storage = std::make_shared<StorageEngine>(
    evaluator, encryption, key_provider
);
```

## Unit Testing

Beispiel mit Mock-Implementierungen:

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "storage/storage_engine.h"

class MockExpressionEvaluator : public IExpressionEvaluator {
public:
    MOCK_METHOD(bool, evaluate, (const std::string&, const void*), (override));
};

TEST(StorageEngineTest, UsesInjectedEvaluator) {
    auto mock_eval = std::make_shared<MockExpressionEvaluator>();
    auto mock_enc = std::make_shared<MockFieldEncryption>();
    auto mock_key = std::make_shared<MockKeyProvider>();
    
    EXPECT_CALL(*mock_eval, evaluate(_, _))
        .WillOnce(Return(true));
    
    auto storage = std::make_shared<StorageEngine>(
        mock_eval, mock_enc, mock_key
    );
    
    EXPECT_TRUE(storage->apply_filter("test", nullptr));
}
```

## Implementierungsdetails

### Default-Implementierungen

Die `createDefault()` Factory-Methode erstellt Standard-Implementierungen:

- **DefaultExpressionEvaluator**: No-op Evaluator (akzeptiert alle Filter)
- **DefaultFieldEncryption**: No-op Verschlüsselung (Passthrough)
- **DefaultKeyProvider**: Dummy-Schlüssel für Entwicklung
- **DefaultIndexManager**: No-op Index-Manager

Diese sind für Tests und Entwicklung gedacht. In Produktion sollten echte Implementierungen verwendet werden.

### Validierung

Der Constructor validiert required Dependencies:

```cpp
if (!evaluator_ || !encryption_ || !key_provider_) {
    throw std::invalid_argument("Required dependencies not provided");
}
```

`index_manager_` ist optional und wird nicht validiert.

## Nächste Schritte

1. **Real Implementations**: Echte Implementierungen der Interfaces für QueryEngine, FieldEncryption, etc.
2. **Server Integration**: Integration in den ThemisDB Server
3. **Performance Tests**: Benchmarks für DI-Overhead (sollte minimal sein)
4. **Additional Interfaces**: Weitere Interfaces für Transaction, Cache, etc.

## Dateien

### Neu erstellt
- `include/themis/base/interfaces/query_interface.h`
- `include/themis/base/interfaces/security_interface.h`
- `include/themis/base/interfaces/storage_interface.h`
- `include/storage/storage_engine.h`
- `src/storage/storage_engine.cpp`
- `include/core/storage_initialization.h`
- `tests/test_storage_engine_di.cpp`
- `docs/de/architecture/PHASE2_STORAGE_DI.md` (dieses Dokument)

### Geändert
- Keine bestehenden Dateien geändert (vollständig rückwärtskompatibel)

## Kompatibilität

Diese Änderungen sind **vollständig rückwärtskompatibel**:
- Bestehender Code funktioniert weiterhin
- Neue Funktionalität ist opt-in
- Keine Breaking Changes

## Referenzen

- **SOLID Principles**: Dependency Inversion Principle
- **Design Patterns**: Dependency Injection, Builder Pattern, Factory Pattern
- **Test Patterns**: Mock Objects, Dependency Injection for Testing
