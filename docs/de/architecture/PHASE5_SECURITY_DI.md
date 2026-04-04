# Phase 5: SecurityLayer Refactoring - Dependency Injection Implementation

## Übersicht

Phase 5 vervollständigt die Dependency Inversion Principle (DIP) Refaktorierung durch die Umwandlung der SecurityLayer in eine reine, abhängigkeitsfreie Komponente, die IN andere Layer injiziert wird, nicht umgekehrt.

## Was wurde geändert

### 1. FieldEncryption implementiert IFieldEncryption Interface

**Datei**: `include/security/encryption.h`

```cpp
class FieldEncryption : public IFieldEncryption {
public:
    // Constructor mit KeyProvider Injection
    explicit FieldEncryption(std::shared_ptr<KeyProvider> key_provider);
    explicit FieldEncryption(IKeyProviderPtr key_provider);
    
    // Factory für Standard-Konfiguration
    static std::shared_ptr<FieldEncryption> createDefault();
    
    // IFieldEncryption Interface Implementierung
    std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext) override;
    
    std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext) override;
    
    bool should_encrypt(const std::string& field_name) const override;
    
    // Konfiguration
    void setEncryptionConfig(const EncryptionConfig& config);
};
```

### 2. EncryptionConfig für feldbasierte Verschlüsselung

```cpp
struct EncryptionConfig {
    /// Mapping von Feldnamen zu Key IDs
    std::unordered_map<std::string, std::string> field_key_mapping;
    
    /// Set von Feldern, die verschlüsselt werden sollen
    std::unordered_set<std::string> encrypted_fields;
    
    /// Standard Key ID für nicht explizit gemappte Felder
    std::string default_key_id = "default";
};
```

### 3. SecurityLayerBuilder - Zentraler Builder für Security-Komponenten

**Datei**: `include/core/security_initialization.h`

```cpp
class SecurityLayerBuilder {
public:
    enum class KeyProviderType {
        LOCAL,   // MockKeyProvider für Testing
        VAULT,   // HashiCorp Vault
        HSM      // Hardware Security Module
    };
    
    struct SecurityLayer {
        std::shared_ptr<IFieldEncryption> field_encryption;
        std::shared_ptr<security::RBAC> rbac;
        std::shared_ptr<auth::JWTValidator> jwt;
    };
    
    // Fluent API
    SecurityLayerBuilder& withKeyProvider(KeyProviderType type, 
                                          const std::string& config_json);
    SecurityLayerBuilder& withFieldEncryption(const EncryptionConfig& config);
    SecurityLayerBuilder& withRBACPolicy(const std::string& policy_file);
    SecurityLayerBuilder& withJWT(const std::string& cert_file,
                                  const std::vector<std::string>& allowed_issuers);
    
    SecurityLayer build();
    static SecurityLayerBuilder standard();
};
```

## Vorteile

### ✅ SecurityLayer hat KEINE Abhängigkeiten

- **FieldEncryption** hängt nur von `IKeyProvider` ab (Abstraktion)
- **RBAC** ist rein funktional (keine Dependencies zu Storage/Query/Index)
- **JWTValidator** ist zustandslos (selbstständige Validierung)

### ✅ Security wird IN andere Layer injiziert

```cpp
// Vorher: Storage kennt konkrete FieldEncryption
class StorageEngine {
    FieldEncryption encryption_;  // ❌ Tight coupling
};

// Nachher: Storage empfängt IFieldEncryption Interface
class StorageEngine {
    IFieldEncryptionPtr encryption_;  // ✅ Abstraktion
};
```

### ✅ Testbarkeit ohne Mocks

```cpp
TEST(SecurityTest, FieldEncryption) {
    auto mock_provider = std::make_shared<MockKeyProvider>();
    auto encryption = std::make_shared<FieldEncryption>(mock_provider);
    
    // Test ohne echte Vault/HSM Connection
    auto encrypted = encryption->encrypt_field("ssn", data);
}
```

### ✅ Alternative Implementierungen möglich

```cpp
// Development: Local Keys
auto layer = SecurityLayerBuilder()
    .withKeyProvider(KeyProviderType::LOCAL, "{}")
    .build();

// Production: Vault
auto layer = SecurityLayerBuilder()
    .withKeyProvider(KeyProviderType::VAULT, R"({
        "vault_addr": "https://vault.example.com",
        "vault_token": "s.token123",
        "kv_mount_path": "themis"
    })")
    .build();

// High-Security: HSM
auto layer = SecurityLayerBuilder()
    .withKeyProvider(KeyProviderType::HSM, R"({
        "library_path": "/usr/lib/libpkcs11.so",
        "slot_id": "0",
        "pin": "1234"
    })")
    .build();
```

## Verwendungsbeispiele

### Einfachste Nutzung: Standard-Builder

```cpp
auto layer = SecurityLayerBuilder::standard().build();

// Nutze Komponenten
layer.field_encryption->encrypt_field("ssn", data);
layer.rbac->checkPermission({"admin"}, "data", "write");
```

### Production-Setup mit vollständiger Konfiguration

```cpp
// 1. Encryption Config
EncryptionConfig enc_config;
enc_config.encrypted_fields.insert("ssn");
enc_config.encrypted_fields.insert("credit_card");
enc_config.field_key_mapping["ssn"] = "pii_key";
enc_config.field_key_mapping["credit_card"] = "payment_key";

// 2. Build Security Layer
auto layer = SecurityLayerBuilder()
    .withKeyProvider(
        SecurityLayerBuilder::KeyProviderType::VAULT,
        R"({
            "vault_addr": "https://vault.prod.example.com",
            "vault_token": "${VAULT_TOKEN}",
            "kv_mount_path": "themis"
        })"
    )
    .withFieldEncryption(enc_config)
    .withRBACPolicy("/etc/themis/rbac_policy.json")
    .withJWT("/etc/themis/jwt_cert.pem", {"https://auth.example.com"})
    .build();

// 3. Inject in andere Komponenten
auto storage = StorageEngineBuilder()
    .withFieldEncryption(layer.field_encryption)
    .build();
```

## Migration Guide

### Bestehender Code (Keine Änderungen notwendig!)

```cpp
// Diese Code funktioniert weiterhin:
auto key_provider = std::make_shared<MockKeyProvider>();
auto encryption = std::make_shared<FieldEncryption>(key_provider);
```

### Neuer Code (Empfohlen)

```cpp
// Nutze Builder Pattern:
auto layer = SecurityLayerBuilder()
    .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}")
    .build();

auto encryption = layer.field_encryption;
```

## Unit Testing

### Test mit Mock Key Provider

```cpp
#include <gtest/gtest.h>
#include "security/encryption.h"
#include "security/mock_key_provider.h"

TEST(FieldEncryptionTest, EncryptsWithMock) {
    auto mock_provider = std::make_shared<MockKeyProvider>();
    auto encryption = std::make_shared<FieldEncryption>(mock_provider);
    
    EncryptionConfig config;
    config.encrypted_fields.insert("ssn");
    encryption->setEncryptionConfig(config);
    
    std::vector<uint8_t> data = {1, 2, 3};
    auto encrypted = encryption->encrypt_field("ssn", data);
    
    EXPECT_FALSE(encrypted.empty());
    
    auto decrypted = encryption->decrypt_field("ssn", encrypted);
    EXPECT_EQ(data, decrypted);
}
```

### Test RBAC Policies

```cpp
TEST(RBACTest, AdminPermissions) {
    security::RBACConfig config;
    config.use_builtin_roles = true;
    auto rbac = std::make_shared<security::RBAC>(config);
    
    // Define admin role
    security::Role admin;
    admin.name = "admin";
    admin.permissions = {{"*", "*"}};  // All permissions
    rbac->addRole(admin);
    
    // Test
    EXPECT_TRUE(rbac->checkPermission({"admin"}, "data", "write"));
    EXPECT_TRUE(rbac->checkPermission({"admin"}, "keys", "rotate"));
}
```

### Integration Test

```cpp
TEST(SecurityIntegrationTest, FullLayerWorks) {
    auto layer = SecurityLayerBuilder::standard().build();
    
    // Test encryption
    std::vector<uint8_t> data = {1, 2, 3};
    auto encrypted = layer.field_encryption->encrypt_field("ssn", data);
    auto decrypted = layer.field_encryption->decrypt_field("ssn", encrypted);
    EXPECT_EQ(data, decrypted);
    
    // Test RBAC
    EXPECT_NE(layer.rbac, nullptr);
    
    // Test JWT
    EXPECT_NE(layer.jwt, nullptr);
}
```

## Implementierungsdetails

### KeyProvider Abstraktion

Alle KeyProvider implementieren `IKeyProvider`:

```cpp
class IKeyProvider {
public:
    virtual std::vector<uint8_t> get_key(const std::string& key_id) = 0;
    virtual std::vector<uint8_t> rotate_key(const std::string& key_id) = 0;
};
```

**Implementierungen:**
- `MockKeyProvider` - In-Memory für Testing
- `VaultKeyProvider` - HashiCorp Vault Integration
- `HSMKeyProviderAdapter` - PKCS#11 Hardware Security Module

### EncryptionConfig Logik

```cpp
bool FieldEncryption::should_encrypt(const std::string& field_name) const {
    // 1. Prüfe encrypted_fields Set
    if (!config_.encrypted_fields.empty()) {
        return config_.encrypted_fields.find(field_name) != 
               config_.encrypted_fields.end();
    }
    
    // 2. Prüfe field_key_mapping
    if (!config_.field_key_mapping.empty()) {
        return config_.field_key_mapping.find(field_name) != 
               config_.field_key_mapping.end();
    }
    
    // 3. Default: encrypt all
    return true;
}
```

## Finale Architektur

```
┌─────────────────────────────────────────────────┐
│         SecurityLayer (PURE, no deps)           │
│  ├── FieldEncryption → IKeyProvider             │
│  ├── RBACPolicy (standalone)                    │
│  └── JWTValidator (standalone)                  │
└─────────────────────────────────────────────────┘
         ↑ (injected INTO other layers)

┌─────────────────────────────────────────────────┐
│         StorageEngine                           │
│  ├── → IExpressionEvaluator                     │
│  ├── → IFieldEncryption                         │
│  ├── → IKeyProvider                             │
│  └── → IIndexManager                            │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│         QueryEngine                             │
│  ├── → IStorageEngine                           │
│  └── → IIndexManager                            │
│         → IExpressionEvaluator (provides)       │
└─────────────────────────────────────────────────┘
```

## Nächste Schritte

1. ✅ **Phase 5 Complete**: SecurityLayer ist rein und injizierbar
2. **Server Integration**: Integration in ThemisDB Server
3. **Performance Tests**: Benchmarks für DI-Overhead
4. **Production Rollout**: Migration zu Production-Vault/HSM

## Dateien

### Neu erstellt
- `include/core/security_initialization.h` - SecurityLayerBuilder
- `src/core/security_initialization.cpp` - Builder Implementierung
- `tests/test_security_di.cpp` - Unit Tests

### Geändert
- `include/security/encryption.h` - IFieldEncryption Interface, EncryptionConfig
- `src/security/field_encryption.cpp` - Interface-Methoden, Factory

## Kompatibilität

Diese Änderungen sind **vollständig rückwärtskompatibel**:
- Bestehender Code funktioniert weiterhin
- Neue Funktionalität ist opt-in
- Keine Breaking Changes

## Referenzen

- **SOLID Principles**: Dependency Inversion Principle
- **Design Patterns**: Dependency Injection, Builder Pattern, Factory Pattern
- **Previous Phases**: Phase 1 (Interfaces), Phase 2 (Plugin System), Phase 2.5 (Storage DI), Phase 3 (Query DI), Phase 4 (Index DI)
