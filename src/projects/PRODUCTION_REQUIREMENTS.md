---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

> **Status:** Anforderungen, keine vollständige Implementierungsbestätigung.
> Stand der Quellenprüfung: 2026-10-31.
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB Projects Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Projects-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Project-Template, Project-Diff, Project-Verwaltung.

## Dokumentabgrenzung (Canonical Split)

- **`src/projects/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/projects/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/projects/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/projects/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

Die folgenden MUST-Anforderungen sind Abnahmekriterien, keine Aussage über
bereits vorhandene Modulmechanismen. Der aktuelle Projects-Code stellt keine
allgemeine Authentifizierung, Deployment-Konfigurationsvalidierung oder
Autorisierungs-Middleware bereit. `shareProject()` und die Objekt-Locks
implementieren ihre jeweiligen Moduloperationen, aber keinen allgemeinen
Zugriffsschutz für beliebige Project-State-Schreibzugriffe. Dieses Modul
enthält außerdem keine produktive Bundle-Implementierung.

- **MUST:** Project-API mit Zugriffskontrolle; kein unauthentifizierter Schreibzugriff auf Project-State.
- **MUST:** Project-Templates werden vor Instanziierung validiert; Template-Injection-Vektoren werden geprüft.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
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
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/projects/PRODUCTION_REQUIREMENTS.md`
- `src/projects/project_template.cpp`
- `src/projects/project_diff.cpp`
