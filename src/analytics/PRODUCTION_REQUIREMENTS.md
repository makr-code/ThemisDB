> **Status:** 2026-08-19 – mit aktuellem Analytics-Code (`analytics_export.cpp`, `model_serving.cpp`, `ml_serving.cpp`, `llm_process_analyzer.cpp`) abgeglichen.

# ThemisDB Analytics Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Analytics-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Analytics-Export, Arrow-Flight, CEP-Engine.

## Dokumentabgrenzung (Canonical Split)

- **`src/analytics/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/analytics/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/analytics/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/analytics/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Analytics-Export-Zugriffskontrolle aktiv.
- **MUST:** Arrow-Flight-Endpoint mit Auth-Middleware abgesichert.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST:** Modell-Importe aus externen Quellen mit Integritätsnachweis (mind. SHA-256) absichern; bei Integritätsmismatch Fail-Closed.
- **MUST:** Externes TF-Serving in Produktion über TLS (`https://`) betreiben; unverschlüsseltes HTTP nur in expliziten Nicht-Produktionsszenarien zulässig.
- **MUST:** LLM-Analyseantworten über task-spezifische Schema-/Typ-/Range-Validierung prüfen, bevor Inhalte in Entscheidungslogik übernommen werden.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.
- Integritäts- oder Transport-Policy-Verstöße müssen deterministisch und reproduzierbar als Fehler zurückgegeben werden.

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

- `src/analytics/PRODUCTION_REQUIREMENTS.md`
- `src/analytics/analytics_export.cpp`
- `src/analytics/arrow_flight.cpp`
