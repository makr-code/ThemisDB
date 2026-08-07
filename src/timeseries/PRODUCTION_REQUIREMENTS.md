> **Status:** 2026-08-07 – mit aktuellen Timeseries-Dokumenten (`PERFORMANCE_BASELINE.md`, `PHASE_6_ACCEPTANCE_CHECKLIST.md`, `OPERATOR_GUIDE.md`) verlinkt.

# ThemisDB Timeseries Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Timeseries-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Compression-Selector, Aggregate-Scheduler, Timeseries-Pipeline.

## Dokumentabgrenzung (Canonical Split)

- **`src/timeseries/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/timeseries/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/timeseries/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/timeseries/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.
- **`src/timeseries/PERFORMANCE_BASELINE.md`:** Release gate baselines, Regression detection, Performance characteristics.
- **`src/timeseries/PHASE_6_ACCEPTANCE_CHECKLIST.md`:** Phase 1–6 completion evidence, acceptance criteria, sign-off.
- **`src/timeseries/OPERATOR_GUIDE.md`:** Deployment, configuration, tuning, monitoring, incident response.

## Verbindliche Produktionsanforderungen

- **MUST:** Timeseries-Retention-Policy konfiguriert; kein unbegrenztes Datenwachstum ohne Bereinigungslogik.
- **MUST:** Aggregate-Scheduling mit definierten Zeitfenstern; unkontrollierte Aggregate-Berechnung in Peak-Zeiten vermeiden.
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

- `src/timeseries/PRODUCTION_REQUIREMENTS.md`
- `src/timeseries/compression_selector.cpp`
- `src/timeseries/aggregate_scheduler_helper.cpp`
