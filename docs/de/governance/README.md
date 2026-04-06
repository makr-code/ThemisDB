# Governance Module

**Stand:** 6. April 2026  
**Version:** 2.0.0  
**Kategorie:** Governance

---

## Übersicht

Das Governance-Modul implementiert Policy-basierte Datenverwaltung, Datenklassifizierung, Compliance-Enforcement und Audit-Trail-Integration für ThemisDB. Es agiert als Policy Enforcement Point (PEP) zwischen der Server/API-Schicht und der Storage-Schicht und unterstützt GDPR, HIPAA, CCPA/CPRA, PCI-DSS, SOC 2 und ISO 27001.

## Strategische Dokumente

- **[CMS Strategy Paper](../strategie/CMS_STRATEGY_PAPER.md)** - Strategiepapier: ThemisDB für Content Management Systeme in Government und Enterprise
  - Multi-Model Architektur für CMS
  - Native AI/LLM Integration
  - Enterprise-Grade Sicherheit und Compliance
  - Wettbewerbsvergleich und TCO-Analyse
  - Implementierungs-Empfehlungen

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| PolicyEngine | `policy_engine.h` | `policy_engine.cpp` | Policy-Evaluation, YAML-Konfiguration, OPA-Integration, Simulation |
| PolicyManager | `policy_manager.h` | `policy_manager.cpp` | Policy-Lifecycle: laden, validieren, aktivieren, deaktivieren |
| PolicyManagerVersioned | `policy_manager_versioned.h` | `policy_manager_versioned.cpp` | Versionierte Policy-Verwaltung mit Rollback und Konflikt-Erkennung |
| PolicyCoordinator | `policy_coordinator.h` | `policy_coordinator.cpp` | Koordination der Policy-Evaluation über verteilte Nodes |
| PolicyValidator | `policy_validator.h` | `policy_validator.cpp` | Syntaktische und semantische Policy-Validierung |
| PolicyValidation | `policy_validation.h` | `policy_validation.cpp` | Konflikterkennung (widersprüchlich, überlappend, zirkulär) |
| PolicyTemplate | `policy_template.h` | `policy_template.cpp` | Eingebaute Policy-Templates (GDPR, HIPAA, SOC 2, Least-Privilege, zeitbasiert) |
| PolicyVersionHistory | `policy_version_history.h` | `policy_version_history.cpp` | Änderungshistorie und Rollback |
| PolicyFileWatcher | `policy_file_watcher.h` | `policy_file_watcher.cpp` | inotify/FSEvents-basiertes Hot-Reload ohne Neustart |
| PolicyReview | `policy_review.h` | `policy_review.cpp` | Review-Workflow: Entwurf → Prüfung → Freigabe → Aktivierung |
| ReviewScheduler | `review_scheduler.h` | `review_scheduler.cpp` | Geplante Policy-Review-Erinnerungen |
| ComplianceReporter | `compliance_reporter.h` | `compliance_reporter.cpp` | GDPR/HIPAA/CCPA/PCI-DSS/SOC 2 Compliance-Berichte |
| ComplianceReporting | `compliance_reporting.h` | `compliance_reporting.cpp` | Berichtserzeugung (PDF, JSON, HTML, CSV) |
| Soc2Controls | `soc2_controls.h` | `soc2_controls.cpp` | SOC 2 Trust Services Controls und Evidence-Sammlung (CC6.1, CC7.2, CC8.1, A1.1, C1.1, PI1.2) |
| CcpaRuleSet | `ccpa_rules.h` | `ccpa_rules.cpp` | CCPA/CPRA-Rechte-Evaluatoren (RightToKnow, RightToDelete, OptOutOfSale, DataPortability) |
| PciDssRules | `pci_dss_rules.h` | `pci_dss_rules.cpp` | PCI-DSS Datenisolierung und Compliance-Regeln |
| DataMasker | `data_masker.h` | `data_masker.cpp` | Feldbasiertes Daten-Masking (REDACT, TOKENIZE, TRUNCATE, HASH) |
| DataLineageTracker | `data_lineage.h` | `data_lineage.cpp` | Datenherkunfts-Tracking für verwaltete Datensätze |
| CrossTenantPolicyInheritance | `cross_tenant_policy_inheritance.h` | `cross_tenant_policy_inheritance.cpp` | Mandantenübergreifende Policy-Vererbung (most-restrictive-wins) |
| ModelGovernancePolicy | `model_governance.h` | `model_governance.cpp` | KI/ML-Modell-Governance, Bias-Auditierung, Training-Datenherkunft |
| OpaAdapter | `opa_adapter.h` | `opa_adapter.cpp` | Open Policy Agent Integration für Rego-basierte Policy-Evaluation |

**Gesamt:** 21 Header, 21 Source-Dateien (+ README, ARCHITECTURE, ROADMAP, FUTURE_ENHANCEMENTS)

## Implementierte Klassen

### PolicyEngine

```cpp
class PolicyEngine {
    // Policy laden
    bool loadFromYAML(const std::string& yaml_path);

    // Audit Logger und OPA-Evaluator setzen
    void setAuditLogger(std::shared_ptr<AuditLogger> logger);
    void setOpaEvaluator(IPolicyEvaluator* evaluator);

    // Policy evaluieren
    PolicyDecision evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route
    ) const;

    // Query-Berechtigung prüfen (inkl. FieldMaskingPolicy)
    QueryPermissionResult checkQueryPermission(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route
    ) const;

    // Trockentest / Simulation (kein Audit-Eintrag)
    SimulationResult simulateDecision(const SimulationRequest& request) const;

    // CCPA Opt-Out-Subjects setzen
    void setCcpaOptOutSubjects(const std::unordered_set<std::string>& subjects);

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
    bool ann_allowed;
    bool require_content_encryption;
    bool export_allowed;
    bool cache_allowed;
    int retention_days;
    bool ccpa_opted_out;             // CCPA Opt-Out-Flag
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

data_masking:
  - field: "email"
    strategy: TOKENIZE
  - field: "ssn"
    strategy: REDACT

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

// Query-Berechtigung mit Daten-Masking prüfen
auto result = engine.checkQueryPermission(
    {{"X-User-Id", "user42"}, {"X-Classification", "offen"}},
    "/api/search"
);
DataMasker masker;
auto masked_docs = masker.maskFieldsArray(raw_docs, result.masking_policy);

// Trockenlauf / Simulation (ohne Audit-Eintrag)
SimulationRequest sim_req;
sim_req.headers = {{"X-Classification", "geheim"}};
sim_req.route = "/api/export";
auto sim = engine.simulateDecision(sim_req);
// sim.decision.export_allowed == false
```

## Verwandte Dokumentation

- [Primärdokumentation: src/governance/README.md](../../../src/governance/README.md) — Modulübersicht und Entwicklerleitfaden
- [Architektur: src/governance/ARCHITECTURE.md](../../../src/governance/ARCHITECTURE.md) — Architektur-Leitfaden mit Komponentendiagramm
- [Roadmap: src/governance/ROADMAP.md](../../../src/governance/ROADMAP.md) — Entwicklungs-Roadmap und Produktionsreife-Checkliste
- [Zukünftige Enhancements: src/governance/FUTURE_ENHANCEMENTS.md](../../../src/governance/FUTURE_ENHANCEMENTS.md) — Geplante Features mit Performance-Zielen
- [Header-API: include/governance/README.md](../../../include/governance/README.md) — Öffentliche API-Header-Übersicht
- [Security: RBAC](../security/security_rbac.md) — Access Control
- [Compliance: Overview](../compliance/compliance_overview.md) — Compliance-Anforderungen
- [Features: PII Detection](../features/features_pii_detection.md) — PII-Erkennung
- [Security: PII API](../security/security_pii_api.md)
- [Security: Policies](../security/security_policies.md)
- [Policies: Data Classification](../policies/policies_data_classification.md)

## Wissenschaftliche Referenzen

[1] European Parliament and Council, "General Data Protection Regulation (GDPR)," *Official Journal of the European Union*, L 119, Mai 2016. https://eur-lex.europa.eu/eli/reg/2016/679/oj

[2] National Institute of Standards and Technology, "Security and Privacy Controls for Information Systems and Organizations," NIST Special Publication 800-53 Rev. 5, Sep. 2020. https://doi.org/10.6028/NIST.SP.800-53r5

[3] R. S. Sandhu, E. J. Coyne, H. L. Feinstein, und C. E. Youman, "Role-Based Access Control Models," *IEEE Computer*, Bd. 29, Nr. 2, S. 38–47, Feb. 1996. https://doi.org/10.1109/2.485845

[4] T. Moses, Hrsg., "eXtensible Access Control Markup Language (XACML) Version 3.0," OASIS Standard, Jan. 2013. https://docs.oasis-open.org/xacml/3.0/xacml-3.0-core-spec-os-en.html

[5] California Legislature, "California Consumer Privacy Act (CCPA)," California Civil Code §1798.100 et seq., 2018. https://oag.ca.gov/privacy/ccpa

[6] Payment Card Industry Security Standards Council, "PCI DSS v4.0," Mär. 2022. https://www.pcisecuritystandards.org/document_library/

[7] AICPA, "SOC 2 — Trust Services Criteria," 2017. https://www.aicpa.org/resources/download/2017-trust-services-criteria

[8] S. Abiteboul, R. Hull, und V. Vianu, *Foundations of Databases*. Addison-Wesley, 1995. https://webdam.inria.fr/Alice/

