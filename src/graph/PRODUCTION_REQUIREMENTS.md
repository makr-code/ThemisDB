---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - SECURITY.md
  - TRUST.md
  - ../../include/graph/
  - ../../include/server/graph_api_handler.h
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

> **Review status:** This is a deployment integration checklist, not proof that the graph module
> implements the listed controls. Authorization, tenant isolation, startup validation, audit
> logging, and environment policy must be verified by the owning service/deployment layer.

# ThemisDB Graph Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Graph-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Graph-Path-Constraints, Query-Explain-Plan, Graph-Analytics.

## Dokumentabgrenzung (Canonical Split)

- **`src/graph/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/graph/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/graph/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/graph/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Produktionsanforderungen zur Integration (nicht automatisch durch das Graph-Modul erzwungen)

- **TODO / Caller:** Use explicit traversal bounds where the selected API exposes them; defaults,
  enforcement, and available limits differ by operation.
- **TODO / Service owner:** Graph-Query-Autorisierung and Tenant-Namespace-Isolation must be
  verified at the server/API boundary; these controls are not established by graph constraints.
- **TODO / Deployment owner:** Verify startup configuration validation and fail-closed behavior
  in the owning deployment layer.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Production-mode setting confirmed by deployment owner (**TODO:** no graph source/configuration reference for the former environment-variable names was found.)

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/graph/PRODUCTION_REQUIREMENTS.md`
- `src/graph/path_constraints.cpp`
- `src/graph/explain_plan.cpp`
