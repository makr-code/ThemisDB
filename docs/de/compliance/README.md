# Compliance Documentation

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Compliance

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Implementierungsstatus](#implementierungsstatus)
- [Source-Code Unterstützung](#source-code-unterstützung)
- [Dokumentation](#dokumentation-in-diesem-ordner)

## Übersicht

ThemisDB erfüllt umfassende Compliance-Anforderungen für den Einsatz in regulierten Umgebungen (BSI C5, ISO 27001, DSGVO, eIDAS, SOC 2).

## Implementierungsstatus

| Framework | Status | Dokumentation |
|-----------|--------|---------------|
| BSI C5 | ✅ Ready | [compliance_full_checklist.md](compliance_full_checklist.md) |
| ISO 27001 | ✅ Ready | [compliance_full_checklist.md](compliance_full_checklist.md) |
| DSGVO | ✅ Ready | [compliance_dpia.md](compliance_dpia.md) |
| eIDAS | ✅ Ready | [compliance_full_checklist.md](compliance_full_checklist.md) |
| SOC 2 | ✅ Ready | [compliance_full_checklist.md](compliance_full_checklist.md) |
| NIS2 | ✅ Ready | [compliance_bcp_drp.md](compliance_bcp_drp.md) |

## Source-Code Unterstützung

Die Compliance-Anforderungen werden durch folgende Module im Source-Code unterstützt:

| Anforderung | Source-Code | Header |
|-------------|-------------|--------|
| Audit Logging | `src/utils/audit_logger.cpp` | `include/utils/audit_logger.h` |
| Field Encryption | `src/security/encryption.cpp` | `include/security/encryption.h` |
| Key Management | `src/security/vault_key_provider.cpp` | `include/security/key_provider.h` |
| RBAC | `src/security/rbac.cpp` | `include/security/rbac.h` |
| PII Detection | `src/utils/pii_detector.cpp` | `include/utils/pii_detector.h` |
| Retention | `src/utils/retention_manager.cpp` | `include/utils/retention_manager.h` |
| TSA (RFC 3161) | `src/security/timestamp_authority.cpp` | `include/security/timestamp_authority.h` |
| CMS Signing | `src/security/cms_signing.cpp` | `include/security/cms_signing.h` |

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [compliance_dashboard.md](compliance_dashboard.md) | Executive Compliance Summary |
| [compliance_full_checklist.md](compliance_full_checklist.md) | Vollständige Audit-Checkliste |
| [compliance_dpia.md](compliance_dpia.md) | DSGVO Datenschutz-Folgenabschätzung |
| [compliance_bcp_drp.md](compliance_bcp_drp.md) | Business Continuity & Disaster Recovery |
| [compliance_risk_register.md](compliance_risk_register.md) | Risiko-Register |
| [compliance_vendor_assessment.md](compliance_vendor_assessment.md) | Vendor Assessment |
| [compliance_audit_todo.md](compliance_audit_todo.md) | Offene Audit-Punkte |

## Verwandte Dokumentation

- [Security Overview](../security/security_overview.md) - Sicherheitsübersicht
- [Security Module](../security/README.md) - Security Implementation
- [Features: Audit Logging](../features/features_audit_logging.md) - Audit-Feature
- [Features: Compliance](../features/features_compliance.md) - Compliance-Features
