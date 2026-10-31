---
Author: "ThemisDB Contributors"
Created: "unknown"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> **Hinweis zum Nachweis:** Dieses Dokument formuliert normative Anforderungen,
> ist aber kein Beleg dafür, dass jeder Punkt im aktuellen Code erzwungen oder
> in einem Deployment umgesetzt wird. Die frühere Code-Abgleich-Angabe vom
> 2026-06-01 wurde in diesem Dokumentationsreview nicht erneut validiert.

> **AI-Provenienz:** AI_Used: true; AI_Models: unknown; AI_Contributor:
> GitHub Copilot; Human_Review: false; Human_Author: ThemisDB Contributors.
> Exact model backend not verifiable from final document text alone.

# ThemisDB Cdc Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Cdc-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Changefeed, CDC-WebSocket-Handler, CDC-Admin.

## Dokumentabgrenzung (Canonical Split)

- **`src/cdc/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/cdc/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/cdc/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/cdc/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** CDC-Consumer mit Autorisierung; kein unauthentifizierter Changefeed-Zugriff.
- **MUST:** Changefeed-Buffer-Limits explizit gesetzt; kein unbegrenztes Buffer-Wachstum.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten (Backends, Registries, Koordinatoren) müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/cdc/PRODUCTION_REQUIREMENTS.md`
- `src/cdc/changefeed.cpp`
- `src/cdc/cdc_ws_handler.cpp`
