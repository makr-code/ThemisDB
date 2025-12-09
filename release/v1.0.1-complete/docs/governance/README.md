# Governance Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Governance

---

## Übersicht

Das Governance-Modul implementiert Datenklassifizierung, Policy-Enforcement und Compliance-Kontrollen für ThemisDB.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| PolicyEngine | `policy_engine.h` | `policy_engine.cpp` | Policy Evaluation |

**Gesamt:** 1 Header, 1 Source-Datei, ~260 LOC

## Implementierte Klassen

### PolicyEngine

```cpp
class PolicyEngine {
    // Policy Evaluation Engine für Datenklassifizierung
    
    // Policy laden
    bool loadFromYAML(const std::string& yaml_path);
    
    // Audit Logger setzen
    void setAuditLogger(std::shared_ptr<AuditLogger> logger);
    
    // Policy evaluieren
    PolicyDecision evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route
    ) const;
    
    // Classification Profile abrufen
    std::optional<ClassificationProfile> getClassificationProfile(
        const std::string& level
    ) const;
    
    static bool isStrictClass(const std::string& cls);
};
```

### ClassificationProfile

```cpp
struct ClassificationProfile {
    std::string level;              // offen, vs-nfd, geheim, streng-geheim
    bool encryption_required;
    bool ann_allowed;               // Approximate NN erlaubt
    bool export_allowed;
    bool cache_allowed;
    std::string redaction_level;    // standard, strict
    int retention_days;
    bool log_encryption;
};
```

### PolicyDecision

```cpp
struct PolicyDecision {
    std::string classification;      // Normalisierte Klassifizierung
    std::string mode;                // enforce | observe
    bool encrypt_logs;
    std::string redaction;           // none | standard | strict
    
    // Abgeleitete Entscheidungen
    bool ann_allowed;
    bool require_content_encryption;
    bool export_allowed;
    bool cache_allowed;
    int retention_days;
};
```

## Classification Levels

| Level | Verschlüsselung | ANN | Export | Cache | Retention |
|-------|-----------------|-----|--------|-------|-----------|
| **offen** | Optional | ✅ | ✅ | ✅ | 365 Tage |
| **vs-nfd** | Required | ✅ | ⚠️ | ⚠️ | 730 Tage |
| **geheim** | Required | ❌ | ❌ | ❌ | 1825 Tage |
| **streng-geheim** | Required | ❌ | ❌ | ❌ | ∞ |

## Policy YAML Format

```yaml
classification_profiles:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: standard
    retention_days: 365

  vs-nfd:
    encryption_required: true
    ann_allowed: true
    export_allowed: false
    cache_allowed: false
    redaction_level: strict
    retention_days: 730

resource_mapping:
  "/vector/search": "offen"
  "/api/classified/*": "vs-nfd"

default_mode: enforce
```

## Beispiel

```cpp
PolicyEngine engine;
engine.loadFromYAML("policies.yaml");
engine.setAuditLogger(audit_logger);

// Policy evaluieren
auto decision = engine.evaluate(
    {{"X-Classification", "vs-nfd"}},
    "/api/documents"
);

if (decision.require_content_encryption) {
    // Content verschlüsseln
}

if (!decision.cache_allowed) {
    // Cache deaktivieren
}
```

## Verwandte Dokumentation

- [Security: RBAC](../security/security_rbac.md) - Access Control
- [Compliance: Overview](../compliance/compliance_overview.md) - Compliance-Anforderungen
- [Features: PII Detection](../features/features_pii_detection.md) - PII-Erkennung
| [governance_security.md](./governance_security.md) | DSGVO, Compliance | 📋 TODO |

## Verwandte Dokumentation

- [Security: PII API](../security/security_pii_api.md)
- [Security: Policies](../security/security_policies.md)
- [Policies: Data Classification](../policies/policies_data_classification.md)
