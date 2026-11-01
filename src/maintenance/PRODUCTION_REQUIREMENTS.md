---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Maintenance Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Maintenance-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Maintenance-Registry, Maintenance-Schedule-Store, Wartungsoperationen.

Die folgenden MUST-Aussagen sind normative Deployment-Anforderungen, keine
Behauptung, dass das Maintenance-Modul sie selbst implementiert oder beim Start
erzwingt. Die HTTP-Routen prüfen ihre Scopes in der Server-Schicht; direkte
In-Process-Aufrufe benötigen eine eigene Autorisierung.

## Dokumentabgrenzung (Canonical Split)

- **`src/maintenance/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/maintenance/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/maintenance/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/maintenance/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Maintenance-Operationen nur für autorisierte Operator-Rollen; kein unauthentifizierter Zugriff auf Wartungs-APIs.
- **MUST:** Maintenance-Scheduling mit definierten Zeitfenstern; unkontrollierte Wartungsoperationen in Produktions-Peak-Zeiten vermeiden.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte des Deployments beim Start validieren; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten. Im aktuellen Maintenance-Code ist kein entsprechender Konfigurationsloader nachgewiesen.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht automatisch als produktionssicher. Der Orchestrator stellt u. a. optionale Persistenz und einen unbegrenzten Tenant-Job-Limit-Default bereit.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus durch den Deployment-Eigner definiert und geprüft

> **TODO/Assumption:** Es ist in den Maintenance-Quellen kein Lesen oder Validieren
> von `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` nachgewiesen. Diese
> Umgebungsvariablen sind daher kein bestätigter Maintenance-Konfigurationsvertrag.

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/maintenance/PRODUCTION_REQUIREMENTS.md`
- `src/maintenance/maintenance_registry.cpp`
- `src/maintenance/maintenance_schedule_store.cpp`
