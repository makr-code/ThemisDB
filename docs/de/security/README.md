# Security Module

**Stand:** 15. Dezember 2025  
**Version:** 2.0 - Mit Vector Encryption (Phase 1 + 2)  
**Kategorie:** Security

---

## 🆕 Vector Encryption (Phase 1 + 2) - VOLLSTÄNDIG IMPLEMENTIERT ✅

**Schnellstart:**
- **[QUICK_START_VECTOR_ENCRYPTION.md](QUICK_START_VECTOR_ENCRYPTION.md)** - 5-Minuten Schnelleinstieg (EN)

**Benutzerhandbücher:**
- **[VECTOR_ENCRYPTION_CONFIGURATION.md](VECTOR_ENCRYPTION_CONFIGURATION.md)** - Phase 1: Vektor-Verschlüsselung in RocksDB (EN)
- **[HNSW_ENCRYPTION_CONFIGURATION.md](HNSW_ENCRYPTION_CONFIGURATION.md)** - Phase 2: HNSW Index Verschlüsselung (EN)

**Implementierungsdetails:**
- **[COMPLETE_IMPLEMENTATION_SUMMARY.md](COMPLETE_IMPLEMENTATION_SUMMARY.md)** - Vollständige Übersicht aller Phasen (EN)
- **[PHASE1_FINAL_REPORT.md](PHASE1_FINAL_REPORT.md)** - Phase 1 Abschlussbericht (EN)
- **[PHASE2_IMPLEMENTATION_REPORT.md](PHASE2_IMPLEMENTATION_REPORT.md)** - Phase 2 Abschlussbericht (EN)

**Build & Test:**
- **[BUILD_VERIFICATION_GUIDE.md](BUILD_VERIFICATION_GUIDE.md)** - Build und Test Anleitung (EN)

**Performance & Optimierung:**
- **[PERFORMANCE_OPTIMIZATION_NOTES.md](PERFORMANCE_OPTIMIZATION_NOTES.md)** - Performance-Optimierungen (EN)

**Analysen:**
- **[HNSW_PERSISTENCE_ENCRYPTION_ANALYSIS.md](HNSW_PERSISTENCE_ENCRYPTION_ANALYSIS.md)** - Sicherheitsanalyse HNSW Persistenz
- **[EMBEDDING_REVERSIBILITY_ANALYSIS.md](EMBEDDING_REVERSIBILITY_ANALYSIS.md)** - Vektor-Embedding Sicherheitsanalyse
- **[ENCRYPTED_HNSW_SEARCHABILITY.md](ENCRYPTED_HNSW_SEARCHABILITY.md)** - Analyse verschlüsselte Suche

**Ergebnis:**
- ✅ 100% At-Rest Verschlüsselung für Vektoren
- ✅ AES-256-GCM für RocksDB Vektoren und HNSW Index-Dateien
- ✅ BSI C5 CRY-03 vollständig konform
- ✅ 8 Integrationstests + 5 Beispiele
- ✅ Migrations-Tool für bestehende Daten

---

## BSI C5 Compliance - Kryptographie

**[➡️ BSI C5 Column Encryption Compliance Report](BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md)**  
Comprehensive analysis of column encryption implementation against BSI C5 requirements (CRY-01 to CRY-06).  
**Compliance Score: 95% → 100% (with new documentation)** ✅

**[➡️ BSI C5 Multi-Model Encryption Analysis](BSI_C5_MULTI_MODEL_ENCRYPTION_ANALYSIS.md)** ⭐ **NEU**  
Detaillierte Analyse der Verschlüsselung über **alle Datenmodell-Schichten**: Relational, Vector, Graph, Geo, Timeline, Process.  
**Ergebnis: Unified Storage Architecture sichert konsistente Verschlüsselung über alle Modelle** ✅

**Formale Dokumentation (Dezember 2025):**
- **[Kryptographie-Policy](CRYPTOGRAPHY_POLICY.md)** - Formale Policy gemäß BSI C5 CRY-01, BSI TR-02102-1 konform
- **[Key Lifecycle Management](KEY_LIFECYCLE_MANAGEMENT.md)** - Vollständiger Schlüssel-Lebenszyklus gemäß BSI C5 CRY-02
- **[Executive Summary (DE)](BSI_C5_ZUSAMMENFASSUNG.md)** - Kurzzusammenfassung für Stakeholder

---

## Übersicht

Das Security-Modul implementiert Field-Level Encryption, Key Management, RBAC, PKI und Malware-Scanning für ThemisDB.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| FieldEncryption | `encryption.h` | `encryption.cpp` | AES-256-GCM |
| KeyProvider | `key_provider.h` | - | Key Provider Interface |
| MockKeyProvider | `mock_key_provider.h` | `mock_key_provider.cpp` | Test Provider |
| VaultKeyProvider | `vault_key_provider.h` | `vault_key_provider.cpp` | HashiCorp Vault |
| HSMProvider | `hsm_provider.h` | `hsm_provider_pkcs11.cpp` | PKCS#11 HSM |
| PKIKeyProvider | `pki_key_provider.h` | `pki_key_provider.cpp` | PKI Integration |
| RBAC | `rbac.h` | `rbac.cpp` | Role-Based Access |
| MalwareScanner | `malware_scanner.h` | `malware_scanner.cpp` | Content Scanning |
| CMSSigning | `cms_signing.h` | `cms_signing.cpp` | CMS Signatures |
| TimestampAuthority | `timestamp_authority.h` | `timestamp_authority.cpp` | RFC 3161 TSA |

**Gesamt:** 16 Header, 16 Source-Dateien, ~8,100 LOC

## Implementierte Klassen

### FieldEncryption (AES-256-GCM)

```cpp
struct EncryptedBlob {
    std::string key_id;           // Logical key identifier
    uint32_t key_version;         // Key version for rotation
    std::vector<uint8_t> iv;      // 12 bytes (GCM standard)
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;     // 16 bytes (auth tag)
    
    std::string toBase64() const;
    static EncryptedBlob fromBase64(const std::string& b64);
};

class FieldEncryption {
    EncryptedBlob encrypt(const std::string& plaintext, const std::string& key_id);
    std::string decrypt(const EncryptedBlob& blob);
    bool rotateKey(const std::string& key_id, uint32_t new_version);
};
```

### KeyProvider Interface

```cpp
class IKeyProvider {
    virtual std::vector<uint8_t> getKey(const std::string& key_id) = 0;
    virtual std::vector<uint8_t> getKeyVersion(const std::string& key_id, uint32_t version) = 0;
    virtual uint32_t getCurrentVersion(const std::string& key_id) = 0;
    virtual bool rotateKey(const std::string& key_id) = 0;
};
```

### Key Providers

| Provider | Beschreibung | Use Case |
|----------|--------------|----------|
| **MockKeyProvider** | Deterministische Keys | Testing |
| **VaultKeyProvider** | HashiCorp Vault KMS | Production |
| **HSMProvider** | PKCS#11 Hardware Module | High Security |
| **PKIKeyProvider** | PKI/Certificate-based | Enterprise |

### RBAC (Role-Based Access Control)

```cpp
class RBAC {
    struct Permission {
        std::string resource;
        std::string action;  // read, write, delete, admin
    };
    
    struct Role {
        std::string name;
        std::vector<Permission> permissions;
    };
    
    bool authorize(const std::string& user, const std::string& resource, const std::string& action);
    void assignRole(const std::string& user, const std::string& role);
    void revokeRole(const std::string& user, const std::string& role);
};
```

### MalwareScanner

```cpp
class IMalwareScanner {
    virtual bool scan(const std::vector<uint8_t>& content) = 0;
    virtual std::string getLastThreat() = 0;
};

class MalwareFilterManager {
    void registerScanner(std::unique_ptr<IMalwareScanner> scanner);
    bool scanContent(const std::vector<uint8_t>& content);
};
```

### CMSSigning (RFC 5652)

```cpp
class CMSSigning {
    std::vector<uint8_t> sign(const std::vector<uint8_t>& data, 
                               const std::string& cert_path,
                               const std::string& key_path);
    bool verify(const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& data,
                const std::string& ca_cert_path);
};
```

### TimestampAuthority (RFC 3161)

```cpp
class TimestampAuthority {
    std::vector<uint8_t> createTimestamp(const std::vector<uint8_t>& data);
    bool verifyTimestamp(const std::vector<uint8_t>& tsr,
                         const std::vector<uint8_t>& data);
};
```

## Verschlüsselungs-Format

```
{key_id}:{version}:{base64(iv)}:{base64(ciphertext)}:{base64(tag)}
```

Beispiel:
```
user_pii:2:YWJjZGVmZ2hpams=:SGVsbG8gV29ybGQ=:MTIzNDU2Nzg5MEFCQ0RFRg==
```

## Verwandte Dokumentation

- [security_overview.md](security_overview.md) - Sicherheitsübersicht
- [security_encryption_strategy.md](security_encryption_strategy.md) - Verschlüsselungsstrategie
- [security_key_management.md](security_key_management.md) - Key Management
- [security_hsm.md](security_hsm.md) - HSM Integration
- [security_pki_architecture.md](security_pki_architecture.md) - PKI Architektur
