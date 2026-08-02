> **Status:** 2026-08-02 – Evidence verified via issue #5650; build/test infrastructure confirmed; Phase 1-6 delivery validated.

# ThemisDB Importers Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Importers-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Adaptive-Import, Semantic-Matcher, Import-Koordination.

## Dokumentabgrenzung (Canonical Split)

- **`src/importers/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/importers/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/importers/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/importers/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Importer mit Input-Validierung und Source-Authentifizierung; nicht-autorisierte Importquellen werden abgewiesen.
- **MUST:** Import-Rate-Limits konfiguriert; kein unbegrenzter Bulk-Import-Throughput ohne definierte Grenzen.
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

- `src/importers/PRODUCTION_REQUIREMENTS.md`
- `src/importers/adaptive_import.cpp`
- `src/importers/semantic_matcher.cpp`
