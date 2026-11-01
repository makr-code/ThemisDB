---
Author: "ThemisDB Team"
Created: "2026-08-07"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_References: "src/timeseries/OPERATIONS.md; src/timeseries/TRUST.md; include/timeseries/tsstore.h; include/timeseries/retention.h; include/core/production_mode.h; include/security/hsm_security_checker.h; include/security/transport_security_checker.h"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Timeseries Module - Production Requirements

> The MUST/MUST NOT statements below are deployment requirements, not claims
> that the timeseries library enforces them. The repository-wide production
> mode is implemented by `themis::core::ProductionMode` in
> `include/core/production_mode.h` and is also read by the HSM and transport
> security checkers. These are generic core/security controls, not
> timeseries-specific initialization; verify that the consuming server invokes
> the relevant controls for its deployment.

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Timeseries-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Compression-Selector, Aggregate-Scheduler, Timeseries-Pipeline.

## Dokumentabgrenzung (Canonical Split)

- **`src/timeseries/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/timeseries/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/timeseries/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/timeseries/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.
- **`src/timeseries/PERFORMANCE_BASELINE.md`:** Benchmark source inventory and evidence limits; no current hardware baseline is asserted.
- **`src/timeseries/PHASE_6_ACCEPTANCE_CHECKLIST.md`:** Historical Phase 6 record and current verification limits.
- **`src/timeseries/OPERATOR_GUIDE.md`:** Deployment, configuration, tuning, monitoring, incident response.

## Verbindliche Produktionsanforderungen

- **MUST:** Timeseries-Retention-Policy konfiguriert; kein unbegrenztes Datenwachstum ohne Bereinigungslogik.
- **MUST:** Aggregate-Scheduling mit definierten Zeitfenstern; unkontrollierte Aggregate-Berechnung in Peak-Zeiten vermeiden.
- **MUST:** Deployment-spezifische sicherheitsrelevante Konfigurationen beim Start validieren; fehlende oder ungültige Werte führen gemäß Server-/Plattformvertrag zu Fail-Closed-Verhalten.
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
- [ ] Repository-weiten Produktionsmodus gemäß `include/core/production_mode.h` gesetzt und im Server-/Plattformpfad wirksam gemacht; das Timeseries-Modul initialisiert ihn nicht selbst

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/timeseries/PRODUCTION_REQUIREMENTS.md`
- `src/timeseries/compression_selector.cpp`
- `src/timeseries/aggregate_scheduler_helper.cpp`
