> **Status:** 2026-06-01 – mit aktuellem Failover-Code (`auto_failover_manager.cpp`) abgeglichen.

# ThemisDB Failover Module - Production Requirements

<!-- Links: ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · README.md · ROADMAP.md -->

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Failover-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Auto-Failover-Manager, Disaster-Recovery-Manager.

## Dokumentabgrenzung (Canonical Split)

- **`src/failover/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/failover/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/failover/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/failover/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Failover-Trigger-Thresholds explizit konfiguriert; kein automatischer Failover ohne definierte Schwellwerte.
- **MUST:** Disaster-Recovery-Manager mit geprüftem Recovery-Playbook konfiguriert; Recovery-Prozeduren müssen getestet sein.
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

- `src/failover/PRODUCTION_REQUIREMENTS.md`
- `src/failover/auto_failover_manager.cpp`
- `src/failover/disaster_recovery_manager.cpp`
