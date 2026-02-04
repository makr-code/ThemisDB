# GAP-004: Security & Governance - Implementierungsübersicht (Aktualisiert)

**Status:** ✅ Basisimplementierung abgeschlossen (ohne Duplikate)  
**Datum:** 4. Februar 2026  
**Version:** 2.0 (Dedupliziert)

---

## 🎯 Übersicht

Dieses Dokument beschreibt die finalisierte Implementierung von GAP-004: Security & Governance für ThemisDB. Nach Analyse vorhandener Strukturen wurden Duplikate entfernt und eine klare Integration mit bestehenden Systemen definiert.

---

## 📋 Implementierte Komponenten

### 1. PolicyManager & PolicyRule (NEU)

**Dateien:**
- `include/governance/policy_manager.h`
- `src/governance/policy_manager.cpp`
- `tests/test_policy_manager.cpp`

**Funktionalität:**
- `PolicyRule`: Struktur für einzelne Governance-Regeln
  - Ressourcen- und Aktionsmuster (mit Wildcard-Unterstützung: `data/*`, `*`)
  - Rollenbasierte Zugriffssteuerung
  - Verschlüsselungs- und Signaturanforderungen
  - Export- und Cache-Berechtigungen
  - Datenaufbewahrungsfristen (Retention)
  - Audit-Anforderungen
  - Prioritätsbasierte Regelauswahl
  
- `PolicyManager`: Verwaltung von Policy-Regeln
  - CRUD-Operationen für Regeln
  - Regelauswertung basierend auf Ressource, Aktion und Benutzerrollen
  - Multi-Regel-Aggregation (restriktivste Einstellungen gewinnen)
  - JSON-Persistierung
  - Validierung von Regelkonflikten
  - Statistiken

**Zweck:** Ergänzt PolicyEngine mit granularer RBAC-Funktionalität

**Beispiel:**
```cpp
PolicyManager manager;

PolicyRule rule;
rule.id = "rule_001";
rule.name = "Sensitive Data Protection";
rule.resources = {"data/sensitive/*"};
rule.actions = {"*"};
rule.required_roles = {"operator", "admin"};
rule.require_encryption = true;
rule.allow_export = false;
rule.retention_days = 90;
rule.priority = 100;

manager.addRule(rule);

auto decision = manager.evaluatePolicy(
    "data/sensitive/users", 
    "read", 
    {"operator"}
);

if (decision.require_encryption) {
    // Verschlüsselung erforderlich
}
```

---

## 🔗 Integration mit bestehenden Systemen

### Integration: PolicyManager + PolicyEngine + RBAC

ThemisDB hat bereits mehrere Governance- und Sicherheitssysteme. PolicyManager ergänzt diese:

**Existing Systems:**
1. **PolicyEngine** (`governance/policy_engine.h`) - VS-Klassifizierungsbasierte Policies
2. **RBAC** (`security/rbac.h`) - Rollenbasierte Zugriffskontrolle
3. **User/UserRoleStore** (`security/rbac.h`) - Benutzerverwaltung
4. **PKIKeyProvider** (`security/pki_key_provider.h`) - Produktions-PKI
5. **SigningService** (`security/signing.h`) - Signatur-Operationen

**Integration Example:**
```cpp
#include "governance/policy_manager.h"
#include "governance/policy_engine.h"
#include "security/rbac.h"
#include "security/pki_key_provider.h"
#include "security/signing.h"

// Setup components
PolicyEngine policy_engine;
policy_engine.loadFromYAML("governance_config.yaml");

PolicyManager policy_manager;
policy_manager.loadRules("rbac_rules.json");

RBAC rbac(rbac_config);
UserRoleStore user_store;

// PKI und Signatur (existierende Implementierungen)
auto pki_provider = std::make_shared<PKIKeyProvider>(
    cert_path, key_path, db, "themis-service"
);
auto signing_service = createKeyProviderSigningService(pki_provider);

// Evaluate access for a request
std::string user_id = "alice@example.com";
std::string resource = "data/sensitive/users";
std::string action = "read";

// 1. Get user roles from RBAC
auto user_roles = user_store.getUserRoles(user_id);

// 2. Check RBAC permissions
if (!rbac.checkPermission(user_roles, "data", action)) {
    // Access denied by RBAC
    return HTTP_403_FORBIDDEN;
}

// 3. Evaluate PolicyManager rules
auto pm_decision = policy_manager.evaluatePolicy(resource, action, user_roles);
if (!pm_decision.allowed) {
    // Access denied by PolicyManager rules
    return HTTP_403_FORBIDDEN;
}

// 4. Apply PolicyEngine classification policies
std::unordered_map<std::string, std::string> headers = {
    {"X-Classification", "geheim"}
};
auto pe_decision = policy_engine.evaluate(headers, "/api/data");

// 5. Combine decisions
bool require_encryption = pm_decision.require_encryption || 
                          pe_decision.require_content_encryption;
bool allow_export = pm_decision.allow_export && pe_decision.export_allowed;

// 6. Apply encryption if required
if (require_encryption) {
    // Use existing PKI infrastructure
    auto key = pki_provider->getKey("data-encryption-key");
    // ... encrypt data
}

// Access granted with applied policies
return HTTP_200_OK;
```

---

## 🚫 Entfernte Duplikate

Nach der Analyse wurden folgende Komponenten als Duplikate identifiziert und entfernt:

### 1. PKI-Stubs (ENTFERNT)

**Entfernte Dateien:**
- ~~`include/security/pki_stub.h`~~
- ~~`src/security/pki_stub.cpp`~~
- ~~`tests/test_pki_stub.cpp`~~

**Grund:** ThemisDB hat bereits produktionsreife PKI-Infrastruktur:
- **PKIKeyProvider** - 3-Tier Key Hierarchy mit VCC-PKI Integration
- **VCCPKIClient** - PKI-Client für Zertifikatsoperationen
- **PKIShardCertificate** - Shard-zu-Shard-Zertifikate

**Verwendung statt Stubs:**
```cpp
// Verwende existierenden PKIKeyProvider
auto pki = std::make_shared<PKIKeyProvider>(
    cert_path, private_key_path, db, "themis-service", true
);

// Zertifikatsoperationen via PKIKeyProvider
auto key = pki->getKey("encryption-key");
```

### 2. Signatur-Stubs (ENTFERNT)

**Entfernte Dateien:**
- ~~SignatureManager in `pki_stub.h/.cpp`~~

**Grund:** ThemisDB hat bereits umfassende Signatur-Infrastruktur:
- **SigningService** - Interface für Signaturoperationen
- **SigningProvider** - KeyProvider-integriertes Signing
- **CMSSigning** - CMS/PKCS#7 Signaturen
- **VaultSigningProvider** - HashiCorp Vault Integration

**Verwendung statt Stubs:**
```cpp
// Verwende existierenden SigningService
auto signing_svc = createKeyProviderSigningService(pki_provider);

// Signatur-Operationen
std::vector<uint8_t> data = {/* ... */};
auto sig_result = signing_svc->sign(data, "signing-key");

if (signing_svc->verify(data, sig_result.signature, "signing-key")) {
    // Signatur gültig
}
```

### 3. Profile & ProfileManager (ENTFERNT)

**Entfernte Komponenten:**
- ~~`Profile` struct~~
- ~~`ProfileManager` class~~
- ~~ProfileManager tests~~

**Grund:** Duplikat der existierenden `User` struct und `UserRoleStore`:
- **User** (`security/rbac.h`) - Hat bereits user_id, roles, attributes
- **UserRoleStore** - Verwaltung von Benutzern und Rollen

**Verwendung statt Profile:**
```cpp
// Verwende existierende RBAC-Strukturen
UserRoleStore user_store;

User user;
user.user_id = "alice@example.com";
user.roles = {"operator", "analyst"};
user.attributes = {
    {"classification_level", "vs-nfd"},
    {"department", "engineering"}
};

user_store.setUser(user);

// Rolle zuweisen
user_store.assignRole("alice@example.com", "admin");

// Rollen abrufen
auto roles = user_store.getUserRoles("alice@example.com");
```

---

## 🧪 Unit-Tests

**Test-Datei:** `tests/test_policy_manager.cpp`

**Test-Abdeckung (20 Tests):**
- PolicyManager:
  - ✅ Hinzufügen/Entfernen/Abrufen von Regeln
  - ✅ Regelauswahl basierend auf Ressourcen und Aktionen
  - ✅ Wildcard-Matching (`data/*`, `*`)
  - ✅ Rollenbasierte Filterung
  - ✅ Policy-Evaluation mit mehreren Regeln
  - ✅ Regel-Aggregation (OR für Requirements, AND für Permissions)
  - ✅ Persistierung (Speichern/Laden JSON)
  - ✅ Validierung
  - ✅ Statistiken
  - ✅ Import/Export

**Test-Ausführung:**
```bash
./build/themis_tests --gtest_filter="PolicyManager*"
```

---

## 📊 Zusammenfassung

### Neue Komponenten (Behalten)
1. ✅ **PolicyManager** - RBAC-Regel-Engine (komplementär zu PolicyEngine)
2. ✅ **PolicyRule** - Regelstruktur mit Patterns und Priorities

### Verwendung bestehender Systeme (Statt Duplikate)
1. ✅ **PKIKeyProvider** statt PKI-Stubs
2. ✅ **SigningService** statt Signatur-Stubs
3. ✅ **User/UserRoleStore** statt Profile/ProfileManager

### Code-Statistiken
- **PolicyManager:** ~370 Zeilen C++ (Header + Implementation)
- **Tests:** ~300 Zeilen (20 Tests)
- **Dokumentation:** ~400 Zeilen

### Entfernte Duplikate
- ~~PKI-Stubs:~~ 386 Zeilen entfernt
- ~~Signatur-Stubs:~~ (Teil von PKI-Stubs)
- ~~Profile/ProfileManager:~~ 210 Zeilen entfernt

**Netto-Code-Reduktion:** ~600 Zeilen durch Deduplizierung

---

## 🔗 Verwandte Dokumente

- [PolicyEngine Dokumentation](../../../include/governance/policy_engine.h)
- [RBAC Dokumentation](../../../include/security/rbac.h)
- [PKIKeyProvider Dokumentation](../../../include/security/pki_key_provider.h)
- [SigningService Dokumentation](../../../include/security/signing.h)
- [GAP-004 Roadmap](GAP_004_ROADMAP.md)
- [GAPS_STUBS_SUMMARY](../development/GAPS_STUBS_SUMMARY.md)

---

**Erstellt:** 4. Februar 2026  
**Aktualisiert:** 4. Februar 2026 (Deduplizierung)  
**Status:** ✅ Bereit für Integration
