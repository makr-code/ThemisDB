---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Einordnung:** Dieses Dokument beschreibt vorgeschlagene Betriebsanforderungen,
> keine durchgehend im Code erzwungenen Garantien. Die Durchsetzung ist anhand
> der genannten APIs/Build-Definitionen nicht vollständig belegt; vor einer
> Produktionsfreigabe muss sie deployment-spezifisch geprüft werden.

# ThemisDB Base Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist ein Vorschlag für produktive Mindestanforderungen des
Base-Moduls. Es ist nicht die Verhaltensquelle für Loader, Sandbox oder Reload;
diese Verträge stehen in den Headern und der Implementierung.

## Dokumentabgrenzung (Canonical Split)

- **`src/base/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** vorgeschlagene Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen; Durchsetzung nicht pauschal verifiziert.
- **`src/base/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/base/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/base/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Vorgeschlagene Produktionsanforderungen (nicht als Code-Garantie belegt)

- **MUST:** Sandbox nach der jeweiligen Deployment-Policy konfigurieren. Die API macht Sandbox-Nutzung optional; eine universelle Aktivierung ist nicht belegt.
- **MUST:** Hot-Reload nur gemäß der Deployment-Policy zulassen. Eine allgemeine Freigabeliste ist in den geprüften Base-APIs nicht belegt.
- **MUST:** Sicherheitsrelevante Konfigurationen vor dem Betrieb prüfen; eine zentrale Startvalidierung aller hier genannten Werte ist nicht belegt.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Vorgeschlagene Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen im Deployment vorsehen; eine Base-eigene Audit-Logging-Garantie ist nicht belegt.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten (Backends, Registries, Koordinatoren) müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Deployment-Prüfliste (Status je Deployment zu ermitteln)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus/Umgebung über den tatsächlich unterstützten Deployment-Mechanismus gesetzt (`THEMIS_PRODUCTION_MODE` und `THEMIS_ENVIRONMENT` sind durch die geprüften Base-Quellen nicht bestätigt)

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/base/PRODUCTION_REQUIREMENTS.md`
- `src/themis/module_loader.cpp` (canonical loader implementation)
- `src/base/module_sandbox.cpp`
